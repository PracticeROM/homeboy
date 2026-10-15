#ifndef _SD_GC_INTERNAL_H
#define _SD_GC_INTERNAL_H

#include "types.h"

#define SD_GC_EXI_WRITE 1
#define SD_GC_EXI_READ  2

bool sd_gc_exi_present(s32 channel);
bool sd_gc_exi_begin(s32 channel, u32 frequency, bool select_device);
bool sd_gc_exi_end(s32 channel);
bool sd_gc_exi_transfer(s32 channel, u8* data, u32 length, u32 mode);

#endif