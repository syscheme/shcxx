#ifndef SPTR_REFCOUNT_HXX_
#define SPTR_REFCOUNT_HXX_

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

static const char* const Sptr_RefCount_hxx_Version =
    "$Id: SptrRefCount.hxx,v 1.5 2001/05/23 01:51:47 bko Exp $";

#include <assert.h>
#include "VMutex.h"
// #include "debug.h"
#include "CountSemaphore.hxx"

// Added By Cary
#ifndef DYNAMIC_CAST
#ifdef _CPPRTTI
#define DYNAMIC_CAST dynamic_cast
#else
#define DYNAMIC_CAST static_cast
#endif
#endif

#define SPTR_DEBUG 1
#define SPTR_TRACEBACK 1

#ifdef CODE_OPTIMIZE
// turn off debugging and tracebacks in optimized code

#undef SPTR_DEBUG
#undef SPTR_TRACEBACK
#define SPTR_DEBUG 0
#define SPTR_TRACEBACK 0

#endif

#if SPTR_DEBUG

#include <map>

/** assists in debugging Sptrs (both use and implementation), by
 * constructing a map of currently active Sptrs.  A singleton. */
class SptrDebugger
{
    private:
	/// default constructor
        SptrDebugger();
	/// used to get an instance.  If no instance exists, create one
        static SptrDebugger* getInstance();

        typedef std::map< void*, bool > PtrMap;

        static SptrDebugger* impl_;

        PtrMap ptrsInUse;

        VMutex mutex;

    public:
	/// add a new pointer when it is new.
        static void newPtr(void* ptr);
	/// remove an pointer after it has been deleted.
        static void deletePtr(void* ptr);
};

// these functions are used as debug breakpoints if SPTR_TRACEBACK
// is set to 1

void sptrDebugMarkTraceback(void* sptr, void* ptr);
void sptrDebugClearTraceback(void* sptr, void* ptr);
void sptrDebugDumpTraceback(char* filename);

#endif

/* TODO: this should be implemented via the CountSemaphore abstraction
   (which should be renamed ) */

/** Template simulates a "smart" pointer which deletes the item it is
    pointing to when no more references to the item exist.  Warning:
    circular references will produce memory leaks. 
    
    <P>Note that only one Sptr should be constructed from the original
    ptr -- Sptr will free twice (and cause havoc) if it is misused like so:
    <P>
    <B> WRONG: </B>
    <PRE>
        T* obj = new T();
        Sptr<T> p;
        Sptr<T> q;
        p = obj;
        q = obj;  
           <I>now both p and q think they are the only ones who will free the 
              memory, so you will get an error.</i>
    </PRE>

*/
template < class T >
class Sptr
{
private:

	mutable  T* ptr;
	mutable CountSemaphore* count;

	/// increment the reference count.
        void increment()
        {
            if (ptr)
            {
                if (!count)
                {
                    count = new CountSemaphore();
                }

		count->increment();
            }
#if SPTR_TRACEBACK
            sptrDebugMarkTraceback((void*) this, (void*) ptr);
#endif

        }

	/// decrement the reference count
        void decrement()
        {
            if (ptr && count)
            {
#if SPTR_TRACEBACK
                sptrDebugClearTraceback((void*) this, (void*) ptr);
#endif
		count->decrement();

                if (count->compare(0))
                {
#if SPTR_DEBUG
#if 0
                    cerr << "deleting " << ptr << endl;
                    breakpoint();
#endif
                    SptrDebugger::deletePtr((void*) ptr);
#endif
                    delete ptr;
                    delete count;
                }
            }
            ptr = 0;
            count = 0;
        }

    public:
#if defined (__SUNPRO_CC)
	/** conversion operator converts pointers of this class to
	 * class Sptr< T2 >., where T2 is a different base class.  This
	 * is most often used when attempting to call a method of the
	 * base class through a derived class pointer.
	 */
        template < class T2 >
        operator Sptr<T2 > () 
        {
            return Sptr < T2 > (ptr, count);
        }
#endif

	/** conversion operator converts pointers of this const class
	 * to class const Sptr< T2 >., where T2 is a different base
	 * class.  This is most often used when attempting to call a
	 * const method of the base class through a derived class
	 * pointer.  This is a workaround for SUNPRO .
	 */
	template < class T2 >
        operator const Sptr<T2 > () const
        {
            return Sptr < T2 > (ptr, count);
        }

	/// default constructor.  points to NULL.
        Sptr() : ptr(0), count(0)
        {}
        ;

