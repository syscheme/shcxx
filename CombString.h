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
// Ident : $Id: CombString.h,v 1.3 2004/08/04 09:28:19 wli Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define Inet address
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/CombString.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 5     08-03-03 16:28 Yixin.tian
// merged changes for linux
// 
// 4     05-12-15 12:00 Ken Qian
//       fix the bug of char*() return NULL when the input is ""
//
// 3     04-09-14 16:02 Kaliven.lee
// fix bug clause by strlen return the length of string exclude NULL
// terminate.
// 
// 2     04-09-14 14:21 Kaliven.lee
// Revision 1.3  2004/08/04 09:28:19  wli
// modified to compatiable with VC 6.0
//
// Revision 1.2  2004/07/08 02:38:07  shao
// operator char*
//
// Revision 1.1  2004/07/08 02:36:41  shao
// create
// ===========================================================================

#ifndef	__ZQ_COM_CombString_H__
#define	__ZQ_COM_CombString_H__

#include "ZQ_common_conf.h"
#include <string>

namespace ZQ {
namespace common {

typedef std::wstring wstring;

class CombString : public wstring
{
public:
	CombString(const char *str)
		:_str(NULL)
	{
		//*this = str;
		assign(str);
	}

	CombString(const WCHAR *str)
		:_str(NULL)
	{
		wstring::assign(str);
	}

	CombString(const CombString& str)
		:_str(NULL)
	{
		*this = str.c_str();
	}
	~CombString()
	{
		if (_str !=NULL)
			delete[] _str;
		_str=NULL;
	}

	void assign(const char *str)
	{
		if (str ==NULL)
			return;

		if (_str)
			delete[] _str;
		_str=NULL;

		size_t len=strlen(str);
		WCHAR* newstr=new WCHAR[len+2];
		
		mbstowcs(newstr,str,len+1);

		wstring::assign(newstr);
		delete[] newstr;
	}

	CombString& operator=(const char *str)
	{
		assign(str);
		return *this;
	}

	operator const char*()
	{
		size_t size = wstring::size();

		if (_str)
			delete[] _str;
		_str=NULL;
		if (size>0)
		{
			_str=new char[size+4];
			wcstombs(_str,c_str(),size+2);
		}
		else
		{
			_str=new char[1];
			_str[0] = '\0';
		}

		return  _str;
	}

private:
	char* _str;
};

} // namespace common
} // namespace ZQ


#endif //	__ZQ_COM_CombString_H__
