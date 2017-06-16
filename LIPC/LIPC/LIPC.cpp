#include "LIPC.h"
#include "TransferFd.h"
//#include <dirent.h>

namespace ZQ {
	namespace LIPC {

// ------------------------------------------------
// class Dispatcher
// ------------------------------------------------
Dispatcher::Dispatcher()
{
}

Dispatcher::~Dispatcher()
{
}

void Dispatcher::scan(const char* pathname)
{/*
	DIR *dirptr=NULL;
	struct dirent *entry;
	std::string filename;
	if((dirptr = opendir(pathname))==NULL)
	{
		printf("opendir failed!");
		return;
	}
	else
	{
		while(entry=readdir(dirptr))
		{
			if (0==strcmp(entry->d_name,".") || 0==strcmp(entry->d_name,".."))
				continue;

			filename = pathname + std::string(entry->d_name);
			printf("filename=%s\n",filename.c_str());
			TransferFdClient* client = new TransferFdClient(*this);
			client->init(get_loop());
			client->connect(filename.c_str());
		}
		closedir(dirptr);
	}*/
}

void Dispatcher::addServant(TransferFdClient* client)
{
	_servantVec.push_back(client);
}

void Dispatcher::doAccept(ZQ::eloop::Handle::ElpeError status)
{
	if (status != ZQ::eloop::Handle::elpeSuccess) {
		fprintf(stderr, "New connection error %s\n", ZQ::eloop::Handle::errDesc(status));
		return;
	}

	ZQ::eloop::TCP *temp = new ZQ::eloop::TCP();
	temp->init(get_loop());

	if (accept(temp) == 0) {

		if (_servantVec.empty())
		{
			char* dummy_buf = "can not find server";
			temp->write(dummy_buf,sizeof(dummy_buf));
			temp->shutdown();
		}
		else
		{
			char* dummy_buf = "a";
			printf("send fd\n");
			int ret = _servantVec[0]->write(dummy_buf,1,temp);
			if(ret != 0)
				printf("write ret = %d,errDesc:%s\n",ret,ZQ::eloop::Handle::errDesc(ret));
		}
	}
	else {
		temp->close();
	}
}


// -------------------------------------------------
// class Servant
// -------------------------------------------------
Servant::Servant(ServantManager& mgr)
:_Mgr(mgr)
{
}
Servant::~Servant()
{
}
void Servant::start()
{
	read_start();
	_Mgr.addServant(this);
}

void Servant::OnRead(ssize_t nread, const char *buf)
{
	if (nread < 0) {
		//			if (nread != elpe__EOF)
		fprintf(stderr, "Read error %s\n", errName(nread));
		close();
		return;
	}
	_Mgr.onRequest(buf,this);
}

void Servant::OnWrote(ZQ::eloop::Handle::ElpeError status)
{

}

void Servant::OnClose()
{
	_Mgr.delServant(this);
	delete this;
}

// -------------------------------------------------
// class JsonRpcService
// -------------------------------------------------
JsonRpcService::JsonRpcService()
{
}
JsonRpcService::~JsonRpcService()
{
}

/*void JsonRpcService::start(Loop& loop,const char* pathname)
{
	init(loop);
	bind(pathname);
	listen();
}*/


void JsonRpcService::onRequest(const char* req,Servant* conn)
{
	Arbitrary respon;
	Process(req,respon);
	std::string resp = GetString(respon).c_str();
	conn->write(resp.c_str(),resp.size());
}



// ------------------------------------------------
// class JsonRpcClient
// ------------------------------------------------
JsonRpcClient::JsonRpcClient()
:m_req(NULL)
{
}
JsonRpcClient::~JsonRpcClient()
{

}

void JsonRpcClient::beginRequest(const char* ip,int port,Request::Ptr req)
{
	connect4(ip,port);
	m_req = req;
}

void JsonRpcClient::Request(Request::Ptr req)
{
	if (req != NULL)
	{
		std::string str = req->toRaw();
		printf("send str = %s\n",str.c_str());
		write(str.c_str(),str.length());
	}
}

void JsonRpcClient::OnConnected(ZQ::eloop::Handle::ElpeError status)
{
	if (status != ZQ::eloop::Handle::elpeSuccess) {
		fprintf(stderr, "on_connect error %s\n", ZQ::eloop::Handle::errDesc(status));
		return;
	}
	read_start();
	Request(m_req);
}

void JsonRpcClient::OnRead(ssize_t nread, const char *buf)
{
	 std::cout << "Received: " << nread << std::endl;
}

void JsonRpcClient::OnWrote(ZQ::eloop::Handle::ElpeError status)
{

}




}}//ZQ::LIPC