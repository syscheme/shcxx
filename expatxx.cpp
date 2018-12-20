// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai,,
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
// Desc  : impl ExpatXX
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/expatxx.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 6     08-12-24 11:21 Hui.shao
// added ExpatException
// 
// 5     08-12-23 15:19 Hui.shao
// restructed the class
// 
// 4     08-03-03 16:50 Yixin.tian
// merged changes for linux
// 
// 3     07-01-23 12:18 Guan.han
// 
// 1     2/27/06 4:53p Hui.shao
// ===========================================================================


#include "expatxx.h"
#include "expat.h"
#include <assert.h>

#include <vector>
#include <fstream>
#include <iostream>

#ifdef WIN32 
#  if defined(_MT) || defined(MT)
#     pragma comment(lib, "libexpatMT.lib")
#  else
#     pragma comment(lib, "libexpat.lib")
#  endif
#endif // WIN32

namespace ZQ {
namespace common {

// -----------------------------
// class ExpatNest
// -----------------------------
// subclass to support a hierarchy of parsers, in a sort of recursion or
// 'nesting' approach, where a top-level parser might create sub-parsers for part of a file
class ExpatNest
{
	friend class ExpatBase;
public:
	ExpatNest(ExpatBase& owner);
	virtual ~ExpatNest();

	operator XML_Parser() const;

protected:
	//		int	_depth;	
	ExpatBase& _owner;
	XML_Parser _parser;
	typedef ::std::vector < ::std::string > ElemNameStack;
	ElemNameStack _elemNameStack;


public:
	// XML interfaces
	int        XML_Parse(const char* buffer, int len, int isFinal);
	XML_Error  XML_GetErrorCode();
	int        XML_GetCurrentLineNumber();
	const XML_Char* XML_GetErrorString();

