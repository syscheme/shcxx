// ===========================================================================
// Copyright (c) 2005 by
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
// ----------------------------
//
// Name  : xstring.h
// Author: Cary (cary.xiao@i-zq.com)
// Date  : 07/15/2005
// Desc  : Implementation the CString without MFC. And support multi-platform
//
// Revision History:
//
//
// ----------------------------
//
//
// ===========================================================================

#ifndef _XSTRING_H_
#define _XSTRING_H_

#include <xdef.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <xcomm.h>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

#ifndef _xassert
#define _xassert(e)		assert((e))
#endif

#ifdef __GNUC__
#define	stricmp			strcasecmp
#endif
//////////////////////////////////////////////////////////////////////////
// helper functions
template <typename TChar>
int StringLength(const TChar* psz)
{
	if (psz == NULL)
		return 0;
	int nCount = 0;
	while (*psz != 0) {
		psz ++;
		nCount ++;
	}

	return nCount;
}

//////////////////////////////////////////////////////////////////////////

template <typename TChar>
class XStringException {
	XStringException(int nCode) : 
		m_nCode(nCode), m_pszMsg(NULL){}

	XStringException(int nCode, TChar* pszMsg) : 
		m_nCode(nCode), m_pszMsg(pszMsg){}

	XStringException(TChar* pszMsg) : 
		m_nCode(-1), m_pszMsg(pszMsg){}

public:
	int		m_nCode;
	TChar*	m_pszMsg;
};

struct XStringData
{
	long LockedIncrement(long * pnCount)
	{
		return InterlockedIncrement(pnCount);
	}

	long LockedDecrement(long * pnCount)
	{
		return InterlockedDecrement(pnCount);
	}

	int AddRef()
	{
		return LockedIncrement(&nRefs);
	}

	int Release()
	{
		return LockedDecrement(&nRefs);
	}

	void* data()
	{
		return this + 1;
	}

	bool IsNull()
	{
		return (nAllocLength == 0 && nDataLength == 0 && nRefs == -1 );
	}

	long	nRefs;
	int		nDataLength;
	int		nAllocLength;
	// TChar	m_pchData[];
};

class XTraitsChar {
public:
	typedef char TChar;

	static bool IsDigit(TChar ch)
	{
		return isdigit(ch) != 0;
	}

	static bool	IsSpace(TChar ch)
	{
		return isspace(ch) != 0;
	}


	static TChar* CharNext(const TChar* psz)
	{
		return (TChar* )++ psz;
	}
	
	static bool IsValidString(const TChar* lpsz, int nLength)
	{
		if(lpsz == NULL)
			return false;
		return true;
	}

	static int Compare(const TChar* pszSrc1, const TChar* pszSrc2)
	{
		return strcmp(pszSrc1, pszSrc2);
	}

	static int CompareIgnore(const TChar* pszSrc1, const TChar* pszSrc2)
	{
		return stricmp(pszSrc1, pszSrc2);
	}

	static TChar* MakeUpper(TChar* psz)
	{
		if (psz == NULL)
			return NULL;

		while (*psz != 0) {
			*psz = toupper(*psz);
			psz ++;
		}
		
		return psz;
	}

	static TChar* MakeLower(TChar* psz)
	{
		if (psz == NULL)
			return NULL;

		while (*psz != 0) {
			*psz = tolower(*psz);
			psz ++;
		}

		return psz;

	}

	static int GetFormattedLength(const TChar* pszFormat, va_list argList)
	{
		TChar szBuffer[1028];
		int nLen = vsprintf(szBuffer, pszFormat, argList);
		_xassert(nLen >= 0 && nLen <= 1024);
		return nLen;

	}

	static void FormatV(TChar* pszBuffer, const TChar* pszFormat, va_list argList)
	{
		vsprintf(pszBuffer, pszFormat, argList);
	}

	static TChar* StrStr(const TChar* lpFirst, const TChar* lpSrc)
	{
		return strstr(lpFirst , lpSrc);
	}

	//static int ConverToMultiByte(unsigned char* lpszDest , int nBuffLen);
	//static int ConverToWideChar(wchar_t* lpszDest , int nBuffLen);
};

