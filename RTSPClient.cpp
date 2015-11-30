// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: RTSPClient.cpp,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl RTSPClient class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/RTSPClient.cpp $
// 
// 53    11/10/15 3:10p Hui.shao
// 
// 52    11/10/15 3:00p Hui.shao
// ticket#18316 to protect multiple in comming message of a same session
// 
// 51    5/06/15 5:24p Hui.shao
// 
// 50    2/08/14 7:16p Hui.shao
// merged from V1.16
// 
// 49    1/15/14 7:00p Hui.shao
// 
// 48    1/15/14 6:50p Hui.shao
// 
// 47    1/15/14 2:06p Hui.shao
// 
// 46    6/20/13 9:46a Hongquan.zhang
// fix compiling error in CentOS6.3
// 
// 45    4/15/13 9:14p Hui.shao
// in order to allow multiple header lines of a same header name, expected
// by NGOD S6
// 
// 44    12/20/12 11:48a Hui.shao
// 
// 43    12/19/12 4:46p Hui.shao
// 
// 42    12/17/12 5:44p Hui.shao
// added transfer MP2T/AVP/UDP as default for udp://
// 
// 41    5/09/12 2:21p Hui.shao
// 
// 40    5/09/12 2:04p Hui.shao
// fixed the case that the contentbody exceeded 8KB
// 
// 39    4/27/12 12:46p Hui.shao
// reset _inCommingByteSeen when failed to process or reconnected
// 
// 38    4/25/12 4:52p Hui.shao
// 
// 37    4/23/12 4:43p Hui.shao
// SYSTIME.wMonth, 1=Jan
// 
// 36    4/02/12 3:55p Hui.shao
// merged from V1.15.1
// 
// 53    3/29/12 11:31a Hui.shao
// 
// 52    3/29/12 11:26a Hui.shao
// expire pending request when push new in
// 
// 51    3/28/12 8:28p Hui.shao
// rolled back to keep use _lockQueueToSend
// 
// 49    3/27/12 6:20p Hui.shao
// 
// 48    3/27/12 5:44p Hui.shao
// 
// 47    3/14/12 10:27a Hui.shao
// 
// 46    3/13/12 11:37a Build
// 
// 45    3/12/12 6:58p Hui.shao
// 
// 44    3/12/12 6:14p Hui.shao
// 
// 43    3/07/12 6:39p Hui.shao
// 
// 42    1/30/12 11:28a Hui.shao
// 
// 41    1/11/12 2:20p Hui.shao
// 
// 39    1/11/12 2:16p Hui.shao
// 
// 38    9/07/11 9:19p Hui.shao
// 
// 37    8/26/11 11:41a Hui.shao
// 
// 36    8/26/11 10:27a Hui.shao
// changed OnRequestComposed to OnRequestPrepare
// 
// 35    8/24/11 4:54p Hui.shao
// added callback OnRequestComposed() and OnRequestClean()
// 
// 34    8/18/11 5:30p Hui.shao
// 
// 33    8/11/11 4:54p Hui.shao
// 
// 32    3/30/11 3:59p Hui.shao
// 
// 31    3/07/11 10:56a Haoyuan.lu
// 
// 30    2/23/11 3:17p Haoyuan.lu
// add sendErrorCount
// 
// 29    1/31/11 4:02p Hui.shao
// 
// 28    1/28/11 4:14p Haoyuan.lu
// 
// 27    1/28/11 2:56p Hui.shao
// moved the parsing of header Session into RTSPClient from RTSPSession
// for SETUP response
// 
// 26    1/28/11 2:00p Hui.shao
// 
// 24    1/27/11 7:35p Hui.shao
// 
// 23    1/27/11 3:58p Hui.shao
// 
// 22    1/26/11 11:44a Hui.shao
// added more measure log lines
// 
// 21    1/25/11 6:02p Hui.shao
// added verbose flags to print more logs
// 
// 20    1/25/11 11:31a Haoyuan.lu
// 
// 19    1/24/11 6:39p Fei.huang
// 
// 18    1/24/11 2:42p Hui.shao
// make the sleep() linger for _messageTimeout/2 to ensure the message
// timeout would not be missed
// 
// 17    1/21/11 3:01p Haoyuan.lu
// 
// 16    1/20/11 5:33p Hui.shao
// 
// 14    1/19/11 2:50p Hui.shao
// expose the negative failures in the sendXXX() methods
// 
// 13    1/19/11 1:03p Haoyuan.lu
// 
// 12    1/19/11 11:02a Hui.shao
// inc last CSeq via atomic int
// 
// 11    1/14/11 10:15a Haoyuan.lu
// 
// 10    1/12/11 2:32p Haoyuan.lu
// 
// 9     1/12/11 2:21p Haoyuan.lu
// dump received buffer
// 
// 8     1/12/11 1:56p Haoyuan.lu
// fix bug when receive no cseq header
// 
// 7     1/11/11 7:31p Fei.huang
// * fix errorno() infinit loop on linux
// 
// 6     1/11/11 5:04p Haoyuan.lu
// 
// 5     10-12-22 10:47 Haoyuan.lu
// 
// 4     10-12-22 10:40 Fei.huang
// + merged to linux
// 
// 3     10-12-17 11:01 Haoyuan.lu
// 
// 2     10-11-24 15:51 Haoyuan.lu
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 12    10-11-11 10:26 Haoyuan.lu
// 
// 11    10-11-02 16:12 Hui.shao
// code to desc
// 
// 10    10-10-29 13:16 Haoyuan.lu
// 
// 9     10-10-29 13:12 Hui.shao
// 
// 8     10-10-22 14:06 Hui.shao
// RTSPClient linkable
// 
// 7     10-10-22 13:44 Hui.shao
// guess by RTSP session to dispatch in coming server requests
// 
// 6     10-10-22 11:05 Hui.shao
// abstract rtsp session manager interface to port into DB in the future
// 
// 5     10-10-21 19:07 Hui.shao
// inherit RTSPRequest from RTSPMessage
// 
// 4     10-10-21 14:59 Haoyuan.lu
// 
// 3     10-10-21 14:49 Hui.shao
// added RTSPSession
// ===========================================================================

#ifndef    LOGFMTWITHTID
#  define  LOGFMTWITHTID
#endif 

#include "RTSPClient.h"
#include "urlstr.h"
#include "SystemUtils.h"
#include <algorithm>

