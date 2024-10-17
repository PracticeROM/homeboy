#include <string.h>

#include "frame.h"
#include "hooks.h"
#include "rom.h"
#include "types.h"
#include "vc.h"

extern u8 cpuExecuteCall_hook_addr_ha[];
extern u8 cpuExecuteCall_hook_addr_l[];
extern u8 frameSetDepth_hook_addr[];

#if IS_GC
extern u8 videoForceRetrace[];
extern u8 romSetCacheSize_hook_addr[];
extern u8 romLoadRange_hook_addr_1[];
extern u8 romLoadRange_hook_addr_2[];

// mcardWrite calls from simulatorWriteXXXX
extern u8 mcardWrite_bl_flash[];
extern u8 mcardWrite_bl_sram[];
extern u8 mcardWrite_bl_eeprom[];
#endif

// Change an instruction in memory.
void patch_instruction(void* addr, u32 value) {
    *(volatile u32*)addr = value;
    DCStoreRange((u32*)addr, sizeof(u32));
    ICInvalidateRange((u32*)addr, sizeof(u32));
}

// Patch the address of an @ha relocation.
void patch_ha(void* addr, void* target) {
    u32 inst = *(volatile u32*)addr;
    u32 lo;
    u32 hi;

    lo = (u32)target & 0xFFFF;
    if (lo & 0x8000) {
        hi = ((u32)target + 0x10000) >> 16;
    } else {
        hi = (u32)target >> 16;
    }

    patch_instruction(addr, (inst & 0xFFFF0000) | (hi & 0xFFFF));
}

// Patch the address of an @l relocation.
void patch_l(void* addr, void* target) {
    u32 inst = *(volatile u32*)addr;
    u32 lo = (u32)target & 0xFFFF;

    patch_instruction(addr, (inst & 0xFFFF0000) | (lo & 0xFFFF));
}

// Change a function call in memory to call a different function instead.
void patch_bl(void* addr, void* target) {
    patch_instruction(addr, 0x48000001 | (((u8*)target - (u8*)addr) & 0x03FFFFFC));
}

// Replaces cpuExecuteCall, patches VC crashes by moving the call to cpuExecuteUpdate earlier.
// See https://pastebin.com/V6ANmXt8
static s32 cpuExecuteCall_hook(Cpu* pCPU, s32 nCount, s32 nAddressN64, s32 nAddressGCN) {
    s32 nReg;
    s32 count;
    s32* anCode;
    s32 saveGCN;
    CpuFunction* node;
    CpuCallerID* block;
    s32 nDeltaAddress;
    s32 nAddressGCNCall;

#if IS_OOT
    nCount = OSGetTick();
#elif IS_MM
    s64 nTime = OSGetTime();
#endif

    if (pCPU->nWaitPC != 0) {
        pCPU->nMode |= 8;
    } else {
        pCPU->nMode &= ~8;
    }

    pCPU->nMode |= 4;
    pCPU->nPC = nAddressN64;

    pCPU->aGPR[31].s32 = nAddressGCN;
    saveGCN = nAddressGCN - 4;

    pCPU->survivalTimer++;

    cpuFindFunction(pCPU, pCPU->nReturnAddrLast - 8, &node);

#if IS_OOT
    if (!cpuExecuteUpdate(pCPU, &nAddressGCNCall, nCount)) {
        return false;
    }
#elif IS_MM
    if (!cpuExecuteUpdate(pCPU, &nAddressGCNCall, nTime)) {
        return false;
    }
#endif

    block = node->block;
    for (count = 0; count < node->callerID_total; count++) {
        if (block[count].N64address == nAddressN64 && block[count].GCNaddress == 0) {
            block[count].GCNaddress = saveGCN;
            break;
        }
    }

    saveGCN = (ganMapGPR[31] & 0x100) ? true : false;
    anCode = (s32*)nAddressGCN - (saveGCN ? 4 : 3);
    if (saveGCN) {
        anCode[0] = 0x3CA00000 | ((u32)nAddressGCN >> 16);
        anCode[1] = 0x60A50000 | ((u32)nAddressGCN & 0xFFFF);
        DCStoreRange(anCode, 8);
        ICInvalidateRange(anCode, 8);
    } else {
        nReg = ganMapGPR[31];
        anCode[0] = 0x3C000000 | ((u32)nAddressGCN >> 16) | (nReg << 21);
        anCode[1] = 0x60000000 | ((u32)nAddressGCN & 0xFFFF) | (nReg << 21) | (nReg << 16);
        DCStoreRange(anCode, 8);
        ICInvalidateRange(anCode, 8);
    }

    nDeltaAddress = (u8*)nAddressGCNCall - (u8*)&anCode[3];
    if (saveGCN) {
        anCode[3] = 0x48000000 | (nDeltaAddress & 0x03FFFFFC);
        DCStoreRange(anCode, 16);
        ICInvalidateRange(anCode, 16);
    } else {
        anCode[2] = 0x48000000 | (nDeltaAddress & 0x03FFFFFC);
        DCStoreRange(anCode, 12);
        ICInvalidateRange(anCode, 12);
    }

#if IS_OOT
    pCPU->nTickLast = OSGetTick();
#elif IS_MM
    pCPU->nTimeLast = nTime;
#endif

    return nAddressGCNCall;
}

