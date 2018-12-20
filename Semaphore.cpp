// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai
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
// Ident : $Id: Semaphore.cpp,v 1.3 2004/06/07 10:58:25 mwang Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl Semaphore
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Semaphore.cpp $
// 
// 2     7/22/11 11:38a Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 21    09-07-07 16:11 Hongquan.zhang
// 
// 20    09-07-07 14:06 Hongquan.zhang
// 
// 17    09-06-23 15:23 Fei.huang
// + added timedWait
// 
// 16    09-05-11 18:30 Fei.huang
// * fix sem_wait interrupt issue
// 
// 15    09-03-19 12:30 Hongquan.zhang
// 
// 13    09-02-27 18:08 Jie.zhang
// 
// 12    09-02-13 16:34 Hongquan.zhang
// 
// 11    09-02-13 16:31 Hongquan.zhang
// add a delay function 
// 
// 10    09-02-11 14:23 Hongquan.zhang
// 
// 9     09-02-11 10:43 Hongquan.zhang
// 
// 8     08-10-27 18:33 Hongquan.zhang
// 
// 7     08-07-07 15:06 Yixin.tian
// 
// 6     08-06-20 14:38 Jie.zhang
// add "const" to enter(),leave() etc
// 
// 5     08-03-03 17:23 Yixin.tian
// merged changes for linux
// 
// 4     07-06-04 18:42 Ken.qian
// remove inline
// 
// 3     07-05-22 17:31 Hui.shao
// 
// 2     8/30/04 11:23a Jie.zhang
// Revision 1.3  2004/06/07 10:58:25  mwang
// no message
//
// Revision 1.2  2004/06/07 10:54:08  mwang
// add WaitThread impl
//
// Revision 1.1  2004/06/07 10:46:13  shao
// added Semaphore
//
// ===========================================================================

#include "Locks.h"
#include <assert.h>
#include <errno.h>

namespace ZQ {
namespace common {

// -----------------------------
// class Mutex
// -----------------------------
#ifdef WIN32

#define DefaultCriticalSectionSpinCount			4000

void delay( uint32 millisecond )
{
	Sleep(millisecond);
}


Mutex::Mutex()
{
#if (_WIN32_WINNT >= 0x0403)
	::InitializeCriticalSectionAndSpinCount(&_mutex, DefaultCriticalSectionSpinCount);
#else
	::InitializeCriticalSection(&_mutex);
#endif
}

Mutex::~Mutex()
{
	::DeleteCriticalSection(&_mutex);
}

///waits for ownership of the specified critical section object,The function returns when the calling thread is granted ownership.
void Mutex::enter() const
{
	::EnterCriticalSection((LPCRITICAL_SECTION)&_mutex);
}

///releases ownership of the specified critical section object.
void Mutex::leave()  const
{
	::LeaveCriticalSection((LPCRITICAL_SECTION)&_mutex);
}

#else//nom-win32
Mutex::Mutex()
{
	pthread_mutexattr_t muattr;
	pthread_mutexattr_init(&muattr);
	pthread_mutexattr_settype(&muattr,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&_mutex,&muattr);
}

Mutex::~Mutex()
{
	try{
		pthread_mutex_destroy(&_mutex);
	}catch(...){}
}

///waits for ownership of the specified critical section object,The function returns when the calling thread is granted ownership.
void Mutex::enter()  const
{
	pthread_mutex_lock((pthread_mutex_t*)&_mutex);
}

///releases ownership of the specified critical section object.
void Mutex::leave()  const
{
	pthread_mutex_unlock((pthread_mutex_t*)&_mutex);
}
#endif


#ifndef WIN32


#include <sys/time.h>
#include <time.h>

void delay( uint32 milliseconds )
{	
	useconds_t u = milliseconds;
	u *= 1000;
	usleep( u );
}


Cond::Cond( )
{
	pthread_cond_init( &fCondition , NULL);	
}
Cond::~Cond( )
{	
	pthread_cond_destroy(&fCondition);
}

bool Cond::wait( Mutex& m , uint32 timeOut )
{
	if( timeOut == (uint32)-1 )
	{
		pthread_cond_wait(&fCondition , &m._mutex );
		return true;
	}
	else
	{
		struct timespec absTime;
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		absTime.tv_sec  = now.tv_sec + timeOut/1000;
		absTime.tv_nsec = now.tv_nsec + (timeOut - timeOut/1000*1000) *1000* 1000;
		if( absTime.tv_nsec > ( 1000 * 1000 * 1000) )
		{
		    absTime.tv_sec++;
		    absTime.tv_nsec = absTime.tv_nsec % (1000*1000*1000);
		}
		int rc = pthread_cond_timedwait(&fCondition , &m._mutex,&absTime);
		return rc == 0;
	}
}

void Cond::signal( )
{
	pthread_cond_signal(&fCondition);
}

void Cond::broadCast( )
{
	pthread_cond_broadcast(&fCondition);
}


#ifdef	_SYSV_SEMAPHORES

extern "C"
{
	#include <sys/types.h>
	#include <sys/ipc.h>
	#include <sys/sem.h>
	
};
#include <cerrno>
// -----------------------------
// class Semaphore
// -----------------------------


Semaphore::Semaphore(size_t resource)
{
#ifdef _AIX
        union semun
        {
                int val; 
                struct semid_ds *buf; 
                unsigned short *array; 
        } arg;
#else
	union semun arg;
#endif

	if((_semaphore = semget(IPC_PRIVATE, 1, 0644 | IPC_CREAT)) == -1)
		throw SyncException("Semaphone create failed");

	arg.val = resource;
	if(semctl(_semaphore, 0, SETVAL, arg) == -1)
		throw SyncException("Semaphone control failed");
}

Semaphore::~Semaphore()
{
	semctl(_semaphore, 0, IPC_RMID);
}

void Semaphore::wait(void)
{
	struct sembuf ops[] = {{0, -1, 0}};
	
	semop(_semaphore, ops, 1);
}

bool Semaphore::tryWait(void)
{
	struct sembuf ops[] = {{0, -1, IPC_NOWAIT}};
	
	return (semop(_semaphore, ops, 1) == EAGAIN) ? false : true;
}

void Semaphore::post(void)
{
	struct sembuf ops[] = {{0, 1, 0}};

	semop(_semaphore, ops, 1);
}

int Semaphore::getValue(void)
{
	return semctl(_semaphore,0,GETVAL);
}

#else // _SYSV_SEMAPHORES

Semaphore::Semaphore(size_t resource) 
{
	if(sem_init(&_semaphore, 0, resource))
		throw SyncException("Semaphone construction failed");
}

Semaphore::~Semaphore() {
	sem_destroy(&_semaphore);
}

void Semaphore::wait(void)
{
	sem_wait(&_semaphore);

//	while(sem_wait(&_semaphore) == (-1) && errno == EINTR) {
//		continue;
//	};
}

void Semaphore::timedWait(timeout_t timeout) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    long long nsec = ts.tv_nsec + timeout*1000000LL;
    ts.tv_sec += nsec/1000000000L;
    ts.tv_nsec = nsec%1000000000L;