namespace ZQ{
namespace common{

// -----------------------------
// class RTSPRequest
// -----------------------------
RTSPRequest::RTSPRequest(RTSPClient& client, uint cseq, const char* commandName, RTSPSession* pSession,
	                    uint32 flags, double start, double end, float scale, const char* contentStr, void* pUserExtData)
	: RTSPMessage(cseq), _client(client), _commandName(commandName), _pUserExtData(pUserExtData),
	 _flags(flags), _startPos(start), _endPos(end), _scale(scale)
{
	if (NULL != pSession)
		_sessGuid = pSession->guid();

	if (NULL !=contentStr)
		contentBody = contentStr;
}

RTSPRequest::~RTSPRequest()
{
	_client.OnRequestClean(*this);
}

// -----------------------------
// class RTSPSink
// -----------------------------
const char* RTSPSink::resultCodeToStr(uint resultCode)
{
	switch( resultCode )
	{
	case rcOK:                   return "OK";
	case rcBadRequest:           return "Bad request";
	case rcUnauthorized:         return "Unauthorized";
	case rcForbidden:	         return "Forbidden";
	case rcObjectNotFound:       return "Object Not Found";
	case rcNotAcceptable:    	 return "Not Acceptable";
	case rcRequestTimeout:       return "Request Timeout";
	case rcBadParameter:         return "Bad Parameter";
	case rcNotEnoughBandwidth:   return "Not Enough Bandwidth";
	case rcSessNotFound:		 return "Session Not Found";
	case rcInvalidState:		 return "Method Not Valid In This State";
	case rcInvalidRange:		 return "Invalid Range";

	case rcInternalError:   	 return "Internal Server Error";
	case rcNotImplement:		 return "Not Implemented";
	case rcServiceUnavail:	     return "Service Unavailable";
	case rcOptionNotSupport:	 return "Option Not Supported";

	case rcNoResponse:				 return "ServerSetupFailed No Response";
	case rcAssetNotFound:			 return "ServerSetupFailed AssetNotFound";
	case rcSopNotAvail:				 return "ServerSetupFailed SOPNotAvailable";
	case rcUnknownSopGroup:			 return "ServerSetupFailed UnknownSOPGroup";
	case rcUnkownSopnames:			 return "ServerSetupFailed UnknownSOPNames";
	case rcNotEnoughVolBandwidth:	 return "ServerSetupFailed InsufficientVolumeBandwidth";
	case rcNotEnoughNetworkBandwidth:return "ServerSetupFailed InsufficientNetworkBandwidth";
	case rcInvalidRequest:			 return "ServerSetupFailed InvalidRequest";

#ifdef RESPONSE_OPTIONS_NOT_SUPPORT
	case rcOptionNotSupport:			return RESPONSE_OPTIONS_NOT_SUPPORT;
#endif //RESPONSE_OPTIONS_NOT_SUPPORT
	default:
		return "Unknown";
	}
}

const char* RTSPSink::requestErrToStr(uint reqErrCode)
{
#pragma message ( __MSGLOC__ "TODO: maps the RequestError to description string")
	switch (reqErrCode)
	{
	case RTSPSink::Err_ConnectError:
		return "ConnectError";
		break;
	case RTSPSink::Err_ConnectionLost:
		return "ConnectionLost";
		break;
	case RTSPSink::Err_InvalidParams:
		return "InvalidParams";
		break;
	case RTSPSink::Err_SendFailed:
		return "SendFailed";
		break;
	case RTSPSink::Err_RequestTimeout:
		return "RequestTimeout";
		break;
	default:
		return "UNKNOWN";
		break;
	}

	return "UNKNOWN";
}

const char* RTSPSink::announceCodeToStr(uint announceCode)
{
	switch( announceCode )
	{
	case racEndOfStream:            return "End-of-Stream Reached";
	case racTransition:             return "Transition";
	case racBeginOfStream:          return "Start-of-Stream Reached";
	case racPauseTimeout:           return "Pause Timeout Reached";

		// TianShan extensions
	case racStateChanged:			return "State Changed";
	case racScaleChanged:           return  "Scale Changed";

	// NGOD compatible
	case racTrickNoConstrained:     return "Trick play no longer constrained";
	case racTrickConstrained:       return "Trick play constrained";
	case racItemSkipped:            return "Skipped play list item";
	case racClientSessionTerminated:return "Client Session Terminated";
	case racSessionInProgress:      return "Session In Progress";
	case racErrorReadingData:       return "Error Reading Content Data";
	case racDownstreamFail:         return "Downstream Failure";
	case racInternalServerError:    return "Internal Server Error";
	case racBandwidthExceeded:      return "Bandwidth Exceeded Limit";
	case racServerResourceUnavail:  return "Server Resources Unavailable";
	case racStreamBwUnaval:         return "Stream Bandwidth Exceeds That Available";
	case racDownstreamUreachable:   return "Downstream Destination Unreachable";
	case racUnableEncrpt:           return "Unable to Encrypt one or more Components";
	default:
		return "UNKNOWN";
	}
}

std::string RTSPSink::trim(char const* str)
{
	if (NULL == str)
		return "";
	int len = strlen(str);
	// The line begins with the desired header name.  Trim off any whitespace
	const char* t =str + len;
	for (; *str == ' ' || *str == '\t'; str++);
	for (; *(t-1) == ' ' || *(t-1) == '\t'; t--);
	return std::string(str, t-str);
}

char* RTSPSink::nextLine(char* startOfLine, int maxByte)
{
	// returns the start of the next line, or NULL if none.  Note that this modifies the input string to add '\0' characters.

	// locate the beginning of the line
	for (; (*startOfLine =='\0' || *startOfLine == '\n') && maxByte >0; startOfLine++, maxByte--);

	// locate the end of line
	char* ptr = startOfLine;
	for (; maxByte >0 && *ptr != '\r' && *ptr != '\n'; ++ptr, maxByte--);

	if (maxByte<=0)
		return NULL;

	// change the "\r\n" as the string NULL
	if (*ptr == '\r')
		*ptr = '\0';

	return startOfLine;
}

bool RTSPSink::parseRequestLine(const std::string& line, std::string& cmdName, std::string& url, std::string& proto)
{
	char* buf = new char[line.length()+2];
	strcpy(buf, line.c_str());
	bool bSucc =false;

	do {
		char* p = buf, *q=p;
		while (*p == ' ' || *p == '\t')	p++; // skip the leading white spaces
		for (q=p; *q && *q!= ' ' && *q != '\t'; q++); // find the next white spaces

		if (*q)
			*q++ = '\0';

		cmdName = trim(p);
		if (NULL == (p = strstr(q, "RTSP/")))
			break; // failed

		*(p-1) = '\0';

		url = trim(q);
		proto = trim(p);

		bSucc = true;

	} while(0);

	delete [] buf;
	return bSucc;
}

bool RTSPSink::parseResponseLine(const std::string& line, uint& resultCode, std::string& resultStr, std::string& proto)
{
	char* buf = new char[line.length()+2];
	strcpy(buf, line.c_str());
	bool bSucc =false;

	do {
		char* p = buf, *q=p;

		if (NULL == (p = strstr(q, "RTSP/")))
			break; // failed

		while (*p == ' ' || *p == '\t')	p++; // skip the leading white spaces
		for (q=p; *q && *q!= ' ' && *q != '\t'; q++); // find the next white spaces

		std::string temp(p, q-p);
		proto = trim(temp.c_str());

		if (*q)
			*q++ = '\0';

		for (p=q; *p && *p!= ' ' && *p != '\t'; p++); // find the next white spaces

		temp = std::string(q, p-q);
		resultCode=atoi(temp.c_str());
		if (resultCode < 100) // result code must be 3-digits
			break;

		resultStr = trim(p);
		bSucc = true;

	} while(0);

	delete [] buf;
	return bSucc;
}

#define RTSP_NPT_NOW (-321.00)

static bool parseNPTTime(char* nptTime, double& npttime)
{
// [rfc2326] 3.6 Normal Play Time
//   npt-time     =   "now" | npt-sec | npt-hhmmss
//   npt-sec      =   1*DIGIT [ "." *DIGIT ]
//   npt-hhmmss   =   npt-hh ":" npt-mm ":" npt-ss [ "." *DIGIT ]
//   npt-hh       =   1*DIGIT     ; any positive number
//   npt-mm       =   1*2DIGIT    ; 0-59
//   npt-ss       =   1*2DIGIT    ; 0-59

	char* pTail = nptTime + strlen(nptTime);
	for (; *nptTime == ' ' || *nptTime == '\t'; nptTime++);
	for (; pTail > nptTime && (*(pTail-1) == ' ' || *(pTail-1) == '\t'); pTail--);
	*pTail = '\0';

	if (strlen(nptTime) <=0)
		return false;

	if (0 == strcmp(nptTime, "now"))
	{
		npttime = 0.0; // TODO: replaced with RTSP_NPT_NOW
		return true;
	}

	if (strchr(nptTime, ':') !=NULL)
	{
		int hh=0, mm=0, ss=0, msec=0;
		int ret = sscanf(nptTime, "%d:%2d:%2d.%3d", &hh, &mm, &ss, &msec);
		if (ret <3)
			return false;

		npttime = (hh*60 + mm) *60 +ss;

		if (ret >3)
		{
			if (msec<10)
				npttime += (float)msec/10;
			else if (msec<100)
				npttime += (float)msec/100;
			else 
				npttime += (float)msec/1000;
		}

		return true;
	}

	if (sscanf(nptTime, "%lf", &npttime) >0)
		return true;

	return false;
}

static bool parseNPTRange(char* str, double& rangeStart, double& rangeEnd)
{
// [rfc2326] 3.6 Normal Play Time
//   npt-range    =   ( npt-time "-" [ npt-time ] ) | ( "-" npt-time )
//   Examples:
//     npt=123.45-125
//     npt=12:05:35.3-
//     npt=now-

	char* pos = strchr(str, '-');
	if (NULL == pos)
		return false;
	*pos = '\0';

	rangeStart = rangeEnd = 0.0;
	bool ret1 = parseNPTTime(str,   rangeStart);
	bool ret2 = parseNPTTime(pos+1, rangeEnd);
	if (!ret1 && !ret2)
		return false;

	return true;
}

/*
bool RTSPSink::isNowNPT(double& npt)
{
	return abs(npt - RTSP_NPT_NOW) < 0.1;
}
*/
bool RTSPSink::parseRangeParam(char const* paramStr, double& rangeStart, double& rangeEnd)
{
	char str[128] = "";
	strncpy(str, paramStr, sizeof(str) -2);

	char* posEq = strchr(str, '=');
	if (NULL == posEq)
		return false;
	*posEq = '\0';

	return parseNPTRange(posEq+1, rangeStart, rangeEnd);
}


static int errorno()
{
#ifdef ZQ_OS_MSWIN
		return WSAGetLastError();
#else
		return errno;
#endif
}

#define ISSUECMD(CMDTYPE, CMDARGS) 		try { CMDTYPE* c=NULL; if (NULL != (c = new CMDTYPE CMDARGS)) c->start(); } catch(...) {}

// -----------------------------
// class MessageProcessCmd
// -----------------------------
class MessageProcessCmd : public ThreadRequest
{
public:
	typedef struct _ReqRespPair
	{
		uint cSeq;
		RTSPMessage::Ptr inMsg;
		RTSPRequest::Ptr  outReq;

		static bool less(_ReqRespPair i, _ReqRespPair j) { return (i.cSeq<j.cSeq); }
	} ReqRespPair;

