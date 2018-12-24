// ===========================================================================
// Copyright (c) 2004 by
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
// Ident : $Id: Exception.h,v 1.8 2004/05/26 09:32:35 mwang Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define common exceptions
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/CryptoAlgm.cpp $
// 
// 9     3/29/17 11:20a Hui.shao
// 
// 8     3/29/17 11:17a Hui.shao
// 
// 6     3/28/17 5:00p Hui.shao
// 
// 5     3/15/17 9:39a Hui.shao
// base64 codec
// 
// 4     3/14/17 4:53p Hui.shao
// 
// 3     3/13/17 5:43p Hui.shao
// 
// 2     3/13/17 4:56p Hui.shao
// drafted md5 and hmac_sha1
// 
// 1     8/24/16 3:47p Hui.shao
// created
// ===========================================================================

#include "CryptoAlgm.h"

#include <string>
#include <exception>

extern "C" {
#include <stdarg.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
}

namespace ZQ {
namespace common {

// -----------------------------
// class CRC32
// -----------------------------
static uint32 crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32 CRC32::crc32(uint32 crc, const void *buf, size_t size)
{
	const uint8* p = (const uint8*) buf;
	crc ^= ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

// -----------------------------
// class SHA1
// -----------------------------
#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
// 20061122 Modify _BIG_ENDIAN_
#ifdef _BIG_ENDIAN_
	#define blk0(i) block->l[i]
#else
#define blk0(i) (block->l[i] = (rol(block->l[i],24)&(uint32)0xFF00FF00) \
		|(rol(block->l[i],8)&(uint32)0x00FF00FF))
#endif

#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
	^block->l[(i+2)&15]^block->l[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

typedef union _BYTE64QUAD16 {
	uint8 c[64];
	uint32 l[16];
} BYTE64QUAD16;

/* Hash a single 512-bit block. This is the core of the algorithm. */
void SHA1::SHA1_Transform(uint32 state[5], uint8 buffer[64]) {
	uint32	a, b, c, d, e;
	BYTE64QUAD16	*block;

	block = (BYTE64QUAD16*)buffer;
	/* Copy context->state[] to working vars */
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	/* 4 rounds of 20 operations each. Loop unrolled. */
	R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
	R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
	R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
	R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
	R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
	R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
	R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
	R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
	R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
	R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
	R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
	R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
	R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
	R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
	R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
	R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
	R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
	R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
	R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
	R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
	/* Add the working vars back into context.state[] */
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	/* Wipe variables */
	a = b = c = d = e = 0;
}


/* SHA1_Init - Initialize new context */
void SHA1::SHA1_Init(SHA1::CTX* context) {
	/* SHA1 initialization constants */
	context->state[0] = 0x67452301;
	context->state[1] = 0xEFCDAB89;
	context->state[2] = 0x98BADCFE;
	context->state[3] = 0x10325476;
	context->state[4] = 0xC3D2E1F0;
	context->count[0] = context->count[1] = 0;
}

/* Run your data through this. */
void SHA1::SHA1_Update(SHA1::CTX *context, const uint8 *data, uint32 len) {
	unsigned int	i, j;
	//To avoid modifying the original data
	//********************************************************
	//uint8 *MicTempBuffer=(uint8*)malloc(len);//added by WangJS 2005.4.20
	uint8 MicTempBuffer[1024]={0};
	memset(MicTempBuffer,1024 , 0);

	memcpy(MicTempBuffer,data,len); //added by WangJS 2005.4.20
	//********************************************************

	j = (context->count[0] >> 3) & 63;
	if ((context->count[0] += len << 3) < (len << 3)) context->count[1]++;
	context->count[1] += (len >> 29);
	if ((j + len) > 63) {
	   // OS_MEMORY_COPY(&context->buffer[j], data, (i = 64-j));  //marked by WangJS 2005.4.20
	    memcpy(&context->buffer[j], MicTempBuffer, (i = 64-j));
	    SHA1_Transform(context->state, context->buffer);
	    for ( ; i + 63 < len; i += 64) {
//	        SHA1_Transform(context->state, &data[i]); //marked by WangJS 2005.4.20
		SHA1_Transform(context->state, &MicTempBuffer[i]); //added by WangJS 2005.4.20
	    }
	    j = 0;
	}
	else i = 0;
//	OS_MEMORY_COPY(&context->buffer[j], &data[i], len - i);  //marked by WangJS 2005.4.20
	memcpy(&context->buffer[j], &MicTempBuffer[i], len - i);   //added by WangJS 2005.4.20
	
}


/* Add padding and return the message digest. */
void SHA1::SHA1_Final(SHA1::Digest digest, SHA1::CTX *context) {
	uint32	i, j;
	uint8	finalcount[8];

	for (i = 0; i < 8; i++) {
	    finalcount[i] = (uint8)((context->count[(i >= 4 ? 0 : 1)]
	     >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
	}
	SHA1_Update(context, (uint8 *)"\200", 1);
	while ((context->count[0] & 504) != 448) {
	    SHA1_Update(context, (uint8 *)"\0", 1);
	}
	/* Should cause a SHA1_Transform() */
	SHA1_Update(context, finalcount, 8);
	for (i = 0; i < SHA1_DIGEST_LENGTH; i++) {
	    digest[i] = (uint8) ((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
	}
	/* Wipe variables */
	i = j = 0;
	memset(context->buffer, 0, SHA1_BLOCK_LENGTH);
	memset(context->state, 0, SHA1_DIGEST_LENGTH);
	memset(context->count, 0, 8);
	memset(&finalcount, 0, 8);
}

// -----------------------------
// class MD5
// -----------------------------
static uint8 PADDING[64] =
{
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21


// PrintMD5: Converts a completed MD5 digest into a char* string.
std::string MD5::Digest2Str(const Digest digest)
{
	char strbuf[40] ="";
	int i;
	uint8* d = (uint8*) digest;

	for (i = 0; i < MD5_DIGEST_LENGTH; i++)
		snprintf(strbuf+2*i, 3, "%02x", d[i]);

	return std::string(strbuf);
}

// MD5String: Performs the MD5 algorithm on a char* string, returning
// the results as a char*.
std::string MD5::MD5ofString(const char* str)
{
	MD5 alg;
	alg.update((uint8*)str, (uint32)strlen(str));
	alg.finalize();

	return alg.toString();
}

// MD5File: Performs the MD5 algorithm on a file (binar or text),
// returning the results as a char*.  Returns NULL if it fails.
std::string MD5::MD5ofFile(const char* filename)
{
	FILE* file;
	MD5 alg;
	int len;
	uint8 chBuffer[1024] ="";

	try
	{
		if ((file = fopen (filename, "rb")) != NULL)
		{
			while ((len = fread (chBuffer, 1, 1024, file)) > 0)
				alg.update(chBuffer, len);

			fclose (file);

			alg.finalize();
			return alg.toString();
		}
	}
	catch(...)
	{
	}

	return "";
}


void MD5::reset()
{
	memset(_counts, 0, 2 * sizeof(uint32));

	_states[0] = 0x67452301;
	_states[1] = 0xefcdab89;
	_states[2] = 0x98badcfe;
	_states[3] = 0x10325476;
}

// MD5::update
// MD5 block update operation. Continues an MD5 message-digest
// operation, processing another message block, and updating the
// context.
void MD5::update(uint8* data, uint32 len)
{
	uint32 i, index, partLen;

	// Compute number of bytes mod 64
	index = (unsigned int)((_counts[0] >> 3) & 0x3F);

	// update number of bits
	if ((_counts[0] += (len << 3)) < (len << 3))
		_counts[1]++;

	_counts[1] += (len >> 29);

	partLen = 64 - index;

	// _transfrom as many times as possible.
	if (len >= partLen)
	{
		memcpy( &_buf[index], data, partLen );
		transfrom(_buf);

		for (i = partLen; i + 63 < len; i += 64)
			transfrom(&data[i]);

		index = 0;
	}
	else
		i = 0;

  // Buffer remaining input
  memcpy( &_buf[index], &data[i], len-i );
}

// MD5::finalize
// MD5 finalization. Ends an MD5 message-digest operation, writing
// the message digest and zeroizing the context.
void MD5::finalize()
{
	uint8 bits[8];
	uint32 index, padLen;

	// Save number of bits
	encode (bits, _counts, 8);

	// Pad out to 56 mod 64
	index = (unsigned int)((_counts[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	update(PADDING, padLen);

	// Append len (before padding)
	update (bits, 8);

	// Store state in digest
	encode (_digest, _states, 16);

	memset(_counts, 0, sizeof(_counts));
	memset(_states, 0, sizeof(_states));
	memset(_buf,    0, sizeof(_buf));
}

#define rotate_left(x, n) ((((uint32)x) << n) | (((uint32)x) >> n))

#define F(x, y, z)        ((((uint32)x) & ((uint32)y)) | ((~((uint32)x) & ((uint32)z))))
#define G(x, y, z)        ((((uint32)x) & ((uint32)z)) | (((uint32)y) & ~((uint32)z)))
#define H(x, y, z)        (((uint32)x) ^ ((uint32)y) ^ ((uint32)z))
#define I(x, y, z)        (((uint32)y) ^ (((uint32)x) | ~((uint32)z)))

#define FF(a, b, c, d, x, s, ac)  { a += F(b, c, d) + x + ac; a = rotate_left(a, s); a += b; }
#define GG(a, b, c, d, x, s, ac)  { a += G(b, c, d) + x + ac; a = rotate_left(a, s); a += b; }
#define HH(a, b, c, d, x, s, ac)  { a += H(b, c, d) + x + ac; a = rotate_left(a, s); a += b; }
#define II(a, b, c, d, x, s, ac)  { a += I(b, c, d) + x + ac; a = rotate_left(a, s); a += b; }

// MD5::transfrom
// MD5 basic transformation. Transforms state based on block.
void MD5::transfrom (uint8* block)
{
  uint32 a = _states[0], b = _states[1], c = _states[2], d = _states[3], x[16];
  decode (x, block, 64);

  // Round 1
  FF (a, b, c, d, x[ 0], S11, 0xd76aa478);
  FF (d, a, b, c, x[ 1], S12, 0xe8c7b756);
  FF (c, d, a, b, x[ 2], S13, 0x242070db);
  FF (b, c, d, a, x[ 3], S14, 0xc1bdceee);
  FF (a, b, c, d, x[ 4], S11, 0xf57c0faf);
  FF (d, a, b, c, x[ 5], S12, 0x4787c62a);
  FF (c, d, a, b, x[ 6], S13, 0xa8304613);
  FF (b, c, d, a, x[ 7], S14, 0xfd469501);
  FF (a, b, c, d, x[ 8], S11, 0x698098d8);
  FF (d, a, b, c, x[ 9], S12, 0x8b44f7af);
  FF (c, d, a, b, x[10], S13, 0xffff5bb1);
  FF (b, c, d, a, x[11], S14, 0x895cd7be);
  FF (a, b, c, d, x[12], S11, 0x6b901122);
  FF (d, a, b, c, x[13], S12, 0xfd987193);
  FF (c, d, a, b, x[14], S13, 0xa679438e);
  FF (b, c, d, a, x[15], S14, 0x49b40821);

 // Round 2
  GG (a, b, c, d, x[ 1], S21, 0xf61e2562);
  GG (d, a, b, c, x[ 6], S22, 0xc040b340);
  GG (c, d, a, b, x[11], S23, 0x265e5a51);
  GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa);
  GG (a, b, c, d, x[ 5], S21, 0xd62f105d);
  GG (d, a, b, c, x[10], S22,  0x2441453);
  GG (c, d, a, b, x[15], S23, 0xd8a1e681);
  GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8);
  GG (a, b, c, d, x[ 9], S21, 0x21e1cde6);
  GG (d, a, b, c, x[14], S22, 0xc33707d6);
  GG (c, d, a, b, x[ 3], S23, 0xf4d50d87);
  GG (b, c, d, a, x[ 8], S24, 0x455a14ed);
  GG (a, b, c, d, x[13], S21, 0xa9e3e905);
  GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8);
  GG (c, d, a, b, x[ 7], S23, 0x676f02d9);
  GG (b, c, d, a, x[12], S24, 0x8d2a4c8a);

  // Round 3
  HH (a, b, c, d, x[ 5], S31, 0xfffa3942);
  HH (d, a, b, c, x[ 8], S32, 0x8771f681);
  HH (c, d, a, b, x[11], S33, 0x6d9d6122);
  HH (b, c, d, a, x[14], S34, 0xfde5380c);
  HH (a, b, c, d, x[ 1], S31, 0xa4beea44);
  HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9);
  HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60);
  HH (b, c, d, a, x[10], S34, 0xbebfbc70);
  HH (a, b, c, d, x[13], S31, 0x289b7ec6);
  HH (d, a, b, c, x[ 0], S32, 0xeaa127fa);
  HH (c, d, a, b, x[ 3], S33, 0xd4ef3085);
  HH (b, c, d, a, x[ 6], S34,  0x4881d05);
  HH (a, b, c, d, x[ 9], S31, 0xd9d4d039);
  HH (d, a, b, c, x[12], S32, 0xe6db99e5);
  HH (c, d, a, b, x[15], S33, 0x1fa27cf8);
  HH (b, c, d, a, x[ 2], S34, 0xc4ac5665);

  // Round 4
  II (a, b, c, d, x[ 0], S41, 0xf4292244);
  II (d, a, b, c, x[ 7], S42, 0x432aff97);
  II (c, d, a, b, x[14], S43, 0xab9423a7);
  II (b, c, d, a, x[ 5], S44, 0xfc93a039);
  II (a, b, c, d, x[12], S41, 0x655b59c3);
  II (d, a, b, c, x[ 3], S42, 0x8f0ccc92);
  II (c, d, a, b, x[10], S43, 0xffeff47d);
  II (b, c, d, a, x[ 1], S44, 0x85845dd1);
  II (a, b, c, d, x[ 8], S41, 0x6fa87e4f);
  II (d, a, b, c, x[15], S42, 0xfe2ce6e0);
  II (c, d, a, b, x[ 6], S43, 0xa3014314);
  II (b, c, d, a, x[13], S44, 0x4e0811a1);
  II (a, b, c, d, x[ 4], S41, 0xf7537e82);
  II (d, a, b, c, x[11], S42, 0xbd3af235);
  II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb);
  II (b, c, d, a, x[ 9], S44, 0xeb86d391);

  _states[0] += a;
  _states[1] += b;
  _states[2] += c;
  _states[3] += d;

  memset(x, 0, sizeof(x));
}

// MD5::encode
// Encodes input (uint32) into output (uint8). Assumes nLength is
// a multiple of 4.
void MD5::encode(uint8* dest, uint32* src, uint32 nLength)
{
	uint32 i, j;

	assert(nLength % 4 == 0);

	for (i = 0, j = 0; j < nLength; i++, j += 4)
	{
		dest[j] = (uint8)(src[i] & 0xff);
		dest[j+1] = (uint8)((src[i] >> 8) & 0xff);
		dest[j+2] = (uint8)((src[i] >> 16) & 0xff);
		dest[j+3] = (uint8)((src[i] >> 24) & 0xff);
	}
}

// MD5::decode
// Decodes input (uint8) into output (uint32). Assumes nLength is
// a multiple of 4.
void MD5::decode(uint32* dest, uint8* src, uint32 nLength)
{
	uint32 i, j;

	assert(nLength % 4 == 0);

	for (i = 0, j = 0; j < nLength; i++, j += 4)
	{
		dest[i] = ((uint32)src[j]) | (((uint32)src[j+1])<<8) |
			      (((uint32)src[j+2])<<16) | (((uint32)src[j+3])<<24);
	}
}

// -----------------------------
// class HMAC_SHA1
// -----------------------------
// Filler bytes
#define IPAD_BYTE	0x36
#define OPAD_BYTE	0x5c
#define ZERO_BYTE	0x00

void HMAC_SHA1::HMAC_SHA1_Init(HMAC_SHA1::CTX* ctx)
{
	memset(&(ctx->key[0]),  ZERO_BYTE, HMAC_SHA1_BLOCK_LENGTH);
	memset(&(ctx->ipad[0]), IPAD_BYTE, HMAC_SHA1_BLOCK_LENGTH);
	memset(&(ctx->opad[0]), OPAD_BYTE, HMAC_SHA1_BLOCK_LENGTH);
	ctx->keylen = 0;
	ctx->hashkey = 0;
}

void HMAC_SHA1::HMAC_SHA1_UpdateKey(HMAC_SHA1::CTX* ctx, uint8 *key, uint32 keylen)
{
	//  Do we have anything to work with?  If not, return right away
	if (keylen < 1)
		return;

	// Is the total key length (current data and any previous data)
	// longer than the hash block length?
	if (ctx->hashkey !=0 || (keylen + ctx->keylen) > HMAC_SHA1_BLOCK_LENGTH)
	{
		// Looks like the key data exceeds the hash block length,
		// so that means we use a hash of the key as the key data
		// instead.
		if (ctx->hashkey == 0)
		{
			// Ah, we haven't started hashing the key
			// data yet, so we must init. the hash
			// monster to begin feeding it.

			// Set the hash key flag to true (non-zero)
			ctx->hashkey = 1;

			// Init. the hash beastie...
			SHA1::SHA1_Init(&ctx->shactx);

			// If there's any previous key data, use it
			if (ctx->keylen > 0)
				SHA1::SHA1_Update(&ctx->shactx, &(ctx->key[0]), ctx->keylen);

			// Reset the key length to the future true
			// key length, HMAC_SHA1_DIGEST_LENGTH
			ctx->keylen = HMAC_SHA1_DIGEST_LENGTH;
		}

		// Now feed the latest key data to the has monster
		SHA1::SHA1_Update(&ctx->shactx, key, keylen);
	}
	else
	{
		// Key data length hasn't yet exceeded the hash
		// block length (HMAC_SHA1_BLOCK_LENGTH), so theres
		// no need to hash the key data (yet).  Copy it
		// into the key buffer.
		memcpy(&(ctx->key[ctx->keylen]), key, keylen);
		ctx->keylen += keylen;
	}
}

void HMAC_SHA1::HMAC_SHA1_EndKey(HMAC_SHA1::CTX* ctx)
{
	uint8	*ipad, *opad, *key;
	unsigned int	i;

	// Did we end up hashing the key?
	if (ctx->hashkey) 
	{
		memset(&(ctx->key[0]), ZERO_BYTE, HMAC_SHA1_BLOCK_LENGTH);
		// Yes, so finish up and copy the key data
		SHA1::SHA1_Final(&(ctx->key[0]), &ctx->shactx);
		// ctx->keylen was already set correctly
	}

	// Pad the key if necessary with zero bytes
	if ((i = HMAC_SHA1_BLOCK_LENGTH - ctx->keylen) > 0)
		memset(&(ctx->key[ctx->keylen]), ZERO_BYTE, i);

	ipad = &(ctx->ipad[0]);
	opad = &(ctx->opad[0]);

	// Precompute the respective pads XORed with the key
	key = &(ctx->key[0]);
	for (i = 0; i < ctx->keylen; i++, key++)
	{
		// XOR the key uint8 with the appropriate pad filler uint8
		*ipad++ ^= *key;
		*opad++ ^= *key;
	}
}

void HMAC_SHA1:: HMAC_SHA1_StartMessage(HMAC_SHA1::CTX* ctx)
{
	SHA1::SHA1_Init(&ctx->shactx);
	SHA1::SHA1_Update(&ctx->shactx, &(ctx->ipad[0]), HMAC_SHA1_BLOCK_LENGTH);
}

void HMAC_SHA1::HMAC_SHA1_UpdateMessage(HMAC_SHA1::CTX* ctx, const uint8 *data, uint32 datalen) 
{
	SHA1::SHA1_Update(&ctx->shactx, data, datalen);
}

void HMAC_SHA1::HMAC_SHA1_EndMessage(uint8 *out, HMAC_SHA1::CTX* ctx) 
{
	uint8	buf[HMAC_SHA1_DIGEST_LENGTH];
	SHA1::CTX		*c = &ctx->shactx;

	SHA1::SHA1_Final(&(buf[0]), c);
	SHA1::SHA1_Init(c);
	SHA1::SHA1_Update(c, &(ctx->opad[0]), HMAC_SHA1_BLOCK_LENGTH);
	SHA1::SHA1_Update(c, buf, HMAC_SHA1_DIGEST_LENGTH);
	SHA1::SHA1_Final(out, c);
}

void HMAC_SHA1::HMAC_SHA1_Done(HMAC_SHA1::CTX* ctx)
{
	// Just to be safe, toast all context data
	memset(&(ctx->ipad[0]), ZERO_BYTE, HMAC_SHA1_BLOCK_LENGTH);
	memset(&(ctx->ipad[0]), ZERO_BYTE, HMAC_SHA1_BLOCK_LENGTH);
	memset(&(ctx->key[0]), ZERO_BYTE, HMAC_SHA1_BLOCK_LENGTH);
	ctx->keylen = 0;
	ctx->hashkey = 0;
} 

HMAC_SHA1::HMAC_SHA1(uint8 *key, uint key_len)
{ 
	HMAC_SHA1_Init(&_ctx); 

	HMAC_SHA1_UpdateKey(&_ctx, key, key_len); 
	HMAC_SHA1_EndKey(&_ctx); 

	HMAC_SHA1_StartMessage(&_ctx); 
}

void HMAC_SHA1::update(const uint8* data, uint len)
{ HMAC_SHA1_UpdateMessage(&_ctx, (const uint8*) data, len); }

void HMAC_SHA1::get(SHA1::Digest digest) 
{ HMAC_SHA1_EndMessage(digest, &_ctx); } 

void HMAC_SHA1::calcSignature(uint8 *data, uint32 data_len, uint8 *key, uint32 key_len, SHA1::Digest digest)
{
	if (NULL == digest)
		return;

	HMAC_SHA1 hmac(key, key_len);
	hmac.update(data, data_len);
	hmac.get(digest); 
}

// -----------------------------
// class Base64
// -----------------------------
// base64 Encoding/Decoding Table
static const char table64[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static uint8 ultouc(unsigned long ulnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) // conversion may lose significant bits
#endif

	return (uint8)(ulnum & (unsigned long) 0xFF);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

static void decodeQuantum(uint8 *dest, const char *src)
{
	const char *s, *p;
	unsigned long i, v, x = 0;

	for (i = 0, s = src; i < 4; i++, s++)
	{
		v = 0;
		p = table64;
		while(*p && (*p != *s)) { v++;	p++; }

		if (*p == *s)
			x = (x << 6) + v;
		else if(*s == '=')
			x = (x << 6);
	}

	dest[2] = ultouc(x & 0xFFUL);
	x >>= 8;
	dest[1] = ultouc(x & 0xFFUL);
	x >>= 8;
	dest[0] = ultouc(x & 0xFFUL);
}

bool Base64::decode(const std::string& src, OUT uint8 *data, IN OUT size_t& datalen)
{
	return decode(src.c_str(), data, datalen);
}

bool Base64::decode(const char* src, OUT uint8 *data, IN OUT size_t& datalen)
{
	if (NULL == data)
		return false;

	// detemine the input len
	size_t len = 0;
	while((src[len] != '=') && src[len])
		len++;

	// a maximum of two = padding characters is allowed
	size_t equalsTerm = 0;
	if (src[len] == '=')
	{
		equalsTerm++;
		if(src[len+equalsTerm] == '=')
			equalsTerm++;
	}

	size_t numQuantums = (len + equalsTerm) / 4;

	// Don't allocate a buffer if the decoded len is 0
	if(numQuantums <= 0)
		return true;

	size_t rawlen = (numQuantums * 3) - equalsTerm;
	uint8 *dest = data, *tail = data + datalen;

	size_t i;
	// Decode all but the last quantum (which may not decode to a multiple of 3 bytes) 
	for (i = 0; i < (numQuantums - 1) && dest < (tail-3); i++)
	{
		if (dest >= (data + datalen - 3))
			break; // output overflow

		decodeQuantum(dest, src);
		dest += 3; src += 4;
	}

	// This final decode may actually read slightly past the end of the buffer
	// if the input string is missing pad bytes.  This will almost always be
	// harmless.
	uint8 lastQuantum[3];
	decodeQuantum(lastQuantum, src);

	for(i = 0; (i < 3 - equalsTerm) && dest < tail; i++)
		*dest++ = lastQuantum[i];

	datalen = dest - data;
	return true;
}

std::string Base64::encode(const uint8* data, size_t len)
{
	std::string str;
	int i;

	if(len <=0)
		len = strlen((const char*)data);

	str.reserve(len*4/3+4 +4); // len*4/3+4 is the expected length, additional +4 was just because of nervousness

	while (len > 0) 
	{
		// determine the input 3 bytes
		uint8 ibuf[3];
		int inputparts;
		for (i = inputparts = 0; i < 3; i++) 
		{
			if (len > 0) 
			{
				inputparts++;
				ibuf[i] = (uint8) *data ++;
				len--;
			}
			else ibuf[i] = 0;
		}

		// convert to the output 4 bytes
		uint8 obuf[4];
		obuf[0] = (uint8) ((ibuf[0] & 0xFC) >> 2);
		obuf[1] = (uint8) (((ibuf[0] & 0x03) << 4) | ((ibuf[1] & 0xF0) >> 4));
		obuf[2] = (uint8) (((ibuf[1] & 0x0F) << 2) | ((ibuf[2] & 0xC0) >> 6));
		obuf[3] = (uint8) (ibuf[2] & 0x3F);

		// format the chars
		char dstr[6];
		switch(inputparts)
		{
		case 1: // only one byte read
			snprintf(dstr, sizeof(dstr)-1, "%c%c==", table64[obuf[0]], table64[obuf[1]]);
			break;

		case 2: // two bytes read
			snprintf(dstr, sizeof(dstr)-1, "%c%c%c=", table64[obuf[0]], table64[obuf[1]], table64[obuf[2]]);
			break;

		default:
			snprintf(dstr, sizeof(dstr)-1, "%c%c%c%c", table64[obuf[0]], table64[obuf[1]], table64[obuf[2]], table64[obuf[3]] );
			break;
		}

		// append to the output
		str += dstr;
	}

	return str;
}

}} // namespaces