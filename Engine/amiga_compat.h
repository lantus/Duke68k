/*
 * amiga compatibility header. Takes care of some legacy code issues
 *  and incompatibilities at the source level.
 *
 *  Written by Ryan C. Gordon (icculus@clutteredmind.org)
 *
 * Please do NOT harrass Ken Silverman about any code modifications
 *  (including this file) to BUILD.
 */

/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */
  
#ifndef _INCLUDE_AMIGA_COMPAT_H_
#define _INCLUDE_AMIGA_COMPAT_H_

#if (!defined PLATFORM_AMIGA)
#error PLATFORM_AMIGA is not defined.
#endif

#if (!defined PLATFORM_TIMER_HZ)
#define PLATFORM_TIMER_HZ 100
#endif

#include <exec/types.h>
#include <stdio.h>

#define __int64 long long

#ifndef PLATFORM_SUPPORTS_SDL
typedef unsigned char   Uint8;
typedef signed char     Sint8;
typedef unsigned short  Uint16;
typedef signed short    Sint16;
typedef unsigned int    Uint32;
typedef signed int      Sint32;
#endif

#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <assert.h>

extern const int hbits[];

/*
  Do some bitwise magic to approximate an algebraic (sign preserving)
  right shift.
 */
#define shift_algebraic_right(value,distance) \
(((value) >> (distance))| \
 (hbits[(distance) + (((value) & 0x80000000) >> 26)]))

/* !!! remove me later! */
/* !!! remove me later! */
/* !!! remove me later! */
#define outpw(x, y)   printf("outpw(0x%X, 0x%X) call in %s, line %d.\n",    \
			      (x), (y), __FILE__, __LINE__)

#define koutpw(x, y)  printf("koutpw(0x%X, 0x%X) call in %s, line %d.\n",   \
			      (x), (y), __FILE__, __LINE__)

#define outb(x, y)    printf("outb(0x%X, 0x%X) call in %s, line %d.\n",     \
			      (x), (y), __FILE__, __LINE__)

#define koutb(x, y)   printf("koutb(0x%X, 0x%X) call in %s, line %d.\n",    \
			      (x), (y), __FILE__, __LINE__)

#define outp(x, y)    printf("outp(0x%X, 0x%X) call in %s, line %d.\n",     \
			      (x), (y), __FILE__, __LINE__)

#define koutp(x, y)
/* !!! */
 /*printf("koutp(0x%X, 0x%X) call in %s, line %d.\n",
		      //        (x), (y), __FILE__, __LINE__) */

#define kinp(x)       _kinp_handler((x), __FILE__, __LINE__)
#define inp(x)        _inp_handler((x), __FILE__, __LINE__)

int _inp_handler(int port, char *source_file, int source_line);
int _kinp_handler(int port, char *source_file, int source_line);
/* !!! remove me later! */
/* !!! remove me later! */
/* !!! remove me later! */

void kprintf(char *b,...);


#define __far
//#define __interrupt
#define interrupt

#ifdef __MORPHOS__
#ifndef __interrupt
#define __interrupt
#endif
#endif

#define far
#define kmalloc(x) malloc(x)
#define kkmalloc(x) malloc(x)
#define kfree(x) free(x)
#define kkfree(x) free(x)
#define FP_OFF(x) ((long) (x))

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef strcmpi
#define strcmpi(x, y) strcasecmp(x, y)
#endif

/* damned -ansi flag... :) */
int stricmp(const char *x, const char *y);

#if (defined __STRICT_ANSI__)
#define inline __inline__
#endif

#define printext16 printext256
#define printext16_noupdate printext256_noupdate

/* Other DOSisms. See unix_compat.c for implementation. */
long filelength(int fhandle);

/* !!! need an implementation of findfirst()/findnext()! */
/*     Look for references to _dos_findfirst() in build.c! */

#if (!defined S_IREAD)
#define S_IREAD S_IRUSR
#endif

#ifndef getch
#define getch() getchar()
#endif

#ifndef max
#define max(x, y)  (((x) > (y)) ? (x) : (y))
#endif

#ifndef min
#define min(x, y)  (((x) < (y)) ? (x) : (y))
#endif

#endif





