// ===========================================================================
// Copyright (c) 2004 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// Ident : $Id: ZQ_common_conf.h,v 1.9 2004/06/23 06:42:41 wli Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define cofigurations and symbos
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ZQ_common_conf.h $
// 
// 14    3/11/16 10:04a Dejian.fei
// NDK android
// 
// 13    9/25/15 2:23p Ketao.zhang
// 
// 12    3/26/15 1:24p Zhiqiang.niu
// 
// 8     1/22/15 2:04p Build
// rollback
// 
// 5     12/20/12 11:08a Hongquan.zhang
// 
// 4     12/20/12 10:54a Hongquan.zhang
// 
// 3     12/19/12 2:37p Hongquan.zhang
// support printf linke function parameter list checking
// 
// 2     1/17/12 11:36a Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 29    10-10-28 17:36 Hui.shao
// 
// 28    09-12-29 19:15 Fei.huang
// + added FMT64U
// 
// 27    09-06-23 15:26 Fei.huang
// * define long as int64 on 64 bit linux 
// + add macro FMT64 for printf family
// 
// 26    09-03-05 11:40 Hongquan.zhang
// 
// 25    09-03-03 18:09 Hongquan.zhang
// add   #	 include <Ws2tcpip.h>
// 
// 24    09-01-07 16:33 Fei.huang
// + include string.h 
// 
// 23    08-11-15 14:11 Jie.zhang
// 
// 22    08-10-08 10:45 Hui.shao
// 
// 21    08-08-14 14:39 Hui.shao
// merged from TianShan 1.7.10
// 
// 22    08-07-29 12:18 Hui.shao
// 
// 21    08-07-07 16:09 Hui.shao
// 
// 19    08-03-14 12:21 Fei.huang
// define _tcsclen to strlen for gcc
// 
// 18    08-03-12 15:18 Fei.huang
// 
// 17    08-03-11 10:18 Fei.huang
// 
// 16    08-03-06 15:58 Hui.shao
// updated for Linux build
// 
// 15    08-02-21 12:49 Hui.shao
// 
// 14    06-11-22 10:38 Ken.qian
// 
// 6     05-04-14 23:02 Daniel.wang
// 
// 5     4/13/05 6:30p Hui.shao
// 
// 4     4/13/05 5:06p Hui.shao
// 
// 3     05-01-11 15:41 Daniel.wang
// delete ssize_t type because of so many libraries defined this type
// Revision 1.9  2004/06/23 06:42:41  wli
// restore to ver 1.7 precompile problem
//
// Revision 1.8  2004/06/22 09:01:37  wli
// add _WIN32_WINNT define and WIN32_LEAN_AND_MEAN define
//
// Revision 1.7  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.6  2004/05/09 03:54:49  shao
// no message
//
// Revision 1.5  2004/04/28 06:25:02  shao
// doxy comment format
//
// Revision 1.4  2004/04/27 08:52:12  shao
// no message
//
// Revision 1.3  2004/04/27 02:29:29  shao
// addjusted file header format
//
// Revision 1.2  2004/04/21 04:24:19  shao
// winsock2
//
// ===========================================================================

#ifndef __ZQ_COMMON_CONF_H__
#define __ZQ_COMMON_CONF_H__


// address the operating systems
#if defined(_WIN32) || defined(WIN32)
#  undef  WIN32
#  define WIN32
#  undef  _WIN32
#  define _WIN32
#endif

#if defined(_WIN64) || defined(WIN64)
#  undef  WIN64
#  define WIN64
#  undef  _WIN64
#  define _WIN64
#endif

#if defined(WIN64) || defined(WIN32) || defined(_Windows) || defined(__MINGW32__)
#  define ZQ_OS_MSWIN
#elif defined (__linux) || defined (__linux__)
#  define ZQ_OS_LINUX
#else
# error unsupported operating system
#endif

// check multithreading
// -------------
#if defined(ZQ_OS_MSWIN) && !defined(__MINGW32__) && !defined(__MT__) && !defined(_MT)
#  error Please enable multithreading
#endif

