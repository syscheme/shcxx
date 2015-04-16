/*
-------------------------------------------------------------
 $Id: expatxx.h,v 1.4 2003/03/20 04:54:26 shao Exp $
 $Author: Admin $
 $Revision: 1 $
 $Date: 10-11-12 15:56 $
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

// this class is created as a static liberary 21Jul02

#ifndef __EXPATXXX_H__
#define __EXPATXXX_H__

#include "ZQ_common_conf.h"
#include "expat.h"

namespace ZQ {
namespace common {

// -----------------------------
// Interface IXMLParserCallBack
// -----------------------------
class IXMLParserCallBack
{
public:
	virtual ~IXMLParserCallBack(){}
private:
	virtual void startElement(const XML_Char* name, const XML_Char** atts) =0;
	virtual void endElement(const XML_Char*) =0;
	virtual void charData(const XML_Char*, int len) =0;
	virtual void processingInstruction(const XML_Char* target, const XML_Char* data) =0;
	virtual void defaultHandler(const XML_Char*, int len) =0;
	virtual int  notStandaloneHandler() =0;
	virtual void unparsedEntityDecl(const XML_Char* entityName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId, const XML_Char* notationName) =0;
	virtual void notationDecl(const XML_Char* notationName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId) =0;
	virtual void startNamespace(const XML_Char* prefix, const XML_Char* uri) =0;
	virtual void endNamespace(const XML_Char*) =0;

	virtual void logicalClose() =0;
};

// -----------------------------
// class ExpatBaseEx
// -----------------------------
class ExpatBaseEx : public IXMLParserCallBack
{
public:
	ExpatBaseEx(bool createParser=true);
	virtual ~ExpatBaseEx();

	operator XML_Parser() const;
	
	bool emptyCharData(const XML_Char* s, int len); // utility often used in overridden charData

// overrideable callbacks, from IXMLParserCallBack
// ------------------------
	virtual void startElement(const XML_Char* name, const XML_Char** atts);
	virtual void endElement(const XML_Char*);
	virtual void charData(const XML_Char*, int len);
	virtual void processingInstruction(const XML_Char* target, const XML_Char* data);
	virtual void defaultHandler(const XML_Char*, int len);
	virtual int  notStandaloneHandler();
	virtual void unparsedEntityDecl(const XML_Char* entityName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId, const XML_Char* notationName);
	virtual void notationDecl(const XML_Char* notationName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId);
	virtual void startNamespace(const XML_Char* prefix, const XML_Char* uri);
	virtual void endNamespace(const XML_Char*);
	
	virtual void logicalClose();

// XML interfaces
	int        XML_Parse(const char* buffer, int len, int isFinal);
	XML_Error  XML_GetErrorCode();
	int        XML_GetCurrentLineNumber();
	const XML_Char* XML_GetErrorString();
	
protected:
	XML_Parser mParser;

	int depth; // for logical close

// interface functions for callbacks
public:
	static void startElementCallback(void *userData, const XML_Char* name, const XML_Char** atts);
	static void endElementCallback(void *userData, const XML_Char* name);
	static void startNamespaceCallback(void *userData, const XML_Char* prefix, const XML_Char* uri);
	static void endNamespaceCallback(void *userData, const XML_Char* prefix);
	static void charDataCallback(void *userData, const XML_Char* s, int len);
	static void processingInstructionCallback(void *userData, const XML_Char* target, const XML_Char* data);
	static void defaultHandlerCallback(void* userData, const XML_Char* s, int len);
	static int  notStandaloneHandlerCallback(void* userData);	
	static void unParsedEntityDeclCallback(void* userData, const XML_Char* entityName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId, const XML_Char* notationName);
	static void notationDeclCallback(void *userData, const XML_Char* notationName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId);
	
// utilities
	static int skipWhiteSpace(const char*);	
};


// -----------------------------
// class ExpatBaseNestingEx
// -----------------------------
class ExpatBaseNestingEx : public ExpatBaseEx
{
// subclass to support a hierarchy of parsers, in a sort of recursion or
// 'nesting' approach, where a top-level parser might create sub-parsers for part of a file

public:
	ExpatBaseNestingEx();
	ExpatBaseNestingEx(ExpatBaseNestingEx* parent);  // NOT a copy ctor!! this is a recursive situation
	virtual ~ExpatBaseNestingEx();
	
	ExpatBaseNestingEx* returnToParent();

protected:
	int	mDepth;	
	ExpatBaseNestingEx* mParent;

// interface functions for callbacks
public:
	static void nestedStartElementCallback(void* userData, const XML_Char* name, const XML_Char** atts);
	static void nestedEndElementCallback(void* userData, const XML_Char* name);
};


} // namespace common
} // namespace ZQ

#endif //__EXPATXXX_H__


/*
-------------------------------------------------------------
 Revision history since 2003/03/20:
 $Log: /ZQProjs/Common/expatxxx.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 4     08-12-24 14:44 Hongquan.zhang
// 
// 3     08-12-09 14:34 Yixin.tian
// 
// 2     07-11-02 11:24 Guan.han
// include "ZQ_common_conf.h" first
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

