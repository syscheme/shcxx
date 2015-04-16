/*
-------------------------------------------------------------
 $Id: ExpatMap.h,v 1.5 2003/03/20 04:18:51 shao Exp $
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

// file   : ExpatMap.h
// author : Nick Shao
// created: 21 July 2002
// desc   : Store XML into a STL map
// ===========================================================================
#ifndef __EXPATMAP_H__
#define __EXPATMAP_H__

#include "ZQ_common_conf.h"

#ifdef ZQ_OS_MSWIN
#  pragma warning(disable:4503 4251 4786)
#endif

#include <assert.h>
#include <fstream>
#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <vector>

#include "expatxxx.h"

namespace ZQ {
namespace common {

class ExpatMapNesting;

// -----------------------------
// class ExpatMap
// -----------------------------
class ExpatMap
{
	friend class ExpatMapNesting;

  public:
	// Define the attribute map
	// @First field is the attribute name
	// @Second field is the attribute value
	  typedef std::map<std::string, std::string> element_attribute_map_t;
	
	// Element data consists of an element's attributes and text value.
	// E.g.  <a key="value">text<b/></a>
	// @Element name				: a
	// @Attribute key/value pair	: key/value
	// @Text						: text
	// @Child elements              : b
	typedef struct 
	{
		element_attribute_map_t attributes;		    // name=value pair
		std::string				content;			// text value/data enclosed in the element
		int						child_vector_index;	// index to the child vector of the child element iterator vector
	} element_data_t;
	
	// Define the element multimap
	// @First field is the element path
	// @Second field is the element data
	typedef std::multimap< std::string, element_data_t > element_map_t;
	
	// Define the child iterator vector
	// This is a "list" of iterators of children of an arbitrary element.
	typedef std::vector< element_map_t::iterator > child_itor_vector_t;
	
	// Define the child vector vector
	// This is a "list" of vectors of iterators of children of an arbitrary element.
	// This allows easily retrieval of an arbitrary element's children.
	typedef std::vector< child_itor_vector_t > child_vector_t;

	// The child vector contains vectors of iterators that designate elements stored
	// in element_map. It is used to iterator children of other elements stored in
	// element_map.
	// E.g.  <a> <b/> </a> ==>The element map contains a data segment for element <a>.
	// This data segment contains an index, child_vector_index.
	// To iterate children of element <a>, reference the child_vector with
	// child_vector_index. The child vector contains vectors of iterators that
	// designate elements stored in element_map, that are children of other elements
	// stored in element_map. (from above)
	// Iterate the resulting vector of iterators. The iterators designate elements
	// stored in element_map, ala, the children of element <a>.
	child_vector_t m_child_vector;
	
	// The map below provides information regarding the elements in the document
	element_map_t m_element_map;

	bool bLogicalClosed;

  public:

	// Constuctor & Deconstructor
	ExpatMap():pEMN(NULL){}
	ExpatMap(const char *szFilename);
	ExpatMap(const char *szBuffer, const int nBufferLen, const int nFinal = 0);
	virtual ~ExpatMap();

	// empty the map
	virtual void clear();

	// Parse the specified buffer
	void Parse(const char *szBuffer, const int nBufferLen, const int nFinal = 0);
	
	// Parse the specified file
	void Parse(const char *szFilename);
	
	// Get the content portion of an element
	const char* GetContent(element_map_t::iterator parent_itor);

	child_itor_vector_t GetChildElements(element_map_t::iterator element_map_itor, char *szName = NULL);
	
	// Set the content portion of an element
	void SetContent(element_map_t::iterator parent_itor, const char *szContent);

	// @Returns a pointer to a child iterator vector, NULL if no children exist.
	child_itor_vector_t* GetChildItorVector(element_map_t::const_iterator parent_itor);
	
	// Insert an element as a child to parent_itor, using the specified element name,
	// attributes and text. When parent_itor is not specified, the element is inserted
	// at the root
	void Insert(element_map_t::iterator parent_itor, const char *szElementName,
		element_attribute_map_t *amAttributes = NULL, const char *szText = NULL);
	void Insert(const char *szElementName, element_attribute_map_t *amAttributes = NULL,
		const char *szText = NULL);
	
	const std::string GetAttributeValue(element_map_t::iterator parent_itor, const char *szName);

	// Insert an attribute in the specified element
	void InsertAttribute(element_map_t::iterator parent_itor, const char *szName,
		const char *szValue);

	// Remove an attribute from the specified element, returns TRUE on success
	bool EraseAttribute(element_map_t::iterator parent_itor, const char *szName);

	// Export the STL map into an ostream, which could be cout
	void Export(std::ostream &out, element_map_t::iterator element_map_itor, bool bIterateChildren);

protected:
	ExpatMapNesting *pEMN;
	std::string error;
	
	//called by Export() 
	void export_(std::ostream &out, element_map_t::iterator element_map_itor, bool bIterateChildren, int depth=-1);

};

// Helper type definitions
typedef ExpatMap::element_attribute_map_t element_attribute_map_t;
typedef ExpatMap::element_data_t          element_data_t;
typedef ExpatMap::element_map_t           element_map_t;
typedef ExpatMap::child_itor_vector_t     child_itor_vector_t;
typedef ExpatMap::child_vector_t          child_vector_t;


// -----------------------------
// class ExpatMapNesting
// -----------------------------
class ExpatMapNesting
     : public ExpatBaseNestingEx
{
	friend class ExpatMap;

  private:
	ExpatMap *pExpatMap;

	ExpatMapNesting(ExpatMap *pem) {pExpatMap=pem;}

// overrideable callbacks
// ------------------------
	virtual void startElement(const XML_Char* name, const XML_Char** atts);
	virtual void endElement(const XML_Char*);
	virtual void charData(const XML_Char*, int len);

  private:
	
	// Define the stack used when creating the element map and child vectors
	// Element stack data definition
	typedef struct
	{
		element_map_t::iterator itor;
		std::string path;
	} stack_element_t;
	
	// Element stack definition
	typedef std::stack<stack_element_t> stack_t;
	
	// The stack provides scope when dealing with element handlers
	stack_t m_stack;

};

} // namespace common
} // namespace ZQ

#endif // __EXPATMAP_H__

/*
-------------------------------------------------------------
 Revision history since 2003/03/20:
 $Log: /ZQProjs/Common/ExpatMap.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 5     08-04-01 17:32 Guan.han
// make firstChild and nextChild retuan all child nodes when firstChild
// function's parameter is empty or length is 0
// 
// 4     08-03-06 16:45 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 3     08-03-03 16:50 Yixin.tian
// merged changes for linux
// 
// 2     07-11-02 11:24 Guan.han
// include "ZQ_common_conf.h" first
// 
// 1     07-01-23 17:19 Guan.han
// 
// 4     07-01-23 16:20 Guan.han
// 
// 3     07-01-23 15:52 Guan.han
// 
// 2     07-01-23 15:06 Guan.han
// 
// 1     07-01-18 16:06 Guan.han
 Revision 1.5  2003/03/20 04:18:51  shao
 Copy to Infraworks VSS

-------------------------------------------------------------
*/

