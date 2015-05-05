// ===========================================================================
// Copyright (c) 2010 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// Ident : $Id: Pointer.h$
// Branch: $Name:  $
// Author: HongQuan Zhang
// Desc  : Define smart pointer
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Pointer.h $
// 
// 5     4/09/15 11:47a Ketao.zhang
// 
// 4     3/26/13 2:46p Hui.shao
// 
// 4     3/26/13 2:37p Hui.shao
// 
// 3     2/17/12 5:57p Li.huang
// 
// 2     10-12-28 12:45 Fei.huang
// * atomic add does not compile on linux
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 4     10-10-22 13:47 Hui.shao
// 
// 3     10-10-21 15:14 Hui.shao
// 
// 2     10-09-16 19:24 Hui.shao
// ===========================================================================

#ifndef __ZQ_COMMON_Pointer_H__
#define __ZQ_COMMON_Pointer_H__

#include "ZQ_common_conf.h"
#include "Exception.h"

namespace ZQ{
namespace common{

class ZQ_COMMON_API SharedObjExpcetion;
class ZQ_COMMON_API NullSharedObjException;
class ZQ_COMMON_API SharedObject;

// -----------------------------
// class AtomicInt
// -----------------------------
class AtomicInt
{
public:
	AtomicInt() : _v (0) {}
	
	void set(uint32 newValue) { _v = newValue; }
	int get() { return _v; }

	int inc(void);
	int dec(void);
	bool decThenIfZero(void);
	int  add(int v);

protected:
#ifdef ZQ_OS_MSWIN
	volatile LONG _v;
#else 
	volatile int _v;
#endif // ZQ_OS
};

#ifdef ZQ_OS_MSWIN
inline int AtomicInt::inc(void)
{
	return InterlockedIncrement(&_v);
}

inline int AtomicInt::dec(void)
{
	return InterlockedDecrement(&_v);
}

inline bool AtomicInt::decThenIfZero(void)
{
	int v = InterlockedDecrement(&_v);
	return (v==0);
}

inline int AtomicInt::add(int v)
{
	return InterlockedExchangeAdd( &_v , v);
}

#elif defined ZQ_OS_LINUX
inline int AtomicInt::inc(void)
{
	__asm__ __volatile__(
		"lock ; incl %0"
		:"=m" (_v)
		:"m" (_v)
		: "memory"
	);

	return _v;
}

inline int AtomicInt::dec(void)
{
	decThenIfZero();
	return _v;
}

inline bool AtomicInt::decThenIfZero(void)
{ 
	unsigned char c;
	__asm__ __volatile__(
		"lock ; decl %0; sete %1"
		:"=m" (_v), "=qm" (c)
		:"m" (_v) : "memory"
	);
	
	return (c != 0);
}

inline int AtomicInt::add(int v)
{
	int tmp = _v;

	__asm__ __volatile__(
		"lock; xadd %1,%0"
		:"=m"(_v) 
		:"r"(v)
		:"memory"
	);

	return tmp;
}

#else
#  error "Unsupported OS for class ZQ::common::AtomicInt"
#endif // ZQ_OS

// -----------------------------
// class SharedObjExpcetion
// -----------------------------
class SharedObjExpcetion : public Exception
{
public:
	SharedObjExpcetion(const std::string& what_arg);
};

// -----------------------------
// class NullSharedObjException
// -----------------------------
class NullSharedObjException : SharedObjExpcetion
{
public:
	NullSharedObjException(const std::string& what_arg);
};

// -----------------------------
// class SharedObject
// -----------------------------
class SharedObject
{
public:
	SharedObject() ;
	SharedObject(const SharedObject&);

	virtual ~SharedObject();

	SharedObject& operator=(const SharedObject)
	{
		//copy is not permitted
		return *this;
	}
	
	bool operator == ( const SharedObject& b);
	