template <typename TChar, class TTraits>
class XStringT {
public:
	// typedef TChar			TChar;
	// typedef TChar*			TChar* ;
	// typedef const TChar*	const TChar* ;

	XStringT()
	{
		Init();
	}

	XStringT(const TChar*  lpsz)
	{
		Init();
		int nLen = SafeStrlen(lpsz);
		if (nLen != 0)
		{
			if(AllocBuffer(nLen)) {
				memcpy(m_pchData, lpsz, nLen * sizeof(TChar));
			}
		}
	}

	XStringT(const XStringT& stringSrc)
	{
		_xassert(stringSrc.GetData()->nRefs != 0);
		if (stringSrc.GetData()->nRefs >= 0)
		{
			_xassert(stringSrc.GetData() != _DataNil);
			m_pchData = stringSrc.m_pchData;
			GetData()->AddRef();
		}
		else
		{
			Init();
			*this = stringSrc.m_pchData;
		}

	}

	virtual ~XStringT()
	{
		if (!GetData()->IsNull()) {
			if (GetData()->Release() <= 0)
				delete[] (unsigned char *)GetData();
		}
	}

	static bool _IsValidString(const TChar* lpsz, int nLength)
	{
		return TTraits::IsValidString(lpsz, nLength);
	}

	int GetLength() const
	{
		return GetData()->nDataLength;
	}

	bool IsEmpty() const
	{
		return GetData()->nDataLength == 0;
	}

	void Empty()
	{
		if (GetData()->nDataLength == 0)
			return;

		if (GetData()->nRefs >= 0)
			Release();
		else
			*this = (const TChar* )"\0\0";

		_xassert(GetData()->nDataLength == 0);
		_xassert(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
	}

	TChar GetAt(int nIndex) const
	{
		_xassert(nIndex >= 0);
		_xassert(nIndex < GetData()->nDataLength);
		return m_pchData[nIndex];
	}

	void SetAt(int nIndex, TChar ch )
	{
		_xassert(nIndex >= 0);
		_xassert(nIndex < GetData()->nDataLength);

		CopyBeforeWrite();
		m_pchData[nIndex] = ch;
	}

	int Compare(const TChar*  lpsz) const
	{
		return TTraits::Compare(m_pchData, lpsz);
	}

	int CompareNoCase(const TChar*  lpsz ) const
	{
		return TTraits::CompareIgonre(m_pchData, lpsz);
	}

	XStringT Mid(int nFirst, int nCount ) const
	{
		// out-of-bounds requests return sensible things
		if (nFirst < 0)
			nFirst = 0;
		if (nCount < 0)
			nCount = 0;

		if (nFirst + nCount > GetData()->nDataLength)
			nCount = GetData()->nDataLength - nFirst;
		if (nFirst > GetData()->nDataLength)
			nCount = 0;

		XStringT dest;
		AllocCopy(dest, nCount, nFirst, 0);
		return dest;
	}

	XStringT Mid(int nFirst) const
	{
		return Mid(nFirst, GetData()->nDataLength - nFirst);
	}

	XStringT Left(int nCount) const
	{
		if (nCount < 0)
			nCount = 0;
		else if (nCount > GetData()->nDataLength)
			nCount = GetData()->nDataLength;

		XStringT dest;
		AllocCopy(dest, nCount, 0, 0);
		return dest;

	}

	XStringT Right(int nCount) const
	{
		if (nCount < 0)
			nCount = 0;
		else if (nCount > GetData()->nDataLength)
			nCount = GetData()->nDataLength;

		XStringT dest;
		AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
		return dest;
	}
	
	void MakeUpper()
	{
		TTraits::MakeUpper(m_pchData);
	}

	void MakeLower()
	{
		TTraits::MakeLower(m_pchData);
	}

	void FormatV( const TChar*  pszFormat, va_list args )
	{
		_xassert( _IsValidString( pszFormat , false) );
		if(pszFormat == NULL)
			return;

		int nLength = TTraits::GetFormattedLength( pszFormat, args );
		TChar*  pszBuffer = GetBuffer( nLength );
		TTraits::FormatV( pszBuffer, pszFormat, args );
		GetData()->nDataLength = nLength;
		// ReleaseBufferSetLength( nLength );
	}

	void _CDECL_ Format(const TChar*  lpszFormat, ... )
	{
		_xassert(_IsValidString(lpszFormat, false));

		va_list argList;
		va_start(argList, lpszFormat);
		FormatV(lpszFormat, argList);
		va_end(argList);
	}

	void TrimLeft()
	{
		CopyBeforeWrite();

		// find first non-space character
		const TChar*  lpsz = m_pchData;
		while (TTraits::IsSpace(*lpsz))
			lpsz = TTraits::CharNext(lpsz);

		// fix up data and length
		int nDataLength = GetData()->nDataLength - (int)(unsigned long* )(lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(TChar));
		GetData()->nDataLength = nDataLength;
	}

	void TrimRight()
	{
		CopyBeforeWrite();

		// find beginning of trailing spaces by starting at beginning (DBCS aware)
		TChar*  lpsz = m_pchData;
		TChar*  lpszLast = NULL;
		while (*lpsz != '\0')
		{
			if (TTraits::IsSpace(*lpsz))
			{
				if (lpszLast == NULL)
					lpszLast = lpsz;
			}
			else
			{
				lpszLast = NULL;
			}
			lpsz = TTraits::CharNext(lpsz);
		}

		if (lpszLast != NULL)
		{
			// truncate at trailing space start
			*lpszLast = '\0';
			GetData()->nDataLength = (int)(unsigned long* )(lpszLast - m_pchData);
		}

	}

	int Find(TChar ch) const
	{
		// find first single character
		TChar*  lpsz = FindChar(m_pchData, ch);

		// return -1 if not found and index otherwise
		return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);

	}

