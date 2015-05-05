//xml.h
//daniel wang

#ifndef _D_LIB_XML_H_
#define _D_LIB_XML_H_

#include <string>

//! Utilities for XML parsing, encoding, and decoding and message handlers.
class XmlParser 
{
public:
    // hokey xml parsing
    //! Returns contents between <tag> and </tag>, updates offset to char after </tag>
    static std::string parseTag(const char* tag, std::string const& xml, int* offset);
	
    //! Returns true if the tag is found and updates offset to the char after the tag
    static bool findTag(const char* tag, std::string const& xml, int* offset);
	
    //! Returns the next tag and updates offset to the char after the tag, or empty string
    //! if the next non-whitespace character is not '<'
    static std::string getNextTag(std::string const& xml, int* offset);
	
    //! Returns true if the tag is found at the specified offset (modulo any whitespace)
    //! and updates offset to the char after the tag
    static bool nextTagIs(const char* tag, std::string const& xml, int* offset);
	
	
    //! Convert raw text to encoded xml.
    static std::string xmlEncode(const std::string& raw);
	
    //! Convert encoded xml to raw text
    static std::string xmlDecode(const std::string& encoded);
	
	
    //! Dump messages somewhere
    static void log(int level, const char* fmt, ...);
	
    //! Dump error messages somewhere
    static void error(const char* fmt, ...);
	
};

#endif//_D_LIB_XML_H_
