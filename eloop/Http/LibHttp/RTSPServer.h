#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__

#include "RTSPConnection.h"

#include <set>

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_HTTP_API RTSPServer;
class ZQ_ELOOP_HTTP_API RTSPHandler;
class ZQ_ELOOP_HTTP_API RTSPPassiveConn;
class ZQ_ELOOP_HTTP_API RTSPResponse;

#define DUMMY_PROCESS_TIMEOUT (60*1000) // 1min a dummy big time

#define DEFAULT_SITE "."

// ---------------------------------------
// class RTSPResponse
// ---------------------------------------
class RTSPResponse : public RTSPMessage
{
public:
	typedef ZQ::common::Pointer<RTSPResponse> Ptr;

	RTSPResponse(RTSPServer& server, const RTSPMessage::Ptr& req);
	virtual ~RTSPResponse() {}

	void post(int statusCode, const char* errMsg = NULL, bool bAsync = true); 
	TCPConnection* getConn();

	int getTimeLeft();

private:
	RTSPServer& _server;
	RTSPMessage::Ptr _req;
	ZQ::common::Mutex _lkIsResp;
	bool		_isResp;
};

// ---------------------------------------
// class RTSPHandler
// ---------------------------------------
class RTSPHandler: public virtual ZQ::common::SharedObject
{
	friend class RTSPPassiveConn;
	friend class RTSPServer;
	// friend class IBaseApplication;
	// friend class RTSPApplication<RTSPHandler>;

public:
	typedef std::map<std::string, std::string> Properties;
	typedef ZQ::common::Pointer<RTSPHandler> Ptr;
	typedef std::map<std::string, Ptr> Map;

public: // about the session management
	// ---------------------------------------
	// subclass server-side RTSPSession
	// ---------------------------------------
	class Session : public RTSPSession
	{
	public:
		typedef ZQ::common::Pointer<Session> Ptr;
		typedef std::map<std::string, Ptr> Map;

	public:
		Session(RTSPServer& server, const std::string& id): _server(server), RTSPSession(id) {}
		virtual ~Session() {}
		virtual void destroy();

		//virtual bool	setup()    { return false; }
		//virtual bool	play()     { return false; }
		//virtual bool	pause()    { return false; }
		//virtual bool	teardown() { return false; }
	protected:
		RTSPServer&	 _server;
	};

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
		int OngoingSize() { return _cOngoings.get(); }

		// NOTE: this method may be accessed by multi threads concurrently
		virtual RTSPHandler::Ptr create(ZQ::eloop::RTSPServer& server, const ZQ::eloop::RTSPMessage::Ptr& req, const ZQ::eloop::RTSPHandler::Properties& dirProps) =0;
		virtual Session::Ptr newSession(RTSPServer& server, const char* sessId = NULL) =0;

	protected:
		RTSPHandler::Properties _appProps;

	public:
		ZQ::common::Log&        _log;

	private:
		friend class RTSPHandler;
		ZQ::common::AtomicInt _cOngoings;
	};

	typedef ZQ::common::Pointer<IBaseApplication> AppPtr;

