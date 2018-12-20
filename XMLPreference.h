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
// Ident : $Id: XMLPreference.h,v 1.7 2004/07/21 11:58:12 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl XML preference
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/XMLPreference.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 13    04-11-17 15:15 Bernie.zhao
// detailed open() comment
// 
// 12    04-11-17 15:08 Bernie.zhao
// modified open() method
// 
// 11    04-10-10 13:59 Bernie.zhao
// added hasNextChild() method
// 
// 10    04-10-06 12:14 Bernie.zhao
// 
// 9     04-09-24 16:15 Bernie.zhao
// added toStream() function to dump node
// 
// 8     04-09-24 15:10 Bernie.zhao
// modified to support child pending
// 
// 7     04-09-03 17:16 Bernie.zhao
// 
// 6     04-09-03 15:41 Bernie.zhao
// 
// 5     9/02/04 4:20p Jie.zhang
// 
// 4     8/26/04 5:05p Jie.zhang
// Revision 1.7  2004/07/21 11:58:12  shao
// added comments
//
// Revision 1.6  2004/07/15 07:18:03  jshen
// MsXm12.h reincluded
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

#ifndef	__ZQ_COMMON_XMLPrefernce_H__
#define	__ZQ_COMMON_XMLPrefernce_H__

#include "ZQ_common_conf.h"

#include "IPreference.h"
#include "Exception.h"
#include <string>
#include <deque>
#include <MsXml2.h>// modified the order of MsXml2.h for vc6 

#define XMLDOC_READONLY	0
#define XMLDOC_CREATE	1
#define XMLDOC_DUMMY	2

namespace ZQ {
namespace common {

class ZQ_COMMON_API XMLPreference;
class ZQ_COMMON_API XMLPrefDoc;
class ZQ_COMMON_API ComInitializer;


// -----------------------------
// class XMLPreference
// -----------------------------
/// XMLPreference implements IPreference to parse xml-base configuration
/// the root node is generated from XMLPrefDoc
/// This class is using class factory pattern so that the user must call
/// the free() method to release the instance
class XMLPreference : public IPreference
{
	friend class XMLPrefDoc;

protected:

	/// protected constructor, this class does not allow public constructor
	/// @param document the XMLPrefDoc object that opens the xml file
	/// @param pElement point to the MS XML element instance
	XMLPreference(XMLPrefDoc& document, IXMLDOMElement *pElement);

	/// destructor
	virtual ~XMLPreference();

public:

	// implement IPreference interface

	/// returns the preference name
	/// @param name    the buf to hold the name string.
	///                it will be the tag name in XML
	/// @param size    the size of the buf, negative means unlimited
	/// @return        the string of the name
	virtual const char* name(char* name, const int size = -1);

	/// test if a key is defined in the current preference
	/// @param key     the key to inquery
	/// @return        true if the specified key exists in this preferece
	virtual bool has(const char *key);

	/// Returns specified preference
	/// @param key     the key to inquery
	/// @param value   to hold the value of the key
	/// @param fallback  the default value if the key doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	virtual const char* get(const char* key, char* value, const char* fallback = "", int maxValueSize = -1);

	/// Returns specified preference, in UTF-8 format
	/// @param key     the key to inquery
	/// @param value   to hold the value of the key
	/// @param fallback  the default value if the key doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	virtual const char* getUtf8(const char* key, char* value, const char* fallback = "", int maxValueSize = -1);

	/// Returns specified preference, in UNICODE format
	/// @param key     the key to inquery
	/// @param value   to hold the value of the key
	/// @param fallback  the default value if the key doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	virtual const wchar_t* getUnicode(const wchar_t* key, wchar_t* value, const wchar_t* fallback = L"", int maxValueSize = -1);

	/// Returns note value	
	/// @param value   to hold the text content of the node
	/// @param fallback  the default value if the note value doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	virtual const char* gettext(char* value, const char* fallback = "", int maxValueSize = -1);

	/// Adds or replaces text content of the node
	/// @param value   the text content of the node to set
	virtual void settext(const char* value);

	/// Adds or replaces preference
	/// @param key     the key to set
	/// @param value   the value of the key to set
	virtual void set(const char *key, const char *value);

