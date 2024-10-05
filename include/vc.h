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

#if VC_VERSION == NACJ
#define init_hook_addr       0x800078E8
#define IOS_OpenAsync_addr   0x800b884c
#define IOS_Open_addr        0x800b8964
#define IOS_CloseAsync_addr  0x800b8a84
#define IOS_Close_addr       0x800b8b44
#define IOS_ReadAsync_addr   0x800b8bec
#define IOS_Read_addr        0x800b8cec
#define IOS_WriteAsync_addr  0x800b8df4
#define IOS_Write_addr       0x800b8ef4
#define IOS_SeekAsync_addr   0x800b8ffc
#define IOS_Seek_addr        0x800b90dc
#define IOS_IoctlAsync_addr  0x800b91c4
#define IOS_Ioctl_addr       0x800b92fc
#define IOS_IoctlvAsync_addr 0x800b9568
#define IOS_Ioctlv_addr      0x800b964c
#define iosCreateHeap_addr   0x800b9810
#define iosAllocAligned_addr 0x800b9b44
#define iosFree_addr         0x800b9b48
#define ramSetSize_addr      0x80041d7c
#define xlHeapTake_addr      0x800810f8
#define heap_size_hook_addr  0x8008A164
#define reset_flag_addr      0x8025D0EC
#define gpSystem_ptr_addr    0x8025CFE0
#define N64_DRAM_SIZE        0x00800000
#define cpuMapObject_addr    0x8003c614
#define xlObjectMake_addr    0x800821f4
#define cpuSetDevicePut_addr 0x8003c7c8
#define cpuSetDeviceGet_addr 0x8003c7b0
#define xlHeapFree_addr      0x8008136c
#define OSCreateThread_addr  0x800929f4
#define OSResumeThread_addr  0x8009305c
#define OSSuspendThread_addr 0x800932f4
#elif VC_VERSION == NACE
#define init_hook_addr       0x800078E8
#define IOS_OpenAsync_addr   0x800b8858
#define IOS_Open_addr        0x800b8970
#define IOS_CloseAsync_addr  0x800b8a90
#define IOS_Close_addr       0x800b8b50
#define IOS_ReadAsync_addr   0x800b8bf8
#define IOS_Read_addr        0x800b8cf8
#define IOS_WriteAsync_addr  0x800b8e00
#define IOS_Write_addr       0x800b8f00
#define IOS_SeekAsync_addr   0x800b9008
#define IOS_Seek_addr        0x800b90e8
#define IOS_IoctlAsync_addr  0x800b91d0
#define IOS_Ioctl_addr       0x800b9308
#define IOS_IoctlvAsync_addr 0x800b9574
#define IOS_Ioctlv_addr      0x800b9658
#define iosCreateHeap_addr   0x800b981c
#define iosAllocAligned_addr 0x800b9b50
#define iosFree_addr         0x800b9b54
#define ramSetSize_addr      0x80041d98
#define xlHeapTake_addr      0x80081104
#define heap_size_hook_addr  0x8008a170
#define reset_flag_addr      0x8025D1EC
#define gpSystem_ptr_addr    0x8025D0E0
#define N64_DRAM_SIZE        0x00800000
#define cpuMapObject_addr    0x8003c630
#define xlObjectMake_addr    0x80082200
#define cpuSetDevicePut_addr 0x8003c7e4
#define cpuSetDeviceGet_addr 0x8003c7cc
#define xlHeapFree_addr      0x8008136c
#define OSCreateThread_addr  0x80092a00
#define OSResumeThread_addr  0x80093068
#define OSSuspendThread_addr 0x80093300
#elif VC_VERSION == NARJ
#define IOS_OpenAsync_addr   0x800c5430
#define IOS_Open_addr        0x800c5548
#define IOS_CloseAsync_addr  0x800c5668
#define IOS_Close_addr       0x800c5728
#define IOS_ReadAsync_addr   0x800c57d0
#define IOS_Read_addr        0x800c58d0
#define IOS_WriteAsync_addr  0x800c59d8
#define IOS_Write_addr       0x800c5ad8
#define IOS_SeekAsync_addr   0x800c5be0
#define IOS_Seek_addr        0x800c5cc0
#define IOS_IoctlAsync_addr  0x800c5da8
#define IOS_Ioctl_addr       0x800c5ee0
#define IOS_IoctlvAsync_addr 0x800c614c
#define IOS_Ioctlv_addr      0x800c6230
#define iosCreateHeap_addr   0x800c6608
#define iosAllocAligned_addr 0x800c693c
#define iosFree_addr         0x800c6940
#define ramSetSize_addr      0x8005083c
#define xlHeapTake_addr      0x800887e0
#define reset_flag_addr      0x80200830
#define gpSystem_ptr_addr    0x80200638
#define N64_DRAM_SIZE        0x00800000
#define cpuSetDevicePut_addr 0x8004B694
#define cpuSetDeviceGet_addr 0x8004B67C
#define xlObjectMake_addr    0x8008979C
#define cpuMapObject_addr    0x8004B27C
#define xlHeapFree_addr      0x80088A60
#define OSCreateThread_addr  0x8009b2d8
#define OSResumeThread_addr  0x8009b948
#define OSSuspendThread_addr 0x8009bbe0
#elif VC_VERSION == NARE
#define IOS_OpenAsync_addr   0x800c4dec
#define IOS_Open_addr        0x800c4f04
#define IOS_CloseAsync_addr  0x800c5024
#define IOS_Close_addr       0x800c50e4
#define IOS_ReadAsync_addr   0x800c518c
#define IOS_Read_addr        0x800c528c
#define IOS_WriteAsync_addr  0x800c5394
#define IOS_Write_addr       0x800c5494
#define IOS_SeekAsync_addr   0x800c559c
#define IOS_Seek_addr        0x800c567c
#define IOS_IoctlAsync_addr  0x800c5764
#define IOS_Ioctl_addr       0x800c589c
#define IOS_IoctlvAsync_addr 0x800c5b08
#define IOS_Ioctlv_addr      0x800c5bec
#define iosCreateHeap_addr   0x800c5fc4
#define iosAllocAligned_addr 0x800c62f8
#define iosFree_addr         0x800c62fc
#define ramSetSize_addr      0x800507c8
#define xlHeapTake_addr      0x80088790
#define reset_flag_addr      0x801FBA28
#define gpSystem_ptr_addr    0x801fb838
#define N64_DRAM_SIZE        0x00800000
#define cpuMapObject_addr    0x8004b208
#define xlObjectMake_addr    0x8008974c
#define cpuSetDevicePut_addr 0x8004b620
#define cpuSetDeviceGet_addr 0x8004b608
#define xlHeapFree_addr      0x80088a10
#define OSCreateThread_addr  0x8009afbc
#define OSResumeThread_addr  0x8009b624
#define OSSuspendThread_addr 0x8009b8bc
#endif

