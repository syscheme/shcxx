#include "UnixSocket.h"
#include "LIPC.h"

namespace ZQ {
namespace eloop {


// ------------------------------------------------
// class AsyncSender
// ------------------------------------------------
class AsyncSender : public ZQ::eloop::Async
{
public:
	AsyncSender(UnixSocket& socket):_socket(socket){}

protected:
	virtual void OnAsync() {_socket.OnAsyncSend();}
	virtual void OnClose(){_socket.OnCloseAsync();}

private:
	UnixSocket& _socket;
};
		
// -----------------------------------------
// class UnixSocket
// -----------------------------------------
int UnixSocket::init(Loop &loop, int ipc)
{
	if (_async == NULL)
	{
		_async = new AsyncSender(*this);
		_async->init(loop);
	}
	return ZQ::eloop::Pipe::init(loop,ipc);
}

void UnixSocket::close()
{
	_async->close();
}

void UnixSocket::OnRead(ssize_t nread, const char *buf)
{
	if (nread <= 0)
	{
		std::string desc = "Read error:";
		desc.append(errDesc(nread));
		onError(nread,desc.c_str());
		return;
	}

	std::string temp;
	temp.assign(buf,nread);
	//printf("recv msg:len = %d,data:%s\n",temp.length(),temp.c_str());
	_lipcLog(ZQ::common::Log::L_DEBUG,CLOGFMT(UnixSocket, "OnRead() received %dB: %s"), temp.length(), temp.c_str());
//	parseMessage();
	processMessage(nread,buf);
}

void UnixSocket::OnWrote(int status)
{
	ZQ::common::MutexGuard gd(_lkSendMsgList);
	if(!_SendMsgList.empty())
	{
		AsyncMessage asyncMsg;
		asyncMsg = _SendMsgList.front();
		_SendMsgList.pop_front();

		send(asyncMsg.msg, asyncMsg.fd);
	}
}

int UnixSocket::AsyncSend(const std::string& msg, int fd)
{
	AsyncMessage asyncMsg;
	asyncMsg.msg = msg;
	asyncMsg.fd = fd;

	{
		ZQ::common::MutexGuard gd(_lkSendMsgList);
		_SendMsgList.push_back(asyncMsg);
	}

	return _async->send();
}

void UnixSocket::OnAsyncSend()
{
 	int i = 3;
	ZQ::common::MutexGuard gd(_lkSendMsgList);
 	while (!_SendMsgList.empty() && i>0)
	{
		AsyncMessage asyncMsg;
		asyncMsg = _SendMsgList.front();
		_SendMsgList.pop_front();

		send(asyncMsg.msg, asyncMsg.fd);
		i--;
	}
}

void UnixSocket::OnCloseAsync()
{
	if (_async != NULL)
	{
		delete _async;
		_async = NULL;
	}
	ZQ::eloop::Pipe::close();
}


int UnixSocket::send(const std::string& msg, int fd)
{
	std::string dest;
	encode(msg, dest);
	//printf("send msg len = %d,data=%s\n", dest.length(), dest.c_str());
	_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "send() sent %dB: %s"), dest.length(), dest.c_str());
	if (fd > 0)
	{
#ifdef ZQ_OS_LINUX
		eloop_buf_t temp;
		temp.base = const_cast<char*>(dest.c_str());
		temp.len = dest.size();
		return Pipe::sendfd(&temp,1,fd);
#else
		return -1;
#endif
	}

	int ret = write(dest.c_str(), dest.length());
	return ret;
}

void UnixSocket::encode(const std::string& src,std::string& dest)
{
	unsigned long len = src.length();
    char strLen[32];

    // format of a netstring is [len]:[string]
    sprintf(strLen, "%lu:", len);
    dest.append(strLen);
    dest.append(src);
    dest.append(",");
}

void UnixSocket::processMessage(ssize_t nread, const char *buf)
{
	_buf.append(buf,nread);
	size_t len = 0;
	size_t index = 0; /* position of ":" */
	size_t i = 0;
	std::string temp;  
	while(!_buf.empty())
	{
		index = _buf.find_first_of(":");
		if(index == std::string::npos)
		{
			char errDesc[10240];
			snprintf(errDesc,sizeof(errDesc),"parse error:not found ':',nread[%d],buf[%s]",nread,_buf.c_str());
			onError(lipcParseError,errDesc);
			_buf.clear();
			return;
		}

		const char* data = _buf.data();
		len = 0;
		for(i = 0 ; i < index ; i++)
		{
			if(!isdigit(data[i]))
			{
				char errDesc[10240];
				snprintf(errDesc,sizeof(errDesc),"parse error:The index is not digital,nread[%d],buf[%s]",nread,_buf.c_str());
				onError(lipcParseError,errDesc);
				_buf.clear();
				return;
				//parse error
			}

			len = len * 10 + (data[i] - (char)0x30);
		}

		if (len < _buf.size()-index-2)
		{
			temp = _buf.substr(index+1,len);
			_buf = _buf.substr(index+len+2);
			_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "multi packet, temp[%s] _buf[%s]"), temp.c_str(), _buf.c_str());
			OnMessage(temp);
		}
		else if(len == _buf.size()-index-2)
		{
			temp = _buf.substr(index+1,len);
			_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "single packet, temp[%s]"), temp.c_str());
			OnMessage(temp);
			_buf.clear();
		}
		else					//len > _buf.size()-index-2
			break;
	}
}
/*
void UnixSocket::parseMessage(ssize_t nread, const char *buf)
{
	_buf.append(buf,nread);
	size_t len = 0;
	size_t index = 0; // position of ":" 
	size_t i = 0;
	std::string temp;  
	while(!_buf.empty())
	{
		index = _buf.find_first_of(":");
		if(index == std::string::npos)
		{
			char errDesc[10240];
			snprintf(errDesc,sizeof(errDesc),"parse error:not found ':',nread[%d],buf[%s]",nread,_buf.c_str());
			onError(lipcParseError,errDesc);
			_buf.clear();
			return;
		}

		const char* data = _buf.data();
		len = 0;
		for(i = 0 ; i < index ; i++)
		{
			if(!isdigit(data[i]))
			{
				char errDesc[10240];
				snprintf(errDesc,sizeof(errDesc),"parse error:The index is not digital,nread[%d],buf[%s]",nread,_buf.c_str());
				onError(lipcParseError,errDesc);
				_buf.clear();
				return;
				//parse error
			}

			len = len * 10 + (data[i] - (char)0x30);
		}

		if (len < _buf.size()-index-2)
		{
			temp = _buf.substr(index+1,len);
			_buf = _buf.substr(index+len+2);
			_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "multi packet, temp[%s] _buf[%s]"), temp.c_str(), _buf.c_str());
			_RecvMsgList.push_back(temp);
		}
		else if(len = _buf.size()-index-2)
		{
			temp = _buf.substr(index+1,len);
			_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "single packet, temp[%s]"), temp.c_str());
			_RecvMsgList.push_back(temp);
			_buf.clear();
		}
	}
}

void UnixSocket::AsyncProcessMessage()
{
	int i = 10;
	while (!_RecvMsgList.empty() && i>0)
	{
		std::string asyncMsg = _RecvMsgList.front();
		_RecvMsgList.pop_front();
		OnMessage(asyncMsg);
		i--;
	}
}
*/

}} // namespaces
