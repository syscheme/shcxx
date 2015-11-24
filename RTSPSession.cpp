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
// Desc  : impl RTSPSession class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/RTSPSession.cpp $
// 
// 22    11/10/15 3:10p Hui.shao
// 
// 22    11/10/15 3:00p Hui.shao
// ticket#18316 to protect multiple in comming message of a same session
// 
// 21    5/06/15 5:24p Hui.shao
// 
// 20    2/08/14 7:16p Hui.shao
// merged from V1.16
// 
// 19    12/11/13 5:59p Hui.shao
// 
// 18    12/11/13 4:44p Hui.shao
// rollback
// 
// 16    6/20/13 9:46a Hongquan.zhang
// fix compiling error in CentOS6.3
// 
// 15    12/19/12 5:31p Hui.shao
// 
// 14    12/17/12 5:44p Hui.shao
// added transfer MP2T/AVP/UDP as default for udp://
// 
// 13    11/28/12 5:09p Hongquan.zhang
// remove a leading white space of a transport value
// 
// 12    10/24/12 12:10p Hongquan.zhang
// 
// 11    4/02/12 3:55p Hui.shao
// merged from V1.15.1
// 
// 16    3/12/12 3:29p Hui.shao
// 
// 15    3/12/12 12:20p Build
// 
// 14    3/12/12 10:43a Hui.shao
// 
// 13    3/09/12 11:32a Hui.shao
// RTSPSessionTimerCmd to access Session via Ptr
// 
// 12    9/13/11 3:27p Hongquan.zhang
// 
// 10    9/13/11 12:02p Hui.shao
// added start/stop default sessmgr
// 
// 9     8/18/11 6:39p Hui.shao
// 
// 8     1/28/11 2:56p Hui.shao
// moved the parsing of header Session into RTSPClient from RTSPSession
// for SETUP response
// 
// 7     1/27/11 3:58p Hui.shao
// 
// 6     1/26/11 2:26p Hui.shao
// parses the timeout returned via SETUP response, log fmts
// 
// 5     12/31/10 6:41p Fei.huang
// + merged with linux
// 
// 4     10-12-22 10:41 Haoyuan.lu
// 
// 3     10-12-17 11:00 Haoyuan.lu
// 
// 2     10-11-24 15:51 Haoyuan.lu
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 7     10-11-11 10:26 Haoyuan.lu
// 
// 6     10-11-03 15:49 Haoyuan.lu
// chuan.li add SDP part
// 
// 6     10-11-03 15:26 Chuan.li
// add the parse/generate get/set of SDP information
//
// 5     10-10-29 13:16 Haoyuan.lu
// 
// 4     10-10-22 14:06 Hui.shao
// RTSPClient linkable
// 
// 3     10-10-22 11:05 Hui.shao
// abstract rtsp session manager interface to port into DB in the future
// 
// 2     10-10-21 19:07 Hui.shao
// inherit RTSPRequest from RTSPMessage
// 
// 1     10-10-21 14:49 Hui.shao
// added RTSPSession
// ===========================================================================

#include "RTSPClient.h"
#include "urlstr.h"
#include "TimeUtil.h"
#include "Guid.h"
#include "Locks.h"
#include "SystemUtils.h"
#include <algorithm>

namespace ZQ{
namespace common{

// -----------------------------
// class RTSPSessionManager
// -----------------------------
class RTSPSessionManager : public NativeThread, public RTSPSession::IRTSPSessionManager
{
public:
	typedef std::map<std::string, RTSPSession::Ptr> SessionMap;
	typedef std::multimap <std::string, std::string> SessionIndex; // map of index to sess._sessGuid
	typedef std::pair <SessionIndex::iterator, SessionIndex::iterator> SessionIndexRange;

	SessionIndex _sessIdIndex;
	SessionMap   _sessMap;
	Mutex        _lockSessMap;

	RTSPSessionManager() : NativeThread(), _bQuit(false)
	{
//		start();
	}

	virtual ~RTSPSessionManager()
	{
		quit();
	}

public: // impl of IRTSPSessionManager
	void add(RTSPSession& sess);

	virtual uint size() { return _sessMap.size(); }

	void remove(RTSPSession& sess);
	void updateIndex(RTSPSession& sess, const char* indexName="SessionId", const char* oldValue=NULL);
	RTSPSession::List lookupByIndex(const char* sessionId, const char* indexName="SessionId");
	RTSPSession::Ptr  lookupByGuid(const char* sessGuid);

public:
	void eraseSessionId(const std::string& sessionId, RTSPSession& sess);
	void wakeup();
	void quit();

protected: // impl of NativeThread

	bool _bQuit;
	SYS::SingleObject _hWakeupEvent;

	virtual int run(void);
	virtual void final(void);
};

RTSPSessionManager gSessMgr;

// -----------------------------
// class RTSPSessionTimerCmd
// -----------------------------
class RTSPSessionTimerCmd : public ThreadRequest
{
public:
	RTSPSessionTimerCmd(RTSPSessionManager& sessMgr, NativeThreadPool& thpool, const std::string& sessGuid)
		: ThreadRequest(thpool), _sessGuid(sessGuid), _sessMgr(sessMgr)
	{
	}

protected:
	std::string _sessGuid;
	RTSPSessionManager& _sessMgr;

	virtual int run()
	{
		try {
			RTSPSession::Ptr pSess = _sessMgr.lookupByGuid(_sessGuid.c_str());
			if (pSess)
				pSess->OnSessionTimer();

			return 0;
		}
		catch(...) {}
		return -1;
	}

