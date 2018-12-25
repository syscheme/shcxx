#include "UnixSocket.h"
// #include "LIPC.h"

namespace ZQ {
namespace eloop {

#define INFO_LEVEL_FLAG (_verboseFlags & FLG_INFO)
#define TRACE_LEVEL_FLAG (_verboseFlags & FLG_TRACE)
// ------------------------------------------------
// class Waker
// ------------------------------------------------
class Waker : public ZQ::eloop::Wakeup
{
public:
	Waker(UnixSocket& socket):_socket(socket), Wakeup(socket.loop()) {}

protected:
	virtual void OnWakedUp() {_socket.OnWakedUp();}
	// virtual void OnClose() {_socket.OnCloseAsync();}

private:
	UnixSocket& _socket;
};
		
// -----------------------------------------
// class UnixSocket
// -----------------------------------------
uint32 UnixSocket::_verboseFlags =0xffffffff;

UnixSocket::UnixSocket(Loop& loop, ZQ::common::LogWrapper& log, int ipc)
: Pipe(loop, ipc), _lipcLog(log), _waker(NULL)
{
	_waker = new Waker(*this);
#ifdef ZQ_OS_LINUX
	//Ignore SIGPIPE signal
	signal(SIGPIPE, SIG_IGN);
#endif
}

UnixSocket::~UnixSocket()
{
	closeUnixSocket();

	ZQ::common::MutexGuard gd(_lkSendMsgList);
	if (_waker)
		delete _waker;
	_waker = NULL;

	_outgoings.clear();
}

//int UnixSocket::init(Loop &loop, int ipc)
//{
//	if (_async == NULL)
//	{
//		_async = new Waker(*this);
//		_async->init(loop);
//	}
//	return ZQ::eloop::Pipe::init(loop,ipc);
//}

void UnixSocket::closeUnixSocket()
{
	//if(_async != NULL)
	//	_async->close();
	//else
		shutdown();
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
	
	if(TRACE_LEVEL_FLAG)
	{
		char hint[40];
		snprintf(hint, sizeof(hint)-2, "UnixSocket() received %dB", nread);
		_lipcLog.hexDump(ZQ::common::Log::L_DEBUG, buf, nread, hint, true);
	}

//	processMessage2(nread,buf);
	processMessage(nread,buf);
}

void UnixSocket::OnWrote(int status)
{
	ZQ::common::MutexGuard gd(_lkSendMsgList);
	if(!_outgoings.empty())
	{
		Message asyncMsg;
		asyncMsg = _outgoings.front();
		_outgoings.pop_front();

		send(asyncMsg.msg, asyncMsg.fd);
	}
}

int UnixSocket::AsyncSend(const std::string& msg, int fd)
{
	if (msg.size() >= RECV_BUF_SIZE)
	{
		_lipcLog(ZQ::common::Log::L_WARNING,CLOGFMT(UnixSocket, "AsyncSend() msg size[%d]too big,limit[%d]"), msg.size(), RECV_BUF_SIZE);
		return -1;
	}

	if (!isActive() || NULL == _waker)
	{
		_lipcLog(ZQ::common::Log::L_WARNING,CLOGFMT(UnixSocket, "AsyncSend() handle inactive or NULL waker"));
		return -1;
	}

	Message asyncMsg;
	asyncMsg.msg = msg;
	asyncMsg.fd = fd;

	{
		ZQ::common::MutexGuard gd(_lkSendMsgList);
		_outgoings.push_back(asyncMsg);
	}

	if (_waker != NULL)
		return _waker->wakeup();

	if(INFO_LEVEL_FLAG)
		_lipcLog(ZQ::common::Log::L_WARNING,CLOGFMT(UnixSocket, "AsyncSend NULL _waker"));

	return -1;
}

void UnixSocket::OnWakedUp()
{
 	int i = 1000;
	ZQ::common::MutexGuard gd(_lkSendMsgList);
	if (!isActive())
	{
		if (TRACE_LEVEL_FLAG)
			_lipcLog(ZQ::common::Log::L_WARNING,CLOGFMT(UnixSocket, "OnWakedUp clear %d outgoing messages per deactive mode"), _outgoings.size());

		_outgoings.clear();
		return;
	}

 	while (!_outgoings.empty() && i>0)
	{
		Message m = _outgoings.front();
		_outgoings.pop_front();

		int ret = send(m.msg, m.fd);
		if (ret < 0)
		{
			std::string desc = "send msg :";
			desc.append(m.msg);
			desc.append(" errDesc:");
			desc.append(errDesc(ret));
			onError(ret,desc.c_str());
		}

		i--;
	}
}

//void UnixSocket::OnCloseAsync()
//{
//	if (_async != NULL)
//	{
//		delete _async;
//		_async = NULL;
//	}
//
//	{
//		ZQ::common::MutexGuard gd(_lkSendMsgList);
//		_outgoings.clear();
//	}
//
//	shutdown();
//}
//

int UnixSocket::send(const std::string& msg, int fd)
{
	if (msg.size() >= RECV_BUF_SIZE)
	{
		_lipcLog(ZQ::common::Log::L_WARNING,CLOGFMT(UnixSocket, "send() msg size[%d]too big,limit[%d]"), msg.size(), RECV_BUF_SIZE);
		return -1;
	}
	std::string dest;
	encode(msg, dest);

	if(TRACE_LEVEL_FLAG)
	{
		char hint[40];
		snprintf(hint, sizeof(hint)-2, "UnixSocket() sent %dBfd(%d)", dest.length(), fd);
		_lipcLog.hexDump(ZQ::common::Log::L_DEBUG, dest.c_str(), dest.length(), hint, true);
	}

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
	int len = src.length();
    char strLen[32];

    // format of a netstring is [len]:[string]
    sprintf(strLen, "~%d:", len);
	dest.reserve(len + strlen(strLen) + 10);
    dest.append(strLen);
    dest.append(src);
    dest.append(",");
}

// void UnixSocket::processMessage2(ssize_t nread, const char *buf)
// {
// 	int MegLen = nread;
// 
// 	size_t len = 0;
// 	size_t index = 0; /* position of ":" */
// 	size_t i = 0; 
// 	while(MegLen > 0)
// 	{
// 		for (int i = 0; i<MegLen; i++)
// 		{
// 			if (buf[i] == ':')
// 				break;
// 		}
// 
// 		if (i >= MegLen)
// 		{
// 			_buf.clear();
// 			char errDesc[10240];
// 			snprintf(errDesc,sizeof(errDesc),"parse error:not found ':',MegLen[%d],Message[%s]",MegLen, pProcessed);
// 			onError(lipcParseError,errDesc);
// 			return;
// 		}
// 
// 		index = i;
// 		len = 0;
// 		for (int j=0; j< index; j++)
// 		{
// 			if(!isdigit(buf[j]))
// 			{
// 				_buf.clear();
// 				char errDesc[10240];
// 				snprintf(errDesc,sizeof(errDesc),"parse error:The index is not digital,MegLen[%d],Message[%s]",MegLen, buf);
// 				onError(lipcParseError,errDesc);
// 				return;
// 				//parse error
// 			}
// 
// 			len = len * 10 + (buf[j] - (char)0x30);
// 		}
// 
// 		if (len <= MegLen-index-2)
// 		{
// 			buf += (index+1);
// 			_buf.append(buf,len);
// 			buf += (len+1);
// 
// 			MegLen -= (len + index+2);
// 			_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "multi packet, temp[%s] MegLen[%d]"), _buf.c_str(), MegLen);
// 			OnMessage(_buf);
// 			_buf.clear();
// 		}
// 		else					//len > _buf.size()-index-2
// 		{
// 			buf += (index+1);
// 			_buf.append(buf,MegLen-index-1);
// 			MegLen
// 			
// 		}
// 	}
// }

void UnixSocket::processMessage(ssize_t nread, const char *buf)
{
//	int buflen = strlen(buf);
	int buflen = 0;
	while(*(buf+buflen) != '\0' && buflen <= nread) 
		buflen++;


	if (nread != buflen)
	{
		char errDesc[10240];
		snprintf(errDesc,sizeof(errDesc),"processMessage nread[%d] is not equal to buflen[%d],buf[%s]",nread,buflen,buf);
		onError(lipcParseError,errDesc);
		_buf.clear();
		return;
	}

	if (nread <= 0)
	{
		std::string desc = "processMessage error:";
		desc.append(errDesc(nread));
		onError(nread,desc.c_str());
		return;
	}

	const char* tempBuf = buf;
	if (!_buf.empty())
	{
		tempBuf = strchr(tempBuf, '~');
		if (tempBuf == NULL)
		{
			char errDesc[10240];
			snprintf(errDesc,sizeof(errDesc),"parse error:The index head is not '~',nread[%d],buf[%s]",nread,buf);
			onError(lipcParseError,errDesc);
			_buf.clear();
			return;
		}
		
		_buf.append(buf, tempBuf - buf);
		const char* data = _buf.data();
		int len = 0;
		if (sscanf(data, "~%d:", &len) != 1)
		{
			char errDesc[10240];
			snprintf(errDesc,sizeof(errDesc),"parse error:The index is not digital,nread[%d],buf[%s]",nread,_buf.c_str());
			onError(lipcParseError,errDesc);
			_buf.clear();
		}

		int index = _buf.find_first_of(":");
		if(index == std::string::npos)
		{
			char errDesc[10240];
			snprintf(errDesc,sizeof(errDesc),"parse error: missing leading-':',bufSize[%d],buf[%s]",_buf.size(),_buf.c_str());
			onError(lipcParseError,errDesc);
			_buf.clear();
		}
		else
		{
			if(len == _buf.length()-index-2)
			{
				std::string onlyMsg = _buf.substr(index+1,len);

				if(TRACE_LEVEL_FLAG)
					_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "single packet, onlyMsg[%s]"), onlyMsg.c_str());
				OnMessage(onlyMsg);
				_buf.clear();
			}
			else
			{
				char errDesc[10240];
				snprintf(errDesc,sizeof(errDesc),"parse error:len[%d] is not equal to bufSize[%d],buf[%s]",len, _buf.size(),_buf.c_str());
				onError(lipcParseError,errDesc);
				_buf.clear();
			}
		}
	}

