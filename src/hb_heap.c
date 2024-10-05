#include <stdbool.h>

#include "hb_heap.h"

class_hb_heap_t* hb_heap_obj = NULL;
int hb_heap_event(void* heap_p, int event, void* arg);

#if HB_HEAP
static _XL_OBJECTTYPE hb_heap_class = {
    "HB-HEAP",
    sizeof(class_hb_heap_t),
    0,
    hb_heap_event,
};

static bool heap_get8(void* callback, uint32_t addr, uint8_t* dest) {
    if (hb_heap_obj != NULL && hb_heap_obj->heap_ptr != NULL) {
        addr -= 0x100C0000;
        *dest = *(uint8_t*)(hb_heap_obj->heap_ptr + addr);
        return true;
    }
    return false;
}

static bool heap_get16(void* callback, uint32_t addr, uint16_t* dest) {
    if (hb_heap_obj != NULL && hb_heap_obj->heap_ptr != NULL) {
        addr -= 0x100C0000;
        *dest = *(uint16_t*)(hb_heap_obj->heap_ptr + addr);
        return true;
    }
    return false;
}

static bool heap_get32(void* callback, uint32_t addr, uint32_t* dest) {
    if (hb_heap_obj != NULL && hb_heap_obj->heap_ptr != NULL) {
        addr -= 0x100C0000;
        *dest = *(uint32_t*)(hb_heap_obj->heap_ptr + addr);
        return true;
    }
    return false;
}

static bool heap_get64(void* callback, uint32_t addr, uint64_t* dest) {
    if (hb_heap_obj != NULL && hb_heap_obj->heap_ptr != NULL) {
        addr -= 0x100C0000;
        *dest = *(uint64_t*)(hb_heap_obj->heap_ptr + addr);
        return true;
    }
    return false;
}

static bool heap_put8(void* callback, uint32_t addr, uint8_t* src) {
    if (hb_heap_obj != NULL && hb_heap_obj->heap_ptr != NULL) {
        addr -= 0x100C0000;
        *(uint8_t*)(hb_heap_obj->heap_ptr + addr) = *src;
        return true;
    }
    return false;
}

static bool heap_put16(void* callback, uint32_t addr, uint16_t* src) {
    if (hb_heap_obj != NULL && hb_heap_obj->heap_ptr != NULL) {
        addr -= 0x100C0000;
        *(uint16_t*)(hb_heap_obj->heap_ptr + addr) = *src;
        return true;
    }
    return false;
}

static bool heap_put32(void* callback, uint32_t addr, uint32_t* src) {
    if (hb_heap_obj != NULL && hb_heap_obj->heap_ptr != NULL) {
        addr -= 0x100C0000;
        *(uint32_t*)(hb_heap_obj->heap_ptr + addr) = *src;
        return true;
    }
    return false;
}

static bool heap_put64(void* callback, uint32_t addr, uint64_t* src) {
    if (hb_heap_obj != NULL && hb_heap_obj->heap_ptr != NULL) {
        addr -= 0x100C0000;
        *(uint64_t*)(hb_heap_obj->heap_ptr + addr) = *src;
        return true;
    }
    return false;
}

int hb_heap_event(void* heap_p, int event, void* arg) {
    class_hb_heap_t* heap = (class_hb_heap_t*)heap_p;
    // device mapped.
    if (event == 0x1002) {
        if (heap->heap_ptr == NULL) {
            allocMEM2(&heap->heap_ptr, 0x800000);
            heap->heap_size = 0x00800000;
        }
        cpuSetDevicePut(SYSTEM_CPU(gpSystem), arg, heap_put8, heap_put16, heap_put32, heap_put64);
        cpuSetDeviceGet(SYSTEM_CPU(gpSystem), arg, heap_get8, heap_get16, heap_get32, heap_get64);
    }
}

void* hb_frame_buffer[2];

void homeboy_heap_init(void) {
    xlObjectMake((void**)&hb_heap_obj, NULL, &hb_heap_class);
    cpuMapObject(SYSTEM_CPU(gpSystem), hb_heap_obj, 0x08060000, 0x0885FFFF, 0);
    allocMEM2(&hb_frame_buffer[0], 0x25800);
    allocMEM2(&hb_frame_buffer[1], 0x25800);
}

#endif
