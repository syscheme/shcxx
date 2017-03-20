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
// 6     3/15/17 9:39a Hui.shao
// base64 codec
// 
// 5     3/14/17 4:53p Hui.shao
// 
// 4     3/14/17 2:29p Hui.shao
// 
// 3     3/13/17 5:43p Hui.shao
// 
// 2     3/13/17 4:56p Hui.shao
// drafted md5 and hmac_sha1
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
class ZQ_COMMON_API MD5;
class ZQ_COMMON_API SHA1;
class ZQ_COMMON_API HMAC_SHA1;

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
// class MD5
// -----------------------------
#define MD5_DIGEST_LENGTH	(16)
class MD5
{
public:
	typedef uint8 Digest[MD5_DIGEST_LENGTH];

	static std::string Digest2Str(const Digest digest);
	static std::string MD5ofString(const char* str);
	static std::string MD5ofFile(const char* filename);

	// Methods
public:
	MD5() { reset(); }

	void	reset();
	void	update(uint8* data, uint32 len);
	void	finalize();
	uint8*	digest() const { return (uint8*)_digest; }
	std::string toString() const { return Digest2Str(_digest); }

private:
	void	encode(uint8* dest, uint32* src, uint32 nLength);
	void	decode(uint32* dest, uint8* src, uint32 nLength);
	void    transfrom (uint8* block);

	uint32		_states[4];
	uint32		_counts[2];
	uint8		_buf[MD5_DIGEST_LENGTH*4];
	Digest		_digest;
	bool		_finalized;
};

// -----------------------------
// class SHA1
// -----------------------------
// Define this if your machine is LITTLE_ENDIAN, otherwise #undef it: 
// #define LITTLE_ENDIAN 20061122 It'll be defined in makefile
// 20061122 Modify it _BIG_ENDIAN_ definition in makefile, not use LITTLE_ENDIAN
class SHA1
{
#define SHA1_BLOCK_LENGTH	(64)
#define SHA1_DIGEST_LENGTH	(20)

public:
	typedef uint8     Digest[SHA1_DIGEST_LENGTH];

	SHA1() { SHA1_Init(&_ctx); }
	void update(const uint8* data, uint len) { SHA1_Update(&_ctx, data, len); }
	void get(Digest digest) { SHA1_Final(digest, &_ctx); } 

	typedef struct _SHA_CTX {
		uint32	state[5];
		uint32	count[2];
		uint8	buffer[SHA1_BLOCK_LENGTH];
	} CTX;

private:  // the original c-implemetations, DO NOT change
	friend class HMAC_SHA1;
	// The SHA1 structure:
	CTX _ctx;

	static void SHA1_Init(CTX *context);
	static void SHA1_Update(CTX *context, const uint8 *data, uint32 len);
	static void SHA1_Final(Digest digest, CTX* context);
	static void SHA1_Transform(uint32 state[5], uint8 buffer[SHA1_BLOCK_LENGTH]);
};

// -----------------------------
// class HMAC_SHA1
// -----------------------------
#define HMAC_SHA1_DIGEST_LENGTH	 SHA1_DIGEST_LENGTH
#define HMAC_SHA1_BLOCK_LENGTH	 SHA1_BLOCK_LENGTH

class HMAC_SHA1
{
public:
	HMAC_SHA1(uint8 *key, uint key_len);
	void update(const uint8* data, uint len);
	void get(SHA1::Digest digest);

	//@output digest - the result digest
	static void calcSignature(uint8 *data, uint32 data_len, uint8 *key, uint32 key_len, SHA1::Digest digest);

	typedef struct
	{
		uint8	ipad[HMAC_SHA1_BLOCK_LENGTH];
		uint8	opad[HMAC_SHA1_BLOCK_LENGTH];
		SHA1::CTX	shactx;
		uint8	key[HMAC_SHA1_BLOCK_LENGTH];
		unsigned int	keylen;
		unsigned int	hashkey;
	} CTX;

private:  // the original c-implemetations, DO NOT change

	CTX _ctx;
	static void HMAC_SHA1_Init(CTX *ctx);
	static void HMAC_SHA1_UpdateKey(CTX *ctx, uint8 *key, uint32 keylen);
	static void HMAC_SHA1_EndKey(CTX *ctx);
	static void HMAC_SHA1_StartMessage(CTX *ctx);
	static void HMAC_SHA1_UpdateMessage(CTX *ctx, const uint8 *data, uint32 datalen);
	static void HMAC_SHA1_EndMessage(uint8 *out, CTX *ctx);
	static void HMAC_SHA1_Done(CTX *ctx);
};

// -----------------------------
// class Base64
// -----------------------------
class Base64
{
public:
	static std::string encode(const uint8* data, size_t len);
	static bool decode(const char* src, OUT uint8 *data, IN OUT size_t& datalen);
	static bool decode(const std::string& src, OUT uint8 *data, IN OUT size_t& datalen);
};

}} // namespaces

#endif // __ZQ_COMMON_CryptoAlgm_H__
