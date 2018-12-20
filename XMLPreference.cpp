// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: XMLPreference.cpp,v 1.8 2004/08/09 10:08:56 jshen Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl XML preference
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/XMLPreference.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 30    06-12-08 14:32 Yonghua.deng
// 
// 29    06-06-15 16:49 Shuai.chen
// 
// 28    06-06-12 15:10 Hongquan.zhang
// 
// 27    05-08-25 22:35 Bernie.zhao
// 
// 26    05-08-25 20:32 Bernie.zhao
// added Utf-8 support
// 
// 25    05-08-10 17:09 Bernie.zhao
// fixed handle count increase problem
// 
// 24    05-05-13 14:10 Kaliven.lee
// 
// 23    05-05-13 14:10 Kaliven.lee
// fix bug when disable valate check
// 
// 22    05-03-14 15:14 Bernie.zhao
// ignore validate
// 
// 21    03-01-18 16:38 Kaliven.lee
// delete ConvertBSTRToString allocate string
// 
// 20    04-12-08 9:36 Bernie.zhao
// 
// 19    04-12-07 11:24 Bernie.zhao
// added error code output in save()
// 
// 18    04-11-17 15:08 Bernie.zhao
// modified open() method
// 
// 17    04-10-14 9:07 Bernie.zhao
// modified addNextChild() to avoid node missing
// 
// 16    04-10-11 15:31 Bernie.zhao
// fixed a mem clear bug in firstChild()
// 
// 15    04-10-10 13:59 Bernie.zhao
// added hasNextChild() method
// 
// 14    04-10-06 12:14 Bernie.zhao
// 
// 13    04-09-24 16:27 Bernie.zhao
// 
// 12    04-09-24 16:15 Bernie.zhao
// added toStream() function to dump node
// 
// 11    04-09-24 15:10 Bernie.zhao
// modified to support child pending
// 
// 10    04-09-23 10:19 Bernie.zhao
// 
// 9     04-09-03 17:16 Bernie.zhao
// 
// 8     04-09-03 15:41 Bernie.zhao
// 
// 7     9/02/04 4:20p Jie.zhang
// 
// 6     8/26/04 5:05p Jie.zhang
// 
// 5     8/26/04 3:31p Jie.zhang
// 
// 4     8/26/04 2:09p Jie.zhang
// 
// 3     8/26/04 2:06p Jie.zhang
// Revision 1.8  2004/08/09 10:08:56  jshen
// replace CW2CT with CW2A
//
// Revision 1.7  2004/07/21 11:58:12  shao
// added comments
//
// Revision 1.6  2004/07/14 09:39:36  bzhao
// has(),set(),remove() impl
// turned fallback in get() from "NULL"2""
//
// Revision 1.5  2004/07/14 06:13:23  shao
// throw Exception if open file failed or syntax error
//
// Revision 1.4  2004/07/07 11:32:29  shao
// throw()
//
// Revision 1.3  2004/07/07 02:48:32  shao
// MSXML impl
//
// ===========================================================================

#include "XMLPreference.h"
#include "CombString.h"
#include <stdio.h>
#include <comutil.h>

#include <msxml2.h>
#include <atlbase.h>
#include <comutil.h>

#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "msxml2.lib")

/*
#ifndef CLSID_IXMLDOMDocument2
#  define CLSID_IXMLDOMDocument2 {0x88d969c0, 0xf192, 0x11d4, {0xa6, 0x5f, 0x00, 0x40, 0x96, 0x32, 0x51, 0xe5}}
#endif// CLSID_IXMLDOMDocument
*/

