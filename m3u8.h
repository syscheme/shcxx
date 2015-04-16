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
// Ident : $Id: ExpatXX.h,v 1.8 2004/05/26 09:32:35 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : m3u8 parser and generator by refering
//         http://tools.ietf.org/html/draft-pantos-http-live-streaming-11
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/m3u8.h $
// 
// 6     7/26/13 10:17a Hui.shao
// duration to msec
// 
// 3     7/11/13 5:46p Hui.shao
// glance of review
// 
// 2     7/08/13 11:08a Ketao.zhang
// 
// 1     6/25/13 12:27p Hui.shao
// draft definition
// ===========================================================================

#ifndef __ZQ_Common_M3U8_H__
#define __ZQ_Common_M3U8_H__

#include "ZQ_common_conf.h"

#include <string>
#include <vector>
namespace ZQ {
namespace common {

class M3U8Stream 
{
public:

	// each Segment maps to a #EXTINF
	typedef struct Segment
	{
		uint32         duration;   // refer to #EXT-X-TARGETDURATION:<s>, segment duration in milliseconds, while the content of M3U8 takes decimal SSS.sss in sec
		int64          byteStart, byteStop; // refer to #EXT-X-BYTERANGE:<n>[@o], indicates that a media segment is a sub-range
                                // of the resource identified by its media URI. byteStart=0 means from start, byteStop=-1 means EOF
		uint32         bandwidth;
		std::string uri;
		std::string attrs;  // refer to #EXT-X-KEY:<attribute-list>

//		std::string url, key_path;   // url key path
//		uint8_t     aes_key[16];     // AES-128
//		bool        bKeyLoaded;
	} Segment;

	typedef std::vector< Segment > Playlist; // maps to a playlist. single item if "Simple Media Playlist file"

	typedef struct _IndexInfo // the index info exported on #EXT-X-STREAM-INF of "Master Playlist file"
	{
		int      programId;
		uint32   bandwidth; // bandwidth usage of stream in bps
		std::string codecs, audio, video, subtitles;
		int      resolution;
	} IndexInfo;

	M3U8Stream(const IndexInfo& streamInfo, const char *url_m3u8 =NULL);
	virtual ~M3U8Stream();

protected:
    IndexInfo     _streamInfo;    // program id, bandwidth, etc
    int           _version;    // protocol version should be 3, refer to #EXT-X-VERSION:%d
    int           _seqNum;     // media sequence number, refer to M3U8:#EXT-X-MEDIA-SEQUENCE:%d
    uint32        _targetDur;   // maximum duration per segment, refer to M3U8::#EXT-X-TARGETDURATION:%d
    uint64        _size;       // stream length is calculated by taking the sum foreach segment of 
	                           // (segment->duration * hls->bandwidth/8)
	bool          _bAllowCache; // #EXT-X-ALLOW-CACHE:%3s
	bool          _bIsVOD; // refer to #EXT-X-PLAYLIST-TYPE:<EVENT|VOD>, default true
	bool          _bEnded; // refer to #EXT-X-PLAYLIST-TYPE:<EVENT|VOD>, default false

    Playlist      _playlist;     // list of segments
	std::string   _uri_m3u8;       // uri to m3u8

public://add by ketao.zhang using for test 
	// export the _idxInfo in the format of EXT-X-STREAM-INF tag
	//@return the EXT-X-STREAM-INF value, such as
	//  "#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=65000,CODECS="mp4a.40.5"\nhttp://example.com/low.m3u8"
	std::string exportSTREAM_INF();

	// export stream info in the format of m3u8
	//@param pContent  buf to the m3u8 content for writting
	//@param maxLen    max bytes allowed in the pContent to write
	//@return the byte written
	int exportM3U8(char* pContent, int maxLen);

	int exportM3U8(std::string& contentstr);

	// this function is basically to parse a Playlist-level m3u8 content
	//@param pContent  buf to the m3u8 content for reading
	//@param maxLen    max bytes available in the pContent
	//@return true if succeeded
	virtual bool parseM3U8(const char* pContent, int maxLen);
	//this functin is basically to read a line from a playlist or a master playlist
	//@param pBegin the begin of the  char array
	//@param pRead the pot of read out param
	//@remLine the num of lines which is not be readed
	std::string _readLine(char* pBegin,char **pRead,int remLine);
	/* sample m3u8 to cover
	#EXTM3U
	#EXT-X-VERSION:3
	#EXT-X-MEDIA-SEQUENCE:7794
	#EXT-X-TARGETDURATION:15

	#EXT-X-KEY:METHOD=AES-128,URI="https://priv.example.com/key.php?r=52"
	#EXTINF:2.833,http://media.example.com/fileSequence52-A.ts
	#EXTINF:15.0,http://media.example.com/fileSequence52-B.ts
	#EXTINF:13.333,http://media.example.com/fileSequence52-C.ts

	#EXT-X-KEY:METHOD=AES-128,URI="https://priv.example.com/key.php?r=53"
	#EXTINF:15.0,http://media.example.com/fileSequence53-A.ts
	
	#EXT-X-ENDLIST
	*/

	size_t push_back(Segment& seg);
	void endPlaylist() { _bEnded = true; }

};

} // namespace common
} // namespace ZQ

#endif // __ZQ_Common_M3U8_H__
