// ===========================================================================
// Copyright (c) 2010 by
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
// Ident : $Id: Pointer.cpp$
// Branch: $Name:  $
// Author: HongQuan Zhang
// Desc  : impl smart pointer
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Pointer.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 3     10-10-21 15:14 Hui.shao
// 
// 2     10-09-16 19:24 Hui.shao
// ===========================================================================

#include "Pointer.h"

namespace ZQ{
namespace common{

// -----------------------------
// class SharedObjExpcetion
// -----------------------------
SharedObjExpcetion::SharedObjExpcetion(const std::string& what_arg)
		:ZQ::common::Exception(what_arg)
{
}

// -----------------------------
// class NullSharedObjException
// -----------------------------
NullSharedObjException::NullSharedObjException(const std::string& what_arg)
:SharedObjExpcetion(what_arg)
{
}

// -----------------------------
// class SharedObject
// -----------------------------
SharedObject::SharedObject( )
 : _noDelete(false)
{
}

SharedObject::SharedObject(const SharedObject&)
 :_noDelete(false)
{
}

SharedObject::~SharedObject()
{
}

bool SharedObject::operator == ( const SharedObject& b)
{
	return (this == &b);
}
	
void SharedObject::__incRef( )
{
	_ref.inc();
}

void SharedObject::__decRef( )
{
	if( _ref.decThenIfZero() && !_noDelete)
	{
		_noDelete = true;
		delete this;
	}
}
	
int SharedObject::__getRef()
{
	 return _ref.add(0);
}
	
void SharedObject::__setNoDelete(bool noDel)
{
	_noDelete = noDel;
}
	

}
}//endof namespace


