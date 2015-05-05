// FileName : RtspInterfaceImpl.h
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : Implement of rtsp message interface

#ifndef __ZQ_RTSP_COMMON_INTERFACE_IMPL_H__
#define __ZQ_RTSP_COMMON_INTERFACE_IMPL_H__

#include "RtspInterface.h"
#include "RtspUtils.h"

namespace ZQRtspCommon
{

class RtspSendMsg : public IRtspSendMsg
{
public:
	RtspSendMsg(ZQ::DataPostHouse::IDataCommunicatorPtr communicator, ZQ::common::Log& log);
public:
	virtual void release();
	virtual void reset();
	virtual void setStartline(const char *buf);
	virtual void setHeader(const char* key, const char* value);
	virtual void setContent(const char* content);
	virtual int32 post(const std::string& strRawMsg = "");
private:
	RtspSendMsg(const RtspSendMsg& oths);
	RtspSendMsg& operator=(const RtspSendMsg& oths);
	~RtspSendMsg(); // only allow to generate this object by new 
private:
	ZQ::DataPostHouse::IDataCommunicatorPtr _communicator;
	ZQ::common::Log& _log;
private:
	RtspMessageT _rtspMsg;
private:
	std::string _strRemoteIP;
	std::string _strRemotePort;

};

class RtspReceiveMsg : public IRtspReceiveMsg
{
public:
	RtspReceiveMsg(RtspMessageT& rtspMsg, ZQ::DataPostHouse::IDataCommunicatorPtr communicator, 
		ZQ::common::Log& log);
public:
	/// release this request object
	virtual void release();
	virtual RTSP_VerbCode getVerb() const;
	virtual const std::string getStartline() const;
	virtual const std::string getUri() const;
	virtual const std::string getProtocol() const;
	virtual const std::string getHeader(const char* key) const;
	virtual const std::string getContent() const;
	virtual int getStatus() const;
	virtual ZQ::DataPostHouse::IDataCommunicatorPtr getCommunicator() const;
	virtual const std::string getStatusString() const;
	virtual int64 getReceiveTime() const;
	virtual std::string getRawMsg() const;
private:
	RtspReceiveMsg(const RtspReceiveMsg& oths);
	RtspReceiveMsg& operator=(const RtspReceiveMsg& oths);
	~RtspReceiveMsg(); // only allow to generate this object by new
private:
	ZQ::DataPostHouse::IDataCommunicatorPtr _communicator;
	ZQ::common::Log& _log;
	RtspMessageT _rtspMsg;
private: // request start line
	std::string _strUri;
	std::string _strProtocol;
	RTSP_VerbCode _method;
private: //response start line
	int _statusCode;
	std::string _strStatusString;
};

} // ZQRtspCommon

#endif // end for __ZQ_RTSP_ENGINE_COMMON_IMPL_H__
