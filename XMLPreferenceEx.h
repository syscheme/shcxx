/*
-------------------------------------------------------------
 $Id: XMLPreferenceEx.h,v 1.9 2003/03/20 04:18:51 shao Exp $
 $Author: Hui.shao $
 $Revision: 2 $
 $Date: 7/24/13 6:13p $
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

#ifndef __XMLPreference_h__
#define __XMLPreference_h__

#include "ZQ_common_conf.h"

#ifdef ZQ_OS_MSWIN
#  pragma warning(disable:4503 4251 4786)
//  #  include <windows.h>
#  define snprintf _snprintf
#  define snwprintf _snwprintf
#endif

#include "ExpatMap.h"
#include "Exception.h"

#include <string>
#include <map>

namespace ZQ{
namespace common{

class ZQ_COMMON_API XMLPreferenceEx;
class ZQ_COMMON_API XMLPreferenceDocumentEx;
class ZQ_COMMON_API XMLException;


class XMLPreferenceEx
{
	friend class XMLPreferenceDocumentEx;

	typedef element_map_t::iterator itor_t;
protected:
	XMLPreferenceEx(itor_t mapnode, XMLPreferenceDocumentEx* doc);
	void GetStringRightAtChar(const ::std::string& contString,char vChar,::std::string &strTemp);

public:
	virtual ~XMLPreferenceEx();

public:
	// return the preference text. if the preference text length is larger than maxsize, only maxsize char returned.
	// name: buffer to receive text
	// maxsize: specify the buffer's size
	bool getPreferenceText(void *name, const int maxsize);

	bool getPreferenceName(void *name, bool basenameonly=false, const int maxsize = -1,  const int charsize=1);
	bool getAttributeValue(const void *attrname, void *value, int maxvaluesize = -1,  int charsize=1);
	std::map<std::string, std::string> getProperties();

	void setAttributeValue(const char *attrname, const char *value=NULL);

	XMLPreferenceEx* findSubPreference(const void *subprefbasename=NULL, int charsize=1);
	XMLPreferenceEx* nextSubPreference();
	XMLPreferenceEx* getPreferenceRoot();

	// name[in], specify the child's name you want to gain
	// num[in], if there are more than 1 child has the same name which you want to gain.
	// the num[in] specifies the order the child lies at
	// if the child not found, function will return NULL.
	XMLPreferenceEx* findChild(const char* name, const int num = 1);

	// return the name property's int value
	// name[in], the property name
	// value[out], stores the property's int value
	// if name property not found, return false, etherwise return true;
	bool getIntProp(const char* name, int& value);

	// return the name property's string value
	// name[in], the property name
	// buff[out], the buffer to receive the property value
	// buffSize[in], the buffer's size
	// if name property not found, return false, etherwise return true;
	bool getStrProp(const char* name, char* buff, const int buffSize);

private:

	itor_t pMapnode;
	child_itor_vector_t::iterator cursor;
	child_itor_vector_t wkchild;
	XMLPreferenceDocumentEx *pDoc;

public:
	/// returns the preference name
	/// @param name    the buf to hold the name string.
	/// @param size    the size of the buf, negative means unlimited
	/// @return        the string of the name
	const char* name(char* name, const int size = -1);

	/// test if a key is defined in the current preference
	/// @param key     the key to inquery
	/// @return        true if the specified key exists in this preferece
	bool has(const char *key);

	/// Returns specified preference
	/// @param key     the key to inquery
	/// @param value   to hold the value of the key
	/// @param fallback  the default value if the key doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	const char* get(const char* key, char* value, const char* fallback = "", int maxValueSize = -1);

	/// Adds or replaces preference
	/// @param key     the key to set
	/// @param value   the value of the key to set
	void set(const char *key, const char *value);

	/// Removes specified preference.
	/// @param key     the key to erase
	void remove(const char* key);

	/// Removes all preferences. 
	void clean();

	/// look for the first matched sub-preference
	/// it uses factory pattern, must call free() to release the instance
	///@param subPrefName   the sub preference name to look for, NULL for unspecified children
	///@return    the first matched IPreference instance, NULL if no matched sub-pref is found
	///@seealso nextChild(), free()
	XMLPreferenceEx* firstChild(const char* subPrefName =NULL);
	
	/// check if current node has next sub-preference
	///@return TRUE if has, FALSE else
	bool hasNextChild();

	/// return the next matched child of last query (firstChild())
	///@return    the next matched IPreference instance, NULL if meet the end
	XMLPreferenceEx* nextChild();

	/// return the associated preference document
	/// @return  the address of the associated preference document object
	XMLPreferenceDocumentEx* doc();

	/// IPreference uses factory pattern, must call free() to release the instance 
	virtual void free();
};


class XMLPreferenceDocumentEx : public ExpatMap
{
public:
	XMLPreferenceDocumentEx(const char* filename=NULL);
	virtual ~XMLPreferenceDocumentEx();

	// override ExpatMap
	virtual void clear();

public:
	// implementation of IPreferenceDocument
	virtual bool open(const char* filename);
	virtual bool read(const void *szBuffer, const int nBufferLen, const int nFinal = 0);
	void save(const char* saveas);
	virtual XMLPreferenceEx* getRootPreference();

	virtual bool isValid();
	virtual const char* getLastErrorMessage();

protected:
	std::string mFilename, lasterrormsg;
	bool bValid;
};

class XMLException : public IOException
{
public:
	XMLException(const std::string &what_arg) // throw()
	 : IOException(what_arg) {};
};

} // namespace common
} // namespace ZQ

#endif // __XMLPreference_h__


/*
-------------------------------------------------------------
 Revision history since 2003/03/20:
 $Log: /ZQProjs/Common/XMLPreferenceEx.h $
// 
// 2     7/24/13 6:13p Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 9     10-10-15 17:44 Fei.huang
// + save modified xml file to disk
//     added new interface
// 
// 8     08-04-01 17:32 Guan.han
// make firstChild and nextChild retuan all child nodes when firstChild
// function's parameter is empty or length is 0
// 
// 7     08-03-06 16:44 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 6     08-03-03 18:23 Yixin.tian
// merged changes for linux
// 
// 5     08-02-21 20:36 Guan.han
// 
// 4     08-02-21 12:39 Guan.han
// 
// 3     07-11-07 16:28 Guan.han
// 
// 2     07-05-28 17:32 Guan.han
// 
// 1     07-01-23 17:19 Guan.han
// 
// 5     07-01-23 16:59 Guan.han
// 
// 4     07-01-23 16:31 Guan.han
// 
// 3     07-01-23 15:06 Guan.han
// 
// 2     07-01-23 13:36 Guan.han
// 
// 3     07-01-19 16:49 Guan.han
// 
// 2     07-01-19 14:14 Guan.han
// 
// 1     07-01-18 16:06 Guan.han
 Revision 1.9  2003/03/20 04:18:51  shao
 Copy to Infraworks VSS

-------------------------------------------------------------
*/

