#include "hb_debug.h"
#include "hb_heap.h"
#include "homeboy.h"
#include "sys.h"
#include "vc.h"
#include <sys/stat.h>

#ifdef HB_DBG

enum dbg_cmd {
    DBG_DUMP_MEM
};

typedef struct {
    uint32_t dbg_cmd;
} hb_dbg_class_t;

static hb_dbg_class_t* hb_dbg_obj = NULL;

int hb_debug_event(void* hb_dbg_p, int event, void* arg);

static _XL_OBJECTTYPE hb_dbg_class = {"HB-DBG", sizeof(hb_dbg_class_t), 0, hb_debug_event};

static void run_cmd(void) {
    switch (hb_dbg_obj->dbg_cmd) {
        case DBG_DUMP_MEM: {
            int file = creat("/kz_mem1_dump.bin", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            write(file, (void*)0x80000000, 0x1800000);
            close(file);
            file = creat("/kz_hb_heap_dump.bin", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            write(file, hb_heap_obj->heap_ptr, hb_heap_obj->heap_size);
            close(file);
        } break;
    }
}

static bool get8(hb_dbg_class_t* hb_fat, uint32_t addr, uint8_t* dst) { return false; }

static bool get16(hb_dbg_class_t* hb_fat, uint32_t addr, uint16_t* dst) { return false; }

static bool get32(hb_dbg_class_t* hb_fat, uint32_t addr, uint32_t* dst) { return true; }

static bool get64(hb_dbg_class_t* hb_fat, uint32_t addr, uint64_t* dst) { return false; }

static bool put8(hb_dbg_class_t* hb_fat, uint32_t addr, uint8_t* src) { return false; }

static bool put16(hb_dbg_class_t* hb_fat, uint32_t addr, uint16_t* src) { return false; }

static bool put32(hb_dbg_class_t* hb_fat, uint32_t addr, uint32_t* src) {
    addr &= 0x7FFF;
    if (addr == 0) {
        hb_dbg_obj->dbg_cmd = *src;
        run_cmd();
    }
    return true;
}

static bool put64(hb_dbg_class_t* hb_fat, uint32_t addr, uint64_t* src) { return false; }

int hb_debug_event(void* hb_dbg_p, int event, void* arg) {
    if (event == 0x1002) {
        cpuSetDevicePut(SYSTEM_CPU(gpSystem), arg, put8, put16, put32, put64);
        cpuSetDeviceGet(SYSTEM_CPU(gpSystem), arg, get8, get16, get32, get64);
    }
}

void homeboy_debug_init(void) {
    xlObjectMake((void**)&hb_dbg_obj, NULL, &hb_dbg_class);
    cpuMapObject(SYSTEM_CPU(gpSystem), hb_dbg_obj, 0x805C000, 0x805FFFF, 0);
}

#endif