#define cur_thread_addr  0x800000C0
#define ex_handlers_addr 0x80003000
#define title_id_addr    0x80003180
#define ios_heap_addr    0x933e8000

typedef int (*iosCreateHeap_t)(void* heap, size_t size);
typedef void* (*iosAllocAligned_t)(int hid, size_t size, size_t page_size);
typedef bool (*iosFree_t)(int hid, void* ptr);
typedef int (*IOS_OpenAsync_t)(const char* file, int mode, void* callback, void* callback_data);
typedef int (*IOS_Open_t)(const char* file, int mode);
typedef int (*IOS_CloseAsync_t)(int fd, void* callback, void* callback_data);
typedef int (*IOS_Close_t)(int fd);
typedef int (*IOS_ReadAsync_t)(int fd, void* data, size_t len, void* callback, void* callback_data);
typedef int (*ios_read_t)(int fd, void* data, size_t len);
typedef int (*IOS_WriteAsync_t)(int fd, void* data, size_t len, void* callback, void* callback_data);
typedef int (*IOS_Write_t)(int fd, void* data, size_t len);
typedef int (*IOS_SeekAsync_t)(int fd, int where, int whence, void* callback, void* callback_data);
typedef int (*IOS_Seek_t)(int fd, int where, int whence);
typedef int (*IOS_IoctlAsync_t)(int fd, int ioctl, void* buffer_in, size_t size_in, void* buffer_io, size_t size_out,
                                void* callback, void* callback_data);
typedef int (*IOS_Ioctl_t)(int fd, int ioctl, void* buffer_in, size_t size_in, void* buffer_io, size_t size_out);
typedef int (*IOS_IoctlvAsync_t)(int fd, int ioctl, int cnt_in, int cnt_io, void* argv, void* callback,
                                 void* callback_data);
