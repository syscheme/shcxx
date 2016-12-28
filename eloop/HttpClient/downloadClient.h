#ifndef __DOWNLOAD_CLIENT_h__
#define __DOWNLOAD_CLIENT_h__

#include "httpClient.h"
#include <NativeThread.h>

// ---------------------------------------
// class DownloadClient
// ---------------------------------------
class DownloadClient:public HttpClient
{
public:
	DownloadClient(ZQ::common::Log& logger);
	~DownloadClient();
	
	void dohttp();
	void closefile();

	virtual bool	onHttpMessage( const HttpMessage::Ptr msg);

	virtual bool	onHttpBody( const char* data, size_t size);

	virtual void	onHttpComplete();

	virtual void	onHttpError( int error );

private:
	ZQ::common::Log&	mLogger;
	HttpMessage::Ptr	mResponse;
	std::string			mRespBody;
	FILE* _fp;
};


// ---------------------------------------
// class EmptyClient
// ---------------------------------------
class EmptyClient:public HttpClient
{
public:
	EmptyClient(ZQ::common::Log& logger);
	~EmptyClient();

	void dohttp();

	virtual bool	onHttpMessage( const HttpMessage::Ptr msg);

	virtual bool	onHttpBody( const char* data, size_t size);

	virtual void	onHttpComplete();

	virtual void	onHttpError( int error );

private:
	ZQ::common::Log&	mLogger;
	HttpMessage::Ptr	mResponse;
	std::string			mRespBody;
};



// ---------------------------------------
// class DownloadThread
// ---------------------------------------
class DownloadThread:public ZQ::common::NativeThread
{
public:
	DownloadThread(ZQ::common::Log& logger);
	~DownloadThread();

	virtual int run(void);

private:
	ZQ::common::Log&	mLogger;
};

#endif