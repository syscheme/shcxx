// FileName : RtspUtils.h
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 


#ifndef __ZQ_RTSP_ENGINE_RTSP_UTILS_H__
#define __ZQ_RTSP_ENGINE_RTSP_UTILS_H__

#include "ZQ_common_conf.h"
#include <string>
#include <map>
#include <vector>

#define CRLF "\r\n"

namespace ZQRtspCommon
{

typedef struct
{
	std::string strStartLine;
	std::map<std::string, std::string> rtspHeaders;
	std::string strContent;
	int64 receiveTime;
}RtspMessageT;

class RtspUtils
{
public:
	static int getMethod(const std::string strMehtod);
	static std::string getMethodString(int method);
	static void resetMessage(RtspMessageT& rtspMessage);
	static void composeMessage(const RtspMessageT& rtspMessage, std::string& strMsg);
private:
	static const char* const RtspMethodStrings[];
};

class StringHelper
{
public:

	///@function : get line from the buffer
	///@data : buffer pointer
	///@len : buffer len
	///@lineLen : strLine.length + strlen(CRLF)
	///@strLine : the line string exclude CRLF
	static bool getLine(const char* data, int len, int& lineLen, std::string& strLine);

	///@function : split string into key and value by ":"
	static bool splitString(const std::string& str, std::string& strKey, std::string& strValue);

	///@function : get the first word in line, the word is translated into upper case
	static bool getFirstWord(const std::string strLine, std::string& strWord);

	static void splitMsg2Line(const char *pMessageBuf, uint16 usBufSize, ::std::vector<std::string> &msgLine);

};

} // end for ZQRtspCommon

#endif // end for __ZQ_RTSP_ENGINE_RTSP_UTILS_H__
