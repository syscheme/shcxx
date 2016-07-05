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
// Ident : $Id: RTSPClient.h,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define RTSPClient class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/RTSPClient.h $
// 
// 29    2/29/16 3:11p Hui.shao
// 
// 28    2/29/16 2:34p Hui.shao
// 
// 27    2/25/16 6:09p Hui.shao
// 
// 25    2/25/16 4:23p Hui.shao
// added RTSPClient_sync
// 
// 24    11/10/15 3:00p Hui.shao
// ticket#18316 to protect multiple in comming message of a same session
// 
// 23    2/08/14 7:16p Hui.shao
// merged from V1.16
// 
// 22    12/17/12 5:44p Hui.shao
// added transfer MP2T/AVP/UDP as default for udp://
// 
// 21    10/24/12 12:10p Hongquan.zhang
// 
// 20    4/02/12 3:55p Hui.shao
// merged from V1.15.1
// 
// 25    3/29/12 11:26a Hui.shao
// expire pending request when push new in
// 
// 24    3/28/12 8:28p Hui.shao
// rolled back to keep use _lockQueueToSend
// 
// 22    3/14/12 10:43a Hui.shao
// 
// 21    3/12/12 10:43a Hui.shao
// 
// 20    3/07/12 6:39p Hui.shao
// 
// 19    9/13/11 12:02p Hui.shao
// added start/stop default sessmgr
// 
// 18    8/26/11 10:27a Hui.shao
// changed OnRequestComposed to OnRequestPrepare
// 
// 17    8/24/11 4:54p Hui.shao
// added callback OnRequestComposed() and OnRequestClean()
// 
// 16    3/07/11 10:56a Haoyuan.lu
// 
// 15    2/23/11 3:17p Haoyuan.lu
// add sendErrorCount
// 
// 14    1/27/11 3:58p Hui.shao
// 
// 13    1/26/11 11:44a Hui.shao
// added more measure log lines
// 
// 12    1/25/11 6:02p Hui.shao
// added verbose flags to print more logs
// 
// 11    1/24/11 6:39p Fei.huang
// 
// 10    1/24/11 2:42p Hui.shao
// make the sleep() linger for _messageTimeout/2 to ensure the message
// timeout would not be missed
// 
// 9     1/21/11 3:01p Haoyuan.lu
// 
// 8     1/19/11 2:50p Hui.shao
// expose the negative failures in the sendXXX() methods
// 
// 7     1/19/11 1:03p Haoyuan.lu
// 
// 6     1/19/11 11:02a Hui.shao
// inc last CSeq via atomic int
// 
// 5     1/14/11 10:15a Haoyuan.lu
// 
// 4     10-12-22 10:47 Haoyuan.lu
// 
// 3     10-12-22 10:40 Fei.huang
// + merged to linux
// 
// 2     10-12-17 11:00 Haoyuan.lu
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 11    10-11-11 10:26 Haoyuan.lu
// 
// 10    10-11-03 15:49 Haoyuan.lu
// chuan.li add SDP part
//
// 10     10-11-03 15:26 Chuan.li
// add the parse/generate get/set of SDP information
// 
// 9     10-11-02 16:12 Hui.shao
// code to desc
// 
// 8     10-10-29 13:16 Haoyuan.lu
// 
// 7     10-10-22 14:06 Hui.shao
// RTSPClient linkable
// 
// 6     10-10-22 13:44 Hui.shao
// guess by RTSP session to dispatch in coming server requests
// 
// 5     10-10-22 11:05 Hui.shao
// abstract rtsp session manager interface to port into DB in the future
// 
// 4     10-10-21 19:07 Hui.shao
// inherit RTSPRequest from RTSPMessage
// 
// 3     10-10-21 14:49 Hui.shao
// added RTSPSession
// ===========================================================================

#ifndef __RTSPClient_H__
#define __RTSPClient_H__

#include "Log.h"
#include "Pointer.h"
#include "TCPSocket.h"
#include "NativeThreadPool.h"
#include "TimeUtil.h"
#include "Pointer.h"

#include <map>
#include <vector>
#include <queue>

