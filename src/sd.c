#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "homeboy.h"
#include "io.h"
#include "sd.h"
#include "vc.h"

typedef struct {
    uint32_t cmd;
    uint32_t cmd_type;
    uint32_t rsp_type;
    uint32_t arg;
    uint32_t blk_cnt;
    uint32_t blk_size;
    void* dma_addr;
    uint32_t isdma;
    uint32_t pad0;
} sdio_request_t;

typedef struct {
    uint32_t rsp_fields[3];
    uint32_t acmd12_response;
} sdio_response_t;

static uint8_t* rw_buffer = NULL;
static const char* sdcard = "/dev/sdio/slot0";
static int32_t sd0_fd = -1;
static uint16_t sd0_rca = 0;
static bool sd0_initialized = false;
static bool sd0_sdhc = false;
static uint8_t sd0_cid[16];
static bool sdio_initialized = false;

static int32_t sdio_sendcommand(uint32_t cmd, uint32_t cmd_type, uint32_t rsp_type, uint32_t arg, uint32_t blk_cnt,
                                uint32_t blk_size, void* buffer, void* reply, uint32_t rlen) {
    int32_t ret;
    IPCIOVector* iovec = iosAllocAligned(hb_hid, sizeof(*iovec) * 3, 32);
    sdio_request_t* request = iosAllocAligned(hb_hid, sizeof(*request), 32);
    sdio_response_t* response = iosAllocAligned(hb_hid, sizeof(*response), 32);

    request->cmd = cmd;
    request->cmd_type = cmd_type;
    request->rsp_type = rsp_type;
    request->arg = arg;
    request->blk_cnt = blk_cnt;
    request->blk_size = blk_size;
    request->dma_addr = buffer;
    request->isdma = buffer != NULL;
    request->pad0 = 0;

    if (request->isdma || sd0_sdhc) {
        iovec[0].base = request;
        iovec[0].length = sizeof(sdio_request_t);
        iovec[1].base = buffer;
        iovec[1].length = blk_cnt * blk_size;
        iovec[2].base = response;
        iovec[2].length = sizeof(sdio_response_t);

        ret = IOS_Ioctlv(sd0_fd, IOCTL_SDIO_SENDCMD, 2, 1, iovec);
    } else {
        ret = IOS_Ioctl(sd0_fd, IOCTL_SDIO_SENDCMD, request, sizeof(*request), response, sizeof(*response));
    }

    if (reply && rlen <= 16) {
        memcpy(reply, response, rlen);
    }

    iosFree(hb_hid, request);
    iosFree(hb_hid, response);
    iosFree(hb_hid, iovec);

    return ret;
}

static int32_t sdio_setclock(uint32_t set) {
    int32_t ret;
    uint32_t* clock = iosAllocAligned(hb_hid, sizeof(*clock), 32);

    *clock = set;
    ret = IOS_Ioctl(sd0_fd, IOCTL_SDIO_SETCLK, clock, sizeof(*clock), NULL, 0);
    iosFree(hb_hid, clock);
    return ret;
}

static int32_t sdio_getstatus(void) {
    int32_t ret;
    uint32_t* status = iosAllocAligned(hb_hid, sizeof(*status), 32);

    ret = IOS_Ioctl(sd0_fd, IOCTL_SDIO_GETSTATUS, NULL, 0, status, sizeof(*status));
    if (ret < 0) {
        iosFree(hb_hid, status);
        return ret;
    }

    ret = *status;
    iosFree(hb_hid, status);
    return ret;
}

static int32_t sdio_resetcard(void) {
    int32_t ret;
    uint32_t* status = iosAllocAligned(hb_hid, sizeof(*status), 32);

    sd0_rca = 0;
    ret = IOS_Ioctl(sd0_fd, IOCTL_SDIO_RESETCARD, NULL, 0, status, sizeof(*status));
    if (ret < 0) {
        iosFree(hb_hid, status);
        return ret;
    }

    sd0_rca = (uint16_t)(*status >> 16);
    ret = *status & 0xFFFF;
    iosFree(hb_hid, status);
    return ret & 0xFFFF;
}

static int32_t sdio_gethcr(uint8_t reg, uint8_t size, uint32_t* val) {
    int32_t ret;

    if (val == NULL) {
        return -4;
    }

    uint32_t* hcr_value = iosAllocAligned(hb_hid, sizeof(*hcr_value), 32);
    uint32_t* hcr_query = iosAllocAligned(hb_hid, sizeof(*hcr_query) * 6, 32);

    *hcr_value = 0;
    *val = 0;

    hcr_query[0] = reg;
    hcr_query[1] = 0;
    hcr_query[2] = 0;
    hcr_query[3] = size;
    hcr_query[4] = 0;
    hcr_query[5] = 0;

    ret = IOS_Ioctl(sd0_fd, IOCTL_SDIO_READHCREG, hcr_query, sizeof(*hcr_query) * 6, hcr_value, sizeof(*hcr_value));
    *val = *hcr_value;
    iosFree(hb_hid, hcr_value);
    iosFree(hb_hid, hcr_query);

    return ret;
}