namespace ZQ {
namespace common {


// -----------------------------
// class XMLPreference
// -----------------------------
XMLPreference::XMLPreference(XMLPrefDoc& document, IXMLDOMElement *pElement)
:_doc(document), _elem(pElement)
{
	if (_elem == NULL)
		throw Exception("invalide element reference");
}

XMLPreference::~XMLPreference()
{
	// clean up the search result
	IPreference *next =NULL;
	while((next = nextChild()) !=NULL)
		next->free();

	// release the com instance itself
	if (_elem != NULL)
		_elem->Release();
	_elem = NULL;
}

void XMLPreference::free()
{
	delete ((XMLPreference*)this);
}

#ifndef CW2A
#define  CW2A(x)	_com_util::ConvertBSTRToString(x)
#endif 

const char* XMLPreference::name(char* name, const int size /*=-1*/)
{
	if (name == NULL || _elem==NULL)
		return NULL;

	name[0] = '\0'; // 

	CComBSTR bstrName;

	if (FAILED(_elem->get_tagName(&bstrName)))
	{
		name[0] = '\0'; // empty name
	}
	else
	{
		if (size<0)
		{
			char* str = CW2A(bstrName);
			strcpy(name, str);
			delete[] str;
		}
		else
		{
			char* str = CW2A(bstrName);
			strncpy(name, str, size-1);
			delete[] str;
		}
	}

	return name;
}

bool XMLPreference::has(const char *key)
{
	CComVariant varAttrValue;
	if (FAILED(_elem->getAttribute(CComBSTR(key), &varAttrValue)))
		return false;
	if(varAttrValue.pbstrVal==NULL)
		return false;
	return true;
}

PreferenceDoc* XMLPreference::doc()
{
	return &_doc;
}

const char* XMLPreference::gettext(char* value, const char* fallback, int maxValueSize)
{
	if (value == NULL)
		return NULL;

	if (maxValueSize>=0)
		strncpy(value, fallback, maxValueSize); 
	else {
		if(fallback==NULL)
			strcpy(value, "");
		else
			strcpy(value, fallback); 
	}

	CComBSTR bstr;

	if (FAILED(_elem->get_text(&bstr)))
		return value;

	if(bstr.m_str != NULL)
	{
		char* pStr = CW2A(bstr);
		if (maxValueSize>0)
			strncpy(value, pStr, maxValueSize);
		else
			strcpy(value, pStr);
		delete[] pStr;
	}

	return value;
}

void XMLPreference::settext(const char* value)
{
	if (value==NULL)
		return;

	CComBSTR bstr(value);

	_elem->put_text(bstr);
}

const char* XMLPreference::get(const char* key, char* value, const char* fallback /*=NULL*/, int valueSize /*=-1*/)
{
	if (key == NULL || *key =='\0' || value == NULL)
		return NULL;

	if (valueSize>=0)
		strncpy(value, fallback, valueSize); 
	else {
		if(fallback==NULL)
			strcpy(value, "");
		else
			strcpy(value, fallback); 
	}

/*
static const CLSID AttrClsId = {0x2933bf86, 0x7b36, 0x11d2, {0xb2, 0x0e, 0x00, 0xc0, 0x4f, 0x98, 0x3e, 0x60}};
	IXMLDOMElement *pElem =NULL;
	HRESULT hr = _node->QueryInterface(AttrClsId,(void**)&pElem);
	if(!SUCCEEDED(hr)) //this node is not element node
		return value;
*/

	// Retrieve the value for the attribute.
	CComVariant varAttrValue;
	if (FAILED(_elem->getAttribute(CComBSTR(key), &varAttrValue)))
		return value;

//	WCHAR *pwValue =( WCHAR*)varAttrValue.pbstrVal;
	if(varAttrValue.pbstrVal!=NULL)
	{
		CombString cstr((WCHAR*)varAttrValue.pbstrVal);
	if (valueSize>0)
		strncpy(value, cstr, valueSize); 
	else
		strcpy(value, cstr); 
	}

	return value;
/*
WCHAR *pwValue =( WCHAR*)varAttrValue.pbstrVal;
	if(pwValue==NULL)
		return value;

	size_t len = (valueSize<0) ? (2*(wcslen(pwValue)+1)) : valueSize;
	wcstombs(value, pwValue, len);
	return value;
*/
}

const char* XMLPreference::getUtf8(const char* key, char* value, const char* fallback /*=NULL*/, int valueSize /*=-1*/)
{
	if (key == NULL || *key =='\0' || value == NULL)
		return NULL;

	if (valueSize>=0)
		strncpy(value, fallback, valueSize); 
	else {
		if(fallback==NULL)
			strcpy(value, "");
		else
			strcpy(value, fallback); 
	}


	// Retrieve the value for the attribute.
	CComVariant varAttrValue;
	if (FAILED(_elem->getAttribute(CComBSTR(key), &varAttrValue)))
		return value;

	if(varAttrValue.pbstrVal!=NULL)
	{
		CombString cstr((WCHAR*)varAttrValue.pbstrVal);
		
		int convertBytes = (valueSize==-1)? cstr.length()*6 : valueSize;
		WideCharToMultiByte(CP_UTF8, 0, cstr.c_str(), cstr.length(), value, convertBytes, NULL, NULL);
	}

	return value;
}

/// Returns specified preference, in UNICODE format
/// @param key     the key to inquery
/// @param value   to hold the value of the key
/// @param fallback  the default value if the key doesn't exist,
///                  it will be copied into the value buf
/// @return        the string of the value
const wchar_t* XMLPreference::getUnicode(const wchar_t* key, wchar_t* value, const wchar_t* fallback /*= L""*/, int valueSize /*= -1*/)
{
	if (key == NULL || *key == '\0' || value == NULL)
		return NULL;
	
	if (valueSize >= 0)
		wcsncpy(value, fallback, valueSize);
	else
	{
		if (fallback == NULL)
			wcscpy(value, L"");
		else
			wcscpy(value, fallback);
	}
	CComVariant varAttrValue;
	if (FAILED(_elem->getAttribute(CComBSTR(key), &varAttrValue)))
		return value;

	if (varAttrValue.pbstrVal != NULL)
	{
		CombString cstr((wchar_t*)varAttrValue.pbstrVal);
		if (valueSize > 0)
			wcsncpy(value, cstr.c_str(), valueSize);
		else
			wcscpy(value, cstr.c_str());
	}
	return value;
}

void XMLPreference::set(const char *key, const char *value)
{
	if (key == NULL || *key =='\0' || value == NULL)
		return;

	CComVariant varAttrValue(value);
	
	// set attribute value
	_elem->setAttribute(CComBSTR(key), varAttrValue);
	
}

void XMLPreference::setUtf8(const char *key, const char *value)
{
	if (key == NULL || *key =='\0' || value == NULL)
		return;

	int			len = strlen(value);
	wchar_t*	wbuff = new wchar_t[len];
	
	MultiByteToWideChar(CP_UTF8, 0, value, -1, wbuff, len);

	CComVariant varAttrValue(wbuff);

	// set attribute value
	_elem->setAttribute(CComBSTR(key), varAttrValue);
	
	delete	wbuff;
}

void XMLPreference::remove(const char* key)
{
	if (key == NULL || *key =='\0')
		return;

	if( FAILED(_elem->removeAttribute(CComBSTR(key))) ) {
		// remove succeeded
		// TODO:	return
	}
	else {
		// remove failed
		// TODO:	return
	}

}

void XMLPreference::clean()
{
	//TODO: impl...
}

IPreference* XMLPreference::firstChild(const char* subPrefName /*=NULL*/)
{
	if (_elem ==NULL)
 		return NULL;

	// clean up the search result
	IPreference *resnext =NULL;
	while((resnext = nextChild()) !=NULL)
		resnext->free();
	_sres.clear();

/*
CComBSTR searchfor = (subPrefName == NULL || *subPrefName== '\0') ? "*" :subPrefName;

	IXMLDOMNodeList* pResList;
	long lCount =0;



	// Retrieve all nodes with the node name.
	if (S_OK != _elem->getElementsByTagName(searchfor, &pResList) || pResList ==NULL)
		return NULL;

	// Retrieve total number of nodes in the list.
    if (FAILED(pResList->get_length(&lCount)) || lCount <=0)
	{
		pResList->Release();
		return NULL;
	}
    
	// Iterate through the node list and put them in the search result vector 
	for (int i = 0; i < lCount; i++)
	{
		IXMLDOMNode* pXMLNode;
		if (FAILED(pResList->get_item(i, &pXMLNode)) || pXMLNode==NULL) 
			continue;

		IXMLDOMElement *pElem =NULL;
		if (S_OK == pXMLNode->QueryInterface(__uuidof(IXMLDOMElement),(void**)&pElem) && pElem !=NULL)
			_sres.push_back(pElem);

		pXMLNode->Release();
	}
*/
	IXMLDOMNode *next;
	HRESULT hr = _elem->get_firstChild(&next);
	while(!FAILED(hr) && next!=NULL)
	{
		IXMLDOMElement *pElem =NULL;
		if (S_OK == next->QueryInterface(__uuidof(IXMLDOMElement),(void**)&pElem) && pElem !=NULL)
		{
			XMLPreference *pref = new XMLPreference(_doc, pElem);
			char buf[512];
			if (subPrefName != NULL && strcmp(subPrefName, pref->name(buf, sizeof(buf))))
				delete pref;
			else _sres.push_back(pref);
		}
		
		IXMLDOMNode *tmp = next;

		hr = tmp->get_nextSibling(&next);

		tmp->Release();
	}

	return nextChild();
}

bool XMLPreference::hasNextChild()
{
	if(_sres.size()<=0)
		return FALSE;
	else
		return TRUE;
}

IPreference* XMLPreference::nextChild()
{
	if (_sres.size()<=0)
		return NULL;

	XMLPreference* nextPref = _sres.front();
    _sres.pop_front();

	return nextPref;
}

bool XMLPreference::addNextChild(IPreference* child)
{
	if(child ==NULL)
		return false;
	 
	VARIANT_BOOL vDeep(-1);
	IXMLDOMNode* NewChild;
	IXMLDOMNode* outNewChild;

	if(FAILED( ((XMLPreference*)child)->_elem->cloneNode( vDeep, &NewChild) ))
		return FALSE;
	
	if(FAILED(	_elem->appendChild(NewChild, &outNewChild) ))
	{
		NewChild->Release();
		return FALSE;
	}

	NewChild->Release();
	outNewChild->Release();
	return TRUE;
}

bool XMLPreference::removeChild(IPreference* child)
{
	if(child == NULL)
		return false;

	IXMLDOMNode* oldChild;
	HRESULT hret =	_elem->removeChild(((XMLPreference*)child)->_elem, &oldChild);
	if(hret==S_OK)
		return true;
	else
		return false;
}

bool XMLPreference::toStream(char* buff, unsigned long* count)
{
	if(!buff) {
		*count = 0;
		return FALSE;
	}
	BSTR xmlout;
	
	HRESULT hr = _elem->get_xml(&xmlout);
	if(hr!=S_OK) {
		*count = 0;
		return false;
	}
	char* ascout=_com_util::ConvertBSTRToString(xmlout);
	unsigned long dwcount = strlen(ascout);
	*count = (dwcount>*count)?*count:dwcount;

	strncpy(buff, ascout, *count+1);
	
	::SysFreeString(xmlout);
	delete[] ascout;

	return TRUE;
}	

// -----------------------------
// class XMLPrefDoc
// -----------------------------

XMLPrefDoc::XMLPrefDoc(const ComInitializer& init)
:_init(init), _doc(NULL)//, _root(NULL)//,_comsvr(NULL)
{
	if (FAILED(_init))
		throw Exception("illegal COM initializer unspecified");
}


XMLPrefDoc::~XMLPrefDoc()
{
	close();
}

void XMLPrefDoc::close()
{
	if (_doc)
	_doc->Release();
	_doc = NULL;
	_filename ="";
}

void XMLPrefDoc::set_root(IPreference* root)
{
	_doc->putref_documentElement(((XMLPreference*)root)->_elem);
}

bool XMLPrefDoc::open(const char* filename, int opentype /*=XMLDOC_READONLY*/)
{
	if (filename ==NULL)
		return false;

	//if (_filename == filename)
	//	return true;

	close();

	/*
	// create the object IXMLDOMDocument2 instance
	//	static const CLSID ClsId = {0x88d969c0, 0xf192, 0x11d4, {0xa6, 0x5f, 0x00, 0x40, 0x96, 0x32, 0x51, 0xe5}};
	//	HRESULT hr = CoInitialize(NULL); // Check the return value, hr...
	HRESULT hr = CoCreateInstance(CLSID_WMSServer, NULL, CLSCTX_ALL, 
	IID_IWMSServer, (void**)&_comsvr);
	if (FAILED(hr))
	return false;

	// Create the playlist object.
	hr = pServer->CreatePlaylist(&_doc);
	if (FAILED(hr))
	{
	close();
	return false;
	}
	*/
		

	// add by dony 


	 if (FAILED(::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_SERVER, 
       IID_IXMLDOMDocument2,(LPVOID*)&_doc)))
	{
		close();
		return false;
	}