protected: // hatched by HttpApplication
	RTSPHandler(const RTSPMessage::Ptr& req, IBaseApplication& app, RTSPServer& server, const RTSPHandler::Properties& dirProps = RTSPHandler::Properties());

	virtual ~RTSPHandler();

	virtual void	onDataSent(size_t size);
	virtual void	onDataReceived( size_t size );
	virtual void	onError( int error,const char* errorDescription ){}

	// non session-based requests
	//@return RTSP status code
	virtual RTSPMessage::ExtendedErrCode onOptions(RTSPResponse::Ptr& resp);
	virtual RTSPMessage::ExtendedErrCode onDescribe(RTSPResponse::Ptr& resp);
	virtual RTSPMessage::ExtendedErrCode onAnnounce(RTSPResponse::Ptr& resp);
	virtual RTSPMessage::ExtendedErrCode onGetParameter(RTSPResponse::Ptr& resp);
	virtual RTSPMessage::ExtendedErrCode onSetParameter(RTSPResponse::Ptr& resp);

	// session-based requests
	// the common entry
	//@return RTSP status code
	virtual RTSPMessage::ExtendedErrCode procSessionRequest(const RTSPMessage::RequestMethod method, RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
	{
		switch(method)
		{
		case RTSPMessage::mtdSETUP: return procSessionSetup(resp, sess);
		case RTSPMessage::mtdPLAY: return procSessionPlay(resp, sess);
		case RTSPMessage::mtdPAUSE: return procSessionPause(resp, sess);
		case RTSPMessage::mtdTEARDOWN: return procSessionTeardown(resp, sess);
		case RTSPMessage::mtdGET_PARAMETER: return procSessionGetParameter(resp, sess);
		case RTSPMessage::mtdSET_PARAMETER: return procSessionSetParameter(resp, sess);
		case RTSPMessage::mtdDESCRIBE: return procSessionDescribe(resp, sess);
		case RTSPMessage::mtdANNOUNCE: return procSessionAnnounce(resp, sess);
		
		case RTSPMessage::mtdUNKNOWN:
		default: break;
		}

		return RTSPMessage::rcMethodNotAllowed;
	}

	//@return RTSP status code
	virtual RTSPMessage::ExtendedErrCode procSessionSetup(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionPlay(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionPause(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionTeardown(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionAnnounce(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionDescribe(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionGetParameter(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionSetParameter(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess);

	virtual std::string mediaSDP(const std::string& mid);

	RTSPMessage::Ptr        _req;
	IBaseApplication&       _app;
	RTSPHandler::Properties _dirProps;
	RTSPServer&		        _server;
};

// ---------------------------------------
// class RTSPServer
// ---------------------------------------
class RTSPServer : public TCPServer
{
public:
	typedef RTSPHandler::Session Session;
	RTSPServer( const TCPServer::ServerConfig& conf, ZQ::common::Log& logger, uint32 maxSess = 10000);
	virtual ~RTSPServer();

 	bool mount(const std::string& uriEx, RTSPHandler::AppPtr app, const RTSPHandler::Properties& props=RTSPHandler::Properties(), const char* virtualSite =DEFAULT_SITE);
	bool unmount(const std::string& uriEx, const char* virtualSite =DEFAULT_SITE);
	void clearMounts() { _vsites.clear(); }

	RTSPHandler::Ptr createHandler(const RTSPMessage::Ptr& req, RTSPPassiveConn& conn, const std::string& virtualSite = std::string(DEFAULT_SITE));
	// RTSPHandler::Ptr findSessionHandler(const std::string& sessId); //TODO: PLAY/PAUSE et operation other than SETUP will give dummy uri, so a RTSPServer should be able to find the proper Handler by session ID

	virtual TCPConnection* createPassiveConn();

public: // about the session management
	std::string	generateSessionID();

	Session::Ptr findSession(const std::string& sessId);
	void addSession(Session::Ptr sess);
	void removeSession(const std::string& sessId);

	const Session::Map& getSessList();

	size_t getSessionCount() const;

	void setMaxSession(uint32 maxSession);
	uint32 getMaxSession() const;

	virtual void OnTimer();

	void checkReqStatus();
	void addReq(RTSPResponse::Ptr resp);
	void removeReq(RTSPResponse::Ptr resp);
	int getPendingRequest();

private:

	typedef struct _MountDir
	{
		std::string					uriEx;
		RTSPHandler::AppPtr	app;
		RTSPHandler::Properties     props;
	} MountDir;

	typedef std::vector<MountDir> MountDirs;
	typedef std::map<std::string, MountDirs> VSites;

	VSites _vsites;

	typedef std::list<RTSPResponse::Ptr>	RequestList;

private:
	ZQ::common::Mutex			_lkSessMap;
	Session::Map     			_sessMap;
	uint32						_maxSession;
	RequestList			        _awaitRequests;
	ZQ::common::Mutex			_lkReqList;
};

// ---------------------------------------
// class RTSPApplication
// ---------------------------------------
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

	// create the handler instance
	virtual RTSPHandler::Ptr create(RTSPServer& server, const RTSPMessage::Ptr& req, const RTSPHandler::Properties& dirProps)
	{ 
		return new HandlerT(req, *this, server, dirProps);
	}

	// create the server-side session
	virtual RTSPHandler::Session::Ptr newSession(RTSPServer& server, const char* sessId = NULL) 
	{
		if (sessId == NULL)
			sessId = server.generateSessionID().c_str();
		return new RTSPHandler::Session(server, sessId); 
	}
};


} } //namespace ZQ::eloop
#endif