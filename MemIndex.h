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
// Ident : $Id: Bz2Stream.h,v 1.13 2004/08/09 10:06:56 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : template of compressed stream
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/MemIndex.h $
// 
// 2     12/11/13 3:50p Hui.shao
// made RTSP sessIndex take new memindex
// 
// 1     12/11/13 3:32p Hui.shao
// created
// ===========================================================================

#ifndef	__ZQ_common_MemoryIndex_H__
#define	__ZQ_common_MemoryIndex_H__

#include "ZQ_common_conf.h"
#include <map>
#include <vector>

namespace ZQ {
namespace common {

// -----------------------------
// template class MemoryIndex
// -----------------------------
template<class KeyT, class ValueT>
class MemoryIndex
{
public:
	typedef std::vector < ValueT > ValueCollection;
protected:
	typedef std::map <KeyT, ValueCollection > Index;
	Index _index;

public:
	MemoryIndex() {}
	virtual ~MemoryIndex()
	{
		clear();
	}

	virtual void clear(void)
	{
		_index.clear();
	}

	virtual void insert(const KeyT& key, const ValueT& val)
	{
		Index::iterator it = _index.find(key);
		if (_index.end() ==it)
		{
			_index.insert(Index::value_type(key, ValueCollection()));
			it = _index.find(key);
			if (_index.end() ==it)
				return;
		}

		it->second.push_back(val); 
	}

	virtual void erase(const KeyT key, const ValueT& val)
	{
		Index::iterator it = _index.find(key);
		if (_index.end() ==it)
			return;

		ValueCollection& vl = it->second;
		for (ValueCollection::iterator itV= vl.begin(); itV <vl.end(); itV++)
		{
			if (*itV != val)
				continue;

			itV = vl.erase(itV); 
			if (vl.end() == itV)
				break;
		}
	}

	virtual ValueCollection find(const KeyT& key)
	{
		Index::iterator it = _index.find(key);
		if (_index.end() ==it)
			return ValueCollection();

		return it->second;
	}

	bool has(const KeyT& key, ValueT& ident)
	{
		Index::iterator it = _index.find(key);
		if (_index.end() ==it)
			return false;

		ValueCollection& vl = it->second;
		for (ValueCollection::iterator itV= vl.first(); itV <vl.end(); itV++)
		{
			if (*itV == key)
				return true; 
		}

		return false;
	}

	size_t size() const
	{
		size_t c=0;
		for (Index::iterator it = _index.begin(); it != _index.end(); it++)
			c += it->second.size();
		return t;
	}

};

} // namespace common
} // namespace ZQ

#endif // __ZQ_common_MemoryIndex_H__