	_filename = filename;

	// create a dummy doc
	if(opentype == XMLDOC_DUMMY)
		return true;

	// create new doc
	if(opentype == XMLDOC_CREATE) {
		save();
		return true;
	}
	
	VARIANT source = _variant_t(filename).Detach();
	VARIANT_BOOL succ;
	VARIANT_BOOL bValid(FALSE);
		
	// disable validate, currently we only care the well-formatted doc
	HRESULT hr = _doc->put_validateOnParse(VARIANT_FALSE);

	 hr = _doc->put_resolveExternals(VARIANT_FALSE);
	hr = _doc->load(source, &succ);
	VariantClear(&source);

	if (FAILED(hr) || !succ)
	{
		IXMLDOMParseError *pIParseError = NULL;
		if (FAILED(_doc->get_parseError(&pIParseError))||pIParseError==NULL)
		{
			close();
			throw Exception(std::string("can not open ") +filename);
		}

		long errcode, linenum;
		CComBSTR reason;
		bool getErrSucc = !FAILED(pIParseError->get_errorCode(&errcode));
		getErrSucc = getErrSucc && !FAILED(pIParseError->get_line(&linenum));
		getErrSucc = getErrSucc && !FAILED(pIParseError->get_reason(&reason));

		pIParseError->Release();
		pIParseError = NULL;
		close();

		if (getErrSucc)
		{
			char buf[1024];
			char* str = CW2A(reason);
			sprintf(buf, "syntax error(0x%x): %s(%d) %s",errcode, filename, linenum, str);
			delete[] str;
			throw Exception(buf);
		}
		else
			throw Exception(std::string("can not open ") +filename);

		return false;
	}

