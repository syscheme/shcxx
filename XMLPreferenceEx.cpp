/*
-------------------------------------------------------------
 $Id: XMLPreferenceEx.cpp,v 1.10 2003/03/26 00:03:35 shao Exp $
 $Author: Hui.shao $
 $Revision: 2 $
 $Date: 3/19/15 10:07a $
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

#include "XMLPreferenceEx.h"
#include <fstream>

extern "C"
{
	#include <stdio.h>
};

namespace ZQ{
namespace common{
	
// 获得conString中右边第一个vChar后面的字符串，若没有vChar则返回空字符串。
void XMLPreferenceEx::GetStringRightAtChar(const ::std::string& contString,char vChar,::std::string &strTemp)
{
	int strLen = (int)contString.size();
	int cur;
	for(cur = strLen-1;cur >= 0;cur--)
	{
		if(contString[cur] == vChar)
			break;
	}
	if(cur < 0)
	{
		strTemp = "";
		return;
	}
	else
	{
		cur++;
		while(cur<strLen)
		{
			strTemp += contString[cur];
			cur++;
		}
	}
}

XMLPreferenceEx::XMLPreferenceEx(XMLPreferenceEx::itor_t mapnode, XMLPreferenceDocumentEx* doc)
			  :pMapnode(mapnode), pDoc(doc)
			   
{
	cursor=wkchild.end();
	
}

XMLPreferenceEx::~XMLPreferenceEx()
{
}

/*
static void XMLPreferenceDestroyer(XMLPreferenceEx** ppref)
{
	if (ppref == NULL || *ppref == NULL)
		return;

	XMLPreferenceEx* p = (XMLPreferenceEx*) *ppref;

	delete p;
	*ppref = NULL;
}
*/

bool XMLPreferenceEx::getPreferenceText(void *name, const int maxsize)
{
	if (name == NULL)
		return false;
	snprintf((char*) name, maxsize, "%s", (*pMapnode).second.content.c_str());
	return true;
}

bool XMLPreferenceEx::getPreferenceName(void *name, bool basenameonly, const int maxsize,  const int charsize)
{
	if (name ==NULL)
		return false;
//	if (pMapnode == NULL)
//		return false;

	// Helper variables
	std::string element_name = (*pMapnode).first;

	if (basenameonly)
	{
		// Determine the element's base name
		int pos = (int)element_name.rfind("/", element_name.length()-1);
		if (pos > -1)
			element_name = element_name.substr(pos+1, element_name.length()-pos-1);
	}

	if (charsize ==1)
	{
		if (maxsize >0)
			snprintf((char*)name, maxsize -1, element_name.c_str());
		else
			sprintf((char*)name, element_name.c_str());
	}
	else
	{
#ifdef ZQ_OS_MSWIN
		if (maxsize >0)
			snwprintf((wchar_t *)name, maxsize/2 -1, L"%S", element_name.c_str());
		else
			swprintf((wchar_t *)name, L"%S",element_name.c_str());
#else
		if (maxsize >0)
			swprintf((wchar_t *)name, maxsize/2 -1, L"%S", element_name.c_str());
		else
		{
			int len = (element_name.length() + 1)/2;
			swprintf((wchar_t *)name, len + 1,L"%S",element_name.c_str());
		}
#endif
	}

	return true;
}

XMLPreferenceEx* XMLPreferenceEx::findChild(const char* name, const int num)
{
	XMLPreferenceEx* pRet = NULL;

	if (num == 1)
		pRet = findSubPreference(name);
	else 
	{
		int cur = 0;
		XMLPreferenceEx* pChild = firstChild(name);
		while (pChild != NULL && ++cur <= num)
		{
			pRet = pChild;
			if (cur == num)
				break; // find
			// not found, release it
			pRet->free();
			pRet = NULL;

			if (hasNextChild())
				pChild = nextChild();
			else 
				pChild = NULL;
		}
	}

	return pRet;
}

bool XMLPreferenceEx::getIntProp(const char* name, int& value)
{
	value = -1;
    char buf[64];
    if(getAttributeValue(name, buf, sizeof(buf)))
    {
        value = atoi(buf);
        return true;
    }
    else
    {
        return false;
    }
}

bool XMLPreferenceEx::getStrProp(const char* name, char* buff, const int buffSize)
{
    return getAttributeValue(name, buff, buffSize);
}

const char* XMLPreferenceEx::name(char* name, const int size)
{
	char buff[MAX_PATH];
	memset(buff,0,MAX_PATH);
	getPreferenceName(buff);
	::std::string tmp;
	GetStringRightAtChar(buff,'/',tmp);
	memset(name,0,sizeof(name));
	if(size<0)
		sprintf(name,tmp.c_str());
	else
	{
		snprintf(name,size-1,tmp.c_str());
		name[size-1] = '\0';
	}
	//getPreferenceName(name);
	return name;
}

bool XMLPreferenceEx::has(const char *key)
{
	if(key == NULL)
		return false;
	
	std::string aname = key;

	element_attribute_map_t *attributes = &(pMapnode->second.attributes);
	element_attribute_map_t::iterator a_itor = attributes->find(key);

	if (a_itor ==attributes->end())
		return false;
	
	return true;
}

