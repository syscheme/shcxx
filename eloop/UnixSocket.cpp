#include "UnixSocket.h"
#include "LIPC.h"

namespace ZQ {
namespace eloop {
		
// -----------------------------------------
// class UnixSocket
// -----------------------------------------
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
	processMessage(nread,buf);
}

int UnixSocket::send(const std::string& msg, int fd)
{
	std::string dest;
	encode(msg, dest);
	//printf("send msg len = %d,data=%s\n", dest.length(), dest.c_str());
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
	_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(UnixSocket, "send() sent %dB: %s"), dest.length(), dest.c_str());
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
			onError(lipcParseError, "parse error");
			return;
		}

		const char* data = _buf.data();
		len = 0;
		for(i = 0 ; i < index ; i++)
		{
			if(!isdigit(data[i]))
			{
				onError(lipcParseError,"parse error");
				return;
				//parse error
			}

			len = len * 10 + (data[i] - (char)0x30);
		}

		if (len < _buf.size()-index-2)
		{
			temp = _buf.substr(index+1,len);
			_buf = _buf.substr(index+len+2);
			OnMessage(temp);
		}
		else if(len = _buf.size()-index-2)
		{
			temp = _buf.substr(index+1,len);
			OnMessage(temp);
			_buf.clear();
		}

		break;
	}
}

}} // namespaces
