#include "m3u8.h"
#include <stdio.h>
namespace ZQ {
	namespace common {
M3U8Stream::M3U8Stream(const IndexInfo& streamInfo, const char *url_m3u8 /* =NULL */)
	: _streamInfo(streamInfo), _version(3), _targetDur(0), _size(0), _seqNum(0), _bEnded(false)
{
	if (NULL != url_m3u8)
		_uri_m3u8 = url_m3u8;
	_playlist.clear();
}
	
M3U8Stream::~M3U8Stream()
{

}
std::string M3U8Stream::exportSTREAM_INF()
{
	std::string strStreamRes("#EXTM3U\n"),strlen("");
	char buffer[256]={0};
	snprintf(buffer, sizeof(buffer)-2, "#EXT-X-STREAM-INF:PROGRAM-ID=%d,BANDWIDTH=%ld,CODECS=\"%s\"",_streamInfo.programId,_streamInfo.bandwidth,_streamInfo.codecs.c_str());
	strlen.append(buffer);
	strStreamRes+=strlen;
	strlen.clear();
	if (_streamInfo.resolution != 0)
	{
		snprintf(buffer, sizeof(buffer)-2, "RESOLUTION=%d",_streamInfo.resolution);
		strlen.append(buffer);
		strStreamRes+=strlen;
		strlen.clear();
	}
	if (!_streamInfo.audio.empty())
	{
		snprintf(buffer, sizeof(buffer)-2, "AUDIO=\"%s\"",_streamInfo.audio.c_str());
		strlen.append(buffer);
		strStreamRes+=strlen;
	}
	else if (!_streamInfo.video.empty())
	{
		snprintf(buffer, sizeof(buffer)-2, "VIDEO=\"%s\"",_streamInfo.video.c_str());
		strlen.append(buffer);
		strStreamRes+=strlen;
	}
	else if (!_streamInfo.subtitles.empty())
	{
		snprintf(buffer, sizeof(buffer)-2, "SUBTITLES=\"%s\"",_streamInfo.subtitles.c_str());
		strlen.append(buffer);
		strStreamRes+=strlen;
	}
	strlen.clear();
	snprintf(buffer, sizeof(buffer)-2, "%s\r\n#EXT-X-ENDLIST\r\n",_uri_m3u8.c_str());
	strlen.append(buffer);
	strStreamRes+=strlen;
	return strStreamRes;
}



int M3U8Stream::exportM3U8(char* pContent, int maxLen)
{
	std::string playlistM3u8;
	exportM3U8(playlistM3u8);
	if (maxLen < playlistM3u8.length())
		maxLen = playlistM3u8.length();

	memccpy(pContent, playlistM3u8.c_str(), 0, maxLen *sizeof(char));
	return maxLen;
}


int M3U8Stream::exportM3U8(std::string& playlistM3u8)
{
	std::string strLine("");
	//char * cLine = const_cast<char *> (strLine.data());
	char cLine[256];
	snprintf(cLine, sizeof(cLine)-2, "#EXTM3U\r\n");
	strLine.append(cLine);
	playlistM3u8 += strLine;
	strLine.clear();
	snprintf(cLine, sizeof(cLine)-2, "#EXT-X-VERSION:%d\r\n",_version);
	strLine.append(cLine);
	playlistM3u8 += strLine;
	strLine.clear();
	snprintf(cLine, sizeof(cLine)-2, "#EXT-X-MEDIA-SEQUENCE:%d\r\n",_seqNum);
	strLine.append(cLine);
	playlistM3u8 += strLine;
	strLine.clear();
	snprintf(cLine, sizeof(cLine)-2, "#EXT-X-TARGETDURATION:%d.%03d\r\n",_targetDur/1000, _targetDur%1000);
	strLine.append(cLine);
	playlistM3u8 += strLine;
	strLine.clear();
	if (_bAllowCache)
	{

		sprintf(cLine,"#EXT-X-ALLOW-CACHE:YES\r\n");
		strLine.append(cLine);
	}
	else
	{
		sprintf(cLine,"#EXT-X-ALLOW-CACHE:NO\r\n");
		strLine.append(cLine);
	}
	playlistM3u8 += strLine;
	strLine.clear();
	if (_bIsVOD)
	{
		sprintf(cLine,"#EXT-X-ALLOW-TYPE:VOD\r\n");
		strLine.append(cLine);
	} 
	else
	{
		sprintf(cLine,"#EXT-X-ALLOW-TYPE:EVENT\r\n");
		strLine.append(cLine);
	}
	playlistM3u8 += strLine;
	strLine.clear();
	std::vector< Segment >::iterator iterPlist = _playlist.begin();
	Segment currentSegment;
	for (iterPlist;iterPlist != _playlist.end();iterPlist++)
	{
		currentSegment = *iterPlist;
		if (!currentSegment.attrs.empty())
		{
			snprintf(cLine, sizeof(cLine)-2, "\r\n#EXT-X-KEY:METHOD=%s,URI=\"%s\"\r\n",currentSegment.attrs.c_str(),currentSegment.uri.c_str());
			strLine.append(cLine);
			playlistM3u8 += strLine;
			strLine.clear();
		}
		else if (currentSegment.byteStop > 0)
		{
			snprintf(cLine, sizeof(cLine)-2, "\r\n#EXT-X-BYTERANGE:%d@%d\r\n",(currentSegment.byteStop-currentSegment.byteStart),currentSegment.byteStart);
			strLine.append(cLine);
			playlistM3u8 += strLine;
			strLine.clear();
		}
		else 
		{
			snprintf(cLine, sizeof(cLine)-2, "\r\n#EXTINF:%d.%03d,\r\n%s\r\n", currentSegment.duration/1000, currentSegment.duration%1000, currentSegment.uri.c_str());
			strLine.append(cLine);
			playlistM3u8 += strLine;
			strLine.clear();
		}
	}

	if (_bEnded)
	{
		snprintf(cLine, sizeof(cLine)-2, "#EXT-X-ENDLIST\r\n");
		strLine.append(cLine);
		playlistM3u8 += strLine;
	}

	return playlistM3u8.length();
}

std::string M3U8Stream::_readLine(char* pBegin,char **pRead,int remLine)
{
	if (remLine <= 0)
	{
		return "";
	}
	std::string line;
	*pRead = pBegin;
	for (;*pRead <= (pBegin + remLine);(*pRead)++)
	{
		//if ((*(*pRead) == '\r' || *(*pRead) == '\n') &&(*((*pRead)+1)) == '\n')
		if(*(*pRead) == '\n')
		{
			(*pRead) +=1;
			break;
		}
		line.push_back(*(*pRead));
	}
	return line;
}

bool M3U8Stream::parseM3U8(const char* pContent, int maxLen)
{
      char* pRead, *pBegin = const_cast<char *>(pContent), *pEnd = pBegin + maxLen;
	std::string line = _readLine(pBegin, &pRead, pEnd - pBegin); pBegin = pRead;
	if (0 != line.compare(0, 7 , "#EXTM3U"))
	{
		//log(ZQ::common::LOG_ERROR, "missing #EXTM3U tag .. abort parsing");
		return false;
	}
	while (pBegin < pEnd)
	{
		line = _readLine(pBegin, &pRead, pEnd - pBegin); pBegin = pRead;
		if (line.empty())
			continue;
		int ival =-1,sequence = -1 ,duration = -1;
		ZQ::common::M3U8Stream::Segment theSegment;
		memset(&theSegment,0,sizeof(theSegment));
		int64 byteStart=0,byteLen=0;
		std::string cache ,playListType;
		char attrs[256],uri[256],codes[256];
		float tmpFloat=0;
		if (sscanf((const char*) line.c_str(),"#EXT-X-VERSION:%d", &ival) >0)
		{
			_version = ival;
		}
		else if (sscanf((const char*) line.c_str(), "#EXT-X-MEDIA-SEQUENCE:%d", &sequence) >0)
		{
			_seqNum = sequence;
		}
		else if (sscanf((const char*) line.c_str(), "#EXT-X-TARGETDURATION:%d", &duration) >0)
		{
			_targetDur=duration;	
		}
		else if (sscanf((const char*) line.c_str(), "#EXT-X-KEY:METHOD=%[^,],URI=\"%[^\"]", attrs,uri) >0)
		{
			theSegment.attrs.append(attrs);
			theSegment.uri.append(uri);
			_playlist.push_back(theSegment);
		}
		else if (sscanf((const char*) line.c_str(), "#EXTINF:%f", &tmpFloat) >0)
		{
			theSegment.duration = tmpFloat*1000;
			line = _readLine(pBegin, &pRead, pEnd - pBegin); pBegin = pRead;
			if (_targetDur >0 && theSegment.duration <= _targetDur)
			{
				theSegment.uri.append(line);
				_playlist.push_back(theSegment);
			}
		}
		else if (sscanf((const char*) line.c_str(), "#EXT-X-BYTERANGE:%lld@%lld", &byteLen,&byteStart) >1)
		{
			theSegment.byteStart=byteStart;
			theSegment.byteStop=byteStart+byteLen;
			_playlist.push_back(theSegment);
		}
		else if (sscanf((const char*) line.c_str(), "#EXT-X-BYTERANGE:%lld", &byteLen) >0)
		{
			theSegment.byteStart=byteStart;
			theSegment.byteStop=byteStart+0;
			_playlist.push_back(theSegment);
		}
		else if (sscanf((const char*) line.c_str(), "#EXT-X-ALLOW-CACHE:%s", &cache) >0)
		{
			if(0 == cache.compare(0 , 3 , "YES"))
			{
				_bAllowCache = true;
			}
			else
			{
				_bAllowCache = false;
			}
		}
		else if (sscanf((const char*) line.c_str(), "#EXT-X-PLAYLIST-TYPE:%s", &playListType) >0)
		{
			if(0 == cache.compare(0 , 3 , "VOD"))
			{
				_bIsVOD = true;
			}
			else
			{
				_bIsVOD = false;
			}
		}
		else if (0 == line.compare(0, 17 , "#EXT-X-STREAM-INF"))
		{
			int fRes=line.find("PROGRAM-ID");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"PROGRAM-ID=%d",&_streamInfo.programId);
			}
			fRes=line.find("BANDWIDTH");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"BANDWIDTH=%ld",&_streamInfo.bandwidth);
			}
			fRes=line.find("CODECS");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"CODECS=\"%[^\"]",codes);
			}
			_streamInfo.codecs.clear();
			_streamInfo.codecs.append(codes);
			 fRes=line.find("RESOLUTION");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"RESOLUTION=%d",&_streamInfo.resolution);
			}
			fRes = line.find("AUDIO");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				char buffer[256];
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"AUDIO=\"%[^\"]",buffer);
				_streamInfo.audio.clear();
				_streamInfo.audio.append(buffer);
			}
			fRes = line.find("VIDEO");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				char buffer[256];
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"VIDEO=\"%[^\"]",buffer);
				_streamInfo.video.clear();
				_streamInfo.video.append(buffer);
			}
			fRes = line.find("SUBTITLES");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				char buffer[256];
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"SUBTITLES=\"%[^\"]",buffer);
				_streamInfo.subtitles.clear();
				_streamInfo.subtitles.append(buffer);
			}
			line = _readLine(pBegin, &pRead, pEnd - pBegin); pBegin = pRead;
			_uri_m3u8.assign(line);
		}
		else if (0 == line.compare(0, 25, "#EXT-X-I-FRAME-STREAM-INF"))
		{
			int fRes=line.find("PROGRAM-ID");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"PROGRAM-ID=%d",&_streamInfo.programId);
			}
			fRes=line.find("BANDWIDTH");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"BANDWIDTH=%ld",&_streamInfo.bandwidth);
			}
			fRes=line.find("CODECS");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"CODECS=\"%[^\"]",codes);
			}
			_streamInfo.codecs.append(codes);
		      fRes=line.find("RESOLUTION");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"RESOLUTION=%d",&_streamInfo.resolution);
			}
			fRes = line.find("VIDEO");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				char buffer[256]={0};
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"VIDEO=\"%[^\"]",buffer);
				_streamInfo.video.append(buffer);
			}
			fRes = line.find("URI");
			if(fRes != std::string::npos)
			{
				std::string strRes;
				char buffer[256]={0};
				strRes.assign(line.begin()+fRes,line.end());
				sscanf((const char *)strRes.c_str(),"URI=\"%[^\"]",buffer);
				_uri_m3u8.append(buffer);
			}
		}
		else
		{
			//this line doesn't fit any tag such as space lines or #EXT-X-ENDLIST
			//do nothing
		}
	}//while
	return true;
}//parseM3U8

size_t M3U8Stream::push_back(Segment& seg)
{
	//TODO: validate the input

	if (_targetDur < seg.duration)
		_targetDur = seg.duration;

	_playlist.push_back(seg);
	return _playlist.size();
}


}} //namespaces
