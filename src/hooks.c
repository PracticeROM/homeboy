#include "hooks.h"
#include "types.h"
#include "vc.h"

extern u8 vc_crash_hook_addr_ha[];
extern u8 vc_crash_hook_addr_l[];

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
static s32 vc_crash_hook(Cpu* pCPU, s32 nCount, s32 nAddressN64, s32 nAddressGCN) {
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

void init_hooks(void) {
    // Replace reference to cpuExecuteCall in cpuExecute
    patch_ha(vc_crash_hook_addr_ha, vc_crash_hook);
    patch_l(vc_crash_hook_addr_l, vc_crash_hook);
}