namespace ZQ{
namespace common{

#define DEFAULT_CLIENT_TIMEOUT  (5000) // 5sec
#define DEFAULT_CONNECT_TIMEOUT (3000) // 3sec
#define MAX_SEND_ERROR_COUNT   (10)
#define MAX_PENDING_REQUESTS   (1000)

class ZQ_COMMON_API RTSPSession;
class ZQ_COMMON_API RTSPClient;
class ZQ_COMMON_API RTSPSink;
class ZQ_COMMON_API RTSPMessage;
class ZQ_COMMON_API RTSPRequest;
class ZQ_COMMON_API RTSPClient_sync;

#define RTSP_MSG_BUF_SIZE (8*1024)

#ifndef FLAG
#  define FLAG(_BIT) (1 << _BIT)
#endif // FLAG

// -----------------------------
// class RTSPMessage
// -----------------------------
/// basic RTSP message
class RTSPMessage : public virtual SharedObject
{
public:
	typedef Pointer < RTSPMessage > Ptr;
	typedef std::map < std::string, std::string > AttrMap;
	typedef std::vector < std::string > AttrList;
	virtual ~RTSPMessage() {}

public:
	RTSPMessage(uint32 cseq=0)
	: cSeq(cseq), stampCreated(TimeUtil::now()),
	  headerCompleted(false), contentBodyRead(0), contentLenToRead(0)
	{}

	std::string startLine;
	AttrMap     headers;
	std::string contentBody;
	uint32		cSeq;
	int64       stampCreated;

	// variables to handle the incomming message:
	bool        headerCompleted;
	uint64      contentBodyRead;
	uint64      contentLenToRead;
};

// -----------------------------
// class RTSPRequest
// -----------------------------
/// RTSPRequest defines an outgoing request to the server or the ANNOUNCE received from the server 
class RTSPRequest : public RTSPMessage
{
public:
	typedef Pointer < RTSPRequest > Ptr;
	virtual ~RTSPRequest();

	RTSPRequest(RTSPClient& client, uint cseq, const char* commandName, RTSPSession* pSession =NULL,
		        uint32 flags =0, double start =-1.0f, double end =-1.0f, float scale =1.0f, const char* contentStr =NULL, void* pUserExtData=NULL);

	void close();

	RTSPClient& _client;
	std::string _commandName;
	std::string _sessGuid;
	uint32      _flags;
	double      _startPos, _endPos;
	float       _scale;
	void*       _pUserExtData; // the inherited child derived from RTSPClient may borrow this to make the RTSP message as sync calls

	int64       _stampResponsed;
};

// -----------------------------
// class RTSPSink
// -----------------------------
/// the callbacks sinking the events during the communication
class RTSPSink
{
public:
	virtual ~RTSPSink() {}	

	typedef enum _RTSPResultCode
	{
		rcOK                    = 200,
		rcContinue				= 100,
		rcBadRequest            = 400,
		rcUnauthorized          = 401,
		rcForbidden 		    = 403,
		rcObjectNotFound        = 404,
		rcNotAcceptable  		= 406,
		rcRequestTimeout        = 408,
		rcBadParameter          = 451,
		rcNotEnoughBandwidth    = 453,
		rcSessNotFound			= 454,
		rcInvalidState			= 455,
		rcInvalidRange			= 457,
		rcInternalError			= 500,
		rcNotImplement			= 501,
		rcServiceUnavail		= 503,
		rcOptionNotSupport		= 551,

		// NGOD-compatible extensions
		rcNoResponse			    = 770,
		rcAssetNotFound			    = 771,
		rcSopNotAvail			    = 772,
		rcUnknownSopGroup		    = 773,
		rcUnkownSopnames		    = 774,
		rcNotEnoughVolBandwidth	    = 775,
		rcNotEnoughNetworkBandwidth = 776,
		rcInvalidRequest			= 777,

	} RTSPResultCode;

	typedef enum _RTSPAnnounceCode
	{
		// TianShan extensions
		racStateChanged             = 8802,
		racScaleChanged             = 8801,

		// NGOD compatible codes
		racEndOfStream              = 2101,
		racTransition               = 2103,
		racBeginOfStream            = 2104,
		racPauseTimeout             = 2105,
		racTrickNoConstrained       = 2201,
		racTrickConstrained         = 2204,
		racItemSkipped              = 2205,
		racClientSessionTerminated  = 5402,
		racSessionInProgress        = 5700,
		racErrorReadingData         = 4400,
		racDownstreamFail           = 5401,
		racInternalServerError      = 5502,
		racBandwidthExceeded        = 5602,
		racServerResourceUnavail    = 5200,
		racStreamBwUnaval           = 6001,
		racDownstreamUreachable     = 6004,
		racUnableEncrpt             = 6005,

	} RTSPAnnounceCode;

	typedef enum _ReqError
	{
		Err_ConnectError,
		Err_ConnectionLost,
		Err_InvalidParams,
		Err_SendFailed,
		Err_RequestTimeout,
	} RequestError;

