// ===========================================================================
// Copyright (c) 2009 by
// syscheme, Shanghai
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
// Ident : 
// Branch: 
// Author: 
// Desc  : 
//
// Revision History: 
// ===========================================================================

#ifndef ZQTS_DUMP_STATE_H
#define ZQTS_DUMP_STATE_H

#include "ZQ_common_conf.h"
#include <string>

class DumpState
{
public:
	///@ in strServiceName, the service name
	DumpState(const std::string& strServiceName);

	bool isDumpState();
	bool setDumpState(bool bDumpping);

	class DumpHepler
	{
	public:
		DumpHepler(DumpState& ds):_ds(ds)
		{
			_ds.setDumpState(true);
		}

		~DumpHepler()
		{
			_ds.setDumpState(false);
		}
	private:
		DumpState& _ds;
	};
    static bool isDumping(const char* serviceName);
protected:
	std::string getEventName();

	bool createDumpState();
	bool clearDumpState();

protected:
	std::string		_strServiceName;
	HANDLE			_hEvent;
};


#endif
