#include "hooks.h"
#include "vc.h"

extern uint8_t vc_crash_hook_addr_ha[];
extern uint8_t vc_crash_hook_addr_l[];

// Change an instruction in memory.
void patch_instruction(void* addr, uint32_t value) {
    *(volatile uint32_t*)addr = value;
    DCStoreRange((uint32_t*)addr, sizeof(uint32_t));
    ICInvalidateRange((uint32_t*)addr, sizeof(uint32_t));
}

// Patch the address of an @ha relocation.
void patch_ha(void* addr, void* target) {
    uint32_t inst = *(volatile uint32_t*)addr;
    uint32_t lo;
    uint32_t hi;

    lo = (uint32_t)target & 0xFFFF;
    if (lo & 0x8000) {
        hi = ((uint32_t)target + 0x10000) >> 16;
    } else {
        hi = (uint32_t)target >> 16;
    }

    patch_instruction(addr, (inst & 0xFFFF0000) | (hi & 0xFFFF));
}

// Patch the address of an @l relocation.
void patch_l(void* addr, void* target) {
    uint32_t inst = *(volatile uint32_t*)addr;
    uint32_t lo = (uint32_t)target & 0xFFFF;

    patch_instruction(addr, (inst & 0xFFFF0000) | (lo & 0xFFFF));
}

// Change a function call in memory to call a different function instead.
void patch_bl(void* addr, void* target) {
    patch_instruction(addr, 0x48000001 | (((uint8_t*)target - (uint8_t*)addr) & 0x03FFFFFC));
}

// Replaces cpuExecuteCall, patches VC crashes by moving the call to cpuExecuteUpdate earlier.
// See https://pastebin.com/V6ANmXt8
static int32_t vc_crash_hook(Cpu* pCPU, int32_t nCount, int32_t nAddressN64, int32_t nAddressGCN) {
    int32_t nReg;
    int32_t count;
    int32_t* anCode;
    int32_t saveGCN;
    CpuFunction* node;
    CpuCallerID* block;
    int32_t nDeltaAddress;
    int32_t nAddressGCNCall;

#if IS_OOT
    nCount = OSGetTick();
#elif IS_MM
    int64_t nTime = OSGetTime();
#endif

    if (pCPU->nWaitPC != 0) {
        pCPU->nMode |= 8;
    } else {
        pCPU->nMode &= ~8;
    }

    pCPU->nMode |= 4;
    pCPU->nPC = nAddressN64;

    pCPU->aGPR[31].int32_t = nAddressGCN;
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
    anCode = (int32_t*)nAddressGCN - (saveGCN ? 4 : 3);
    if (saveGCN) {
        anCode[0] = 0x3CA00000 | ((uint32_t)nAddressGCN >> 16);
        anCode[1] = 0x60A50000 | ((uint32_t)nAddressGCN & 0xFFFF);
        DCStoreRange(anCode, 8);
        ICInvalidateRange(anCode, 8);
    } else {
        nReg = ganMapGPR[31];
        anCode[0] = 0x3C000000 | ((uint32_t)nAddressGCN >> 16) | (nReg << 21);
        anCode[1] = 0x60000000 | ((uint32_t)nAddressGCN & 0xFFFF) | (nReg << 21) | (nReg << 16);
        DCStoreRange(anCode, 8);
        ICInvalidateRange(anCode, 8);
    }

    nDeltaAddress = (uint8_t*)nAddressGCNCall - (uint8_t*)&anCode[3];
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
