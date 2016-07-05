#include "writeThread.h"

namespace LibAsync{

writeThread::writeThread(ZQ::common::Log& log)
	:_looprun(false), _log(log)
{

}

writeThread::~writeThread()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(writeThread, "~writeThread() entry ."));	
}

bool	writeThread::start()
{
	_looprun = true;
	return ZQ::common::NativeThread::start();
}

void writeThread::stop()
{
	_looprun = false;
	_wakeup.signal();
	SYS::sleep(60000);
}

void writeThread::addData(const std::string& fileName, const std::string& data)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(writeThread, "addData() with file[%s] entry."), fileName.c_str());
	DATA  da;
	da.fileName = fileName;
	da.data = data;
	{
		ZQ::common::MutexGuard gd(_dataMapLocker);
		_dataMap.push(da);	
	}
	_wakeup.signal();
}


bool writeThread::popData(std::string& fileName, std::string& data)
{
	//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(writeThread, "popData() entry."));
	DATA da;
	{
		ZQ::common::MutexGuard gd(_dataMapLocker);
		if (_dataMap.empty())
			return false;
		da = _dataMap.front();
		_dataMap.pop();
	}
	fileName = da.fileName;
	data = da.data;
	return true;
}

int writeThread::run()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(writeThread, "run() entry."));
	while(_looprun)
	{
		SYS::SingleObject::STATE sigState = _wakeup.wait(10000000);
		std::string fileName = "";
		std::string data = "";
		while( popData(fileName, data) )
		{
			writeFile(fileName, data);
			fileName.clear();
			data.clear();
		}
	}
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(writeThread, "run() out."));
	return 0;
}


void writeThread::writeFile(std::string& fileName, std::string& data)
{
	if (fileName.empty() || data.empty() )
		return;
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(writeThread, "writeFile()  file[%s] currBytes[%d] entry ."), fileName.c_str() );
	FILE* fd = fopen(fileName.c_str(), "a+");
	if (fd == NULL)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(writeThread, "writeFile() open file [%s] failed."), fileName.c_str());
		return;
	}
	
	if( fwrite(data.c_str(), sizeof(char), data.length(), fd) < data.length() )
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(writeThread, "writeFile() write file [%s] failed."), fileName.c_str());
	else
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(writeThread, "writeFile() write file [%s] successful."), fileName.c_str());
	fclose(fd);
	return;
}

}
