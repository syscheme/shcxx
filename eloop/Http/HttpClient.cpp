#include "HttpClient.h"
#include <urlstr.h>


namespace ZQ {
namespace eloop {
// ---------------------------------------
// class HttpClient
// ---------------------------------------
HttpClient::HttpClient(ZQ::common::Log& logger)
	:HttpConnection(true,logger),
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
		return;
	}
	read_start();
	std::string str = _SendMsg->toRaw();
	write(str.c_str(),str.length());
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
} }//namespace ZQ::eloop