    sem_timedwait(&_semaphore, &ts);
}

void Semaphore::post(void) {
	sem_post(&_semaphore);
}

bool Semaphore::tryWait(void) {
	return (sem_trywait(&_semaphore) == 0) ? true : false;
}

#ifndef __CYGWIN32__
int Semaphore::getValue(void)
{
	int value;
	sem_getvalue(&_semaphore, &value);
	return value;
}
#endif


#endif // _SYSV_SEMAPHORES

#else // WIN32


//win32 Impl

static DWORD waitThread(HANDLE hRef, timeout_t timeout)
{
	return ::WaitForSingleObjectEx(hRef, timeout, TRUE);
}

Semaphore::Semaphore(size_t resource)
{
	semObject = ::CreateSemaphore((LPSECURITY_ATTRIBUTES)NULL, resource, MAX_SEM_VALUE, (LPCTSTR)NULL);
}	

Semaphore::~Semaphore()
{
	::CloseHandle(semObject);
}

void Semaphore::wait(void)
{
	waitThread(semObject, INFINITE);
}

void Semaphore::timedWait(timeout_t milli) {
    waitThread(semObject, milli);
}

bool Semaphore::tryWait(void)
{
	return waitThread(semObject, 0) == WAIT_OBJECT_0;
}

void Semaphore::post(void)
{
	::ReleaseSemaphore(semObject, 1, (LPLONG)NULL);
}

//////////////////////////////////////////////////////////////////////////
//implementation of Cond
Cond::Cond( )
{
	waitCount	=	0;
	fCondition		=	::CreateEvent( NULL , FALSE , FALSE , NULL );
	assert( fCondition != NULL );
}
Cond::~Cond( )
{
	if( fCondition )
	{
		CloseHandle( fCondition );
		fCondition = NULL;
	}
}

bool Cond::wait( Mutex& m , uint32 timeOut )
{
	waitCount ++;
	m.leave( );	
	DWORD dwRet  = WaitForSingleObject( fCondition ,timeOut );
	m.enter( );
	if( dwRet == WAIT_OBJECT_0  )
	{
		return true;
	}
	else
	{
		return false;
	}	
}

void Cond::signal( )
{
	SetEvent( fCondition );
}

void Cond::broadCast( )
{
	uint32 uCount = waitCount;
	for( uint32 i = 0 ; i < uCount ; i++ )
	{
		SetEvent( fCondition) ;
	}
}

#endif //WIN32

}
}