	int Find(const TChar*  lpszSub ) const
	{
		_xassert(_IsValidString(lpszSub, false));

		// find first matching substring
		TChar*  lpsz = FindSubStr(m_pchData, lpszSub);

		// return -1 for not found, distance from beginning otherwise
		return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
	}

	int FindOneOf(const TChar*  lpszCharSet ) const
	{
		_xassert(_IsValidString(lpszCharSet, false));
		TChar*  lpsz = FindOneOf(m_pchData, lpszCharSet);
		return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);

	}

	operator const TChar* () const
	{
		return m_pchData;
	}

	TChar operator[](int nIndex) const
	{
		// same as GetAt
		_xassert(nIndex >= 0);
		_xassert(nIndex < GetData()->nDataLength);
		return m_pchData[nIndex];
	}

	const XStringT& operator=(const XStringT& stringSrc)
	{
		if (m_pchData != stringSrc.m_pchData)
		{
			if ((GetData()->nRefs < 0 && GetData() != _DataNil) || stringSrc.GetData()->nRefs < 0)
			{
				// actual copy necessary since one of the strings is locked
				AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
			}
			else
			{
				// can just copy references around
				Release();
				_xassert(stringSrc.GetData() != _DataNil);
				m_pchData = stringSrc.m_pchData;
				GetData()->AddRef();
			}
		}
		return *this;
	}

	const XStringT& operator=( TChar ch )
	{
		_xassert(!_istlead(ch));    // can't set single lead byte
		AssignCopy(1, &ch);
		return *this;
	}

	const XStringT& operator=(const TChar*  lpsz )
	{
		_xassert(lpsz == NULL || _IsValidString(lpsz, false));
		AssignCopy(SafeStrlen(lpsz), lpsz);
		return *this;
	}

	const XStringT& operator+=(const XStringT& stringSrc)
	{
		ConcatInPlace(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		return *this;
	}

	const XStringT& operator+=(TChar ch )
	{
		ConcatInPlace(1, &ch);
		return *this;
	}

	const XStringT& operator+=(const TChar*  lpsz )
	{
		_xassert(lpsz == NULL || _IsValidString(lpsz, false));
		ConcatInPlace(SafeStrlen(lpsz), lpsz);
		return *this;

	}

	inline void ReleaseBuffer(int nNewLength = -1)
	{
		CopyBeforeWrite();  // just in case GetBuffer was not called

		if (nNewLength == -1)
			nNewLength = StringLength(m_pchData); // zero terminated

		_xassert(nNewLength <= GetData()->nAllocLength);
		GetData()->nDataLength = nNewLength;
		m_pchData[nNewLength] = '\0';
	}

	void ReleaseBufferSetLength( int nLength )
	{
		Release();
		AllocBuffer(nLength);
	}

	inline TChar* GetBuffer(int nMinBufLength)
	{
		_xassert(nMinBufLength >= 0);

		if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
		{
			// we have to grow the buffer
			XStringData* pOldData = GetData();
			int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
			if (nMinBufLength < nOldLen)
				nMinBufLength = nOldLen;

			if(AllocBuffer(nMinBufLength))
			{
				memcpy(m_pchData, pOldData->data(), (nOldLen + 1) * sizeof(TChar));
				GetData()->nDataLength = nOldLen;
				XStringT<TChar, TTraits>::Release(pOldData);
			}
		}
		_xassert(GetData()->nRefs <= 1);

		// return a pointer to the character storage for this string
		_xassert(m_pchData != NULL);
		return m_pchData;
	}

	inline int Replace(TChar chOld, TChar chNew)
	{
		int nCount = 0;

		// short-circuit the nop case
		if (chOld != chNew)
		{
			// otherwise modify each character that matches in the string
			CopyBeforeWrite();
			LPTSTR psz = m_pchData;
			LPTSTR pszEnd = psz + GetData()->nDataLength;
			while (psz < pszEnd)
			{
				// replace instances of the specified character only
				if (*psz == chOld)
				{
					*psz = chNew;
					nCount++;
				}
				psz = TTraits::CharNext(psz);
			}
		}
		return nCount;
	}

	inline int Replace(const TChar* lpszOld, const TChar* lpszNew)
	{
		// can't have empty or NULL lpszOld

		int nSourceLen = SafeStrlen(lpszOld);
		if (nSourceLen == 0)
			return 0;
		int nReplacementLen = SafeStrlen(lpszNew);

		// loop once to figure out the size of the result string
		int nCount = 0;
		TChar* lpszStart = m_pchData;
		TChar* lpszEnd = m_pchData + GetData()->nDataLength;
		TChar* lpszTarget;
		while (lpszStart < lpszEnd)
		{
			while ((lpszTarget = TTraits::StrStr(lpszStart, lpszOld)) != NULL)
			{
				nCount++;
				lpszStart = lpszTarget + nSourceLen;
			}
			lpszStart += lstrlen(lpszStart) + 1;
		}

		// if any changes were made, make them
		if (nCount > 0)
		{
			CopyBeforeWrite();

			// if the buffer is too small, just
			//   allocate a new buffer (slow but sure)
			int nOldLength = GetData()->nDataLength;
			int nNewLength =  nOldLength + (nReplacementLen - nSourceLen) * nCount;
			if (GetData()->nAllocLength < nNewLength || GetData()->nRefs > 1)
			{
				XStringData* pOldData = GetData();
				LPTSTR pstr = m_pchData;
				if(!AllocBuffer(nNewLength))
					return -1;
				memcpy(m_pchData, pstr, pOldData->nDataLength * sizeof(TChar));
				XStringT::Release(pOldData);
			}
			// else, we just do it in-place
			lpszStart = m_pchData;
			lpszEnd = m_pchData + GetData()->nDataLength;

			// loop again to actually do the work
			while (lpszStart < lpszEnd)
			{
				while ( (lpszTarget = TTraits::StrStr(lpszStart, lpszOld)) != NULL)
				{
					int nBalance = nOldLength - ((int)(unsigned long *)(lpszTarget - m_pchData) + nSourceLen);
					memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen, nBalance * sizeof(TCHAR));
					memcpy(lpszTarget, lpszNew, nReplacementLen * sizeof(TCHAR));
					lpszStart = lpszTarget + nReplacementLen;
					lpszStart[nBalance] = '\0';
					nOldLength += (nReplacementLen - nSourceLen);
				}
				lpszStart += lstrlen(lpszStart) + 1;
			}
			_xassert(m_pchData[nNewLength] == '\0');
			GetData()->nDataLength = nNewLength;
		}

		return nCount;
	}