const char* XMLPreferenceEx::get(const char* key, char* value, const char* fallback, int maxValueSize)
{
    if(getAttributeValue(key, value, maxValueSize))
    {
        return value;
    }
    else
    {
        if(value && fallback && 0 <= snprintf(value, maxValueSize, "%s", fallback))
        {
            return value;
        }
        else
        {
            return NULL;
        }
    }
}

std::map<std::string, std::string> XMLPreferenceEx::getProperties()
{
	return(pMapnode->second.attributes);
}

bool XMLPreferenceEx::getAttributeValue(const void *attrname, void *value, int maxvaluesize,  int charsize)
{
	if (attrname ==NULL || value == NULL)
		return false;
//	if(pMapnode == NULL)
//		return false;

	std::string aname;

	if (charsize ==1)
	{
		*((char*)value) = 0;
		aname = (const char*) attrname;
	}
	else
	{
		*((wchar_t*)value) = 0;
		char buf[MAX_PATH];
		wchar_t *wp=(wchar_t*) attrname;

		for (int i =0; i< MAX_PATH && *wp; i++, wp++)
			buf[i] = *wp & 0xff;

		aname = buf;
	}

	element_attribute_map_t *attributes = &(pMapnode->second.attributes);
	element_attribute_map_t::iterator a_itor = attributes->find(aname);

	if (a_itor ==attributes->end())
		return false;

	std::string avalue = a_itor->second;

	if (charsize ==1)
	{
		if (maxvaluesize >0)
			snprintf((char*)value, maxvaluesize -1, "%s", avalue.c_str());
		else
			sprintf((char*)value, "%s", avalue.c_str());
	}
	else
	{
#ifdef ZQ_OS_MSWIN
		if (maxvaluesize >0)
			snwprintf((wchar_t *)value, maxvaluesize/2 -1, L"%S", avalue.c_str());
		else
			swprintf((wchar_t *)value, L"%S",avalue.c_str());
#else

		if (maxvaluesize >0)
			swprintf((wchar_t *)value, maxvaluesize/2 -1, L"%S", avalue.c_str());
		else
		{
			int len = (avalue.length() + 1)/2;
			swprintf((wchar_t *)value, len + 1, L"%S",avalue.c_str());	
		}
#endif
	}

	return true;
}

void XMLPreferenceEx::setAttributeValue(const char *attrname, const char *value)
{
	if (attrname==NULL)
		return;

	std::string aname = attrname, avalue=(value==NULL)? "": value;

	pMapnode->second.attributes[aname] = avalue;
}

void XMLPreferenceEx::set(const char *key, const char *value)
{
	setAttributeValue(key,value);
}

void XMLPreferenceEx::remove(const char* key)
{
	if(key == NULL)
		return;
	std::string akey = key;

	element_attribute_map_t *attributes = &(pMapnode->second.attributes);
	element_attribute_map_t::iterator a_itor = attributes->find(akey);

	if(a_itor != attributes->end())
		attributes->erase(a_itor);
}

void XMLPreferenceEx::clean()
{
}

XMLPreferenceEx* XMLPreferenceEx::firstChild(const char* subPrefName)
{
	return findSubPreference(subPrefName);
}

XMLPreferenceDocumentEx* XMLPreferenceEx::doc()
{
	return pDoc;
}

void XMLPreferenceEx::free()
{
	delete(this);
	return;
}

bool XMLPreferenceEx::hasNextChild()
{
	if (++cursor == wkchild.end())
	{
		--cursor;
		return false;
	}
	--cursor;
	return true;
}

XMLPreferenceEx* XMLPreferenceEx::nextChild()
{
	return nextSubPreference();
}


XMLPreferenceEx* XMLPreferenceEx::findSubPreference(const void *subprefbasename, int charsize)
{
//	if (subprefbasename==NULL || pDoc==NULL )
// 		return NULL;
 
 	std::string searchfor;
 
	if (charsize ==1)
		searchfor = (NULL != (const char*) subprefbasename) ? (const char*) subprefbasename : "";
 	else
 	{
 		char buf[MAX_PATH];
		const wchar_t *wp=(NULL != (const wchar_t*) subprefbasename) ? (const wchar_t*) subprefbasename : L"";
 
 		for (int i =0; i< MAX_PATH && *wp; i++, wp++)
 			buf[i] = *wp & 0xff;
 
 		searchfor = buf;
 	}

	/*cursor =*wkchild.end();*/
	wkchild = pDoc->GetChildElements(pMapnode, (char*)searchfor.c_str());
	cursor = (wkchild.size()>0 ? wkchild.begin() : wkchild.end());

	
	return (cursor!=wkchild.end()) ? new XMLPreferenceEx(*cursor, pDoc) : NULL;
	
}

XMLPreferenceEx* XMLPreferenceEx::nextSubPreference()
{
	if (pDoc ==NULL)
		return NULL;
//	if (pMapnode ==NULL || cursor ==NULL)
//		return NULL;

	if (++cursor == wkchild.end())
		return NULL;

	return new XMLPreferenceEx(*cursor, pDoc);
}

