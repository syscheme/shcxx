// ============================================================================================
// Copyright (c) 2006, 2007 by
// syscheme, Shanghai
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of syscheme Possession, use, duplication or dissemination of the
// software and media is authorized only pursuant to a valid written license from ZQ Interactive,
// Inc.
//
// This software is furnished under a  license  and  may  be used and copied only in accordance
// with the terms of  such license and with the inclusion of the above copyright notice.  This
// software or any other copies thereof may not be provided or otherwise made available to any
// other person.  No title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and should not be
// construed as a commitment by syscheme
// --------------------------------------------------------------------------------------------
// Author: Xia Chen
// Desc  : Implement the NTFS file IO Render
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/MD5CheckSumUtil.h 1     10-11-12 15:56 Admin $Log:

#ifndef __ZQ_MD5CheckSumUtil_H__
#define __ZQ_MD5CheckSumUtil_H__


#include "ZQ_common_conf.h"
#include "md5.h"
#include <string.h>

namespace ZQ {
namespace common {

class ZQ_COMMON_API MD5ChecksumUtil;

class MD5ChecksumUtil
{
public:
	MD5ChecksumUtil();
	~MD5ChecksumUtil();

public:
	// calculate the check sum, return input buff checksum and add up the checksum for all the buffer
	void checksum(const char* buffer, unsigned int len);

	// get the checksum of all the buffer
	const char* lastChecksum();

	// reset the checksum, for next round calculating.
	void  reset();

public:	
	// get the checksum of a file
	static bool fileChecksum(const char* filename, char* checksum, int& len);
	
private:
  md5  _alg;
	char _finalchecksum[256];
};


}}

#endif // __ZQ_MD5CheckSumUtil_H__
