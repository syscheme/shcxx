#ifndef _TEST_HTTP_H
#define _TEST_HTTP_H
#include <http.h>
#include <FileLog.h>
#include <Locks.h>

namespace LibAsync{
	class TestHttpClient : public HttpClient
	{
	public:
		TestHttpClient(ZQ::common::Log& log, int index);
		virtual ~TestHttpClient();

	private:
		virtual void	onReqMsgSent( size_t size);

		// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
		virtual void	onHttpDataReceived( size_t size );

		virtual bool	onHttpMessage( const HttpMessagePtr msg);

		virtual bool	onHttpBody( const char* data, size_t size);

		virtual void	onHttpComplete();

		virtual void	onHttpError( int error );

	private:
		ZQ::common::Log&	_log;
		HttpMessagePtr		_headerMsg;
		unsigned long		_recvCount;

		AsyncBufferS		_recvBufs;
		int64				_startTime;
		bool				_bDownloadComplete;
		//for test
		int					_index;
		int					_cycleNum;

	};

	typedef ZQ::common::Pointer<TestHttpClient> TestHttpClientPtr;
}
#endif