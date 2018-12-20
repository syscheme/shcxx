#include "eloop_file.h"
//#include <fcntl.h>


using namespace ZQ::eloop;

class MyFileEvent : public FileEvent
{
public:
	MyFileEvent() {}
	~MyFileEvent() {}

	virtual void OnFileEvent(const char *filename, _Event events, ElpeError status)
	{
		if (status != Handle::elpeSuccess)
		{
			printf("File detect errors!\n");
			return;
		}

		if(events == fseChanged)
			printf("the file %s has changed!\n",filename);
	}
};


class MyFile : public File
{
public:
	MyFile(Loop& loop):File(loop){}
	~MyFile() {}
	virtual void OnWrite(int result)
	{
		printf("OnWrite result = %d\n",result);
		this->close();
	}

	virtual void OnOpen(int result)
	{
		printf("open end! result = %d\n",result);
		
		char* buf = "1234";
		this->write(buf,4,-1);
//		this->read(2,-1);
	}

	virtual void OnClose(int result)
	{
		printf("OnClose!\n");
	}

	virtual void OnRead(char* data,int len)
	{
		printf("OnRead data = %s,len = %d\n",data,len);
		this->close();
	}
};

class MyTimer : public Timer
{
public:
	MyTimer() {}
	~MyTimer() {}
	virtual void OnTimer()
	{
		printf("Timer test\n");
		MyFile* file = static_cast<MyFile *>(this->data);
		file->open("file.txt", File::CREAT_RDWR, File::RD_WR);
		close();
	}
};

int main()
{
	Loop loop(true);
	MyFile* file = new MyFile(loop);


	MyFileEvent fileevent;
	fileevent.init(loop);
	fileevent.start("file.txt");

	MyTimer timer;
	timer.init(loop);
	timer.start(2000,2000);
	timer.data = file;


	
	loop.run(Loop::Default);
	delete file;
	getchar();
	return 0;
}