static int32_t sdio_sethcr(uint8_t reg, uint8_t size, uint32_t data) {
    int32_t ret;
    uint32_t* hcr_query = iosAllocAligned(hb_hid, sizeof(*hcr_query) * 6, 32);

    hcr_query[0] = reg;
    hcr_query[1] = 0;
    hcr_query[2] = 0;
    hcr_query[3] = size;
    hcr_query[4] = data;
    hcr_query[5] = 0;

    ret = IOS_Ioctl(sd0_fd, IOCTL_SDIO_WRITEHCREG, hcr_query, sizeof(*hcr_query) * 6, NULL, 0);
    iosFree(hb_hid, hcr_query);

    return ret;
}

static int32_t sdio_waithcr(uint8_t reg, uint8_t size, uint8_t unset, uint32_t mask) {
    uint32_t val;
    int32_t ret;
    int32_t tries = 10;

    while (tries-- > 0) {
        ret = sdio_gethcr(reg, size, &val);
        if (ret < 0) {
            return ret;
        }

        if ((unset && !(val & mask)) || (!unset && (val & mask))) {
            return 0;
        }

        for (int i = 0; i < 0x2B73A840 * ((float)10000 / (float)1000000); i++)
            ;
    }

    return -1;
}

static int32_t sdio_setbuswidth(uint32_t bus_width) {
    int32_t ret;
    uint32_t hc_reg = 0;

    ret = sdio_gethcr(SDIOHCR_HOSTCONTROL, 1, &hc_reg);
    if (ret < 0) {
        return ret;
    }

    hc_reg &= 0xFF;
    hc_reg &= ~SDIOHCR_HOSTCONTROL_4BIT;
    if (bus_width == 4) {
        hc_reg |= SDIOHCR_HOSTCONTROL_4BIT;
    }

    return sdio_sethcr(SDIOHCR_HOSTCONTROL, 1, hc_reg);
}

static int32_t sd0_getrca(void) {
    int32_t ret;
    uint32_t rca;

    ret = sdio_sendcommand(SDIO_CMD_SENDRCA, 0, SDIO_RESPONSE_R5, 0, 0, 0, NULL, &rca, sizeof(rca));
    if (ret < 0) {
        return ret;
    }

    sd0_rca = (uint16_t)(rca >> 16);
    return rca & 0xFFFF;
}

static int32_t sd0_select(void) {
    int32_t ret;

    ret = sdio_sendcommand(SDIO_CMD_SELECT, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1B, sd0_rca << 16, 0, 0, NULL, NULL, 0);

    return ret;
}

static int32_t sd0_deselect(void) {
    int32_t ret;

    ret = sdio_sendcommand(SDIO_CMD_DESELECT, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1B, 0, 0, 0, NULL, NULL, 0);

    return ret;
}

static int32_t sd0_setblocklength(uint32_t blk_len) {
    int32_t ret;

    ret = sdio_sendcommand(SDIO_CMD_SETBLOCKLEN, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1, blk_len, 0, 0, NULL, NULL, 0);

    return ret;
}

static int32_t sd0_setbuswidth(uint32_t bus_width) {
    uint16_t val = 0;
    int32_t ret;

    if (bus_width == 4) {
        val = 2;
    }

    ret = sdio_sendcommand(SDIO_CMD_APPCMD, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1, sd0_rca << 16, 0, 0, NULL, NULL, 0);
    if (ret < 0) {
        return ret;
    }

    ret = sdio_sendcommand(SDIO_ACMD_SETBUSWIDTH, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1, val, 0, 0, NULL, NULL, 0);

    return ret;
}

static int32_t sd0_getcid(void) {
    int32_t ret;

    ret = sdio_sendcommand(SDIO_CMD_ALL_SENDCID, 0, SDIO_RESPOSNE_R2, sd0_rca << 16, 0, 0, NULL, sd0_cid, 16);

    return ret;
}

