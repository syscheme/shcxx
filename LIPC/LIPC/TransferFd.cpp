#include "TransferFd.h"
#include "LIPC.h"

namespace ZQ {
	namespace LIPC {
		
// -----------------------------------------
// class PipeConnection
// -----------------------------------------
void PipeConnection::OnRead(ssize_t nread, const char *buf)
{
	if (nread <= 0) {
		std::string desc = "Read error:";
		desc.append(errDesc(nread));
		onError(nread,desc.c_str());
		return;
	}
//	printf("read buf :%s\n",buf);
	processMessage(nread,buf);
}

int PipeConnection::send(std::string buf,ZQ::eloop::Handle* send_handle)
{
	std::string dest;
	encode(buf,dest);
	return write(dest.c_str(),dest.size(),send_handle);
}

int PipeConnection::sendfd(std::string buf,int fd)
{
	std::string dest;
	encode(buf,dest);
	if (fd > 0)
	{
		eloop_buf_t temp;
		temp.base = const_cast<char*>(dest.c_str());
		temp.len = dest.size();
//		return sendfd(&temp,1,fd);
	}
	return write(dest.c_str(),dest.size());
}

void PipeConnection::encode(const std::string& src,std::string& dest)
{
	unsigned long len = src.length();
    char strLen[32];

    // format of a netstring is [len]:[string]
    sprintf(strLen, "%lu:", len);
    dest.append(strLen);
    dest.append(src);
    dest.append(",");
}

void PipeConnection::processMessage(ssize_t nread, const char *buf)
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
			if(isdigit(data[i]))
			{
				len = len * 10 + (data[i] - (char)0x30);
			}
			else
			{
				onError(lipcParseError,"parse error");
				return;
				//parse error
			}
		}

		if(len < _buf.size()-index-2)
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

// -----------------------------
// class PipePassiveConn
// -----------------------------
PipePassiveConn::PipePassiveConn(Service& service)
:_service(service),_sendAck(true)
{

}

PipePassiveConn::~PipePassiveConn()
{

}

void PipePassiveConn::OnMessage(std::string& req)
{
	_service.OnMessage(req,this);
}

void PipePassiveConn::start()
{
	read_start();
	_service.addConn(this);
	printf("new pipe Passive Conn\n");
}

void PipePassiveConn::OnWrote(int status)
{
	_sendAck = true;
}

void PipePassiveConn::OnClose()
{
	_service.delConn(this);
	delete this;
}

}}
