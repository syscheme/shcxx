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

#include "DumpState.h"
#include <algorithm>

bool DumpState::createDumpState()
{
	// check if the dumpping state event exist
	std::string strEventName = getEventName();
	_hEvent = CreateEventA(NULL, NULL, FALSE, strEventName.c_str());
	if (_hEvent == INVALID_HANDLE_VALUE || !_hEvent)
	{
		int nErrCode = GetLastError();

		if (nErrCode != ERROR_ALREADY_EXISTS)
			return false;
	}

	return true;
}

bool DumpState::clearDumpState()
{
	// only support to clearDumpState for local process createDumpState called
	if (_hEvent == INVALID_HANDLE_VALUE || !_hEvent)
		return false;

	CloseHandle(_hEvent);
	_hEvent = NULL;

	return true;
}

bool DumpState::setDumpState(bool bDumpping)
{
	if (bDumpping)
		return createDumpState();

	return clearDumpState();
}

bool DumpState::isDumpState()
{
    return isDumping(_strServiceName.c_str());
}

static std::string buildDumpStateEventName(const std::string& serviceName)
{
	std::string strEventName = serviceName;
	std::transform(strEventName.begin(), strEventName.end(), strEventName.begin(), (int(*)(int)) tolower);;
    return (strEventName + "_dumpping_state");
}

std::string DumpState::getEventName()
{
    return buildDumpStateEventName(_strServiceName);
}

DumpState::DumpState(const std::string& strServiceName)
{
	_hEvent = 0;
	_strServiceName = strServiceName;
}

bool DumpState::isDumping(const char* serviceName)
{
    if(serviceName != NULL && serviceName[0] != '\0')
    {
        // check if the dumpping state event exist
        std::string strEventName = buildDumpStateEventName(serviceName);

        HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, NULL, strEventName.c_str());
        if (hEvent == INVALID_HANDLE_VALUE || !hEvent)
            return false;

		CloseHandle(hEvent);
        return true;
    }

	return false;
}
