#ifndef __SLEEP_H_

#define __SLEEP_H_



#ifdef WIN32

	#include <windows.h>

	#define sleep(sec) Sleep((sec) * 1000)

#else // #ifdef WIN32

	#ifdef LINUX

		#include <unistd.h>

	#else // #ifdef LINUX

		#error This is a unknown platform

	#endif // #ifdef LINUX

#endif // #ifdef WIN32



#endif // #ifndef __SLEEP_H_

