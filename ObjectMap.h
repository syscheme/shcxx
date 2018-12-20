// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: ObjectMap.h,v 1.2 2004/06/22 13:21:45 wli Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define UDPDuplex, a two-way UDP connection
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ObjectMap.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// Revision 1.2  2004/06/22 13:21:45  wli
// CORRECT VARIABLE ERROR
//
// Revision 1.1  2004/06/22 06:13:02  shao
// created
//
// ===========================================================================

#ifndef	__ZQ_COM_ObjectMap_H__
#define	__ZQ_COM_ObjectMap_H__

#include "ZQ_common_conf.h"
#include "Exception.h"
#include "Locks.h"

#include <map>

namespace ZQ {
namespace common {

template < typename _Key, typename _Obj >
class ObjectMap
{
public: 

	ObjectMap() {};
	virtual ~ObjectMap() {};

	bool reg(_Key k, _Obj* obj);
	bool unreg(_Key k, _Obj* obj);

	_Obj* find(_Key k);

protected:
	//
	typedef std::map< _Key, _Obj* > ObjMap_t;

	ObjMap_t	_objs;
	Mutex		_objsMutex;
};

template < typename _Key, typename _Obj >
bool ObjectMap< _Key, _Obj >::reg(_Key k, _Obj* obj)
{
	if (obj == NULL)
		return false;

	bool  succ = false;

	Guard<Mutex> guard(_objsMutex);

	if (_objs[k] != NULL)
		succ = (_objs[k] == obj);
	else
	{
		_objs[k] = obj;
		succ = true;
	}

	return succ;
}

template < typename _Key, typename _Obj >
bool ObjectMap< _Key, _Obj >::unreg(_Key k, _Obj* obj)
{
	bool  succ = false;

	Guard<Mutex> guard(_objsMutex);

	ObjMap_t::iterator i = _objs.find(k);

	if (i == _objs.end())
		succ = false;
	else if (NULL == obj || obj == i->second)
	{
		_objs.erase(i);
		succ = true;
	}

	return succ;
}

template < typename _Key, typename _Obj >
_Obj* ObjectMap< _Key, _Obj >::find(_Key k)
{
	ObjMap_t::iterator i = _objs.find(k);
	return (i == _objs.end()) ? NULL : i->second;
}

} // namespace common
} // namespace ZQ

#endif //__ZQ_COM_ObjectMap_H__
