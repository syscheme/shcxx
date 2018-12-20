#include <stdio.h>
#include "..\MtCommmisc.h"
using namespace ZQ::common;
#include "../ScLog.h"
#define PACKET_SIZE 8192

ZQ::common::ScLog sclog("mfclog.txt",Log::L_DEBUG,8*1024*1024,2048,0);
ZQ::common::Log &ZQ::common::glog = sclog;

#define CLI_CFG "cli.cfg"

class TestConn: public ZQ::common::MtTcpConnection
{
public:
	virtual void OnActiveConnect()
	{
		glog(Log::L_INFO,__FUNCTION__);
		//printf("%s:connected OK\r\n",__FUNCTION__);
		char *msg=(char*)gMalloc(PACKET_SIZE);
		if(!msg) return;
		//sprintf(msg,"faint, just for test");
		send((BYTE*)msg,4096);
	}

	virtual void OnReceived(BYTE *msg, int len)
	{
		//SleepEx(20,true);
		//printf("received one msg, len is %d\r\n",len);
		send(msg,len);
	}

	virtual void OnClose(DWORD dwError, int iWsaError)
	{
		glog(Log::L_INFO,__FUNCTION__);
	}

	virtual void OnConnectFailed(DWORD dwError, int iError)
	{
		printf("Connect failed");
	}
};


class EchoCli
{
public:
	EchoCli()
	{
		InitMtTcpState(3,NULL);
		m_wPortNum=1970;
	}

	~EchoCli(){}



	bool start()
	{
		printf("started!!!");
		return create_conn();
	}

	void stop()
	{
		_conn.drop();
		SleepEx(200,TRUE);
	}
private:

	bool create_conn()
	{
		FILE *fp=fopen(CLI_CFG,"r");
		if(!fp) return false;



		ConnectOptions_t opts;
		memset(&opts,0,sizeof(opts));
		opts.dwFlags =  OPEN_RECV|MESSAGE_LENGTH_SIZE;
		opts.bOpenRecv = TRUE;
		opts.byteMessageLengthSize=4;

		FtCommAddrList_t LocalAddrs;
		LocalAddrs.dwAddrCount = 2;
		LocalAddrs.pAddrs = new FtCommAddr_t[2];
		memset(LocalAddrs.pAddrs, 0, sizeof(FtCommAddr_t)*2);

		FtCommAddrList_t RemoteAddrs;
		RemoteAddrs.dwAddrCount = 2;
		RemoteAddrs.pAddrs = new FtCommAddr_t[2];
		memset(RemoteAddrs.pAddrs, 0, sizeof(FtCommAddr_t)*2);

		//TODO: change address here!!!
		fgets(RemoteAddrs.pAddrs[0].szAddr,MAX_COMM_ADDR_LENGTH,fp);
		fgets(RemoteAddrs.pAddrs[1].szAddr,MAX_COMM_ADDR_LENGTH,fp);
		//sprintf(RemoteAddrs.pAddrs[0].szAddr,"192.168.0.66");
		//sprintf(RemoteAddrs.pAddrs[1].szAddr,"192.168.0.182");

		fgets(LocalAddrs.pAddrs[0].szAddr,MAX_COMM_ADDR_LENGTH,fp);
		fgets(LocalAddrs.pAddrs[1].szAddr,MAX_COMM_ADDR_LENGTH,fp);
		/*sprintf(LocalAddrs.pAddrs[0].szAddr,"192.168.0.183");
		sprintf(LocalAddrs.pAddrs[1].szAddr,"192.168.0.184");*/


		TYPEINST ident;
		ident.s.dwType = 23;
		ident.s.dwInst = 1;

		_conn.connectFt(m_wPortNum,&LocalAddrs,&RemoteAddrs,ident,&opts);
		glog(Log::L_INFO,__FUNCTION__);

		delete [] LocalAddrs.pAddrs;
		delete [] RemoteAddrs.pAddrs;

		return true;
	}

	unsigned short m_wPortNum;

protected:
	TestConn _conn;
};


#include <iostream>
int main(int argc, char* argv[])
{
	EchoCli svc;
	svc.start();
	SleepEx(2000,TRUE);
	svc.stop();
	printf("end here");
	return 0;
}