	void final(int retcode =0, bool bCancelled =false)
	{
		delete this;
	}
};

// -----------------------------
// class RTSPSessionManager
// -----------------------------
void RTSPSessionManager::add(RTSPSession& sess)
{
	if (sess._sessGuid.empty())
	{
		// un-assigned user's session id, generate one
		Guid guid;
		guid.create();
		char buf[80];
		guid.toCompactIdstr(buf, sizeof(buf) -2);
		sess._sessGuid = buf;
	}
	MutexGuard g(_lockSessMap);
	MAPSET(SessionMap, _sessMap, sess._sessGuid, &sess);
	if (!sess._sessionId.empty())
		_sessIdIndex.insert(SessionIndex::value_type(sess._sessionId, sess._sessGuid));
}

void RTSPSessionManager::remove(RTSPSession& sess)
{
	if (!sess._sessionId.empty())
		eraseSessionId(sess._sessionId, sess);

	MutexGuard g(_lockSessMap);
	_sessMap.erase(sess._sessGuid);
}

void RTSPSessionManager::updateIndex(RTSPSession& sess, const char* indexName, const char* oldValue)
{
	std::string oldSessId = oldValue? oldValue:sess._sessionId;

	if (!oldSessId.empty())
		eraseSessionId(oldSessId, sess);

	MutexGuard g(_lockSessMap);
	_sessIdIndex.insert(SessionIndex::value_type(sess._sessionId, sess._sessGuid));
}

RTSPSession::List RTSPSessionManager::lookupByIndex(const char* sessionId, const char* indexName)
{
	RTSPSession::List list;

	if (NULL == sessionId)
		return list;

	SessionIndexRange range;

	MutexGuard g(_lockSessMap);
	range = _sessIdIndex.equal_range(sessionId);

	for (SessionIndex::iterator it = range.first; it != range.second; ++it)
	{
		SessionMap::iterator itS = _sessMap.find(it->second);
		if (_sessMap.end() ==itS)
			continue;

		list.push_back(itS->second);
	}

	return list;
}

RTSPSession::Ptr RTSPSessionManager::lookupByGuid(const char* sessGuid)
{
	if (NULL == sessGuid)
		return NULL;

	MutexGuard g(_lockSessMap);
	SessionMap::iterator it = _sessMap.find(sessGuid);
	if (_sessMap.end() ==it)
		return NULL;

	return it->second;
}

void RTSPSessionManager::eraseSessionId(const std::string& sessionId, RTSPSession& sess)
{
	SessionIndexRange range;

	MutexGuard g(_lockSessMap);
	bool bFound =true;
	while (bFound)
	{
		range = _sessIdIndex.equal_range(sessionId);
		bFound = false;
		for (SessionIndex::iterator it = range.first; it != range.second; ++it)
		{
			if (0 != it->second.compare(sess._sessGuid))
				continue;

			bFound = true;
			_sessIdIndex.erase(it);
			break;
		}
	}
}

void RTSPSessionManager::wakeup()
{
	_hWakeupEvent.signal();
}

void RTSPSessionManager::quit()
{
	_bQuit = true;
	wakeup();
}

int RTSPSessionManager::run(void)
{
	int nextSleep = 30*1000;
	while(!_bQuit)
	{
		while(_sessMap.size() <=0 && !_bQuit)
		{
			_hWakeupEvent.wait(nextSleep);
		}

		if(_bQuit)
			break;

		// scan for those sessions that met timeout, and determine the minimal sleep time
		nextSleep = 30*1000;
		{
			MutexGuard guard(_lockSessMap);
			int64 stampNow = TimeUtil::now();
			for (SessionMap::iterator it = _sessMap.begin(); !_bQuit && it != _sessMap.end(); it++)
			{
				if (!it->second || it->second->_sessTimeout <=1000)
					continue;

				if (it->second->_stampLastMessage <=0)
				{
					it->second->_stampLastMessage = stampNow;
					continue;
				}

				int32 workSessTimeout = (int32) (it->second->_sessTimeout *0.8 +500);

				int64 timeout = it->second->_stampLastMessage + workSessTimeout - stampNow;

				if (timeout <= 0)
				{
					try {
						(new RTSPSessionTimerCmd(*this, it->second->_thrdpool, it->first))->start();
					}
					catch (...) {}
				}
				else if (timeout < nextSleep)
					nextSleep = (int) timeout;
			}
		}

		_hWakeupEvent.wait(nextSleep);
	}

	return 0;
}

void RTSPSessionManager::final(void)
{
	{
		MutexGuard guard(_lockSessMap);
		_sessMap.clear();
		_sessIdIndex.clear();
	}

	quit();
}

// -----------------------------
// class RTSPSession
// -----------------------------
void RTSPSession::startDefaultSessionManager()
{
	gSessMgr.start();
}

void RTSPSession::stopDefaultSessionManager()
{
	gSessMgr.quit();
	gSessMgr.waitHandle(-1);
}

#define SESSLOGFMT(_X)     CLOGFMT(RTSPSession, "sess[%s,%s] " _X), _sessGuid.c_str(), _sessionId.c_str()
RTSPSession::IRTSPSessionManager* RTSPSession::_sessMgr = &gSessMgr;

RTSPSession::IRTSPSessionManager* RTSPSession::setSessionManager(IRTSPSessionManager* rsm)
{
	if (NULL != _sessMgr && _sessMgr->size()>0) 
		return _sessMgr; // don't change if there are active sessions already

	_sessMgr = (NULL!=rsm) ? rsm : &gSessMgr;
	return _sessMgr;
}

void RTSPSession::updateIndex(RTSPSession* sess)
{
	if (NULL != _sessMgr) 
		return _sessMgr->updateIndex(*sess);
}

RTSPSession::RTSPSession(Log& log, NativeThreadPool& thrdpool, const char* streamDestUrl, const char* filePath, Log::loglevel_t verbosityLevel, int timeout, const char* sessGuid)
: _thrdpool(thrdpool), _tpType(tpt_UNKNOWN), _log(log, verbosityLevel), _stampSetup(0), _stampLastMessage(0), _filePath(trim(filePath)), _sessTimeout(timeout)
{
	if (NULL == sessGuid || strlen(sessGuid) <=0)
	{
		// un-assigned user's session id, generate one
		char buf[80];
		Guid guid;
		guid.create();
		guid.toCompactIdstr(buf, sizeof(buf) -2);
		_sessGuid = buf;
	}
	else
		_sessGuid = sessGuid;

	setStreamDestination(streamDestUrl, false);

	_sessMgr->add(*this);
}

RTSPSession::~RTSPSession()
{
	ZQ::common::MutexGuard g(_lockIncomming); // to avoid if incomming message is happened processed
//	if (_sessMgr == &gSessMgr)
//	{
//		destroy();
//	}
}

void RTSPSession::destroy()
{
	_sessMgr->remove(*this);
}

RTSPSession::List RTSPSession::lookupBySessionId(const char* sessionId)
{
	return _sessMgr->lookupByIndex(sessionId, "SessionId");
}

RTSPSession::Ptr RTSPSession::lookupBySessionGuid(const char* userSessionId)
{
	return _sessMgr->lookupByGuid(userSessionId);
}

void RTSPSession::OnResponse(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
{
#define MAP_RESP_BEGIN() if (0)
#define MAP_RESP(_CMD) else if (0 == pReq->_commandName.compare(#_CMD)) OnResponse_##_CMD(rtspClient, pReq, pResp, resultCode, resultString)
#define MAP_RESP_END()

	_log(Log::L_DEBUG, SESSLOGFMT("OnResponse() %s(%d) received response: %d %s"), pReq->_commandName.c_str(), pReq->cSeq, resultCode, resultString);

	MAP_RESP_BEGIN();
	MAP_RESP(SETUP);
	MAP_RESP(TEARDOWN);
	MAP_RESP(PLAY);
	MAP_RESP(PAUSE);
	MAP_RESP(GET_PARAMETER);
	MAP_RESP(SET_PARAMETER);
	MAP_RESP_END();
}

void RTSPSession::OnServerRequest(RTSPClient& rtspClient, const char* cmdName, const char* url, RTSPMessage::Ptr& pInMessage)
{
#define MAP_REQ_BEGIN() if (0)
#define MAP_REQ(_CMD) else if (0 == strcmp(cmdName, #_CMD)) On##_CMD(rtspClient, pInMessage)
#define MAP_REQ_END()

	_log(Log::L_DEBUG, SESSLOGFMT("OnServerRequest() received %s(%d) from the server"), cmdName?cmdName:"", pInMessage->cSeq);
	MAP_REQ_BEGIN();
	MAP_REQ(ANNOUNCE);
	MAP_REQ_END();
}

void RTSPSession::OnRequestError(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RequestError errCode, const char* errDesc)
{
	if (!pReq)
		return;

	if (NULL == errDesc || strlen(errDesc) <=0)
		errDesc = requestErrToStr(errCode);

	char buf[64];
	_log(Log::L_ERROR, SESSLOGFMT("OnRequestError() conn[%s] %s(%d): %d %s; %s +%d"),
		rtspClient.connDescription(), pReq->_commandName.c_str(), pReq->cSeq, errCode, errDesc, TimeUtil::TimeToUTC(pReq->stampCreated, buf, sizeof(buf)-2, true), rtspClient.getTimeout());
}


const char* RTSPSession::sessionDescription(bool generate, const char* serverType)
{
	if (!generate)
		return _sessionDescription.c_str();

	if (_stampSetup >0 && !_sessionId.empty())
	{
		_log(Log::L_WARNING, SESSLOGFMT("sessionDescription() generate session SDP failed, has already been SETUP"));
		return _sessionDescription.c_str();
	}

	_sessionDescription = ""; // reset first

#pragma message ( __MSGLOC__ "TODO: not yet SETUP, generate the SDP body to _sessionDescription instead")
//	Chuan.li Add to add generate operation for SDP 
//#ifdef USING_SDP
	const int TMP_DATA_SIZE = 256;
	char szTmp[TMP_DATA_SIZE] = {0};

	//	v=
	_sessionDescription += "v=";
	_sessionDescription += _protocolVersion;
	_sessionDescription += "\r\n";
	
	//	o=
	_sessionDescription += "o=";
	_sessionDescription += _userName;
	_sessionDescription += " ";
	_sessionDescription += _sessionId;
	_sessionDescription += " ";
	_sessionDescription += _sessVersion;
	_sessionDescription += " ";
	_sessionDescription += _sessNetType;
	_sessionDescription += " ";
	_sessionDescription += _sessAddrType;
	_sessionDescription += " ";
	_sessionDescription += _sessAddr;
	_sessionDescription += "\r\n";

	//	s=
	_sessionDescription += "s=";
	_sessionDescription += _sessionName;
	_sessionDescription += "\r\n";

	//	i=
	if(_sessionInformation.compare("")!=0)
	{
		_sessionDescription += "i=";
		_sessionDescription += _sessionInformation;
		_sessionDescription += "\r\n";
	}

	//	u=
	if(_descriptionURI.compare("")!=0)
	{
		_sessionDescription += "u=";
		_sessionDescription += _descriptionURI;
		_sessionDescription += "\r\n";
	}

	//	e=
	if(_emailAddr.compare("")!=0)
	{
		_sessionDescription += "e=";
		_sessionDescription += _emailAddr;
		_sessionDescription += "\r\n";
	}

	//	p=
	if(_phoneNum.compare("")!=0)
	{
		_sessionDescription += "p=";
		_sessionDescription += _phoneNum;
		_sessionDescription += "\r\n";
	}

	//	c=
	if(_connNetType.compare("")!=0)
	{
		_sessionDescription += "c=";
		_sessionDescription += _connNetType;
		_sessionDescription += " ";
		_sessionDescription += _connAddrType;
		_sessionDescription += " ";
		_sessionDescription += _connAddr;
		if(_connTTL.compare("")!=0)
		{
			_sessionDescription += "/";
			_sessionDescription += _connTTL;
		}
		_sessionDescription += "\r\n";
	}


	//	b=
	if(_bandwidthModifier.compare("")!=0)
	{
		_sessionDescription += "b=";
		_sessionDescription += _bandwidthModifier;
		_sessionDescription += ":";
		_sessionDescription += _bandwidthValue;
		_sessionDescription += "\r\n";
	}

	//	t=
	_sessionDescription += "t=";
	_sessionDescription += _starttime;
	_sessionDescription += " ";
	_sessionDescription += _stoptime;
	_sessionDescription += "\r\n";

	if(_timelyRepeatConfig.size()>0)
	{		
		for(std::vector<RepeatConfig>::iterator iter=_timelyRepeatConfig.begin(); iter!=_timelyRepeatConfig.end();iter++)
		{
			_sessionDescription += "r=";
			std::vector<int>::iterator iterData=(*iter).begin();
			_sessionDescription += itoa(*iterData++, szTmp, 10);
			for(; iterData!=(*iter).end(); iterData++)
			{				
				_sessionDescription += " ";
				_sessionDescription += itoa(*iterData, szTmp, 10);
			}
			_sessionDescription += "\r\n";
		}
	}

	//	z=
	if(_timezoneAdjustList.size()>0)
	{
		_sessionDescription += "z=";

		::std::map<int64, ::std::string>::iterator iter=_timezoneAdjustList.begin();
#ifdef ZQ_OS_MSWIN
		_sessionDescription += _i64toa((*iter).first, szTmp, 10);
#else
		_sessionDescription += itoa((*iter).first, szTmp, 10);
#endif
		_sessionDescription += " ";
		_sessionDescription += (*iter).second;
		iter++;
		for(; iter!=_timezoneAdjustList.end();iter++)
		{
			_sessionDescription += " ";
#ifdef ZQ_OS_MSWIN
			_sessionDescription += _i64toa((*iter).first, szTmp, 10);
#else
			_sessionDescription += itoa((*iter).first, 0, 10);
#endif
			_sessionDescription += " ";
			_sessionDescription += (*iter).second;
		}
		_sessionDescription += "\r\n";
	}

	//	k=
	if(_encryptMethod.compare("")!=0)
	{
		_sessionDescription += "k=";
		_sessionDescription += _encryptMethod;
		if(_encryptKey.compare("")!=0)
		{
			_sessionDescription += ":";
			_sessionDescription += _encryptKey;
		}
		_sessionDescription += "\r\n";
	}

	//	a=
	if(_attrList.size()>0)
	{
		for(std::vector<std::string>::iterator iter=_attrList.begin(); iter!=_attrList.end();iter++)
		{
			_sessionDescription += "a=";
			_sessionDescription += *(iter);
			_sessionDescription += "\r\n";
		}
	}

	if(_attrPairList.size()>0)
	{
		for(std::map<std::string, std::string>::iterator iter=_attrPairList.begin(); iter!=_attrPairList.end();iter++)
		{
			_sessionDescription += "a=";
			_sessionDescription += (*iter).first;
			_sessionDescription += ":";
			_sessionDescription += (*iter).second;
			_sessionDescription += "\r\n";
		}
	}

	//	m=
	if(_mediaDataList.size()>0)
	{
		for(::std::vector<MediaPropertyData>::iterator iter=_mediaDataList.begin(); iter!=_mediaDataList.end();iter++)
		{		
			//	m= in media
			_sessionDescription += "m=";
			_sessionDescription += (*iter).mediaType;
			_sessionDescription += " ";
			_sessionDescription += itoa((*iter).mediaPort, szTmp, 10);
			_sessionDescription += " ";
			_sessionDescription += (*iter).mediaTransport;
			_sessionDescription += " ";
			_sessionDescription += (*iter).mediaFmt;
			_sessionDescription += "\r\n";

			// i= in media
			if((*iter).mediaTitle.compare("")!=0)
			{
				_sessionDescription += "i=";
				_sessionDescription += (*iter).mediaTitle;
				_sessionDescription += "\r\n";
			}

			// c= in media
			if((*iter).mediaConnNetType.compare("")!=0)
			{
				_sessionDescription += "c=";
				_sessionDescription += (*iter).mediaConnNetType;
				_sessionDescription += " ";
				_sessionDescription += (*iter).mediaConnAddrType;
				_sessionDescription += " ";
				_sessionDescription += (*iter).mediaConnAddr;
				if((*iter).mediaConnTTL.compare("")!=0)
				{
					_sessionDescription += "/";
					_sessionDescription += (*iter).mediaConnTTL;
				}
				_sessionDescription += "\r\n";
			}

			// b= in media
			if((*iter).mediaBandwidthModifier.compare("")!=0)
			{
				_sessionDescription += "b=";
				_sessionDescription += (*iter).mediaBandwidthModifier;
				_sessionDescription += " ";
				_sessionDescription += (*iter).mediaBandwidthValue;
				_sessionDescription += "\r\n";
			}

			// k= in media
			if((*iter).mediaEncryptMethod.compare("")!=0)
			{
				_sessionDescription += "k=";
				_sessionDescription += (*iter).mediaEncryptMethod;
				if((*iter).mediaEncryptKey.compare("")!=0)
				{
					_sessionDescription += ":";
					_sessionDescription += (*iter).mediaEncryptKey;
				}
				_sessionDescription += "\r\n";
			}

			// a= in media
			if((*iter).mediaAttrList.size()>0)
			{
				for(std::vector<std::string>::iterator iterAttr=(*iter).mediaAttrList.begin(); iterAttr!=(*iter).mediaAttrList.end();iterAttr++)
				{
					_sessionDescription += "a=";
					_sessionDescription += *(iterAttr);
					_sessionDescription += "\r\n";
				}
			}

			if((*iter).mediaAttrPairList.size()>0)
			{
				for(std::map<std::string, std::string>::iterator iterAttrPair=(*iter).mediaAttrPairList.begin(); iterAttrPair!=(*iter).mediaAttrPairList.end();iterAttrPair++)
				{
					_sessionDescription += "a=";
					_sessionDescription += (*iterAttrPair).first;
					_sessionDescription += ":";
					_sessionDescription += (*iterAttrPair).second;
					_sessionDescription += "\r\n";
				}
			}

		}
	}
	return _sessionDescription.c_str();
}

const char* RTSPSession::getTransport(bool generate, const char* serverType, bool streamOutgoing)
{
	if (!generate)
		return _tpTransport.c_str();

	if (_stampSetup >0 && !_sessionId.empty())
	{
		_log(Log::L_WARNING, SESSLOGFMT("getTransport() generate Transport failed, has already been SETUP"));
		return _tpTransport.c_str();
	}

	_log(Log::L_DEBUG, SESSLOGFMT("getTransport() generating Transport based destUrl[%s] and Server[%s]"), _tpStreamDestUrl.c_str(), serverType?serverType:"n/a");
	_tpTransport = ""; // reset first

#pragma message ( __MSGLOC__ "TODO: take the URL proto definition like VLC on _tpStreamDestUrl, and determine the SETUP header Transport")
	// http://tools.ietf.org/html/rfc2326#page-58 
	// sample formats:
    // udp://@223.12.12.21:1234[?server_port=1000]    -> RAW/RAW/UDP;multicast;destination=223.12.12.21;client_port=1234[;server_port=1000]
    // udp://@192.168.1.22:1234[?client=00112233445566]  -> RAW/RAW/UDP;unicast;destination=192.168.1.22;client_port=1234[;client=00112233445566]
	// rtpudp://@192.168.1.22:1234[?client=00112233445566]-> RTP/AVP/UDP;unicast;destination=192.168.1.22;client_port=1234[;client=00112233445566]
	// rtp://@192.168.1.22:1234[?client=00112233445566] -> RTP/AVP/TCP;unicast;destination=192.168.1.22;client_port=1234[;client=00112233445566]
	// dvbc://@192.168.1.22:1234/57400000/23[?symbol_rate=51200] -> MP2T/DVBC/UDP;unicast;destination=192.168.1.22;client_port=1234;frequence=57400000;program_number=23[;symbol_rate=51200]

	// generate the transport statement

	bool bMcastStreaming = false;
	URLStr url(_tpStreamDestUrl.c_str());
	try {
		InetMcastAddress addr(url.getHost());
		bMcastStreaming = true;
	} catch (...) {}

	std::string protocal = url.getProtocol();
	std::transform(protocal.begin(), protocal.end(), protocal.begin(), tolower);
	if("udp" == protocal)
		_tpType = RTSPSession::tpt_MP2T_AVP_UDP;
	else if ("rawudp" == protocal)
		_tpType = RTSPSession::tpt_RAW_RAW_UDP;
	else if("rtpudp" == protocal)
		_tpType = RTSPSession::tpt_RTP_AVP;
	else if("rtp" == protocal)
		_tpType = RTSPSession::tpt_RTP_AVP_TCP;
	else if("dvbc" == protocal)
		_tpType = RTSPSession::tpt_MP2T_DVBC_UDP;

	char buf[8196] = { 0 };

	switch (_tpType)
	{
/*
	case RTSPSession::tpt_RAW_RAW_UDP:
		snprintf(buf, sizeof(buf) -2, "RAW/RAW/UDP%s%s;destination=%s;client_port=%d",
			(bMcastStreaming ? ";multicast" : ";unicast"), (streamOutgoing ? ";mode=receive" : ""),
			InetAddress(url.getHost()).getHostAddress(), url.getPort());
		break;
*/
	case RTSPSession::tpt_RAW_RAW_UDP:
		snprintf(buf, sizeof(buf) -2, "MP2T/AVP/UDP%s%s;destination=%s;client_port=%d",
			(bMcastStreaming ? ";multicast" : ";unicast"), (streamOutgoing ? ";mode=receive" : ""),
			InetAddress(url.getHost()).getHostAddress(), url.getPort());
		break;

	case RTSPSession::tpt_RTP_AVP:
		snprintf(buf, sizeof(buf) -2, "RTP/AVP%s%s;destination=%s;client_port=%d-%d",
			(bMcastStreaming ? ";multicast" : ";unicast"), (streamOutgoing ? ";mode=receive" : ""),
			InetAddress(url.getHost()).getHostAddress(), _tpRTPPort, _tpRTCPPort);
		break;

	case RTSPSession::tpt_RTP_AVP_TCP:
		if (bMcastStreaming)
			return _tpTransport.c_str();

		snprintf(buf, sizeof(buf) -2, "RTP/AVP/TCP;unicast%s;interleaved=%d-%d",
			(streamOutgoing ? ";mode=receive" : ""),
			_tpRTPPort, _tpRTCPPort);
		break;

	case RTSPSession::tpt_MP2T_DVBC_UDP:
		snprintf(buf, sizeof(buf) -2, " MP2T/DVBC/UDP%s%s;destination=%s;client_port=%d",
			(bMcastStreaming ? ";multicast" : ";unicast"), (streamOutgoing ? ";mode=receive" : ""),
			InetAddress(url.getHost()).getHostAddress(), url.getPort());
		break;
	}

	_tpTransport = buf;
	::std::map<std::string, std::string>::iterator iter;
	for (iter = _transportEx.begin(); iter != _transportEx.end(); iter++)
	{
		_tpTransport = _tpTransport + ";" + iter->first + "=" + iter->second;
	}

	return _tpTransport.c_str();
}

bool RTSPSession::setStreamDestination(const char* destUrl, bool validate)
{
	if (_stampSetup >0 && !_sessionId.empty())
	{
		_log(Log::L_ERROR, SESSLOGFMT("setStreamDestination() failed, has already been SETUP"));
		return false;
	}

	if (NULL == destUrl || strlen(destUrl) <= 0)
	{
		_log(Log::L_ERROR, SESSLOGFMT("setStreamDestination() failed, null destination URL"));
		return false;
	}

	_tpStreamDestUrl = destUrl;

	if (!validate)
		return true;

	std::string tsValue = getTransport(true);
	return !tsValue.empty();
}

void RTSPSession::OnSessionTimer()
{
#pragma message ( __MSGLOC__ "TODO: impl here, trigger RTSP PING or GET_PARAMETER")
	_log(Log::L_DEBUG, SESSLOGFMT("OnSessionTimer(), need to PING"));
}

// response handling
void RTSPSession::OnResponse_SETUP(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
{
#pragma message ( __MSGLOC__ "TODO: impl here")
	_stampSetup = now();
	// _log(Log::L_DEBUG, SESSLOGFMT("OnResponse_SETUP() [%d %s]"), resultCode, resultString);
	_sessMgr->updateIndex(*this);
	_log(Log::L_DEBUG, SESSLOGFMT("OnResponse_SETUP() [%d %s] w/ session timeout[%d]msec"), resultCode, resultString, _sessTimeout);
}

void RTSPSession::OnResponse_TEARDOWN(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
{
#pragma message ( __MSGLOC__ "TODO: impl here")
	_log(Log::L_DEBUG, SESSLOGFMT("OnResponse_TEARDOWN() [%d %s]"), resultCode, resultString);
}

void RTSPSession::OnResponse_PLAY(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
{
#pragma message ( __MSGLOC__ "TODO: impl here")
	_log(Log::L_DEBUG, SESSLOGFMT("OnResponse_PLAY() [%d %s]"), resultCode, resultString);
}

void RTSPSession::OnResponse_PAUSE(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
{
#pragma message ( __MSGLOC__ "TODO: impl here")
	_log(Log::L_DEBUG, SESSLOGFMT("OnResponse_PAUSE() [%d %s]"), resultCode, resultString);
}

void RTSPSession::OnResponse_GET_PARAMETER(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
{
#pragma message ( __MSGLOC__ "TODO: impl here")
	_log(Log::L_DEBUG, SESSLOGFMT("OnResponse_GET_PARAMETER() [%d %s]"), resultCode, resultString);
}

void RTSPSession::OnResponse_SET_PARAMETER(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
{
#pragma message ( __MSGLOC__ "TODO: impl here")
	_log(Log::L_DEBUG, SESSLOGFMT("OnResponse_SET_PARAMETER() [%d %s]"), resultCode, resultString);
}

// new overwriteable entries, dispatched from OnServerRequest()
void RTSPSession::OnANNOUNCE(RTSPClient& rtspClient, RTSPMessage::Ptr& pInMessage)
{
	_log(Log::L_DEBUG, SESSLOGFMT("OnANNOUNCE()"));

	if (pInMessage->headers.find("Session") != pInMessage->headers.end())
	{
		std::string notice = "";
		if (pInMessage->headers.find("Notice") != pInMessage->headers.end())
		{
			notice = pInMessage->headers["Notice"];
		}

		_log(Log::L_DEBUG, SESSLOGFMT("OnANNOUNCE() sess[%s] cseq(%d) Notice[%s]"), pInMessage->headers["Session"].c_str(), pInMessage->cSeq, notice.c_str());
	}
}



bool RTSPSession::initializeWithSDP(const char* sdpDescription)
{
//#ifdef USING_SDP
	//	2010-10-29 Chuan.li modify
	if(sdpDescription==NULL)return false;
	std::vector<std::string> msgLine;
	//split message to line by line

	//RTSPMessageParser::splitMsg2Line(sdpDescription, (uint16)strlen(sdpDescription), msgLine);
	::std::string strMessage(sdpDescription, (uint16)strlen(sdpDescription));
	::std::string::size_type pos_begin = 0;
	::std::string::size_type pos_end = 0;
	while(true)
	{
		pos_end = strMessage.find_first_of("\r\n", pos_begin);

		//	Note!!! The parser is modified!
		if(::std::string::npos == pos_end) // not a valid macro reference
		{
			if(pos_begin!=strMessage.size())
			{
				::std::string line = strMessage.substr(pos_begin, strMessage.max_size());
				msgLine.push_back(line);
			}
			break;
		}

		::std::string line = strMessage.substr(pos_begin, pos_end - pos_begin);

		//get line
		msgLine.push_back(line);

		//move pos_begin to next line
		pos_begin = pos_end + 2;
	}

	
	for (std::vector<std::string>::iterator strIter = msgLine.begin(); strIter != msgLine.end(); strIter++)
	{
		const char* pTmp = (*strIter).c_str();
		char  cData= *pTmp;
		::std::string strLine = (*strIter).substr(2, (*strIter).max_size());
		switch(cData)
		{
		case 'a':	//a=<attribute> a=<attribute>:<value>							//(zero or more session attribute lines)
			parseSDPLine_a(strLine);
			break;
		case 'v':	//v=<version>													//(protocol version)
			_protocolVersion = (pTmp+2);
			break;
		case 'o':	//o=<username> <session id> <version> <network type> <address type> <address>	//(owner/creator and session identifier).
			parseSDPLine_o(strLine);
			break;
		case 's':	//s=<session name>												//(session name)
			_sessionName = (pTmp+2);
			//parseSDPLine_s(pTmp);
			break;
		case 'i':	//i=<session description>										//(session information)
			//parseSDPLine_i(pTmp);
			_sessionInformation = (pTmp+2);
			break;
		case 'u':	//u=<URI>														//(URI of description)
			_descriptionURI = (pTmp+2);
			break;
		case 'e':	//e=<email address>												//(email address)
			_emailAddr = (pTmp+2);
			break;
		case 'p':	//p=<phone number>												//(phone number)
			_phoneNum = (pTmp+2);
			break;
		case 'c':	//c=<network type> <address type> <connection address>			//(connection information - not required if included in all media)
			parseSDPLine_c(strLine);
			break;
		case 'b':	//b=<modifier>:<bandwidth-value>								//(bandwidth information)
			parseSDPLine_b(strLine);
			break;
		case 'z':	//z=<adjustment time> <offset> <adjustment time> <offset> ....	//(time zone adjustments)
			parseSDPLine_z(strLine);
			break;
		case 'k':	//k=<method>	k=<method>:<encryption key>						//(encryption key)
			parseSDPLine_k(strLine);
			break;



		case 't':	//t=<start time> <stop time>									//(time the session is active)
			parseSDPLine_t(strLine);
			break;
		case 'r':	//r=<repeat interval> <active duration> <list of offsets from start-time> //(zero or more repeat times)
			parseSDPLine_r(strLine);
			break;


		case 'm':	//m=<media> <port> <transport> <fmt list>						//(media name and transport address)
			parseSDPLine_m(strLine, cData);
			{
				while((++strIter)!=msgLine.end())
				{
					const char* pTmpInMedia = (*strIter).c_str();
					char cDataInMedia= *pTmpInMedia;
					::std::string strLineInMedia = (*strIter).substr(2, (*strIter).max_size());
					if(cDataInMedia=='m')
					{
						strIter--;
						break;
					}
					else
					{
						parseSDPLine_m(strLineInMedia, cDataInMedia);
					}
				}
				//	If meet the end, means all data parsed
				if(strIter==msgLine.end())
				{
					strIter--;
				}
			}
			break;
		default:
			;
		}
	}
	////////////////////////////////////////////////////////////////////////////

	msgLine.clear();
	return true;
//#else
//	#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
//#endif

}



//******************//
//#ifdef USING_SDP
bool RTSPSession::clearSDPdata(const char szType)
{
	//	If the szType is '*', clear All data
	switch(szType)
	{
	case '*':
	case 'v': 	// parse v=
		_protocolVersion = "";
		if(szType!='*')break;
	case 'o':	// parse o=
		_userName = "";
		//std::string _sessId;	// (???) Chuan.li: Since this value is set outside the SDP parse, so not do clean here
		_sessVersion = "";	//	Session Version 
		_sessNetType = "";	//	Net type
		_sessAddrType = "";
		_sessAddr = "";
		if(szType!='*')break;
	case 's':	// parse s=
		// (???) Chuan.li: Since this value is set outside the SDP parse, so not do clean here
		if(szType!='*')break;
	case 'i':	// parse i=
		_sessionInformation = "";;
		if(szType!='*')break;
	case 'u':	// parse u=
		_descriptionURI = "";;
		if(szType!='*')break;
	case 'e':	// parse e=
		_emailAddr = "";;
		if(szType!='*')break;
	case 'p':	// parse p=
		_phoneNum = "";;
		if(szType!='*')break;
	case 'c':	// parse c=
		_connNetType = "";;
		_connAddrType = "";;
		_connAddr = "";;
		_connTTL = "";;
		if(szType!='*')break;
	case 'b':	// parse b=
		_bandwidthModifier = "";;
		_bandwidthValue = "";;
		if(szType!='*')break;
	case 't':	// parse t=
		_starttime = "";;
		_stoptime = "";;
		if(szType!='*')break;
	case 'r':	// parse r=
		_timelyRepeatConfig.clear();
		if(szType!='*')break;
	case 'z':	// parse z=
		_timezoneAdjustList.clear();
		if(szType!='*')break;
	case 'k':	// parse k=
		_encryptMethod	= "";
		_encryptKey		= "";
		if(szType!='*')break;
	case 'a':	// parse a=
		_attrList.clear();
		_attrPairList.clear();
		if(szType!='*')break;
	case 'm':	// parse m=
		_mediaDataList.clear(); 
		if(szType!='*')break;
	default:
		;
	}

	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}



bool RTSPSession::setSDPValue_v(const std::string& protocolVersion)
{
	_protocolVersion = protocolVersion;
	return true;
}

bool RTSPSession::setSDPValue_o(const std::string& userName, const std::string& sessionId, const std::string& sessVersion, 
									const std::string& sessNetType, const std::string& sessAddrType, const std::string& sessAddr)
{
	_userName		= userName;
	
	_sessionId		= sessionId;
	_sessVersion	= sessVersion;
	_sessNetType	= sessNetType;
	_sessAddrType	= sessAddrType;
	_sessAddr		= sessAddr;
	return true;
}

bool RTSPSession::setSDPValue_s(const std::string& sessionName)
{
	 _sessionName = sessionName;
	return false;
}

bool RTSPSession::setSDPValue_i(const std::string& sessionInformation)
{
	_sessionInformation = sessionInformation;
	return true;
}

bool RTSPSession::setSDPValue_u(const std::string& descriptionURI)
{
	_descriptionURI = descriptionURI;
	return true;
}

bool RTSPSession::setSDPValue_e(const std::string& emailAddr)
{
	_emailAddr = emailAddr;
	return true;
}

bool RTSPSession::setSDPValue_p(const std::string& phoneNum)
{
	_phoneNum = phoneNum;
	return true;
}

bool RTSPSession::setSDPValue_c(const std::string& connNetType, const std::string& connAddrType, 
											const std::string& connAddr, const std::string& connTTL)
{
	_connNetType	= connNetType;
	_connAddrType	= connAddrType;
	_connAddr		= connAddr;
	_connTTL		= connTTL;
	return true;
}

bool RTSPSession::setSDPValue_b(const std::string&	bandwidthModifier, const std::string& bandwidthValue)
{
	_bandwidthModifier	= bandwidthModifier;
	_bandwidthValue		= bandwidthValue;
	return true;
}

bool RTSPSession::setSDPValue_t(const std::string&	starttime, const std::string& stoptime)
{
	_starttime = starttime;
	_stoptime  = stoptime;
	return true;
}


bool RTSPSession::addSDPValue_r(const RepeatConfig& rept)
{
	_timelyRepeatConfig.push_back(rept);
	return true;
}

bool RTSPSession::addSDPValue_z(int64 iTime, const ::std::string strOffset)
{
	_timezoneAdjustList[iTime] = strOffset;
	return true;
}

bool RTSPSession::setSDPValue_k(const std::string& encryptMethod, const std::string& encryptKey)
{
	_encryptMethod	= encryptMethod;
	_encryptKey		= encryptKey;
	return true;
}


bool RTSPSession::addSDPValue_a(const std::string& strAttr)
{
	_attrList.push_back(strAttr);
	return true;
}

bool RTSPSession::addSDPValue_a_pair(const std::string& strKey, const std::string& strValue)
{
	_attrPairList[strKey] = strValue;
	return true;
}

bool RTSPSession::addSDPValue_m(const MediaPropertyData& mediaProper)
{
	_mediaDataList.push_back(mediaProper);
	return true;
}

void RTSPSession::addTransportEx(const std::string& key, const std::string& value)
{
	if(key.empty()) 
		return;

	if (value.empty() && _transportEx.end() != _transportEx.find(key))
		_transportEx.erase(key);
	else _transportEx[key] = value;
}


//	To get out the data, the whole vector/map will be the result, the result is not come out one by one
bool RTSPSession::getSDPValue_v(std::string& protocolVersion)
{
	protocolVersion = _protocolVersion;
	return true;
}

bool RTSPSession::getSDPValue_o(std::string& userName, std::string& sessId, std::string& sessVersion,
									std::string& sessNetType, std::string& sessAddrType, std::string& sessAddr)
{
	userName = _userName;
	sessId	= _sessionId;
	sessVersion = _sessVersion;
	sessNetType = _sessNetType;
	sessAddrType= _sessAddrType;
	sessAddr	= _sessAddr;
	return true;
}

bool RTSPSession::getSDPValue_s(std::string& sessionName)
{
	
	sessionName= _sessionName;
	return false;
}

bool RTSPSession::getSDPValue_i(std::string& sessionInformation)
{
	sessionInformation = _sessionInformation;
	return true;
}

bool RTSPSession::getSDPValue_u(std::string& descriptionURI)
{
	descriptionURI = _descriptionURI;
	return true;
}

bool RTSPSession::getSDPValue_e(std::string& emailAddr)
{
	emailAddr = _emailAddr;
	return true;
}

bool RTSPSession::getSDPValue_p(std::string& phoneNum)
{
	phoneNum = _phoneNum;
	return true;
}

bool RTSPSession::getSDPValue_c(std::string& connNetType, std::string& connAddrType, std::string& connAddr, std::string& connTTL)
{
	connNetType = _connNetType;
	connAddrType = _connAddrType;
	connAddr = _connAddr;
	connTTL = _connTTL;
	return true;
}

bool RTSPSession::getSDPValue_b(std::string&	bandwidthModifier, std::string& bandwidthValue)
{
	bandwidthModifier	= _bandwidthModifier;
	bandwidthValue		= _bandwidthValue;
	return true;
}

bool RTSPSession::getSDPValue_t(std::string&	starttime, std::string& stoptime)
{
	starttime	= _starttime; 
	stoptime	= _stoptime;
	return true;
}


std::vector<ZQ::common::RTSPSession::RepeatConfig>& RTSPSession::getSDPValue_r()
{
	return _timelyRepeatConfig;
}

ZQ::common::RTSPSession::TimeZoneAdjustList& RTSPSession::getSDPValue_z()
{
	return _timezoneAdjustList;
}

bool RTSPSession::getSDPValue_k(std::string& encryptMethod, std::string& encryptKey)
{
	encryptMethod	= _encryptMethod;	
	encryptKey		= _encryptKey;
	return true;
}

::std::vector<std::string>&				RTSPSession::getSDPValue_a()
{
	return _attrList;
}

::std::map<std::string, std::string>&	RTSPSession::getSDPValue_a_pair()
{
	return _attrPairList;
}

::std::string		RTSPSession::getSDPValue_a_pair(::std::string strKey)	// To get the value of a key\value pair
{
	for(std::map<std::string, std::string>::iterator iter=_attrPairList.begin(); iter!=_attrPairList.end();iter++)
	{
		if((*iter).first.compare(strKey)==0)
		{
			return (*iter).second;
		}
	}
	return "";
}

::std::vector<ZQ::common::RTSPSession::MediaPropertyData>&	RTSPSession::getSDPValue_m()
{
	return _mediaDataList;
}



bool RTSPSession::parseSDPLine_a(std::string& strLine)//const char* sdpLine)
{

	std::string::size_type pos_begin	= std::string::npos;
	std::string::size_type pos_end		= std::string::npos;

	//	Whether it is a control:value pair
	if((pos_begin=strLine.find(":"))!= std::string::npos)				
	{
		::std::string strValue = strLine.substr((pos_begin+1), strLine.max_size());
		::std::string strKey   = strLine.substr(0, pos_begin);
		_attrPairList[strKey] = strValue;

		if(strKey.compare("control")==0)
		{
			_controlUri = strValue;
		}

	}
	else
	{
		_attrList.push_back(strLine);
	}

	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}

//o=<username> <session id> <version> <network type> <address type> <address>	//(owner/creator and session identifier).
bool RTSPSession::parseSDPLine_o(std::string& strLine)//const char* sdpLine)
{
	std::string::size_type pos_begin	= std::string::npos;
	std::string::size_type pos_last_end	= std::string::npos;

	if((pos_begin=strLine.find(" "))!= std::string::npos)
	{
		_userName = strLine.substr(0, pos_begin);
		pos_last_end = pos_begin + 1;
		if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
		{
			//	(???) Whether need the parser of SDP reset the sessId???
			_sessionId = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
			pos_last_end = pos_begin + 1;
			if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
			{
				_sessVersion = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
				pos_last_end = pos_begin + 1;
				if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
				{
					_sessNetType = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
					pos_last_end = pos_begin + 1;
					if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
					{
						_sessAddrType = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
						pos_last_end = pos_begin + 1;
						_sessAddr = strLine.substr(pos_last_end, strLine.max_size());

					}
				}
			}
		}
	}
	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}

//c=<network type> <address type> <connection address>			//(connection information - not required if included in all media)
bool RTSPSession::parseSDPLine_c(std::string& strLine)//const char* sdpLine)
{	
	std::string::size_type pos_begin	= std::string::npos;
	std::string::size_type pos_last_end	= std::string::npos;

	if((pos_begin=strLine.find(" "))!= std::string::npos)
	{
		_connNetType = strLine.substr(0, pos_begin);
		pos_last_end = pos_begin + 1;
		if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
		{
			_connAddrType = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
			pos_last_end = pos_begin + 1;

			_connAddr = strLine.substr(pos_last_end, strLine.max_size());
			if((pos_begin=_connAddr.find("/"))!= std::string::npos)
			{
				_connTTL = _connAddr.substr(pos_begin+1, _connAddr.max_size());
				_connAddr = _connAddr.substr(0, pos_begin);					
			}

		}
	}

	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}


//b=<modifier>:<bandwidth-value>								//(bandwidth information)
bool RTSPSession::parseSDPLine_b(std::string& strLine)//const char* sdpLine)
{
	std::string::size_type pos_begin	= std::string::npos;
	std::string::size_type pos_last_end	= std::string::npos;

	if((pos_begin=strLine.find(":"))!= std::string::npos)
	{
		_bandwidthModifier = strLine.substr(0, pos_begin);
		pos_last_end = pos_begin + 1;
		_bandwidthValue = strLine.substr(pos_last_end, strLine.max_size());
	}

	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}

//z=<adjustment time> <offset> <adjustment time> <offset> ....	//(time zone adjustments)
bool RTSPSession::parseSDPLine_z(std::string& strLine)//const char* sdpLine)
{
	std::string::size_type pos_begin	= std::string::npos;
	std::string::size_type pos_last_end	= std::string::npos;
	std::string strAdjTime, strOffset;

	if((pos_begin=strLine.find(" "))!= std::string::npos)
	{
		strAdjTime = strLine.substr(0, pos_begin);
		pos_last_end = pos_begin + 1;
		if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
		{
			strOffset = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
			pos_last_end = pos_begin + 1;
			// The timezone adjust pair retrieved, add it in _timezoneAdjustList
			_timezoneAdjustList[_atoi64(strAdjTime.c_str())] =  strOffset;

			const int TIMEZONEADJ_PAIRS_MAX_COUNT = 256;
			int iCount = TIMEZONEADJ_PAIRS_MAX_COUNT;
			while(((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)	// retrieve the following timezone adjust pairs
				&&(iCount--))
			{
				strAdjTime = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
				pos_last_end = pos_begin + 1;
				if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
				{
					strOffset = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
					pos_last_end = pos_begin + 1;
					
					// The timezone adjust pair retrieved, add it in _timezoneAdjustList
					_timezoneAdjustList[_atoi64(strAdjTime.c_str())] =  strOffset;
				}
				else //The end position is reached
				{
					strOffset = strLine.substr(pos_last_end, strLine.max_size());
					// The timezone adjust pair retrieved, add it in _timezoneAdjustList
					_timezoneAdjustList[_atoi64(strAdjTime.c_str())] =  strOffset;
				}
			}
		}
	}

	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}

//k=<method>	k=<method>:<encryption key>						//(encryption key)
bool RTSPSession::parseSDPLine_k(std::string& strLine)//const char* sdpLine)
{
	std::string::size_type pos_begin	= std::string::npos;

	if((pos_begin=strLine.find(":"))!= std::string::npos)
	{
		_encryptMethod	= strLine.substr(0, pos_begin);
		_encryptKey		= strLine.substr(pos_begin+1, strLine.max_size());		
	}
	else
	{
		_encryptMethod	= strLine;
		_encryptKey		= "";
	}
	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}

//t=<start time> <stop time>									//(time the session is active)
bool RTSPSession::parseSDPLine_t(std::string& strLine)//const char* sdpLine)
{
	std::string::size_type pos_begin	= std::string::npos;
	std::string::size_type pos_last_end	= std::string::npos;

	if((pos_begin=strLine.find(" "))!= std::string::npos)
	{
		_starttime = strLine.substr(0, pos_begin);
		pos_last_end = pos_begin + 1;
		_stoptime = strLine.substr(pos_last_end, strLine.max_size());
	}

	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}

//r=<repeat interval> <active duration> <list of offsets from start-time> //(zero or more repeat times)
    //周期				// 每次延续时间		//一个周期里多次发生的话，这些事件发生的时间点
bool RTSPSession::parseSDPLine_r(std::string& strLine)//const char* sdpLine)
{
	RepeatConfig repeatCfg;

	
	std::string::size_type pos_begin	= std::string::npos;
	std::string::size_type pos_last_end	= std::string::npos;
	
	std::string strPeriod, strDuration, strStartOffset; 
	if((pos_begin=strLine.find(" "))!= std::string::npos)
	{
		strPeriod = strLine.substr(0, pos_begin);
		repeatCfg.push_back(atoi(strPeriod.c_str()));
		pos_last_end = pos_begin + 1;
		if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
		{
			strDuration = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
			repeatCfg.push_back(atoi(strDuration.c_str()));
			pos_last_end = pos_begin + 1;

			const int MAX_REPEAT_START_OFFSET_COUNT = 256;
			int iRepeatCount = MAX_REPEAT_START_OFFSET_COUNT;	// 2010-11-1 Chuan.li add: No such requirement in spec, but add such control to limit the data length
			while(((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
				&&(iRepeatCount--))
			{
				strStartOffset = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
				repeatCfg.push_back(atoi(strStartOffset.c_str()));
				pos_last_end = pos_begin + 1;		
			}
			if(iRepeatCount>0)//	If the end of the string is reached, not the situation that the repeat count more than the limit
			{
				strStartOffset = strLine.substr(pos_last_end, strLine.max_size());
				repeatCfg.push_back(atoi(strStartOffset.c_str()));
			}

			//	After all the data collected, push it into _timelyRepeatConfig
			_timelyRepeatConfig.push_back(repeatCfg);
		}
	}
	
	return true;
//#pragma message ( __MSGLOC__ "TODO: impl here")
//	return false;
}

//m=<media> <port> <transport> <fmt list>						//(media name and transport address)
bool RTSPSession::parseSDPLine_m(std::string& strLine, char szPropertyType)//const char* sdpLine)
{
	std::string::size_type pos_begin	= std::string::npos;
	std::string::size_type pos_last_end	= std::string::npos;
	
	switch(szPropertyType)
	{
	case 'm':	//	The basic media information
		{
				MediaPropertyData mediaProper;
				std::string strMedia, strPort, strTransport, strFmtList;
				int iPort = 0;
				if((pos_begin=strLine.find(" "))!= std::string::npos)
				{
					strMedia = strLine.substr(0, pos_begin);
					pos_last_end = pos_begin + 1;
					if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
					{
						strPort = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
						iPort = atoi(strPort.c_str());
						pos_last_end = pos_begin + 1;
						if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
						{
							strTransport = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
							pos_last_end = pos_begin + 1;
							strFmtList	 = strLine.substr(pos_last_end, strLine.max_size());

							//	All the data retrived, so add to the _mediaTypeList
							mediaProper.mediaType	= strMedia;
							mediaProper.mediaPort	= iPort;
							mediaProper.mediaTransport= strTransport;
							mediaProper.mediaFmt	=strFmtList;;

							_mediaDataList.push_back(mediaProper);

						}
					}
				}
		}
		break;
	case 'i':
		{
			int iListSize = _mediaDataList.size();
			if(iListSize<=0) return false;	// No corresponding media property. This should NOT happen.
			MediaPropertyData& pData = _mediaDataList.at(iListSize-1);
			pData.mediaTitle = strLine;
		}
		break;
	case 'c':
		{
			int iListSize = _mediaDataList.size();
			if(iListSize<=0) return false;	// No corresponding media property. This should NOT happen.
			MediaPropertyData& pData = _mediaDataList.at(iListSize-1);

			if((pos_begin=strLine.find(" "))!= std::string::npos)
			{
				pData.mediaConnNetType = strLine.substr(0, pos_begin);
				pos_last_end = pos_begin + 1;
				if((pos_begin=strLine.find(" ", pos_last_end))!= std::string::npos)
				{
					pData.mediaConnAddrType = strLine.substr(pos_last_end, (pos_begin-pos_last_end));
					pos_last_end = pos_begin + 1;

					pData.mediaConnAddr = strLine.substr(pos_last_end, strLine.max_size());
					if((pos_begin=pData.mediaConnAddr.find("/"))!= std::string::npos)
					{
						pData.mediaConnTTL = pData.mediaConnAddr.substr(pos_begin+1, pData.mediaConnAddr.max_size());
						pData.mediaConnAddr = pData.mediaConnAddr.substr(0, pos_begin);					
					}

				}
			}
		}
		break;
	case 'b':
		{
			int iListSize = _mediaDataList.size();
			if(iListSize<=0) return false;	// No corresponding media property. This should NOT happen.
			MediaPropertyData& pData = _mediaDataList.at(iListSize-1);

			if((pos_begin=strLine.find(":"))!= std::string::npos)
			{
				pData.mediaBandwidthModifier = strLine.substr(0, pos_begin);
				pos_last_end = pos_begin + 1;
				pData.mediaBandwidthValue = strLine.substr(pos_last_end, strLine.max_size());
			}
		}
		break;
	case 'k':
		{
			int iListSize = _mediaDataList.size();
			if(iListSize<=0) return false;	// No corresponding media property. This should NOT happen.
			MediaPropertyData& pData = _mediaDataList.at(iListSize-1);
			if((pos_begin=strLine.find(":"))!= std::string::npos)
			{
				pData.mediaEncryptMethod	= strLine.substr(0, pos_begin);
				pData.mediaEncryptKey		= strLine.substr(pos_begin+1, strLine.max_size());		
			}
			else
			{
				pData.mediaEncryptMethod	= strLine;
				pData.mediaEncryptKey		= "";
			}
		}
		break;
	case 'a':
		{
			int iListSize = _mediaDataList.size();
			if(iListSize<=0) return false;	// No corresponding media property. This should NOT happen.
			MediaPropertyData& pData = _mediaDataList.at(iListSize-1);
			//	Whether it is a control:value pair
			if((pos_begin=strLine.find(":"))!= std::string::npos)				
			{
				::std::string strValue = strLine.substr((pos_begin+1), strLine.max_size());
				::std::string strKey   = strLine.substr(0, pos_begin);
				pData.mediaAttrPairList[strKey] = strValue;

			}
			else
			{
				pData.mediaAttrList.push_back(strLine);
			}
		}
		break;
	default:
		;
	}

	return true;


}


}}//endof namespace