	return true;
}

IPreference* XMLPrefDoc::root()
{
	if (!isValid())
		return NULL;

	IXMLDOMElement* rootElem=NULL;
	if(FAILED(_doc->get_documentElement(&rootElem)) || rootElem ==NULL)
		return NULL;

	return new XMLPreference(*this, rootElem);
}

bool XMLPrefDoc::read(const void *buffer, const int buflen, const int nFinal /*=0*/)
{
	//TODO: impl
	if (buffer==NULL)
		return false;

	close();

	if (FAILED(::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
       IID_IXMLDOMDocument2,(LPVOID*)&_doc)))
	{
		close();
		return false;
	}

	_filename = "";

	CComBSTR source((const char*)buffer);
	VARIANT_BOOL succ;
	HRESULT hr = _doc->loadXML(source, &succ);
	
	if (FAILED(hr) || !succ)
	{
		IXMLDOMParseError *pIParseError = NULL;
		if (FAILED(_doc->get_parseError(&pIParseError))||pIParseError==NULL)
		{
			close();
			throw Exception(std::string("can not open "));
		}

		long errcode, linenum;
		CComBSTR reason;
		bool getErrSucc = !FAILED(pIParseError->get_errorCode(&errcode));
		getErrSucc = getErrSucc && !FAILED(pIParseError->get_line(&linenum));
		getErrSucc = getErrSucc && !FAILED(pIParseError->get_reason(&reason));

		pIParseError->Release();
		pIParseError = NULL;
		close();

		if (getErrSucc)
		{
			char buf[1024];
			char* str = CW2A(reason);
			sprintf(buf, "syntax error(0x%x): (%d) %s",errcode, linenum, str);
			delete[] str;
			throw Exception(buf);
		}
		else
			throw Exception(std::string("can not open "));

		return false;
	}

	return true;
}

IPreference* XMLPrefDoc::newElement(const char* tagname)
{
	CComBSTR tag(tagname);
	IXMLDOMElement* ele;
	HRESULT hr = _doc->createElement(tag, &ele);

	if(FAILED(hr)){
		throw Exception(std::string("Can not create element ")+tagname);
		return false;	
	}

	return new XMLPreference(*this, ele);
}

bool XMLPrefDoc::save(const char* filename /*=NULL*/)
{
	if (!isValid())
		return false;

	char buff[MAX_PATH];
	if(!filename)
		strcpy(buff, _filename.c_str());
	else
		strcpy(buff, filename);

	CComBSTR bfile(buff);
	CComVariant varString(bfile);

	HRESULT hr = _doc->save(varString);
    if(SUCCEEDED(hr))
         return true;
	
	if(FAILED(hr)){
		std::string errstr("Can not save to ");
		char errbuff[16];
		errstr+=buff;
		errstr+=". Error code:";
		errstr+=itoa(hr, errbuff, 16);
		throw Exception(errstr);
		return false;	
	}
		
	return false;
}

} // namespace common
} // namespace ZQ

