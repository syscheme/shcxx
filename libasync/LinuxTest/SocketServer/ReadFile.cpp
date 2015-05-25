#include "ReadFile.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
namespace LibAsync{

ReadFile::ReadFile(ZQ::common::Log& log, std::string filename, EventLoop& loop)
	: _log(log),
  	_fileName(filename),
  	_offset(0),
  	_fileEnd(false),
  	_fd(-1),
	_getNum(0),
  	Timer(loop)
{

}

ReadFile::~ReadFile( )
{
	if( -1 != _fd)
	{
		::close(_fd);
		_fd = -1;
	}
	while( !_dataQueue.empty() )
	{
		DATA d = _dataQueue.front();
		_dataQueue.pop();
		if(d.data != NULL)
		{
			::free(d.data);
			d.data = NULL;
		}
	}

}

bool ReadFile::initReadFile()
{
	/*
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ReadFIle, "[%p]initReadFile() with file[%s]."),this, _fileName.c_str() );
	_fd = ::open(_fileName.c_str(), O_RDONLY | O_NONBLOCK );
	if( _fd == -1)
		return false;
	//pushBuffer();*/
	_offset = 0;
	updateTimer(2);
	return true;
}

void ReadFile::onTimer()
{
	cancelTimer();
	pushBuffer();
	if( !_fileEnd )
		updateTimer(10);
}

void ReadFile::pushBuffer()
{
	_getNum ++;
	if( _getNum > 500)
		_fileEnd = true;
	if( _dataQueue.size() > 20 )
	{
         _log(ZQ::common::Log::L_ERROR, CLOGFMT(ReadFIle, "[%p]pushBuffer()the queue max then 20."),this);
		return;
	}
	int len = 200 * 1024;
	char* buffer = (char*)malloc(sizeof(char) * len );
	if(buffer == NULL)
		assert(false && "failed to malloc mem.");
	/*
	_fd = ::open(_fileName.c_str(), O_RDONLY | O_NONBLOCK );
	if( _fd == -1)
		return ;

	if( ::lseek(_fd, _offset + 1, SEEK_SET) != (_offset + 1))
		return;


	int res = ::read(_fd, buffer, len);
	::close(_fd);
	_fd = -1;
	*/
	int cx = 0;
	while(cx < len - 10)
	{
		int res = snprintf(buffer, len - cx, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		cx += res;
	}

	//if(res > 0)
	if(true)
	{
		DATA   da;
		da.data = buffer;
		da.len = len;
		_offset +=len;
		_dataQueue.push(da);
		// _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ReadFIle, "[%p]pushBuffer() get file[%ld] size[%d]."),this, _offset, _dataQueue.size() );
	}
	/*
	else if(res == 0)
	{
		_fileEnd = true;
		 _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ReadFIle, "[%p]pushBuffer() we may reach the end of the file."),this );
	}
	else
	{
	 _log(ZQ::common::Log::L_ERROR, CLOGFMT(ReadFIle, "[%p]pushBuffer() failed to read file with erron[%d]."),this , errno);
	}*/
	//updateTimer(10);
	//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ReadFIle, "[%p]pushBuffer()."),this );
}

bool ReadFile::getBuffer(char* data, int& len)
{
	/*
	if(_getNum > 1000)
		return false;
	_getNum ++;
	int cx = 0;
	while(cx < len - 10)
	{
		int res = snprintf(data, len - cx, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		cx += res;
	}
	*/
	if( _dataQueue.empty() )
	{
		if(_fileEnd)
		{
			len = 0;
			return false;
		}
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ReadFIle, "[%p]getBuffer() there are no data to send."),this );
		len = -1;
		return true;
	}
		
	DATA   da = _dataQueue.front();
	_dataQueue.pop();
	if(len < da.len)
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ReadFIle, "[%p]getBuffer() the data buffer is not enough to get the buffer."),this);
		len = -2;
		::free(da.data);
		da.data=NULL;
		return true;
	}
	memcpy(data, da.data, da.len);
	len = da.len;
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ReadFIle, "[%p]getBuffer() get data size[%d]."),this, len );
	
	::free(da.data);
	da.data = NULL;

	return true;

}


}
