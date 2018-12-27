// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
//
// Ident : $Id: ExpatXX.h,v 1.8 2004/05/26 09:32:35 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define ExpatXX
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/expatxx.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 7     08-12-25 14:13 Yixin.tian
// 
// 6     08-12-24 14:52 Hui.shao
// 
// 5     08-12-24 11:21 Hui.shao
// added ExpatException
// 
// 4     08-12-23 15:19 Hui.shao
// restructed the class
// 
// 3     07-01-23 12:18 Guan.han
// ===========================================================================

// this class is created as a static liberary 21Jul02

#ifndef __EXPATXX_H__
#define __EXPATXX_H__

#include "ZQ_common_conf.h"
#include "Exception.h"

#ifndef XML_Char
#  if defined(_UNICODE) || defined(UNICODE) // Information is UTF-16 encoded.
#  ifdef XML_UNICODE_WCHAR_T
   typedef wchar_t XML_Char;
   typedef wchar_t XML_LChar;
#  else
   typedef unsigned short XML_Char;
   typedef char XML_LChar;
#  endif // XML_UNICODE_WCHAR_T
#  else                  // Information is UTF-8 encoded.
  typedef char XML_Char;
  typedef char XML_LChar;
#  endif //XML_UNICODE
#endif // XML_Char

namespace ZQ {
namespace common {

class ZQ_COMMON_API ExpatBase;
class ZQ_COMMON_API ExpatException;
class  ExpatNest;
// -----------------------------
// class ExpatException
// -----------------------------
/// A sub-hierarchy for Expat classes.
class ExpatException : public Exception
{
public:
	ExpatException(const std::string &what_arg) throw()
	: Exception(what_arg) {}

	virtual ~ExpatException() throw()
	{}
};

// -----------------------------
// class ExpatBase
// -----------------------------
class ExpatBase
{
	friend class ExpatNest;

public:
	ExpatBase();
	virtual ~ExpatBase();

	typedef int16 ErrCode;

	bool emptyCharData(const XML_Char* s, int len); // utility often used in overridden OnCharData

	// Parse the specified buffer
	void parse(const char *szBuffer, const int nBufferLen, const int nFinal = 0); // throw(ExpatException);
	
	// Parse the specified file
	void parse(const char *szFilename); // throw(ExpatException);

	::std::string getHiberarchyName() const;
	
	virtual void processingInstruction(const XML_Char* target, const XML_Char* data);
	virtual void defaultHandler(const XML_Char*, int len);
	virtual int  notStandaloneHandler();
	virtual void unparsedEntityDecl(const XML_Char* entityName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId, const XML_Char* notationName);
	virtual void notationDecl(const XML_Char* notationName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId);

protected:
	// overrideable callbacks, from IXMLParserCallBack
	virtual void OnStartElement(const XML_Char* name, const XML_Char** atts);
	virtual void OnEndElement(const XML_Char*);
	virtual void OnCharData(const XML_Char*, int len);
	virtual void OnStartNamespace(const XML_Char* prefix, const XML_Char* uri);
	virtual void OnEndNamespace(const XML_Char*);
	virtual void OnLogicalClose();

	::std::string _lastErr;

private:

	ExpatNest* _nest;

	// interface functions for callbacks
	// utilities
	static int skipWhiteSpace(const char*);	
};



} // namespace common
} // namespace ZQ

#endif //__EXPATXX_H__

