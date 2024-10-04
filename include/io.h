#ifndef _IO_H
#define _IO_H

#include <stdint.h>

// ipcclt.h
typedef struct IPCIOVector {
    /* 0x0 */ void* base;
    /* 0x4 */ uint32_t length;
} IPCIOVector;

#endif