protected:

	static int s_InitData[];
	static XStringData* _DataNil;

	void Init()
	{
		m_pchData = (TChar*  )_DataNil->data();
	}

	XStringData* GetData() const
	{
		return ((XStringData* )m_pchData) - 1;
	}
	
	bool AllocBuffer(int nLen)
	{
		_xassert(nLen >= 0);
		_xassert(nLen <= 0x7fffffff - 1);    // max size (enough room for 1 extra)
		if (nLen == 0) {
			Init();
		}
		else {
			XStringData* pData = NULL;
			int nSize = sizeof(XStringData) + (nLen + 1) * sizeof(TChar);
			pData = (XStringData* )new unsigned char[nSize];
			if (pData == NULL)
				return false;

			memset(pData, 0, nSize);
			pData->nRefs = 1;
			pData->nDataLength = nLen;
			pData->nAllocLength = nLen;
			m_pchData = (TChar*  )pData->data();
		}

		return true;
	}

	void AllocCopy(XStringT& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const
	{
		// will clone the data attached to this string
		// allocating 'nExtraLen' characters
		// Places results in uninitialized string 'dest'
		// Will copy the part or all of original data to start of new string

		int nNewLen = nCopyLen + nExtraLen;
		if (nNewLen == 0)
		{
			dest.Init();
		}
		else
		{
			dest.AllocBuffer(nNewLen);
			memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(TChar));
		}
	}

	void AssignCopy(int nSrcLen, const TChar*  lpszSrcData)
	{
		AllocBeforeWrite(nSrcLen);
		memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TChar));
		GetData()->nDataLength = nSrcLen;
		m_pchData[nSrcLen] = '\0';
	}

	void ConcatCopy(int nSrc1Len, const TChar*  lpszSrc1Data, int nSrc2Len, const TChar*  lpszSrc2Data)
	{
	  // -- master concatenation routine
	  // Concatenate two sources
	  // -- assume that 'this' is a new XStringT object

		int nNewLen = nSrc1Len + nSrc2Len;
		if (nNewLen != 0)
		{
			AllocBuffer(nNewLen);
			memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(TChar));
			memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(TChar));
		}


	}

	void ConcatInPlace(int nSrcLen, const TChar*  lpszSrcData)
	{
		//  -- the main routine for += operators

		// concatenating an empty string is a no-op!
		if (nSrcLen == 0)
			return;

		// if the buffer is too small, or we have a width mis-match, just
		//   allocate a new buffer (slow but sure)
		if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
		{
			// we have to grow the buffer, use the ConcatCopy routine
			XStringData* pOldData = GetData();
			ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
			_xassert(pOldData != NULL);
			XStringT::Release(pOldData);
		}
		else
		{
			// fast concatenation when buffer big enough
			memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(TChar));
			GetData()->nDataLength += nSrcLen;
			_xassert(GetData()->nDataLength <= GetData()->nAllocLength);
			m_pchData[GetData()->nDataLength] = '\0';
		}
	}

	void CopyBeforeWrite()
	{

		if (GetData()->nRefs > 1)
		{
			XStringData* pData = GetData();
			Release();
			AllocBuffer(pData->nDataLength);
			memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TChar));
		}
		_xassert(GetData()->nRefs <= 1);
	}

	void AllocBeforeWrite(int nLen)
	{
		if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
		{
			Release();
			AllocBuffer(nLen);
		}
		_xassert(GetData()->nRefs <= 1);
	}

	void Release()
	{
		XStringData* pData = GetData();
		if (pData != _DataNil)
		{
			_xassert(pData->nRefs != 0);
			if (pData->Release() <= 0)
				FreeData(pData);
			Init();
		}
	}

	static void Release(XStringData* pData)
	{
		if (!pData->IsNull())
		{
			_xassert(pData->nRefs != 0);
			if (pData->Release() <= 0)
				FreeData(pData);
		}
	}

	static int SafeStrlen(const TChar*  lpsz)
	{
		if (lpsz == NULL)
			return 0;
		return StringLength(lpsz);
	}

	static void FreeData(XStringData* pData)
	{
		delete pData;
	}

	inline int GetAllocLength() const
	{
		return GetData()->nAllocLength;
	}

