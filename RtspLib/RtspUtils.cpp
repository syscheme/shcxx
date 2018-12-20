// FileName : RtspUtils.cpp
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#include "RtspUtils.h"
#include <cstring>

namespace ZQRtspCommon
{

const char* const RtspUtils::RtspMethodStrings[14] ={ "", "ANNOUNCE", "DESCRIBE", "PLAY", "RECORD", 
"SETUP", "TEARDOWN", "PAUSE", "GET_PARAMETER", "OPTIONS", 
"REDIRECT", "SET_PARAMETER", "PING", "RESPONSE"};

int RtspUtils::getMethod(const std::string strMehtod)
{
	for (int i = 0; i < 14; i++)
	{
		if (strMehtod == RtspMethodStrings[i])
		{
			return i;
		}
	}
	return 0;
}

std::string RtspUtils::getMethodString(int method)
{
	if (method < 0 || method > 13)
	{
		return "";
	}
	return RtspMethodStrings[method];
}

void RtspUtils::resetMessage(RtspMessageT &rtspMessage)
{
	rtspMessage.strStartLine = "";
	rtspMessage.rtspHeaders.clear();
	rtspMessage.strContent = "";
}

void RtspUtils::composeMessage(const ZQRtspCommon::RtspMessageT &rtspMessage, std::string &res)
{
	res = rtspMessage.strStartLine;
	res += CRLF;
	std::map<std::string, std::string>::const_iterator iter = rtspMessage.rtspHeaders.begin();
	for (; iter != rtspMessage.rtspHeaders.end(); iter++)
	{
		res += iter->first;
		res += ": ";
		res += iter->second;
		res += CRLF;
	}
	if (!rtspMessage.strContent.empty())
	{
		char contentLength[16];
		sprintf(contentLength, "%ld", rtspMessage.strContent.size());
		res += "Content-Length: ";
		res += contentLength;
		res += CRLF;
	}
	res += CRLF;
	if (!rtspMessage.strContent.empty())
	{
		res += rtspMessage.strContent;
	}
}

//---------------------------------------------------------------------------------------------

bool StringHelper::getLine(const char* data, int len, int& bytesDecoded, std::string& strLine)
{
	int i = 0;
	while (i + 1 < len)
	{
		if (data[i] == '\r' && data[i + 1] == '\n')
		{
			strLine.assign(data, i);
			bytesDecoded = i + 2;
			return true;
		}
		i++;
	}
	return false;
}

bool StringHelper::getFirstWord(const std::string strLine, std::string &strFirstWord)
{
	size_t nStart = strLine.find_first_not_of(" \f\t\v");
	if (nStart == std::string::npos)
	{
		return false;
	}
	size_t nEnd = strLine.find_first_of(" \f\t\v", nStart);
	if (nEnd == std::string::npos)
	{
		strFirstWord = strLine.substr(nStart);
	}
	else
	{
		strFirstWord = strLine.substr(nStart, nEnd - nStart);
	}
	char* p = const_cast<char*>(strFirstWord.c_str());
	while(*p) { *p = toupper(*p); ++p; }
	return true;
}

bool StringHelper::splitString(const std::string& str, std::string& strKey, std::string& strValue)
{
	size_t nSplit = str.find(":");
	if (nSplit == std::string::npos)
	{
		return false;
	}

	// get key 
	strKey = str.substr(0, nSplit);
	size_t nStart = strKey.find_first_not_of(" \f\t\v");
	if (nStart == std::string::npos)
	{
		return false;
	}
	size_t nEnd = strKey.find_last_not_of(" \f\t\v");
	strKey = strKey.substr(nStart, nEnd - nStart + 1);

	// get value
	strValue = str.substr(nSplit + 1);
	nStart = strValue.find_first_not_of(" \f\t\v");
	if (nStart == std::string::npos)
	{
		strValue = "";
	}
	else
	{
		nEnd = strValue.find(" \f\t\v");
		strValue = strValue.substr(nStart, nEnd - nStart + 1);
	}
	return true;
}


void StringHelper::splitMsg2Line(const char *pMessageBuf, uint16 usBufSize, ::std::vector<std::string> &msgLine)
{
	::std::string strMessage(pMessageBuf, usBufSize);
	::std::string::size_type pos_begin = 0;
	::std::string::size_type pos_end = 0;

	while(1)
	{
		pos_end = strMessage.find_first_of("\r\n", pos_begin);

		if(::std::string::npos == pos_end) // not a valid macro reference
			break;

		::std::string line = strMessage.substr(pos_begin, pos_end - pos_begin);

		//get line
		msgLine.push_back(line);

		//move pos_begin to next line
		pos_begin = pos_end + 2;
	}

}

}// end for ZQRtspCommon
