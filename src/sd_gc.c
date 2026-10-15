#include <stddef.h>
#include <string.h>

#include "os.h"
#include "sd.h"
#include "sd_gc_internal.h"
#include "version.h"

#if IS_GC

#define SD_GC_CHANNEL             1
#define SD_GC_INIT_FREQUENCY      0
#define SD_SECTOR_SIZE            512
#define SD_INIT_TIMEOUT_TICKS     40500000
#define SD_RESPONSE_TIMEOUT_TICKS 4050000
#define SD_BUSY_TIMEOUT_TICKS     20250000

static const u8 sd_clock_div[64] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x03, 0x05, 0xFF, 0x00, 0x03, 0x05, 0xFF, 0x00, 0x03, 0x05,
    0xFF, 0x00, 0x03, 0x05, 0xFF, 0x01, 0x04, 0x05, 0xFF, 0x01, 0x04, 0x05, 0xFF, 0x01, 0x04, 0x05,
    0xFF, 0x01, 0x05, 0x05, 0xFF, 0x02, 0x05, 0x05, 0xFF, 0x02, 0x05, 0x05, 0xFF, 0x02, 0x05, 0x05,
    0xFF, 0x02, 0x05, 0x05, 0xFF, 0x02, 0x05, 0x05, 0xFF, 0x02, 0x05, 0x05, 0xFF, 0x03, 0x05, 0x05,
};

extern OSTick OSGetTick(void);

static bool sd_initialized;
static bool sd_sdhc;
static u32 sd_last_error;
static u32 sd_last_error_detail;
static u32 sd_frequency = SD_GC_INIT_FREQUENCY;
static u8 sd_scratch[SD_SECTOR_SIZE];

static bool sd_begin(bool select_device);

static bool sd_wait_not_busy_selected(u32 timeout_ticks) {
    u8 value;
    OSTick start = OSGetTick();

    do {
        value = 0xFF;
        if (!sd_gc_exi_transfer(SD_GC_CHANNEL, &value, 1, SD_GC_EXI_READ)) {
            return false;
        }
        if (value != 0) {
            return true;
        }
    } while ((u32)(OSGetTick() - start) < timeout_ticks);

    return false;
}

static u8 sd_crc7(const u8* data, u32 length) {
    u8 crc = 0;

    for (u32 i = 0; i < length; i++) {
        u8 value = data[i];
        for (u32 bit = 0; bit < 8; bit++) {
            crc <<= 1;
            if (((value ^ crc) & 0x80) != 0) {
                crc ^= 0x09;
            }
            value <<= 1;
        }
    }

    return (u8)((crc << 1) | 1);
}

static u16 sd_crc16(const u8* data, u32 length) {
    u16 crc = 0;

    for (u32 i = 0; i < length; i++) {
        crc ^= (u16)data[i] << 8;
        for (u32 bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000) ? (u16)((crc << 1) ^ 0x1021) : (u16)(crc << 1);
        }
    }

    return crc;
}

static bool sd_begin(bool select_device) {
    return sd_gc_exi_begin(SD_GC_CHANNEL, sd_frequency, select_device);
}

static bool sd_send_idle_clocks(void) {
    u8 clocks[0x80];

    memset(clocks, 0xFF, sizeof(clocks));
    if (!sd_gc_exi_begin(SD_GC_CHANNEL, SD_GC_INIT_FREQUENCY, false)) {
        return false;
    }

    for (u32 i = 0; i < 0x14; i++) {
        if (!sd_gc_exi_transfer(SD_GC_CHANNEL, clocks, sizeof(clocks), SD_GC_EXI_WRITE)) {
            sd_gc_exi_end(SD_GC_CHANNEL);
            return false;
        }
    }

    return sd_gc_exi_end(SD_GC_CHANNEL);
}

static bool sd_send_command(u8 command, u32 argument) {
    u8 packet[6];
    u8 clocks[10];
    bool result;

    packet[0] = 0x40 | command;
    packet[1] = (u8)(argument >> 24);
    packet[2] = (u8)(argument >> 16);
    packet[3] = (u8)(argument >> 8);
    packet[4] = (u8)argument;
    packet[5] = sd_crc7(packet, 5);
    memset(clocks, 0xFF, sizeof(clocks));

    if (!sd_begin(true)) {
        return false;
    }

    result = sd_gc_exi_transfer(SD_GC_CHANNEL, clocks, sizeof(clocks), SD_GC_EXI_WRITE) &&
             sd_gc_exi_transfer(SD_GC_CHANNEL, packet, 5, SD_GC_EXI_WRITE) &&
             sd_gc_exi_transfer(SD_GC_CHANNEL, packet + 5, 1, SD_GC_EXI_WRITE);
    sd_gc_exi_end(SD_GC_CHANNEL);
    return result;
}

