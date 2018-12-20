#ifndef _XDEF_H_
#define _XDEF_H_

#ifdef __GNUC__
//////////////////////////////////////////////////////////////////////////
// For GCC
#undef _CDECL_
#define _CDECL_		__attribute__((__cdecl__))

#undef _STDCALL_
#define _STDCALL_	__attribute__((__stdcall__)) 
#else // #ifdef __GNUC__
#ifdef _MSC_VER
//////////////////////////////////////////////////////////////////////////
// For MSC

#undef _CDECL_
#define _CDECL_		__stdcall

#undef _STDCALL_
#define _STDCALL_	__cdecl

#else // #ifdef _MSC_VER
//////////////////////////////////////////////////////////////////////////
// For other
#error This is a unknown platform.

#endif // #ifdef _MSC_VER
#endif // #ifdef __GNUC__

#define MAX_PATH		260

#ifndef NULL
#define NULL			0
#endif

#ifndef FALSE
#define FALSE			0
#endif

#ifndef TRUE
#define TRUE			1
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#undef far
#undef near
#undef pascal

#define far
#define near
#define pascal		_STDCALL_

#undef CALLBACK
#define CALLBACK    _STDCALL_

#undef WINAPI
#define WINAPI      _STDCALL_

#undef WINAPIV
#define WINAPIV     _CDECL_

#undef APIENTRY
#define APIENTRY    WINAPI

#undef APIPRIVATE
#define APIPRIVATE  _STDCALL_

#undef PASCAL
#define PASCAL      _STDCALL_

#undef FAR
#undef NEAR
#define FAR
#define NEAR

#ifndef CONST
#define CONST		const
#endif

#ifndef UNUSED
#define UNUSED(x)	(x)
#endif

#ifndef UNUSED_ALWAYS
#define UNUSED_ALWAYS(x)	(x)
#endif

#define _countof(arr)		(sizeof(arr) / sizeof(arr[0]))
#endif // #ifndef _XDEF_H_
