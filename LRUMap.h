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
// Ident : $Id: LRUMap.h,v $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define Base Logger
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/LRUMap.h $
// 
// 14    2/24/16 11:34a Hongquan.zhang
// revert to previous version
// 
// 10    11/13/15 3:35p Ketao.zhang
// 
// 9     10/16/15 4:09p Ketao.zhang
// 
// 8     9/10/15 3:42p Hongquan.zhang
// remove unreferenced header file
// 
// 7     9/09/15 3:03p Ketao.zhang
// check in for LRUmap bug in operator[]
// 
// 6     11/20/13 11:43a Hui.shao
// 
// 2     11/20/13 11:42a Hui.shao
// 
// 5     11/19/13 10:42a Hui.shao
// 
// 3     5/15/13 3:23p Bin.ren
// 
// 2     4/18/13 1:52p Li.huang
// 
// 1     1/09/13 4:59p Hui.shao
// simple LRU impl
// ===========================================================================

#ifndef __ZQ_Common_LRUMap_H__
#define __ZQ_Common_LRUMap_H__

#include "ZQ_common_conf.h"
#include "Locks.h"

#include <map>
#include <string>


namespace ZQ {
namespace common {

// Note: thread unsafe
template< class K, class V >
class LRUMap 
{
protected:
    typedef std::map< K, V > base_type;
    typedef size_t timestamp;
    typedef typename std::map< K, timestamp > KeyToStamp;
    typedef typename std::map< timestamp, K > StampToKey;

    KeyToStamp _k2t; // from key to timestamp
    StampToKey _t2k; // from timestamp to key
	base_type  _realData;

    timestamp _stampLast;
    size_t _capacity;

public:
	typedef typename base_type::iterator iterator;
	typedef typename base_type::const_iterator const_iterator;
	typedef typename base_type::value_type value_type;

    LRUMap(size_t cap=1000)
        : _stampLast(1), _capacity(cap)
	{}

	iterator find( const K& k ) {
		return _realData.find(k);
	}

	const_iterator find( const K& k ) const {
		return _realData.find(k);
	}

	iterator end() {
		return _realData.end();
	}

	const_iterator end() const {
		return _realData.end();
	}

	V& operator[](const K& k)
	{

		timestamp theStamp = _k2t[k];
		// if the timestamp already exist, delete it from _t2k
		if (theStamp) {
			_t2k.erase(theStamp);
			_k2t.erase(k);
		}

		// update timestamp in _k2t
		if (0 == (theStamp = _stampLast++))
		{
			// simply clear the LRU if _stampLast rounds over
			_k2t.clear();
			_t2k.clear();
			_realData.clear();

			// recalcuate the new theStamp
			theStamp = _stampLast++;
		}

		_k2t[k] = theStamp;
		_t2k[theStamp]=k; // update key in _t2k
		V& v = _realData[k];

		// remove the oldest if necessary
		if ( _realData.size() + 1 > _capacity && _realData.size() >= 1 )
        {
			K kToBeErased = _t2k.begin()->second; // get the eldest key
            erase(kToBeErased);
        }
		return v;
	}

	size_t size(){ return _realData.size() ;}
	
	void  erase_eldest()
	{
		if(_realData.size() <= 0 )
			return;
		K kToBeErased = _t2k.begin()->second; // get the eldest key
		//V& v =  _realData[kToBeErased];
		erase(kToBeErased);
	}


	void erase(const K& k)
    {
        // erase timestamp <-> key reference
        _t2k.erase(_k2t[k]);
        _k2t.erase(k);
        // then the actual data
        _realData.erase(k);
    }

	void resize(size_t size)
	{
		_capacity = size;
	}

};

} // namespace common
} // namespace ZQ

#endif // __ZQ_Common_LRUMap_H__
