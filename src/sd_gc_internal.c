#include <stddef.h>

#include "sd_gc_internal.h"

#define EXI_REGS                ((volatile u32*)0xCC006800)
#define EXI_REG(channel, index) EXI_REGS[(channel) * 5 + (index)]
#define EXI_SPIN_LIMIT          0x10000

bool sd_gc_exi_present(s32 channel) {
    u32 status;

    if (channel < 0 || channel > 1) {
        return false;
    }

    status = EXI_REG(channel, 0);
    return (status & 0x1000) != 0;
}

bool sd_gc_exi_begin(s32 channel, u32 frequency, bool select_device) {
    u32 status;

    if (channel < 0 || channel > 1 || frequency > 5 || !sd_gc_exi_present(channel)) {
        return false;
    }

    status = EXI_REG(channel, 0) & 0x405;
    status |= frequency << 4;
    if (select_device) {
        status |= 0x80;
    }
    EXI_REG(channel, 0) = status;
    return true;
}

bool sd_gc_exi_end(s32 channel) {
    if (channel < 0 || channel > 1) {
        return false;
    }

    EXI_REG(channel, 0) &= ~0x80;
    return true;
}

bool sd_gc_exi_transfer(s32 channel, u8* data, u32 length, u32 mode) {
    if (channel < 0 || channel > 1 || data == NULL || (mode != SD_GC_EXI_WRITE && mode != SD_GC_EXI_READ)) {
        return false;
    }

    while (length > 0) {
        u32 chunk = length < 4 ? length : 4;
        u32 value = 0;
        u32 tries = EXI_SPIN_LIMIT;

        for (u32 i = 0; i < chunk; i++) {
            if (mode == SD_GC_EXI_READ) {
                data[i] = 0xFF;
            }
            value |= (u32)data[i] << ((3 - i) * 8);
        }
        EXI_REG(channel, 4) = value;

        EXI_REG(channel, 3) = (mode << 2) | 1 | ((chunk - 1) << 4);
        while (tries > 0 && (EXI_REG(channel, 3) & 1) != 0) {
            tries--;
        }
        if ((EXI_REG(channel, 3) & 1) != 0) {
            return false;
        }

        if (mode == SD_GC_EXI_READ) {
            value = EXI_REG(channel, 4);
            for (u32 i = 0; i < chunk; i++) {
                data[i] = (u8)(value >> ((3 - i) * 8));
            }
        }

        data += chunk;
        length -= chunk;
    }

    return true;
}

bool EXISelectSD(long chan, unsigned long dev, unsigned long freq) {
    return dev == 0 && sd_gc_exi_begin((s32)chan, (u32)freq, false);
}
