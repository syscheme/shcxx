// rwlock.h: interface for the rwlock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RWLOCK_H__0BD0826A_45F4_43DF_B1EC_6D2E749C55D7__INCLUDED_)
#define AFX_RWLOCK_H__0BD0826A_45F4_43DF_B1EC_6D2E749C55D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ZQ_common_conf.h"
#include <assert.h>

#ifndef WIN32
extern "C" {
#include <errno.h>
#include <stdlib.h>
#include <string.h>
}
#endif

#ifdef _DEBUG
#define RWLOCK_DEADLOCK_TIMEO		5000
#else
#define RWLOCK_DEADLOCK_TIMEO		10000
#endif

typedef void* _rwlock_t;
_rwlock_t ZQ_COMMON_API create_rwlock();
void ZQ_COMMON_API destroy_rwlock(_rwlock_t rwlock);

// 测试是否已经加了读锁, 如果加了写锁, 也会认为已经加了读销
int ZQ_COMMON_API rwlock_locked_read(_rwlock_t rwlock);

// 测试是否加了写锁, 如果加了读锁,就不可能再加写锁了
// 两个测试函数意义有限, 在多线程情况下,这个函数返回时
// 状态可能已经变化
int ZQ_COMMON_API rwlock_locked_write(_rwlock_t rwlock);

int ZQ_COMMON_API rwlock_lock_read(_rwlock_t rwlock, 
								   uint32 timeo = RWLOCK_DEADLOCK_TIMEO);
int ZQ_COMMON_API rwlock_unlock_read(_rwlock_t rwlock);

int ZQ_COMMON_API rwlock_lock_write(_rwlock_t rwlock, 
									uint32 timeo = RWLOCK_DEADLOCK_TIMEO);
int ZQ_COMMON_API rwlock_unlock_write(_rwlock_t rwlock);

#include <stdio.h>

#ifdef _DEBUG
#define _PRINT						printf
#define XRWLOCK_LOCKR(rwl, timeo)	(rwl).lockRead(timeo, \
										__FILE__, __LINE__)

#define	XRWLOCK_UNLOCKR(rwlock)		(rwlock).unlockRead(__FILE__, __LINE__)

#define XRWLOCK_LOCKW(rwl, timeo)	(rwl).lockWrite(timeo, \
										__FILE__, __LINE__)

#define	XRWLOCK_UNLOCKW(rwlock)		(rwlock).unlockWrite(__FILE__, __LINE__)

#else
#define _PRINT						
#define XRWLOCK_LOCKR(rwl, timeo)	(rwl).lockRead(timeo)
#define	XRWLOCK_UNLOCKR(rwlock)		(rwlock).unlockRead()

#define XRWLOCK_LOCKW(rwl, timeo)	(rwl).lockWrite(timeo)

#define	XRWLOCK_UNLOCKW(rwlock)		(rwlock).unlockWrite();

#endif

class XRWLock {
public:

	XRWLock()
	{
		_rwlock = create_rwlock();
		assert(_rwlock);
	}

	virtual ~XRWLock()
	{
		assert(_rwlock);
		destroy_rwlock(_rwlock);
	}

#ifdef _DEBUG

	void lockRead(const uint32 timeo = RWLOCK_DEADLOCK_TIMEO, 
		const char* file = __FILE__, int line = __LINE__)
	{
		assert(_rwlock);
		int result = rwlock_lock_read(_rwlock, timeo);
#ifdef WIN32
		if (!result) {
			_PRINT("%s(%d) GetLastError() = %x\n", file, line, 
				GetLastError());
			assert(false); // deadlock may exist
		}
#else
		if (!result) {
			_PRINT("%s(%d) errno = %s\n", file, line, 
				strerror(errno));
			assert(result); // deadlock may exist
		}
#endif
	}

	void unlockRead(const char* file = __FILE__, int line = __LINE__)
	{
		assert(_rwlock);
		int result = rwlock_unlock_read(_rwlock);
#ifdef WIN32
		if (!result) {
			_PRINT("%s(%d) GetLastError() = %x\n", file, line, 
				GetLastError());
			assert(result); // deadlock may exist
		}
#else
		if (!result) {
			_PRINT("%s(%d) errno = %s\n", file, line, 
				strerror(errno));
			assert(result); // deadlock may exist
		}
#endif
	}

	void lockWrite(const uint32 timeo = RWLOCK_DEADLOCK_TIMEO, 
		const char* file = __FILE__, int line = __LINE__)
	{
		assert(_rwlock);
		int result = rwlock_lock_write(_rwlock, timeo);
#ifdef WIN32
		if (!result) {
			_PRINT("%s(%d) GetLastError() = %x\n", file, line, 
				GetLastError());
			assert(result); // deadlock may exist
		}
#else
		if (!result) {
			_PRINT("%s(%d) errno = %s\n", file, line, 
				strerror(errno));
			assert(result); // deadlock may exist
		}
#endif
		
	}

	void unlockWrite(const char* file = __FILE__, int line = __LINE__)
	{
		assert(_rwlock);
		int result = rwlock_unlock_write(_rwlock);
#ifdef WIN32
		if (!result) {
			_PRINT("%s(%d) GetLastError() = %x\n", file, line, 
				GetLastError());
			assert(result); // deadlock may exist
		}
#else
		if (!result) {
			_PRINT("%s(%d) errno = %s\n", file, line, 
				strerror(errno));
			assert(result); // deadlock may exist
		}
#endif
	}

#else // #ifdef _DEBUG

	void lockRead(const uint32 timeo = RWLOCK_DEADLOCK_TIMEO)
	{
		assert(_rwlock);
		int result = rwlock_lock_read(_rwlock, timeo);
		assert(result); // deadlock may exist
	}

	void unlockRead()
	{
		assert(_rwlock);
		int result = rwlock_unlock_read(_rwlock);
		assert(result); // deadlock may exist
	}

	void lockWrite(const uint32 timeo = RWLOCK_DEADLOCK_TIMEO)
	{
		assert(_rwlock);
		int result = rwlock_lock_write(_rwlock, timeo);
		assert(result); // deadlock may exist
	}

	void unlockWrite()
	{
		assert(_rwlock);
		int result = rwlock_unlock_write(_rwlock);
		assert(result); // deadlock may exist
	}

#endif // #ifdef _DEBUG

protected:
	_rwlock_t		_rwlock;
};

class RWLockGuard {
public:
	enum {
		LOCK_READ, 
		LOCK_WRITE
	};

	RWLockGuard(XRWLock& rwlock, int type = LOCK_WRITE, 
		const uint32 timeo = RWLOCK_DEADLOCK_TIMEO) :
		_rwlock(rwlock)
	{
		_type = type;
		if (type == LOCK_READ)
			rwlock.lockRead(timeo);
		else if (type == LOCK_WRITE)
			rwlock.lockWrite(timeo);
#ifdef _DEBUG
		else
			assert(false);
#endif
			
	}

	~RWLockGuard()
	{
		if (_type == LOCK_READ)
			_rwlock.unlockRead();
		else if (_type == LOCK_WRITE)
			_rwlock.unlockWrite();

#ifdef _DEBUG
		else
			assert(false);
#endif

	}

protected:
	XRWLock&	_rwlock;
	int			_type;
};

#endif // !defined(AFX_RWLOCK_H__0BD0826A_45F4_43DF_B1EC_6D2E749C55D7__INCLUDED_)
