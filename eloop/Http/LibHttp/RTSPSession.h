#ifndef __RTSP_SESSION_H__
#define __RTSP_SESSION_H__

#include <map>
#include "Guid.h"
#include "Pointer.h"
#include "Locks.h"
#include "eloop.h"

namespace ZQ {
	namespace eloop {
// ---------------------------------------
// class RTSPSession
// ---------------------------------------
class RTSPSession : public virtual ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<RTSPSession> Ptr;
	typedef std::map<std::string, Ptr> Map;

public:
	RTSPSession(const std::string& id):_id(id)
	{
	}

	void setSessionGroup(const std::string& SessionGroup) { _sessGroup = SessionGroup; }
	const std::string& getSessionGroup() const { return _sessGroup; }

	const std::string& id() const { return _id; }
	std::string streamsInfo(){ return _info; }


//	virtual int seek(int pos = -1) {};

	virtual bool	setup() { return false; }
	virtual bool	play() { return false; }
	virtual bool	pause() { return false; }
	virtual bool	teardown() { return false; }

private:
	std::string		_id;
	std::string		_info;
	std::string		_sessGroup;
};


// ---------------------------------------
// class RTSPSessionManager
// ---------------------------------------
class RTSPSessionManager//:public ZQ::eloop::Timer
{
public:
	RTSPSessionManager(uint32 maxSess = 10000):_maxSession(maxSess){}

	std::string	generateSessionID();

	RTSPSession::Ptr findSession(const std::string& sessId);
	void addSession(RTSPSession::Ptr sess);
	void removeSession(const std::string& sessId);

	size_t getSessionCount() const;

	void setMaxSession(uint32 maxSession);
	uint32 getMaxSession() const;


protected:
	virtual void OnTimer() {}


private:
	ZQ::common::Mutex			_lkSessMap;
	RTSPSession::Map			_sessMap;
	uint32						_maxSession;

};


} }//namespace ZQ::eloop
#endif