// Disable level 4 warnings explicitly, for STL in VC++
// -------------
#ifdef _MSC_VER
#  pragma comment(user, "Compiled on " __DATE__ " at " __TIME__ ) 
// #  pragma comment(linker, "/merge:.data=.text")
// #  pragma comment(linker, "/merge:.rdata=.text")
#  if (_MSC_VER >= 1400) //VC2005+
#    pragma warning (disable: 4800 4355 4786 4503 4275 4251 4290 4786 4996)
#  else
#    pragma warning (disable: 4800 4355 4786 4503 4275 4251 4290 4786)
#    pragma comment(linker, "/OPT:REF")
#    pragma comment(linker, "/INCREMENTAL:no")
#    pragma comment(linker, "/OPT:ICF")
#    pragma comment(linker, "/OPT:NOWIN98")
#  endif
#endif

// -------------
// include the basic headers
// -------------
#ifdef ZQ_OS_MSWIN

#  ifdef WIN32_MFC
#    include "StdAfx.h"
#  elif defined(_MSC_VER) || defined(__MINGW32__)
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#    include <winsock2.h>
#    include <windows.h>
#    include <winbase.h>
#ifdef __cplusplus
};
#endif 
//#	 include <Ws2tcpip.h>
#  endif // WIN32_MFC

#elif defined(ZQ_OS_LINUX)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#   include <unistd.h>
#   include <sys/types.h>
#ifndef ZQ_COMMON_ANDROID
#   include <error.h>
#endif
#   include <pthread.h>
#   include <string.h>
#ifdef __cplusplus
};
#endif 

#endif // ZQ_OS_MSWIN

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#	include <stdio.h>
#	include <stdlib.h>
#ifdef __cplusplus
};
#endif 

// -------------
// the following sections covers
//   1) __EXPORT, __DLLRTL
//   2) int8, int16, int32, int64, uint8, uint16, uint32, uint64
//   3) TCHAR, _tcsclen, _ttol, _stscanf, _vstprintf, _T(x), _itoa, __strrev, watol
// -------------
#if defined(__MINGW32__) || defined(__CYGWIN32__)
// CygWin32 definition
#  define	HAVE_OLD_IOSTREAM
#  undef  __EXPORT
#  define __EXPORT
#  undef  __stdcall
#  define __stdcall

   typedef char int8;
   typedef short int16;
   typedef long int32;
   typedef long long int64;
   typedef unsigned char uint8;
   typedef unsigned short uint16;
   typedef unsigned long uint32;
   typedef unsigned long long uint64;

#  ifdef __MINGW32__
#     define HAVE_MODULES   1
#     define alloca(x)      __builtin_alloca(x)
#     define THROW(x)       throw x
#     define THROWS(x)      throw(x)
      typedef unsigned int  uint;
#     define        snprintf            _snprintf
#     ifndef ETC_PREFIX
#        define ETC_PREFIX   "c:/"
#     endif

#	  define _tcsclen	strlen

#  else
      typedef DWORD size_t;
#  endif // __MINGW32__

// end of CygWin32 definition
#elif defined(ZQ_OS_MSWIN)

// non-CygWin32 Windows definition
// -----------------------
#  define	__EXPORT  __declspec(dllexport)
#  define	__DLLRTL  __declspec(dllimport)

#  define	snprintf	_snprintf
#  define	vsnprintf	_vsnprintf
#  define	PRINTFLIKE(m,n)	

   typedef __int8  int8;
   typedef __int16 int16;
   typedef __int32 int32;
   typedef __int64 int64;

   typedef unsigned int     uint;
   typedef unsigned __int8  uint8;
   typedef unsigned __int16 uint16;
   typedef unsigned __int32 uint32;
   typedef unsigned __int64 uint64;
   
#define FMT64 "%I64d"
#define FMT64U "%I64u"

#elif defined(ZQ_OS_LINUX) 

#   define __DLLRTL __attribute__ ((visibility("default")))
#   define __EXPORT __attribute__ ((visibility("default")))
#   define	PRINTFLIKE(m,n)	__attribute__((format(printf,m,n)))
//base data type
   typedef char			int8;
   typedef short		int16;
   typedef int 			int32;
   typedef unsigned char		uint8;
   typedef unsigned short		uint16;
   typedef unsigned int 		uint32;

//other data type 
   typedef wchar_t		WCHAR;
#  ifndef _atoi64 
#     define _atoi64(_x)        strtoull(_x, (char**)NULL, 10)
#  endif  // _atoi64 

