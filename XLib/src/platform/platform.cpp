
#ifdef WIN32
	#include "win32.cpp"
#else // #ifdef WIN32
	#ifdef LINUX
		#include "linux.cpp"
	#else // #ifdef LINUX
		#error This is a Unknown platform.
	#endif // #ifdef LINUX
#endif // #ifdef WIN32
