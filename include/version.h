#ifndef _VERSION_H
#define _VERSION_H

#define D43J 1 // GameCube OOT Master Quest Japan
#define D43E 2 // GameCube OOT Master Quest US
#define PZLJ 3 // GameCube OOT Collector's Edition Japan
#define PZLE 4 // GameCube OOT Collector's Edition US
#define NACJ 5 // Wii OOT Japan
#define NACE 6 // Wii OOT US
#define NARJ 7 // Wii MM Japan
#define NARE 8 // Wii MM US

#if VC_VERSION == D43J
#define IS_GC  1
#define IS_OOT 1
#elif VC_VERSION == D43E
#define IS_GC  1
#define IS_OOT 1
#elif VC_VERSION == PZLJ
#define IS_GC  1
#define IS_OOT 1
#elif VC_VERSION == PZLE
#define IS_GC  1
#define IS_OOT 1
#elif VC_VERSION == NACJ
#define IS_WII 1
#define IS_OOT 1
#elif VC_VERSION == NACE
#define IS_WII 1
#define IS_OOT 1
#elif VC_VERSION == NARJ
#define IS_WII 1
#define IS_MM  1
#elif VC_VERSION == NARE
#define IS_WII 1
#define IS_MM  1
#else
#error "Unknown version"
#endif

#endif