static bool sd_read_response(u8* response, u32 length, bool skip_stuff_byte) {
    u8 value = 0xFF;
    OSTick start;
    bool result = false;

    if (response == NULL || length == 0 || !sd_begin(true)) {
        return false;
    }

    if (skip_stuff_byte && !sd_gc_exi_transfer(SD_GC_CHANNEL, &value, 1, SD_GC_EXI_READ)) {
        goto done;
    }

    start = OSGetTick();
    do {
        value = 0xFF;
        if (!sd_gc_exi_transfer(SD_GC_CHANNEL, &value, 1, SD_GC_EXI_READ)) {
            goto done;
        }
    } while ((value & 0x80) != 0 && (u32)(OSGetTick() - start) < SD_RESPONSE_TIMEOUT_TICKS);

    if ((value & 0x80) != 0) {
        goto done;
    }

    response[0] = value;
    if (length > 1) {
        memset(response + 1, 0xFF, length - 1);
        if (!sd_gc_exi_transfer(SD_GC_CHANNEL, response + 1, length - 1, SD_GC_EXI_READ)) {
            goto done;
        }
    }
    result = true;

done:
    sd_gc_exi_end(SD_GC_CHANNEL);
    return result;
}

static bool sd_command_response(u8 command, u32 argument, u8* response, u32 length) {
    return sd_send_command(command, argument) && sd_read_response(response, length, command == 12);
}

static bool sd_read_data_block(u8* data, u32 length) {
    u8 token = 0xFF;
    u8 crc_bytes[2];
    OSTick start;
    bool result = false;

    if (data == NULL || length == 0 || !sd_begin(true)) {
        return false;
    }

    start = OSGetTick();
    do {
        token = 0xFF;
        if (!sd_gc_exi_transfer(SD_GC_CHANNEL, &token, 1, SD_GC_EXI_READ)) {
            goto done;
        }
    } while (token == 0xFF && (u32)(OSGetTick() - start) < SD_RESPONSE_TIMEOUT_TICKS);

    if (token != 0xFE || !sd_gc_exi_transfer(SD_GC_CHANNEL, data, length, SD_GC_EXI_READ)) {
        goto done;
    }

    crc_bytes[0] = crc_bytes[1] = 0xFF;
    if (!sd_gc_exi_transfer(SD_GC_CHANNEL, crc_bytes, sizeof(crc_bytes), SD_GC_EXI_READ)) {
        goto done;
    }

    result = (((u16)crc_bytes[0] << 8) | crc_bytes[1]) == sd_crc16(data, length);

done:
    sd_gc_exi_end(SD_GC_CHANNEL);
    return result;
}

static bool sd_write_data_block(const u8* data, u32 length, u8 token, u32 block_index) {
    u16 crc;
    u8 crc_bytes[2];
    u8 response;
    u8 busy;
    OSTick start;
    bool result = false;

    if (data == NULL || length == 0 || !sd_begin(true)) {
        return false;
    }

    crc = sd_crc16(data, length);
    crc_bytes[0] = (u8)(crc >> 8);
    crc_bytes[1] = (u8)crc;
    if (!sd_gc_exi_transfer(SD_GC_CHANNEL, &token, 1, SD_GC_EXI_WRITE) ||
        !sd_gc_exi_transfer(SD_GC_CHANNEL, (u8*)data, length, SD_GC_EXI_WRITE)) {
        goto done;
    }

    start = OSGetTick();
    while ((u32)(OSGetTick() - start) < 41) {}

    if (!sd_gc_exi_transfer(SD_GC_CHANNEL, crc_bytes, sizeof(crc_bytes), SD_GC_EXI_WRITE)) {
        goto done;
    }
    sd_gc_exi_end(SD_GC_CHANNEL);

    if (!sd_begin(true)) {
        sd_last_error = SDIO_ERROR_WRITE_DATA;
        return false;
    }

    start = OSGetTick();
    do {
        response = 0xFF;
        if (!sd_gc_exi_transfer(SD_GC_CHANNEL, &response, 1, SD_GC_EXI_READ)) {
            goto done;
        }
        response &= 0x1F;
        if (response == 0x05 || response == 0x0B || response == 0x0D) {
            break;
        }
    } while ((u32)(OSGetTick() - start) < SD_RESPONSE_TIMEOUT_TICKS);
    if (response != 0x05 && response != 0x0B && response != 0x0D) {
        sd_last_error_detail = ((u32)response << 8) | (block_index & 0xFF);
        sd_last_error = SDIO_ERROR_WRITE_DATA;
        goto done;
    }
    sd_last_error_detail = (u32)response << 8;

    start = OSGetTick();
    do {
        busy = 0;
        if (!sd_gc_exi_transfer(SD_GC_CHANNEL, &busy, 1, SD_GC_EXI_READ)) {
            goto done;
        }
    } while (busy == 0 && (u32)(OSGetTick() - start) < SD_BUSY_TIMEOUT_TICKS);
    sd_last_error_detail |= busy;

    if (busy == 0) {
        sd_last_error = SDIO_ERROR_WRITE_BUSY_TIMEOUT;
        goto done;
    }
    if (response != 0x05) {
        sd_last_error = SDIO_ERROR_WRITE_REJECTED;
        goto done;
    }
    result = true;

done:
    sd_gc_exi_end(SD_GC_CHANNEL);
    return result;
}

