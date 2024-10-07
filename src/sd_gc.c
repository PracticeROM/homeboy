#include "sd.h"
#include "version.h"

#if IS_GC

bool sdio_is_initialized(void) {
    return false;
}

bool sdio_is_inserted(void) {
    return false;
}

bool sdio_is_sdhc(void) {
    return false;
}

bool sdio_write_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer) {
    return false;
}

bool sdio_read_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer) {
    return false;
}

bool sdio_stop(void) {
    return false;
}

bool sdio_start(void) {
    return false;
}

#endif