protected:
	static TChar* FindChar(const TChar* p, TChar ch)
	{
		//strchr for '\0' should succeed
		while (*p != 0)
		{
			if (*p == ch)
				break;
			p = TTraits::CharNext(p);
		}

		return (TChar*)((*p == ch) ? p : NULL);
	}

	static TChar* FindOneOf(const TChar* p, const TChar* lpszCharSet)
	{
		while (*p != 0)
		{
			if (FindChar(lpszCharSet, *p) != NULL)
			{
				return (TChar* )p;
				break;
			}
			p = TTraits::CharNext(p);
		}
		return NULL;
	}

	static TChar* FindSubStr(const TChar* pStr, const TChar* pCharSet)
	{
		int nLen = StringLength(pCharSet);
		if (nLen == 0)
			return (TChar*)pStr;

		const TChar* pRet = NULL;
		const TChar* pCur = pStr;
		while((pStr = FindChar(pCur, *pCharSet)) != NULL)
		{
			if(memcmp(pCur, pCharSet, nLen * sizeof(TChar)) == 0)
			{
				pRet = pCur;
				break;
			}
			pCur = TTraits::CharNext(pCur);
		}

		return (TChar* ) pRet;
	}

#ifdef _MSC_VER
	friend XStringT<TChar, TTraits> _STDCALL_ operator+ (const XStringT<TChar, TTraits>& string1, const XStringT<TChar, TTraits>& string2);
	friend XStringT<TChar, TTraits> _STDCALL_ operator+ (const XStringT<TChar, TTraits>& string, TChar ch);
	friend XStringT<TChar, TTraits> _STDCALL_ operator+ (TChar ch, const XStringT<TChar, TTraits>& string);
	friend XStringT<TChar, TTraits> _STDCALL_ operator+ (const XStringT& string, const TChar* lpsz);
	friend XStringT<TChar, TTraits> _STDCALL_ operator+ (const TChar* lpsz, const XStringT<TChar, TTraits>& string);
