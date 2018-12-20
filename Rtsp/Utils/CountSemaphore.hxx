#ifndef COUNT_SEMAPHORE_HXX_
#define COUNT_SEMAPHORE_HXX_

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

static const char* const CountSemaphore_hxx_Version =
    "$Id: CountSemaphore.hxx,v 1.14 2001/05/25 23:19:53 sprajpat Exp $";

#include "VMutex.h"
#include "global.h"
#include <assert.h>


#if !defined (WIN32) && (defined(__svr4__) || defined (__SUNPRO_CC) || !defined(__GNUC__))

#define USE_VMUTEX_LOCKING 1
#else
#define USE_VMUTEX_LOCKING 0
#endif

#if USE_VMUTEX_LOCKING

// these macros define the appropriate operators if using VMutex as the basis for
// the reference counting.
#define LOCK() mutex.lock()
#define UNLOCK() mutex.unlock()
#define INC(x) ++x
#define DEC(x) --x
#define DEC_AND_CMP(x,r) --x,r = (x == 0)
#define EXCHANGE(ptr,val) void* tmp; tmp = *ptr,*ptr = val, val = tmp
#else

// these macros define the appropriate operators if using i386 assembly for
// the reference counting.

#define LOCK()
#define UNLOCK()

#ifdef WIN32
#include <windows.h>
#define INC(x)				InterlockedIncrement(&(x))
#define DEC(x)				InterlockedDecrement(&(x))
#define DEC_AND_CMP(x,r)	DEC(x),r = ((x) == 0)
#define EXCHANGE(ptr,val)	(val = (void *)InterlockedExchange((volatile long* )ptr, (volatile long )val))

#else // #ifdef WIN32

#define INC(x) \
({   \
asm volatile ("lock; incl %0" : : "m" (x) ); \
})

#define DEC(x) \
({   \
asm volatile ("lock; decl %0" : : "m" (x) ); \
})

#define DEC_AND_CMP(x,r) \
({   \
asm volatile ("lock; decl %1; sete %0 " : "=m" (r) : "m" (x) ); \
})

#define EXCHANGE(ptr,val) \
({   \
asm volatile ("lock; xchg %0,%1" : "=r" (val), "=m" (*ptr) : "m" (*ptr) , "0" (val) ); \
})

#endif // #ifdef WIN32
#endif // #if USE_VMUTEX_LOCKING

#define INLINE_ inline

/* TODO: 

   fix the name.  It should be called ReferenceCount or something like
   that.  

   figure out why the count is volatile.

   change decrement() to decrementAndCompare() .

*/

/** A thread safe reference count.  The implementation is designed to
 * use either VMutex or inline assembly when using GCC (for
 * efficiency).
 */
class CountSemaphore
{
    public:
	/// constructor
        INLINE_ explicit CountSemaphore(long value=0)
        :
#if USE_VMUTEX_LOCKING
        mutex(),
#endif
        count(value)
        {}

	/** compares the current value in the reference count to
	 * value.  returns true if equal.
	 * @param value   integer to compare against.
	 */
        INLINE_ bool compare(long value)
        {
            return (value == count);
        }

	/** increment the reference count by one.  This operation is atomic.
	 */
        INLINE_ void increment()
        {
            LOCK();
            INC(count);
            UNLOCK();
        }

	/** decrement the reference count by one.  This operation is
	 * atomic.  If the reference count now equals 0, decrement
	 * returns true.
	 */
        INLINE_ bool decrement()
        {
            bool retVal;
            LOCK();
            assert(count > 0);
            DEC_AND_CMP(count, retVal);
            UNLOCK();
            return retVal;
        }

        INLINE_ void exchange(void** ptr, void** val)
        {
            LOCK();
	    EXCHANGE(ptr, *val);
            UNLOCK();
        }

	INLINE_ long getCount()
	{
	    return count;
	}

	bool operator==(long value) const
	{
            return (value == count);
	}

    private:
	/// suppress copying
        CountSemaphore(const CountSemaphore&);
	/// suppress copying
        const CountSemaphore& operator=(const CountSemaphore&);
	/// suppress comparison
        bool operator==(const CountSemaphore&);

#if USE_VMUTEX_LOCKING
        VMutex mutex;
#endif

        volatile long count; // why is this volatile ?
};

#endif
