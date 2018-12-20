#include "Scheduler.h"
#include <time.h>

using namespace ZQ::common;

class TaskA : public ScheduleTask
{
public:
	TaskA(Scheduler& schd, DWORD inst) : ScheduleTask(schd), instId(inst)
		{
		}

	void displaytime()
		{
			struct tm *newtime;
			time_t aclock;
			time( &aclock );   // Get time in seconds
			newtime = localtime( &aclock );   // Convert time to struct tm form
			printf("TaskA [%02d]	Current time: %s", instId, asctime( newtime ) );
		}
protected:
	virtual int run(void) 
		{
			struct tm *newtime;
			time_t aclock;
			time( &aclock );   // Get time in seconds
			newtime = localtime( &aclock );   // Convert time to struct tm form
			
			printf("TaskA [%02d]	Begins ...\n", instId);
			displaytime();
			printf("TaskA [%02d]	Ends ...\n", instId);

			return 0;
		}

private:
	DWORD	instId;
};

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	NativeThreadPool	pool(5);
	Scheduler	theScheduler(pool);
	std::vector<TaskA*>	taskVec;

	// create tasks
	for(int i=0; i<10; i++)
	{
		TaskA*	pTask = new TaskA(theScheduler, i+1);
		taskVec.push_back(pTask);
	}
	
	// start tasks
	for(i=0; i<taskVec.size(); i++)
	{
		printf("Task %02d is scheduled, starting in %d msec(s)\n", i+1, 1000*(i+1));
		taskVec[i]->startWait(1000*(i+1));
		taskVec[i]->displaytime();
	}

	// task can only be updated at least 1 second before running
	SYSTEMTIME st;
	::GetSystemTime(&st);
	st.wSecond += 14;
	printf("\nTask %02d is updated, starting in %d msec(s)\n", 10, 14*1000);
	taskVec[9]->startAt(st);
	taskVec[9]->displaytime();

	printf("\nLife is full of waiting ...\n\n");

	
	// wait until they all die
	for(;;)
	{
		bool bQuit = true;
		for(int j=0; j<taskVec.size(); j++)
		{
			if(taskVec[j]->getStatus()!=NativeThread::DISABLED)
			{
				bQuit = false;
				break;
			}
		}

		if(bQuit)
			break;

		::Sleep(1000);
	}
	
	// delete tasks
	while(!taskVec.empty())
	{
		TaskA* pExTask = taskVec.back();
		delete pExTask;
		taskVec.pop_back();
	}

	getchar();
	return 0;
}