#else // #ifdef _MSVC
	template <typename TChar1, class TTraits1>
	friend XStringT<TChar1, TTraits1> _STDCALL_ operator+ (const XStringT<TChar1, TTraits1>& string1, const XStringT<TChar1, TTraits1>& string2);
	template <typename TChar1, class TTraits1>
	friend XStringT<TChar1, TTraits1> _STDCALL_ operator+ (const XStringT<TChar1, TTraits1>& string, TChar1 ch);
	template <typename TChar1, class TTraits1>
	friend XStringT<TChar1, TTraits1> _STDCALL_ operator+ (TChar1 ch, const XStringT<TChar1, TTraits1>& string);
	template <typename TChar1, class TTraits1>
	friend XStringT<TChar1, TTraits1> _STDCALL_ operator+ (const XStringT<TChar1, TTraits1>& string, const TChar1* lpsz);
	template <typename TChar1, class TTraits1>
	friend XStringT<TChar1, TTraits1> _STDCALL_ operator+ (const TChar1* lpsz, const XStringT<TChar1, TTraits1>& string);
#endif // #ifdef _MSVC

protected:
	TChar* 		m_pchData;
};

template <typename TChar, class TTraits>
int XStringT<TChar, TTraits>::s_InitData[]= { -1, 0, 0, 0 };


template <typename TChar, class TTraits>
XStringData* XStringT<TChar, TTraits>::_DataNil = 
	(XStringData* )&XStringT<TChar, TTraits>::s_InitData;

