#ifndef _FRAME_H
#define _FRAME_H

#include "types.h"
#include "version.h"

#if IS_GC

typedef struct Frame {
    /* 0x00000 */ u8 pad1[0x124];
    /* 0x00124 */ f32 rDepth;
    /* 0x00128 */ f32 rDelta;
    /* 0x0012C */ u8 pad2[0x3D024];
} Frame; // size = 0x3D150

#elif IS_WII && IS_OOT

typedef struct Frame {
    /* 0x00000 */ u8 pad1[0x148];
    /* 0x00148 */ f32 rDepth;
    /* 0x0014C */ f32 rDelta;
    /* 0x00150 */ u8 pad2[0x3D000];
} Frame; // size = 0x3D150

#elif IS_WII && IS_MM

typedef struct Frame {
    /* 0x00000 */ u8 pad1[0x1DC];
    /* 0x001DC */ f32 rDepth;
    /* 0x001E0 */ f32 rDelta;
    /* 0x001E4 */ u8 pad2[0x557D4];
} Frame; // size = 0x559B8

#endif

#endif
