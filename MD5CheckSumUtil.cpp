// ============================================================================================
// Copyright (c) 2006, 2007 by
// syscheme, Shanghai,,
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
// $Header: /ZQProjs/Common/MD5CheckSumUtil.cpp 2     12/15/11 12:49p Hui.shao $Log:


#include <stdio.h>
#include "MD5CheckSumUtil.h"
#include <stdlib.h>

namespace ZQ {
namespace common {

MD5ChecksumUtil::MD5ChecksumUtil()
{
	reset();
}

MD5ChecksumUtil::~MD5ChecksumUtil()
{

}

void MD5ChecksumUtil::checksum(const char* buffer,unsigned int len)
{	
	_alg.Update((unsigned char*)buffer, len);	
}
const char*  MD5ChecksumUtil::lastChecksum()
{
	char *str;

	_alg.Finalize();
	str = md5::PrintMD5(_alg.Digest());
    strcpy(_finalchecksum,str);
	free(str);

	return _finalchecksum;
}

void  MD5ChecksumUtil::reset()
{
	_alg.Init();
	memset(_finalchecksum, 0x00, 256*sizeof(char));
}

bool MD5ChecksumUtil::fileChecksum(const char* filename, char* checksum, int& len)
{
	if(NULL == filename || NULL == checksum || len <= 32)
	{
		return false;
	}

	MD5ChecksumUtil context;

	int bufLen = 0;
	char buffer[64*1024];

	FILE* file = fopen(filename, "rb");
	if(file != NULL)
	{
		while((bufLen = (int)fread (buffer, 1, 64*1024, file)) != 0)
		{
			context.checksum(buffer, bufLen);
		}
		const char* outputcs = context.lastChecksum();
		len = strlen(outputcs);
		strcpy(checksum, outputcs);
		
		fclose(file);

		return true;
	}
	return false;
}

}}
