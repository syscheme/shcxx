#include "HttpClient.h"
#include <urlstr.h>


namespace ZQ {
namespace eloop {
// ---------------------------------------
// class HttpClient
// ---------------------------------------
HttpClient::HttpClient(ZQ::common::Log& logger)
	:HttpConnection(true,logger),
	_Logger(logger),
	_SendMsg(NULL)
{

}

HttpClient::~HttpClient()
{

}

void HttpClient::OnConnected(ElpeError status)
{
	if (status != elpeSuccess) {
		fprintf(stderr, "on_connect error %s\n", errDesc(status));
		onError(status,errDesc(status));
		return;
	}
	read_start();
	std::string str = _SendMsg->toRaw();
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(HttpClient,"OnConnected,send str = %s"),str.c_str());
	//printf("OnConnected,send str = %s\n",str.c_str());
	write(str.c_str(),str.length());
	_SendMsg = NULL;
}

bool HttpClient::beginRequest( HttpMessage::Ptr msg, const std::string& url)
{

//	printf("urlstr = %s\n",url.c_str());
	ZQ::common::URLStr urlstr(url.c_str());
	const char* host = urlstr.getHost();

	//change uri, host in msg
	msg->url( urlstr.getPathAndParam() );
	if(msg->url().empty() ) 
		msg->url("/");

	//printf("ip = %s,port = %d,url = %s \n",host,urlstr.getPort(),msg->url().c_str());

	msg->header("Host",host);
	msg->header("Connection", "Keep-Alive");

	_SendMsg = msg;
	connect4(host,urlstr.getPort());
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

	_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient, "onError [%p] [%s:%d => %s:%d], errorCode[%d],Description[%s]"), 
		this, locip, locport, peerip, peerport,error,errorDescription);

	shutdown();
}

} }//namespace ZQ::eloop