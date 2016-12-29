#include "downloadClient.h"





// ---------------------------------------
// class EmptyClient
// ---------------------------------------
EmptyClient::EmptyClient(ZQ::common::Log& logger)
:mLogger(logger)
{

}

EmptyClient::~EmptyClient()
{

}

void EmptyClient::dohttp()
{

	HttpMessage::Ptr msg = new HttpMessage(HTTP_REQUEST);
	msg->method(HTTP_POST);
	msg->url("*");

	beginRequest(msg,"http://192.168.81.28:9978/empty");
}


bool EmptyClient::onHttpMessage( const HttpMessage::Ptr msg)
{
	mResponse = msg;
	return true;
}

bool	EmptyClient::onHttpBody( const char* data, size_t size)
{
	//mRespBody.append(data,size);

	return true;
}

void	EmptyClient::onHttpComplete()
{
	
}

void	EmptyClient::onHttpError( int error )
{
	
}



// ---------------------------------------
// class DownloadClient
// ---------------------------------------
DownloadClient::DownloadClient(ZQ::common::Log& logger)
:_fp(NULL),mLogger(logger)
{

}

DownloadClient::~DownloadClient()
{

}

void DownloadClient::dohttp()
{

	HttpMessage::Ptr msg = new HttpMessage(HTTP_REQUEST);
	msg->method(HTTP_POST);
	msg->url("*");
	

	//mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(DownloadClient,"client count %d"),m_count);

	char filename[128];
	sprintf(filename,"%s","D:\\Download\\hunan.ts");

	_fp = fopen(filename,"ab+");
	if (_fp == NULL)
	{
		return;
	}

	beginRequest(msg,"http://192.168.81.28:9978/download/hunan1.ts");
}


bool DownloadClient::onHttpMessage( const HttpMessage::Ptr msg)
{
	mResponse = msg;
	return true;
}

bool DownloadClient::onHttpBody( const char* data, size_t size)
{
	//mRespBody.append(data,size);

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
DownloadThread::DownloadThread(ZQ::common::Log& logger)
	:mLogger(logger)
{

}

DownloadThread::~DownloadThread()
{

}

int DownloadThread::run()
{
	Loop loop(false);
	for(int i = 1;i< 501;i++)
	{
		EmptyClient* client = new EmptyClient(mLogger);
		client->init(loop);
		client->dohttp();
	}

	loop.run(Loop::Default);
	return 0;
}