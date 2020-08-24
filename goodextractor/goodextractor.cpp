/*
** GoodExtractor
** Win32 command-line utility to extract all relevant files from 7-zip goodsets
** Usage: goodextractor <Source folder> <Destination folder> [options]
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stdafx.h"
#include "goodextractor.h"

extern "C"
{
#include "../lzmasdk/7zip/Archive/7z_C/7zCrc.h"
#include "../lzmasdk/7zip/Archive/7z_C/7zExtract.h"
#include "../lzmasdk/7zip/Archive/7z_C/7zIn.h"
}

/* 7-zip related declarations */
typedef struct _CFileInStream
{
  ISzInStream InStream;
  FILE *File;
} CFileInStream;

#ifdef _LZMA_IN_CB

#define kBufferSize (1 << 12)
Byte g_Buffer[kBufferSize];

SZ_RESULT SzFileReadImp(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc;
  if (maxRequiredSize > kBufferSize)
    maxRequiredSize = kBufferSize;
  processedSizeLoc = fread(g_Buffer, 1, maxRequiredSize, s->File);
  *buffer = g_Buffer;
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#else

SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc = fread(buffer, 1, size, s->File);
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#endif

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;
  int res = fseek(s->File, (long)pos, SEEK_SET);
  if (res == 0)
    return SZ_OK;
  return SZE_FAIL;
}


int main(int argc, char *argv[])
{
	COMMAND_LINE_OPTIONS clo;
	BOOL bLoop = TRUE, bQuote = FALSE;
	int count = 0;
	char szSwitches[512];

	memset(&szSwitches,0,sizeof(szSwitches));
	
	memset(&clo,0,sizeof(clo));

	printf("GoodExtractor .01 Copyright 2006 (c) Zeb Rasco\n\n");
	
	if (argc < 3)
		usage(0);
	else
	{
		/* Parse the command-line first */
		while(count < argc - 1)
		{
			count++;

			if (argv[count][0] == '-' && 
				(argv[count][2] == '\0'))
			/* Option switch */
			{
				strcat(szSwitches,argv[count]);
				strcat(szSwitches," ");

				if (argv[count][1] == 'c')
				/* Country code argument */
				{
					char *ccode = argv[count + 1];

					if (ccode == NULL)
					/* Operator error check! */
					{
						usage('c');
						return 1;
					}
					else
					/* Check some more */
					{
						int len = strlen(ccode);

						if (strlen(ccode) > 3)
						/* No country code is bigger then 3 bytes */
						{
							usage('c');
							return 1;
						}
						else
						/* OK up to this point, we can pretty much assume its valid */
						{
							count++;
							strcat(szSwitches,argv[count]);
							strcat(szSwitches," ");

							strcpy(clo.szCountryCode[clo.nCountryCodes],argv[count]);
							clo.nCountryCodes++;
						}
					}
				}
				else
				{
					strcat(szSwitches,argv[count]);
					strcat(szSwitches," ");

					if (argv[count][1] == 'v')
					/* Use verified dumps only */
						clo.bVerified = TRUE;
					else if (argv[count][1] == 'n')
					/* Extract dumps with no tags */
						clo.bShowNoTags = TRUE;
					else if (argv[count][1] == 'h')
					/* Skip prototype/hacks */
						clo.bNoHacksOrPrototypes = TRUE;
					else if (argv[count][1] == 'p')
					/* Extract only the highest PRG # */
						clo.bNoOldPRG = TRUE;
					else if (argv[count][1] == 'e')
					/* Extract files(will list by default) */
						clo.bExtract = TRUE;
				}
			}

			else
			/* One of the folders */
			{
				if (!clo.szSourcePath[0])
					strcpy(clo.szSourcePath,argv[count]);
				else if (!clo.szDestPath[0])
					strcpy(clo.szDestPath,argv[count]);
			}
		}

		if (!clo.szSourcePath[0] || !clo.szDestPath[0] || !szSwitches[0])
			usage(0);
		else
		/* All command-line options are OK! */
		{
			WIN32_FIND_DATA aDt;
			HANDLE hSearch;
			char szSourcePathExt[520];

			/* Enumerate files in source folder */
			strcpy(szSourcePathExt,clo.szSourcePath);
			strcat(szSourcePathExt,"\\*.7z");

			hSearch = FindFirstFile(szSourcePathExt, &aDt);

			if ( hSearch != INVALID_HANDLE_VALUE )
			/* Go through every .7z file in the specified folder */
			{
				OperateResult(&aDt,&clo);

				while (FindNextFile(hSearch, &aDt))
				/* Keep going... */
				{
					OperateResult(&aDt,&clo);
				}

				FindClose(hSearch);

			}
			else
			{
				fprintf(stderr,"Error: Folder doesn't exist, or no 7-zip files in source folder.\n");
				return 1;
			}

		}
	}

	return 0;
}