	/** constructor used most often as the constructor from a
	 * plain pointer.  Do not use this to convert a single pointer
	 * to a smart pointer multiple times -- this will result in an
	 * error (see class introduction for details).
	 */
        Sptr(T* original, CountSemaphore* myCount = 0)
        : ptr(original), count(myCount)
        {
            if (ptr)
            {
#if SPTR_DEBUG
                if (!myCount)
                {
                    SptrDebugger::newPtr((void*) ptr);  // void* takes away warning
                }

#endif
                increment();
            }
        };

	/** copy constructor
	 */
        Sptr(const Sptr& x)
        : ptr(x.ptr), count(x.count)
        {
            increment();
        };

	/// destructor
        ~Sptr()
        {
            {
                decrement();
            }
        }

	/// dereference operator
        T& operator*() const
        {
            assert(ptr);
            return *ptr;
        }

	/// ! operator .  Returns true if ptr == 0, false otherwise.
        int operator!() const
        {
            if (ptr)
            {
                return (ptr == 0);
            }
            else
                return true;
        }

	/// pointer operator.
        T* operator->() const
        {
            return ptr;
        }

	/** dynamicCast works similarly to the actual dynamic_cast()
	 * operator, like so:
	 
	 <PRE>
	     class A {
	       ...
	     };
	     class B : public A {
	       ...
	     };
	     class C {
	       ...
	     };
	     ...
	     int main()
	     {
		 Sptr< A > a;
		 Sptr< B > b;
		 Sptr< C > c;

		 a = new B;

		 b.dynamicCast(a);
		 // now, b points to the same thing as a

		 c.dynamicCast(a);
		 // now, c is the NULL pointer.
	     }
	 </PRE>
	 */
        template < class T2 > Sptr& dynamicCast(const Sptr < T2 > & x)
        {
            if (ptr == x.getPtr()) return *this;
            decrement();
			
			// Modified by Cary
            if(T* p = DYNAMIC_CAST < T* > (x.getPtr()))
            {
                count = x.getCount();
                ptr = p;
                increment();
            }
            return *this;
        }
	/** assignment operator -- this is most often used to assign
	 * from a smart pointer to a derived type to a smart pointer
	 * of the base type.
	 */
        template < class T2 >
        Sptr& operator=(const Sptr < T2 > & x)
        {
            if (ptr == x.getPtr()) return * this;
            decrement();
            ptr = x.getPtr();
            count = x.getCount();
            increment();
            return *this;
        }


	/** assignment operator from plain pointer.  Do not use this
	 * to convert a single pointer to a smart pointer multiple
	 * times -- this will result in an error (see class
	 * introduction for details).
	 */
        Sptr& operator=(T* original)
        {
            if (ptr == original) return * this;
            decrement();
#if SPTR_DEBUG
            SptrDebugger::newPtr((void*) original);
#endif
            ptr = original;
            increment();
            return *this;
        };

	/// assignment operator
        Sptr& operator=(const Sptr& x)
        {
            if (ptr == x.ptr) return * this;
            decrement();
            ptr = x.ptr;
            count = x.count;
            increment();
            return *this;
        }

	/// compare whether a pointer and a smart pointer point to different things
        friend bool operator!=(const void* y, const Sptr& x)
	{
            if (x.ptr != y)
                return true;
            else
                return false;
	}

	/// compare whether a smart pointer and a pointer point to different things
        friend bool operator!=(const Sptr& x, const void* y)
	{
            if (x.ptr != y)
                return true;
            else
                return false;
	}

	/// compare whether a pointer and a smart pointer point to the same thing
        friend bool operator==(const void* y, const Sptr& x)
        {
            if (x.ptr == y)
                return true;
            else
                return false;
        }

	/// compare whether a smart pointer and a pointer point to the same thing
        friend bool operator==(const Sptr& x, const void* y)
        {
            if (x.ptr == y)
                return true;
            else
                return false;
        }

	/// compare whether two smart pointers point to the same thing
        bool operator==(const Sptr& x) const
        {
            if (x.ptr == ptr)
                return true;
            else
                return false;
        }

	/// compare whether two smart pointers point to the same thing
        bool operator!=(const Sptr& x) const
        {
            if (x.ptr != ptr)
                return true;
            else
                return false;
        }

	/**
	   this interface is here because it may sometimes be
	   necessary.  DO NOT USE unless you must use it.
	   get the actual mutex of the smart pointer.
	*/
        VMutex* getMutex() const
        {
            return 0;
        }

	/**
	   this interface is here because it may sometimes be
	   necessary.  DO NOT USE unless you must use it.
	   get the value of the reference count of the smart pointer.
	*/
        CountSemaphore* getCount() const
        {
            return count;
        }

	/**
	   this interface is here because it may sometimes be
	   necessary.  DO NOT USE unless you must use it.
	   get the pointer to which the smart pointer points.
	*/
        T* getPtr() const
        {
            return ptr;
        }

};

#endif