// Replaces frameSetDepth, correctly converts N64 depth value to GC/Wii depth value.
// In frameDrawSetup2D (used for gSPFillRectangle), the GC/Wii projection matrix is
// set up so that the near plane is at 0 (z=0) and the far plane is at 1001 (z=-1001).
// Meanwhile, the depth value passed to frameSetDepth is in the range [0, 1], so any
// depth other than 0 is outside the GC/Wii view frustrum. This doesn't affect anything
// in-game, but we fix it here for gz.
bool frameSetDepth_hook(Frame* pFrame, f32 rDepth, f32 rDelta) {
    pFrame->rDepth = rDepth * -1001.0f;
    pFrame->rDelta = rDelta;
    return true;
}

#if IS_GC

bool romSetCacheSize_hook(Rom* pROM, s32 nSize) {
    s32 nSizeCacheRAM = 0x400000; // 4 MiB

    pROM->nSizeCacheRAM = nSizeCacheRAM;
    pROM->nCountBlockRAM = nSizeCacheRAM / 0x2000;

    if (!xlHeapTake(&pROM->pBuffer, nSizeCacheRAM | 0x30000000)) {
        return false;
    }

    pROM->pCacheRAM = (u8*)pROM->pBuffer;
    return true;
}

bool mcardWrite_hook(MemCard* pMCard, s32 address, s32 size, char* data) {
    // if it's a gz save, run the non-OoT logic of mcardWrite (copy-pasted here)
    // else, call the real mcardWrite to keep the original behavior for a game save
    if (address == 0x7A00) {
        memcpy(&pMCard->file.game.buffer[address], data, size);

        if (pMCard->saveToggle == true) {
            simulatorRumbleStop(0);
            if (!mcardUpdate()) {
                return false;
            }
        } else {
            pMCard->saveToggle = true;
            pMCard->wait = false;
            mcardOpenDuringGame(pMCard);
            if (pMCard->saveToggle == true) {
                if (!mcardUpdate()) {
                    return false;
                }
            }
        }
    } else {
        mcardWrite(pMCard, address, size, data);
    }
}

#endif

void init_hooks(void) {
    // Replace reference to cpuExecuteCall in cpuExecute
    patch_ha(cpuExecuteCall_hook_addr_ha, cpuExecuteCall_hook);
    patch_l(cpuExecuteCall_hook_addr_l, cpuExecuteCall_hook);

    // Replace call to frameSetDepth in rdbParseGBI
    patch_bl(frameSetDepth_hook_addr, frameSetDepth_hook);

#if IS_GC
    // Patches videoForceRetrace so that DMA can proceed even if N64 VI registers
    // or not yet initialized (required for gz to load).
    // https://decomp.me/scratch/MgT6o
    patch_instruction(videoForceRetrace + 0x24, 0x41820010); // beq- +0x10
    patch_instruction(videoForceRetrace + 0x30, 0x40820024); // bne- +0x24

    // Patches call to romSetCacheSize in systemSetupGameRAM
    patch_bl(romSetCacheSize_hook_addr, romSetCacheSize_hook);

    // Patches romLoadRange size to load less ROM permanently into cache. This will
    // load to about the end of objects, skipping place names and skyboxes.
    patch_instruction(romLoadRange_hook_addr_1, 0x3CA0012D); // lis r5, 0x12D
    patch_instruction(romLoadRange_hook_addr_2, 0x3CA0012D); // lis r5, 0x12D

    // Hook to mcardWrite to fix gz settings not saving
    patch_bl(mcardWrite_bl_flash, mcardWrite_hook);
    patch_bl(mcardWrite_bl_sram, mcardWrite_hook);
    patch_bl(mcardWrite_bl_eeprom, mcardWrite_hook);
#endif
}
