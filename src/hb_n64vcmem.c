#include <stdbool.h>
#include <stddef.h>
#include "hb_n64vcmem.h"
#include "homeboy.h"

#if HB_N64VCMEM
static void *hb_n64vcmem_dummy = NULL;
int hb_n64vcmem_event(void *dummy_p, int event, void *arg);


static class_type_t hb_n64vcmem_class = {
    "HB-N64VCMEM",
    sizeof(void*),
    0,
    hb_n64vcmem_event
};

static bool n64vcmem_lb(void *callback, uint32_t addr, uint8_t *dest){
    addr -= 0x110C0000;
    *dest = *(uint8_t*)((uint32_t)n64_dram + addr);
    return true;
}

static bool n64vcmem_lh(void *callback, uint32_t addr, uint16_t *dest){
    addr -= 0x110C0000;
    *dest = *(uint16_t*)((uint32_t)n64_dram + addr);
    return true;
}

static bool n64vcmem_lw(void *callback, uint32_t addr, uint32_t *dest){
    addr -= 0x110C0000;
    *dest = *(uint32_t*)((uint32_t)n64_dram + addr);
    return true;
}

static bool n64vcmem_ld(void *callback, uint32_t addr, uint64_t *dest){
    addr -= 0x110C0000;
    *dest = *(uint64_t*)((uint32_t)n64_dram + addr);
    return true;
}

static bool n64vcmem_sb(void *callback, uint32_t addr, uint8_t *src){
    addr -= 0x110C0000;
    *(uint8_t*)((uint32_t)n64_dram + addr) = *src;
    return true;
}

static bool n64vcmem_sh(void *callback, uint32_t addr, uint16_t *src){
    addr -= 0x110C0000;
    *(uint16_t*)((uint32_t)n64_dram + addr) = *src;
    return true;
}

static bool n64vcmem_sw(void *callback, uint32_t addr, uint32_t *src){
    addr -= 0x110C0000;
    *(uint32_t*)((uint32_t)n64_dram + addr) = *src;
    return true;
}

static bool n64vcmem_sd(void *callback, uint32_t addr, uint64_t *src){
    addr -= 0x110C0000;
    *(uint64_t*)((uint32_t)n64_dram + addr) = *src;
    return true;
}

int hb_n64vcmem_event(void *dummy_p, int event, void *arg){
    if(event == 0x1002){
        cpuSetDevicePut(gSystem->cpu, arg, n64vcmem_sb, n64vcmem_sh, n64vcmem_sw, n64vcmem_sd);
        cpuSetDeviceGet(gSystem->cpu, arg, n64vcmem_lb, n64vcmem_lh, n64vcmem_lw, n64vcmem_ld);
    }
}

void homeboy_n64vcmem_init(void){
    xlObjectMake((void**)&hb_n64vcmem_dummy, NULL, &hb_n64vcmem_class);
    cpuMapObject(gSystem->cpu, hb_n64vcmem_dummy, 0x8860000, 0x905FFFF, 0);
}

#endif
