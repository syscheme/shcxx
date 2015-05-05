// debug.cpp: implementation of the debug class.
//
//////////////////////////////////////////////////////////////////////

#if defined(_DEBUG)

#include "ServiceFrame.h"
#include "Locks.h"
#include "debug.h"

#pragma warning(disable:4786)
#include <map>

struct _mem_info {
	size_t			size;
	const char*		filename;
	int				line;
};

typedef std::map<void*, _mem_info>		mem_info_map_t;
typedef std::pair<void*, _mem_info>		mem_info_pair_t;

static mem_info_map_t		mem_info_map;

using namespace ZQ;
static common::Mutex	mem_info_mutex;

static void* __cdecl operator new(size_t nSize)
{
	void* r = malloc(nSize);
	return r;
}

void __cdecl operator delete(void* p, const char*  lpszFileName, 
					 int nLine, const char*  pszFunction)
{
	free(p);
}

void* __cdecl operator new(size_t nSize, const char*  lpszFileName, 
						   int nLine, const char*  pszFunction)
{
	void *res = operator new( nSize );
	MutexGuard guard(mem_info_mutex);
	//mem_info_mutex.enter();
	mem_info_map_t::iterator itor;
	itor = mem_info_map.find(res);
	if (itor != mem_info_map.end()) {
		printFault("the memory was reallocated");
		assert(false);
	} else {
		_mem_info info = {
			nSize, lpszFileName, nLine
		};

		mem_info_map.insert(mem_info_pair_t(res, info));
	}
			
#if 0
	printDebug("new() Addr = %p, size = %x, %s(%d), entry count = %d", 
		res, nSize, lpszFileName, nLine, mem_info_map.size());
#endif
	
	// mem_info_mutex.leave();
	return res;
}

void __cdecl _printDelInfo(void* p, const char*  lpszFileName, int nLine, 
						   const char* pszFunction)
{
	mem_info_mutex.enter();
	mem_info_map_t::iterator itor;
	itor = mem_info_map.find(p);
	if (itor == mem_info_map.end()) {
		mem_info_mutex.leave();
// 		printFault("the memory already was freed");
// 		assert(false);
		// DebugBreak();
		return;
	}

	mem_info_map.erase(itor);

#if 0
	printDebug("delete() Addr = %p, %s(%d), entry count = %d", 
		p, lpszFileName, nLine, mem_info_map.size());
#endif

	mem_info_mutex.leave();
}

void _dumpForDebugging()
{
	mem_info_mutex.enter();
	int size = mem_info_map.size();
	printNotice("Begin dump memory(leak count = %d):", size);
	if (size != 0) {
		mem_info_map_t::iterator itor;		
		for (itor = mem_info_map.begin(); itor != mem_info_map.end(); itor ++) {
			printNotice("Dump addr: %p, size: %x, filename: %s, line: %d", itor->first, 
				itor->second.size, itor->second.filename, itor->second.line);
		}
	}
	printNotice("End dump memory");
	mem_info_mutex.leave();
}

#endif // #if defined(_DEBUG)