	/// Callback when the out-going request is failed to issue:
	///@rtspClient  The "RTSPClient" object on which the original request was issued.
	///@pReq        The request
	///@errCode     the case of failure, see RequestError
	///@errDesc     a string description of the failure
	virtual void OnRequestError(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RequestError errCode, const char* errDesc=NULL) =0;

	/// Callback when response arrived to a RTSP request:
	///@rtspClient  The "RTSPClient" object on which the original command was issued.
	///@resultCode If zero, then the command completed successfully.  If non-zero, then the command did not complete
	///         successfully, and "resultCode" indicates the error, as follows:
	///             A positive "resultCode" is a RTSP error code (for example, 404 means "not found")
	///             A negative "resultCode" indicates a socket/network error; 0-"resultCode" is the standard "errno" code.
	///@resultString  A ('\0'-terminated) string returned along with the response, or else NULL.
	virtual void OnResponse(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString) =0;

	/// Callback when an ANNOUNCE is recieved from the server
	///@rtspClient  The "RTSPClient" object on which the original request was issued.
	///@cmdName     ANNOUNCE
	///@url         the url of the ANNOUNCE
	///@pInMessage  the message
	virtual void OnServerRequest(RTSPClient& rtspClient, const char* cmdName, const char* url, RTSPMessage::Ptr& pInMessage) =0;

public:

	static std::string trim(char const* str);
	static char* nextLine(char* startOfLine, int maxByte); // this func may change the input chars of startOfLine

	static bool parseRequestLine(const std::string& line, std::string& cmdName, std::string& url, std::string& proto);
	static bool parseResponseLine(const std::string& line, uint& resultCode, std::string& resultStr, std::string& proto);
	static bool parseRangeParam(char const* paramStr, double& rangeStart, double& rangeEnd);

	/// maps the RTSPResultCode to description string
	static const char* resultCodeToStr(uint resultCode);

	/// maps the RequestError to description string
	static const char* requestErrToStr(uint reqErrCode);

	/// maps the RTSPAnnounceCode to description string
	static const char* announceCodeToStr(uint announceCode);
};

// -----------------------------
// class RTSPSession
// -----------------------------
/// RTSPSession on client-side is a stub mapping to the running session on the server-side
/// it is defined as a context about the session, you should define your own extension about the session context
/// by inheriting RTSPSession.
/// RTSPSession also sinks the session-oriented events, such as the arriable of the response of session operation
/// request and ANNOUNCE
class RTSPSession : public RTSPSink, virtual public SharedObject
{
	friend class RTSPClient;
public:
	typedef Pointer < RTSPSession > Ptr;
	typedef std::vector < Ptr > List;
	
	/// RTSPSessionManager is an aggregator of sessions in the case that the client-side has multiple alive RTSPSessions
	/// it provides a way to index the session instance by userSessId, a unique key to this client-side
	/// the built-in RTSPSessionManager also can do PING automation to keep all managed session alive on the server-side
	class IRTSPSessionManager
	{
	public:
		virtual ~IRTSPSessionManager() {}
		virtual void add(RTSPSession& sess) =0;
		virtual uint size() =0;
		virtual void remove(RTSPSession& sess) =0;
		
		virtual Ptr  lookupByGuid(const char* usrSessId) =0;
		virtual void updateIndex(RTSPSession& sess, const char* indexName="SessionId", const char* oldValue=NULL)=0;
		virtual List lookupByIndex(const char* sessionId, const char* indexName="SessionId") =0;
	};

	static IRTSPSessionManager* setSessionManager(IRTSPSessionManager* rsm=NULL);
	static void startDefaultSessionManager();
	static void stopDefaultSessionManager();

	static void updateIndex(RTSPSession* sess);

protected:
	/// the constructor only accessible by child classes
	RTSPSession(Log& log, NativeThreadPool& thrdpool, Log::loglevel_t verbosityLevel=Log::L_WARNING)
		:_log(log, verbosityLevel), _thrdpool(thrdpool)
	{}

public:

	/// constructor
	///@log the logger where the verbose information of the class will be written into
	///@thrdpool thread pool that the RTSPSession may issue background processes into
	///@filePath the uri that appends the url of the RTSPClient to order
	RTSPSession(Log& log, NativeThreadPool& thrdpool, const char* streamDestUrl, const char* filePath=NULL, Log::loglevel_t verbosityLevel=Log::L_WARNING, int timeout=600000, const char* sessGuid=NULL);

	// an RTSPSession must take destroy to delete
	virtual ~RTSPSession();

