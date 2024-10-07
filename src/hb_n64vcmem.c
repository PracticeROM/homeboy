#include <stddef.h>

#include "hb_n64vcmem.h"
#include "homeboy.h"
#include "types.h"

#if HB_N64VCMEM
static void* hb_n64vcmem_dummy = NULL;
int hb_n64vcmem_event(void* dummy_p, int event, void* arg);

static _XL_OBJECTTYPE hb_n64vcmem_class = {
    "HB-N64VCMEM",
    sizeof(void*),
    0,
    hb_n64vcmem_event,
};

static bool n64vcmem_get8(void* callback, uint32_t addr, uint8_t* dest) {
    addr -= 0x110C0000;
    *dest = *(uint8_t*)((uint32_t)n64_dram + addr);
    return true;
}

static bool n64vcmem_get16(void* callback, uint32_t addr, uint16_t* dest) {
    addr -= 0x110C0000;
    *dest = *(uint16_t*)((uint32_t)n64_dram + addr);
    return true;
}

static bool n64vcmem_get32(void* callback, uint32_t addr, uint32_t* dest) {
    addr -= 0x110C0000;
    *dest = *(uint32_t*)((uint32_t)n64_dram + addr);
    return true;
}

static bool n64vcmem_get64(void* callback, uint32_t addr, uint64_t* dest) {
    addr -= 0x110C0000;
    *dest = *(uint64_t*)((uint32_t)n64_dram + addr);
    return true;
}

static bool n64vcmem_put8(void* callback, uint32_t addr, uint8_t* src) {
    addr -= 0x110C0000;
    *(uint8_t*)((uint32_t)n64_dram + addr) = *src;
    return true;
}

static bool n64vcmem_put16(void* callback, uint32_t addr, uint16_t* src) {
    addr -= 0x110C0000;
    *(uint16_t*)((uint32_t)n64_dram + addr) = *src;
    return true;
}

static bool n64vcmem_put32(void* callback, uint32_t addr, uint32_t* src) {
    addr -= 0x110C0000;
    *(uint32_t*)((uint32_t)n64_dram + addr) = *src;
    return true;
}

static bool n64vcmem_put64(void* callback, uint32_t addr, uint64_t* src) {
    addr -= 0x110C0000;
    *(uint64_t*)((uint32_t)n64_dram + addr) = *src;
    return true;
}

int hb_n64vcmem_event(void* dummy_p, int event, void* arg) {
    if (event == 0x1002) {
        cpuSetDevicePut(SYSTEM_CPU(gpSystem), arg, n64vcmem_put8, n64vcmem_put16, n64vcmem_put32, n64vcmem_put64);
        cpuSetDeviceGet(SYSTEM_CPU(gpSystem), arg, n64vcmem_get8, n64vcmem_get16, n64vcmem_get32, n64vcmem_get64);
    }
}

void homeboy_n64vcmem_init(void) {
    xlObjectMake((void**)&hb_n64vcmem_dummy, NULL, &hb_n64vcmem_class);
    cpuMapObject(SYSTEM_CPU(gpSystem), hb_n64vcmem_dummy, 0x8860000, 0x905FFFF, 0);
}

#endif
