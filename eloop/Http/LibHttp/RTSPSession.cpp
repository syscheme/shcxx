#include "RTSPSession.h"

namespace ZQ {
	namespace eloop {

// ---------------------------------------
// class RTSPSession
// ---------------------------------------




// ---------------------------------------
// class RTSPSessionManager
// ---------------------------------------

std::string	RTSPSessionManager::generateSessionID()
{
	char buf[80];
	ZQ::common::Guid guid;
	guid.create();
	guid.toCompactIdstr(buf, sizeof(buf) -2);
	return buf;
}

RTSPSession::Ptr RTSPSessionManager::findSession(const std::string& sessId)
{
	ZQ::common::MutexGuard g(_lkSessMap);
	RTSPSession::Map::iterator it = _sessMap.find(sessId);
	if (it != _sessMap.end())
		return it->second;
	return NULL;
}

void RTSPSessionManager::addSession(RTSPSession::Ptr sess)
{
	ZQ::common::MutexGuard g(_lkSessMap);
	if (sess)
		_sessMap[sess->id()] = sess;
}

void RTSPSessionManager::removeSession(const std::string& sessId)
{
	ZQ::common::MutexGuard g(_lkSessMap);
	RTSPSession::Map::iterator it = _sessMap.find(sessId);
	if (it != _sessMap.end())
		_sessMap.erase(it);
}

size_t RTSPSessionManager::getSessionCount() const
{
	ZQ::common::MutexGuard g(_lkSessMap);
	return _sessMap.size();
}

void RTSPSessionManager::setMaxSession(uint32 maxSession)
{
	_maxSession = maxSession;
}

uint32 RTSPSessionManager::getMaxSession() const
{
	return _maxSession;
}

} }//namespace ZQ::eloop