	/// Adds or replaces preference, in UTF-8 format
	/// @param key     the key to set
	/// @param value   the value of the key to set
	virtual void setUtf8(const char *key, const char *value);

	/// Removes specified preference.
	/// @param key     the key to erase
	virtual void remove(const char* key);

	/// Removes all preferences. 
	virtual void clean();

	/// look for the first matched sub-preference
	/// it uses factory pattern, must call free() to release the instance
	///@param subPrefName   the sub preference name to look for, NULL for unspecified children
	///@return    the first matched IPreference instance, NULL if no matched sub-pref is found
	///@seealso nextChild(), free()
	virtual IPreference* firstChild(const char* subPrefName =NULL);
	
	/// check if current node has next sub-preference
	///@return TRUE if has, FALSE else
	virtual bool hasNextChild();

	/// return the next matched child of last query (firstChild())
	///@return    the next matched IPreference instance, NULL if meet the end
	virtual IPreference* nextChild();

	/// append an IPreference object as the child
	/// @param child	the object to add
	/// @return	ture if add successfully
	virtual bool addNextChild( IPreference* child);
	
	/// remove an IPreference object from the children list
	/// @param child	the object to remove
	/// @return ture if remove successfully
	virtual bool removeChild( IPreference* child);

	/// output the node and all children to buffer
	/// @param[out] buff		the buff containing output stream
	/// @param[in&out] count	the number of buff, and return the actual buff size used
	virtual bool toStream(char* buff, unsigned long* count);

	/// return the associated preference document
	/// @return  the address of the associated preference document object
	virtual PreferenceDoc* doc();

	/// IPreference uses factory pattern, must call free() to release the instance 
	virtual void free();

protected:

	XMLPrefDoc& _doc;
	IXMLDOMElement* _elem;

	typedef std::deque < XMLPreference* > search_result_t;
	search_result_t _sres;
};

// -----------------------------
// class XMLPrefDoc
// -----------------------------
/// XMLPrefDoc impl the file operation for an XML-based preference document
class XMLPrefDoc : public PreferenceDoc
{
public:

	/// constructor
	/// @param _init    to make sure the MS COM evironment has been initialized
	///                 before creating XMLPrefDoc object
	XMLPrefDoc(const ComInitializer& _init);

	/// destructor
	virtual ~XMLPrefDoc();

	/// set new root for doc
	void set_root(IPreference* root);

	/// return the root preference in the document
	/// @return  point to a new generated IPreference implementatin
	virtual IPreference* root();

	/// @return  true if this is a successfully openned XML document
	virtual bool isValid() { return (_doc !=NULL); }

	virtual const char* lastErrorMessage() { return NULL; }

	/// open the specified XML document
	/// @param filename  the filename to open
	/// @param opentype	 the way file used to open with
	/// @param opentype	 XMLDOC_READONLY if xml associated with a file, XMLDOC_CREATE if a new file should be created, XMLDOC_DUMMY if no file should be associated
	/// @remarks	if opentype is XMLDOC_CREATE, unless you call save() method, the file will be empty; if XMLDOC_DUMMY, unless you call save() method, no file will exist.
	/// @return  true if open successfully
	virtual bool open(const char* filename, int opentype=XMLDOC_READONLY);

	virtual bool read(const void *buffer, const int buflen, const int nFinal = 0);

	/// create a new element to the document
	/// @param tagname	the name of the element tag
	/// @return	  a new IPreference node
	virtual IPreference* newElement(const char* tagname);

	/// close the current XML document
	void close();

	/// Saves the preferences to disk if a preferences file has been specified
	/// @param filename    save as the new file if provided
	virtual bool save(const char* filename=NULL);

protected:

	const ComInitializer& _init;

	IXMLDOMDocument* _doc;
	std::string _filename;
};

class ComInitializer
{
public:
 ComInitializer() : m_hr(CoInitialize(NULL)) { }
 ~ComInitializer()
	{ if (SUCCEEDED(m_hr)) CoUninitialize(); }
 operator HRESULT() const { return S_OK; }
 HRESULT m_hr;
};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COMMON_XMLPrefernce_H__