int usage(char error)
{
	if (error == 'c')
	/* Invalid country code */
		fprintf(stderr,"Error: Invalid country code specified.\n\n");

	fprintf(stderr,"Usage: goodextractor <source folder> <destination folder> [options]\n\n");
	fprintf(stderr,"Options:\n\n");
	fprintf(stderr,"-c (code): Extract dump(s) by country code(U for USA, J for Japan, etc...)\n");
	fprintf(stderr,"-v: Extract verified dump(s)\n");
	fprintf(stderr,"-n: Extract dump(s) with no tags\n");
	fprintf(stderr,"-h: Skip hacks and prototypes\n");
	fprintf(stderr,"-p: Skip redundant PRG# ROMs(except verified with -v), only keep highest.\n");

	fprintf(stderr,"\nNotes:\n\t The -c parameter may be used multiple times.\n");
	fprintf(stderr,"\tUsing -v and -p together will extract both the highest PRG and verified dumps.\n");

	return 0;
}

int OperateResult(WIN32_FIND_DATA *paDt, COMMAND_LINE_OPTIONS *pclo)
/*
** Called for every 7-zip file specified in the source folder
** Extracts every ROM using the specified command-line options
*/
{
	char szFullName[1024];
	ROMFILE_INFO_STRUCT RIS;

	memset(&RIS,0,sizeof(RIS));

	CFileInStream archiveStream;
	CArchiveDatabaseEx db;
	SZ_RESULT res;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	sprintf(szFullName,"%s\\%s",pclo->szSourcePath,paDt->cFileName);

	printf("Opening %s...",szFullName);

	/* Open the archive */
    
	archiveStream.File = fopen(szFullName,"rb");

	if (archiveStream.File == 0)
	/* Can't open file! */
	{
		fprintf(stderr,"Can't open!\n");
		return 1;
	}

	archiveStream.InStream.Read = SzFileReadImp;
	archiveStream.InStream.Seek = SzFileSeekImp;

	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;

	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	InitCrcTable();
	SzArDbExInit(&db);
	res = SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp);

	if (res == SZ_OK)
	/* Ready to rock! */
	{
		UInt32 count, blockindex = 0xFFFFFFFF;
		int count2, nHighestPRG, nHighestPRGIndex;
		Byte *outBuffer = 0;
		size_t outBufferSize = 0;



		printf("has %d files\n",db.Database.NumFiles);

		nHighestPRG = 0;
		nHighestPRGIndex = 0;

		for (count = 0; count < db.Database.NumFiles; count++)
		/* Go thru each compressed file */
		{
			BOOL bExtract = FALSE;
			size_t offset;
			size_t outSizeProcessed;
			CFileItem *f = db.Database.Files + count;

			memset(&RIS,0,sizeof(RIS));

			GetROMTagInfo(f->Name,&RIS);

			if (RIS.nPRG > nHighestPRG)
			{
				nHighestPRG = RIS.nPRG;
				nHighestPRGIndex = count;
			}

			if (RIS.bHackOrPrototype && pclo->bNoHacksOrPrototypes)
			/* We can go ahead and skip this one */
				continue;

			/* If it has a matching country code, we're good */
			for (count2 = 0; count2 < pclo->nCountryCodes; count2++)
			{
				if (strstr(RIS.szCountryCodes,pclo->szCountryCode[count2]) ||
					(pclo->szCountryCode[count2][0] == 'U' &&
					pclo->szCountryCode[count2][1] == 'n' &&
					pclo->szCountryCode[count2][2] == 'k' &&
					RIS.bUnknownCountry == TRUE))
				/* Found matching country code */
				{
					if (pclo->bVerified == TRUE)
					/* We only want verified ROM's with 1 tag(The verify tag!) */
					{
						if (RIS.bVerified == TRUE && RIS.nTags == 1)
						{
							if (pclo->bExtract == TRUE)
							{
								bExtract = TRUE;
							}
							else
							{
								printf("\t%s\n",f->Name);
							}
						}
					}
					if (pclo->bShowNoTags == TRUE)
					/* If its not verified, no tags is our next best bet */
					{
						if (RIS.nTags == 0)
						{
							if (pclo->bExtract == TRUE)
							{
								bExtract = TRUE;
							}
							else
								printf("\t%s\n",f->Name);
						}
						else
							continue;
					}
				}
			}

			if (pclo->nCountryCodes == 0)
			/* No country codes specified */
			{
				if (pclo->bVerified == TRUE)
				/* Extract all verified ROM's */
				{
					if (RIS.bVerified == TRUE)
						if (pclo->bExtract == TRUE)
						{
							bExtract = TRUE;
						}
						else
							printf("\t%s\n",f->Name);
				}
				else
				/* Extract all ROM's */
				{
					if (pclo->bExtract == TRUE)
					{
						bExtract = TRUE;
					}
					else
						printf("\t%s\n",f->Name);
				}

			}

			if (bExtract == TRUE)
			/* Extract the file here */
			{
				printf("Extracting %s to %s...\n",f->Name,pclo->szDestPath);
				res = SzExtract(&archiveStream.InStream,&db,count,&blockindex,&outBuffer,
					&outBufferSize,&offset,&outSizeProcessed,&allocImp,&allocTempImp);

				if (res != SZ_OK)
				{
					printf("Error! res != SZ_OK");
					break;
				}
				else
				{
					FILE *outputHandle;
					UInt32 processedSize;
					char *filename = f->Name;
					size_t nameLen = strlen(f->Name);
					char fullfilename[1024];

					for (; nameLen > 0; nameLen--)
					{
						if (f->Name[nameLen - 1] == '/')
						{
							filename = f->Name + nameLen;
							break;
						}
					}

					sprintf(fullfilename,"%s%s",pclo->szDestPath,filename);

					outputHandle = fopen(fullfilename,"wb+");

					if (outputHandle == 0)
					{
						printf("Can not open output file!");
						res = SZE_FAIL;
						break;
					}

					processedSize = fwrite(outBuffer + offset, 1, outSizeProcessed, outputHandle);

					if (processedSize != outSizeProcessed)
					{
						printf("Can not write output file");
						res = SZE_FAIL;
						break;
					}

					if (fclose(outputHandle))
					{
						printf("Can not close output file");
						res = SZE_FAIL;
						break;
					}

				}
			}
		}

		allocImp.Free(outBuffer);
	}

	SzArDbExFree(&db, allocImp.Free);

	fclose(archiveStream.File);

	if (res == SZ_OK)
	{
		printf("\n");
		return 0;
	}

	if (res == SZE_OUTOFMEMORY)
		fprintf(stderr,"can not allocate memory");
	else
	{
		fprintf(stderr,"\nERROR #%d\n", res);
	}

	return 1;
}