template <typename TChar, class TTraits>
inline XStringT<TChar, TTraits> _STDCALL_ operator+(const XStringT<TChar, TTraits>& string1, const XStringT<TChar, TTraits>& string2)
{
	XStringT<TChar, TTraits> s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

template <typename TChar, class TTraits>
inline XStringT<TChar, TTraits> _STDCALL_ operator+(const XStringT<TChar, TTraits>& string, const TChar*  lpsz)
{
	_xassert((lpsz == NULL || XStringT<TChar, TTraits>::_IsValidString(lpsz, false)));
	XStringT<TChar, TTraits> s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData, XStringT<TChar, TTraits>::SafeStrlen(lpsz), lpsz);
	return s;
}

template <typename TChar, class TTraits>
inline XStringT<TChar, TTraits> _STDCALL_ operator+(const TChar*  lpsz, const XStringT<TChar, TTraits>& string)
{
	_xassert((lpsz == NULL || XStringT<TChar, TTraits>::_IsValidString(lpsz, false)));
	XStringT<TChar, TTraits> s;
	s.ConcatCopy(XStringT<TChar, TTraits>::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

template <typename TChar, class TTraits>
inline XStringT<TChar, TTraits> _STDCALL_ operator+(const XStringT<TChar, TTraits>& string1, TChar ch)
{
	XStringT<TChar, TTraits> s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
	return s;
}

template <typename TChar, class TTraits>
inline XStringT<TChar, TTraits> _STDCALL_ operator+(TChar ch, const XStringT<TChar, TTraits>& string)
{
	XStringT<TChar, TTraits> s;
	s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

template <typename TChar, class TTraits>
inline bool _STDCALL_ operator==(const XStringT<TChar, TTraits>& s1, const XStringT<TChar, TTraits>& s2)
	{ return s1.Compare(s2) == 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator==(const XStringT<TChar, TTraits>& s1, const TChar*  s2)
	{ return s1.Compare(s2) == 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator==(const TChar*  s1, const XStringT<TChar, TTraits>& s2)
	{ return s2.Compare(s1) == 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator!=(const XStringT<TChar, TTraits>& s1, const XStringT<TChar, TTraits>& s2)
	{ return s1.Compare(s2) != 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator!=(const XStringT<TChar, TTraits>& s1, const TChar*  s2)
	{ return s1.Compare(s2) != 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator!=(const TChar*  s1, const XStringT<TChar, TTraits>& s2)
	{ return s2.Compare(s1) != 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator<(const XStringT<TChar, TTraits>& s1, const XStringT<TChar, TTraits>& s2)
	{ return s1.Compare(s2) < 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator<(const XStringT<TChar, TTraits>& s1, const TChar*  s2)
	{ return s1.Compare(s2) < 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator<(const TChar*  s1, const XStringT<TChar, TTraits>& s2)
	{ return s2.Compare(s1) > 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator>(const XStringT<TChar, TTraits>& s1, const XStringT<TChar, TTraits>& s2)
	{ return s1.Compare(s2) > 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator>(const XStringT<TChar, TTraits>& s1, const TChar*  s2)
	{ return s1.Compare(s2) > 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator>(const TChar*  s1, const XStringT<TChar, TTraits>& s2)
	{ return s2.Compare(s1) < 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator<=(const XStringT<TChar, TTraits>& s1, const XStringT<TChar, TTraits>& s2)
	{ return s1.Compare(s2) <= 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator<=(const XStringT<TChar, TTraits>& s1, const TChar*  s2)
	{ return s1.Compare(s2) <= 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator<=(const TChar*  s1, const XStringT<TChar, TTraits>& s2)
	{ return s2.Compare(s1) >= 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator>=(const XStringT<TChar, TTraits>& s1, const XStringT<TChar, TTraits>& s2)
	{ return s1.Compare(s2) >= 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator>=(const XStringT<TChar, TTraits>& s1, const TChar*  s2)
	{ return s1.Compare(s2) >= 0; }
template <typename TChar, class TTraits>
inline bool _STDCALL_ operator>=(const TChar*  s1, const XStringT<TChar, TTraits>& s2)
	{ return s2.Compare(s1) <= 0; }

#ifndef UNICODE
typedef XStringT< char, XTraitsChar >	XString;
#else
#error No implementation for Unicode.
#endif

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifdef _XSTRING_H_

