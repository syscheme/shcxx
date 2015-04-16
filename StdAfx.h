// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(__STDAFX_H__)
#define __STDAFX_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif
//
// Platform Includes
//
#if defined(_WIN32)
 #if defined(_AFXDLL)
  #include <afx.h>
  #include <afxmt.h>
  #include <winsock2.h>
  #include <afxsock.h>
 #else
  #include <windows.h>
  #include <winsock2.h>
 #endif // defined(_AFXDLL)
 #include <process.h>
 #if defined(_DEBUG)
  #include <crtdbg.h>
 #endif
#endif // defined(_WIN32)
#include <tchar.h>
//
// Ansi Includes
//
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__STDAFX_H__)
