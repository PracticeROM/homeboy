#ifndef _OS_H
#define _OS_H

#include "types.h"

// os.h
#define OS_CACHED_REGION_PREFIX 0x8000
#define OS_BASE_CACHED          (OS_CACHED_REGION_PREFIX << 16)

// OSAddress.h
static inline void* OSPhysicalToCached(u32 ofs) {
    return (void*)(ofs + OS_BASE_CACHED);
}

// OSContext.h
typedef struct OSContext {
    /* 0x0 */ u32 gprs[32];
    /* 0x80 */ u32 cr;
    /* 0x84 */ u32 lr;
    /* 0x88 */ u32 ctr;
    /* 0x8C */ u32 xer;
    /* 0x90 */ f64 fprs[32];
    /* 0x190 */ u32 fpscr_pad;
    /* 0x194 */ u32 fpscr;
    /* 0x198 */ u32 srr0;
    /* 0x19C */ u32 srr1;
    /* 0x1A0 */ u16 mode;
    /* 0x1A2 */ u16 state;
    /* 0x1A4 */ u32 gqrs[8];
    /* 0x1C4 */ u32 psf_pad;
    /* 0x1C8 */ f64 psfs[32];
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
    /* 0x000 */ OSContext context;
    /* 0x2C8 */ u16 state;
    /* 0x2CA */ u16 flags;
    /* 0x2CC */ s32 suspend;
    /* 0x2D0 */ s32 priority;
    /* 0x2D4 */ s32 base;
    /* 0x2D8 */ u32 val;
    /* 0x2DC */ OSThreadQueue* queue;
    /* 0x2E0 */ struct OSThread* next;
    /* 0x2E4 */ struct OSThread* prev;
    /* 0x2E8 */ OSThreadQueue joinQueue;
    /* 0x2F0 */ struct OSMutex* mutex;
    /* 0x2F4 */ OSMutexQueue mutexQueue;
    /* 0x2FC */ struct OSThread* nextActive;
    /* 0x300 */ struct OSThread* prevActive;
    /* 0x304 */ u32* stackBegin;
    /* 0x308 */ u32* stackEnd;
    /* 0x30C */ s32 error;
    /* 0x310 */ void* specific[2];
} OSThread;

typedef void* (*OSThreadFunc)(void* arg);
typedef void (*OSExceptionHandler)(u8 type, OSContext* ctx);

#endif
