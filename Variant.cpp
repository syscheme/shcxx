// ===========================================================================
// Copyright (c) 2004 by
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
// Ident : $Id: Variant.cpp,v 1.8 2004/05/26 09:32:35 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl Variant
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Variant.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 15    09-06-23 15:27 Fei.huang
// * always use long long for int64 on linux
// 
// 14    09-05-11 18:30 Fei.huang
// 
// 13    08-03-06 16:40 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 12    08-03-03 18:22 Yixin.tian
// merged changes for linux
// 
// 11    08-02-19 15:59 Hui.shao
// 
// 10    07-03-28 15:45 Ken.qian
// 
// 9     07-03-20 20:22 Ken.qian
// 
// 8     07-01-03 21:09 Ken.qian
// 
// 7     06-03-23 14:43 Hongquan.zhang
// 
// 6     06-03-16 17:10 Cary.xiao
// 
// 1     06-03-08 15:24 Cary.xiao
// 
// 1     06-03-07 10:44 Cary.xiao
// 
// 1     06-02-27 14:10 Cary.xiao
// 
// 5     11/28/05 6:47p Hui.shao
// 
// 4     11/28/05 6:14p Hui.shao
// added quadword set/get
// 
// 3     9/20/05 4:47p Hui.shao
// define a variant
// ===========================================================================

#include "Variant.h"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#ifdef ZQ_OS_MSWIN
#  include <tchar.h>
#else
#  include <stdarg.h>
#endif
}

#include "base64.h"
#include <sstream>   // for sstream

