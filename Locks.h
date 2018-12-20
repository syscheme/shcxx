// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai,,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
//
// Ident : $Id: Locks.h,v 1.9 2004/06/30 07:24:54 jshen Exp $
// Branch: $Name:  $
// Author: mwang
// Desc  : Define simple mutex and semaphore
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Locks.h $
// 
// 2     1/21/15 5:57p Hui.shao
// tested sadd/srem/smembers
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 20    09-06-23 15:23 Fei.huang
// + added timedWait
// 
// 19    09-04-29 14:50 Hongquan.zhang
// 
// 18    09-03-19 12:30 Hongquan.zhang
// 
// 17    09-02-24 17:49 Hongquan.zhang
// export delay in windows
// 
// 16    09-02-13 16:31 Hongquan.zhang
// add a delay function 
// 
// 15    09-02-11 10:43 Hongquan.zhang
// 
// 14    08-10-28 10:33 Hongquan.zhang
// 
// 13    08-07-07 15:06 Yixin.tian
// 
// 12    08-06-20 14:36 Jie.zhang
// add "const" to  enter(),leave() etc
// 
// 11    08-03-06 16:19 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 10    08-03-03 17:23 Yixin.tian
// merged changes for linux
// 
// 9     07-05-22 17:31 Hui.shao
// 
// 3     05-06-24 14:58 Bernie.zhao
// modified 'MutexGuard' to  'ZQ::common::Guard<ZQ::common::Mutex>' for
// convinient use between namespaces
// Revision 1.9  2004/06/30 07:24:54  jshen
// remove some warnings
//
// Revision 1.8  2004/06/23 07:37:39  wli
// add coede to avoid redefine WIN32_LEAN_AND_MEAN
//
// Revision 1.7  2004/06/13 03:46:50  mwang
// no message
//
// Revision 1.6  2004/06/13 03:42:41  mwang
// no message
//
// Revision 1.5  2004/06/07 13:33:30  shao
// no message
//
// Revision 1.4  2004/06/07 10:58:25  mwang
// no message
//
// Revision 1.3  2004/06/07 10:54:08  mwang
// add WaitThread impl
//
// Revision 1.2  2004/06/07 10:46:13  shao
// added Semaphore
//
// ===========================================================================

#ifndef __ZQ_COMMON_LOCKS_H__
#define __ZQ_COMMON_LOCKS_H__

#include "ZQ_common_conf.h"
#include "Exception.h"
#include "SystemUtils.h"
#include "Pointer.h"

#ifndef ZQ_OS_MSWIN
extern "C" {
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
}
#endif

#ifndef ZQ_OS_MSWIN

//#pragma message ( __MSGLOC__ "TODO: impl here for non-Win32")

#  if (defined(__FreeBSD__) && __FreeBSD__ <= 3) || defined(_AIX)
#    define	_SYSV_SEMAPHORES
#  endif

#  ifndef HAVE_PTHREAD_H

//#pragma message ( __MSGLOC__ "TODO: impl here for non-Win32")

#    include <pthread.h>
#    ifndef _SYSV_SEMAPHORES
#      include <semaphore.h>
#    endif
#  endif
#endif // !ZQ_OS_MSWIN

#ifndef ZQ_OS_MSWIN
#  include <time.h>
#  include <signal.h>
#  include <unistd.h>
typedef	unsigned long	timeout_t;
#else // ZQ_OS_MSWIN
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#  include <windows.h>
typedef DWORD   timeout_t;
#  define	MAX_SEM_VALUE	1000000
#endif // !ZQ_OS_MSWIN

namespace ZQ {
namespace common  {

class ZQ_COMMON_API Mutex;
class ZQ_COMMON_API Semaphore;
class ZQ_COMMON_API Cond;

ZQ_COMMON_API void	delay( uint32 millisecond );

#ifndef _TEST_DEADLOCK
// -----------------------------
// class Mutex
// -----------------------------
///a wrap of CRITICAL_SECTION on windows
///suppose the functions will never fail!!!(if it fails, the OS will be ...)
class Mutex
{
public:
	friend class Cond;
	///initializes the critical section object
	Mutex();

	///releases all resources used by an unowned critical section object.
	~Mutex();

	///waits for ownership of the specified critical section object,The function returns when the calling thread is granted ownership.
	void enter() const;