	typedef std::vector <ReqRespPair> MsgPairs;

	MessageProcessCmd(RTSPClient& client, const ReqRespPair& pair)
		:ThreadRequest(client._thrdpool), _client(client), _pair(pair)
	{
		ThreadRequest::setPriority(DEFAULT_REQUEST_PRIO -1); // make the priority of this ThreadRequest higher than normal
	}

protected:
	RTSPClient& _client;
	ReqRespPair _pair;

	virtual int run()
	{
		std::string cmdName, url, proto, resultStr;
		uint resultCode =0;
		int64 stampNow = TimeUtil::now();

		try {
			if (!_pair.inMsg)
				return -1;

			RTSPSession::Ptr pSession;

			if (_pair.outReq)
			{
				// a matched response of request
				if (!RTSPSink::parseResponseLine(_pair.inMsg->startLine, resultCode, resultStr, proto))
				{
					_client._log(Log::L_WARNING, CLOGFMT(MessageProcessCmd, "failed to parse the response startline[%s][%d]"), _pair.inMsg->startLine.c_str(), _pair.cSeq);
					return -2;
				}

#ifdef _DEBUG
				_client._log(Log::L_DEBUG, CLOGFMT(MessageProcessCmd, "dispatching response of %s(%d): %s"), _pair.outReq->_commandName.c_str(), _pair.cSeq, _pair.inMsg->startLine.c_str());
#endif // _DEBUG
				pSession = RTSPSession::lookupBySessionGuid(_pair.outReq->_sessGuid.c_str());

				if (pSession)
				{
					MutexGuard sessg(pSession->_lockIncomming);

					RTSPMessage::AttrMap::iterator itHeader = _pair.inMsg->headers.find("Session");
					std::string sessId; int timeoutSec=0;
					if (_pair.inMsg->headers.end() != itHeader)
					{
						sessId = itHeader->second;
						// check if the server specifies the timeout of session
						// rfc2326: section 12.37: "Session" ":" session-id [ ";" "timeout" "=" delta-seconds ]
						size_t pos_timeout = sessId.find(';');
						if (::std::string::npos != pos_timeout)
						{
							sessId = RTSPSink::trim(sessId.substr(0, pos_timeout).c_str());
							pos_timeout = itHeader->second.find("timeout", pos_timeout);
							if (::std::string::npos != pos_timeout && ::std::string::npos != (pos_timeout = itHeader->second.find('=', pos_timeout)))
								timeoutSec = atoi(itHeader->second.substr(pos_timeout +1).c_str());
						}
					}

					if (0 == _pair.outReq->_commandName.compare("SETUP"))
					{
						if ((resultCode >=200 && resultCode <300) && !sessId.empty())
						{
							// a succeeded SETUP
							pSession->_sessionId = sessId;

							if (timeoutSec > 0)
								pSession->_sessTimeout = timeoutSec *1000;
						}
					}
					else
					{
						// non SETUPs
						if (0 != sessId.compare(pSession->_sessionId))
							_client._log(Log::L_WARNING, CLOGFMT(MessageProcessCmd, "session[%s,%s] %s(%d) got unmatched sessionId[%s] in response: %s"), 
								pSession->_sessGuid.c_str(), pSession->_sessionId.c_str(), _pair.outReq->_commandName.c_str(), _pair.cSeq, sessId.c_str(), _pair.inMsg->startLine.c_str());
					}

					pSession->_stampLastMessage = stampNow;
					pSession->OnResponse(_client, _pair.outReq, _pair.inMsg, resultCode, resultStr.c_str());
				}
				else
					_client.OnResponse(_client, _pair.outReq, _pair.inMsg, resultCode, resultStr.c_str());

				long latencyMessage =(long) (_pair.inMsg->stampCreated - _pair.outReq->stampCreated);
				long latencyDispatch =(long) (stampNow - _pair.inMsg->stampCreated);
				long latencyCreated =(long) (stampNow - _pair.outReq->stampCreated);
				_client._log(Log::L_DEBUG, CLOGFMT(MessageProcessCmd, "dispatched response of %s(%d): %s; latencies [%d/%d/%d]msec"), _pair.outReq->_commandName.c_str(), _pair.cSeq, _pair.inMsg->startLine.c_str(), latencyMessage, latencyDispatch, latencyCreated);
				return 0;
			}

			// the incomming request
			if (!RTSPSink::parseRequestLine(_pair.inMsg->startLine, cmdName, url, proto))
			{
				_client._log(Log::L_WARNING, CLOGFMT(MessageProcessCmd, "failed to parse the request startline[%s][%d]"), _pair.inMsg->startLine.c_str(), _pair.cSeq);
				return -3;
			}

			RTSPMessage::AttrMap::iterator itHeader = _pair.inMsg->headers.find("Session");
			std::string sessId;
			if (_pair.inMsg->headers.end() != itHeader)
			{
				sessId = itHeader->second;

				// check if the server specifies the timeout of session
				// rfc2326: section 12.37: "Session" ":" session-id [ ";" "timeout" "=" delta-seconds ]
				size_t pos_timeout = sessId.find(';');
				if (::std::string::npos != pos_timeout)
					sessId = RTSPSink::trim(sessId.substr(0, pos_timeout).c_str());
			}

#ifdef _DEBUG
			_client._log(Log::L_DEBUG, CLOGFMT(MessageProcessCmd, "dispatching ServerRequest %s(%d) sess[%s]"), cmdName.c_str(), _pair.cSeq, sessId.c_str());
#endif // _DEBUG
			if (!sessId.empty())
			{
				// session-oriented ServerRequest
				RTSPSession::List list = RTSPSession::lookupBySessionId(sessId.c_str());
				if (list.size() >0)
				{
					// guess the matched RTSPSession if there are multiple matched RTSP session Ids
					_client._log(Log::L_DEBUG, CLOGFMT(MessageProcessCmd, "found %d RTSP session[%s] for the ServerRequest[%s][%d]"), list.size(), itHeader->second.c_str(), _pair.inMsg->startLine.c_str(), _pair.cSeq);
					pSession = list[0];
					for (size_t i=0; i< list.size(); i++)
					{
						if (!list[i])
							continue;
						if (0 == url.compare(list[i]->_theURL) || 0 == url.compare(list[i]->_controlUri))
							pSession = list[i];
					}
				}
				else 
					_client._log(Log::L_DEBUG, CLOGFMT(MessageProcessCmd, "ServerRequest %s(%d) failed to find sess[%s], dispatch to the RTSPClient"), cmdName.c_str(), _pair.cSeq, sessId.c_str());
			}

			if (pSession)
			{
				MutexGuard sessg(pSession->_lockIncomming);

				pSession->_stampLastMessage = stampNow;
				pSession->OnServerRequest(_client, cmdName.c_str(), url.c_str(), _pair.inMsg);
			}
			else
			{
				// non-session-oriented ServerRequest
				_client.OnServerRequest(_client, cmdName.c_str(), url.c_str(), _pair.inMsg);
			}

			long latencyDispatch =(long) (stampNow - _pair.inMsg->stampCreated);
			_client._log(Log::L_DEBUG, CLOGFMT(MessageProcessCmd, "dispatched ServerRequest %s(%d) sess[%s], latency %dmsec"), cmdName.c_str(), _pair.cSeq, sessId.c_str(), latencyDispatch);
			return 0;
		}
		catch(...) {}

		return -5;
	}

	void final(int retcode =0, bool bCancelled =false)
	{
		delete this;
	}
};

// -----------------------------
// class RequestErrCmd
// -----------------------------
class RequestErrCmd : public ThreadRequest
{
public:
	RequestErrCmd(RTSPClient& client, const RTSPRequest::Ptr& req, RTSPSink::RequestError errCode, const char* errDesc=NULL)
		:ThreadRequest(client._thrdpool), _client(client), _req(req), _errCode(errCode)
	{
		if (errDesc)
			_errDesc = errDesc;
	}

protected:
	RTSPClient& _client;
	RTSPRequest::Ptr _req;
	RTSPSink::RequestError _errCode;
	std::string _errDesc;

	virtual int run()
	{
		try {
			RTSPSession::Ptr pSession = RTSPSession::lookupBySessionGuid(_req->_sessGuid.c_str());

			if (pSession)
				pSession->OnRequestError(_client, _req, _errCode, _errDesc.c_str());
			else
				_client.OnRequestError(_client, _req, _errCode, _errDesc.c_str());
		}
		catch(...) {}

		return 0;
	}

