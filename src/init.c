#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fs.h"
#include "hb_debug.h"
#include "hb_exception.h"
#include "hb_fat.h"
#include "hb_heap.h"
#include "hb_n64vcmem.h"
#include "homeboy.h"
#include "hooks.h"
#include "rom.h"
#include "sys.h"
#include "types.h"
#include "vc.h"

#define HB_HEAPSIZE 0xD000

/**
 * @brief Fix `gnFlagZelda` not being set properly on GameCube versions.
 *
 * It checks the rom's filename and set the flag to 1 if it's an MQ rom, otherwise the value used will be 2.
 * The emulator checks the bits of the flag's value, if the bit 2 is set (from `gnFlagZelda & 2`), it will use non-MQ
 * mode.
 */
static void patch_gnFlagZelda(void) {
#if IS_GC
    Rom* pROM = SYSTEM_ROM(gpSystem);

    // MQ roms are named "urazlj_f.n64", checking the first 3 characters will tell if the game used is MQ or not.
    if (pROM->acNameFile[0] == 'u' && pROM->acNameFile[1] == 'r' && pROM->acNameFile[2] == 'a') {
        gnFlagZelda = 0;
    } else {
        gnFlagZelda = 2;
    }
#endif
}

INIT bool _start(Ram* pRAM, s32 nSize) {
    if (!ramSetSize(pRAM, 0x00800000)) {
        return false;
    }

    patch_gnFlagZelda();

    n64_dram = pRAM->pBuffer;

    init_hooks();
    homeboy_init();
#ifdef HB_HEAP
    homeboy_heap_init();
#endif
#ifdef HB_FAT
    homeboy_fat_init();
#endif
#ifdef HB_DBG
    homeboy_debug_init();
#endif
#ifdef HB_N64VCMEM
    homeboy_n64vcmem_init();
#endif
#ifdef HB_EXCEPTIONS
    init_hb_exceptions();
#endif

#if IS_WII
    if (hb_hid < 0) {
        hb_hid = iosCreateHeap((void*)ios_heap_addr, HB_HEAPSIZE);
    }

    if (hb_hid >= 0) {
        homeboy_obj->key = 0x1234;
    }

    fs_init();

    char title_id_str[9];
    for (int i = 0; i < 8; i++) {
        int digit = (title_id >> (28 - i * 4)) & 0xF;
        title_id_str[i] = digit < 10 ? '0' + digit : 'a' + digit - 10;
    }
    title_id_str[8] = '\0';

    strcat(dram_fn, "/title/00010001/");
    strcat(dram_fn, title_id_str);
    strcat(dram_fn, "/data/dram_save");

    // Check if a dram restore needs to be done.
    int fd = fs_open(dram_fn, 1);
    if (fd >= 0) {
        uint32_t dram_params[2];
        fs_read(fd, dram_params, sizeof(dram_params));
        fs_read(fd, (char*)n64_dram + dram_params[0], dram_params[1]);
        fs_close(fd);
        fs_delete(dram_fn);
        homeboy_obj->dram_restore_key = 0x6864;
    }
#endif

    return true;
}
