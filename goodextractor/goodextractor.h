/*
** goodextractor.h
** Header for goodextractor.c
** Copyright 2006 (c) Zeb Rasco
*/

#include <windows.h>

typedef struct ROMFILE_INFO_STRUCT
/* Info on the rom file(most for future use) */
{
	/* Generic tags */
	BOOL bVerified;				/* [!] */
	BOOL bAlternate;			/* [a] */
	BOOL bBaddump;				/* [b] */
	BOOL bHacked;				/* [h] */
	BOOL bFixed;				/* [f] */
	BOOL bOverdump;				/* [o] */
	BOOL bPirated;				/* [p] */
	BOOL bTrained;				/* [t] */
	BOOL bOldTranslation;		/* [T-] */
	BOOL bNewTranslation;		/* [T+] */

	/* Generic codes */
	BOOL bUnlicensed;			/* (Unl) */
	BOOL bUnknownCountry;		/* (Unk) */
	BOOL bUnknownYear;			/* (-) */
	int nLanguages;				/* (M#) */
	int nSize;					/* (??k) */
	int nChecksum;				/* (#####) */

	/* Colecovision */
	BOOL bADAM;					/* (Adam) */
	/* Gameboy */
	BOOL bGBAEnhanced;			/* [C] */
	BOOL bSGBEnhanced;			/* [S] */
	BOOL bBungfix;				/* [BF] */

	/* Gameboy advanced */
	int nIntroHacks;			/* [hl??] */

    /* Sega megadrive/genesis */
	BOOL bFaultyChecksum;		/* [c] */
	BOOL bBadChecksum;			/* [x] */
	BOOL bCountries;			/* [R-] */

	/* Neo-geo pocket color */
	BOOL bMonoOnly;				/* [M] */

	/* Nintendo */
	BOOL bPC10;					/* (PC10) */
	int nPRG;					/* (PRG#) */
	BOOL bVSUnisystem;			/* (VS) */
	BOOL bFFE;					/* [hFFE] */
	int nMapperHack;			/* [hM##] */

	/* Super Nintendo */
	BOOL bBroadcastSat;			/* (BS) */
	BOOL bSufamiTurbo;			/* (ST) */
	BOOL bNintendoPowerOnly;	/* (NP) */

	/* Internal use */
	BOOL bHackOrPrototype;
	char szCountryCodes[16];	/* Any code not covered by now goes here */
	int nTags;
} ROMFILE_INFO_STRUCT;

typedef struct COMMAND_LINE_OPTIONS
/* Command-line options structure */
{
	BOOL bShowNoTags;
	BOOL bVerified;
	BOOL bNoHacksOrPrototypes;
	BOOL bNoOldPRG;
	BOOL bExtract;
	int nCountryCodes;
	char szCountryCode[32][16];
	char szSourcePath[512];
	char szDestPath[512];
} COMMAND_LINE_OPTIONS;

int parse(int, char*[], char*, char*, char*);
int usage(char);
int OperateResult(WIN32_FIND_DATA*, COMMAND_LINE_OPTIONS*);
void GetROMTagInfo(char*, ROMFILE_INFO_STRUCT*);