	void final(int retcode =0, bool bCancelled =false)
	{
		delete this;
	}
};

// -----------------------------
// class RTSPClient
// -----------------------------
uint16 RTSPClient::_verboseFlags =0;
void RTSPClient::setVerboseFlags(uint16 flags)
{
	_verboseFlags = flags; 
}

RTSPClient::RTSPClient(Log& log, NativeThreadPool& thrdpool, InetHostAddress& bindAddress, const std::string& baseURL, const char* userAgent, Log::loglevel_t verbosityLevel, tpport_t bindPort)
: TCPClient(bindAddress, bindPort), _thrdpool(thrdpool), _log(log, verbosityLevel), 
  _baseURL(trim(baseURL.c_str())), _cTcpStreams(0), _pCurrentMsg(NULL), _inCommingByteSeen(0)
// , _bindAddress(bindAddress), _bindPort(bindPort), 
{
	_userAgent = userAgent ? userAgent: "SeaChangeRTSPClient";
	_lastCSeq.set(1);
	_sendErrorCount.set(0);
	setClientTimeout(DEFAULT_CONNECT_TIMEOUT, DEFAULT_CLIENT_TIMEOUT);

	do {
		// step 1. parse the URL for the IP and port
		ZQ::common::URLStr url(_baseURL.c_str());
		if (!_serverAddress.setAddress(url.getHost()))
			break;

		_serverPort = url.getPort();
		if (_serverPort <=0)
			_serverPort = 554;

		setPeer(_serverAddress, _serverPort);

	} while(0);
}

RTSPClient::~RTSPClient()
{
}

uint RTSPClient::lastCSeq()
{
	int v = _lastCSeq.add(1);
	if (v>0 && v < MAX_CLIENT_CSEQ)
		return (uint) v;

	static Mutex lock;
	MutexGuard g(lock);
	v = _lastCSeq.add(1);
	if (v >0 && v < MAX_CLIENT_CSEQ)
		return (uint) v;

	_lastCSeq.set(1);
	v = _lastCSeq.add(1);

	return (uint) v;
}

int RTSPClient::sendDESCRIBE(Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "DESCRIBE"), headerToOverride);
}

int RTSPClient::sendOPTIONS(Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "OPTIONS"), headerToOverride);
}

int RTSPClient::sendANNOUNCE(const char* sdpDescription, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "ANNOUNCE", NULL, 0, 0.0, 0.0, 0.0, sdpDescription), headerToOverride);
}

int RTSPClient::sendSET_PARAMETER(const RTSPRequest::AttrMap& paramMap, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	char paramString[1024] = "\0", *p = paramString;

	for (RTSPRequest::AttrMap::const_iterator it = paramMap.begin(); it != paramMap.end(); it++)
	{
		snprintf(p, paramString + sizeof(paramString)-2 -p, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
		p += strlen(p);
	}

	return sendRequest(new RTSPRequest(*this, lastCSeq(), "SET_PARAMETER", NULL, 0, 0.0, 0.0, 0.0, paramString), headerToOverride);
}

int RTSPClient::sendGET_PARAMETER(const RTSPRequest::AttrList& paramNames, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	char paramString[1024] = "\0", *p = paramString;

	for (RTSPRequest::AttrList::const_iterator it = paramNames.begin(); it < paramNames.end(); it++)
	{
		snprintf(p, paramString + sizeof(paramString)-2 -p, "%s ", it->c_str());
		p += strlen(p);
	}

	snprintf(p, paramString + sizeof(paramString)-2 -p, "\r\n");

	return sendRequest(new RTSPRequest(*this, lastCSeq(), "GET_PARAMETER", NULL, 0, 0.0, 0.0, 0.0, paramString), headerToOverride);
}

int RTSPClient::sendPLAY(RTSPSession& session, double start, double end, float scale, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	session._stampLastMessage = ZQ::common::now();
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "PLAY", &session, 0, start, end, scale), headerToOverride);
}

int RTSPClient::sendPAUSE(RTSPSession& session, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	session._stampLastMessage = ZQ::common::now();
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "PAUSE", &session), headerToOverride);
}

int RTSPClient::sendRECORD(RTSPSession& session, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	session._stampLastMessage = ZQ::common::now();
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "RECORD", &session), headerToOverride);
}

int RTSPClient::sendTEARDOWN(RTSPSession& session, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	session._stampLastMessage = ZQ::common::now();
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "TEARDOWN", &session), headerToOverride);
}

int RTSPClient::sendSET_PARAMETER(RTSPSession& session, const RTSPRequest::AttrMap& paramMap, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	char paramString[1024] = "\0", *p = paramString;

	for (RTSPRequest::AttrMap::const_iterator it = paramMap.begin(); it != paramMap.end(); it++)
	{
		snprintf(p, paramString + sizeof(paramString)-2 -p, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
		p += strlen(p);
	}

	session._stampLastMessage = ZQ::common::now();
	int result = sendRequest(new RTSPRequest(*this, lastCSeq(), "SET_PARAMETER", &session, 0, 0.0, 0.0, 0.0, paramString), headerToOverride);
	return result;
}

int RTSPClient::sendGET_PARAMETER(RTSPSession& session, const RTSPRequest::AttrList& paramNames, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	char paramString[1024] = "\0", *p = paramString;

	for (RTSPRequest::AttrList::const_iterator it = paramNames.begin(); it < paramNames.end(); it++)
	{
		snprintf(p, paramString + sizeof(paramString)-2 -p, "%s ", it->c_str());
		p += strlen(p);
	}

	snprintf(p, paramString + sizeof(paramString)-2 -p, "\r\n");

	session._stampLastMessage = ZQ::common::now();
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "GET_PARAMETER", &session, 0, 0.0, 0.0, 0.0, paramString), headerToOverride);
}

int RTSPClient::sendSETUP(RTSPSession& session, const char* SDP, Authenticator* authenticator, const RTSPMessage::AttrMap& headerToOverride)
{
	std::string sdpDesc = SDP ? SDP : session.sessionDescription(true, _serverType.c_str());

	session._stampLastMessage = ZQ::common::now();
	return sendRequest(new RTSPRequest(*this, lastCSeq(), "SETUP", &session, 0, session._maxPlayStartTime, session._maxPlayEndTime, 0.0, sdpDesc.c_str()), headerToOverride);
}

