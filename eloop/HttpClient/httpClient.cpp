#include "httpClient.h"
#include <urlstr.h>

HttpClient::HttpClient()
	:HttpProcessor(true),
	mSendMsg(NULL)
{

}

HttpClient::~HttpClient()
{

}

void HttpClient::OnConnected(ElpeError status)
{
	if (status != ElpeError::elpeSuccess) {
		fprintf(stderr, "on_connect error %s\n", Error(status).str());
		return;
	}
	read_start();
	std::string str = mSendMsg->toRaw();
	send(str.c_str(),str.length());
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

	mSendMsg = msg;
	connect4(host,urlstr.getPort());
	return true;
}