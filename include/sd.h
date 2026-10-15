#ifndef _SD_H
#define _SD_H

#include <stdint.h>

#include "types.h"
#include "version.h"

#define PAGE_SIZE512                 512

#define SDIOHCR_HOSTCONTROL          0x28
#define SDIOHCR_POWERCONTROL         0x29
#define SDIOHCR_CLOCKCONTROL         0x2c
#define SDIOHCR_TIMEOUTCONTROL       0x2e
#define SDIOHCR_SOFTWARERESET        0x2f

#define SDIOHCR_HOSTCONTROL_4BIT     0x02

#define SDIO_DEFAULT_TIMEOUT         0xe

#define IOCTL_SDIO_WRITEHCREG        0x01
#define IOCTL_SDIO_READHCREG         0x02
#define IOCTL_SDIO_RESETCARD         0x04
#define IOCTL_SDIO_SETCLK            0x06
#define IOCTL_SDIO_SENDCMD           0x07
#define IOCTL_SDIO_GETSTATUS         0x0B

#define SDIOCMD_TYPE_AC              3

#define SDIO_RESPONSE_R1             1
#define SDIO_RESPONSE_R1B            2
#define SDIO_RESPOSNE_R2             3
#define SDIO_RESPONSE_R3             4
#define SDIO_RESPONSE_R5             6
#define SDIO_RESPONSE_R6             7

#define SDIO_CMD_GOIDLE              0x00
#define SDIO_CMD_ALL_SENDCID         0x02
#define SDIO_CMD_SENDRCA             0x03
#define SDIO_CMD_SELECT              0x07
#define SDIO_CMD_DESELECT            0x07
#define SDIO_CMD_SENDIFCOND          0x08
#define SDIO_CMD_SETBLOCKLEN         0x10
#define SDIO_CMD_READMULTIBLOCK      0x12
#define SDIO_CMD_WRITEMULTIBLOCK     0x19
#define SDIO_CMD_APPCMD              0x37

#define SDIO_ACMD_SETBUSWIDTH        0x06
#define SDIO_ACMD_SENDOPCOND         0x29

#define SDIO_STATUS_CARD_INSERTED    0x1
#define SDIO_STATUS_CARD_INITIALIZED 0x10000
#define SDIO_STATUS_CARD_SDHC        0x100000

#define SD_ERROR_SUCCESS             0
#define SD_ERROR_INVAL               1

#if IS_GC
#define SDIO_ERROR_NONE               0
#define SDIO_ERROR_NO_EXI             1
#define SDIO_ERROR_IDLE_CLOCKS        2
#define SDIO_ERROR_CMD0               3
#define SDIO_ERROR_CMD8               4
#define SDIO_ERROR_ACMD41             5
#define SDIO_ERROR_CMD58              6
#define SDIO_ERROR_SET_BLOCKLEN       7
#define SDIO_ERROR_READ_CMD           8
#define SDIO_ERROR_READ_DATA          9
#define SDIO_ERROR_READ_STOP          10
#define SDIO_ERROR_WRITE_CMD          11
#define SDIO_ERROR_WRITE_DATA         12
#define SDIO_ERROR_WRITE_REJECTED     14
#define SDIO_ERROR_WRITE_BUSY_TIMEOUT 15

#define SD_GC_EXI_WRITE               1
#define SD_GC_EXI_READ                2

uint32_t sdio_get_error(void);
uint32_t sdio_get_error_detail(void);
#endif

bool sdio_is_initialized(void);
bool sdio_is_inserted(void);
bool sdio_is_sdhc(void);
bool sdio_write_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer);
bool sdio_read_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer);
bool sdio_stop(void);
bool sdio_start(void);

#endif