int RTSPClient::sendRequest(RTSPRequest::Ptr pRequest, const RTSPMessage::AttrMap& headerToOverride)
{
	if (!pRequest || pRequest->cSeq<=0)
	{
		_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() NULL request or negative cseq passed in"));
		return -2;
	}

	// step 3.1 a callback to notify an out-going request
	OnRequestPrepare(pRequest); 

	// int withdrawCode = 
	// if (0 != withdrawCode)
	// {
	//	_log(Log::L_WARNING, CLOGFMT(RTSPClient, "sendRequest() request[%s] client withdraw the out-going request: %d"), reqDesc.c_str(), withdrawCode);
	//	return -3;
	// }

	RTSPRequest::AttrMap headerMap = headerToOverride; // copy from what of the request
	char buf[8196];

	RTSPSink::RequestError lastErrCode = RTSPSink::Err_SendFailed;
	std::string lastErr;

	RTSPSession::Ptr pSession = RTSPSession::lookupBySessionGuid(pRequest->_sessGuid.c_str());

	// initialize the URL
	std::string cmdURL = _baseURL;
	if (pSession && !pSession->_filePath.empty())
	{
		// fix up the cmdURL
		if ('/' ==  pSession->_filePath[0])
		{
			// start from the root of the server
			ZQ::common::URLStr url(_baseURL.c_str());
			snprintf(buf, sizeof(buf)-2, "%s://%s:%d%s", url.getProtocol(), url.getHost(), _serverPort, pSession->_filePath.c_str());
			cmdURL = buf;
		}
		else
		{
			// take the _baseURL as the base folder
			if ('/' !=  cmdURL[cmdURL.length()-1])
				cmdURL += "/";

			cmdURL += pSession->_filePath;
		}
	}

	for (bool dummyLoop = true; dummyLoop; dummyLoop =false) // only one round of this loop, program can quit by either "break" or "continue"
	{
		bool connectionIsPending = false;

		{
			MutexGuard g(_lockQueueToSend); // borrow _lockQueueToSend to avoid thread-unsafe at non-blocking TCPClient::connect()

			if (_requestsQueueToSend.size() >0 || (Socket::stConnecting == TCPClient::state()))
			{
				// a connection is currently pending with at least one queued request.
				connectionIsPending = true;
			}
			else if (_so < 0 || _so == INVALID_SOCKET)
			{ 
				// need to open a connection
				if (!TCPClient::connect(_connectTimeout))
				{
					lastErrCode = Err_ConnectError;
					lastErr = "error occured at connect()";
					_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() req[%s(%d)] %s"), pRequest->_commandName.c_str(), pRequest->cSeq, lastErr.c_str());
					break; // an error occurred
				}

				connectionIsPending = (Socket::stConnecting == TCPClient::state());
			}

			if (connectionIsPending)
			{
				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "sendRequest() req[%s(%d)] connect in progress, wait for next try"), pRequest->_commandName.c_str(), pRequest->cSeq);
				// flush the headerToOverride into headers of the request
				for (RTSPMessage::AttrMap::const_iterator it = headerToOverride.begin(); it != headerToOverride.end(); it++)
				{
					if (it->first.empty())
						continue;

					MAPSET(RTSPMessage::AttrMap, pRequest->headers, it->first, it->second);
				}

				_requestsQueueToSend.push(pRequest);

				// give up those expired requests in the queue and prevent this queue from growing too big
				int64 stampExp = ZQ::common::now() - _timeout;
				while (!_requestsQueueToSend.empty())
				{
					RTSPRequest::Ptr pReq = _requestsQueueToSend.front();

					if (pReq && (_timeout <=0 || pReq->stampCreated > stampExp) && _requestsQueueToSend.size() < MAX_PENDING_REQUESTS)
						break;

					_requestsQueueToSend.pop();

					if (!pReq)
						continue;

					ISSUECMD(RequestErrCmd, (*this, pReq, RTSPSink::Err_ConnectError));
				}

				return pRequest->cSeq;
			}
		}

		snprintf(buf, sizeof(buf)-2, "req[%s(%d)@%s]", pRequest->_commandName.c_str(), pRequest->cSeq, connDescription());
		std::string reqDesc = buf; 
		snprintf(buf, sizeof(buf)-2, "req[%s(%d)@%08x]", pRequest->_commandName.c_str(), pRequest->cSeq, TCPSocket::get());
		std::string reqDescShort = buf; 

		// step 1. adjust some headers per request type
		if (0 == pRequest->_commandName.compare("DESCRIBE"))
		{
			MAPSET(RTSPRequest::AttrMap, headerMap, "Accept", "application/sdp");
		} 
		else if (0 == pRequest->_commandName.compare("OPTIONS"))
		{
		} 
		else if (0 == pRequest->_commandName.compare("ANNOUNCE"))
		{
			MAPSET(RTSPRequest::AttrMap, headerMap, "Content-Type", "application/sdp");
		} 
		else if (0 == pRequest->_commandName.compare("SETUP"))
		{
			if (!pSession)
			{
				lastErrCode = Err_InvalidParams;
				lastErr = "no RTSPSession record was attached for SETUP";
				_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s %s"), reqDesc.c_str(), lastErr.c_str());
				break;
			}

			pSession->_theURL = pSession->_controlUri = cmdURL; // initialize the control URI by cmdURL

//			bool streamUsingTCP = (pRequest->_flags & 0x1) != 0;
			bool streamOutgoing = (pRequest->_flags & 0x2) != 0;
//			bool forceMulticastOnUnspecified = (pRequest->_flags &0x4) != 0;

			// generate the transport statement
			URLStr url(pSession->_tpStreamDestUrl.c_str());
			std::string protocal = url.getProtocol();
			transform(protocal.begin(), protocal.end(), protocal.begin(), tolower);
			if ("udp" == protocal)
				pSession->_tpType = RTSPSession::tpt_MP2T_AVP_UDP;
			else if ("rawudp" == protocal)
				pSession->_tpType = RTSPSession::tpt_RAW_RAW_UDP;
			else if("rtpudp" == protocal)
			{
				pSession->_tpRTPPort =  ++_cTcpStreams;
				pSession->_tpRTCPPort = ++_cTcpStreams;
				pSession->_tpType = RTSPSession::tpt_RTP_AVP;
			}
			else if("rtp" == protocal)
			{
				pSession->_tpRTPPort =  ++_cTcpStreams;
				pSession->_tpRTCPPort = ++_cTcpStreams;
				pSession->_tpType = RTSPSession::tpt_RTP_AVP_TCP;
			}

#pragma message ( __MSGLOC__ "TODO: replace Transport preparation with pSession->getTransport()")
			if(headerMap.find("Transport") == headerMap.end()) // if user specify Transport, skip it
			{
				std::string transport = pSession->getTransport(true, _serverType.c_str(), streamOutgoing);
				if (transport.empty())
				{
					lastErrCode = Err_InvalidParams;
					lastErr = "failed to determine transport";
					_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s destination[%s] %s"), reqDesc.c_str(), pSession->_tpStreamDestUrl.c_str(), lastErr.c_str());
					break;
				}
				else MAPSET(RTSPRequest::AttrMap, headerMap, "Transport", transport);
			}

		} // end of SETUP
		else
		{ 
			// "PLAY", "PAUSE", "TEARDOWN", "RECORD", "SET_PARAMETER", "GET_PARAMETER"
			if (!pSession)
			{
				if (0 != pRequest->_commandName.compare("SET_PARAMETER") && 0 != pRequest->_commandName.compare("GET_PARAMETER"))
				{
					lastErrCode = Err_InvalidParams;
					lastErr = "no RTSPSession record was attached";
					_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s %s"), reqDesc.c_str(), lastErr.c_str());
					break;
				}

#pragma message ( __MSGLOC__ "TODO: non-session-oriented SET_PARAMETER or GET_PARAMETER here")
			}
			else
			{
				// session-oriented "PLAY", "PAUSE", "TEARDOWN", "RECORD", "SET_PARAMETER", "GET_PARAMETER"
				if (pSession->_sessionId.empty())
				{
					lastErrCode = Err_InvalidParams;
					lastErr = "RTSPSession has no sessionId";
					_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s %s"), reqDesc.c_str(), lastErr.c_str());
					break;
				}

				MAPSET(RTSPRequest::AttrMap, headerMap, "Session", pSession->_sessionId);
				
				// initialize cmdURL with the _baseURL of the client, then adjust per _controlUri for non-TEARDOWN
				if (0 != pRequest->_commandName.compare("TEARDOWN") && !pSession->_controlUri.empty())
						cmdURL = pSession->_controlUri;

				float originalScale = pSession->scale();

				if (0 == pRequest->_commandName.compare("PLAY"))
				{
					// header Scale
					if (originalScale != 1.0f || originalScale != pRequest->_scale)
					{
						snprintf(buf, sizeof(buf) -2, "%f", pRequest->_scale);
						MAPSET(RTSPRequest::AttrMap, headerMap, "Scale", buf);
					}

					// header Range
					if (pRequest->_startPos == 0 && pRequest->_endPos ==0); // no range specified
					else
					{
						if (pRequest->_startPos < 0)
							snprintf(buf, sizeof(buf)-2, "npt=now-");
						else 
							snprintf(buf, sizeof(buf)-2, "npt=%.3f-", pRequest->_startPos);

						char* p = buf + strlen(buf);

						if (pRequest->_endPos > 0)
							snprintf(p, sizeof(buf) + buf -p -2, "%.3f", pRequest->_endPos);

						MAPSET(RTSPRequest::AttrMap, headerMap, "Range", buf);
					}
				} // end of PLAY
			}
		}

		// step 3. start sending the request message now
		snprintf(buf, sizeof(buf)-2, "%s %s %s",  pRequest->_commandName.c_str(), cmdURL.c_str(), "RTSP/1.0");
		pRequest->startLine = buf;

		size_t awaitsize = 0, poolsize = _thrdpool.size();
		int64 stampNow = TimeUtil::now();

		if (_messageTimeout >0 && stampNow - pRequest->stampCreated > _messageTimeout)
		{
			_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s timeout prior to sending"), reqDesc.c_str());
			ISSUECMD(RequestErrCmd, (*this, pRequest, lastErrCode));
		}
		else
		{
			MutexGuard g(_lockAwaitResponse);
			int ret = sendMessage(RTSPMessage::Ptr::dynamicCast(pRequest), headerMap, reqDescShort.c_str());

			if (ret < 0)
			{
				lastErr = "failed at sendMessage()";
				_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s %s, ret=%d"), reqDesc.c_str(), lastErr.c_str(), ret);
				break;
			}

			stampNow = TimeUtil::now();
			long sendLatency = (long) (stampNow - pRequest->stampCreated);

			if (_messageTimeout >20 && sendLatency > _messageTimeout/4)
				_log(Log::L_WARNING, CLOGFMT(RTSPClient, "sendRequest() %s took %dmsec, too long until sent"), reqDesc.c_str(), sendLatency);

			if (pSession)
				pSession->_stampLastMessage = stampNow;
			// The command send succeeded, so enqueue the request record, so that its response (when it comes) can be handled:

			//			_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "%s MAPSET AwaitResponse [%d, %s]"), connDescription(), pRequest->cSeq, pRequest->startLine.c_str());
			MAPSET(CSeqToRTSPRequestMap, _requestsAwaitResponse, pRequest->cSeq, pRequest);
			awaitsize = _requestsAwaitResponse.size();
		}

		if (awaitsize > poolsize *10)
		{
			static int64 stampLast = stampNow;

			if (stampNow > stampLast + _messageTimeout)
			{
				stampLast = stampNow; // update the stampLast immediately to avoid other send enter this too

				_cleanupExpiredAwaitRequests(3, "sendRequest");
				if (awaitsize > poolsize *20) // if still failed to compress the AwaitRequests print a warning
					_log(Log::L_WARNING, CLOGFMT(RTSPClient, "sendRequest() client[%s] %d requests await responses, too many according to threadpool[%d], pending [%d] requests"), connDescription(), awaitsize, poolsize, _thrdpool.pendingRequestSize());

				// yield the CPU
#ifdef ZQ_OS_MSWIN
				Sleep(1);
#else
				usleep(100);
#endif
			}
		}

		return pRequest->cSeq;

	} // end of the dummyLoop

	// An error occurred, so call the response handler immediately (indicating the error)
	ISSUECMD(RequestErrCmd, (*this, pRequest, lastErrCode));
	pRequest = NULL;

	return -5;
}

