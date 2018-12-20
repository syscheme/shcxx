// ===========================================================================
// Copyright (c) 2005 by
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
// ----------------------------
//
// Name  : xlist.h
// Author: Cary (cary.xiao@i-zq.com)
// Date  : 07/15/2005
// Desc  : Implementation the CString without MFC. And support to be multi-platform
//
// Revision History:
//
//
// ----------------------------
//
//
// ===========================================================================

#ifndef _XLIST_H_

#include <assert.h>
#include <list>
#include <iterator>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

#ifndef _xassert
#define _xassert(e)		assert((e))
#endif

//#define POSITION		iterator

#define _P2I(pos)		(pos)
#define _I2P(itor)		(itor)

template <class TYPE, class ARG_TYPE>
class XListT : public std::list<TYPE> {
public:

	typedef iterator POSITION;

	XListT()
	{

	}
	
	int GetCount() const
	{
		return size();
	}

	bool IsEmpty() const
	{
		return size() <= 0;
	}

	TYPE& GetHead()
	{
		return front();
	}

	TYPE GetHead() const
	{
		return front();
	}

	TYPE GetTail() const
	{
		return back();
	}

	TYPE& GetTail()
	{
		return back();
	}

// Operations
	// get head or tail (and remove it) - don't call on empty list !
	TYPE RemoveHead()
	{
		TYPE e = front();
		pop_front();
		return e;
	}

	TYPE RemoveTail()
	{
		TYPE e = back();
		pop_back();
		return e;
	}

	// add before head or after tail
	POSITION AddHead(ARG_TYPE newElement)
	{
		push_front(newElement);
		return begin();
	}

	POSITION AddTail(ARG_TYPE newElement)
	{
		push_back(newElement);
		return end();
	}

	// remove all elements
	void RemoveAll()
	{
		clear();
	}

	// iteration
	POSITION GetHeadPosition() 
	{
		return _I2P(begin());
	}

	POSITION GetTailPosition() 
	{
		return _I2P(end());
	}

	TYPE& GetNext(POSITION& rPosition) // return *Position++
	{
		iterator iter = _P2I(rPosition);
		TYPE& e = *iter;
		iter ++;
		if (iter == end())
			rPosition = NULL;
		else {
			rPosition = _I2P(iter);
		}

		return e;
	}

	TYPE GetNext(POSITION& rPosition) const // return *Position++
	{
		iterator iter = _P2I(rPosition);
		TYPE& e = *iter;
		iter ++;
		if (iter == end())
			rPosition = NULL;
		else {
			rPosition = _I2P(iter);
		}

		return e;
	}

	TYPE& GetPrev(POSITION& rPosition) // return *Position--
	{
		iterator iter = _P2I(rPosition);
		if (iter == end())
			iter --;

		TYPE& e = *iter;
		if (iter == begin())
			rPosition = NULL;
		else {
			iter --;
			rPosition = _I2P(iter);
		}
		return e;
	}

	TYPE GetPrev(POSITION& rPosition) const // return *Position--
	{
		iterator iter = _P2I(rPosition);
		_xassert(iter != begin());
		TYPE& e = *iter;
		iter --;		
		if (iter == begin())
			rPosition = NULL;
		else
			rPosition = _I2P(iter);
		return e;
	}

	// getting/modifying an element at a given position
	TYPE& GetAt(POSITION position)
	{
		iterator iter = _I2P(rPosition);
		return *iter;
	}

	TYPE GetAt(POSITION position) const
	{
		iterator iter = _P2I(rPosition);
		return *iter;
	}

	void SetAt(POSITION pos, ARG_TYPE newElement)
	{
		iterator iter = _P2I(pos);
		*iter = newElement;
	}

	void RemoveAt(POSITION position)
	{
		erase(_P2I(position));
	}

	// inserting before or after a given position
	POSITION InsertBefore(POSITION position, ARG_TYPE newElement)
	{
		iterator iter = _P2I(position);
		insert(iter, newElement);
		return -- iter;
	}

	POSITION InsertAfter(POSITION position, ARG_TYPE newElement)
	{
		iterator iter = _P2I(position);
		iter ++;
		insert(iter, newElement);
		return -- iter;
	}

	// helper functions (note: O(n) speed)
	// defaults to starting at the HEAD, return NULL if not found
	POSITION Find(ARG_TYPE searchValue, POSITION startAfter = NULL)
	{
		iterator iter;
		if (startAfter == NULL)
			iter = begin();
		else
			iter = _P2I(startAfter);

		for ( ; iter != end( ); iter ++) {
			if (*iter == searchValue)
				return iter;
		}

		return NULL;
	}
	
	POSITION FindIndex(int nIndex)
	{
		_xassert(nIndex >= 0 && nIndex < size());
		iterator iter;
		for ( iter = begin( ); iter != end( ); iter ++, nIndex --) {
			if (nIndex <= 0)
				break;
		}

		return iter;
	}

};

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifdef _XSTRING_H_