bool sdio_is_initialized(void) {
    return sd_initialized && sd_gc_exi_present(SD_GC_CHANNEL);
}

bool sdio_is_inserted(void) {
    return sd_gc_exi_present(SD_GC_CHANNEL);
}

bool sdio_is_sdhc(void) {
    return sd_initialized && sd_sdhc;
}

uint32_t sdio_get_error(void) {
    return sd_last_error;
}

uint32_t sdio_get_error_detail(void) {
    return sd_last_error_detail;
}

bool sdio_write_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer) {
    u8 response;
    u8* data = (u8*)buffer;
    u64 end_sector = (u64)sector + sec_cnt;

    sd_last_error = SDIO_ERROR_NONE;
    sd_last_error_detail = 0;

    if (!sdio_is_initialized() || sec_cnt == 0 || end_sector > 0x100000000ULL) {
        sd_last_error = SDIO_ERROR_WRITE_CMD;
        return false;
    }

    if (data == NULL) {
        memset(sd_scratch, 0, sizeof(sd_scratch));
    }

    u64 byte_address = sd_sdhc ? sector : (u64)sector * SD_SECTOR_SIZE;
    if (byte_address > 0xFFFFFFFFULL || (!sd_sdhc && end_sector * SD_SECTOR_SIZE > 0x100000000ULL)) {
        sd_last_error = SDIO_ERROR_WRITE_CMD;
        return false;
    }

    for (u32 i = 0; i < sec_cnt; i++) {
        u64 block_address = sd_sdhc ? (u64)sector + i : ((u64)sector + i) * SD_SECTOR_SIZE;
        response = 0xFF;
        if (!sd_command_response(24, (u32)block_address, &response, 1) || response != 0) {
            sd_last_error = SDIO_ERROR_WRITE_CMD;
            sd_last_error_detail = ((u32)response << 8) | (i & 0xFF);
            return false;
        }

        if (data == NULL) {
            memset(sd_scratch, 0, sizeof(sd_scratch));
            if (!sd_write_data_block(sd_scratch, sizeof(sd_scratch), 0xFE, i)) {
                if (sd_last_error == SDIO_ERROR_NONE) {
                    sd_last_error = SDIO_ERROR_WRITE_DATA;
                }
                return false;
            }
        } else if (!sd_write_data_block(data + i * SD_SECTOR_SIZE, SD_SECTOR_SIZE, 0xFE, i)) {
            if (sd_last_error == SDIO_ERROR_NONE) {
                sd_last_error = SDIO_ERROR_WRITE_DATA;
            }
            return false;
        }
    }

    sd_last_error = SDIO_ERROR_NONE;
    sd_last_error_detail = 0;
    return true;
}

