#ifndef __zq_dataposthouse_shared_object_inline_definition_header_file_h__
#define __zq_dataposthouse_shared_object_inline_definition_header_file_h__

#include <ZQ_common_conf.h>

namespace ZQ
{
namespace DataPostHouse
{
//copy ideas from ICE
//for more detail about ICE .please visit http://www.zeroc.com

#if defined ZQ_OS_MSWIN
struct ice_atomic_t
{
	ice_atomic_t()
	{
		counter = 0;
	}

public:

	volatile LONG counter;
};

inline void ice_atomic_set( ice_atomic_t* v, LONG i )
{
	v->counter = i;
}

inline void ice_atomic_inc(ice_atomic_t *v)
{
	InterlockedIncrement( &(v->counter) );	
}

inline int ice_atomic_dec_and_test( ice_atomic_t *v )
{
	LONG  l = InterlockedDecrement( &(v->counter) );
	return l == 0;
}

inline int ice_atomic_exchange_add( LONG i, ice_atomic_t* v )
{
	return InterlockedExchangeAdd( &( v->counter ) , i );	
}
#elif defined ZQ_OS_LINUX

struct ice_atomic_t
{
	volatile int counter;
	ice_atomic_t()
	{
		counter = 0;
	}
};

inline void ice_atomic_set(ice_atomic_t* v, int i)
{
	v->counter = i;
}

inline void ice_atomic_inc(ice_atomic_t *v)
{
	__asm__ __volatile__(
		"lock ; incl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
}

inline int ice_atomic_dec_and_test(ice_atomic_t *v)
{
	unsigned char c;
	__asm__ __volatile__(
		"lock ; decl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

inline int ice_atomic_exchange_add(int i, ice_atomic_t* v)
{
	int tmp = i;
	__asm__ __volatile__(
		"lock ; xadd %0,(%2)"
		:"+r"(tmp), "=m"(v->counter)
		:"r"(v), "m"(v->counter)
		: "memory");
	return tmp + i;
}

#endif //ZQ_OS

}}//namespace ZQ::DataPostHouse

#endif//__zq_dataposthouse_shared_object_inline_definition_header_file_h__

