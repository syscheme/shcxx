#include "RTSPClient.h"
#include <urlstr.h>

namespace ZQ {
	namespace eloop {

#define RTSP_MAX_CSEQ    0x0fffffff
// ---------------------------------------
// class RTSPClient
// ---------------------------------------
void RTSPClient::OnConnected(ElpeError status)
{
	if (status != ZQ::eloop::Handle::elpeSuccess)
	{
		std::string desc = "connect error:";
		desc.append(ZQ::eloop::Handle::errDesc(status));
		onError(status,desc.c_str());
		return;
	}
	read_start();
	_isConnected = true;

	for (RTSPMessage::MsgVec::iterator it = _reqList.begin();it != _reqList.end();)
	{
		sendRequest(*it);
		_reqList.erase(it++);
	}
	_reqList.clear();
}


uint RTSPClient::lastCSeq()
{
	int v = _lastCSeq.add(1);
	if (v>0 && v < RTSP_MAX_CSEQ)
		return (uint) v;

	static ZQ::common::Mutex lock;
	ZQ::common::MutexGuard g(lock);
	v = _lastCSeq.add(1);
	if (v >0 && v < RTSP_MAX_CSEQ)
		return (uint) v;

	_lastCSeq.set(1);
	v = _lastCSeq.add(1);

	return (uint) v;
}

int RTSPClient::sendRequest(RTSPMessage::Ptr req, int64 timeout)
{
	if (!_isConnected)
	{
		_reqList.push_back(req);

		std::string url = req->url();
		ZQ::common::URLStr urlstr(url.c_str());
		const char* host = urlstr.getHost();

		connect4(host,urlstr.getPort());
		return 0;
	}

	uint cseq = lastCSeq();
	req->cSeq(cseq);

	AwaitRequest ar;
	ar.req = req;
	_timeout = (timeout > 0)?timeout:_timeout;
	ar.expiration = ZQ::common::now() + _timeout;

	{
		ZQ::common::MutexGuard g(_lkAwaits);
		_awaits.insert(AwaitRequestMap::value_type(req->cSeq(), ar));
	}

	OnRequestPrepared(req);
	std::string reqStr = req->toRaw();
	int ret = write(reqStr.c_str(), reqStr.size());
	
	if (ret < 0)
	{
		OnRequestDone(cseq,ret);
		return ret;
	}

	return cseq;
}

} }//namespace ZQ::eloop