int RTSPClient::sendMessage(RTSPMessage::Ptr pMessage, const RTSPMessage::AttrMap& headerToOverride, const char* msgDesc)
{
	if (!pMessage || pMessage->startLine.empty() || pMessage->cSeq <=0)
		return -1;

	char buf[RTSP_MSG_BUF_SIZE];
	// step 1. copy the header of message
	RTSPMessage::AttrMap outgoingHeaders = pMessage->headers;
	
	// step 2. fillin the common headers
	MAPSET(RTSPMessage::AttrMap, outgoingHeaders, "User-Agent", _userAgent);

	// step 3. overwrite the headers if specified
	for (RTSPMessage::AttrMap::const_iterator it = headerToOverride.begin(); it != headerToOverride.end(); it++)
	{
		if (it->first.empty())
			continue;

		MAPSET(RTSPMessage::AttrMap, outgoingHeaders, it->first, it->second);
	}

	// step 4. format the out-going message
	char* p =buf, *tail=buf+sizeof(buf) -2;
	snprintf(p, tail -p, "%s\r\n", pMessage->startLine.c_str()); p+=strlen(p);
	snprintf(p, tail -p, "CSeq: %d\r\n", pMessage->cSeq); p+=strlen(p);

	for (RTSPRequest::AttrMap::iterator itOut = outgoingHeaders.begin(); itOut != outgoingHeaders.end(); itOut ++)
	{
		std::string headerName=itOut->first;
		size_t posPonSign = headerName.find('#');
		if (std::string::npos != posPonSign)
			headerName = headerName.substr(0, posPonSign); 
		snprintf(p, tail -p, "%s: %s\r\n",  headerName.c_str(), itOut->second.c_str()); p+=strlen(p);
	}

	int contentLen = pMessage->contentBody.length();
	snprintf(p, tail -p, "Content-Length: %d\r\n", contentLen); p+=strlen(p);

	{
		// stamp the header Date
		static const char* daysOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", ""};
		static const char* namesOfMon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", ""};
#ifdef ZQ_OS_MSWIN
		SYSTEMTIME time;
		::GetSystemTime(&time);
		snprintf(p, tail -p, "Date: %s, %02d %s %04d %02d:%02d:%02d GMT\r\n",
			daysOfWeek[time.wDayOfWeek % (sizeof(daysOfWeek) / sizeof(const char*))],          // 0=sun
			time.wDay, namesOfMon[(time.wMonth-1) % (sizeof(namesOfMon) / sizeof(const char*))], // ms724950, 1=Jan
			time.wYear, time.wHour, time.wMinute, time.wSecond); 
#else
		time_t now;
		struct tm time;
		gmtime_r(&now, &time);
		snprintf(p, tail-p, "Date: %s, %02d %s %04d %02d:%02d:%02d GMT\r\n",
			daysOfWeek[time.tm_wday % (sizeof(daysOfWeek) / sizeof(const char*))],
			time.tm_wday, 
			namesOfMon[time.tm_mon % (sizeof(namesOfMon) / sizeof(const char*))],
			time.tm_year, time.tm_hour, time.tm_min, time.tm_sec);
#endif

		p+=strlen(p);
	}

	snprintf(p, tail -p, "\r\n"); p+=strlen(p); // a black line

	if (tail -p < contentLen)
	{
		_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendMessage() %s too large to send"), msgDesc? msgDesc: "");
		return -2;
	}
	// must use memcpy instead of strcpy or sprintf here, do not move p
	memcpy(p, pMessage->contentBody.c_str(), contentLen);

	_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "sendMessage() conn[%s] sending %s"), connDescription(), msgDesc? msgDesc: "");
	int msgSize = p-buf + contentLen;
	int ret = TCPClient::send(buf, msgSize);

	if (ret < 0)
	{
		if (ret == SOCKET_ERROR)
		{
			int errnum = SYS::getLastErr(SYS::SOCK);
			std::string errMsg = SYS::getErrorMessage(SYS::SOCK);
			_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s caught socket-err(%d)%s"),
				msgDesc? msgDesc: "", errnum, errMsg.c_str());

#ifdef ZQ_OS_MSWIN
			if (errnum == WSAEWOULDBLOCK)// retry send until reach 5
#else
			if (errnum == EWOULDBLOCK) 
#endif
			{
				int retRetry   =-1;
				int retryTimes = 0;

				for (; retryTimes<5 && retRetry<0; retryTimes++)
				{
					SYS::sleep(200);
					retRetry = TCPClient::send(buf, msgSize);
				}

				if (retRetry >=0)
				{
					_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "sendRequest() %s succeeded after %d retrie(s) per EWOULDBLOCK"),
						msgDesc? msgDesc: "", retryTimes);
					return 1;
				}
				else
				{
					_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s retry failed"), msgDesc? msgDesc: "");
				}
			}
		}

		// send failed, if it reaches MAX_SEND_ERROR_COUNT, close current connection
		// MutexGuard g(_lockQueueToSend); // borrow _lockQueueToSend to avoid thread-unsafe at non-blocking TCPClient::connect()
		int v = _sendErrorCount.add(1);
		if(v > MAX_SEND_ERROR_COUNT)
			OnError();
			
		_log(Log::L_ERROR, CLOGFMT(RTSPClient, "sendRequest() %s failed at socket send()"), msgDesc? msgDesc: "");
		return -3;
	}

	char sockdesc[100];
	if (NULL == msgDesc || 0x00 == msgDesc[0])
		snprintf(sockdesc, sizeof(sockdesc)-2, CLOGFMT(RTSPClient, "sendMessage() conn[%08x]"), (uint) TCPSocket::get());
	else snprintf(sockdesc, sizeof(sockdesc)-2, CLOGFMT(RTSPClient, "sendMessage() %s"), msgDesc);

	if (RTSP_VERBOSEFLG_SEND_HEX & _verboseFlags)
		_log.hexDump(Log::L_DEBUG, buf, msgSize, sockdesc);
	else _log.hexDump(Log::L_INFO, buf, msgSize, sockdesc, true);

	return ret;
}

void RTSPClient::OnConnected()
{
	_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnConnected() connected to the peer, new conn[%s]"), connDescription());
	TCPSocket::setTimeout(_messageTimeout >>1); // half of _messageTimeout to wake up the socket sleep()
	_inCommingByteSeen =0; // reset _inCommingByteSeen to start from the beginning of the receive buffer

	ZQ::common::MutexGuard g(_lockQueueToSend);
	RequestQueue& pendingRequests = _requestsQueueToSend;

	// Resume sending all pending requests:
	int64 stampNow = TimeUtil::now();
	int cFlushed=0, cExpired=0;
	while (!pendingRequests.empty())
	{
		RTSPRequest::Ptr pReq = pendingRequests.front();
		pendingRequests.pop();
		if (!pReq)
			continue;

		try {
			if (pReq->stampCreated + _timeout < stampNow)
			{
				ISSUECMD(RequestErrCmd, (*this, pReq, RTSPSink::Err_RequestTimeout));
				cExpired++;
				continue;
			}

			sendRequest(pReq, pReq->headers);
			cFlushed++;
		}
		catch(...)
		{
			_log(Log::L_INFO, CLOGFMT(RTSPClient, "OnConnected() sendRequest catch ..."));
			break;
		}

		pReq = NULL;
	}

	if (pendingRequests.empty())
	{
		_log(Log::L_INFO, CLOGFMT(RTSPClient, "OnConnected() new conn[%s], %d pending request(s) reflushed and %d expired, took %lldmsec"), connDescription(), cFlushed, cExpired, TimeUtil::now() - stampNow);
		return; // successful exit point of the func
	}

	// an error occurred.  tell all pending requests about the error:
	_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnConnected() new conn[%s] established but failed to send, %d pending request(s) to cancel"), connDescription(), pendingRequests.size());
	while (!pendingRequests.empty())
	{
		RTSPRequest::Ptr pReq = pendingRequests.front();
		pendingRequests.pop();

		ISSUECMD(RequestErrCmd, (*this, pReq, RTSPSink::Err_SendFailed));
	}

	// close the socket per the error
	_log(Log::L_WARNING, CLOGFMT(RTSPClient, "OnConnected() new conn[%s] has problem, giving it up"), connDescription());
	disconnect();
}

