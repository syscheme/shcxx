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
// ===========================================================================

#ifndef _ISA_FORMATSTR_H_
#define _ISA_FORMATSTR_H_

#pragma warning (disable:4786)

#include <string>
#include <vector>

enum FormatProc
{
	FP_NONE,
	FP_LOC2UTC
};

#define LOC2UTC "LOCAL2UTC"

std::string FormatString(const char* strFmtStr, ...);

std::string FormatString(const char* strFmtStr, const std::vector<std::string>& arrParament);

std::string PreFormat(const char* strFmtStr, FormatProc& fp);

#endif//_ISA_FORMATSTR_H_