namespace ZQ {
namespace common {

/// -----------------------------
/// class VariantException
/// -----------------------------
VariantException::VariantException(const std::string &what_arg) throw()
            :Exception(what_arg)
{
}

VariantException::~VariantException() throw()
{
}

/// -----------------------------
/// class Variant
/// -----------------------------
const tchar* Variant::Typename(type_e type)
{
	static const tchar* tnames[] = {
		_TC("nil"), //T_NIL,
		_TC("boolean"), //T_BOOL,
		_TC("BYTE"),    //T_BYTE,
		_TC("short"),   //T_SHORT,
		_TC("WORD"),    // T_USHORT,
		_TC("integer"), //T_INT,
		_TC("long"),   //T_LONG
		_TC("DWORD"),  //T_ULONG
		_TC("double"), //T_DOUBLE,
		_TC("__int64"), // T_LONGLONG
		_TC("ULONGLONG"), // T_ULONGLONG
		_TC("string"), //T_STRING,
		_TC("time"), //T_TIME,
		_TC("binary"), //T_BASE64,
		_TC("array"), //T_ARRAY,
		_TC("struct") //T_STRUCT
	};

	if (type <0 || type > int(sizeof(tnames) / sizeof(tchar*)))
		return tnames[type];
	else return _TC("unknown");
}

Variant::Variant()
: _type(T_NIL)
{
	_value.vBinary = NULL;
}

Variant::Variant(bool value)
: _type(T_BOOL)
{
	_value.vBool = value;
}

Variant::Variant(uint8 value)
: _type(T_BYTE)
{
	_value.vByte = value;
}

Variant::Variant(short value)
: _type(T_SHORT)
{
	_value.vShort = value;
}

Variant::Variant(unsigned short value)
: _type(T_USHORT)
{
	_value.vUShort = value;
}


Variant::Variant(int value)
: _type(T_INT)
{
	_value.vInt = value;
}

Variant::Variant(long value)
: _type(T_LONG)
{
	_value.vLong = value;
}

Variant::Variant(unsigned long value)
: _type(T_ULONG)
{
	_value.vULong = value;
}

Variant::Variant(double value)
: _type(T_DOUBLE)
{
	_value.vDouble = value;
}

#if defined(ZQ_OS_LINUX)
Variant::Variant(long long value)
: _type(T_LONGLONG)
{
	_value.vLonglong = value;
}

Variant::Variant(unsigned long long value)
: _type(T_ULONGLONG)
{
	_value.vULonglong = value;
}
#else
Variant::Variant(int64 value)
: _type(T_LONGLONG)
{
	_value.vLonglong = value;
}

Variant::Variant(uint64 value)
: _type(T_ULONGLONG)
{
	_value.vULonglong = value;
}
#endif
		
Variant::Variant(const tstring & value)
: _type(T_STRING)
{
	_value.vString = new tstring(value);
}
	
Variant::Variant(const tchar* value)
: _type(T_STRING)
{
	_value.vString = new tstring(value);
}
		
Variant::Variant(struct tm* value) 
: _type(T_TIME) 
{
	_value.vTime = new struct tm(*value);
}
		
Variant::Variant(void* value, int nBytes)
: _type(T_BASE64)
{
	uint8* v= (uint8*) value;
	_value.vBinary = new BinaryData(v, (v+nBytes));
}

Variant::Variant(const BinaryData& bindata)
: _type(T_BASE64)
{
	_value.vBinary = new BinaryData(bindata);
}

		
// Copy
Variant::Variant(const Variant& rhs)
: _type(T_NIL)
{
	*this = rhs;
}
		
// Destructor
Variant::~Variant()
{
	clear();
}

void Variant::throwExcpt(const tchar* fmt, ...) throw (VariantException)
{
	char msg[2048];
	va_list args;
	va_start(args, fmt);
	_vTCprintf(msg, fmt, args);
	va_end(args);
	
	throw VariantException(msg);
}

// Clean up
void Variant::clear()
{
	switch (_type) {
	case T_STRING:    delete _value.vString; break;
	case T_TIME:	  delete _value.vTime;   break;
	case T_BASE64:    delete _value.vBinary; break;
	case T_ARRAY:     delete _value.vArray;  break;
	case T_STRUCT:    delete _value.vStruct; break;
	default: break;
	}
	_type = T_NIL;
	_value.vBinary = NULL;
}

// type_e checking
void Variant::assertTypeOrInvalid(type_e t)
{
	if (_type == T_NIL)
	{
		_type = t;
		switch (_type) {    // Ensure there is a valid value for the type
		case T_STRING:   _value.vString = new tstring(); break;
		case T_TIME:	_value.vTime = new struct tm();     break;
		case T_BASE64:   _value.vBinary = new BinaryData();  break;
		case T_ARRAY:    _value.vArray = new ValueArray();   break;
		case T_STRUCT:   _value.vStruct = new ValueStruct(); break;
		default:           _value.vBinary = 0; break;
		}
	}
	else if (_type != t)
		throwExcpt(_TC("Variant::assertTypeOrInvalid() invalid type"));
}

void Variant::assertArray(int size) const
{
	if (_type != T_ARRAY)
		throwExcpt(_TC("type error: expected an array"));
	else if (int(_value.vArray->size()) < size)
		throwExcpt(_TC("range error: array index too large"));
}

void Variant::assertArray(int size)
{
	if (_type == T_NIL)
	{
		_type = T_ARRAY;
		_value.vArray = new ValueArray(size);
	}
	else if (_type == T_ARRAY)
	{
		if (int(_value.vArray->size()) < size)
			_value.vArray->resize(size);
	}
	else
		throwExcpt(_TC("type error: expected an array"));
}

void Variant::assertStruct()
{
	if (_type == T_NIL) {
		_type = T_STRUCT;
		_value.vStruct = new ValueStruct();
	}
	else if (_type != T_STRUCT)
		throwExcpt(_TC("type error: expected a struct"));
}


// Operators
Variant& Variant::operator=(const Variant& rhs)
{
	if (this != &rhs)
	{
		clear();
		_type = rhs._type;
		switch (_type) {
		case T_BOOL:  _value.vBool = rhs._value.vBool; break;
		case T_BYTE:    _value.vByte = rhs._value.vByte; break;
		case T_SHORT:   _value.vShort = rhs._value.vShort; break;
		case T_USHORT:  _value.vUShort = rhs._value.vUShort; break;
		case T_INT:      _value.vInt = rhs._value.vInt; break;
		case T_LONG:    _value.vLong = rhs._value.vLong; break;
		case T_ULONG:   _value.vULong = rhs._value.vULong; break;
		case T_DOUBLE:   _value.vDouble = rhs._value.vDouble; break;
		case T_LONGLONG: _value.vLonglong = rhs._value.vLonglong; break;
		case T_ULONGLONG: _value.vULonglong = rhs._value.vULonglong; break;
		case T_TIME: _value.vTime = new struct tm(*rhs._value.vTime); break;
		case T_STRING:   _value.vString = new tstring(*rhs._value.vString); break;
		case T_BASE64:   _value.vBinary = new BinaryData(*rhs._value.vBinary); break;
		case T_ARRAY:    _value.vArray = new ValueArray(*rhs._value.vArray); break;
		case T_STRUCT:   _value.vStruct = new ValueStruct(*rhs._value.vStruct); break;
		default:           _value.vBinary = 0; break;
		}
	}
	return *this;
}

Variant& Variant::operator=(uint8 const& rhs)
{
	return operator=(Variant(rhs));
}

Variant& Variant::operator=(short const& rhs)
{
	return operator=(Variant(rhs));
}

Variant& Variant::operator=(unsigned short const& rhs)
{
	return operator=(Variant(rhs));
}

Variant& Variant::operator=(int const& rhs)
{
	return operator=(Variant(rhs));
}

Variant& Variant::operator=(long const& rhs)
{
	return operator=(Variant(rhs));
}

Variant& Variant::operator=(unsigned long const& rhs)
{
	return operator=(Variant(rhs));
}

Variant& Variant::operator=(double const& rhs)
{
	return operator=(Variant(rhs));
}

#if defined(ZQ_OS_LINUX)
Variant& Variant::operator=(long long const& rhs)
{
	return operator=(Variant(rhs));
}

Variant& Variant::operator=(unsigned long long const& rhs)
{
	return operator=(Variant(rhs));
}
#else
Variant& Variant::operator=(int64 const& rhs)
{
	return operator=(Variant(rhs));
}

Variant& Variant::operator=(uint64 const& rhs)
{
	return operator=(Variant(rhs));
}
#endif

Variant& Variant::operator=(const tchar* rhs)
{
	return operator=(Variant(tstring(rhs)));
}

// Predicate for tm equality
static bool tmEq(struct tm const& t1, struct tm const& t2) {
	return t1.tm_sec == t2.tm_sec && t1.tm_min == t2.tm_min &&
		t1.tm_hour == t2.tm_hour && t1.tm_mday == t1.tm_mday &&
		t1.tm_mon == t2.tm_mon && t1.tm_year == t2.tm_year;
}

bool Variant::operator==(const Variant& other) const
{
	if (_type != other._type)
		return false;
	
	switch (_type) {
	case T_BOOL:  return ( !_value.vBool && !other._value.vBool) ||
					  ( _value.vBool && other._value.vBool);
	case T_BYTE:     return _value.vByte == other._value.vByte;
	case T_SHORT:    return _value.vShort == other._value.vShort;
	case T_USHORT:   return _value.vUShort == other._value.vUShort;
	case T_INT:      return _value.vInt == other._value.vInt;
	case T_LONG:     return _value.vLong == other._value.vLong;
	case T_ULONG:    return _value.vULong == other._value.vULong;
	case T_DOUBLE:   return _value.vDouble == other._value.vDouble;
	case T_LONGLONG: return _value.vLonglong == other._value.vLonglong;
	case T_ULONGLONG: return _value.vULonglong == other._value.vULonglong;
	case T_TIME: return tmEq(*_value.vTime, *other._value.vTime);
	case T_STRING:   return *_value.vString == *other._value.vString;
	case T_BASE64:   return *_value.vBinary == *other._value.vBinary;
	case T_ARRAY:    return *_value.vArray == *other._value.vArray;
		
		// The map<>::operator== requires the definition of value< for kcc
	case T_STRUCT:   //return *_value.vStruct == *other._value.vStruct;
		{
			if (_value.vStruct->size() != other._value.vStruct->size())
				return false;
			
			ValueStruct::const_iterator it1=_value.vStruct->begin();
			ValueStruct::const_iterator it2=other._value.vStruct->begin();
			while (it1 != _value.vStruct->end()) {
				const Variant& v1 = it1->second;
				const Variant& v2 = it2->second;
				if ( ! (v1 == v2))
					return false;
				it1++;
				it2++;
			}
			return true;
		}
	default: break;
	}
	return true;    // Both invalid values ...
}

bool Variant::operator!=(const Variant& other) const
{
	return !(*this == other);
}

Variant::operator bool&()
{
	assertTypeOrInvalid(T_BOOL);
	return _value.vBool;
}

Variant::operator uint8&()
{
	assertTypeOrInvalid(T_BYTE);
	return _value.vByte;
}

Variant::operator short&()
{
	assertTypeOrInvalid(T_SHORT);
	return _value.vShort;
}

Variant::operator unsigned short&()
{
	assertTypeOrInvalid(T_USHORT);
	return _value.vUShort;
}


Variant::operator int&()
{
	assertTypeOrInvalid(T_INT);
	return _value.vInt;
}

Variant::operator long&()
{
	assertTypeOrInvalid(T_LONG);
	return _value.vLong;
}

Variant::operator unsigned long&()
{
	assertTypeOrInvalid(T_ULONG);
	return _value.vULong;
}

Variant::operator double&()
{
	assertTypeOrInvalid(T_DOUBLE);
	return _value.vDouble;
}

#if defined(ZQ_OS_LINUX)
Variant::operator long long&()
{
	assertTypeOrInvalid(T_LONGLONG);
	return _value.vLonglong;
}

Variant::operator unsigned long long&()
{
	assertTypeOrInvalid(T_ULONGLONG);
	return _value.vULonglong;
}
#else
Variant::operator int64&()
{
	assertTypeOrInvalid(T_LONGLONG);
	return _value.vLonglong;
}

Variant::operator uint64&()
{
	assertTypeOrInvalid(T_ULONGLONG);
	return _value.vULonglong;
}
#endif

Variant::operator tstring&()
{
	assertTypeOrInvalid(T_STRING);
	return *_value.vString;
}

Variant::operator Variant::BinaryData&()
{
	assertTypeOrInvalid(T_BASE64);
	return *_value.vBinary;
}

Variant::operator struct tm&()
{
	assertTypeOrInvalid(T_TIME);
	return *_value.vTime;
}

Variant& Variant::operator[](int i)
{
	assertArray(i+1);
	return _value.vArray->at(i);
}

Variant& Variant::operator[](const tchar* key)
{
	assertStruct();
	tstring s(key);
	return (*_value.vStruct)[s];
}

Variant& Variant::operator[](const tstring& key)
{
	assertStruct();
	return (*_value.vStruct)[key];
}

const tstring& Variant::key(int i)
{
	assertStruct();
	const static tstring emptystr;
	if (i <0 || i >=size())
		return emptystr;

	ValueStruct::iterator it = (*_value.vStruct).begin();
	int j=0;
	for (j=0; j< i && it != (*_value.vStruct).end(); it++, j++);
	
	if (it != (*_value.vStruct).end())
	{
		tstring key = it->first;
		return it->first;
	}

	return emptystr;
}

// Works for strings, binary data, arrays, and structs.
int Variant::size() const
{
	switch (_type)
	{
	case T_STRING: return int(_value.vString->size());
	case T_BASE64: return int(_value.vBinary->size());
	case T_ARRAY:  return int(_value.vArray->size());
	case T_STRUCT: return int(_value.vStruct->size());
	case T_NIL:    return 0;
	default:
		throwExcpt(_TC("Variant::size() type error: %d"), _type);
	}
	return 0;
}

// Checks for existence of struct member
bool Variant::has(const tchar* key) const
{
	return _type == T_STRUCT && _value.vStruct->find(key) != _value.vStruct->end();
}

void Variant::set(int index, const Variant& value)
{
	assertArray(index+1);
	_value.vArray->at(index) = value;
}

void Variant::set(const tchar* key, const Variant& value)
{
	assertStruct();
	tstring s(key);
	(*_value.vStruct)[s] = value;
}

bool Variant::getQuadword(Variant& var, const tchar* key, uint64& i)
{
	if (NULL == key || Variant::T_STRUCT != var.type())
		return false;
	tstring k=key;
	i = 0;
	
	try
	{
		if (var.has((k + ".hdw").c_str()))
		{
			i= (int) var[k + ".hdw"];
			i <<=32;
		}
	}
	catch(...) {}
	
	try
	{
		if (var.has((k + ".hdw").c_str()))
			i |= (int) var[k + ".ldw"];
	}
	catch(...) {}
	
	return true;
}

bool Variant::setQuadword(Variant& var, const tchar* key, const uint64 i)
{
	if (NULL == key || Variant::T_STRUCT != var.type())
		return false;
	tstring k=key;
	
	try
	{
		var.set((k + ".ldw").c_str(), (int)(i & 0xffffffff));
		var.set((k + ".hdw").c_str(), (int)(i <<32));
	}
	catch(...) {}
	return true;
}


/// -----------------------------
/// class VariantException
/// -----------------------------
UnserializeException::UnserializeException(const std::string &what_arg) throw()
            :Exception(what_arg)
{
}

UnserializeException::~UnserializeException() throw()
{
}

/// -----------------------------
/// class Unserializer
/// -----------------------------
void Unserializer::initArray(Variant& var) throw (VariantException)
{
	try {
		var.assertArray(0);
	}
	catch(Exception e)	{ throwExcpt(e.getString());};
}

void Unserializer::initStruct(Variant& var) throw (VariantException)
{
	try {
		var.assertStruct();
	}
	catch(Exception e)	{ throwExcpt(e.getString());};
}

void Unserializer::initBinary(Variant& var, const Variant::BinaryData& bindata) throw (VariantException)
{
	try {
		var = Variant(bindata);
	}
	catch(Exception e)	{ throwExcpt(e.getString());};
}

void Unserializer::throwExcpt(const tchar* fmt, ...) throw (UnserializeException)
{
	char msg[2048];
	va_list args;
	va_start(args, fmt);
	_vTCprintf(msg, fmt, args);
	va_end(args);
	
	throw UnserializeException(msg);
}

/// -----------------------------
/// class Serializer
/// -----------------------------
#define DATATIME_FMT "%4d-%2d-%2d %2d:%2d:%2d"

static void serializeEx(Variant& var, tostream& ostrm)
{
	switch (var.type())
	{
	case Variant::T_NIL: 
		ostrm << "NIL";
		break;

	case Variant::T_BOOL: 
		{
			bool val = var;
			ostrm << (val ? "true":"false");
		}
		break;

	case Variant::T_BYTE:
		ostrm << ((uint8)var);
		break;

	case Variant::T_SHORT:
		ostrm << ((short)var);
		break;

	case Variant::T_USHORT:
		ostrm << ((unsigned short)var);
		break;

	case Variant::T_INT:
		ostrm << ((int)var);
		break;

	case Variant::T_LONG:
		ostrm << ((long)var);
		break;

	case Variant::T_ULONG:
		ostrm << ((unsigned long)var);
		break;

	case Variant::T_DOUBLE:
		ostrm << ((double)var);
		break;

	case Variant::T_LONGLONG:
		ostrm << ((int64)var);
		break;

	case Variant::T_ULONGLONG:
		ostrm << ((uint64)var);
		break;

	case Variant::T_TIME:
		{
			tchar dtstr[80];
			struct tm tmst = var;
			_sTCprintf(dtstr, _T(DATATIME_FMT), tmst.tm_year, tmst.tm_mon, tmst.tm_mday, tmst.tm_hour, tmst.tm_min, tmst.tm_sec);
			ostrm << dtstr;
		}
		break;

	case Variant::T_STRING:
		{
			tstring tmp = (tstring)var;
			for (tstring::size_type pos = tmp.find('\"'); pos != tstring::npos; pos = tmp.find('\"', pos+2))
			{
				tmp.replace(pos, 1, "\\\"");
			}

			ostrm << "\"" << tmp << "\"";
		}
		break;

	case Variant::T_BASE64:
		{
			// convert to base64
			Variant::BinaryData bindata = var;
			std::vector<tchar> base64data;
			int iostatus = 0;
			base64<tchar> encoder;
			std::back_insert_iterator<std::vector<tchar> > ins = std::back_inserter(base64data);
			encoder.put(bindata.begin(), bindata.end(), ins, iostatus, base64<>::crlf());
			
			ostrm << "[" ;
			for (std::vector<tchar>::iterator it = base64data.begin(); it < base64data.end() ; it ++)
				ostrm << ((tchar) *it);
			
			ostrm << "]";
			
		}
		break;

	case Variant::T_ARRAY:
		{
			ostrm << "{";
			for (int i=0; i< var.size(); i++)
			{
				if (i >0)
					ostrm <<", ";
				serializeEx(var[i], ostrm);
			}
			ostrm << "}";
		}
		break;

	case Variant::T_STRUCT:
		{
			ostrm << "{";
			for (int i=0; i< var.size(); i++)
			{
				if (i >0)
					ostrm <<", ";
				tstring key = var.key(i);
				ostrm << key << "=";
				serializeEx(var[key], ostrm);
			}
			ostrm << "}";
		}
		break;

	}
}

void Serializer::serialize()
{
	serializeEx(_var, _ostrm);
}

} // namespace common
} // namespace ZQ
