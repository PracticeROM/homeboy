#ifndef _VERSION_H
#define _VERSION_H

#ifndef VC_VERSION
#error no vc version specified
#endif

#define D43J   0 // GameCube OOT Master Quest Japan
#define D43E   1 // GameCube OOT Master Quest US
#define PZLJ   2 // GameCube OOT Collector's Edition Japan
#define PZLE   3 // GameCube OOT Collector's Edition US
#define NACJ   4 // Wii OOT Japan
#define NACE   5 // Wii OOT US
#define NARJ   6 // Wii MM Japan
#define NARE   7 // Wii MM US

#define IS_GC  (VC_VERSION == D43J || VC_VERSION == D43E || VC_VERSION == PZLJ || VC_VERSION == PZLE)
#define IS_WII (VC_VERSION == NACJ || VC_VERSION == NACE || VC_VERSION == NARJ || VC_VERSION == NARE)

#define OOT    0 // Ocarina of Time
#define MM     1 // Majora's Mask

#define IS_MM  (GAME == MM)
#define IS_OOT (GAME == OOT)

#endif