bool sdio_read_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer) {
    u8 response;
    u8* data = (u8*)buffer;
    u64 end_sector = (u64)sector + sec_cnt;

    if (!sdio_is_initialized() || sec_cnt == 0 || end_sector > 0x100000000ULL) {
        sd_last_error = SDIO_ERROR_READ_CMD;
        return false;
    }

    u64 byte_address = sd_sdhc ? sector : (u64)sector * SD_SECTOR_SIZE;
    if (byte_address > 0xFFFFFFFFULL || (!sd_sdhc && end_sector * SD_SECTOR_SIZE > 0x100000000ULL)) {
        sd_last_error = SDIO_ERROR_READ_CMD;
        return false;
    }

    if (!sd_command_response(18, (u32)byte_address, &response, 1) || response != 0) {
        sd_last_error = SDIO_ERROR_READ_CMD;
        return false;
    }

    for (u32 i = 0; i < sec_cnt; i++) {
        u8* target = data != NULL ? data + i * SD_SECTOR_SIZE : sd_scratch;
        if (!sd_read_data_block(target, SD_SECTOR_SIZE)) {
            u8 stop_response;
            sd_command_response(12, 0, &stop_response, 1);
            sd_last_error = SDIO_ERROR_READ_DATA;
            return false;
        }
    }

    if (!sd_command_response(12, 0, &response, 1)) {
        sd_last_error = SDIO_ERROR_READ_STOP;
        return false;
    }
    sd_last_error = SDIO_ERROR_NONE;
    return true;
}

bool sdio_stop(void) {
    if (!sd_initialized) {
        return false;
    }

    sd_initialized = false;
    sd_sdhc = false;
    sd_last_error = SDIO_ERROR_NONE;
    sd_frequency = SD_GC_INIT_FREQUENCY;
    return true;
}

bool sdio_start(void) {
    u8 response[5];
    u8 csd[16];
    u8 command_response;
    u32 ocr;
    OSTick start;
    bool v2_card;

    if (sd_initialized) {
        return true;
    }

    sd_sdhc = false;
    sd_frequency = SD_GC_INIT_FREQUENCY;
    sd_last_error = SDIO_ERROR_NONE;
    if (!sdio_is_inserted()) {
        sd_last_error = SDIO_ERROR_NO_EXI;
        return false;
    }
    if (!sd_send_idle_clocks()) {
        sd_last_error = SDIO_ERROR_IDLE_CLOCKS;
        return false;
    }

    if (!sd_command_response(0, 0, &command_response, 1) || command_response != 1) {
        sd_last_error = SDIO_ERROR_CMD0;
        return false;
    }

    if (!sd_command_response(8, 0x1AA, response, sizeof(response))) {
        sd_last_error = SDIO_ERROR_CMD8;
        return false;
    }
    if (response[0] == 1) {
        if (response[3] != 1 || response[4] != 0xAA) {
            sd_last_error = SDIO_ERROR_CMD8;
            return false;
        }
        v2_card = true;
    } else if ((response[0] & 4) != 0) {
        v2_card = false;
    } else {
        sd_last_error = SDIO_ERROR_CMD8;
        return false;
    }

    start = OSGetTick();
    do {
        if (!sd_command_response(55, 0, &command_response, 1) || (command_response != 0 && command_response != 1)) {
            sd_last_error = SDIO_ERROR_ACMD41;
            return false;
        }

        if (!sd_command_response(41, v2_card ? 0x40300000 : 0x00300000, &command_response, 1)) {
            sd_last_error = SDIO_ERROR_ACMD41;
            return false;
        }
        if (command_response == 0) {
            break;
        }
        if (command_response != 1) {
            sd_last_error = SDIO_ERROR_ACMD41;
            return false;
        }
    } while ((u32)(OSGetTick() - start) < SD_INIT_TIMEOUT_TICKS);

    if (command_response != 0 || !sd_command_response(58, 0, response, sizeof(response)) || response[0] != 0) {
        sd_last_error = SDIO_ERROR_CMD58;
        return false;
    }

    ocr = ((u32)response[1] << 24) | ((u32)response[2] << 16) | ((u32)response[3] << 8) | response[4];
    sd_sdhc = (ocr & 0x40000000) != 0;

    if (!sd_sdhc) {
        if (!sd_command_response(16, SD_SECTOR_SIZE, &command_response, 1) || command_response != 0) {
            sd_last_error = SDIO_ERROR_SET_BLOCKLEN;
            return false;
        }
    }

    if (sd_command_response(9, 0, &command_response, 1) && command_response == 0 &&
        sd_read_data_block(csd, sizeof(csd))) {
        u8 transfer_speed = csd[3];
        u8 speed_unit = transfer_speed & 7;
        if (speed_unit <= 3) {
            u8 frequency = sd_clock_div[(((transfer_speed >> 3) & 0xF) * 4) + speed_unit];
            if (frequency != 0xFF) {
                sd_frequency = frequency;
            }
        }
    }

    sd_initialized = true;
    sd_last_error = SDIO_ERROR_NONE;
    return true;
}

#endif