	// destroy would withdraw this session from background session manager, further leaving of Ptr would lead the deletion of this session
	virtual void destroy();

//	static Ptr newSession(Log& log, NativeThreadPool& thpool, const char* streamDestinationURL, Log::loglevel_t verbosityLevel=Log::L_WARNING, const char* sessGuid=NULL);

	// lookupBySessionId will redirect to the SessionManager
	static List lookupBySessionId(const char* sessionId);
	
	// lookupBySessionId will redirect to the SessionManager
	static Ptr lookupBySessionGuid(const char* userSessionId);

	/// set the destination where the stream should be pumped to
	///@destUrl the URL sepecifies the destination to pump to
	bool setStreamDestination(const char* destUrl, bool validate=true);

	/// reserved
	void getSessionRange(double& startTime, double& endTime) const
		{ startTime = _maxPlayStartTime; endTime = _maxPlayEndTime; }

	/// reserved
	void setSessionRange(double startTime, double endTime)
		{ _maxPlayStartTime =startTime; _maxPlayEndTime =endTime; }

	///@ return the scale of session
	float& scale() { return _scale; }

	//const char* mediaSessionType() const { return _mediaSessionType.c_str(); }
	const char* sessionName() const { return _sessionName.c_str(); }
	const char* controlUri() const { return _controlUri.c_str(); }
	const char* guid() const { return _sessGuid.c_str(); }
	int64 getStampSetup() { return _stampSetup; };

	virtual const char* sessionDescription(bool generate=false, const char* serverType =NULL);
	virtual const char* getTransport(bool generate=false, const char* serverType =NULL, bool streamOutgoing =false);

protected: // impl of RTSPSink
	friend class MessageProcessCmd;
	friend class RequestErrCmd;

	/// event of per-session request failed to issue, see RTSPSink for more details about the parameters
	/// see RTSPClient for non-session requests
	virtual void OnRequestError(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RequestError errCode, const char* errDesc=NULL);
	// dispatch OnResponse() to OnResponse_XXX() instead, not encourage to overwrite OnResponse()

	/// event of response received of per-session request, see RTSPSink for more details about the parameters
	/// see RTSPClient for non-session responses
	///@note if not override, the reponses will be dispatched to OnResponse_XXXXX() according the method type
	virtual void OnResponse(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);

	/// event of per-session ANNOUNCE received from the server, see RTSPSink for more details about the parameters
	/// see RTSPClient for non-session ANNOUNCE
	virtual void OnServerRequest(RTSPClient& rtspClient, const char* cmdName, const char* url, RTSPMessage::Ptr& pInMessage);

	// callback from the session timer
	friend class RTSPSessionManager;
	friend class RTSPSessionTimerCmd;
	virtual void OnSessionTimer();

	// new overwriteable entries, dispatched from OnResponse()
	virtual void OnResponse_SETUP(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);
	virtual void OnResponse_TEARDOWN(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);
	virtual void OnResponse_PLAY(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);
	virtual void OnResponse_PAUSE(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);
	virtual void OnResponse_GET_PARAMETER(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);
	virtual void OnResponse_SET_PARAMETER(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);

	// new overwriteable entries, dispatched from OnServerRequest()
	virtual void OnANNOUNCE(RTSPClient& rtspClient, RTSPMessage::Ptr& pInMessage);



////////////NOT TESTED, DO NOT USE/////////////////////////////////
//	2010-10-29	Chuan.li Add: a list to contain all parsed result
//#ifdef USING_SDP

public:
	//	2010-11-1	Chuan.li Add: a series of map to contain the timely repeat configurations
	typedef ::std::vector<int>				RepeatConfig;

	//	2010-11-1	Chuan.li Add: a series of map to contain the timely repeat configurations
	typedef ::std::map<int64, ::std::string> TimeZoneAdjustList;

	//	2010-11-1	Chuan.li Add: a series of map to contain the media properties
	typedef struct _MediaPropertyData
	{
		::std::string mediaType;
		int mediaPort;
		::std::string mediaTransport;
		::std::string mediaFmt;

		//i=
		::std::string mediaTitle;
		//c=
		std::string mediaConnNetType;
		std::string mediaConnAddrType;
		std::string mediaConnAddr;
		std::string mediaConnTTL;
		//b=
		std::string mediaBandwidthModifier;
		std::string mediaBandwidthValue;

		//k=
		std::string mediaEncryptMethod;
		std::string mediaEncryptKey;

		//a=
		std::map<std::string, std::string> mediaAttrPairList;
		std::vector<std::string> mediaAttrList;
	} MediaPropertyData;


public:
	bool initializeWithSDP(const char* sdpDescription);

