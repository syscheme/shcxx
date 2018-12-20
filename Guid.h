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
// Ident : $Id: Guid.h,v 1.13 2004/07/22 06:16:42 shao Exp $
// Branch: $Name:  $
// Author: Hui.Shao
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Guid.h $
// 
// 5     10/16/14 4:10p Ketao.zhang
// 
// 4     10/14/14 11:17a Hui.shao
// decode the compat id string
// 
// 3     10-12-17 14:56 Fei.huang
// + use data structure conforms to Microsoft impl
// + convert uuid_t to MS impl back and forth
// 
// 2     10-12-16 17:04 Fei.huang
// + merge impl of win/linux
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 9     08-12-09 14:37 Yixin.tian
// 
// 8     08-03-06 16:14 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 7     08-03-03 17:03 Yixin.tian
// merged changes for linux
// 
// 2     04-08-20 14:16 Kaliven.lee
// Revision 1.13  2004/07/22 06:16:42  shao
// comment
//
// Revision 1.12  2004/07/20 13:28:15  shao
// no message
//
// Revision 1.11  2004/07/12 09:08:03  shao
// no message
//
// Revision 1.10  2004/07/09 11:21:14  shao
// adjust interface
//
// Revision 1.9  2004/06/22 06:19:08  mwang
// add comment
//
// Revision 1.8  2004/06/16 14:21:18  mwang
// add const restrict
//
// Revision 1.7  2004/06/16 06:06:17  shao
// renamed UUID to Guid
//
// Revision 1.6  2004/06/16 06:01:51  shao
// Guid
// ===========================================================================
#ifndef __ZQ_COMMON_GUID_H__
#define __ZQ_COMMON_GUID_H__

#include "ZQ_common_conf.h"
#include <string>
#ifdef ZQ_OS_LINUX
#include <uuid/uuid.h>
#endif

namespace ZQ{
namespace common{

class ZQ_COMMON_API Guid;

// -----------------------------
// class Guid
// -----------------------------
class Guid {

public:

#ifdef ZQ_OS_MSWIN
	typedef GUID UUID;
#else
    typedef struct {
		uint32 Data1;
		uint16 Data2;
		uint16 Data3;
		uint8  Data4[8];	
		uuid_t data;
	} UUID;
#endif

	/// constructor, from a GUID string
	Guid(const char* uuidstr = NULL);

	/// constructor, from binary data fields
	Guid(unsigned long Data1, unsigned short Data2, unsigned short Data3, unsigned char Data4[8]);

	/// copier
	Guid(const Guid &rhs);

	Guid(const UUID &rhs);

	// type convert to UUID
	operator UUID();

	// set the Guid to equal to the src
	Guid& operator=(const UUID &src);

	/// set the Guid from a uuid string
	Guid& operator=(const char *str);

	/// converts a UUID to a display string
	int toString(char* buf, int bufSize) const;

	/// convert this UUDI to a compact and case-sensitive id string encoded with encodeCompactIdstr()
	///@note the result normally needs a space up to 26 characters for case-insensitive and 23 for case-sensitive
	int toCompactIdstr(char* buf, int bufSize, bool bCaseSensitve=true) const;

	/// creates a new UUID
	/// @param nil     true if to make a nil guid
	bool create(bool nil = false);

	/// determine whether this is a nil-valued UUID
	bool isNil() const;

	bool operator==(const Guid &rhs) const;

	bool operator >(const Guid &rhs) const;

	bool operator <(const Guid &rhs) const;

	bool operator!=(const Guid &rhs) const;

	/// compare two UUIDs and determine their order. The returned value gives the order
	int compare(const Guid &rhs) const;

	/// static util func to generate an id string built up with digits and letters
	static int encodeCompactIdstr(uint64 id, char* buf, const int bufSize, bool bCaseSensitve =true);

	/// static util func to convert an id string to uint64
	static uint64 decoderCompactIdstr(char* str, bool bCaseSensitve);

private:

#ifdef ZQ_OS_LINUX
	void uuid_to_guid();
	void guid_to_uuid();
#endif

	UUID _uuid;
};

}
}//endof namespace
#endif // __ZQ_COMMON_GUID_H__

// vim: ts=4 sw=4 nu bg=dark