void RTSPClient::OnDataArrived()
{
	struct sockaddr_in fromAddress;
	socklen_t addressSize = sizeof(fromAddress);

	MessageProcessCmd::MsgPairs tmpResponseList, pendingPeerRequests;

//	int bytesRead = recvfrom(_so, (char*) &_inCommingBuffer[_inCommingByteSeen], sizeof(_inCommingBuffer) - _inCommingByteSeen,
//		0,  (struct sockaddr*)&fromAddress, &addressSize);

	MutexGuard g(_lockInCommingMsg);

	int bytesToRead = sizeof(_inCommingBuffer) - _inCommingByteSeen;
	if (bytesToRead <=0)
	{
		_log(Log::L_WARNING, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] last incomplete message exceed bufsz[%d] from offset[%d], give it up"), connDescription(), sizeof(_inCommingBuffer), _inCommingByteSeen);
		_inCommingByteSeen =0;
		bytesToRead = sizeof(_inCommingBuffer) - _inCommingByteSeen;
	}

	int bytesRead = recv(_so, (char*) &_inCommingBuffer[_inCommingByteSeen], bytesToRead, 0);

	if (bytesRead <= 0)
	{
		int err = errorno();
#ifdef ZQ_OS_MSWIN
		if (WSAEWOULDBLOCK == err || WSAEINPROGRESS == err || WSAEALREADY == err)
#else
		if (EINPROGRESS == err)
#endif // ZQ_OS_MSWIN
			_log(Log::L_WARNING, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] recv() temporary fail[%d/%d], errno[%d]"), connDescription(), bytesRead, bytesToRead, err);
		else
		{
			_log(Log::L_ERROR, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] recv() failed[%d/%d], errno[%d]"), connDescription(), bytesRead, bytesToRead, err);
			OnError();
		}

		return;
	}

// 	if (0 == bytesRead)
// 	{
// 		_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] closed"), connDescription());
// 		disconnect();
// 		return;
// 	}

	_sendErrorCount.set(0); //current connection is normal, reset _sendErrorCount

	{
		char sockdesc[100];
		snprintf(sockdesc, sizeof(sockdesc)-2, CLOGFMT(RTSPClient, "OnDataArrived() conn[%08x]"), TCPSocket::get());

		if (RTSP_VERBOSEFLG_RECV_HEX & _verboseFlags)
			_log.hexDump(Log::L_DEBUG, &_inCommingBuffer[_inCommingByteSeen], bytesRead, sockdesc);
		else
			_log.hexDump(Log::L_INFO, &_inCommingBuffer[_inCommingByteSeen], bytesRead, sockdesc, true);
	}

	if (RTSP_VERBOSEFLG_TCPTHREADPOOL & _verboseFlags)
	{
		int poolSize, activeCount, pendingSize=0;
		if (TCPSocket::getThreadPoolStatus(poolSize, activeCount, pendingSize) && pendingSize>0)
			_log(pendingSize>50*poolSize? Log::L_WARNING :Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() TCP ThreadPool[%d/%d] pending [%d] requests"), activeCount, poolSize, pendingSize);
	}

	if (RTSP_VERBOSEFLG_THREADPOOL & _verboseFlags)
	{
		int poolSize, activeCount, pendingSize=0;
		poolSize    = _thrdpool.size();
		pendingSize = _thrdpool.pendingRequestSize();
		activeCount = _thrdpool.activeCount();

		if (pendingSize >0)
			_log(pendingSize>100? Log::L_WARNING :Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() client ThreadPool[%d/%d] pending [%d] requests"), activeCount, poolSize, pendingSize);
	}

	char* pProcessed = _inCommingBuffer, *pEnd = _inCommingBuffer + _inCommingByteSeen + bytesRead;
	bool bFinishedThisDataChuck = false;
	int64 stampNow = TimeUtil::now();
	while ((pProcessed < pEnd && !bFinishedThisDataChuck) || (_pCurrentMsg && _pCurrentMsg->headerCompleted && _pCurrentMsg->contentLenToRead==0))
	{
		if (!_pCurrentMsg)
			_pCurrentMsg = new RTSPMessage();

		if (_pCurrentMsg->headerCompleted)
		{
			// read the data as the content body of the current message
			// step 1. determin the length to read
			int64 len = 0;
			if (_pCurrentMsg->contentLenToRead >0) 
				len = _pCurrentMsg->contentLenToRead - _pCurrentMsg->contentBodyRead;

			if (len > pEnd - pProcessed)
				len = pEnd - pProcessed;

//			if (pEnd - pProcessed < len)
//			{
//				bFinishedThisDataChuck = true;
//				break;
//			}

			_pCurrentMsg->contentBody += std::string(pProcessed, (uint) len);
			pProcessed += len;
			_pCurrentMsg->contentBodyRead += len;

			if (_pCurrentMsg->contentBodyRead < _pCurrentMsg->contentLenToRead)
			{
				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] incompleted message left, appended[%lld], Content-Length[%lld/%lld]"), connDescription(), len, _pCurrentMsg->contentBodyRead, _pCurrentMsg->contentLenToRead);
				continue;
			}

			// the current message has been read completely when reach here
			RTSPRequest::AttrMap::iterator itHeader;

			if (_serverType.empty()) // record down the type of server
			{
				itHeader = _pCurrentMsg->headers.find("Server");
				if (_pCurrentMsg->headers.end() != itHeader)
					_serverType = itHeader->second;
			}

			// check the header CSeq
			itHeader = _pCurrentMsg->headers.find("CSeq");
			if (_pCurrentMsg->headers.end() == itHeader)
			{
				_log(Log::L_WARNING, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] ignore illegal response withno CSeq"), connDescription());
				_pCurrentMsg = new RTSPMessage();
				continue;
			}

			MessageProcessCmd::ReqRespPair pair;
			pair.cSeq =_pCurrentMsg->cSeq = atoi(itHeader->second.c_str());
			pair.inMsg = _pCurrentMsg;
			pair.inMsg->stampCreated = stampNow; // the _pCurrentMsg could be left from previous round, stamp the time again to be more accurate
			_pCurrentMsg = new RTSPMessage();

			if (pair.cSeq <=0)
			{
				_log(Log::L_WARNING, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] ignore in comming message with illegal CSeq(%d)"), connDescription(), pair.cSeq);
				continue;
			}

			std::string cmdName, url, proto;
			if (parseRequestLine(pair.inMsg->startLine, cmdName, url, proto))
				pendingPeerRequests.push_back(pair);
			else tmpResponseList.push_back(pair);

			continue;
		}

		// beginning of header reading
		while (!bFinishedThisDataChuck && pProcessed < pEnd)
		{
			char* line = nextLine(pProcessed, pEnd - pProcessed);
			if (NULL == line)
			{
				// met an incompleted line, shift it to the beginning of buffer then wait for the next OnDataArrived()
				bFinishedThisDataChuck = true;
				break;
			}

			int len = strlen(line);
			pProcessed += (len + 2); // skip /r/n

			if (len <=0) // an empty line
			{
				if (!_pCurrentMsg->startLine.empty())
				{
					_pCurrentMsg->headerCompleted = true;
					// finished this header reading
					break;
				}

				continue; // sounds like a bad line here
			}

			if (_pCurrentMsg->startLine.empty())
			{
				_pCurrentMsg->startLine = line;
//				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [%s]"), connDescription(), _pCurrentMsg->startLine.c_str());
				continue;
			}

			std::string header, value;
			char* pos = strchr(line, ':');
			if (NULL ==pos)
				continue; // illegal header

			*pos = '\0';
			header = trim(line);
			value = trim(pos+1);
			MAPSET(RTSPRequest::AttrMap, _pCurrentMsg->headers, header, value);
			if (0 == header.compare("Content-Length"))
				_pCurrentMsg->contentLenToRead = atol(value.c_str());
// 			else if(0 == header.compare("CSeq"))
// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [CSeq: %s]"), connDescription(), _pCurrentMsg->headers["CSeq"].c_str());
// 			else if(0 == header.compare("Method-Code"))
// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [Method-Code: %s]"), connDescription(), _pCurrentMsg->headers["Method-Code"].c_str());
// 			else if(0 == header.compare("Session"))
// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [Session: %s]"), connDescription(), _pCurrentMsg->headers["Session"].c_str());
		}; // end of header reading;

	} // end of current buffer reading

	// shift the unhandled buffer to the beginning, process with next OnData()
	Log::loglevel_t llevel = Log::L_DEBUG;
	if (_inCommingByteSeen>0 || pendingPeerRequests.size() + tmpResponseList.size() >1 || pEnd - pProcessed >0)
		llevel = Log::L_INFO;

	_log(llevel, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received %d bytes, appending to buf[%d], chopped out %d ServerRequests and %d Responses, %d incompleted bytes left"), connDescription(), bytesRead, _inCommingByteSeen, pendingPeerRequests.size(), tmpResponseList.size(), pEnd - pProcessed);
	if (pEnd >= pProcessed)
	{
		_inCommingByteSeen = pEnd - pProcessed;
		memcpy(_inCommingBuffer, pProcessed, _inCommingByteSeen);
	}

	// fire MessageProcessCmd for the chopped message
	::std::sort(tmpResponseList.begin(), tmpResponseList.end(), MessageProcessCmd::ReqRespPair::less);
	MessageProcessCmd::MsgPairs::iterator itStack = tmpResponseList.begin();
	MessageProcessCmd::MsgPairs pendingResponses, expiredRequests;
	std::string expCSeqStr;

	{
		MutexGuard g(_lockAwaitResponse);
		stampNow = TimeUtil::now();
		for (CSeqToRTSPRequestMap::iterator it = _requestsAwaitResponse.begin(); itStack < tmpResponseList.end() && it != _requestsAwaitResponse.end(); it++)
		{
			while (itStack < tmpResponseList.end() && itStack->cSeq < it->first)
			{
				// the in-coming message is not recoganized to link to the known request
				_log(Log::L_WARNING, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] ignore unrecoganized response CSeq(%d), it may have already expired"), connDescription(), itStack->cSeq);
				itStack++;
			}

			if (itStack != tmpResponseList.end() && itStack->cSeq == it->first)
			{
				// the matched response to the request
				itStack->outReq = it->second;
				pendingResponses.push_back(*itStack);
				itStack ++;
				continue;
			}

			// only exist in the _requestsAwaitResponse here, check if it has been expired
			if (it->second->stampCreated + _messageTimeout < stampNow)
			{
				MessageProcessCmd::ReqRespPair pair;
				pair.cSeq = it->second->cSeq;
				pair.outReq = it->second;
				expiredRequests.push_back(pair);
				char buf[20];
				snprintf(buf, sizeof(buf)-2, "%d ", pair.cSeq);
				expCSeqStr += buf;
			}
		}

		// tmpResponseList.clear();

		// clean up _requestsAwaitResponse per newPendingResponse and expiredRequests
		for (itStack = pendingResponses.begin(); itStack < pendingResponses.end(); itStack++)
		{
			_requestsAwaitResponse.erase(itStack->cSeq);
		}

		for (itStack = expiredRequests.begin(); itStack < expiredRequests.end(); itStack++)
		{
			_requestsAwaitResponse.erase(itStack->cSeq);
		}
	}

	// now issue the MessageProcessCmd
	int cPending = _thrdpool.pendingRequestSize();
	int cThreads = _thrdpool.size();

	_log((cPending>(cThreads*3)) ? Log::L_WARNING : Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] associated %d ServerRequests and %d Responses, %d requests were found expired, [%d]pendings on threadpool[%d/%d]"),
		connDescription(), pendingPeerRequests.size(), tmpResponseList.size(), expiredRequests.size(), 
		cPending, _thrdpool.activeCount(), cThreads);

	// the associated response of request
	for (itStack = pendingResponses.begin(); itStack < pendingResponses.end(); itStack ++)
	{
		ISSUECMD(MessageProcessCmd, (*this, *itStack));
	}

	// the RTSP requests from the peer
	for (itStack = pendingPeerRequests.begin(); itStack < pendingPeerRequests.end(); itStack ++)
	{
		ISSUECMD(MessageProcessCmd, (*this, *itStack));
	}

	for (itStack = expiredRequests.begin(); itStack < expiredRequests.end(); itStack ++)
	{
		ISSUECMD(RequestErrCmd, (*this, itStack->outReq, Err_RequestTimeout));
	}

	if (expiredRequests.size() >0)
		llevel = Log::L_INFO;

	_log(llevel, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] message process dispatched: %d ServerRequests and %d Responses, %d expired requests: %s"), connDescription(), pendingPeerRequests.size(), tmpResponseList.size(), expiredRequests.size(), expCSeqStr.c_str());
}

void RTSPClient::OnError()
{
	MutexGuard g(_lockQueueToSend); // borrow _lockQueueToSend to avoid thread-unsafe at non-blocking TCPClient::connect()
	std::string conndesc = connDescription(); // remember the conn desc for logging later

	_log(Log::L_ERROR, CLOGFMT(RTSPClient, "OnError() conn[%s] socket error[%d] occurred, disconnecting... "), conndesc.c_str(), checkSoErr());
	disconnect();

	_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnError() conn[%s] disconnected, cancelling %d requests queued"), conndesc.c_str(), _requestsQueueToSend.size());
	while (!_requestsQueueToSend.empty())
	{
		RTSPRequest::Ptr pReq = _requestsQueueToSend.front();
		_requestsQueueToSend.pop();

		ISSUECMD(RequestErrCmd, (*this, pReq, RTSPSink::Err_SendFailed));
	}
}

void RTSPClient::setClientTimeout(int32 connectTimeout, int32 messageTimeout)
{
	_connectTimeout = connectTimeout;
	_messageTimeout = messageTimeout;

	if (_messageTimeout <=2) // ensure the value divided by 2 is still valid
		_messageTimeout = DEFAULT_CLIENT_TIMEOUT;

	if (_connectTimeout <=0)
		_connectTimeout = DEFAULT_CLIENT_TIMEOUT;

	TCPSocket::setTimeout(_messageTimeout >>1); // half of _messageTimeout to wake up the socket sleep()
}

void RTSPClient::_cleanupExpiredAwaitRequests(uint8 multiplyTimeout, char* func) // private use
{
	RequestQueue expiredRequests;
	MutexGuard g(_lockAwaitResponse);
	int64 stampNow = TimeUtil::now();

	if (multiplyTimeout <1)
		multiplyTimeout =1;

	int64 thdtime = stampNow - _messageTimeout * multiplyTimeout;

	for (CSeqToRTSPRequestMap::iterator it = _requestsAwaitResponse.begin(); it != _requestsAwaitResponse.end(); it++)
	{
		// only exist in the _requestsAwaitResponse here, check if it has been expired
		if (it->second->stampCreated < thdtime)
			expiredRequests.push(it->second);
	}

	char buf[100];

	while (!expiredRequests.empty())
	{
		RTSPRequest::Ptr pReq = expiredRequests.front();
		expiredRequests.pop();

		try {
			if (!pReq)
				continue;

			_requestsAwaitResponse.erase(pReq->cSeq);
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPClient, "%s() conn[%s] %s(%d) as of [%s]"), func?func:"", connDescription(), pReq->_commandName.c_str(), pReq->cSeq, TimeUtil::TimeToUTC(pReq->stampCreated, buf, sizeof(buf)-2, true));
			ISSUECMD(RequestErrCmd, (*this, pReq, RTSPSink::Err_RequestTimeout));
		}
		catch(...) {}
	}
}