	///releases ownership of the specified critical section object.
	void leave() const;
		
#if _WIN32_WINNT >=0x0400
	bool tryEnter()  const
		{return (::TryEnterCriticalSection((LPCRITICAL_SECTION)&_mutex) == TRUE);}
#endif //_WIN32_WINNT
#ifndef ZQ_OS_MSWIN
	bool tryEnter() const
	{return (pthread_mutex_trylock((pthread_mutex_t*)&_mutex)==0);}
#endif
	
private:
#ifdef ZQ_OS_MSWIN
	CRITICAL_SECTION  _mutex;
#else
	pthread_mutex_t  _mutex;
#endif
	///can't be copied!!!
	Mutex(const Mutex &);
	Mutex &operator=(const Mutex &);
};

// -----------------------------
// template Guard
// -----------------------------
///a simple wrap for locks,for exception safe
template <typename _LOCK >
class Guard
{
public:
	Guard(const _LOCK& lock,char* pSth=NULL)
		:_lock(lock) { _lock.enter(); }

	~Guard()
		{ _lock.leave(); }
private:
	///need to be const???
	const _LOCK& _lock;
	///also ,can't be copied!!!
	Guard(const Guard &);
	Guard &operator=(const Guard &);
};


#else//_TEST_DEADLOCK

#include <string>

class Mutex
{
public:
#define	INVALID_ID	0
	Mutex()
	{
		m_LastThreadID=INVALID_ID;
		m_hEvent=CreateEvent (NULL,FALSE,TRUE,NULL);
		m_TimeOut=5000;
		m_Ref=0;
	}
	~Mutex()
	{
		CloseHandle(m_hEvent);
	}
	void enter(char* loc=NULL)
	{
		if(m_LastThreadID==INVALID_ID)
		{
			Get ();
		}
		else
		{
			if(m_LastThreadID!=GetCurrentThreadId ())
			{
				Get ();
			}
		}		
		InterlockedIncrement(&m_Ref);
		SetLastLoc(loc);
	}
	void leave()
	{
		if(InterlockedDecrement(&m_Ref)==0)		
		{
			SetEvent(m_hEvent);
		}
	}
	void Get()
	{
		if(WaitForSingleObject (m_hEvent,m_TimeOut)==WAIT_TIMEOUT)
		{
			throw "time out";
		}		
		m_LastThreadID=GetCurrentThreadId ();
	}
	void	SetLastLoc(char* strLoc)
	{
		if(strLoc)
			_LastLoc=strLoc;
		else
			_LastLoc="";
	}
private:
	HANDLE	m_hEvent;
	DWORD	m_LastThreadID;
	int		m_TimeOut;
	long volatile m_Ref;
	std::string	_LastLoc;
#undef INVALID_ID
};

template <typename _LOCK >
class Guard
{
public:
	Guard(_LOCK& lock,char* loc=NULL)
		:_lock(lock) { _lock.enter(loc); }
	
	~Guard()
	{ _lock.leave(); }
private:
	///need to be const???
	_LOCK& _lock;
	///also ,can't be copied!!!
	Guard(const Guard &);
	Guard &operator=(const Guard &);
};

#endif//_TEST_DEADLOCK

typedef Guard<ZQ::common::Mutex> MutexGuard;

// -----------------------------
// class Semaphore
// -----------------------------
/// The semaphore has a counter which only permits access by one or more threads when
/// the value of the semaphore is non-zero. Each access reduces the current value of
/// the semaphore by 1. One or more threads can wait on a semaphore until it is no
/// longer 0, and hence the semaphore can be used as a simple thread synchronization
/// object to enable one thread to pause others until the thread is ready or has
/// provided data for them. Semaphores are typically used as a counter for protecting
/// or limiting concurrent access to a given resource, such as to permitting at most
/// "x" number of threads to use resource "y", for example.   
class Semaphore
{
private:
#ifndef ZQ_OS_MSWIN
#ifdef	_SYSV_SEMAPHORES
	int _semaphore;
#else
	sem_t _semaphore;
#endif
#else // ZQ_OS_MSWIN
	HANDLE	semObject;
#endif // !ZQ_OS_MSWIN

public:

	/// The initial value of the semaphore can be specified. An initial value is often
	/// used When used to lock a finite resource or to specify the maximum number of
	/// thread instances that can access a specified resource.
	/// @param resource specify initial resource count or 0 default.
	Semaphore(size_t resource = 0);

	/// Destroying a semaphore also removes any system resources associated with it.
	/// If a semaphore has threads currently waiting on it, those threads will all
	/// continue when a semaphore is destroyed.
	virtual ~Semaphore();

