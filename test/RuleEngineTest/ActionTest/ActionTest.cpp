// This is the main DLL file.

#include "stdafx.h"
#include <Log.h>
#include <filelog.h>
#include "RuleEngineModule.h"
#include "TestAction.h"
#pragma unmanaged

BOOL APIENTRY DllMain( HINSTANCE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
	return TRUE;
}

using namespace ZQ::common;
TestAction* test = NULL;

bool Initialize(RuleEngine* engine, ZQ::common::Log* log)
{
	test = new TestAction(*engine, *log);
	return true;
}

bool UnInitialize(void)
{	
	try
	{
		if(test)
		{
			delete test;
			test = NULL;
		}
	}
	catch (...)
	{
	}
	return true;
}