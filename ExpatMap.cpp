/*
-------------------------------------------------------------
 $Id: ExpatMap.cpp,v 1.5 2003/03/20 04:18:51 shao Exp $
 $Author: Dejian.fei $
 $Revision: 3 $
 $Date: 1/11/16 4:48p $
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

// file   : ExpatMap.cpp
// author : Nick Shao
// created: 21 July 2002
// desc   : Store XML into a STL map
// ===========================================================================

#include "ExpatMap.h"
#include <fstream>

namespace ZQ {
namespace common {

// -----------------------------
// class ExpatMapNesting
// -----------------------------
ExpatMap::ExpatMap(const char *szFilename)
         :bLogicalClosed(true),pEMN(NULL) 
{
	Parse(szFilename);
}

ExpatMap::ExpatMap(const char *szBuffer, const int nBufferLen, const int nFinal)
         :bLogicalClosed(true),pEMN(NULL) 
{
	Parse(szBuffer, nBufferLen, nFinal);
}

ExpatMap::~ExpatMap()
{
	clear();
	if (pEMN != NULL)
		delete pEMN;
	pEMN = NULL;
}

void ExpatMap::clear()
{
	m_child_vector.clear();
	m_element_map.clear();
	error = "";
	bLogicalClosed =true;
}


void ExpatMap::Parse(const char *szBuffer, const int nBufferLen, const int nFinal)
{
	if (pEMN==NULL)
		pEMN = new ExpatMapNesting(this);

	// Parse the specified buffer
	// If there is an error, create an error string and throw
	if (!pEMN->XML_Parse(szBuffer, nBufferLen, nFinal))
	{
		char lineno[16];
		itoa(pEMN->XML_GetCurrentLineNumber(), lineno, 10);
		error = std::string("(") + lineno + ") " + pEMN->XML_GetErrorString();
		throw error.c_str();
	}
}

void ExpatMap::Parse(const char *szFilename)
{
	char szBuffer[8192];
	bool done = false;
	
	// Open the specified file
	std::ifstream from(szFilename);
	if (from.rdstate())
		throw "Could not open input file.";
	
	// Read to EOF
	while (!done)
	{
		// Read data from the input file; store the bytes read
		from.read(szBuffer, sizeof(szBuffer));
		
		done = from.eof();
//		if (from.rdstate()) 	// If another error is encountered
//			throw "Could not read data from input file"; 
		
		// Parse the data read
		Parse(szBuffer, (int) from.gcount(), done);
	}
}

const char* ExpatMap::GetContent(element_map_t::iterator parent_itor)
{
	// Grab the parent element data
	element_data_t *parent_data = &((*parent_itor).second);
	
	// Assign the new content value
	return parent_data->content.c_str();
}

void ExpatMap::SetContent(element_map_t::iterator parent_itor, const char *szContent)
{
	// Grab the parent element data
	element_data_t *parent_data = &((*parent_itor).second);
	
	// Assign the new content value
	parent_data->content = szContent;
}

child_itor_vector_t* ExpatMap::GetChildItorVector(element_map_t::const_iterator parent_itor)
{
	// Grab the parent element data
	const element_data_t *parent_data = &((*parent_itor).second);
	
	// If the element has no children, return
	if (parent_data->child_vector_index == -1)
		return NULL;
	
	// Return a pointer to the child iterator vector
	return &m_child_vector[parent_data->child_vector_index];
}

void ExpatMap::Insert(element_map_t::iterator parent_itor, const char *szElementName, element_attribute_map_t *amAttributes, const char *szContent)
{
	// Add the new element name to the parent path
	std::string path = (*parent_itor).first;
	path += "/";
	path += szElementName;
	
	// Allocate and initialize an element
	element_data_t data;
	data.child_vector_index = -1;			// XXX make sure we don't insert too many
	
	// If attributes were specified
	if (amAttributes)
	{
		// Populate the element's attribute list
		for (element_attribute_map_t::iterator itor = amAttributes->begin();
		     itor != amAttributes->end();
		     ++itor)
		{
			data.attributes[ (*itor).first ] = (*itor).second;
		}
	}
	
	// Store the element's content value
	data.content = szContent ? szContent : "";
	
	// Insert the new path/element into the map
	element_map_t::iterator 
		itor = m_element_map.insert(std::pair< std::string, element_data_t >(path, data));
		
	// Grab the parent element data
	element_data_t *parent_data = &((*parent_itor).second);
	
	// If a child vector index for the parent does not yet exist, create it,
	// and insert the child's itor.
	//
	if (parent_data->child_vector_index == -1)
	{
		// Allocate an child_itor_vector and store the current element itor
		// (i.e. the itor of the element we are about to push onto the
		// stack).
		//
		child_itor_vector_t child_itor_vector;
		child_itor_vector.insert(child_itor_vector.end(), itor);
		
		// Initialize the child vector index, and store the itor vector
		// at the end of the child vector
		//
		parent_data->child_vector_index = (int)m_child_vector.size();
		m_child_vector.insert(m_child_vector.end(), child_itor_vector);
	}
	
	// Otherwise, grab the itor vector from the child vector, and store
	// the current element itor.
	//
	else
	{
		// Allocate an child_itor_vector pointer, retrieve the child vector
		// for the element currently on the stack (i.e. the parent to 
		// the element we are about to push onto the stack), and store
		// the current element itor (i.e. the itor of the element we are
		// about to push onto the stack).
		//
		child_itor_vector_t *child_itor_vector;
		child_itor_vector = &(m_child_vector[ parent_data->child_vector_index ]);
		child_itor_vector->insert(child_itor_vector->end(), itor);
	}	
}

void ExpatMap::Insert(const char *szElementName, element_attribute_map_t *amAttributes, const char *szContent)
{
	// Create the new element name
	std::string path = "/";
	path += szElementName;
	
	// Allocate and initialize an element
	element_data_t data;
	data.child_vector_index = -1;			// XXX make sure we don't insert too many
	
	// If attributes were specified
	if (amAttributes)
	{
		// Populate the element's attribute list
		for (element_attribute_map_t::iterator itor = amAttributes->begin();
		itor != amAttributes->end();
		++itor)
		{
			data.attributes[ (*itor).first ] = (*itor).second;
		}
	}
	
	// Store the element's content value
	data.content = szContent ? szContent : "";
	
	// Insert the new path/element into the map
	m_element_map.insert(std::pair< std::string, element_data_t >(path, data)
		);	
}

const std::string ExpatMap::GetAttributeValue(element_map_t::iterator parent_itor, const char *szName)
{
	element_data_t *element_data = &(parent_itor->second);
	element_attribute_map_t *attributes = &(element_data->attributes);
	return (*attributes)[szName];
}

void ExpatMap::InsertAttribute(element_map_t::iterator parent_itor, const char *szName,
							   const char *szValue)
{
	// Grab the parent element data
	element_data_t *parent_data = &((*parent_itor).second);
	
	// Insert the new attribute
	parent_data->attributes[szName] = szValue;
}

bool ExpatMap::EraseAttribute(element_map_t::iterator parent_itor, const char *szName)
{
	element_data_t *parent_data = & ((*parent_itor).second);
	return parent_data->attributes.erase(szName) ? true : false;
}

// Display the specified element's information and recurse downward
void ExpatMap::Export(std::ostream &out, element_map_t::iterator element_map_itor, bool bIterateChildren)
{
	export_(out, element_map_itor, bIterateChildren);
}

void ExpatMap::export_(std::ostream &out, element_map_t::iterator element_map_itor, bool bIterateChildren, int depth)
{
	// Increment the depth
	++depth;

	// Helper variables
	std::string element_name = (*element_map_itor).first;
	element_data_t *element_data = &((*element_map_itor).second);

	// Determine the element's base name
	std::string base_name;
	int pos = (int)element_name.rfind("/", element_name.length()-1);
	if (pos > -1)
		base_name = element_name.substr(pos+1, element_name.length()-pos-1);
	else base_name = element_name;

	// Add tabs before the base tags
	for (int count=0; count<depth;  ++count)
		out << "\t";

	// Display the element path
	out << "<" << base_name;

	// Attribute map (e.g. name="value", etc)
	element_attribute_map_t *attributes = &(element_data->attributes);

	// Display the element attributes
	for (element_attribute_map_t::iterator attribute_itor = attributes->begin();
		 attribute_itor != attributes->end();
		 ++attribute_itor)
	{
		// Helper variables
		std::string attribute_name = (*attribute_itor).first;
		std::string attribute_value = (*attribute_itor).second;

		// The attribute map returns a pair<> when dereferenced
		out << " " << attribute_name << "=\"" << attribute_value << "\"";
	}

	// Grab a pointer to the child vector, if it exists
	child_itor_vector_t *child_itor_vector = GetChildItorVector(element_map_itor);

	// Close the element if there are no children to be displayed, and 
	// no data to be enclosed in the element
	if (!child_itor_vector && element_data->content.length() == 0)
	{
		out << "/>" << std::endl;
		--depth;
		return;
	}
	else out << ">";
			
	// Display the children
	if (child_itor_vector && bIterateChildren)
	{
		out << std::endl;

		// Iterate the children of the parent
		for (child_itor_vector_t::iterator __itor = child_itor_vector->begin();
			 __itor != child_itor_vector->end();
			++__itor)
		{
			element_map_t::iterator element_iterator = *__itor;
			export_(out, element_iterator, true, depth);
		}

		// Add tabs before the closing tag, since there are children, and like endl's.
		for (int count=0; count<depth; ++count)
			out << "\t";
	}
			
	// Show the element's value, if available
	if (element_data->content.length() > 0)
		out << element_data->content;

	// Close the element
	out << "</" << base_name << ">" << std::endl;

	// Decrement the depth
	--depth;
}

child_itor_vector_t ExpatMap::GetChildElements(element_map_t::iterator element_map_itor, char *szName)
{
	child_itor_vector_t ret;

	// Helper variables
	std::string element_name = (*element_map_itor).first;
//	element_data_t *element_data = &((*element_map_itor).second);

	// Grab a pointer to the child vector, if it exists
	child_itor_vector_t *child_itor_vector = GetChildItorVector(element_map_itor);

	// return empty if there are no children
	if (!child_itor_vector)
		return ret;
	
	if (szName == NULL || strlen(szName) == 0)
		return (*child_itor_vector);

	// Iterate the children of the parent
	for (child_itor_vector_t::iterator __itor = child_itor_vector->begin();
		 __itor != child_itor_vector->end();
		 ++__itor)
	{
		element_map_t::iterator element_iterator = *__itor;
		// Determine the element's base name
		std::string elem_name=element_iterator->first;
		std::string base_name;
		int pos = (int)elem_name.rfind("/", elem_name.length()-1);
		if (pos > -1)
			base_name = elem_name.substr(pos+1, elem_name.length()-pos-1);
		else base_name = element_name;
			 
		if (base_name.compare(szName)==0)
			ret.push_back(element_iterator);
	}
	return ret;
}

// -----------------------------
// class ExpatMapNesting
// -----------------------------
void ExpatMapNesting::startElement(const XML_Char* name, const XML_Char** atts)
{
	// The path string
	std::string path;
	
	// Initialize the path with the path to the parent
	if (!m_stack.empty()) path = m_stack.top().path;
	
	// Add the new element name to the path
	path += "/";
	path += name;
	
	// Allocate and initialize an element
	element_data_t data;
	data.content = "";
	data.child_vector_index = -1;			// XXX make sure we don't insert too many
	
	// Populate the element's attribute list
	for (int n = 0; atts[n]; n += 2)
	{
		data.attributes[ atts[n] ] = atts[ n + 1 ];
	}
	
	// Insert the current path/element into the map
	element_map_t::iterator itor = pExpatMap->m_element_map.insert(std::pair< std::string, element_data_t >(path, data));
	
	// If there is data on the stack, we must be a child.
	// Insert the itor in the parent vector.
	if (!m_stack.empty())
	{
		// Grab the parent element data
		element_data_t *parent_data = &((*m_stack.top().itor).second);
		
		// If a child vector index for the parent does not yet exist, create it,
		// and insert the child's itor.
		if (parent_data->child_vector_index == -1)
		{
			// Allocate an child_itor_vector and store the current element itor
			// (i.e. the itor of the element we are about to push onto the
			// stack).
			child_itor_vector_t child_itor_vector;
			child_itor_vector.insert(child_itor_vector.end(), itor);
			
			// Initialize the child vector index, and store the itor vector
			// at the end of the child vector
			parent_data->child_vector_index = (int)pExpatMap->m_child_vector.size();
			pExpatMap->m_child_vector.insert(pExpatMap->m_child_vector.end(), child_itor_vector);
		}
		// Otherwise, grab the itor vector from the child vector, and store
		// the current element itor.
		else
		{
			// Allocate an child_itor_vector pointer, retrieve the child vector
			// for the element currently on the stack (i.e. the parent to 
			// the element we are about to push onto the stack), and store
			// the current element itor (i.e. the itor of the element we are
			// about to push onto the stack).
			child_itor_vector_t *child_itor_vector;
			child_itor_vector = &(pExpatMap->m_child_vector[parent_data->child_vector_index]);
			child_itor_vector->insert(child_itor_vector->end(), itor);
		}
	}
	
	// Store the path and itor information on the stack
	stack_element_t stack_element;
	stack_element.itor = itor;
	stack_element.path = path;
	m_stack.push(stack_element);

	pExpatMap->bLogicalClosed=false;
}


void ExpatMapNesting::endElement(const XML_Char* name)
{
	m_stack.pop();

	if (m_stack.empty())
		pExpatMap->bLogicalClosed=true;
}

void ExpatMapNesting::charData(const XML_Char* strData, int len)
{
	int pos = 0;
	std::string local;
	
	// Append the data to the local string
	local.append(strData, len);
	
	// Remove extraneous characters
	while (1)
	{
		// Determine the position of the first extraneous char
		pos = (int)local.find_first_of("\n\t", 0);
		
		// Break if no extraneous char exists
		if (pos == -1)  break;
		
		// Remove the extraneous char
		local.erase(pos, 1);
	}
	
	// Return if the string length is 0
	if (local.length() == 0)  return;
	
	// Grab the current element iterator
	element_map_t::iterator itor = m_stack.top().itor;
	
	// Grab the current element data and store the new value string
	element_data_t *element_data = &((*itor).second);
	element_data->content += local;
}

} // namespace common
} // namespace ZQ

/*
-------------------------------------------------------------
 Revision history since 2003/03/20:
 $Log: /ZQProjs/Common/ExpatMap.cpp $
// 
// 3     1/11/16 4:48p Dejian.fei
// 
// 2     3/19/15 9:59a Hui.shao
// x64 compile warnings
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 7     08-09-01 10:43 Xiaoming.li
// 
// 6     08-08-05 17:02 Xiaohui.chai
// 
// 5     08-04-01 17:32 Guan.han
// make firstChild and nextChild retuan all child nodes when firstChild
// function's parameter is empty or length is 0
// 
// 4     08-03-03 16:50 Yixin.tian
// merged changes for linux
// 
// 3     07-11-08 13:52 Guan.han
// move delete pEMN from clear() to destructor
// 
// 2     07-11-08 11:46 Guan.han
// 
// 1     07-01-23 17:19 Guan.han
// 
// 2     07-01-23 16:20 Guan.han
// 
// 1     07-01-18 16:06 Guan.han
 Revision 1.5  2003/03/20 04:18:51  shao
 Copy to Infraworks VSS

-------------------------------------------------------------
*/

