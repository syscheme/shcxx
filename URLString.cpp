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
// Ident : $Id: URLString.cpp,v 1.2 2004/05/26 09:32:35 mwang Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl URL string and its codec
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/URLString.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 2     08-03-06 16:37 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// Revision 1.2  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.1  2004/04/30 02:05:07  shao
// codec impl-ed
//
// ===========================================================================

#ifndef	__ZQ_COMMON_URLString_H__
#define	__ZQ_COMMON_URLString_H__

#include "ZQ_common_conf.h"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <cerrno>
#include <iostream>
#ifdef	ZQ_OS_MSWIN
#include <io.h>
#endif
#include <cctype>

namespace ZQ {
namespace common {

static const unsigned char alphabet[65] = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* b64Encode(const char *source, char *dest, unsigned limit)
{
	b64Encode((const unsigned char*)source,strlen(source),
			dest,limit);
	return dest;
}

char* b64Decode(char *source, char *dest)
{
	size_t srcsize = strlen(source);
	char* dst = dest?dest:source;
	size_t dstsize = 
		b64Decode(source,(unsigned char*)dst,srcsize+1);
	dst[dstsize] = 0;
	return dst;
}

size_t b64Encode(const unsigned char *src, size_t srcsize,
		               char *dst, size_t dstsize)
{
	if (!dstsize) return 0;
	
	char* pdst = dst;
	unsigned bits;
	
	while(srcsize >= 3 && dstsize > 4)
	{
		bits = (((unsigned)src[0])<<16) | (((unsigned)src[1])<<8) 
			| ((unsigned)src[2]);
		src += 3;
		srcsize -= 3;
		*(pdst++) = alphabet[bits >> 18];
	    	*(pdst++) = alphabet[(bits >> 12) & 0x3f];
	    	*(pdst++) = alphabet[(bits >> 6) & 0x3f];
	    	*(pdst++) = alphabet[bits & 0x3f];
		dstsize -= 4;
	}
	if (srcsize && dstsize > 4) 
	{
		bits = ((unsigned)src[0])<<16;
		*(pdst++) = alphabet[bits >> 18];
		if (srcsize == 1)
		{
			*(pdst++) = alphabet[(bits >> 12) & 0x3f];
	    		*(pdst++) = '=';
		}
		else
		{
			bits |= ((unsigned)src[1])<<8;
			*(pdst++) = alphabet[(bits >> 12) & 0x3f];
	    		*(pdst++) = alphabet[(bits >> 6) & 0x3f];
		}
	    	*(pdst++) = '=';
	}
	*pdst = 0;
	return pdst-dst;
}

size_t b64Decode(const char *src, unsigned char *dst, size_t dstsize)
{
        char decoder[256];
    	int i, bits, c;

	unsigned char *pdst = dst;
	
	for (i = 0; i < 256; ++i)
		decoder[i] = 64;
    	for (i = 0; i < 64 ; ++i) 
		decoder[alphabet[i]] = i;

    	bits = 1;

	while(*src)
	{ 
		c = (unsigned char)(*(src++));
		if (c == '=')
		{
			if (bits & 0x40000)
			{
				if (dstsize < 2) break;
				*(pdst++) = (bits >> 10);
				*(pdst++) = (bits >> 2) & 0xff;
				break;
			}
			if (bits & 0x1000 && dstsize)
				*(pdst++) = (bits >> 4);
			break;
		}
		// skip invalid chars
		if (decoder[c] == 64)
			continue;
		bits = (bits << 6) + decoder[c];
		if (bits & 0x1000000) 
		{
			if (dstsize < 3) break;
			*(pdst++) = (bits >> 16);
			*(pdst++) = (bits >> 8) & 0xff;
			*(pdst++) = (bits & 0xff);
		    	bits = 1;
			dstsize -= 3;
		} 
	}
	return pdst-dst;
}

char* urlDecode(char *source, char *dest)
{
	char *ret;
	char hex[3];

	if(!dest)
		dest = source;
	else
		*dest = 0;

	ret = dest;

	if(!source)
		return dest;

	while(*source)
	{
		switch(*source)
		{
		case '+':
			*(dest++) = ' ';
			break;
		case '%':
			// NOTE: wrong input can finish with "...%" giving
			// buffer overflow, cut string here
			if(source[1])
			{
				hex[0] = source[1];
				++source;
				if(source[1])
				{
					hex[1] = source[1];
					++source;
				}
				else
					hex[1] = 0;
			}
			else
				hex[0] = hex[1] = 0;	
			hex[2] = 0;
			*(dest++) = (char)strtol(hex, NULL, 16);
			break;
		default:
			*(dest++) = *source;
		}
		++source;
	}
	*dest = 0;
	return ret;
}	

char* urlEncode(const char *source, char *dest, unsigned max)	
{
	static const char *hex = "0123456789abcdef";
	unsigned len = 0;
	unsigned char ch;
	char *ret = dest;

	*dest = 0;
	if(!source)
		return dest;

	while(len < max - 4 && *source)
	{
		ch = (unsigned char)*source;
		if(*source == ' ')
			*(dest++) = '+';
		else if(isalnum(*source) || strchr("/.-", *source))
			*(dest++) = *source;
		else
		{
			*(dest++) = '%';
			// char in C++ can be more than 8bit
			*(dest++) = hex[(ch >> 4)&0xF];
			*(dest++) = hex[ch % 16];
		}	
		++source;
	}
	*dest = 0;
	return ret;
}

/// Encode a STL string using base64 coding into a STL string
std::string b64Encode(const std::string& src)
{
  size_t limit = (src.length()+2)/3*4+1;  // size + null must be included
  char* buffer = new char[limit];

  unsigned size = b64Encode((const unsigned char *)src.c_str(), src.length(), buffer, limit);
  buffer[size] = '\0';

  std::string final = string(buffer);
  delete buffer;
  return final;
}

/// Decode a STL string using base64 coding into an STL String.
std::string b64Decode(const std::string& src)
{
  size_t limit = src.length()/4*3;
  unsigned char* buffer = new unsigned char[limit+1];

  unsigned size = b64Decode(src.c_str(), buffer, limit);
  buffer[size] = '\0';

  std::string final = string((char*)buffer);
  delete buffer;
  return final;
}

/// Encode a octet stream using base64 coding into a STL string
std::string b64Encode(const unsigned char *src, size_t srcsize)
{
  size_t limit = (srcsize+2)/3*4+1;
  char* buffer = new char[limit];

  unsigned size = b64Encode(src, srcsize, buffer, limit);
  buffer[size] = '\0';

  std::string final = string(buffer);
  delete buffer;
  return final;
}

/// Decode an STL string encoded using base64.
size_t b64Decode(const std::string& src, unsigned char *dst, size_t dstsize)
{
  return b64Decode(src.c_str(), dst, dstsize);
}

} // namespace common
} // namespace ZQ
