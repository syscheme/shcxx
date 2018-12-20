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
// Ident : $Id: IPreference.h,v 1.5 2004/07/21 11:58:12 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define common exceptions
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/IPreference.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 14    05-07-12 14:12 Bernie.zhao
// 
// 13    05-07-06 11:22 Bernie.zhao
// 
// 12    05-07-06 11:06 Bernie.zhao
// added guard
// 
// 11    04-11-17 15:05 Bernie.zhao
// 
// 10    04-10-10 13:59 Bernie.zhao
// added hasNextChild() method
// 
// 9     04-10-06 12:18 Bernie.zhao
// added removeChild()
// 
// 8     04-09-24 16:15 Bernie.zhao
// added toStream() function to dump node
// 
// 7     04-09-24 15:10 Bernie.zhao
// modified to support child pending
// 
// 6     9/07/04 10:52a Jie.zhang
// 
// 5     04-09-03 17:16 Bernie.zhao
// 
// 4     04-09-03 15:41 Bernie.zhao
// added addChild() & newElement()
// 
// 3     04-09-02 12:22 Bernie.zhao
// 
// 2     8/26/04 5:05p Jie.zhang
// Revision 1.5  2004/07/21 11:58:12  shao
// added comments
//
// Revision 1.4  2004/07/20 13:28:37  shao
// no message
//
// Revision 1.3  2004/07/07 02:20:40  shao
// adjust the interface definition
//
// ===========================================================================

#ifndef	__ZQ_COMMON_IPreference_H__
#define	__ZQ_COMMON_IPreference_H__

#include "ZQ_common_conf.h"
#include "Exception.h"

#include <string>
#include <exception>
#include "oaidl.h"

namespace ZQ {
namespace common {

class IPreference;
class PreferenceDoc;

// -----------------------------
// interface IPreference
// -----------------------------
/// used by an application to parse and store the preferences of its user.
/// it uses the factory pattern, free() must be called to release the rescource
class IPreference
{
public:
	/// returns the preference name
	/// @param name    the buf to hold the name string.
	/// @param size    the size of the buf, negative means unlimited
	/// @return        the string of the name
	virtual const char* name(char* name, const int size = -1) =0;

	/// test if a key is defined in the current preference
	/// @param key     the key to inquery
	/// @return        true if the specified key exists in this preferece
	virtual bool has(const char *key) =0;

	/// Returns specified preference
	/// @param key     the key to inquery
	/// @param value   to hold the value of the key
	/// @param fallback  the default value if the key doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	virtual const char* get(const char* key, char* value, const char* fallback = "", int maxValueSize = -1) =0;

	/// Returns specified preference, in UTF-8 format
	/// @param key     the key to inquery
	/// @param value   to hold the value of the key
	/// @param fallback  the default value if the key doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	virtual const char* getUtf8(const char* key, char* value, const char* fallback = "", int maxValueSize = -1) =0;
	
	/// Returns specified preference, in UNICODE format
	/// @param key     the key to inquery
	/// @param value   to hold the value of the key
	/// @param fallback  the default value if the key doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	virtual const wchar_t* getUnicode(const wchar_t* key, wchar_t* value, const wchar_t* fallback = L"", int maxValueSize = -1) =0;

	/// Adds or replaces preference
	/// @param key     the key to set
	/// @param value   the value of the key to set
	virtual void set(const char *key, const char *value) =0;

	/// Adds or replaces preference, in UTF-8 format
	/// @param key     the key to set
	/// @param value   the value of the key to set
	virtual void setUtf8(const char *key, const char * value) =0;

	/// Returns note value	
	/// @param value   to hold the text content of the node
	/// @param fallback  the default value if the note value doesn't exist,
	///                  it will be copied into the value buf
	/// @return        the string of the value
	virtual const char* gettext(char* value, const char* fallback = "", int maxValueSize = -1)=0;

	/// Adds or replaces text content of the node
	/// @param value   the text content of the node to set
	virtual void settext(const char* value)=0;

	/// Removes specified preference.
	/// @param key     the key to erase
	virtual void remove(const char* key) =0;

	/// Removes all preferences. 
	virtual void clean() =0;

	/// look for the first matched sub-preference
	/// it uses factory pattern, must call free() to release the instance
	///@param subPrefName   the sub preference name to look for, NULL for unspecified children
	///@return    the first matched IPreference instance, NULL if no matched sub-pref is found
	///@seealso nextChild(), free()
	virtual IPreference* firstChild(const char* subPrefName =NULL) =0;
	
	/// check if current node has next sub-preference
	///@return TRUE if has, FALSE else
	virtual bool hasNextChild() =0;

	/// return the next matched child of last query (firstChild())
	///@return    the next matched IPreference instance, NULL if meet the end
	virtual IPreference* nextChild() =0;

	/// append an IPreference object as the child
	/// @param child	the object to add
	/// @return	ture if add successfully
	virtual bool addNextChild( IPreference* child) =0;
	
	/// remove an IPreference object from the children list
	/// @param child	the object to remove
	/// @return ture if remove successfully
	virtual bool removeChild( IPreference* child) =0;

	/// output the node and all children to buffer
	/// @param[out] buff		the buff containing output stream
	/// @param[in&out] count	the number of buff, and return the actual buff size used
	virtual bool toStream( char* buff, unsigned long* count) =0;

	/// return the associated preference document
	/// @return  the address of the associated preference document object
	virtual PreferenceDoc* doc() =0;

	/// IPreference uses factory pattern, must call free() to release the instance 
	virtual void free() =0;
};

// -----------------------------
// class PreferenceDoc
// -----------------------------
class PreferenceDoc
{
public:

	/// return the root preference in the document
	/// @return  point to a new generated IPreference implementation
	virtual IPreference* root() =0;

	/// @return  true if this is a successfully opened XML document
	virtual bool isValid() =0;
//	virtual const char* lastErrorMessage() =0;

	/// open the specified preference document
	/// @param filename  the filename to open
	/// @param opentype	 if associated with file
	/// @return  true if open successfully
	virtual bool open(const char* filename, int opentype)=0;
	virtual bool read(const void *buffer, const int buflen, const int nFinal = 0) =0;

	/// create a new element to the document
	/// @param tagname	the name of the element tag
	/// @return	  a new IPreference node
	virtual IPreference* newElement(const char* tagname)=0;

	/// Saves the preferences to disk if a preferences file has been specified
	/// @param filename    save as the new file if provided
	virtual bool save(const char* filename=NULL) =0;
};

// -----------------------------
// class PrefGuard
// -----------------------------
class PrefGuard
{
public:
	PrefGuard(IPreference* pref = NULL):_pref(pref)
		{}

	~PrefGuard()
		{ if(_pref) _pref->free(); _pref = NULL;}

	IPreference* pref()
		{ return _pref;}

	void pref(IPreference* pref)
		{ if(_pref) _pref->free(); _pref=NULL; _pref=pref;}

	bool valid()
		{ return (_pref!=NULL); }
	
private:
	IPreference*	_pref;
private:
	///can't be copied!!!
	PrefGuard(const PrefGuard&);
	PrefGuard &operator=(const PrefGuard&);
};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COMMON_IPreference_H__


