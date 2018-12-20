#include "HttpClient.h"
#include <urlstr.h>


namespace ZQ {
namespace eloop {
// ---------------------------------------
// class HttpClient
// ---------------------------------------
HttpClient::HttpClient(ZQ::common::Log& logger)
	: HttpConnection(logger), _logger(logger), _req(NULL)
{
}

HttpClient::~HttpClient()
{
}

void HttpClient::OnConnected(ElpeError status)
{
	if (status != elpeSuccess) {
//		fprintf(stderr, "on_connect error %s\n", errDesc(status));
		std::string desc = "on_connect error:";
		desc.append(errDesc(status));
		onError(status,errDesc(status));
		return;
	}
	read_start();
	beginSend(_req);
	endSend();
}

bool HttpClient::beginRequest( HttpMessage::Ptr msg, const std::string& url)
{
	ZQ::common::URLStr urlstr(url.c_str());
	const char* host = urlstr.getHost();

	//change uri, host in msg
	msg->url( urlstr.getPathAndParam() );
	if(msg->url().empty() ) 
		msg->url("/");

//	printf("ip = %s,port = %d,url = %s \n",host,urlstr.getPort(),msg->url().c_str());

	msg->header("Host",host);

	_req = msg;
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "beginRequest() connect to[%s:%d]"),host,urlstr.getPort());
	connect4(host,urlstr.getPort());
	return true;
}

bool HttpClient::beginRequest( HttpMessage::Ptr msg, const std::string& ip, const unsigned int& port )
{
	_req = msg;
	connect4(ip.c_str(),port);
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "beginRequest() connect to[%s:%u]"), ip.c_str(), port);
	return true;
}

void HttpClient::onError( int error,const char* errorDescription )
{
	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getpeerIpPort(peerip,peerport);

	_logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient, "onError [%p] [%s:%d => %s:%d], errorCode[%d],Description[%s]"), 
		this, locip, locport, peerip, peerport,error,errorDescription);

	shutdown();
}

} }//namespace ZQ::eloop
