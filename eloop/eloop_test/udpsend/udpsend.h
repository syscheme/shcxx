#ifndef __UDP_SEND_H__
#define __UDP_SEND_H__
#include "eloop_net.h"
#include "eloop_file.h"
#include <stdio.h>

using namespace ZQ::eloop;

class udpsend :public UDP
{
public:
	udpsend(const char* ip,int port);
	~udpsend();

	int NativeSend4(const char *buf, size_t length);
	virtual void OnSent(ElpeError status);
	virtual void OnReceived(ssize_t nread, const char *buf, const struct sockaddr *addr, unsigned flags);

private:
	const char* _destIp;
	int _destPort;
};



class MyTimer : public Timer
{
public:
	MyTimer() {}
	~MyTimer() {}
	virtual void OnTimer();
};


class FileHandle : public File
{
public:
	FileHandle(Loop& loop,udpsend* udp,size_t dataSize,int intervalTime,int startParket);
	~FileHandle();

	int FileRead();
	virtual void OnWrite(int result);
	virtual void OnOpen(int result);
	virtual void OnClose(int result);
	virtual void OnRead(char* data,int len);


private:
	udpsend* _udp;
	MyTimer _timer;
	int64_t _offset;
	size_t  _dataSize;
	int		_intervalTime;
};



class FileOperate
{
public:
	FileOperate(Loop& loop,udpsend* udp,size_t dataSize,int intervalTime);
	~FileOperate();

	int open(const char* filename);
	int read();
	void close();

private:
	FILE* _fp;
	udpsend* _udp;
	MyTimer _timer;
	int64_t _offset;
	size_t  _dataSize;
	char* _buf;
	int		_intervalTime;
};



#endif