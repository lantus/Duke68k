// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef _STDAFX_H_
#define _STDAFX_H_

//#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define WIN32_EXTRA_LEAN

// Windows Header Files:
#include <windows.h>
#include <mmsystem.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define FILE_EXSISTS 6
#ifdef _DEBUG
#define STUBBED(x) fprintf(stderr,"STUB: %s (%s, %s:%d)\n",x,__FUNCTION__,__FILE__,__LINE__)
#else
#define STUBBED(x)
#endif


// This block of macros allows one to 
// output a line of text to VC which you
// can click and be taken straight to
// that line in code
#define STR_LINE2(x) #x 
#define STR_LINE(x)     STR_LINE2(x)
#define __STR_LINE__ STR_LINE(__LINE__)
#define MESSAGE(x) message( __FILE__ "("__STR_LINE__") : " x )

//TEMP
#pragma message (" FIXME: Remove these PRAGMA warning stubs ")
#pragma warning( disable : 4244 ) //conversion, possible data loss
#pragma warning( disable : 4267 ) //conversion, possible data loss
#pragma warning( disable : 4101 ) //unref local var
#pragma warning( disable : 4033 ) //must return a value
#pragma warning( disable : 4311 ) //pointer truncation
#pragma warning( disable : 4312 ) //conversion to greater size
#pragma warning( disable : 4013 ) //undefined, assuming extern
#pragma warning( disable : 4716 ) //must return a value

// TODO: reference additional headers your program requires here

#endif