#include "TestHttp.h"
#include <TimeUtil.h>

namespace LibAsync{

	TestHttpClient::TestHttpClient(ZQ::common::Log& log, int index)
		:_log(log), _recvCount(0), _index(index), _cycleNum(1), _bDownloadComplete(false)
	{
		for (int i=0; i<8; i++)
		{
			AsyncBuffer buf;
			buf.len = 8* 1024;
			buf.base = (char*)malloc(sizeof(char)* buf.len);
			_recvBufs.push_back(buf);
		}
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] constructor entry"), _index, _cycleNum);
		//_recvBuf.len = 8 * 1024;
		//_recvBuf.base = (char*)malloc(sizeof(char)* _recvBuf.len);
	}

	TestHttpClient::~TestHttpClient()
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] destructor entry"), _index, _cycleNum);
	}

	bool TestHttpClient::onHttpBody(const char* data, size_t size)
	{
		_recvCount += size;
		
		if (_headerMsg->chunked())
		{
			if ((_recvCount / 1024) < 1024)
			{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[ client: %p ] receive data size [ %5ldB ], received total data size [ %5ldKB ]"), this, size, _recvCount/1024);
			}
			else{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[ client: %p ] receive data size [ %5ldB ], received total data size [ %.03fMB ]"), this, size, (float)_recvCount/(1024 * 1024));
			}
		}
		else{
			int64 totalLen = _headerMsg->contentLength();
			if ((_recvCount / 1024) < 1024)
			{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[ client: %p ] Received data size [ %5ldKB / %.03fMB ]"), this, _recvCount/1024, (float)totalLen/(1024 * 1024));
			}
			else{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[ client: %p ] Received data size [ %.03fMB / %.03fMB ]"), this, (float)_recvCount/(1024 * 1024), (float)totalLen/(1024 * 1024));
			}
		}

		return true;
	}

	void TestHttpClient::onHttpComplete()
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] onHttpComplete() entry"), _index, _cycleNum);
		_bDownloadComplete = true;
	}

	bool TestHttpClient::onHttpMessage(const HttpMessagePtr msg)
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] onHttpMessage() entry"), _index, _cycleNum);
		_headerMsg = msg;
		_startTime = ZQ::common::TimeUtil::now();
		return true;
	}

	void TestHttpClient::onHttpDataReceived( size_t size )
	{
		//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] onHttpDataReceived() entry"), _index, _cycleNum);
		if (_bDownloadComplete)
		{
			int64 endTime = ZQ::common::TimeUtil::now();
			int64 useTime = endTime - _startTime;
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] Got all data"), _index, _cycleNum);
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] Use Time %.03fs, Average speed %.03fKBps"), _index, _cycleNum, (float)useTime/1000, ((float)_recvCount/1024)/(useTime/1000));
			printf("\n[Index = %5d, Cycle = %2d] Got all data\n",  _index, _cycleNum);
			printf("[Index = %5d, Cycle = %2d] Use Time %.03fs, Average speed %.03fKBps\n\n", _index, _cycleNum, (float)useTime/1000, ((float)_recvCount/1024)/(useTime/1000));

			//next cycle
			/*_cycleNum++;
			_recvCount = 0;
			LibAsync::HttpMessagePtr sendMsgPtr = new LibAsync::HttpMessage(http_parser_type::HTTP_REQUEST);
			sendMsgPtr->method(http_method::HTTP_GET);
			sendMsgPtr->url("/scs/getfile?file=${file}&ic=10000000000&rate=375000&range=0-1048577");
			sendMsgPtr->keepAlive(true);

			_bDownloadComplete = false;
			if (!beginRequest(sendMsgPtr, "10.15.10.74", 12000))
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] failue to send request"), _index, _cycleNum);
			}*/
		}else{
			recvRespBody(_recvBufs);
		}
	}

	void TestHttpClient::onReqMsgSent(size_t size)
	{
		//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "onReqMsgSent() entry"));
		if (!getResponse())
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(TestHttpClient, "onReqMsgSent() getResponse failed"));
		}
	}

	void TestHttpClient::onHttpError( int error )
	{
		printf("[Index = %5d, Cycle = %2d] onHttpError() entry, [error : %d]\n", _index, _cycleNum, error);
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] onHttpError() entry, [error : %d]"), _index, _cycleNum, error);
	}
}