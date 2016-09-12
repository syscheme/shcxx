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
// Ident : $Id: Exception.h,v 1.8 2004/05/26 09:32:35 mwang Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define common exceptions
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/CryptoAlgm.h $
// 
// 1     8/24/16 3:47p Hui.shao
// created
// ===========================================================================

#ifndef	__ZQ_COMMON_CryptoAlgm_H__
#define	__ZQ_COMMON_CryptoAlgm_H__

#include "ZQ_common_conf.h"

#include <string>
#include <exception>

extern "C" {
#include <stdarg.h>
#include <stdio.h>
}

namespace ZQ {
namespace common {

class ZQ_COMMON_API CRC32;
class ZQ_COMMON_API SHA1;

// -----------------------------
// class CRC32
// -----------------------------
class CRC32
{
public:
	static uint32 crc32(uint32 crc, const void *buf, size_t size);

public:
	CRC32() : _crc32(0) {}
	uint32 get() const { return _crc32; }

	uint32 update(const void *buf, size_t size) 
	{
		_crc32 = crc32(_crc32, buf, size);
		return _crc32;
	}

protected:
	uint32 _crc32;
};

// -----------------------------
// class SHA1
// -----------------------------
// Define this if your machine is LITTLE_ENDIAN, otherwise #undef it: 
// #define LITTLE_ENDIAN 20061122 It'll be defined in makefile
// 20061122 Modify it _BIG_ENDIAN_ definition in makefile, not use LITTLE_ENDIAN
class SHA1
{
public:
	typedef uint32	  quadbyte;	// 4 byte type
	typedef uint8	  byte;	// single byte type

#define SHA1_BLOCK_LENGTH	(64)
#define SHA1_DIGEST_LENGTH	(20)

	SHA1() { SHA1_Init(&_ctx); }
	void update(const byte* data, uint len) { SHA1_Update(&_ctx, data, len); }
	void get(byte digest[SHA1_DIGEST_LENGTH]) { SHA1_Final(digest, &_ctx); } 

protected:
	friend class HMAC_SHA1;
	// The SHA1 structure:
	typedef struct _SHA_CTX {
		quadbyte	state[5];
		quadbyte	count[2];
		byte	    buffer[SHA1_BLOCK_LENGTH];
	} CTX;

	CTX _ctx;

private:

	static void SHA1_Init(CTX *context);
	static void SHA1_Update(CTX *context, const byte *data, unsigned int len);
	static void SHA1_Final(byte digest[SHA1_DIGEST_LENGTH], CTX* context);
	static void SHA1_Transform(quadbyte state[5], byte buffer[64]);
};

/*
// -----------------------------
// class HMAC_SHA1
// -----------------------------
#define HMAC_SHA1_DIGEST_LENGTH	(20)
#define HMAC_SHA1_BLOCK_LENGTH	(64)

class HMAC_SHA1
{
public:
	HMAC_SHA1() { HMAC_SHA1_Init(&_ctx); }
	void update(const byte* data, uint len) { SHA1_Update(&_ctx, data, len); }
	void get(byte digest[SHA1_DIGEST_LENGTH]) { SHA1_Final(digest, &_ctx); } 

protected:
	// The HMAC_SHA1 structure:
	typedef struct{
		unsigned char	ipad[HMAC_SHA1_BLOCK_LENGTH];
		unsigned char	opad[HMAC_SHA1_BLOCK_LENGTH];
		SHA1::CTX	shactx;
		unsigned char	key[HMAC_SHA1_BLOCK_LENGTH];
		unsigned int	keylen;
		unsigned int	hashkey;
	} CTX;

	static void HMAC_SHA1_Init(CTX *ctx);
	static void HMAC_SHA1_UpdateKey(CTX *ctx, unsigned char *key, unsigned int keylen);
	static void HMAC_SHA1_EndKey(CTX *ctx);
	static void HMAC_SHA1_StartMessage(CTX *ctx);
	static void HMAC_SHA1_UpdateMessage(CTX *ctx, unsigned char *data, unsigned int datalen);
	static void HMAC_SHA1_EndMessage(unsigned char *out, CTX *ctx);
	static void HMAC_SHA1_Done(CTX *ctx);
	static void hmac_sha1(unsigned char *text, unsigned int text_len,
		unsigned char *key,  unsigned int key_len,
		unsigned char *digest);
};
*/

}} // namespaces

#endif // __ZQ_COMMON_CryptoAlgm_H__
