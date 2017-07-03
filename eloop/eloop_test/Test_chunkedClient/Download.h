#ifndef __DOWNLOAD_CLIENT_h__
#define __DOWNLOAD_CLIENT_h__

#include "HttpMessage.h"
#include "HttpClient.h"
//#include <NativeThread.h>

namespace ZQ {
namespace eloop {
// ---------------------------------------
// class DownloadClient
// ---------------------------------------
class DownloadClient:public HttpClient
{
public:
	DownloadClient(ZQ::common::Log& logger,const std::string& filename);
	~DownloadClient();

	void dohttp(std::string& url);

	void closefile();

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);

	virtual bool	onBodyData( const char* data, size_t size);

	virtual void	onMessageCompleted();

	virtual void	onError( int error,const char* errorDescription );

private:
	ZQ::common::Log&	_Logger;
	HttpMessage::Ptr	_Response;
	std::string			_RespBody;
	FILE*				_fp;
	const std::string&			_filename;
	int					_count;
	std::string			_url;
};
} }//namespace ZQ::eloop
#endif
