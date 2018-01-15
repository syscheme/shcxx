#include "RTSPServer.h"


namespace ZQ {
namespace eloop {

// ---------------------------------------
// class RTSPPassiveConn
// ---------------------------------------
void RTSPPassiveConn::onError( int error,const char* errorDescription )
{

}

void RTSPPassiveConn::OnResponses(RTSPMessage::MsgVec& responses)
{

}

void RTSPPassiveConn::OnRequests(RTSPMessage::MsgVec& requests)
{
	for(RTSPMessage::MsgVec::iterator it = requests.begin(); it != requests.end(); it++)
	{
		RTSPMessage::Ptr msgPtr = (*it);
// 		switch(msgPtr->method())
// 		{
// 		case "d":break;
// 		}
	}
}



// ---------------------------------------
// class RTSPServer
// ---------------------------------------
bool RTSPServer::mount(const std::string& uriEx, RTSPHandler::AppPtr app, const RTSPHandler::Properties& props, const char* virtualSite)
{
	std::string vsite = (virtualSite && *virtualSite) ? virtualSite :DEFAULT_SITE;

	MountDir dir;
	try {
		dir.re.assign(uriEx);
	}
	catch( const boost::regex_error& )
	{
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer, "mount() failed to add [%s:%s] as url uriEx"), vsite.c_str(), uriEx.c_str());
		return false;
	}

	dir.uriEx = uriEx;
	dir.app = app;
	dir.props = props;

	// address the virtual site
	VSites::iterator itSite = _vsites.find(vsite);
	if (_vsites.end() == itSite)
	{
		_vsites.insert(VSites::value_type(vsite, MountDirs()));
		itSite = _vsites.find(vsite);
	}

	itSite->second.push_back(dir);
	return true;
}


RTSPHandler::Ptr RTSPServer::createHandler( const std::string& uri, RTSPPassiveConn& conn, const std::string& virtualSite)
{
	return NULL;
}

RTSPSession::Ptr RTSPServer::findSession(const std::string& sessId)
{
	RTSPSession::Map::iterator it = _sessMap.find(sessId);
	if (it != _sessMap.end())
		return it->second;
	return NULL;
}

void RTSPServer::addSession(RTSPSession::Ptr sess)
{
	if (sess)
		_sessMap[sess->id()] = sess;
}

void RTSPServer::removeSession(const std::string& sessId)
{
	RTSPSession::Map::iterator it = _sessMap.find(sessId);
	if (it != _sessMap.end())
		_sessMap.erase(it);
}

} }//namespace ZQ::eloop