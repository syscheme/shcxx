// bufferpooltest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//BufferPool<int, 10, 3> bp;

void Printf(const char* strFormat, ...)
{
	static bool volatile s_bBusy = false;
	while (s_bBusy)
	{
		Sleep(50);
	}

	s_bBusy = true;
	
	va_list vl;
	va_start(vl, strFormat);
	vprintf(strFormat, vl);
	va_end(vl);

	s_bBusy = false;
}

unsigned long WINAPI ThreadProc1(void*)
{
	ZQ::common::BufferPool<int> bp;

	static int s_nTime = 0;

	int* p = NULL;
	while (1)
	{
		++s_nTime;
		p = bp.Alloc(INFINITE);
		Printf("Allocate: %u :1-%d\n", (unsigned)p, s_nTime);
		Sleep(rand()%200);
		if (p)
		{
			bp.Free(p);
			Printf("Free: %u :1-%d\n", (unsigned)p, s_nTime);
		}
		else
		{
			Printf("Not Free: 1-%d\n", s_nTime);
		}
		Sleep(rand()%200);
	}
	return 0;
}

unsigned long WINAPI ThreadProc2(void*)
{
	ZQ::common::BufferPool<int> bp;

	static int s_nTime = 0;

	int* p = NULL;
	while (1)
	{
		++s_nTime;
		p = bp.Alloc(INFINITE);
		Printf("Allocate: %u :2-%d\n", (unsigned)p, s_nTime);
		Sleep(rand()%200);
		if (p)
		{
			bp.Free(p);
			Printf("Free: %u :2-%d\n", (unsigned)p, s_nTime);
		}
		else
		{
			Printf("Not Free: 2-%d\n", s_nTime);
		}
		Sleep(rand()%200);
	}
	return 0;
}

unsigned long WINAPI ThreadProc3(void*)
{
	ZQ::common::BufferPool<int> bp;

	static int s_nTime = 0;

	int* p = NULL;
	while (1)
	{
		++s_nTime;
		p = bp.Alloc(INFINITE);
		Printf("Allocate: %u :3-%d\n", (unsigned)p, s_nTime);
		Sleep(rand()%200);
		if (p)
		{
			bp.Free(p);
			Printf("Free: %u :3-%d\n", (unsigned)p, s_nTime);
		}
		else
		{
			Printf("Not Free: 3-%d\n", s_nTime);
		}
		Sleep(rand()%200);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	ZQ::common::BufferPool<int> bp(5, 2);
	//bp.Debug();
	
	CreateThread(NULL, 0, ThreadProc1, NULL, 0, 0);
	CreateThread(NULL, 0, ThreadProc2, NULL, 0, 0);
	CreateThread(NULL, 0, ThreadProc3, NULL, 0, 0);

	for (;;)
	{
		//bp.Debug();
	}

	system("pause");
	return 0;
}

