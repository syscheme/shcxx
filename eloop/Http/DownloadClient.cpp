#include "DownloadClient.h"





// ---------------------------------------
// class EmptyClient
// ---------------------------------------
EmptyClient::EmptyClient(ZQ::common::Log& logger,DownloadThread* thread)
:_Logger(logger),_Thread(thread)
{

}

EmptyClient::~EmptyClient()
{

}

void EmptyClient::dohttp(std::string& url)
{

	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::HTTP_REQUEST);
	msg->method(HTTP_POST);
	msg->url("*");

//	printf("url = %s\n",url.c_str());
	//beginRequest(msg,"http://192.168.81.28:9978/empty");
	beginRequest(msg,url);
}


void EmptyClient::OnConnected(ElpeError status)
{
	HttpClient::OnConnected(status);
	_Thread->add();
}


bool EmptyClient::onHttpMessage( const HttpMessage::Ptr msg)
{
	_Response = msg;
	return true;
}

bool	EmptyClient::onHttpBody( const char* data, size_t size)
{
	//_RespBody.append(data,size);

	return true;
}

void	EmptyClient::onHttpComplete()
{
	
}

void	EmptyClient::onHttpError( int error )
{
	_Thread->subtract();
}



// ---------------------------------------
// class DownloadClient
// ---------------------------------------
DownloadClient::DownloadClient(ZQ::common::Log& logger)
:_fp(NULL),_Logger(logger)
{

}

DownloadClient::~DownloadClient()
{

}

void DownloadClient::dohttp(std::string& url)
{

	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::HTTP_REQUEST);
	msg->method(HTTP_POST);
	msg->url("*");
	

	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(DownloadClient,"client count %d"),m_count);

	char filename[128];
	sprintf(filename,"%s","D:\\Download\\hunan.ts");
	//sprintf(filename,"/opt/TianShan/logs/hunan.ts");

	_fp = fopen(filename,"ab+");
	if (_fp == NULL)
	{
		return;
	}

	//beginRequest(msg,"http://192.168.81.28:9978/download/hunan1.ts");
	beginRequest(msg,url);
}


bool DownloadClient::onHttpMessage( const HttpMessage::Ptr msg)
{
	_Response = msg;
	return true;
}

bool DownloadClient::onHttpBody( const char* data, size_t size)
{
	//_RespBody.append(data,size);

	int ret = fwrite(data,1,size,_fp);
	if (ret != size)
	{
		closefile();
	}
	return true;
}

void DownloadClient::onHttpComplete()
{
	closefile();
}

void DownloadClient::onHttpError( int error )
{
	closefile();
}

void DownloadClient::closefile()
{
	if (_fp != NULL)
	{
		fclose(_fp);
	}	
}

// ---------------------------------------
// class DownloadThread
// ---------------------------------------
DownloadThread::DownloadThread(std::string& url,ZQ::common::Log& logger,int count,int interval,int session)
	:_Logger(logger),
	_url(url),
	_Count(count),
	_ConnectCount(0),
	_SessionCount(session),
	_IntervalTime(interval)
{

}

DownloadThread::~DownloadThread()
{

}

int DownloadThread::run()
{
	Loop loop(false);
	int port = 0;
	for(int i = 1;i<= _SessionCount;i++)
	{
		port = _Count*_SessionCount + i +50000;
		EmptyClient* client = new EmptyClient(_Logger,this);
		client->init(loop);
		_Logger(ZQ::common::Log::L_DEBUG,CLOGFMT(DownloadClient,"creat session id =%d,SessionCount=%d"),port-50000,_SessionCount);
	//	if (client->bind4("192.168.81.28",port) != 0)
	//	{
	//		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(DownloadClient,"bind port %d error!"),port);
	//	}
		client->dohttp(_url);
		Sleep(_IntervalTime);
	}

	loop.run(Loop::Default);
	return 0;
}

void DownloadThread::subtract()
{
	ZQ::common::MutexGuard gd(_LockerCount);
	_ConnectCount--;
}

void DownloadThread::add()
{
	ZQ::common::MutexGuard gd(_LockerCount);
	_ConnectCount++;
}

int DownloadThread::getCount()
{
	ZQ::common::MutexGuard gd(_LockerCount);
	return _ConnectCount;
}
