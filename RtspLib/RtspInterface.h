// FileName : RtspInterface.h
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : Interface definition for rtsp message

#ifndef __ZQ_RTSP_COMMON_INTERFACE_H__
#define __ZQ_RTSP_COMMON_INTERFACE_H__

#include "ZQ_common_conf.h"
#include "DataCommunicatorUnite.h"
#include <string>

namespace ZQRtspCommon
{

typedef enum 
{
	RTSP_MTHD_NULL = 0, // first several are most common ones
	RTSP_MTHD_ANNOUNCE, 
	RTSP_MTHD_DESCRIBE,
	RTSP_MTHD_PLAY,
	RTSP_MTHD_RECORD,
	RTSP_MTHD_SETUP,
	RTSP_MTHD_TEARDOWN,
	RTSP_MTHD_PAUSE,
	RTSP_MTHD_GET_PARAMETER,
	RTSP_MTHD_OPTIONS,
	RTSP_MTHD_REDIRECT,
	RTSP_MTHD_SET_PARAMETER,
	RTSP_MTHD_PING, 
	RTSP_MTHD_RESPONSE,

	//some verb is used in LSC protocol
	RTSP_MTHD_STATUS,		//used in LSCP to get current streaming status
	RTSP_MTHD_UNKNOWN       // 13
} REQUEST_VerbCode;

typedef REQUEST_VerbCode RTSP_VerbCode;

/// IRtspSendMsg is used to send rtsp message. 
class IRtspSendMsg
{
public:
	/// release this request object
	virtual void release() = 0;

	/// reset this request object
	virtual void reset() = 0;

	///set rtsp message start line using string
	///@param buf the buffer to hold the start line content
	virtual void setStartline(const char *buf) = 0;

	/// set a header field
	///@param key the field name
	///@param value the field value
	virtual void setHeader(const char* key, const char* value) = 0;

	/// set the content body of the response
	/// @remark post() method will set Content-Length for you, so it mustn't set Content-Length header by
	///         call setHeader("Content-Length", "...")
	virtual void setContent(const char* content) = 0;

	/// post a server response on the given connection
	///@return bytes sent
	virtual int32 post(const std::string& strRawMsg = "") = 0;

protected:
	virtual ~IRtspSendMsg() {}
};

//class IRtspSendMsgFactory
//{
//public:
//	IRtspSendMsg* createRtspSendMsg(ZQ::DataPostHouse::IDataCommunicatorPtr communicator);
//protected:
//	virtual ~IRtspSendMsgFactory(){}
//};

/// IRtspResponse is used to receive rtsp message
class IRtspReceiveMsg
{
public:
	/// release this request object
	virtual void release() = 0;

	/// get rtsp message start line
	///@return the start line string
	virtual const std::string getStartline() const = 0; 

	/// get the request verb
	///@return the verb string
	virtual RTSP_VerbCode getVerb() const = 0;

	/// get the request content uri
	///@return the uri string
	virtual const std::string getUri() const = 0;

	/// get the rtsp protocol
	///@return the protocol string
	virtual const std::string getProtocol() const = 0;

	/// get status code 
	virtual int getStatus() const = 0;

	/// get status string
	virtual const std::string getStatusString() const = 0;

	/// get a header field
	///@param key the name of the header field
	///@return value string
	virtual const std::string getHeader(const char* key) const = 0;

	/// get the content body of the request
	///@return content string
	virtual const std::string getContent() const = 0;

	/// get the communicator object attach to this request
	virtual ZQ::DataPostHouse::IDataCommunicatorPtr getCommunicator() const = 0;

	/// get the time when receive this messagee
	virtual int64 getReceiveTime() const = 0;

	virtual std::string getRawMsg() const = 0;

protected:
	virtual ~IRtspReceiveMsg(){}
};

class IHandler
{
public:
	virtual ~IHandler(){}

	/// @return : if return false, caller must be responsible for freeing param(request) 
	///           by call request->release() and sendMsg->release()
	virtual bool HandleMsg(IRtspReceiveMsg* receiveMsg, IRtspSendMsg* sendMsg) = 0;

	virtual void onCommunicatorError(ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
	{
		communicator = NULL;
	}
};

class IRtspDak
{
public:
	///@Remark start dak to process requests, if success, stop() must be called
	virtual bool start() = 0;

	///@Remark stop dak
	virtual void stop() = 0;

	///@remark : RtspDak is responsible for freeing IHandler pointers, 
	///          this method must be called before you call start()
	virtual void registerHandler(IHandler* handler) = 0;

	virtual ZQ::DataPostHouse::DataPostHouseEnv& getDataPostHouseEnv() = 0;

	virtual ZQ::DataPostHouse::DataPostDak* getDak() = 0;

	/// release this object
	virtual void release() = 0;

protected:
	virtual ~IRtspDak(){}
};


} // end for ZQRtspCommon

#endif // end for __ZQ_RTSP_COMMON_INTERFACE_H__
