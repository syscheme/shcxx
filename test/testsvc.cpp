#include "../MtCommmisc.h"
using namespace ZQ::common;
#include "../ScLog.h"
#include "../lock.h"
#include <assert.h>
#include <list>
class EchoSvc;
#include <iostream>

#define SVC_CFG "svc.cfg"

class TestConn: public ZQ::common::MtTcpConnection
{
public:

	TestConn(EchoSvc &server):_server(server){}


	virtual void OnReceived(BYTE *msg, int len)
	{
		//SleepEx(5,true);
		//BYTE *newmsg=gMalloc(len);
		//assert(newmsg!=NULL);
		//memcpy(newmsg,msg,len);
		//gFree(msg);
		//printf("received one msg, len is %d\r\n",len);
		send(msg,len);
	}

	virtual void OnPassiveConnect();

	virtual void OnClose(DWORD dwError, int iWsaError);
private:
	EchoSvc &_server;
};

class MyMtTcpListener: public ZQ::common::MtTcpListener
{
public:
	MyMtTcpListener(EchoSvc &server):_server(server){}

	virtual MtTcpConnection *OnCreateNewConnection(){return new TestConn(_server);}

	virtual bool OnAccept(MtTcpConnection *pconn)
	{
		TYPEINST typeinst;
		pconn->getRemoteIdent(typeinst);
		printf("remote ident is %d:%d\r\n",typeinst.s.dwInst,typeinst.s.dwType);
		glog(Log::L_INFO,__FUNCTION__);
		return true;
	}

	virtual void OnClose(){}
private:
	EchoSvc &_server;
};

typedef std::list<TestConn *>ConnList;

class EchoSvc
{
public:
	EchoSvc():_listener(*this)
	{
		InitMtTcpState();
		m_wPortNum=1970;
	}

	~EchoSvc()
	{
	}

	bool start()
	{
		return start_listen();
	}

	void add_conn(TestConn *pconn)
	{
		CriticalSectionOwner guard(_lock);
		_conns.push_back(pconn);
	}

	void del_conn(TestConn *pconn)
	{
		CriticalSectionOwner guard(_lock);
		for(ConnList::iterator it=_conns.begin();it!=_conns.end();it++)
		{
			TestConn *itpconn=*it;
			if(pconn==itpconn)
			{
				_conns.erase(it);
				return;
			}
		}
	}

	void stop()
	{
		printf("begin to drop listener...\r\n");
		_listener.drop(1);
		for(ConnList::iterator it=_conns.begin();it!=_conns.end();it++)
		{
			TestConn *pconn=*it;
			pconn->drop();
		}
		printf("begin to purge...\r\n");
		SleepEx(1000,TRUE);
		exit(0);
	}

private:
	bool start_listen()
	{
		FILE *fp=fopen(SVC_CFG,"r");
		if(!fp) return false;

		TYPEINST ident;
		ident.s.dwType = 22;
		ident.s.dwInst = 1;
		ConnectOptions_t opts;
		memset(&opts, 0, sizeof(ConnectOptions_t) );
		opts.dwFlags =  OPEN_RECV|MESSAGE_LENGTH_SIZE;
		opts.bOpenRecv = TRUE;
		opts.byteMessageLengthSize=4;

		FtCommAddrList_t LocalAddrs;
		LocalAddrs.dwAddrCount = 2;
		LocalAddrs.pAddrs = new FtCommAddr_t[2];
		memset(LocalAddrs.pAddrs, 0, sizeof(FtCommAddr_t)*2);


		//TODO: change address here!!
		//sprintf(LocalAddrs.pAddrs[0].szAddr,"192.168.0.66");
		//sprintf(LocalAddrs.pAddrs[1].szAddr,"192.168.0.182");

		fgets(LocalAddrs.pAddrs[0].szAddr,MAX_COMM_ADDR_LENGTH,fp);
		fgets(LocalAddrs.pAddrs[1].szAddr,MAX_COMM_ADDR_LENGTH,fp);

		printf("\nStarting listener %d on port \n", m_wPortNum);

		_listener.startFtListen(m_wPortNum,&LocalAddrs,ident,&opts);

		delete [] LocalAddrs.pAddrs;

		return true;
	}
	unsigned short m_wPortNum;

	MyMtTcpListener _listener;

	ConnList _conns;

	CriticalSection _lock;
};

void TestConn::OnPassiveConnect()
{
	printf("new connected!!\r\n",__FUNCTION__);
	glog(Log::L_INFO,__FUNCTION__);
	_server.add_conn(this);
}

void TestConn::OnClose(DWORD dwError, int iWsaError)
{
	glog(Log::L_INFO,__FUNCTION__);
	printf("deleted one connection!!\r\n");
	_server.del_conn(this);
	delete this;
}


ZQ::common::ScLog sclog("mflog.txt",Log::L_DEBUG,8*1024*1024,2048,0);

ZQ::common::Log &ZQ::common::glog = sclog;

int main(int argc, char* argv[])
{
	EchoSvc svc;
	svc.start();
	char bb;
	std::cin>>bb;
	svc.stop();
	printf("end here\r\n");
	PurgeMtTcpState();
	printf("purge tcp state ok!!\r\n");
	return 0;
}
