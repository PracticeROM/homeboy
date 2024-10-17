#ifndef _MCARDGCN_H
#define _MCARDGCN_H

#include "types.h"
#include "os.h"
#include "version.h"

#if IS_GC

// unused enums
typedef int MemCardCommand;
typedef int MemCardError;
typedef int MemCardMessage;

typedef struct CARDFileInfo {
    /* 0x00 */ s32 chan;
    /* 0x04 */ s32 fileNo;
    /* 0x08 */ s32 offset;
    /* 0x0C */ s32 length;
    /* 0x10 */ u16 iBlock;
} CARDFileInfo;

typedef struct MemCardGameInfo {
    /* 0x00 */ s32 configuration;
    /* 0x04 */ s32 size;
    /* 0x08 */ s32 offset;
    /* 0x0C */ char* buffer;
    /* 0x10 */ bool* writtenBlocks;
    /* 0x14 */ bool writtenConfig;
} MemCardGameInfo; // size = 0x18

typedef struct MemCardFileInfo {
    /* 0x000 */ s32 currentGame;
    /* 0x004 */ s32 fileSize;
    /* 0x008 */ char name[33];
    /* 0x02C */ s32 numberOfGames;
    /* 0x030 */ MemCardGameInfo game;
    /* 0x048 */ bool changedDate;
    /* 0x04C */ bool changedChecksum;
    /* 0x050 */ s32 gameSize[16];
    /* 0x090 */ s32 gameOffset[16];
    /* 0x0D0 */ s32 gameConfigIndex[16];
    /* 0x110 */ char gameName[16][33];
    /* 0x320 */ OSCalendarTime time;
    /* 0x348 */ CARDFileInfo fileInfo;
} MemCardFileInfo; // size = 0x35C

typedef struct _MCARD {
    /* 0x000 */ MemCardFileInfo file;
    /* 0x35C */ MemCardError error;
    /* 0x360 */ s32 slot;
    /* 0x364 */ bool (*pPollFunction)(void);
    /* 0x368 */ s32 pollPrevBytes;
    /* 0x36C */ s32 pollSize;
    /* 0x370 */ char pollMessage[256];
    /* 0x470 */ bool saveToggle;
    /* 0x474 */ char* writeBuffer;
    /* 0x478 */ char* readBuffer;
    /* 0x47C */ bool writeToggle;
    /* 0x480 */ bool soundToggle;
    /* 0x484 */ s32 writeStatus;
    /* 0x488 */ s32 writeIndex;
    /* 0x48C */ s32 accessType;
    /* 0x490 */ bool gameIsLoaded;
    /* 0x494 */ char saveFileName[256];
    /* 0x594 */ char saveComment[256];
    /* 0x694 */ char* saveIcon;
    /* 0x698 */ char* saveBanner;
    /* 0x69C */ char saveGameName[256];
    /* 0x79C */ s32 saveFileSize;
    /* 0x7A0 */ s32 saveGameSize;
    /* 0x7A4 */ bool bufferCreated;
    /* 0x7A8 */ s32 cardSize;
    /* 0x7AC */ bool wait;
    /* 0x7B0 */ bool isBroken;
    /* 0x7B4 */ s32 saveConfiguration;
} MemCard; // size = 0x7B8

#endif

#endif