void GetROMTagInfo(char *pszFileName, ROMFILE_INFO_STRUCT *pRIS)
/*
** GetROMTagInfo()
** Takes the specified file name and parses the goodset tags
*/
{
	int count, workcount = 0, tagcount = 0;
	BOOL bWork = FALSE;
	char ch;
	char szWork[256];

	memset(szWork,0,sizeof(szWork));

	for (count = 0; pszFileName[count]; count++)
	{
		ch = pszFileName[count];

		if (bWork == TRUE)
		{
			szWork[workcount] = ch;
			workcount++;
		}

		if (ch == '(' || ch == '[')
		{
			bWork = TRUE;
			tagcount++;
		}
		else if (ch == ')' || ch == ']')
		/* End of tag/code */
		{
			tagcount--;

			if (tagcount >= 1)
				continue;

			workcount--;									/* Nevermind! ;) */

			bWork = FALSE;
			szWork[workcount] = 0;
            
			if (ch == ']')
			/* Working with a tag */
			{
				pRIS->nTags++;

				switch (szWork[0])
				{
					case '!':
						pRIS->bVerified = TRUE;
					break;
					case 'a':
						pRIS->bAlternate = TRUE;
					break;
					case 'b':
						pRIS->bBaddump = TRUE;
					break;
					case 'f':
						pRIS->bFixed = TRUE;
					break;
					case 'h':
						if (szWork[1] == 'l')
							pRIS->nIntroHacks = atoi(&szWork[2]);
						else if (szWork[1] == 'F' && szWork[2] == 'F' &&
							szWork[3] == 'E')
							pRIS->bFFE = TRUE;
						else if (szWork[1] == 'M')
							pRIS->nMapperHack = atoi(&szWork[2]);
						else
							pRIS->bHacked = TRUE;
					break;
					case 'o':
						pRIS->bOverdump = TRUE;
					break;
					case 'p':
						pRIS->bPirated = TRUE;
					break;
					case 't':
						pRIS->bTrained = TRUE;
					break;
					case 'T':
					{
						if (szWork[1] == '-')
							pRIS->bOldTranslation = TRUE;
						else if (szWork[1] == '+')
							pRIS->bNewTranslation = TRUE;
					}
					break;

					/* GB Advanced-specific codes */
					case 'C':
						pRIS->bGBAEnhanced = TRUE;
					break;
					case 'S':
						pRIS->bSGBEnhanced = TRUE;
					break;
					case 'B':
						if (szWork[1] == 'F')
							pRIS->bBungfix = TRUE;
					break;

					/* Megadrive / Sega Genesis */
					case 'c':
						pRIS->bFaultyChecksum = TRUE;
					break;
					case 'x':
						pRIS->bBadChecksum = TRUE;
					break;
					case 'R':
						if (szWork[1] == '-')
							pRIS->bCountries = TRUE;
					break;

					/* Neo-Geo Pocket Color */
					case 'M':
						pRIS->bMonoOnly = TRUE;
					break;
					
					/* Unrecognized tag */
					default:
						pRIS->nTags--;
					break;
				}
			}
			else
			/* Working with a code(check against all possible codes first */
			{
				if (szWork[0] == 'U' && szWork[1] == 'n'
					&& szWork[2] == 'l')
					pRIS->bUnlicensed = TRUE;
				else if (szWork[0] == 'U' && szWork[1] == 'n' &&
					szWork[3] == 'k')
					pRIS->bUnknownCountry = TRUE;
				else if (szWork[0] == '-')
					pRIS->bUnknownYear = TRUE;
				else if (szWork[0] == 'M' && (szWork[2] == 0 || szWork[3] == 0
					|| szWork[4] == 0 || szWork[5] == 0))
					pRIS->nLanguages = atoi(&szWork[1]);
				else if (szWork[workcount - 1] == 'k' &&
					szWork[workcount - 2] != 'c' &&
					szWork[workcount - 3] != 'a')
					/* k argument w/ protection against false-positive from hacks */
					sscanf(szWork,"%dk",&pRIS->nSize);
				else if (szWork[0] == 'A' && szWork[1] == 'd'
					&& szWork[2] == 'a' && szWork[3] == 'm' &&
					szWork[4] == '\0')
					pRIS->bADAM = TRUE;
				else if (szWork[0] == 'P' &&
					szWork[1] == 'R' && szWork[2] == 'G')
				/* PRG dump(NES only) */
					pRIS->nPRG = atoi(&szWork[3]);
				else if (szWork[0] == 'P' &&
					szWork[1] == 'C' && szWork[2] == '1' && 
					szWork[3] == '0')
				/* PC10 dump(NES only) */
					pRIS->bPC10 = TRUE;
				else if (szWork[0] == 'V' && szWork[1] == 'S')
					pRIS->bVSUnisystem = TRUE;
				else if (szWork[0] == 'B' && szWork[1] == 'S')
					pRIS->bBroadcastSat = TRUE;
				else if (szWork[0] == 'S' && szWork[1] == 'T')
					pRIS->bSufamiTurbo = TRUE;
				else if (szWork[0] == 'N' && szWork[1] == 'P')
					pRIS->bNintendoPowerOnly = TRUE;
				else
				/* Should be a country code! If 3 bytes or less that is :P */
				{
					if (strlen(szWork) <= 3)
						strcpy(pRIS->szCountryCodes,szWork);
					else
					/* Check for hacks, prototypes, etc */
					{
						strlwr(szWork);

						if (strstr(szWork,"hack") ||
							strstr(szWork,"prototype"))
						/* Hack/prototype found */
							pRIS->bHackOrPrototype = TRUE;
					}
				}
			}

			memset(szWork,0,sizeof(szWork));
			workcount = 0;
		}
	}
}