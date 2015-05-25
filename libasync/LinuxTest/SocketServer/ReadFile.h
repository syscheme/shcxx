#ifndef  READFILE_H
#define  READFILE_H

#include <eventloop.h>
#include <queue>
namespace LibAsync{

typedef struct DataBuffer{
	char*   data;
	int     len;
} DATA;

class ReadFile : public Timer
{
public:
	typedef ZQ::common::Pointer<ReadFile> Ptr;
	ReadFile(ZQ::common::Log& log, std::string filename, EventLoop& loop);
	~ReadFile();

	bool getBuffer(char* data, int& len);
	bool initReadFile();
protected:
	virtual void onTimer();
private:
	void pushBuffer();
private:
	ZQ::common::Log&      _log;
	//EventLoop             _loop;
	std::string           _fileName;
	int64                 _offset;
	bool                  _fileEnd;
	int                   _fd;
	int                   _getNum;
	
	std::queue<DATA>  _dataQueue;            
	
};

}
#endif