	/// Wait is used to keep a thread held until the semaphore counter is greater than
	/// 0. If the current thread is held, then another thread must increment the
	/// semaphore.  Once the thread is accepted, the semaphore is automatically
	/// decremented, and the thread continues execution. The pthread semaphore object
	/// does not support a timed "wait", and hence to maintain consistancy, neither
	/// the posix nor win32 source trees support "timed" semaphore objects.
	void wait(void);

    void timedWait(timeout_t milliseconds);
	/// TryWait is a non-blocking variant of Wait. If the semaphore counter is greater
	/// than 0, then the thread is accepted and the semaphore counter is decreased. If
	/// the semaphore counter is 0 TryWait returns immediately with false, NO decrement.
	/// @return true if thread is accepted otherwise false
	bool tryWait(void);

	/// Posting to a semaphore increments its current value and releases the first
	/// thread waiting for the semaphore if it is currently at 0. multiple increments
	/// must perform multiple post operations.
	void post(void);

	/// TODO: how implement getValue for posix compatibility ?
	/// Get the current value of a semaphore.
	/// @return current value.
#ifndef ZQ_OS_MSWIN
#ifndef __CYGWIN32__
	int getValue(void);
#endif
#endif
};

class Cond
{
public:
	Cond( );
	~Cond( );
public:
	void	signal( );
	///parameter timeOut -1 to wait infinite
	bool	wait( Mutex& m , uint32 timeOut );
	void	broadCast( );
private:

#ifdef ZQ_OS_MSWIN
	
	HANDLE		fCondition;
	uint32		waitCount;

#elif defined ZQ_OS_LINUX
	
	pthread_cond_t      fCondition;

#endif
};

class RWLock
{
//private:
public:
    mutable int m_reader;
    mutable int m_writer;
    mutable int m_reader_queue;
    mutable int m_writer_queue;
#ifdef ZQ_OS_MSWIN
	mutable CRITICAL_SECTION m_lock;
    mutable HANDLE m_nextreader;
    mutable HANDLE m_nextwriter;
#else
	mutable pthread_mutex_t  m_lock;
	mutable sem_t m_nextreader;
	mutable sem_t m_nextwriter;
#endif

public:
#ifdef ZQ_OS_MSWIN
    RWLock() 
    {
        ::InitializeCriticalSection(&m_lock);
		m_reader = 0;
        m_writer = 0;
        m_reader_queue = 0;
        m_writer_queue = 0;		
        m_nextreader = ::CreateEvent( NULL,  // no security attributes
                                      TRUE,  // manual-reset event
                                      FALSE, // initial state is unsignaled
                                      NULL   // create without name
                                    ); 

        m_nextwriter = ::CreateEvent( NULL,  // no security attributes
                                      FALSE, // auto-reset event
                                      FALSE, // initial state is unsignaled
                                      NULL   // create without name
                                    );
    }

    ~RWLock() 
    { 
        ::DeleteCriticalSection(&m_lock); 
        ::CloseHandle(m_nextreader);
        ::CloseHandle(m_nextwriter);
    }

    void ReadLock() const 
    {
        ::EnterCriticalSection(&m_lock); 
        ++ m_reader_queue;

        if (m_writer > 0)
        {
            ::LeaveCriticalSection(&m_lock); 
            ::WaitForSingleObject(m_nextreader, INFINITE);

            ::EnterCriticalSection(&m_lock); 
            -- m_reader_queue;
            ++ m_reader;
            if (m_reader_queue == 0) 
                ::ResetEvent(m_nextreader);
            ::LeaveCriticalSection(&m_lock); 
        }
        else
        {
            -- m_reader_queue;
            ++ m_reader;
            ::LeaveCriticalSection(&m_lock); 
        }
    }
    
    void ReadUnlock() const
    {
        ::EnterCriticalSection(&m_lock);
        -- m_reader;

        if (m_writer_queue > 0 && m_reader == 0)
        {
            ::LeaveCriticalSection(&m_lock); 
            ::SetEvent(m_nextwriter);
        }
        else
        {
            ::LeaveCriticalSection(&m_lock); 
        }
    }

    void WriteLock() const 
    {
        ::EnterCriticalSection(&m_lock); 
        ++ m_writer_queue;

        if (m_reader == 0 && m_writer == 0)
        {
            -- m_writer_queue;
            ++ m_writer;
	      ::LeaveCriticalSection(&m_lock);              
        }
        else
        {
            ::LeaveCriticalSection(&m_lock); 
            ::WaitForSingleObject(m_nextwriter, INFINITE);

            ::EnterCriticalSection(&m_lock); 
            -- m_writer_queue;
            ++ m_writer;
            ::LeaveCriticalSection(&m_lock); 
        }
    }