static bool sd0_initio(void) {
    int32_t ret;
    int32_t tries;
    uint32_t status;
    sdio_response_t resp;

    sdio_resetcard();
    status = sdio_getstatus();

    if (!(status & SDIO_STATUS_CARD_INSERTED)) {
        return false;
    }

    if (!(status & SDIO_STATUS_CARD_INITIALIZED)) {
        // IOS doesn't like this card, so we need to convice it to accept it.

        // reopen the handle which makes IOS clean stuff up
        IOS_Close(sd0_fd);

        sd0_fd = IOS_Open(sdcard, 3);

        // reset the host controller
        if (sdio_sethcr(SDIOHCR_SOFTWARERESET, 1, 7) < 0) {
            goto fail;
        }

        if (sdio_waithcr(SDIOHCR_SOFTWARERESET, 1, 1, 7) < 0) {
            goto fail;
        }

        // initialize interrupts (sd_reset_card does this on success)
        sdio_sethcr(0x34, 4, 0x13F00C3);
        sdio_sethcr(0x38, 4, 0x13F00C3);

        // enable power
        sd0_sdhc = true;
        ret = sdio_sethcr(SDIOHCR_POWERCONTROL, 1, 0xE);
        if (ret < 0) {
            goto fail;
        }

        ret = sdio_sethcr(SDIOHCR_POWERCONTROL, 1, 0xF);
        if (ret < 0) {
            goto fail;
        }

        // enable internal clock, wait until it gets stable and enable sd clock
        ret = sdio_sethcr(SDIOHCR_CLOCKCONTROL, 2, 0);
        if (ret < 0) {
            goto fail;
        }

        ret = sdio_sethcr(SDIOHCR_CLOCKCONTROL, 2, 0x101);
        if (ret < 0) {
            goto fail;
        }

        ret = sdio_waithcr(SDIOHCR_CLOCKCONTROL, 2, 0, 2);
        if (ret < 0) {
            goto fail;
        }

        ret = sdio_sethcr(SDIOHCR_CLOCKCONTROL, 2, 0x107);
        if (ret < 0) {
            goto fail;
        }

        // setup timeout
        ret = sdio_sethcr(SDIOHCR_TIMEOUTCONTROL, 1, SDIO_DEFAULT_TIMEOUT);
        if (ret < 0) {
            goto fail;
        }

        // standard SDHC initialization process
        ret = sdio_sendcommand(SDIO_CMD_GOIDLE, 0, 0, 0, 0, 0, NULL, NULL, 0);
        if (ret < 0) {
            goto fail;
        }

        ret = sdio_sendcommand(SDIO_CMD_SENDIFCOND, 0, SDIO_RESPONSE_R6, 0x1AA, 0, 0, NULL, &resp, sizeof(resp));
        if (ret < 0) {
            goto fail;
        }

        if ((resp.rsp_fields[0] & 0xFF) != 0xAA) {
            goto fail;
        }

        tries = 10;
        while (tries-- > 0) {
            ret = sdio_sendcommand(SDIO_CMD_APPCMD, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1, 0, 0, 0, NULL, NULL, 0);
            if (ret < 0) {
                goto fail;
            }

            ret = sdio_sendcommand(SDIO_ACMD_SENDOPCOND, 0, SDIO_RESPONSE_R3, 0x40300000, 0, 0, NULL, &resp,
                                   sizeof(resp));
            if (ret < 0) {
                goto fail;
            }

            if (resp.rsp_fields[0] & (1 << 31)) {
                break;
            }

            for (int i = 0; i < 0x2B73A840 * ((float)10000 / (float)1000000); i++)
                ;
        }

        if (tries < 0) {
            goto fail;
        }

        // FIXME: SDv2 cards which are not high-capacity won't work :/
        if (resp.rsp_fields[0] & (1 << 30)) {
            sd0_sdhc = true;
        } else {
            sd0_sdhc = false;
        }

        ret = sd0_getcid();
        if (ret < 0) {
            goto fail;
        }

        ret = sd0_getrca();
        if (ret < 0) {
            goto fail;
        }
    } else if (status & SDIO_STATUS_CARD_SDHC) {
        sd0_sdhc = true;
    } else {
        sd0_sdhc = false;
    }

    ret = sdio_setbuswidth(4);
    if (ret < 0) {
        return false;
    }

    ret = sdio_setclock(1);
    if (ret < 0) {
        return false;
    }

    ret = sd0_select();
    if (ret < 0) {
        return false;
    }

    ret = sd0_setblocklength(PAGE_SIZE512);
    if (ret < 0) {
        ret = sd0_deselect();
        return false;
    }

    ret = sd0_setbuswidth(4);
    if (ret < 0) {
        ret = sd0_deselect();
        return false;
    }

    sd0_deselect();

    sd0_initialized = 1;
    return true;

fail:
    sdio_sethcr(SDIOHCR_SOFTWARERESET, 1, 7);
    sdio_waithcr(SDIOHCR_SOFTWARERESET, 1, 1, 7);
    IOS_Close(sd0_fd);
    sd0_fd = IOS_Open(sdcard, 3);
    return false;
}

static bool sdio_deinitialize(void) {
    if (sd0_fd >= 0) {
        IOS_Close(sd0_fd);
    }

    sd0_fd = -1;
    sdio_initialized = false;
    return true;
}