	void __incRef( );
	void __decRef( );
	int __getRef();
	void __setNoDelete(bool noDel);
	
protected:
	AtomicInt	_ref;
	bool		_noDelete;
};

// -----------------------------
// template Pointer
// -----------------------------
template<typename T>
class Pointer
{
public:
	typedef T element_type;
	T* _ptr;
	
	// constructor
	// -----------------------------
	Pointer(T* p = NULL)
	{
		this->_ptr = p;

		if (this->_ptr)
			this->_ptr->__incRef();
	}
	~Pointer()
	{
		if (this->_ptr)
			this->_ptr->__decRef();
	}
	// copier constructors
	// -----------------------------
	template<typename Y>
	Pointer(const Pointer<Y>& r)
	{
		this->_ptr = r._ptr;

		if (this->_ptr)
			this->_ptr->__incRef();
	}

	Pointer(const Pointer& r)
	{
		this->_ptr = r._ptr;

		if (this->_ptr)
			this->_ptr->__incRef();
	}
	
	// copiers
	// -----------------------------
	Pointer& operator=(const Pointer& r)
	{
		if(this->_ptr != r._ptr)
		{
			if (r._ptr)
				r._ptr->__incRef();

			T* ptr = this->_ptr;
			this->_ptr = r._ptr;

			if (ptr)
				ptr->__decRef();
		}
		
		return *this;
	}

	// dynamicCast
	// -----------------------------
	template<class Y>
	static Pointer dynamicCast(const Pointer<Y>& r)
	{
#ifdef __BCPLUSPLUS__
		return Pointer<T>(dynamic_cast<T*>(r._ptr));
#else
		return Pointer(dynamic_cast<T*>(r._ptr));
#endif
	}

	template<class Y>
	static Pointer dynamicCast(Y* p)
	{
#ifdef __BCPLUSPLUS__
		return Pointer<T>(dynamic_cast<T*>(p));
#else
		return Pointer(dynamic_cast<T*>(p));
#endif
	}
	
	// access to the pointer values
	T* get() const { return _ptr; }

	inline T* operator->() const
	{
		if (!_ptr)
		{
			// We don't throw directly NullObjectHandleException here to
			// keep the code size of this method to a minimun (the
			// assembly code for throwing an exception is much bigger
			// than just a function call). This maximises the chances
			// of inlining by compiler optimization.
			throwNullObjectHandleException(__FILE__, __LINE__);           
		}

		return _ptr;
	}

	inline T& operator*() const
	{
		if (!_ptr)
		{
			//
			// We don't throw directly NullObjectHandleException here to
			// keep the code size of this method to a minimun (the
			// assembly code for throwing an exception is much bigger
			// than just a function call). This maximises the chances
			// of inlining by compiler optimization.
			//
			throwNullObjectHandleException(__FILE__, __LINE__);           
		}

		return *_ptr;
	}

	operator bool() const
	{
		return _ptr ? true : false;
	}

	void swap(Pointer& other)
	{
		std::swap(_ptr, other._ptr);
	}

protected:

	void throwNullObjectHandleException(const char *, int) const;
};

// -----------------------------
// impl of Pointer
// -----------------------------
template<typename T>
inline void Pointer<T>::throwNullObjectHandleException(const char* file, int line) const
{
	throw NullSharedObjException("Null ObjectHandle");
}

template<typename T, typename U>
inline bool operator ==(const Pointer<T>& lhs, const Pointer<U>& rhs)
{
	T* l = lhs.get();
	U* r = rhs.get();
	if(l && r)
		return *l == *r;

	return !l && !r;
}

template<typename T, typename U>
inline bool operator!=(const Pointer<T>& lhs, const Pointer<U>& rhs)
{
	T* l = lhs.get();
	U* r = rhs.get();
	if(l && r)
		return *l != *r;

	return l || r;
}

template<typename T, typename U>
inline bool operator<(const Pointer<T>& lhs, const Pointer<U>& rhs)
{
	T* l = lhs.get();
	U* r = rhs.get();
	if(l && r)
		return l < r;

	return !l && r;
}

}
}//endof namespace

#endif // __ZQ_COMMON_Pointer_H__


