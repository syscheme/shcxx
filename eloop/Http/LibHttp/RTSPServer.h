#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__

#include "RTSPConnection.h"
#include "RTSPSession.h"

#include "TCPServer.h"

#include <set>

namespace ZQ {
namespace eloop {

#define DEFAULT_SITE "."

class RTSPServer;

// ---------------------------------------
// class RTSPHandler
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
		virtual RTSPHandler::Ptr create(RTSPServer& server, const RTSPHandler::Properties& dirProps) =0;

	protected:
		RTSPHandler::Properties _appProps;
		ZQ::common::Log&        _log;
	};

	typedef ZQ::common::Pointer<IBaseApplication> AppPtr;


protected: // hatched by HttpApplication
	RTSPHandler(IBaseApplication& app, RTSPServer& server, const RTSPHandler::Properties& dirProps = RTSPHandler::Properties())
		: _app(app), _server(server), _dirProps(dirProps)
	{
	}

	virtual void	onDataSent(size_t size){}
	virtual void	onDataReceived( size_t size ){}


	virtual void	onOptions(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onDescribe(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);

	virtual void	onAnnounce(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);

	virtual void	onSetup(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onPlay(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onPause(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onTeardown(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);

	virtual std::string mediaSDP(const std::string& mid) {return "";}
	virtual RTSPSession::Ptr createSession(const std::string& sessId) {return new RTSPSession(sessId);}

	IBaseApplication&       _app;
	RTSPHandler::Properties _dirProps;
	RTSPServer&		        _server;
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

	virtual RTSPHandler::Ptr create(RTSPServer& server,const RTSPHandler::Properties& dirProps)
	{ 
		return new HandlerT(*this, server, dirProps);
	}
};

// ---------------------------------------
// class RTSPServer
// ---------------------------------------
class RTSPServer : public TCPServer, public ZQ::eloop::RTSPSessionManager
{
public:
	RTSPServer( const TCPServer::ServerConfig& conf, ZQ::common::Log& logger)
		:TCPServer(conf,logger)
	{}

	virtual ~RTSPServer()
	{}

 	bool mount(const std::string& uriEx, RTSPHandler::AppPtr app, const RTSPHandler::Properties& props=RTSPHandler::Properties(), const char* virtualSite =DEFAULT_SITE);
	RTSPHandler::Ptr createHandler(const std::string& uri, RTSPPassiveConn& conn, const std::string& virtualSite = std::string(DEFAULT_SITE));
	RTSPHandler::Ptr findSessionHandler(const std::string& sessId); //TODO: PLAY/PAUSE et operation other than SETUP will give dummy uri, so a RTSPServer should be able to find the proper Handler by session ID

	virtual TCPConnection* createPassiveConn();

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
};

} }//namespace ZQ::eloop
#endif