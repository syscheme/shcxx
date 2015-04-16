// ===========================================================================
// Copyright (c) 2004 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// Ident : $Id: URLString.h,v 1.2 2004/05/26 09:32:35 mwang Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define URL string and its codec
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/URLString.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
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

namespace ZQ {
namespace common {

//TODO: class URLString
class URLString
{
public:
	URLString();
	URLString(const char* url);
	URLString(const std::string& url);
	URLString(URL& url);

	~URLString();

	const char* getHost();
	const char* getScheme();
	const char* getFile();
	const char* getDir();
	const char* getFName();
	const char* getExt();
	int  getPort();

	void setFile(const char* file);
	void setFile(const std::string& file);
	void append(const char* path);
	
	std::string toString();
	int hashCode();

	URLString& operator = (const URLString& url);
	URLString& operator = (const char* URLString);

protected:
	int _port;
	std::string _scheme, _host, _file, _dir, _fname, _ext;
	std::string _urlString;

	void set(const URLString& url);
	void set(const char* url);
	void extractFName();
};

/// Decode an url parameter (ie "\%20" -> " ")
/// @param source string
/// @param dest destination buffer. If NULL source is used
char* ZQ_COMMON_API urlDecode(char *source, char *dest = NULL);

/// Encode an url parameter (ie " " -> "+")
/// @param source string
/// @param dest destination buffer. Do not overlap with source
/// @param size destination buffer size.
char* ZQ_COMMON_API urlEncode(const char *source,
							  char *dest, unsigned size);

/// Decode a string using base64 coding.
/// Destination size should be at least strlen(src)+1.
/// Destination will be a string, so is always terminated .
/// This function is deprecated, base64 can use binary source, not only string
/// use overloaded b64Decode.
/// @return string coded
/// @param src  source buffer
/// @param dest destination buffer. If NULL src is used
char* ZQ_COMMON_API b64Decode(char *src, char *dest = NULL);

/// Encode a string using base64 coding.
/// Destination size should be at least strlen(src)/4*3+1.
/// Destination is string terminated.
/// This function is deprecated, coded stream can contain terminator character
/// use overloaded b64Encode instead.
/// @return destination buffer
/// @param source source string
/// @param dest   destination octet buffer
/// @param size   destination buffer size
char* ZQ_COMMON_API b64Encode(const char *source,
							  char *dest,unsigned size);

/// Encode a octet stream using base64 coding.
/// Destination size should be at least (srcsize+2)/3*4+1.
/// Destination will be a string, so is always terminated 
/// (unless you pass dstsize == 0).
/// @return size of string written not counting terminator
/// @param src     source buffer
/// @param srcsize source buffer size
/// @param dst     destination buffer
/// @param dstsize destination buffer size
size_t ZQ_COMMON_API b64Encode(const unsigned char *src, size_t srcsize,
                               char *dst, size_t dstsize);

/// Decode a string using base64 coding.
/// Destination size should be at least strlen(src)/4*3.
/// Destination are not string terminated (It's just a octet stream).
/// @return number of octets written into destination buffer
/// @param src     source string
/// @param dst     destination octet buffer
/// @param dstsize destination buffer size
size_t ZQ_COMMON_API b64Decode(const char *src,
                               unsigned char *dst, size_t dstsize);

/// Encode a STL string using base64 coding into a STL string
/// @return base 64 encoded string
/// @param src source string
std::string ZQ_COMMON_API b64Encode(const std::string& src);

/// Decode a STL string using base64 coding into an STL String.
/// Destination size should be at least strlen(src)/4*3.
/// Destination are not string terminated (It's just a octet stream).
/// @return decoded string
/// @param src     source string
std::string ZQ_COMMON_API b64Decode(const std::string& src);

/// Encode a octet stream using base64 coding into a STL string
/// @return base 64 encoded string
/// @param src     source buffer
/// @param srcsize source buffer size
std::string ZQ_COMMON_API b64Encode(const unsigned char *src, size_t srcsize);

/// Decode a string using base64 coding.
/// Destination size should be at least strlen(src)/4*3.
/// Destination are not string terminated (It's just a octet stream).
/// @return number of octets written into destination buffer
/// @param src     source string
/// @param dst     destination octet buffer
/// @param dstsize destination buffer size
size_t ZQ_COMMON_API b64Decode(const std::string& src,
							   unsigned char *dst, size_t dstsize);

} // namespace common
} // namespace ZQ

#endif // __ZQ_COMMON_URLString_H__