	while(tempBuf != NULL && *tempBuf)
	{
		if (tempBuf[0] != '~')
		{
			char errDesc[10240];
			snprintf(errDesc,sizeof(errDesc),"parse error:The index head is not '~',nread[%d],buf[%s]",nread,tempBuf);
			onError(lipcParseError,errDesc);
			tempBuf = strchr(tempBuf, '~');
			continue;
		}

		const char* onlyBuf = strchr(tempBuf, ':');
		if (onlyBuf == NULL)
		{
			_buf = tempBuf;
			break;
		}

		int len = 0;
		if (sscanf(tempBuf, "~%d:", &len) != 1)
		{
			char errDesc[10240];
			snprintf(errDesc,sizeof(errDesc),"parse error:The index is not digital,nread[%d],buf[%s]",nread,tempBuf);
			onError(lipcParseError,errDesc);
			tempBuf = strchr(onlyBuf, '~');
			continue;
			//parse error
		}

		if (onlyBuf+1+len > buf + nread)
		{
			if (strchr(onlyBuf, '~') != NULL)
			{
				char errDesc[10240];
				snprintf(errDesc,sizeof(errDesc),"parse error:out of range,len[%d],nread[%d],buf[%s]",len, nread,onlyBuf);
				onError(lipcParseError,errDesc);
				tempBuf = strchr(onlyBuf, '~');
				continue;
			}
			_buf = tempBuf;
			break;
		}

		tempBuf = onlyBuf+1+len+1;
		std::string onlyMsg(onlyBuf+1, len);
		OnMessage(onlyMsg);
	}

/*
	_buf.append(buf, nread);
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
			snprintf(errDesc,sizeof(errDesc),"parse error: missing leading-':',nread[%d],buf[%s]",nread,_buf.c_str());
			onError(lipcParseError,errDesc);
			_buf.clear();
			return;
		}

		const char* data = _buf.data();
		if (data[0] != '~')
		{
			char errDesc[10240];
			snprintf(errDesc,sizeof(errDesc),"parse error:The index head is not '~',nread[%d],buf[%s]",nread,_buf.c_str());
			onError(lipcParseError,errDesc);
			_buf.clear();
			return;
			//parse error
		}
		len = 0;
		for(i = 1 ; i < index ; i++)
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

		if (len < _buf.length()-index-2)
		{
			temp = _buf.substr(index+1,len);
			_buf = _buf.substr(index+len+2);

			if(TRACE_LEVEL_FLAG)
				_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "multi packet, temp[%s] _buf[%s]"), temp.c_str(), _buf.c_str());
			OnMessage(temp);
		}
		else if(len == _buf.length()-index-2)
		{
			temp = _buf.substr(index+1,len);

			if(TRACE_LEVEL_FLAG)
				_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "single packet, temp[%s]"), temp.c_str());
			OnMessage(temp);
			_buf.clear();
		}
		else					//len > _buf.size()-index-2
			break;
	}
*/
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
