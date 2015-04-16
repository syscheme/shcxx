/*
-------------------------------------------------------------
 $Id: expatxx.cpp,v 1.4 2003/03/20 04:54:26 shao Exp $
 $Author: Hui.shao $
 $Revision: 2 $
 $Date: 3/19/15 9:55a $
------------------------------------------------------------

Copyright (C) 2002-2003 Infraworks Corp.
All Rights Reserved, Infraworks Corp.

This program is a part of Infraworks software, it is distributed by
Infraworks Corp.

This program may be originally introduced from ShCxx into this source
tree for the project of InTether Community Server. ShCxx is a freeware
created by Hui Shao. (C) 2000-2001 Hui Shao, please see the GNU General
Public License for more details.

*/

#include "expatxxx.h"
#include <assert.h>

#ifdef WIN32 
#  if defined(_MT) || defined(MT)
#     pragma comment(lib, "libexpatMT.lib")
#  else
#     pragma comment(lib, "libexpat.lib")
#  endif
#endif // WIN32

// -----------------------------
// class ExpatBaseEx
// -----------------------------

namespace ZQ {
namespace common {

ExpatBaseEx::ExpatBaseEx(bool createParser)
	      :depth(0)
{
	if (createParser) {
		// subclasses may call this ctor after parser created!
		mParser = XML_ParserCreate(0);
		::XML_SetUserData(mParser, this);
		::XML_SetElementHandler(mParser, startElementCallback, endElementCallback);
		::XML_SetCharacterDataHandler(mParser, charDataCallback);
		::XML_SetProcessingInstructionHandler(mParser, processingInstructionCallback);
		::XML_SetDefaultHandler(mParser, defaultHandlerCallback);
		::XML_SetUnparsedEntityDeclHandler(mParser, unParsedEntityDeclCallback);
		::XML_SetNotationDeclHandler(mParser, notationDeclCallback);
		::XML_SetNotStandaloneHandler(mParser, notStandaloneHandlerCallback);
		::XML_SetNamespaceDeclHandler(mParser, startNamespaceCallback, endNamespaceCallback);
		::XML_SetParamEntityParsing(mParser, XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
	}
}


ExpatBaseEx::~ExpatBaseEx()
{
	if (mParser)  // allows subclasses to avoid finishing parsing
	  ::XML_ParserFree(mParser);
}


ExpatBaseEx::operator XML_Parser() const
{
	return mParser;
}

int ExpatBaseEx::XML_Parse(const char *s, int len, int isFinal)
{
	return ::XML_Parse(mParser, s, len, isFinal);
}

XML_Error ExpatBaseEx::XML_GetErrorCode()
{
	return ::XML_GetErrorCode(mParser);
}

int ExpatBaseEx::XML_GetCurrentLineNumber()
{
	return ::XML_GetCurrentLineNumber(mParser);
}

const XML_Char* ExpatBaseEx::XML_GetErrorString()
{
	return ::XML_ErrorString(XML_GetErrorCode());
}

void ExpatBaseEx::startElementCallback(void *userData, const XML_Char* name, const XML_Char** atts)
{
	((ExpatBaseEx*)userData)->depth++;
	((ExpatBaseEx*)userData)->startElement(name, atts);
}

void ExpatBaseEx::endElementCallback(void *userData, const XML_Char* name)
{
	((ExpatBaseEx*)userData)->depth--;
	((ExpatBaseEx*)userData)->endElement(name);

	if (((ExpatBaseEx*)userData)->depth ==0)
		((ExpatBaseEx*)userData)->logicalClose();
}

void ExpatBaseEx::startNamespaceCallback(void *userData, const XML_Char* prefix, const XML_Char* uri)
{
	((ExpatBaseEx*)userData)->startNamespace(prefix, uri);
}

void ExpatBaseEx::endNamespaceCallback(void *userData, const XML_Char* prefix)
{
	((ExpatBaseEx*)userData)->endNamespace(prefix);
}

void ExpatBaseEx::charDataCallback(void *userData, const XML_Char* s, int len)
{
	((ExpatBaseEx*)userData)->charData(s, len);
}

void ExpatBaseEx:: processingInstructionCallback(void *userData, const XML_Char* target, const XML_Char* data)
{
	((ExpatBaseEx*)userData)->processingInstruction(target, data);
}

void ExpatBaseEx::defaultHandlerCallback(void* userData, const XML_Char* s, int len)
{
	((ExpatBaseEx*)userData)->defaultHandler(s, len);
}

int ExpatBaseEx::notStandaloneHandlerCallback(void* userData)
{
	return ((ExpatBaseEx*)userData)->notStandaloneHandler();
}

void ExpatBaseEx::unParsedEntityDeclCallback(void* userData, const XML_Char* entityName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId, const XML_Char* notationName)
{
	((ExpatBaseEx*)userData)->unparsedEntityDecl(entityName, base, systemId, publicId, notationName);
}

void ExpatBaseEx::notationDeclCallback(void *userData, const XML_Char* notationName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId)
{
	((ExpatBaseEx*)userData)->notationDecl(notationName, base, systemId, publicId);
}

//int ExpatBaseEx::externalEntityRefCallback(XML_Parser parser, const XML_Char* openEntityNames, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId)
//{
//	((ExpatBaseEx*)parser)->externalEntityRef(openEntityNames, base, systemId, publicId);
//}


void ExpatBaseEx::startElement(const XML_Char*, const XML_Char**)
{
}


void ExpatBaseEx::endElement(const XML_Char*)
{
}

void ExpatBaseEx::startNamespace(const XML_Char* /* prefix */, const XML_Char* /* uri */)
{
}

void ExpatBaseEx::logicalClose()
{
}

void ExpatBaseEx::endNamespace(const XML_Char*)
{
}

void ExpatBaseEx::charData(const XML_Char*, int )
{
}

void
ExpatBaseEx::processingInstruction(const XML_Char*, const XML_Char*)
{
}

void
ExpatBaseEx::defaultHandler(const XML_Char*, int)
{
}


int ExpatBaseEx::notStandaloneHandler()
{
	return 0;
}

void ExpatBaseEx::unparsedEntityDecl(const XML_Char*, const XML_Char*, const XML_Char*, const XML_Char*, const XML_Char*)
{
}

void ExpatBaseEx::notationDecl(const XML_Char*, const XML_Char*, const XML_Char*, const XML_Char*)
{
}

int ExpatBaseEx::skipWhiteSpace(const char* startFrom)
{
	// use our own XML definition of white space
	// TO DO - confirm this is correct!
	const char* s = startFrom;
	char c = *s;
	while ((c==' ') || (c=='\t') || (c=='\n') || (c=='\r')) {
		s++;
		c = *s;
	}
	
	return (int)(s - startFrom);
}

bool ExpatBaseEx::emptyCharData(const XML_Char *s, int len)
{
// usually call from top of overriden charData methods
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


// -----------------------------
// class ExpatBaseNestingEx
// -----------------------------
ExpatBaseNestingEx::ExpatBaseNestingEx()
              : mDepth(0), mParent(0)
{
// WARNING
// the assumption that is not obvious here is that if you want to use 
// nested parsers, then your topmost parser must also be an ExpatBaseNestingEx
// subclass, NOT an ExpatBaseEx subclass, because we need the following special
// callbacks to override those in the ExpatBaseEx ctor
	::XML_SetElementHandler(mParser, nestedStartElementCallback, nestedEndElementCallback);
}

ExpatBaseNestingEx::ExpatBaseNestingEx(ExpatBaseNestingEx* parent)
              :	ExpatBaseEx(false),  // don't create parser - we're taking over from inParent
                mDepth(0), mParent(parent)
{
	mParser = parent->mParser;
	assert(mParser);
	::XML_SetUserData(mParser, this);
}


ExpatBaseNestingEx::~ExpatBaseNestingEx()
{
	assert(!mParent);  // if we are a sub-parser, should not delete without calling returnToParent
}


ExpatBaseNestingEx* ExpatBaseNestingEx::returnToParent()
{
	ExpatBaseNestingEx* ret = mParent;
	::XML_SetUserData(mParser, mParent);
	mParent=0;
	mParser=0;  // prevent parser shutdown!!
	delete this;  // MUST BE LAST THING CALLED IN NON-VIRTUAL FUNCTION
	return ret;
}

void ExpatBaseNestingEx::nestedStartElementCallback(void *userData, const XML_Char* name, const XML_Char** atts)
{
	ExpatBaseNestingEx* nestedParser = (ExpatBaseNestingEx*)userData;
	nestedParser->mDepth++;
	((ExpatBaseEx*)userData)->startElement(name, atts);  // probably user override
}

void ExpatBaseNestingEx::nestedEndElementCallback(void *userData, const XML_Char* name)
{
	ExpatBaseNestingEx* nestedParser = (ExpatBaseNestingEx*)userData;
// we don't know until we hit a closing tag 'outside' us that our run is done 	
	if (nestedParser->mDepth==0) {
		ExpatBaseNestingEx* parentParser = nestedParser->returnToParent();
		nestedEndElementCallback(parentParser, name);   // callbacks for ExpatBaseNestingEx stay registered, so safe 
		//if we don't invoke their callback, they will not balance their mDepth
	}
	else {
	// end of an element this parser has started
		nestedParser->endElement(name);  // probably user override
		if ((--nestedParser->mDepth) ==0)
			nestedParser->logicalClose();
	}
}

} // namespace common
} // namespace ZQ



/*
-------------------------------------------------------------
 Revision history since 2003/03/20:
 $Log: /ZQProjs/Common/expatxxx.cpp $
// 
// 2     3/19/15 9:55a Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 3     08-03-03 16:51 Yixin.tian
// merged changes for linux
// 
// 2     07-11-07 16:27 Guan.han
// disable DTD
// 
// 1     07-01-23 17:19 Guan.han
// 
// 3     07-01-23 16:20 Guan.han
// 
// 2     07-01-23 15:52 Guan.han
// 
// 2     07-01-23 15:06 Guan.han
// 
// 1     07-01-18 16:06 Guan.han
 Revision 1.4  2003/03/20 04:54:26  shao
 no message

 Revision 1.3  2003/03/20 04:18:51  shao
 Copy to Infraworks VSS

-------------------------------------------------------------
*/