	//	To clear the original SDP data by SDP type if in need, since may not all the data are need to be changed
	bool clearSDPdata(const char szType='*');	// Not implement now


	//	Set SDP data
	//	Note: if the data is a vector or a map, the operation is to add, the data should be add one by one!!!
	//  												But if   to get, the total data will be get out once
	//		otherwise, the operation is set/get
	//		To 'a=' values, the set operation seperate to simple and pair
	bool setSDPValue_v(const std::string& protocolVersion);
	bool setSDPValue_o(const std::string& userName, const std::string& sessId, const std::string& sessVersion, const std::string& sessNetType, const std::string& sessAddrType, const std::string& sessAddr);
	bool setSDPValue_s(const std::string& sessionName);
	bool setSDPValue_i(const std::string& sessionInformation);
	bool setSDPValue_u(const std::string& descriptionURI);
	bool setSDPValue_e(const std::string& emailAddr);
	bool setSDPValue_p(const std::string& phoneNum);
	bool setSDPValue_c(const std::string& connNetType, const std::string& connAddrType, const std::string& connAddr, const std::string& connTTL);
	bool setSDPValue_b(const std::string& bandwidthModifier, const std::string& bandwidthValue);
	bool setSDPValue_t(const std::string& starttime, const std::string& stoptime);

	bool addSDPValue_r(const RepeatConfig& rept);	
	bool addSDPValue_z(int64 iTime, const ::std::string strOffset);
	bool setSDPValue_k(const std::string& encryptMethod, const std::string& encryptKey);

	bool addSDPValue_a(const std::string& strAttr);
	bool addSDPValue_a_pair(const std::string& strKey, const std::string& strValue);
	bool addSDPValue_m(const MediaPropertyData& mediaProper);

	void addTransportEx(const std::string& key, const std::string& value);

	//	To get out the data, the whole vector/map will be the result, the result is not come out one by one
	bool getSDPValue_v(std::string& protocolVersion);
	bool getSDPValue_o(std::string& userName, std::string& sessId, std::string& sessVersion, std::string& sessNetType, std::string& sessAddrType, std::string& sessAddr);
	bool getSDPValue_s(std::string& sessionName);
	bool getSDPValue_i(std::string& sessionInformation);
	bool getSDPValue_u(std::string& descriptionURI);
	bool getSDPValue_e(std::string& emailAddr);
	bool getSDPValue_p(std::string& phoneNum);
	bool getSDPValue_c(std::string& connNetType, std::string& connAddrType, std::string& connAddr, std::string& connTTL);
	bool getSDPValue_b(std::string&	bandwidthModifier, std::string& bandwidthValue);
	bool getSDPValue_t(std::string&	starttime, std::string& stoptime);

	std::vector<RepeatConfig>&					getSDPValue_r();
	TimeZoneAdjustList&							getSDPValue_z();
	bool										getSDPValue_k(std::string& encryptMethod, std::string& encryptKey);

	std::vector<std::string>&				getSDPValue_a();
	std::map<std::string, std::string>&	getSDPValue_a_pair();
	std::string								getSDPValue_a_pair(::std::string strKey);	// To get the value of a key\value pair
	std::vector<MediaPropertyData>&			getSDPValue_m();

//END OF	2010-11-1	Chuan.li Add
protected:
	// parse v=
	std::string _protocolVersion;

	// parse o=
	std::string _userName;
	//std::string _sessId;	// Already defined in session context
	std::string _sessVersion;	//	Session Version 
	std::string _sessNetType;	//	Net type
	std::string _sessAddrType;
	std::string _sessAddr;

	// parse s=
	//	Session Name already defined

	// parse i=
	std::string _sessionInformation;

	// parse u=
	std::string _descriptionURI;
	
	// parse e=
	std::string _emailAddr;
	
	// parse p=
	std::string _phoneNum;


	// parse c=
	std::string _connNetType;
	std::string _connAddrType;
	std::string _connAddr;
	std::string _connTTL;

	// parse b=
	std::string _bandwidthModifier;
	std::string _bandwidthValue;


	// parse t=
	std::string _starttime;
	std::string _stoptime;

	// parse r=
	std::vector<RepeatConfig>	_timelyRepeatConfig;

	// parse z=
	TimeZoneAdjustList _timezoneAdjustList;

	// parse k=
	std::string _encryptMethod;
	std::string _encryptKey;

	// parse a=
	//	2010-10-29 Chuan.li add: to contain all the lines parsed from sdp data
	std::vector<std::string> _attrList;
	std::map<std::string, std::string> _attrPairList;

