#ifndef _INCLUDE_PLATFORM_H_
#define _INCLUDE_PLATFORM_H_

#if (defined PLATFORM_WIN32)
#include "win32_compat.h"
#elif (defined PLATFORM_UNIX)
#include "unix_compat.h"
#elif (defined PLATFORM_DOS)
#include "doscmpat.h"
#elif (defined PLATFORM_AMIGA)
#include "amiga_compat.h"
#else
#error Define your platform!
#endif

#if (!defined __EXPORT__)
#define __EXPORT__
#endif

#if ((defined PLATFORM_SUPPORTS_SDL) && (!defined PLATFORM_TIMER_HZ))
#define PLATFORM_TIMER_HZ 1000
#endif

#if (!defined PLATFORM_TIMER_HZ)
#error You need to define PLATFORM_TIMER_HZ for your platform.
#endif

#if (defined __WATCOMC__)
#define snprintf _snprintf
#endif

static __inline unsigned SwapSHORT(short val)
{
	__asm __volatile
	(
		"ror.w	#8,%0"

		: "=d" (val)
		: "0" (val)
		);

	return val;
}

static __inline unsigned long SwapLONG(long val)
{
	__asm __volatile
	(
		"ror.w	#8,%0 \n\t"
		"swap	%0 \n\t"
		"ror.w	#8,%0"

		: "=d" (val)
		: "0" (val)
		);

	return val;
}

static __inline unsigned short _swap16(unsigned short D)
{
#if PLATFORM_MACOSX
    register unsigned short returnValue;
    __asm__ volatile("lhbrx %0,0,%1"
	: "=r" (returnValue)
	: "r" (&D)
    );
    return returnValue;
#else
    return (unsigned short)((D<<8)|(D>>8));
#endif
}

static __inline unsigned int _swap32(unsigned int D)
{
#if PLATFORM_MACOSX
    register unsigned int returnValue;
    __asm__ volatile("lwbrx %0,0,%1"
	: "=r" (returnValue)
	: "r" (&D)
    );
    return returnValue;
#else
    return((D<<24)|((D<<8)&0x00FF0000)|((D>>8)&0x0000FF00)|(D>>24));
#endif
}

#if PLATFORM_MACOSX
    #define PLATFORM_BIGENDIAN 1
    #define BUILDSWAP_INTEL16(x) _swap16(x)
    #define BUILDSWAP_INTEL32(x) _swap32(x)
#elif PLATFORM_AMIGA
    #define PLATFORM_BIGENDIAN 1
    #define BUILDSWAP_INTEL16(x) SwapSHORT(x)
    #define BUILDSWAP_INTEL32(x) SwapLONG(x)
#else
    #define PLATFORM_LITTLEENDIAN 1
    #define BUILDSWAP_INTEL16(x) (x)
    #define BUILDSWAP_INTEL32(x) (x)
#endif

#endif  /* !defined _INCLUDE_PLATFORM_H_ */

/* end of platform.h ... */


