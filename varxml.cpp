
#include "varxml.h"
#include <stdarg.h>


//#define USE_WINDOWS_DEBUG // To make the error and log messages go to VC++ debug output
#ifdef USE_WINDOWS_DEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


#include "base64.h"

// Version id
const char VARIANT_VER[] = "Variant 0.1.0";

int getVerbosity()
{
/*
code here
	*/
	return 0;
}

void XmlParser::log(int level, const char* fmt, ...)
{
	if (level <= getVerbosity())
	{
		va_list va;
		char buf[1024];
		va_start( va, fmt);
		_vsnprintf(buf,sizeof(buf)-1,fmt,va);
		buf[sizeof(buf)-1] = 0;
		//log(level, buf);
	}
}


void XmlParser::error(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	char buf[1024];
	_vsnprintf(buf,sizeof(buf)-1,fmt,va);
	buf[sizeof(buf)-1] = 0;
	//error(buf);
}


// Returns contents between <tag> and </tag>, updates offset to char after </tag>
std::string 
XmlParser::parseTag(const char* tag, std::string const& xml, int* offset)
{
	if (*offset >= int(xml.length())) return std::string();
	size_t istart = xml.find(tag, *offset);
	if (istart == std::string::npos) return std::string();
	istart += strlen(tag);
	std::string etag = "</";
	etag += tag + 1;
	size_t iend = xml.find(etag, istart);
	if (iend == std::string::npos) return std::string();
	
	*offset = int(iend + etag.length());
	return xml.substr(istart, iend-istart);
}


// Returns true if the tag is found and updates offset to the char after the tag
bool 
XmlParser::findTag(const char* tag, std::string const& xml, int* offset)
{
	if (*offset >= int(xml.length())) return false;
	size_t istart = xml.find(tag, *offset);
	if (istart == std::string::npos)
		return false;
	
	*offset = int(istart + strlen(tag));
	return true;
}


// Returns true if the tag is found at the specified offset (modulo any whitespace)
// and updates offset to the char after the tag
bool 
XmlParser::nextTagIs(const char* tag, std::string const& xml, int* offset)
{
	if (*offset >= int(xml.length())) return false;
	const char* cp = xml.c_str() + *offset;
	int nc = 0;
	while (*cp && isspace(*cp)) {
		++cp;
		++nc;
	}
	
	int len = int(strlen(tag));
	if  (*cp && (strncmp(cp, tag, len) == 0)) {
		*offset += nc + len;
		return true;
	}
	return false;
}

// Returns the next tag and updates offset to the char after the tag, or empty string
// if the next non-whitespace character is not '<'
std::string 
XmlParser::getNextTag(std::string const& xml, int* offset)
{
	if (*offset >= int(xml.length())) return std::string();
	
	size_t pos = *offset;
	const char* cp = xml.c_str() + pos;
	while (*cp && isspace(*cp)) {
		++cp;
		++pos;
	}
	
	if (*cp != '<') return std::string();
	
	std::string s;
	do {
		s += *cp;
		++pos;
	} while (*cp++ != '>' && *cp != 0);
	
	*offset = int(pos);
	return s;
}



// xml encodings (xml-encoded entities are preceded with '&')
static const char  AMP = '&';
static const char  rawEntity[] = { '<',   '>',   '&',    '\'',    '\"',    0 };
static const char* xmlEntity[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
static const int   xmlEntLen[] = { 3,     3,     4,      5,       5 };


// Replace xml-encoded entities with the raw text equivalents.

std::string 
XmlParser::xmlDecode(const std::string& encoded)
{
	std::string::size_type iAmp = encoded.find(AMP);
	if (iAmp == std::string::npos)
		return encoded;
	
	std::string decoded(encoded, 0, iAmp);
	std::string::size_type iSize = encoded.size();
	decoded.reserve(iSize);
	
	const char* ens = encoded.c_str();
	while (iAmp != iSize) {
		if (encoded[iAmp] == AMP && iAmp+1 < iSize) {
			int iEntity;
			for (iEntity=0; xmlEntity[iEntity] != 0; ++iEntity)
				//if (encoded.compare(iAmp+1, xmlEntLen[iEntity], xmlEntity[iEntity]) == 0)
				if (strncmp(ens+iAmp+1, xmlEntity[iEntity], xmlEntLen[iEntity]) == 0)
				{
					decoded += rawEntity[iEntity];
					iAmp += xmlEntLen[iEntity]+1;
					break;
				}
				if (xmlEntity[iEntity] == 0)    // unrecognized sequence
					decoded += encoded[iAmp++];
				
		} else {
			decoded += encoded[iAmp++];
		}
	}
    
	return decoded;
}


// Replace raw text with xml-encoded entities.

std::string 
XmlParser::xmlEncode(const std::string& raw)
{
	std::string::size_type iRep = raw.find_first_of(rawEntity);
	if (iRep == std::string::npos)
		return raw;
	
	std::string encoded(raw, 0, iRep);
	std::string::size_type iSize = raw.size();
	
	while (iRep != iSize) {
		int iEntity;
		for (iEntity=0; rawEntity[iEntity] != 0; ++iEntity)
			if (raw[iRep] == rawEntity[iEntity])
			{
				encoded += AMP;
				encoded += xmlEntity[iEntity];
				break;
			}
			if (rawEntity[iEntity] == 0)
				encoded += raw[iRep];
			++iRep;
	}
	return encoded;
}

