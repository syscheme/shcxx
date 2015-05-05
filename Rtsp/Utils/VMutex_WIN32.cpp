#include <windows.h>
#include "VMutex.h"

VMutex::VMutex()
{
	_mutex = (void *)CreateMutex(NULL, FALSE, NULL);
}

VMutex::~VMutex()
{
	CloseHandle((HANDLE )_mutex);
}

void VMutex::lock()
{
	WaitForSingleObject((HANDLE )_mutex, INFINITE);
}
	
void VMutex::unlock()
{
	ReleaseMutex((HANDLE)_mutex);
}
