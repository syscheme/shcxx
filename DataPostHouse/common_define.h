
#ifndef _data_posthouse_common_deinfe_header_file_h__
#define _data_posthouse_common_deinfe_header_file_h__

#include "sharedObjectInline.inl"
#include "Exception.h"
#include <assert.h>

namespace ZQ
{
namespace DataPostHouse
{

class SharedObjExpcetion : public ZQ::common::Exception
{
public:
	SharedObjExpcetion(const std::string& what_arg)
		:ZQ::common::Exception(what_arg)
	{
	}
};

class NullSharedObjException : SharedObjExpcetion
{
public:
	NullSharedObjException(const std::string& what_arg)
		:SharedObjExpcetion(what_arg)
	{

	}
};

//copy idea from ICE
//for more detail about ICE please visit http://www.zeroc.com
class SharedObject
{
public:
	SharedObject( )
		:_noDelete(false)
	{
	}

	SharedObject(const SharedObject&)		
		:_noDelete(false)
	{
	}

	virtual ~SharedObject( )
	{

	}
	SharedObject& operator=(const SharedObject)
	{
		//copy is not permitted
		return *this;
	}
	bool operator == ( const SharedObject& b)
	{
		return this == &b;
	}
	
	void	__incRef( )
	{
		assert(ice_atomic_exchange_add(0, &_ref) >= 0);
		ice_atomic_inc(&_ref);
	}

	void	__decRef( )
	{
		assert(ice_atomic_exchange_add(0, &_ref) > 0);
		if( ice_atomic_dec_and_test(&_ref) && !_noDelete)
		{
			_noDelete = true;
			delete this;
		}
	}
	int __getRef() const
	{
		 return ice_atomic_exchange_add(0, const_cast<ice_atomic_t*>(&_ref));
	}
	void __setNoDelete(bool noDel)
	{
		_noDelete = noDel;
	}
protected:
	ice_atomic_t	_ref;
	bool			_noDelete;

};

template<typename T>
class ObjectHandleBase
{
public:

	typedef T element_type;

	T* get() const
	{
		return _ptr;
	}

	inline T* operator->() const
	{
		if(!_ptr)
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

		return _ptr;
	}

	inline T& operator*() const
	{
		if(!_ptr)
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

	void swap(ObjectHandleBase& other)
	{
		std::swap(_ptr, other._ptr);
	}

	T* _ptr;

private:

	void throwNullObjectHandleException(const char *, int) const;
};

template<typename T> inline void 
ObjectHandleBase<T>::throwNullObjectHandleException(const char* file, int line) const
{
	throw NullSharedObjException("Null ObjectHandle");
}

template<typename T, typename U>
inline bool operator==(const ObjectHandleBase<T>& lhs, const ObjectHandleBase<U>& rhs)
{
	T* l = lhs.get();
	U* r = rhs.get();
	if(l && r)
	{
		return *l == *r;
	}
	else
	{
		return !l && !r;
	}   
}

template<typename T, typename U>
inline bool operator!=(const ObjectHandleBase<T>& lhs, const ObjectHandleBase<U>& rhs)
{
	T* l = lhs.get();
	U* r = rhs.get();
	if(l && r)
	{
		return *l != *r;
	}
	else
	{
		return l || r;
	}   
}

template<typename T, typename U>
inline bool operator<(const ObjectHandleBase<T>& lhs, const ObjectHandleBase<U>& rhs)
{
	T* l = lhs.get();
	U* r = rhs.get();
	if(l && r)
	{
		return l < r;
	}
	else
	{
		return !l && r;
	}
}

///To avoid name conflict with ICE
///rename ObjectHandle to ObjectObjectHandle
template<typename T>
class ObjectHandle : public ObjectHandleBase<T>
{
public:

	ObjectHandle(T* p = 0)
	{
		this->_ptr = p;

		if(this->_ptr)
		{
			this->_ptr->__incRef();
		}
	}

	template<typename Y>
	ObjectHandle(const ObjectHandle<Y>& r)
	{
		this->_ptr = r._ptr;

		if(this->_ptr)
		{
			this->_ptr->__incRef();
		}
	}

	ObjectHandle(const ObjectHandle& r)
	{
		this->_ptr = r._ptr;

		if(this->_ptr)
		{
			this->_ptr->__incRef();
		}
	}

	~ObjectHandle()
	{
		if(this->_ptr)
		{
			this->_ptr->__decRef();
		}
	}

	ObjectHandle& operator=(T* p)
	{
		if(this->_ptr != p)
		{
			if(p)
			{
				p->__incRef();
			}

			T* ptr = this->_ptr;
			this->_ptr = p;

			if(ptr)
			{
				ptr->__decRef();
			}
		}
		return *this;
	}

	template<typename Y>
	ObjectHandle& operator=(const ObjectHandle<Y>& r)
	{
		if(this->_ptr != r._ptr)
		{
			if(r._ptr)
			{
				r._ptr->__incRef();
			}

			T* ptr = this->_ptr;
			this->_ptr = r._ptr;

			if(ptr)
			{
				ptr->__decRef();
			}
		}
		return *this;
	}

	ObjectHandle& operator=(const ObjectHandle& r)
	{
		if(this->_ptr != r._ptr)
		{
			if(r._ptr)
			{
				r._ptr->__incRef();
			}

			T* ptr = this->_ptr;
			this->_ptr = r._ptr;

			if(ptr)
			{
				ptr->__decRef();
			}
		}
		return *this;
	}

	template<class Y>
	static ObjectHandle dynamicCast(const ObjectHandleBase<Y>& r)
	{
#ifdef __BCPLUSPLUS__
		return ObjectHandle<T>(dynamic_cast<T*>(r._ptr));
#else
		return ObjectHandle(dynamic_cast<T*>(r._ptr));
#endif
	}

	template<class Y>
	static ObjectHandle dynamicCast(Y* p)
	{
#ifdef __BCPLUSPLUS__
		return ObjectHandle<T>(dynamic_cast<T*>(p));
#else
		return ObjectHandle(dynamic_cast<T*>(p));
#endif
	}
};


}}//namespace ZQ::DataPostHouse


#endif//_data_posthouse_common_deinfe_header_file_h__
