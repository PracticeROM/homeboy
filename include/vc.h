#ifndef _VC_H
#define _VC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cpu.h"
#include "version.h"

#define INIT __attribute__((section(".init")))

enum ppc_exception {
    EX_RESET,
    EX_MACH_CHECK,
    EX_DSI,
    EX_ISI,
    EX_EXT_INTR,
    EX_ALIGN,
    EX_PROG,
    EX_FP_UNAVAIL,
    EX_DECR,
    EX_SYSCALL,
    EX_TRACE,
    EX_MAX
};

// system.h
typedef enum SystemMode {
    SM_NONE = -1,
    SM_RUNNING = 0,
    SM_STOPPED = 1,
} SystemMode;

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

typedef int32_t SystemRomType; // big enum, no need to have it here for now

#if IS_OOT

// system.h
typedef struct System {
    /* 0x00 */ bool bException;
    /* 0x04 */ SystemMode eMode;
    /* 0x08 */ SystemObjectType storageDevice;
    /* 0x0C */ SystemRomType eTypeROM;
    /* 0x10 */ void* apObject[SOT_COUNT];
    /* 0x6C */ int32_t unk_6C;
    /* 0x70 */ uint64_t nAddressBreak;
    /* 0x78 */ int32_t unk_78[19];
    /* 0xC4 */ void* pSound;
    /* 0xC8 */ uint8_t anException[16];
} System; // size = 0xD8

#elif IS_MM

//! TODO: figure out the System struct on MM
typedef struct System {
    char unk_0x00[0x28];
    void* apObject[SOT_COUNT];
} System;

#endif

#define SYSTEM_CPU(pSystem) ((void*)(((System*)(pSystem))->apObject[SOT_CPU]))

// xlObject.h
typedef struct _XL_OBJECTTYPE _XL_OBJECTTYPE;

typedef int (*EventFunc)(void* pObject, int nEvent, void* pArgument);

struct _XL_OBJECTTYPE {
    /* 0x0 */ char* szName;
    /* 0x4 */ int32_t nSizeObject;
    /* 0x8 */ _XL_OBJECTTYPE* pClassBase;
    /* 0xC */ EventFunc pfEvent;
}; // size = 0x10

// OSContext.h
typedef struct OSContext {
    /* 0x0 */ uint32_t gprs[32];
    /* 0x80 */ uint32_t cr;
    /* 0x84 */ uint32_t lr;
    /* 0x88 */ uint32_t ctr;
    /* 0x8C */ uint32_t xer;
    /* 0x90 */ double fprs[32];
    /* 0x190 */ uint32_t fpscr_pad;
    /* 0x194 */ uint32_t fpscr;
    /* 0x198 */ uint32_t srr0;
    /* 0x19C */ uint32_t srr1;
    /* 0x1A0 */ uint16_t mode;
    /* 0x1A2 */ uint16_t state;
    /* 0x1A4 */ uint32_t gqrs[8];
    /* 0x1C4 */ uint32_t psf_pad;
    /* 0x1C8 */ double psfs[32];
} OSContext;

// OSThread.h
typedef struct OSThreadQueue {
    /* 0x0 */ struct OSThread* head;
    /* 0x4 */ struct OSThread* tail;
} OSThreadQueue;

typedef struct OSMutexQueue {
    /* 0x0 */ struct OSMutex* head;
    /* 0x4 */ struct OSMutex* tail;
} OSMutexQueue;

typedef struct OSThread {
    OSContext context;
    /* 0x2C8 */ uint16_t state;
    /* 0x2CA */ uint16_t flags;
    /* 0x2CC */ int32_t suspend;
    /* 0x2D0 */ int32_t priority;
    /* 0x2D4 */ int32_t base;
    /* 0x2D8 */ uint32_t val;
    /* 0x2DC */ OSThreadQueue* queue;
    /* 0x2E0 */ struct OSThread* next;
    /* 0x2E4 */ struct OSThread* prev;
    /* 0x2E8 */ OSThreadQueue joinQueue;
    /* 0x2F0 */ struct OSMutex* mutex;
    /* 0x2F4 */ OSMutexQueue mutexQueue;
    /* 0x2FC */ struct OSThread* nextActive;
    /* 0x300 */ struct OSThread* prevActive;
    /* 0x304 */ uint32_t* stackBegin;
    /* 0x308 */ uint32_t* stackEnd;
    /* 0x30C */ int32_t error;
    /* 0x310 */ void* specific[2];
} OSThread;

typedef void (*ex_handler_t)(enum ppc_exception);

#if IS_OOT
bool cpuExecuteUpdate(Cpu* cpu, int32_t* pnAddressGCN, int32_t nCount);
#elif IS_MM
bool cpuExecuteUpdate(Cpu* cpu, int32_t* pnAddressGCN, int64_t nTime);
#endif
int cpuMapObject(Cpu* cpu, void* dev_p, uint32_t address_start, uint32_t address_end, uint32_t param_5);
int cpuSetDeviceGet(Cpu* cpu, CpuDevice* dev, void* lb, void* lh, void* lw, void* ld);
int cpuSetDevicePut(Cpu* cpu, CpuDevice* dev, void* sb, void* sh, void* sw, void* sd);
bool cpuFindFunction(Cpu* cpu, int theAddress, CpuFunction** tree_node);
bool ramSetSize(void** dest, uint32_t size);
bool xlHeapTake(void** dest, uint32_t size);
bool xlHeapFree(void* ptr);
int xlObjectMake(void** obj, void* parent, _XL_OBJECTTYPE* class);

void DCStoreRange(const void* buf, uint32_t len);
void ICInvalidateRange(const void* buf, uint32_t len);

void OSReport(const char* msg, ...);
void OSCreateThread(OSThread* thread, void* (*func)(void*), void* arg, void* stack, size_t stack_size, int pri,
                    int detached);
void OSResumeThread(OSThread* thread);
void OSSuspendThread(OSThread* thread);
int64_t OSGetTime(void);
uint32_t OSGetTick(void);

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

extern int32_t ganMapGPR[32];
extern System* gpSystem;
extern uint32_t reset_flag; // TODO: use decomp name

// TODO: use decomp types and names
#define cur_thread           (*(volatile OSThread**)0x800000C0)
#define ex_handlers          ((volatile ex_handler_t*)0x80003000)
#define title_id             (*(volatile uint32_t*)0x80003180)

#define ios_heap_addr        0x933E8000
#define allocMEM2(ptr, size) xlHeapTake((void**)(ptr), (0x70000000 | (size)))

#endif