	// interface functions for callbacks
public:
	static void cb_startElement(void *userData, const XML_Char* name, const XML_Char** atts);
	static void cb_endElement(void *userData, const XML_Char* name);
	static void cb_startNamespace(void *userData, const XML_Char* prefix, const XML_Char* uri);
	static void cb_endNamespace(void *userData, const XML_Char* prefix);
	static void cb_charData(void *userData, const XML_Char* s, int len);
	static void cb_processingInstruction(void *userData, const XML_Char* target, const XML_Char* data);
	static void cb_defaultHandler(void* userData, const XML_Char* s, int len);
	static int  cb_notStandaloneHandler(void* userData);	
	static void cb_unParsedEntityDecl(void* userData, const XML_Char* entityName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId, const XML_Char* notationName);
	static void cb_notationDecl(void *userData, const XML_Char* notationName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId);

};

// -----------------------------
// class ExpatBase
// -----------------------------
ExpatNest::ExpatNest(ExpatBase& owner)
: _owner(owner), _parser(NULL)
{
	_parser = XML_ParserCreate(0);
	::XML_SetUserData(_parser, &_owner);
	::XML_SetElementHandler(_parser, cb_startElement, cb_endElement);
	::XML_SetCharacterDataHandler(_parser, cb_charData);
	::XML_SetProcessingInstructionHandler(_parser, cb_processingInstruction);
	::XML_SetDefaultHandler(_parser, cb_defaultHandler);
	::XML_SetUnparsedEntityDeclHandler(_parser, cb_unParsedEntityDecl);
	::XML_SetNotationDeclHandler(_parser, cb_notationDecl);
	::XML_SetNotStandaloneHandler(_parser, cb_notStandaloneHandler);
	::XML_SetNamespaceDeclHandler(_parser, cb_startNamespace, cb_endNamespace);

	//	::XML_SetElementHandler(_parser, nestedStartElementCallback, nestedEndElementCallback);
}

ExpatNest::~ExpatNest()
{
	if (NULL != _parser)
		::XML_ParserFree(_parser);

	_parser = NULL;
}

ExpatNest::operator XML_Parser() const
{
	return _parser;
}

int ExpatNest::XML_Parse(const char *s, int len, int isFinal)
{
	return ::XML_Parse(_parser, s, len, isFinal);
}

XML_Error ExpatNest::XML_GetErrorCode()
{
	return ::XML_GetErrorCode(_parser);
}

int ExpatNest::XML_GetCurrentLineNumber()
{
	return ::XML_GetCurrentLineNumber(_parser);
}

const XML_Char* ExpatNest::XML_GetErrorString()
{
	return ::XML_ErrorString(XML_GetErrorCode());
}

void ExpatNest::cb_startElement(void *userData, const XML_Char* name, const XML_Char** atts)
{
	ExpatBase* pExpatB = (ExpatBase*)userData;
	if (NULL == pExpatB || NULL == pExpatB->_nest)
		return;

	pExpatB->_nest->_elemNameStack.push_back(name ? name : "");

	try {
		((ExpatBase*)userData)->OnStartElement(name, atts);
	}
	catch(...) {}
}

void ExpatNest::cb_endElement(void *userData, const XML_Char* name)
{
	ExpatBase* pExpatB = (ExpatBase*)userData;
	if (NULL == pExpatB || NULL == pExpatB->_nest)
		return;

	try {
		pExpatB->OnEndElement(name);
	}
	catch(...) {}

	if (0 == (*--pExpatB->_nest->_elemNameStack.end()).compare(name))
	{
		pExpatB->_nest->_elemNameStack.pop_back();
		if (0 == pExpatB->_nest->_elemNameStack.size())
		{
			try {
				pExpatB->OnLogicalClose();
			}
			catch(...) {}
		}
	}
}

void ExpatNest::cb_startNamespace(void *userData, const XML_Char* prefix, const XML_Char* uri)
{
	((ExpatBase*)userData)->OnStartNamespace(prefix, uri);
}

void ExpatNest::cb_endNamespace(void *userData, const XML_Char* prefix)
{
	((ExpatBase*)userData)->OnEndNamespace(prefix);
}

void ExpatNest::cb_charData(void *userData, const XML_Char* s, int len)
{
	((ExpatBase*)userData)->OnCharData(s, len);
}

void ExpatNest::cb_processingInstruction(void *userData, const XML_Char* target, const XML_Char* data)
{
	((ExpatBase*)userData)->processingInstruction(target, data);
}

void ExpatNest::cb_defaultHandler(void* userData, const XML_Char* s, int len)
{
	((ExpatBase*)userData)->defaultHandler(s, len);
}

int ExpatNest::cb_notStandaloneHandler(void* userData)
{
	return ((ExpatBase*)userData)->notStandaloneHandler();
}

void ExpatNest::cb_unParsedEntityDecl(void* userData, const XML_Char* entityName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId, const XML_Char* notationName)
{
	((ExpatBase*)userData)->unparsedEntityDecl(entityName, base, systemId, publicId, notationName);
}

void ExpatNest::cb_notationDecl(void *userData, const XML_Char* notationName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId)
{
	((ExpatBase*)userData)->notationDecl(notationName, base, systemId, publicId);
}

// -----------------------------
// class ExpatBase
// -----------------------------
ExpatBase::ExpatBase()
:_nest(NULL)
{
//	_nest = new ExpatNest(*this);
}

ExpatBase::~ExpatBase()
{
	if (_nest)
		delete _nest;
	_nest = NULL;
}

void ExpatBase::parse(const char *szBuffer, const int nBufferLen, const int nFinal)
 throw(ExpatException)
{
	if (NULL == _nest)
		_nest = new ExpatNest(*this);

	// Parse the specified buffer
	// If there is an error, create an error string and throw
	if (!_nest->XML_Parse(szBuffer, nBufferLen, nFinal))
	{
		char lineno[16];
		itoa(_nest->XML_GetCurrentLineNumber(), lineno, 10);
		_lastErr = std::string("line(") + lineno + ") " + _nest->XML_GetErrorString();
		throw ExpatException(_lastErr);
	}
}

void ExpatBase::parse(const char *szFilename)
 throw(ExpatException)
{
	char szBuffer[8192];
	bool done = false;
	
	// Open the specified file
	std::ifstream from(szFilename);
	if (from.rdstate())
	{
		_lastErr = std::string("Could not open input file ") + (szFilename ? szFilename : "null");
		throw ExpatException(_lastErr);
	}
	
	// Read to EOF
	while (!done)
	{
		// Read data from the input file; store the bytes read
		from.read(szBuffer, sizeof(szBuffer));
		done = from.eof();

		// Parse the data read
		parse(szBuffer, from.gcount(), done);
	}
}

::std::string ExpatBase::getHiberarchyName() const
{
	if (!_nest)
		return "";

	std::string hname;

	for (ExpatNest::ElemNameStack::iterator it = _nest->_elemNameStack.begin(); it < _nest->_elemNameStack.end(); it++)
		hname += std::string(LOGIC_FNSEPS) + *it;

	return hname;
}

void ExpatBase::OnStartElement(const XML_Char*, const XML_Char**)
{
}

void ExpatBase::OnEndElement(const XML_Char*)
{
}

void ExpatBase::OnStartNamespace(const XML_Char* /* prefix */, const XML_Char* /* uri */)
{
}

void ExpatBase::OnLogicalClose()
{
}

void ExpatBase::OnEndNamespace(const XML_Char*)
{
}

void ExpatBase::OnCharData(const XML_Char*, int )
{
}

void ExpatBase::processingInstruction(const XML_Char*, const XML_Char*)
{
}

void ExpatBase::defaultHandler(const XML_Char*, int)
{
}

int ExpatBase::notStandaloneHandler()
{
	return 0;
}

void ExpatBase::unparsedEntityDecl(const XML_Char*, const XML_Char*, const XML_Char*, const XML_Char*, const XML_Char*)
{
}

void ExpatBase::notationDecl(const XML_Char*, const XML_Char*, const XML_Char*, const XML_Char*)
{
}

int ExpatBase::skipWhiteSpace(const char* startFrom)
{
	// use our own XML definition of white space
	// TO DO - confirm this is correct!
	const char* s = startFrom;
	char c = *s;
	while ((c==' ') || (c=='\t') || (c=='\n') || (c=='\r')) {
		s++;
		c = *s;
	}
	const int numSkipped = s - startFrom;
	return numSkipped;
}

bool ExpatBase::emptyCharData(const XML_Char *s, int len)
{
	// usually call from top of overriden OnCharData methods
	if (len==0)
		return true;  //*** early exit - empty string, may never occur??

	// skip newline and empty whitespace
	if (
		((len==1) && ( (s[0]=='\n') || (s[0]=='\r')) ) ||  // just CR or just LF
		((len==2) && (s[0]=='\r') && (s[1]=='\n'))  // DOS-style CRLF
		)
		return true;  //*** early exit - newline

	const int lastCharAt = len-1;
	if (s[lastCharAt]==' ') {  // maybe all whitespace
		int i;
		for (i=0; i<lastCharAt; i++) {
			if (s[i]!=' ')
				break;
		}
		if (i==lastCharAt)
			return true;	  //*** early exit - all spaces
	}
	return false;
}

/// -----------------------------
/// class ExpatException
/// -----------------------------
ExpatException::ExpatException(const std::string &what_arg) throw()
:Exception(what_arg)
{
}

ExpatException::~ExpatException() throw()
{
}


} // namespace common
} // namespace ZQ
