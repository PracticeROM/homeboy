#ifndef _VC_H
#define _VC_H

#include <stddef.h>
#include <stdint.h>

#include "cpu.h"
#include "os.h"
#include "version.h"

#define INIT __attribute__((section(".init")))

// system.h
typedef enum SystemMode {
    SM_NONE = -1,
    SM_RUNNING = 0,
    SM_STOPPED = 1,
} SystemMode;

#if IS_GC

typedef enum SystemObjectType {
    SOT_NONE = -1,
    SOT_CPU = 0,
    SOT_PIF = 1,
    SOT_RAM = 2,
    SOT_ROM = 3,
    SOT_RSP = 4,
    SOT_RDP = 5,
    SOT_MIPS = 6,
    SOT_DISK = 7,
    SOT_FLASH = 8,
    SOT_SRAM = 9,
    SOT_AUDIO = 10,
    SOT_VIDEO = 11,
    SOT_SERIAL = 12,
    SOT_LIBRARY = 13,
    SOT_PERIPHERAL = 14,
    SOT_RDB = 15,
    SOT_COUNT = 16,
} SystemObjectType;

#elif IS_WII

typedef enum SystemObjectType {
    SOT_NONE = -1,
    SOT_CPU = 0,
    SOT_PIF = 1,
    SOT_RAM = 2,
    SOT_ROM = 3,
    SOT_RSP = 4,
    SOT_RDP = 5,
    SOT_MI = 6,
    SOT_DISK = 7,
    SOT_AI = 8,
    SOT_VI = 9,
    SOT_SI = 10,
    SOT_PI = 11,
    SOT_RDB = 12,
    SOT_PAK = 13,
    SOT_SRAM = 14,
    SOT_FLASH = 15,
    SOT_CODE = 16,
    SOT_HELP = 17,
    SOT_LIBRARY = 18,
    SOT_FRAME = 19,
    SOT_AUDIO = 20,
    SOT_VIDEO = 21,
    SOT_CONTROLLER = 22,
    SOT_COUNT = 23,
} SystemObjectType;

#endif

typedef s32 SystemRomType; // big enum, no need to have it here for now

#if IS_GC

typedef bool (*SystemCopyCallbackFunc)(void);

typedef struct SystemRomCopy {
    /* 0x00 */ s32 nSize;
    /* 0x04 */ s32 nOffsetRAM;
    /* 0x08 */ s32 nOffsetROM;
    /* 0x0C */ SystemCopyCallbackFunc pCallback;
} SystemRomCopy; // size = 0x10

typedef struct System {
    /* 0x00 */ void* pFrame;
    /* 0x04 */ void* pSound;
    /* 0x08 */ bool bException;
    /* 0x0C */ SystemMode eMode;
    /* 0x10 */ SystemRomCopy romCopy;
    /* 0x20 */ SystemRomType eTypeROM;
    /* 0x24 */ void* apObject[SOT_COUNT];
    /* 0x68 */ u64 nAddressBreak;
    /* 0x70 */ SystemObjectType storageDevice;
    /* 0x74 */ u8 anException[16];
    /* 0x84 */ bool bJapaneseVersion;
} System; // size = 0x88

#elif IS_WII && IS_OOT

typedef struct System {
    /* 0x00 */ bool bException;
    /* 0x04 */ SystemMode eMode;
    /* 0x08 */ SystemObjectType storageDevice;
    /* 0x0C */ SystemRomType eTypeROM;
    /* 0x10 */ void* apObject[SOT_COUNT];
    /* 0x6C */ s32 unk_6C;
    /* 0x70 */ u64 nAddressBreak;
    /* 0x78 */ s32 unk_78[19];
    /* 0xC4 */ void* pSound;
    /* 0xC8 */ u8 anException[16];
} System; // size = 0xD8

#elif IS_WII && IS_MM

// TODO: figure out the System struct on MM
typedef struct System {
    char unk_0x00[0x28];
    void* apObject[SOT_COUNT];
} System;

#endif

typedef struct Ram {
    /* 0x00 */ void* pHost;
    /* 0x04 */ u8* pBuffer;
    /* 0x08 */ u32 nSize;
#if IS_WII
    /* 0x0C */ u32 RDRAM_CONFIG_REG;
    /* 0x10 */ u32 RDRAM_DEVICE_ID_REG;
    /* 0x14 */ u32 RDRAM_DELAY_REG;
    /* 0x18 */ u32 RDRAM_MODE_REG;
    /* 0x1C */ u32 RDRAM_REF_INTERVAL_REG;
    /* 0x20 */ u32 RDRAM_REF_ROW_REG;
    /* 0x24 */ u32 RDRAM_RAS_INTERVAL_REG;
    /* 0x28 */ u32 RDRAM_MIN_INTERVAL_REG;
    /* 0x2C */ u32 RDRAM_ADDR_SELECT_REG;
    /* 0x30 */ u32 RDRAM_DEVICE_MANUF_REG;
    /* 0x34 */ u32 RI_MODE_REG;
    /* 0x38 */ u32 RI_CONFIG_REG;
    /* 0x3C */ u32 RI_SELECT_REG;
    /* 0x40 */ u32 RI_REFRESH_REG;
    /* 0x44 */ u32 RI_LATENCY_REG;
#endif
} Ram; // size = 0x48