#if defined(__x86_64)
    typedef long int64;
    typedef unsigned long uint64;
#   define FMT64 "%ld"
#   define FMT64U "%lu"
#else
    typedef long long int64;
    typedef unsigned long long	uint64;
#   define FMT64 "%lld"
#   define FMT64U "%llu"
#endif

//base macro
#  define MAX_PATH		260

//variant type
#  ifdef _UNICODE 
		typedef wchar_t		_TCHAR;
		typedef wchar_t		TCHAR;
		#define _T(x)		L## ""x
		#define _tcsclen	 wcslen
		#define _ttol		watol//define in linux_com.h
		#define _stscanf	sscanf
		#define _vstprintf	vswprintf
#  else
		typedef char		_TCHAR;
		typedef char		TCHAR;
		#define	_T(x)		 x
		#define _tcsclen	strlen
		#define _ttol		atol
		#define _stscanf	sscanf
		#define _vstprintf	vsprintf
#  endif

//function reference
#  define _itoa   itoa
#  define __strrev strrev

#  define stricmp strcasecmp

   char* strrev(char* szT);
   char* itoa(int value, char*  str, int radix);
   long watol(wchar_t* wch);

#endif // ZQ_OS_LINUX

// -------------
// the following sections covers
//   1) timeout_t
//   2) separators: FNSEP, PHSEP
// -------------
#  define LOGIC_FNSEPS "/"
#  define LOGIC_FNSEPC '/'
   
#ifdef ZQ_OS_MSWIN
#  define FNSEPS "\\"
#  define PHSEPS ";"
#  define FNSEPC '\\'
#  define PHSEPC ';'

#if !defined(__MINGW32__)
   typedef DWORD timeout_t;        // msec
#else
   typedef unsigned long timeout_t;
#endif

#else
#  define FNSEPS "/"
#  define PHSEPS ":"
#  define FNSEPC '/'
#  define PHSEPC ':'

   typedef	unsigned	long timeout_t;
#endif

typedef	uint64			timeout64_t; // msec
#define TIMEOUT_INF		~((timeout_t) 0)
#define TIMEOUT64_INF	~((timeout64_t) 0)

// -------------
#define HAVE_MODULES 1
#undef  HAVE_PTHREAD_RWLOCK
#undef  PTHREAD_MUTEXTYPE_RECURSIVE


// define endian macros
#define __BYTE_ORDER __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321


// ZQ_COMMON_API
// -------------
#ifdef ZQCOMMON_EXPORTS
#  undef  ZQCOMMON_DLL
#  define ZQCOMMON_DLL
#  define ZQ_COMMON_API __EXPORT
#else
#  define ZQ_COMMON_API __DLLRTL
#endif

#ifndef ZQCOMMON_DLL
#  undef  ZQ_COMMON_API
#  define ZQ_COMMON_API // trust as static build
#endif // ZQCOMMON_DLL

#define	COMMON_TPPORT_TYPE_DEFINED

#ifdef _DEBUG
#  define VODLIBEXT "_d.lib"
#else
#  define VODLIBEXT ".lib"
#endif

#ifndef  __MSGLOC__
// definitions for macro #pragma message(__MSGLOC__  "blah blah")
#  define __STR2__(x) #x
#  define __STR1__(x) __STR2__(x)
#  define __MSGLOC__ __FILE__ "("__STR1__(__LINE__)") : "
#endif // __MSGLOC__

#define __TODO__ __MSGLOC__  "!TODO: " __FUNCTION__ "() "

#ifndef IN
#  define IN
#endif
#ifndef OUT
#  define OUT
#endif
#ifndef INOUT
#  define INOUT
#endif

#ifndef MIN
#  define MIN(_A, _B) ((_A) >(_B) ? (_B) :(_A))
#endif

#ifndef MAX
#  define MAX(_A, _B) ((_A) >(_B) ? (_A) :(_B))
#endif

#ifdef __cplusplus
/// The root namespace of ZQ classes
namespace ZQ   {
/// ZQ common c++ classes
namespace common  {

   ZQ_COMMON_API  void settid();
   ZQ_COMMON_API  unsigned int getthreadid();

}; // namespace VOD
}; // namespace ZQ
#endif // __cplusplus

#endif // __ZQ_COMMON_CONF_H__