bool sdio_start(void) {
    if (sdio_initialized) {
        return true;
    }

    if (rw_buffer == NULL) {
        rw_buffer = iosAllocAligned(hb_hid, 4096, 32);

        if (rw_buffer == NULL) {
            return false;
        }
    }

    sd0_fd = IOS_Open(sdcard, 3);

    if (sd0_fd < 0) {
        sdio_deinitialize();
        return false;
    }

    if (!sd0_initio()) {
        sdio_deinitialize();
        return false;
    }

    sdio_initialized = true;
    return true;
}

bool sdio_stop(void) {
    if (!sd0_initialized) {
        return false;
    }

    sdio_deinitialize();
    sd0_initialized = false;

    return true;
}

bool sdio_read_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer) {
    int32_t ret;
    uint8_t* ptr;
    uint32_t blk_off;
    bool null_buf = false;

    if (buffer == NULL) {
        allocMEM2(&buffer, sec_cnt * PAGE_SIZE512);
        memset(buffer, 0, sec_cnt * PAGE_SIZE512);
        null_buf = 1;
    }

    ret = sd0_select();

    if (ret >= 0) {
        if ((uint32_t)buffer & 0x1F) {
            int secs_to_read;
            ptr = (uint8_t*)buffer;

            while (sec_cnt > 0) {
                if (sd0_sdhc) {
                    blk_off = sector;
                } else {
                    blk_off = sector * PAGE_SIZE512;
                }

                if (sec_cnt > 8) {
                    secs_to_read = 8;
                } else {
                    secs_to_read = sec_cnt;
                }

                ret = sdio_sendcommand(SDIO_CMD_READMULTIBLOCK, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1, blk_off,
                                       secs_to_read, PAGE_SIZE512, rw_buffer, NULL, 0);
                if (ret < 0) {
                    break;
                } else {
                    memcpy(ptr, rw_buffer, secs_to_read * PAGE_SIZE512);
                    ptr += secs_to_read * PAGE_SIZE512;
                    sector += secs_to_read;
                    sec_cnt -= secs_to_read;
                }
            }
        } else {
            if (!sd0_sdhc) {
                sector *= PAGE_SIZE512;
            }

            ret = sdio_sendcommand(SDIO_CMD_READMULTIBLOCK, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1, sector, sec_cnt,
                                   PAGE_SIZE512, buffer, NULL, 0);
        }

        sd0_deselect();
    }

    if (null_buf) {
        xlHeapFree(&buffer);
    }

    return !(ret < 0);
}

bool sdio_write_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer) {
    int32_t ret;
    uint8_t* ptr;
    uint32_t blk_off;
    bool null_buf = false;

    if (buffer == NULL) {
        allocMEM2(&buffer, sec_cnt * PAGE_SIZE512);
        memset(buffer, 0, sec_cnt * PAGE_SIZE512);
        null_buf = true;
    }

    ret = sd0_select();

    if (ret >= 0) {
        if ((uint32_t)buffer & 0x1F) {
            int secs_to_write;
            ptr = (uint8_t*)buffer;

            while (sec_cnt > 0) {
                if (sd0_sdhc == 0) {
                    blk_off = sector * PAGE_SIZE512;
                } else {
                    blk_off = sector;
                }

                if (sec_cnt > 8) {
                    secs_to_write = 8;
                } else {
                    secs_to_write = sec_cnt;
                }

                memcpy(rw_buffer, ptr, secs_to_write * PAGE_SIZE512);
                ret = sdio_sendcommand(SDIO_CMD_WRITEMULTIBLOCK, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1, blk_off,
                                       secs_to_write, PAGE_SIZE512, rw_buffer, NULL, 0);
                if (ret >= 0) {
                    ptr += secs_to_write * PAGE_SIZE512;
                    sector += secs_to_write;
                    sec_cnt -= secs_to_write;
                } else {
                    break;
                }
            }
        } else {
            if (sd0_sdhc == 0) {
                sector *= PAGE_SIZE512;
            }

            ret = sdio_sendcommand(SDIO_CMD_WRITEMULTIBLOCK, SDIOCMD_TYPE_AC, SDIO_RESPONSE_R1, sector, sec_cnt,
                                   PAGE_SIZE512, buffer, NULL, 0);
        }

        sd0_deselect();
    }

    if (null_buf) {
        xlHeapFree(&buffer);
    }

    return !(ret < 0);
}

bool sdio_is_inserted(void) {
    return ((sdio_getstatus() & SDIO_STATUS_CARD_INSERTED) == SDIO_STATUS_CARD_INSERTED);
}

bool sdio_is_initialized(void) {
    return ((sdio_getstatus() & SDIO_STATUS_CARD_INITIALIZED) == SDIO_STATUS_CARD_INITIALIZED);
}

bool sdio_is_sdhc(void) {
    return sd0_sdhc;
}