#define SYSTEM_CPU(pSystem) ((void*)(((System*)(pSystem))->apObject[SOT_CPU]))
#define SYSTEM_ROM(pSystem) ((Rom*)(((System*)(pSystem))->apObject[SOT_ROM]))

// xlObject.h
typedef struct _XL_OBJECTTYPE _XL_OBJECTTYPE;

typedef int (*EventFunc)(void* pObject, int nEvent, void* pArgument);

struct _XL_OBJECTTYPE {
    /* 0x0 */ char* szName;
    /* 0x4 */ s32 nSizeObject;
    /* 0x8 */ _XL_OBJECTTYPE* pClassBase;
    /* 0xC */ EventFunc pfEvent;
}; // size = 0x10

#if IS_OOT
bool cpuExecuteUpdate(Cpu* pCPU, s32* pnAddressGCN, u32 nCount);
#elif IS_MM
bool cpuExecuteUpdate(Cpu* cpu, s32* pnAddressGCN, s64 nTime);
#endif
bool cpuMapObject(Cpu* pCPU, void* pObject, u32 nAddress0, u32 nAddress1, s32 nType);
bool cpuSetDeviceGet(Cpu* pCPU, CpuDevice* pDevice, void* pfGet8, void* pfGet16, void* pfGet32, void* pfGet64);
bool cpuSetDevicePut(Cpu* pCPU, CpuDevice* pDevice, void* pfPut8, void* pfPut16, void* pfPut32, void* pfPut64);
bool cpuFindFunction(Cpu* pCPU, s32 theAddress, CpuFunction** tree_node);
bool ramSetSize(Ram* pRAM, s32 nSize);
bool xlHeapTake(void** ppHeap, s32 nByteCount);
bool xlHeapFree(void** ppHeap);
bool xlObjectMake(void** ppObject, void* pArgument, _XL_OBJECTTYPE* pType);

void DCStoreRange(const void* buf, u32 len);
void ICInvalidateRange(const void* buf, u32 len);

void OSReport(const char* msg, ...);
bool OSCreateThread(OSThread* thread, OSThreadFunc func, void* funcArg, void* stackBegin, u32 stackSize, s32 prio,
                    u16 flags);
void OSResumeThread(OSThread* thread);
void OSSuspendThread(OSThread* thread);
s64 OSGetTime(void);
u32 OSGetTick(void);

extern s32 ganMapGPR[32];
extern System* gpSystem;
extern u32 gnFlagZelda;

// TODO: use decomp types and names
#define cur_thread  (*(volatile OSThread**)0x800000C0)
#define ex_handlers ((volatile OSExceptionHandler*)0x80003000)

#if IS_WII

int IOS_OpenAsync(const char* file, int mode, void* callback, void* callback_data);
int IOS_Open(const char* file, int mode);
int IOS_CloseAsync(int fd, void* callback, void* callback_data);
int IOS_Close(int fd);
int IOS_ReadAsync(int fd, void* data, size_t len, void* callback, void* callback_data);
int IOS_Read(int fd, void* data, size_t len);
int IOS_WriteAsync(int fd, void* data, size_t len, void* callback, void* callback_data);
int IOS_Write(int fd, void* data, size_t len);
int IOS_SeekAsync(int fd, int where, int whence, void* callback, void* callback_data);
int IOS_Seek(int fd, int where, int whence);
int IOS_IoctlAsync(int fd, int ioctl, void* buffer_in, size_t size_in, void* buffer_io, size_t size_out, void* callback,
                   void* callback_data);
int IOS_Ioctl(int fd, int ioctl, void* buffer_in, size_t size_in, void* buffer_io, size_t size_out);
int IOS_IoctlvAsync(int fd, int ioctl, int cnt_in, int cnt_io, void* argv, void* callback, void* callback_data);
int IOS_Ioctlv(int fd, int ioctl, int cnt_in, int cnt_io, void* argv);

int iosCreateHeap(void* heap, size_t size);
void* iosAllocAligned(int hid, size_t size, size_t page_size);
bool iosFree(int hid, void* ptr);

extern u32 reset_flag; // TODO: use decomp name

#define title_id             (*(volatile u32*)0x80003180)

#define ios_heap_addr        0x933E8000
#define allocMEM2(ptr, size) xlHeapTake((void**)(ptr), (0x70000000 | (size)))

#endif

#endif