	// parse m=
	std::vector<MediaPropertyData> _mediaDataList; 
	
	// extension for header [transport] such as sop_name
	std::map<std::string, std::string> _transportEx;
	
protected:
	//	Parse functions
	bool parseSDPLine_a(std::string& strLine);//const char* sdpLine);
	bool parseSDPLine_o(std::string& strLine);//const char* sdpLine);
	bool parseSDPLine_c(std::string& strLine);//const char* sdpLine);
	bool parseSDPLine_b(std::string& strLine);//const char* sdpLine);
	bool parseSDPLine_z(std::string& strLine);//const char* sdpLine);
	bool parseSDPLine_k(std::string& strLine);//const char* sdpLine);
	bool parseSDPLine_t(std::string& strLine);//const char* sdpLine);
	bool parseSDPLine_r(std::string& strLine);//const char* sdpLine);
	bool parseSDPLine_m(std::string& strLine, char szPropertyType='m');//const char* sdpLine);
//#else
//	//	(???)2010-10-29 Chuan.li move it to public to test
//	bool initializeWithSDP(const char* sdpDescription);
//	
//
//	//	(???)2010-10-29 Chuan.li modify: All the following functions may not in need, one parse operation is enough
//	//  (??? the following function seems useless)
//	bool parseSDPLine(const char* input, const char*& nextLine);
//	bool parseSDPLine_s(const char* sdpLine);
//	bool parseSDPLine_i(const char* sdpLine);
//	//	(???)2010-10-29 Chuan.li modify: All the following functions may not in need, one parse operation is enough
//	bool parseSDPAttribute_type(const char* sdpLine);
//	bool parseSDPAttribute_control(const char* sdpLine);
//	bool parseSDPAttribute_range(const char* sdpLine);
//	bool parseSDPAttribute_source_filter(const char* sdpLine);
//#endif


//	static char* lookupPayloadFormat(uint rtpPayloadType,	uint& rtpTimestampFrequency, uint& numChannels);
//	static uint guessRTPTimestampFrequency(const char* mediumName,	const char* codecName);

protected:
	typedef enum _TransportType
	{
		tpt_RAW_RAW_UDP,
		tpt_RTP_AVP,
		tpt_RTP_AVP_TCP,
		tpt_MP2T_DVBC_UDP,
		tpt_MP2T_AVP_UDP,
		tpt_UNKNOWN,
	} TransportType;

	LogWrapper		  _log;
	NativeThreadPool&  _thrdpool;

	std::string   _sessGuid; // the user's session assigned by the client-side
	std::string   _sessionId;
	std::string   _filePath;
	TransportType _tpType;
	std::string   _tpStreamDestUrl;
	std::string   _tpTransport; // the value of header transport 
	tpport_t      _tpRTPPort;
	tpport_t      _tpRTCPPort;

	double      _maxPlayStartTime;
	double      _maxPlayEndTime;

	std::string   _theURL; // url as of SETUP

	// fields set from a SDP description:
	//std::string _mediaSessionType; // holds a=type value
	std::string _sessionName; // holds s=<session name> value
	std::string _sessionDescription; // holds i=<session description> value
	std::string _controlUri; // holds optional a=control: string

	// about the media variables
	RTSPRequest::AttrMap _transportParams;
	float _scale; // set from a RTSP "Scale:" header
	int64 _stampSetup; // timestamp of when the session has been SETUP
	int64 _stampLastMessage; // timestamp of the last messaging with the server about the session
	int32 _sessTimeout; // the session timeout in msec, may be set by SETUP response from the server

private:
	Mutex        _lockIncomming; // lock for processing incomming message, called by MessageProcessCmd only

	// about the session manager
	static IRTSPSessionManager* _sessMgr;
};

#define Authenticator char
#define MAX_CLIENT_CSEQ    0x0fffffff

#define RTSP_VERBOSEFLG_SEND_HEX       FLAG(0)
#define RTSP_VERBOSEFLG_RECV_HEX       FLAG(1)
#define RTSP_VERBOSEFLG_TCPTHREADPOOL  FLAG(2)
#define RTSP_VERBOSEFLG_THREADPOOL     FLAG(3)

// -----------------------------
// class RTSPClient
// -----------------------------
/// represent a tcp connection to the RTSP server, may be shared by multiple RTSPSessions
/// RTSPClient connects to the server once an outgoing request is posted, it as a built-in automation to reconnect
/// the server if the connection is lost but there is pending request to send. 
/// All the requests of RTSPClient is handled asynchronously, which means you should override the relatived callback 
/// methods OnXXXX of RTSPClient or RTSPSession to handle the response and/or ANNOUNCE
class RTSPClient : protected RTSPSink, public TCPClient
{
	friend class RTSPRequest;
	friend class RTSPSession;

public:
	RTSPClient(Log& log, NativeThreadPool& thrdpool, InetHostAddress& bindAddress, const std::string& baseURL, const char* userAgent = NULL, Log::loglevel_t verbosityLevel =Log::L_WARNING, tpport_t bindPort=0);
	virtual ~RTSPClient();

