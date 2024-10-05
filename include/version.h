#ifndef _VERSION_H
#define _VERSION_H

#ifndef VC_VERSION
#error no vc version specified
#endif

#define NACJ 0
#define NACE 1
#define NARJ 2
#define NARE 3

#define IS_MM  (VC_VERSION == NARJ || VC_VERSION == NARE)
#define IS_OOT (VC_VERSION == NACJ || VC_VERSION == NACE)

#endif
