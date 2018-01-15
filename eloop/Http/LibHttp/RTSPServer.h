#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__


#include "RTSPConnection.h"
#include <SystemUtils.h>
#include <boost/regex.hpp>
#include <set>
#include "RTSPSession.h"

namespace ZQ {
namespace eloop {

#define DEFAULT_SITE "."

class RTSPServer;

// ---------------------------------------
// interface RTSPHandler
// ---------------------------------------
class RTSPHandler: public virtual ZQ::common::SharedObject
{
	friend class RTSPPassiveConn;
	friend class IBaseApplication;

public:
	typedef std::map<std::string, std::string> Properties;
	typedef ZQ::common::Pointer<RTSPHandler> Ptr;
	typedef std::map<std::string, Ptr> Map;

	virtual ~RTSPHandler() {}

	// ---------------------------------------
	// interface IBaseApplication
	// ---------------------------------------
	// present an RTSP application that respond a URI mount
	// the major funcation of the application is to instantiaze a RTSPHandler
	class IBaseApplication : public ZQ::common::SharedObject
	{
	public:
		IBaseApplication(ZQ::common::Log& logger, const RTSPHandler::Properties& appProps = RTSPHandler::Properties())
			: _log(logger), _appProps(appProps)
		{}

		virtual ~IBaseApplication() {}

		RTSPHandler::Properties getProps() const { return _appProps; }
		ZQ::common::Log& log() { return _log; }

		// NOTE: this method may be accessed by multi threads concurrently
		virtual RTSPHandler::Ptr create(const RTSPHandler::Properties& dirProps) =0;

	protected:
		RTSPHandler::Properties _appProps;
		ZQ::common::Log&        _log;
	};

	typedef ZQ::common::Pointer<IBaseApplication> AppPtr;


protected: // hatched by HttpApplication
	RTSPHandler(IBaseApplication& app, const RTSPHandler::Properties& dirProps = RTSPHandler::Properties())
		: _app(app), _dirProps(dirProps)
	{
	}

	virtual void	onDataSent(size_t size){}
	virtual void	onDataReceived( size_t size ){}


// 	virtual bool	options() = 0;
// 	virtual bool	describe() = 0;
// 
// 	virtual bool	setup() = 0;
// 	virtual bool	play() = 0;
// 	virtual bool	pause() = 0;
// 	virtual bool	teardown() = 0;


	IBaseApplication& _app;
	RTSPHandler::Properties _dirProps;

};

template <class Handler>
class RTSPApplication: public RTSPHandler::IBaseApplication
{
public:
	typedef ZQ::common::Pointer<RTSPApplication> Ptr;
	typedef Handler HandlerT;

public:
	RTSPApplication(ZQ::common::Log& logger, const RTSPHandler::Properties& appProps = RTSPHandler::Properties())
		: IBaseApplication(logger, appProps) {}
	virtual ~RTSPApplication() {}

	virtual RTSPHandler::Ptr create(const RTSPHandler::Properties& dirProps)
	{ 
		return new HandlerT(*this, dirProps);
	}
};


// ---------------------------------------
// class RTSPPassiveConn
// ---------------------------------------
class RTSPPassiveConn : public RTSPConnection
{
public:
	RTSPPassiveConn(ZQ::common::Log& log, TCPServer* tcpServer):RTSPConnection(log, tcpServer){}
	~RTSPPassiveConn(){}

	virtual void onError( int error,const char* errorDescription );

protected: // impl of RTSPParseSink
	virtual void OnResponses(RTSPMessage::MsgVec& responses);
	virtual void OnRequests(RTSPMessage::MsgVec& requests);
};


// ---------------------------------------
// class RTSPServer
// ---------------------------------------
class RTSPServer : public TCPServer
{
public:
	RTSPServer( const TCPServer::ServerConfig& conf,ZQ::common::Log& logger)
	:TCPServer(conf,logger){}
	~RTSPServer(){}

 	bool mount(const std::string& uriEx, RTSPHandler::AppPtr app, const RTSPHandler::Properties& props=RTSPHandler::Properties(), const char* virtualSite =DEFAULT_SITE);

//	HttpHandler::Ptr createHandler( const std::string& uri, HttpPassiveConn& conn, const std::string& virtualSite = std::string(DEFAULT_SITE));

	RTSPHandler::Ptr createHandler( const std::string& uri, RTSPPassiveConn& conn, const std::string& virtualSite = std::string(DEFAULT_SITE));

	RTSPSession::Ptr findSession(const std::string& sessId);
	void addSession(RTSPSession::Ptr sess);
	void removeSession(const std::string& sessId);

private:
	typedef struct _MountDir
	{
		std::string					uriEx;
		boost::regex				re;
		RTSPHandler::AppPtr	app;
		RTSPHandler::Properties     props;
	} MountDir;

	typedef std::vector<MountDir> MountDirs;
	typedef std::map<std::string, MountDirs> VSites;

	VSites _vsites;


	RTSPSession::Map			_sessMap;
};



} }//namespace ZQ::eloop
#endif