typedef int (*IOS_Ioctlv_t)(int fd, int ioctl, int cnt_in, int cnt_io, void* argv);
typedef bool (*ramSetSize_t)(void** dest, uint32_t size);
typedef bool (*xlHeapTake_t)(void** dest, uint32_t size);
typedef bool (*xlHeapFree_t)(void* ptr);
typedef int (*xlObjectMake_t)(void** obj, void* parent, _XL_OBJECTTYPE* class);
typedef int (*cpuMapObject_t)(Cpu* cpu, void* dev_p, uint32_t address_start, uint32_t address_end, uint32_t param_5);
typedef int (*cpuSetDevicePut_t)(Cpu* cpu, CpuDevice* dev, void* sb, void* sh, void* sw, void* sd);
typedef int (*cpuSetDeviceGet_t)(Cpu* cpu, CpuDevice* dev, void* lb, void* lh, void* lw, void* ld);
typedef void* (*OSThreadEntry_t)(void* arg);
typedef void (*OSCreateThread_t)(OSThread* thread, OSThreadEntry_t entry, void* arg, void* stack, size_t stack_size,
                                 int pri, int detached);
typedef void (*OSResumeThread_t)(OSThread* thread);
typedef void (*OSSuspendThread_t)(OSThread* thread);
typedef void (*ex_handler_t)(enum ppc_exception);

#define title_id             (*(uint32_t*)title_id_addr)
#define reset_flag           (*(uint32_t*)reset_flag_addr)

#define cur_thread           (*(OSThread**)cur_thread_addr)
#define ex_handlers          ((ex_handler_t*)ex_handlers_addr)
#define gpSystem             (*(System**)gpSystem_ptr_addr)
#define SYSTEM_CPU(pSystem)  ((void*)(((System*)(pSystem))->apObject[SOT_CPU]))

#define IOS_OpenAsync        ((IOS_OpenAsync_t)IOS_OpenAsync_addr)
#define IOS_Open             ((IOS_Open_t)IOS_Open_addr)
#define IOS_CloseAsync       ((IOS_CloseAsync_t)IOS_CloseAsync_addr)
#define IOS_Close            ((IOS_Close_t)IOS_Close_addr)
#define IOS_ReadAsync        ((IOS_ReadAsync_t)IOS_ReadAsync_addr)
#define IOS_Read             ((ios_read_t)IOS_Read_addr)
#define IOS_WriteAsync       ((IOS_WriteAsync_t)IOS_WriteAsync_addr)
#define IOS_Write            ((IOS_Write_t)IOS_Write_addr)
#define IOS_SeekAsync        ((IOS_SeekAsync_t)IOS_SeekAsync_addr)
#define IOS_Seek             ((IOS_Seek_t)IOS_Seek_addr)
#define IOS_IoctlAsync       ((IOS_IoctlAsync_t)IOS_IoctlAsync_addr)
#define IOS_Ioctl            ((IOS_Ioctl_t)IOS_Ioctl_addr)
#define IOS_IoctlvAsync      ((IOS_IoctlvAsync_t)IOS_IoctlvAsync_addr)
#define IOS_Ioctlv           ((IOS_Ioctlv_t)IOS_Ioctlv_addr)

#define iosCreateHeap        ((iosCreateHeap_t)iosCreateHeap_addr)
#define iosAllocAligned      ((iosAllocAligned_t)iosAllocAligned_addr)
#define iosFree              ((iosFree_t)iosFree_addr)

#define ramSetSize           ((ramSetSize_t)ramSetSize_addr)
#define xlHeapTake           ((xlHeapTake_t)xlHeapTake_addr)
#define xlHeapFree           ((xlHeapFree_t)xlHeapFree_addr)
#define xlObjectMake         ((xlObjectMake_t)xlObjectMake_addr)
#define cpuMapObject         ((cpuMapObject_t)cpuMapObject_addr)
#define cpuSetDevicePut      ((cpuSetDevicePut_t)cpuSetDevicePut_addr)
#define cpuSetDeviceGet      ((cpuSetDeviceGet_t)cpuSetDeviceGet_addr)
#define OSCreateThread       ((OSCreateThread_t)OSCreateThread_addr)
#define OSResumeThread       ((OSResumeThread_t)OSResumeThread_addr)
#define OSSuspendThread      ((OSSuspendThread_t)OSSuspendThread_addr)

#define allocMEM2(ptr, size) xlHeapTake((void**)(ptr), (0x70000000 | (size)))

#endif