	static void setVerboseFlags(uint16 flags =0);

protected:
	// impl of RTSPSink
	friend class MessageProcessCmd;
	friend class RequestErrCmd;

	/// event of non-session request failed to issue, see RTSPSink for more details about the parameters
	/// see RTSPClient for per-session requests
	virtual void OnRequestError(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RequestError errCode, const char* errDesc=NULL);
	// dispatch OnResponse() to OnResponse_XXX() instead, not encourage to overwrite OnResponse()

	/// event of response received of non-session request, see RTSPSink for more details about the parameters
	/// see RTSPClient for per-session responses
	///@note if not override, the reponses will be dispatched to OnResponse_XXXXX() according the method type
	virtual void OnResponse(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);

	/// event of non-session ANNOUNCE received from the server, see RTSPSink for more details about the parameters
	/// see RTSPClient for per-session ANNOUNCE
	virtual void OnServerRequest(RTSPClient& rtspClient, const char* cmdName, const char* url, RTSPMessage::Ptr& pInMessage);

	// overridding of TCPSocket
	/// event when the tcp connection is established or reconnected
	virtual void OnConnected();

	/// event when the tcp connection has error or lost
	virtual void OnError();

	/// event when the data received thru the tcp connection
	virtual void OnDataArrived();

	/// event if the connection is idle for long
	virtual void OnTimeout();

	// new  callbacks
	virtual int  OnRequestPrepare(RTSPRequest::Ptr& pReq) { return 0; }
	virtual void OnRequestClean(RTSPRequest& req) {}

public: // RTSP commands

