#include "fs.h"
#include "hb_debug.h"
#include "hb_exception.h"
#include "hb_fat.h"
#include "hb_heap.h"
#include "hb_n64vcmem.h"
#include "homeboy.h"
#include "sys.h"
#include "vc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define HB_HEAPSIZE 0xD000

INIT bool _start(void** dest, size_t size) {
    if (!ramSetSize(dest, 0x00800000)) {
        return 0;
    }

    n64_dram = dest[1];

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

    return 1;
}
