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
// Ident : $Id: Guid.h,v 1.13 2004/07/22 06:16:42 shao Exp $
// Branch: $Name:  $
// Author: Hui.Shao
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Guid.cpp $
// 
// 5     10/14/14 11:17a Hui.shao
// decode the compat id string
// 
// 4     10-12-28 12:44 Fei.huang
// * fix: size of underlaying uuid structure differ in linux
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
// 7     09-07-21 11:17 Fei.huang
// * fix create() behave like Windows version 
// 
// 6     08-03-06 16:14 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 5     08-03-03 17:03 Yixin.tian
// merged changes for linux
// 
// 4     06-07-13 13:44 Hui.shao
// 
// 3     06-04-26 14:22 Cary.xiao
// 
// 2     06-04-05 18:36 Cary.xiao
// 
// 1     11/23/05 3:26p Hui.shao
// moved impl to Guid.cpp, enabled dll export
// ===========================================================================
#include "Guid.h"
#ifdef ZQ_OS_MSWIN
#pragma comment(lib,"Rpcrt4.lib")
#endif

namespace ZQ{
namespace common{

// -----------------------------
// class Guid
// -----------------------------
/// constructor, from a GUID string
Guid::Guid(const char* uuidstr)
{
	if (uuidstr)
	{
#ifdef ZQ_OS_MSWIN
		if (UuidFromStringA((unsigned char*)uuidstr, &_uuid)==RPC_S_OK) {
#else
        if(!uuid_parse(uuidstr, _uuid.data)) {
			uuid_to_guid();
#endif
            return;
        }
	}
#ifdef ZQ_OS_MSWIN
	UuidCreateNil(&_uuid);
#else
    uuid_clear(_uuid.data);
#endif

}

/// constructor, from binary data fields
Guid::Guid(unsigned long Data1, unsigned short Data2, unsigned short Data3, unsigned char Data4[8])
{
	_uuid.Data1 = Data1;
	_uuid.Data2 = Data2;
	_uuid.Data3 = Data3;
	memcpy(_uuid.Data4, Data4, sizeof(_uuid.Data4));
#ifdef ZQ_OS_LINUX
	guid_to_uuid();
#endif
}

/// copier
Guid::Guid(const Guid& rhs)
	:_uuid(rhs._uuid) 
{
#ifdef ZQ_OS_LINUX
    uuid_copy(_uuid.data, rhs._uuid.data);
	uuid_to_guid();
#endif
}

Guid::Guid(const UUID& rhs)
	:_uuid(rhs) 
{
#ifdef ZQ_OS_LINUX
    uuid_copy(_uuid.data, rhs.data);
	uuid_to_guid();
#endif
}

// type convert to UUID
Guid::operator UUID() 
{
	return _uuid;
}

// set the Guid to equal to the src
Guid& Guid::operator=(const UUID& src)
{
#ifdef ZQ_OS_MSWIN
	_uuid=src;
#else
    uuid_copy(_uuid.data, src.data);
	uuid_to_guid();
#endif
	return *this;
}

/// set the Guid from a uuid string
Guid& Guid::operator=(const char *str)
{
#ifdef ZQ_OS_MSWIN
	UuidFromStringA((unsigned char*)str,&_uuid);
#else
    uuid_parse(str, _uuid.data); 
	uuid_to_guid();
#endif
	return *this;
}

/// converts a UUID to a display string
int Guid::toString(char* buf, int bufSize) const
{
#ifdef ZQ_OS_MSWIN
	unsigned char *pout=NULL;
	if(RPC_S_OK!=UuidToStringA(const_cast<UUID*>(&_uuid),&pout))
		return 0;
	strncpy(buf, (char *)pout, bufSize)	;
	RpcStringFreeA(&pout);

	return strlen(buf);
#else
    uuid_unparse(_uuid.data, buf);
#endif
	return strlen(buf);
}

/// creates a new UUID
/// @param nil     true if to make a nil guid
bool Guid::create(bool nil)
{
	bool succ = false;
#ifdef ZQ_OS_MSWIN
	succ = (nil) ? (UuidCreateNil(&_uuid)==RPC_S_OK) : (UuidCreate(&_uuid)==RPC_S_OK);
#else
    if(nil)
	{
        uuid_clear(_uuid.data);
    }
    else
	{
        uuid_generate(_uuid.data);
		uuid_to_guid();
    }

	if(!nil && !isNil())
		succ = true;
#endif

	return succ;
}

/// determine whether this is a nil-valued UUID
bool Guid::isNil() const
{
#ifdef ZQ_OS_MSWIN
	RPC_STATUS Status;
	return (TRUE==UuidIsNil(const_cast<UUID*>(&_uuid),&Status));
#else
    return uuid_is_null(_uuid.data);
#endif
}

/// compare two UUIDs and determine their order. The returned value gives the order
int Guid::compare(const Guid &rhs) const
{
#ifdef ZQ_OS_MSWIN
	RPC_STATUS status;
	return ::UuidCompare(const_cast<UUID*>(&_uuid), const_cast<UUID*>(&rhs._uuid), &status);
#else
    return uuid_compare(_uuid.data, rhs._uuid.data);
#endif
}

bool Guid::operator==(const Guid &rhs)const
{
	return ( compare(rhs) == 0);
}

bool Guid::operator >(const Guid &rhs)const
{
	return ( compare(rhs) > 0);
}

bool Guid::operator <(const Guid &rhs)const
{
	return ( compare(rhs) < 0);
}

bool Guid::operator!=(const Guid &rhs)const
{
	return !( *this == rhs);
}

int Guid::toCompactIdstr(char* buf, int bufSize, bool bCaseSensitve) const
{
	if (NULL == buf || bufSize<=0)
		return 0;

	char *pout=buf;

	int nbyte2compact=0;
#ifdef ZQ_OS_MSWIN
	int size = sizeof(_uuid);
#else
	int size = sizeof(_uuid)-sizeof(_uuid.data);
#endif	
	for (int nbyte2process = size; nbyte2process >0; nbyte2process-=nbyte2compact)
	{
		uint64 i64 =0;
		nbyte2compact = nbyte2process>int(sizeof(uint64)) ? sizeof(uint64) : nbyte2process;

		memcpy(&i64, ((char*) &_uuid) + sizeof(_uuid) - nbyte2process, nbyte2compact);

		pout = pout + encodeCompactIdstr(i64, pout, buf + bufSize -pout, bCaseSensitve);
	}

	return strlen(buf);
}

static const char idchars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static const int  nidchars = sizeof(idchars) -1;

int Guid::encodeCompactIdstr(uint64 id, char* buf, const int bufSize, bool bCaseSensitve)
{
	if (NULL == buf || bufSize<=0)
		return 0;

	int nchars = bCaseSensitve ? nidchars : (nidchars-26);

	char *p = buf + bufSize -1, *q= buf;
	for(*p-- ='\0'; id >0 && p>=buf; p--)
	{
		*p = idchars[(int) (id % nchars)];
		id = id / nchars;
	}

	for (p++; '\0' != *p; p++, q++)
		*q = *p;

	*q= '\0';

	return strlen(buf);
}

uint64 Guid::decoderCompactIdstr(char* str, bool bCaseSensitve)
{
	uint64 v = 0;
	uint8  base = nidchars, ch, i;
	if (!bCaseSensitve)
		base -=26;
	for (int j=0; str[j]; j++)
	{
		ch = bCaseSensitve?str[j]:toupper(str[j]);
		for (i=0; i < base; i++)
		{
			if (idchars[i] == ch)
				break;
		}
		if (i >=base)
			return v; // illegal ch reached
		v *= base;
		v += i;
	}

	return v;
}

#ifdef ZQ_OS_LINUX
void Guid::uuid_to_guid() {
	uint8* dst = (uint8*)&_uuid.Data1;
	uint8* src = _uuid.data+3;

	short i = 0;
	while(i++ < 4) *dst++ = *src--;

	dst = (uint8*)&_uuid.Data2;
	src = _uuid.data+5;

	i = 0;
	while(i++ < 2) *dst++ = *src--;

	dst = (uint8*)&_uuid.Data3;
	src = _uuid.data+7;

	i = 0;
	while(i++ < 2) *dst++ = *src--; 

	dst = _uuid.Data4;
	src = _uuid.data+8;

	i = 0;
	while(i++ < 8) *dst++ = *src++; 
}

void Guid::guid_to_uuid() {
	uint8* dst = _uuid.data;
	uint8* src = (uint8*)&_uuid.Data1+3;

	short j = 0;
	while(j++ < 4) *dst++ = *src--;

	src = (uint8*)&_uuid.Data2+1;
	j = 0;
	while(j++ < 2) *dst++ = *src--; 

	src = (uint8*)&_uuid.Data3+1;
	j = 0;
	while(j++ < 2) *dst++ = *src--;

	src = (uint8*)&_uuid.Data4;
	j = 0;
	while(j++ < 8) *dst++ = *src++;
}
#endif

}
}//endof namespace

// vim: ts=4 sw=4 bg=dark nu

