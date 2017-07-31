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
//			if (nread != elpe__EOF)
		fprintf(stderr, "Read error %s\n", errName(nread));
		close();
		return;
	}
	std::string temp = buf;
//	printf("PipeConnection read:%s\n",buf);
	decode(nread,buf);
}

int PipeConnection::send(const char* buf,size_t len,ZQ::eloop::Handle* send_handle)
{
	std::string src = buf;
	std::string dest;
	encode(src,dest);
	return write(dest.c_str(),dest.size(),send_handle);
}

void PipeConnection::encode(const std::string& src,std::string& dest)
{
	unsigned long len = src.length();
    char strLen[32];

    /* format of a netstring is [len]:[string], */
    sprintf(strLen, "%lu:", len);
    dest.append(strLen);
    dest.append(src);
    dest.append(",");
}

void PipeConnection::decode(ssize_t nread, const char *buf)
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
			//parse error
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
			//parse error
		  }
		}
		if(len < _buf.size()-index-2)
		{
			temp = _buf.substr(index+1,len);
			_buf = _buf.substr(index+len+2);
			OnRequest(temp);
		}
		else if(len = _buf.size()-index-2)
		{
			temp = _buf.substr(index+1,len);
			OnRequest(temp);
			_buf.clear();
		}
		else
			break;
	}
}
// -----------------------------
// class TransferFdClient
// -----------------------------
TransferFdClient::TransferFdClient(Dispatcher& disp)
:_disp(disp)
{
}
	
TransferFdClient::~TransferFdClient()
{
}
	 
void TransferFdClient::OnConnected(ZQ::eloop::Handle::ElpeError status)
{
	if (status != Handle::elpeSuccess) {
			fprintf(stderr, "on_connect error %s\n", Handle::errDesc(status));
			return;
		}
//	read_start();
	printf("connect suc!");
	_disp.addServant(this);
}


// -----------------------------
// class PipePassiveConn
// -----------------------------
PipePassiveConn::PipePassiveConn(TransferFdService& service)
:_service(service),_sendAck(true)
{

}

PipePassiveConn::~PipePassiveConn()
{

}

void PipePassiveConn::start()
{
	read_start();
	_service.addConn(this);
	printf("new pipe Passive Conn\n");
}

void PipePassiveConn::OnRequest(std::string& req)
{
	_service.onRequest(req,this);
}

int PipePassiveConn::send(const char* buf,size_t len)
{
	_sendAck = false;
	return PipeConnection::send(buf,len);
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

// -----------------------------
// class TransferFdService
// -----------------------------
TransferFdService::TransferFdService()
{

}

TransferFdService::~TransferFdService()
{

}

void TransferFdService::addConn(PipePassiveConn* conn)
{
	_ClientList.push_back(conn);
}

void TransferFdService::delConn(PipePassiveConn* conn)
{
	PipeClientList::iterator iter = _ClientList.begin();
	while(iter != _ClientList.end())
	{
		if (*iter == conn)
			iter = _ClientList.erase(iter);		//_PipeConn.erase(iter++);
		else
			iter++;
	}
}

void TransferFdService::doAccept(ZQ::eloop::Handle::ElpeError status)
{
	if (status != Handle::elpeSuccess) {
		fprintf(stderr, "New connection error %s\n",Handle::errDesc(status));
		return;
	}

	PipePassiveConn *client = new PipePassiveConn(*this);
	client->init(get_loop());

	if (accept(client) == 0) {
		client->start();
	}
	else {
		client->close();
	}
}

}}