void RTSPClient::OnTimeout()
{
	_cleanupExpiredAwaitRequests(2, "sendRequest");

	if (RTSP_VERBOSEFLG_TCPTHREADPOOL & _verboseFlags)
	{
		int poolSize, activeCount, pendingSize=0;
		if (TCPSocket::getThreadPoolStatus(poolSize, activeCount, pendingSize) && pendingSize>0)
			_log(pendingSize>100? Log::L_WARNING :Log::L_DEBUG, CLOGFMT(RTSPClient, "OnTimeout() TCP ThreadPool[%d/%d] pending [%d] requests"), activeCount, poolSize, pendingSize);
	}

	if (RTSP_VERBOSEFLG_THREADPOOL & _verboseFlags)
	{
		int poolSize, activeCount, pendingSize=0;
		poolSize    = _thrdpool.size();
		pendingSize = _thrdpool.pendingRequestSize();
		activeCount = _thrdpool.activeCount();

		if (pendingSize >0)
			_log(pendingSize>100? Log::L_WARNING :Log::L_DEBUG, CLOGFMT(RTSPClient, "OnTimeout() client ThreadPool[%d/%d] pending [%d] requests"), activeCount, poolSize, pendingSize);
	}
}

// about the non-session requests
void RTSPClient::OnResponse(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
{
	_log(Log::L_INFO, CLOGFMT(RTSPClient, "OnResponse() conn[%s] %s(%d) received response: %d %s"), connDescription(), pReq->_commandName.c_str(), pReq->cSeq, resultCode, resultString);
}

void RTSPClient::OnServerRequest(RTSPClient& rtspClient, const char* cmdName, const char* url, RTSPMessage::Ptr& pInMessage)
{
	_log(Log::L_INFO, CLOGFMT(RTSPClient, "OnServerRequest() conn[%s] received peer request %s(%d)"), connDescription(), pInMessage->startLine.c_str(), pInMessage->cSeq);
}

void RTSPClient::OnRequestError(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RequestError errCode, const char* errDesc)
{
	if (NULL == errDesc || strlen(errDesc) <=0)
		errDesc = requestErrToStr(errCode);

	char buf[64];
	_log(Log::L_INFO, CLOGFMT(RTSPClient, "OnRequestError() conn[%s] %s(%d): %d %s; %s +%d"),
		 connDescription(), pReq->_commandName.c_str(), pReq->cSeq, errCode, errDesc, TimeUtil::TimeToUTC(pReq->stampCreated, buf, sizeof(buf)-2, true), _timeout);
}


}}//endof namespace

// vim: ts=4 sw=4 bg=dark nu
