#ifndef _VERSION_H
#define _VERSION_H

#ifndef VC_VERSION
#error no vc version specified
#endif

#define D43J   0 // GameCube OOT Master Quest Japan
#define NACJ   1 // Wii OOT Japan
#define NACE   2 // Wii OOT US
#define NARJ   3 // Wii MM Japan
#define NARE   4 // Wii MM US

#define IS_GC  (VC_VERSION == D43J)
#define IS_WII (VC_VERSION == NACJ || VC_VERSION == NACE || VC_VERSION == NARJ || VC_VERSION == NARE)

#define IS_MM  (VC_VERSION == NARJ || VC_VERSION == NARE)
#define IS_OOT (VC_VERSION == D43J || VC_VERSION == NACJ || VC_VERSION == NACE)

#endif