XMLPreferenceEx* XMLPreferenceEx::getPreferenceRoot()
{
	if (pDoc ==NULL)
		return NULL;

	return pDoc->getRootPreference();
}


XMLPreferenceDocumentEx::XMLPreferenceDocumentEx(const char* filename)
			          :ExpatMap(), bValid(false)
{
	lasterrormsg="";

	if (filename != NULL)
		bValid = open(filename);
}

XMLPreferenceDocumentEx::~XMLPreferenceDocumentEx()
{
	clear();
}

void XMLPreferenceDocumentEx::clear()
{
	ExpatMap::clear();
	bValid = false;
	lasterrormsg=mFilename="";
}

/*
static void XMLPreferenceDocumentDestroyer(XMLPreferenceDocumentEx**pprefd)
{
	if (pprefd == NULL || *pprefd == NULL)
		return;

	XMLPreferenceDocumentEx* p = (XMLPreferenceDocumentEx*) *pprefd;

	delete p;
	*pprefd = NULL;
}
*/


bool XMLPreferenceDocumentEx::open(const char* filename)
{
	bValid = false;
	mFilename = "";

	try
	{
		ExpatMap::Parse(filename);
		mFilename = filename;
		bValid =true;
	}
	catch(const char*e)
	{
		lasterrormsg =e;
		throw XMLException(lasterrormsg);
	}
	catch(...)
	{
		lasterrormsg = std::string("Error: openning file ") +filename;
		throw XMLException(lasterrormsg);
	}

	return bValid;
}

bool XMLPreferenceDocumentEx::read(const void *szBuffer, const int nBufferLen, const int nFinal)
{
	if (bValid)
	{
		lasterrormsg = std::string("Fail to read: already loaded ") + mFilename;
		return false;
	}

	try
	{
		ExpatMap::Parse((const char *)szBuffer, nBufferLen, nFinal);
		bValid =true;
	}
	catch(const char*e)
	{
		lasterrormsg = mFilename.empty() ? std::string("line") +e : mFilename+e;
		throw XMLException(lasterrormsg);
	}
	catch(...)
	{
		lasterrormsg = "Fail to open read buffer";
		throw XMLException(lasterrormsg);
	}

	return bValid;
}

void XMLPreferenceDocumentEx::save(const char* filename) {
	if(!bValid || m_element_map.empty()) {
		throw XMLException("invalid XML file");
	} 
	if(!filename)  {
		throw XMLException("invalid destination file");
	}
	std::ofstream of(filename);
	if(!of.is_open()) {
		throw XMLException("failed to open file for write");
	}
	Export(of, m_element_map.begin(), true);	
	of.close();
}

bool XMLPreferenceDocumentEx::isValid()
{
	return bValid;
}

const char* XMLPreferenceDocumentEx::getLastErrorMessage()
{
	return lasterrormsg.c_str();
}


XMLPreferenceEx* XMLPreferenceDocumentEx::getRootPreference()
{
	return (!bValid ? NULL : new XMLPreferenceEx(m_element_map.begin(), this));
}

} // namespace ZQ
} // namespace common
/*
-------------------------------------------------------------
 Revision history since 2003/03/20:
 $Log: /ZQProjs/Common/XMLPreferenceEx.cpp $
// 
// 2     3/19/15 10:07a Hui.shao
// x64 compile warnings
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 16    10-10-15 17:57 Fei.huang
// 
// 15    10-10-15 17:44 Fei.huang
// + save modified xml file to disk
//     added new interface
// 
// 14    09-09-03 18:29 Xiaohui.chai
// make several function implement fit the interface
// 
// 13    09-05-12 18:21 Fei.huang
// 
// 12    08-09-03 16:23 Xiaohui.chai
// 
// 11    08-09-01 10:42 Xiaoming.li
// 
// 10    08-04-17 11:24 Guan.han
// 
// 9     08-04-01 17:32 Guan.han
// make firstChild and nextChild retuan all child nodes when firstChild
// function's parameter is empty or length is 0
// 
// 8     08-03-06 16:44 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 7     08-03-03 18:23 Yixin.tian
// merged changes for linux
// 
// 6     08-02-21 20:36 Guan.han
// 
// 5     08-02-21 12:39 Guan.han
// 
// 4     07-11-07 16:28 Guan.han
// 
// 3     07-06-12 10:23 Guan.han
// 
// 2     07-05-28 17:32 Guan.han
// 
// 1     07-01-23 17:19 Guan.han
// 
// 4     07-01-23 16:59 Guan.han
// 
// 3     07-01-23 16:31 Guan.han
// 
// 2     07-01-23 15:06 Guan.han
// 
// 3     07-01-19 16:49 Guan.han
// 
// 2     07-01-19 14:14 Guan.han
// 
// 1     07-01-18 16:06 Guan.han
 Revision 1.10  2003/03/26 00:03:35  shao
 changed the name column to display reg ident str on 2.4 gui

 Revision 1.9  2003/03/20 04:18:51  shao
 Copy to Infraworks VSS

-------------------------------------------------------------
*/
