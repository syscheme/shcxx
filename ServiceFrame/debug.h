// debug.h: interface for the debug class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEBUG_H__72606D51_3B4A_4801_AB0B_2DBDE345D189__INCLUDED_)
#define AFX_DEBUG_H__72606D51_3B4A_4801_AB0B_2DBDE345D189__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _NO_TRACK_MEMORY
// #define _NO_TRACK_MEMORY
#endif

#if defined(_DEBUG) && !defined(_NO_TRACK_MEMORY)
#ifndef __FUNCTION__
#define __FUNCTION__		"Function()"
#endif

void* operator new(size_t nSize, const char* lpszFileName, int nLine, 
				   const char* pszFunction);
void _printDelInfo(void* p, const char* lpszFileName, int nLine, 
				   const char*  pszFunction);

void operator delete(void* p, const char*  lpszFileName, 
					 int nLine, const char*  pszFunction);

#define debug_new		new(__FILE__, __LINE__, __FUNCTION__)

#define _delete(X)		do { _printDelInfo(X, __FILE__, __LINE__, __FUNCTION__), delete X; }while(0);

#define DUMP(objPtr)	(objPtr)->dump()

#else //  && !defined(_NO_TRACK_MEMORY)
#define _delete(X)		delete X
#define debug_new		new
#endif //  && !defined(_NO_TRACK_MEMORY)

#endif // !defined(AFX_DEBUG_H__72606D51_3B4A_4801_AB0B_2DBDE345D189__INCLUDED_)