	/// issues a RTSP "DESCRIBE" command, then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendDESCRIBE(Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues a RTSP "OPTIONS" command, then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendOPTIONS(Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues a RTSP "ANNOUNCE" command (with "sdpDescription" as parameter),
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param sdpDescription to specify the SDP description if needed
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendANNOUNCE(const char* sdpDescription, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "SET_PARAMETER" command on "connection", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param paramMap a <key, value> map to specify paramters to set
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendSET_PARAMETER(const RTSPRequest::AttrMap& paramMap, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "GET_PARAMETER" command on "connection", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param parameterNames a list of keynames of parameters that wish to query for
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendGET_PARAMETER(const RTSPRequest::AttrList& parameterNames, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues a RTSP "SETUP" command, then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to setup, user must instantize an RTSPSession object before SETUP
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendSETUP(RTSPSession& session, const char* SDP=NULL, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "PLAY" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendPLAY(RTSPSession& session, double start = 0.0f, double end = -1.0f, float scale = 1.0f, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "PAUSE" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendPAUSE(RTSPSession& session, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "RECORD" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendRECORD(RTSPSession& session, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "TEARDOWN" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendTEARDOWN(RTSPSession& session, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "SET_PARAMETER" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param paramMap a <key, value> map to specify paramters to set
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendSET_PARAMETER(RTSPSession& session, const RTSPRequest::AttrMap& paramMap, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	/// issues an aggregate RTSP "GET_PARAMETER" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param parameterNames a list of keynames of parameters that wish to query for
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendGET_PARAMETER(RTSPSession& session, const RTSPRequest::AttrList& parameterNames, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	/// to specify timeouts
	///@connectTimeout timeout before establish the tcp connection
	///@messageTimeout timeout counted from when the request is being sent and when its response is received
	void setClientTimeout(int32 connectTimeout =DEFAULT_CONNECT_TIMEOUT, int32 messageTimeout =DEFAULT_CLIENT_TIMEOUT);

protected:
	NativeThreadPool&  _thrdpool;
	LogWrapper		  _log;

	std::string       _lastErr;

	// about the connection
	InetHostAddress  _serverAddress;
	tpport_t         _serverPort;
	int32            _connectTimeout;
	int32            _messageTimeout;

	// about the common variable of the client
	std::string      _userAgent;
	std::string		 _baseURL;
	std::string      _serverType;

	uint32			 _cTcpStreams;
	AtomicInt        _lastCSeq;
	AtomicInt        _sendErrorCount;

	// about the request and pending queue
	typedef std::queue < RTSPRequest::Ptr > RequestQueue;
	RequestQueue _requestsQueueToSend;
	Mutex        _lockQueueToSend;

	// about the requests that is waiting for responses
	typedef std::map <uint, RTSPRequest::Ptr> CSeqToRTSPRequestMap;
	CSeqToRTSPRequestMap _requestsAwaitResponse;
	Mutex				_lockAwaitResponse;

	// send a request thru the connection
	//@return CSeq num in [1, MAX_CLIENT_CSEQ) if succeeded, or negative if failed
	int  sendRequest(RTSPRequest::Ptr pRequest, const RTSPMessage::AttrMap& headerToOverride);

	int  sendMessage(RTSPMessage::Ptr pMessage, const RTSPMessage::AttrMap& headerToOverride, const char* msgDesc =NULL);

	RTSPMessage::Ptr _pCurrentMsg;
	char             _inCommingBuffer[RTSP_MSG_BUF_SIZE];
	int	             _inCommingByteSeen;
	Mutex	         _lockInCommingMsg;

	/// increase last CSeq and then return the new CSeq in the range [1, MAX_CLIENT_CSEQ)
	uint lastCSeq();

private:
	void _cleanupExpiredAwaitRequests(uint8 multiplyTimeout=1, char* func=""); // private use
	static uint16 _verboseFlags;
};

// -----------------------------
// class RTSPClient_sync
// -----------------------------
/// RTSPClient_sync inherits from asynchronous RTSPClient by adding a way to wait for response
/// see new API waitForResponse()
/// if specify non-zero _disconnectByTimeouts, the connection will be re-established after continuous messaging timeouts is encountered
class RTSPClient_sync : public RTSPClient
{
public:

	RTSPClient_sync(Log& log, NativeThreadPool& thrdpool, InetHostAddress& bindAddress, const std::string& baseURL, const char* userAgent =NULL, Log::loglevel_t verbosityLevel =Log::L_WARNING, tpport_t bindPort =0);
	virtual ~RTSPClient_sync() {}

	bool waitForResponse(uint32 cseq);

protected:

	// it is recommend to execute base RTSPClient::OnXXXX() in the impl when override the following events
	//	virtual void OnResponse(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString);
	//	virtual void OnServerRequest(RTSPClient& rtspClient, const char* cmdName, const char* url, RTSPMessage::Ptr& pInMessage);
	//	virtual void OnRequestError(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RequestError errCode, const char* errDesc=NULL);

	virtual int  OnRequestPrepare(RTSPRequest::Ptr& pReq);
	virtual void OnRequestClean(RTSPRequest& req);

	void wakeupByCSeq(uint32 cseq, bool success=true);

protected:
	class Event : public SharedObject
	{
	public: 
		typedef Pointer < Event > Ptr;

		Event(): _bSuccess(false) {}

		SYS::SingleObject::STATE wait(timeout_t timeout=TIMEOUT_INF) { return _so.wait(timeout); }
		void signal(bool success=true) { if (!_bSuccess) _bSuccess = success; _so.signal(); }
		bool isSuccess() const { return _bSuccess; }

	protected:
		SYS::SingleObject _so;
		bool _bSuccess;
	};

	typedef std::map<uint32, Event::Ptr> EventMap;
	EventMap          _eventMap;
	ZQ::common::Mutex _lkEventMap;

	uint			  _disconnectByTimeouts;
	uint			  _cContinuousTimeoutInConn;
	uint			  _cContinuousTimeout;
	int64             _stampLastRespInTime;
};


#ifndef MAPSET
#  define MAPSET(_MAPTYPE, _MAP, _KEY, _VAL) if (_MAP.end() ==_MAP.find(_KEY)) _MAP.insert(_MAPTYPE::value_type(_KEY, _VAL)); else _MAP[_KEY] = _VAL
#endif // MAPSET

}}//endof namespace

// brief usage:
//    RTSPClient client(logger, threadpool, bindAddress, "rtsp://192.168.11.22");
//    RTSPSession::Ptr sess = new RTSPSession(logger, threadpool, "udp://@223.12.12.12:1234", "/aaa.mpg");
//    client.sendSETUP(*sess);
//    ... wait for the overwritten of RTSPSession::OnResponse_SETUP()
//    client.sendPLAY(*sess);
//    ... wait
//    client.sendTEARDOWN(*sess);
//    sess->destroy(); //must call to destroy the session

#endif // __RTSPClient_H__