    void WriteUnlock() const
    {
        ::EnterCriticalSection(&m_lock); 
        -- m_writer;

        if (m_writer_queue > 0)
        {
            ::LeaveCriticalSection(&m_lock); 
            ::SetEvent(m_nextwriter);
        }
        else if (m_reader_queue > 0)
        {
            ::LeaveCriticalSection(&m_lock); 
            ::SetEvent(m_nextreader);
        }
        else
        {
            ::LeaveCriticalSection(&m_lock); 
        }
    }
	
#else //NON-ZQ_OS_MSWIN
	RWLock() 
    {        
        m_reader = 0;
        m_writer = 0;
        m_reader_queue = 0;
        m_writer_queue = 0;

		pthread_mutexattr_t muattr;
		pthread_mutexattr_init(&muattr);
		pthread_mutexattr_settype(&muattr,PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&m_lock,&muattr);
		sem_init(&m_nextreader,0,0);
		sem_init(&m_nextwriter,0,0);
    }

    ~RWLock() 
    {
		pthread_mutex_destroy(&m_lock);
		sem_destroy(&m_nextreader);
		sem_destroy(&m_nextwriter);
    }

    void ReadLock() const 
    {
		pthread_mutex_lock(&m_lock);
        ++ m_reader_queue;

        if (m_writer > 0)
        {
			pthread_mutex_unlock(&m_lock);
			sem_wait(&m_nextreader);

			pthread_mutex_lock(&m_lock);
			-- m_reader_queue;
            ++ m_reader;
			if (m_reader_queue == 0)//set read sem nonsignal
			{
				int nval = 0;
				sem_getvalue(&m_nextreader,&nval);
                if(nval > 0)
				{
					while(nval--)
						sem_trywait(&m_nextreader);
				}
			}
            
			pthread_mutex_unlock(&m_lock);
        }
        else
        {
            -- m_reader_queue;
            ++ m_reader;
			pthread_mutex_unlock(&m_lock);
        }
    }
    
    void ReadUnlock() const
    {
		pthread_mutex_lock(&m_lock);
        -- m_reader;

        if (m_writer_queue > 0 && m_reader == 0)
        {
			pthread_mutex_unlock(&m_lock);
			sem_post(&m_nextwriter);
        }
        else
        {
			pthread_mutex_unlock(&m_lock);
        }
    }

    void WriteLock() const 
    {
		pthread_mutex_lock(&m_lock);
        ++ m_writer_queue;

        if (m_reader == 0 && m_writer == 0)
        {
            -- m_writer_queue;
            ++ m_writer;
			pthread_mutex_unlock(&m_lock);             
        }
        else
        {
			pthread_mutex_unlock(&m_lock);
			sem_wait(&m_nextwriter);

			pthread_mutex_lock(&m_lock);
			-- m_writer_queue;
            ++ m_writer;
			pthread_mutex_unlock(&m_lock);
        }
    }

    void WriteUnlock() const
    {
		pthread_mutex_lock(&m_lock); 
        -- m_writer;

        if (m_writer_queue > 0)
        {
			pthread_mutex_unlock(&m_lock);
			sem_post(&m_nextwriter);
        }
        else if (m_reader_queue > 0)
        {
			pthread_mutex_unlock(&m_lock);
			for(int i=0; i<m_reader_queue; i++)//set all read sem signal
				sem_post(&m_nextreader);
        }
        else
        {
			pthread_mutex_unlock(&m_lock);
        }
    }
#endif
};

class AutoReadLock
{
private:
    RWLock & _rwlock;
public:
    AutoReadLock(RWLock & m) : _rwlock(m) { _rwlock.ReadLock(); }
    ~AutoReadLock() { _rwlock.ReadUnlock(); }
};

class AutoWriteLock
{
private:
    RWLock & _rwlock;
public:
    AutoWriteLock(RWLock & m) : _rwlock(m) { _rwlock.WriteLock(); }
    ~AutoWriteLock() { _rwlock.WriteUnlock(); }
};

class Event : public SharedObject // Wrapper the stupid naming of SingleObject
{
public: 
	typedef Pointer < Event > Ptr;

	bool wait(timeout_t timeout=TIMEOUT_INF) { return (SYS::SingleObject::SIGNALED == _so.wait(timeout)); }
	void signal() { _so.signal(); }

protected:
	SYS::SingleObject _so;
};



}
}

#endif  //__ZQ_COMMON_LOCKS_H__
