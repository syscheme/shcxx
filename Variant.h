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
// Ident : $Id: Variant.h,v 1.8 2004/05/26 09:32:35 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define Variant
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Variant.h $
// 
// 3     12/19/12 5:26p Hongquan.zhang
// 
// 2     12/19/12 3:41p Hui.shao
// printf-like
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 16    09-06-23 15:27 Fei.huang
// * always use long long for int64 on linux
// 
// 15    08-12-09 14:42 Yixin.tian
// 
// 14    08-03-06 16:40 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 13    08-03-03 18:22 Yixin.tian
// merged changes for linux
// 
// 12    08-02-19 15:59 Hui.shao
// 
// 11    07-12-14 13:58 Ken.qian
// adjust header include order 
// 
// 10    07-03-28 15:45 Ken.qian
// 
// 9     07-03-20 20:22 Ken.qian
// 
// 8     07-01-03 21:09 Ken.qian
// ===========================================================================

#ifndef __ZQ_Common_Variant_H__
#define __ZQ_Common_Variant_H__


#include<string>
#include<vector>
#include<iostream>

#include "ZQ_common_conf.h"
#include "Exception.h"
#ifdef ZQ_OS_MSWIN
#include "TCHAR.h"
#endif


#if defined(_UNICODE_COMMON)
#  define tstring std::wstring
#  define tistream std::wistream
#  define tchar  wchar_t
#  define _TC	_T
#  define _sTCprintf wprintf
#  define _vTCprintf swprintf
#  define tostream std::wostream
#else
#  define tstring std::string
#  define tchar char
#  define _TC	
#  define _sTCprintf printf
#  define _vTCprintf sprintf
#  define tistream std::istream
#  define tostream std::ostream
#endif // !_UNICODE

#include <vector>
#include <map>

namespace ZQ {
namespace common {

class ZQ_COMMON_API Variant;
class ZQ_COMMON_API VariantException;
class ZQ_COMMON_API Unserializer;
class ZQ_COMMON_API UnserializeException;
class ZQ_COMMON_API Serializer;
	
/// -----------------------------
/// class VariantException
/// -----------------------------
class VariantException : public Exception
{
public:
	VariantException(const std::string &what_arg) throw();
	virtual ~VariantException() throw();
};

/// -----------------------------
/// class Variant
/// -----------------------------
class Variant
{
	friend class Unserializer;
public:
	typedef enum _type_e
	{
		T_NIL,
			T_BOOL,
			T_BYTE,
			T_SHORT,
			T_USHORT,
			T_INT,
			T_LONG,
			T_ULONG,
			T_DOUBLE,
			T_LONGLONG,
			T_ULONGLONG,
			T_STRING,
			T_TIME,
			T_BASE64,
			T_ARRAY,
			T_STRUCT
	} type_e;
	
	// Non-primitive types
	typedef std::vector<uint8> BinaryData;
	typedef std::vector<Variant> ValueArray;
	typedef std::map<tstring, Variant> ValueStruct;
	
	static const tchar* Typename(type_e type);
	
public:
	
	/// Constructors
	Variant();
	Variant(bool value);
	Variant(uint8 value);
	Variant(short value);
	Variant(unsigned short value);
	Variant(int value);
	Variant(long value);
	Variant(unsigned long value);
	Variant(double value);
#if defined(ZQ_OS_LINUX)
	Variant(long long value);
	Variant(unsigned long long value);
#else
	Variant(int64 value);
	Variant(uint64 value);
#endif
	
	Variant(const tstring & value);
	Variant(const tchar* value);
	
	Variant(struct tm* value);
	
	Variant(void* value, int nBytes);

	/// Copy
	Variant(const Variant& rhs);
	
	/// Destructor (make virtual if you want to subclass)
	virtual ~Variant();

protected:
	Variant(const BinaryData& bindata);

public:
	
	/// Erase the current value
	void clear();
	
	// Operators
	Variant& operator=(const Variant& rhs);
	Variant& operator=(uint8 const& rhs);
	Variant& operator=(short const& rhs);
	Variant& operator=(unsigned short const& rhs);
	Variant& operator=(int const& rhs);
	Variant& operator=(long const& rhs);
	Variant& operator=(unsigned long const& rhs);
	Variant& operator=(double const& rhs);
#if defined(ZQ_OS_LINUX)
	Variant& operator=(long long const& rhs);
	Variant& operator=(unsigned long long const& rhs);
#else
	Variant& operator=(int64 const& rhs);
	Variant& operator=(uint64 const& rhs);
#endif
	Variant& operator=(const tchar* rhs);
	
	bool operator==(const Variant& other) const;
	bool operator!=(const Variant& other) const;
	
	operator bool&();
	operator uint8&();
	operator short&();
	operator unsigned short&();
	operator int&();
	operator long&();
	operator unsigned long&();
	operator double&();
#if defined(ZQ_OS_LINUX)
	operator long long&();
	operator unsigned long long&();
#else
	operator int64&();
	operator uint64&();
#endif
	operator tstring&();
	operator BinaryData&();
	operator struct tm&();
	
	Variant& operator[](int i);

	const tstring& key(int i);
	Variant& operator[](const tchar* key);
	Variant& operator[](const tstring& key);
	
	void set(int index, const Variant& value);
	void set(const tchar* key, const Variant& value);
	
	// Accessors
	/// Return true if the value has been set to something.
	bool valid() const { return _type != T_NIL; }
	
	/// Return the type of the value stored. \see type_e.
	type_e const & type() const { return _type; }
	
	/// Return the size for string, base64, array, and struct values.
	int size() const;
	
	/// Specify the size for array values. Array values will grow beyond this size if needed.
	void setSize(int size)    { assertArray(size); }
	
	/// Check for the existence of a struct member by name.
	bool has(const tchar* key) const;

	static bool getQuadword(Variant& var, const tchar* key, uint64& i);
	static bool setQuadword(Variant& var, const tchar* key, const uint64 i);

	
protected:
	
	static void throwExcpt(const tchar* fmt, ...) throw (VariantException) PRINTFLIKE(1, 2);

	// type_e checking
	void assertTypeOrInvalid(type_e t);
	void assertArray(int size) const;
	void assertArray(int size);
	void assertStruct();
	
	// type_e tag and values
	type_e _type;
	
	// At some point I will split off Arrays and Structs into
	// separate ref-counted objects for more efficient copying.
	union {
		bool          vBool;
		uint8          vByte;
		short         vShort;
		unsigned short vUShort;
		int           vInt;
		long          vLong;
		unsigned long vULong;
		double        vDouble;
#if defined(ZQ_OS_LINUX)
		long long       vLonglong;
		unsigned long long     vULonglong;
#else
		int64       vLonglong;
		uint64     vULonglong;
#endif
		struct tm*    vTime;
		tstring*	  vString;
		BinaryData*   vBinary;
		ValueArray*   vArray;
		ValueStruct*  vStruct;
	} _value;
	
};


/// -----------------------------
/// class UnserializeException
/// -----------------------------
class UnserializeException : public Exception
{
public:
	UnserializeException(const std::string &what_arg) throw();
	virtual ~UnserializeException() throw();
};

/// -----------------------------
/// class Unserializer
/// -----------------------------
class Unserializer
{
public:
	Unserializer(Variant& var, tistream& istrm) : _var(var), _istrm(istrm) {}
	virtual ~Unserializer(){}
	
	virtual void unserialize() throw (UnserializeException) =0;
	
protected:
	static void throwExcpt(const tchar* fmt, ...)  throw (UnserializeException) PRINTFLIKE(1, 2);

	void initArray(Variant& var) throw (VariantException);
	void initStruct(Variant& var) throw (VariantException);
	void initBinary(Variant& var, const Variant::BinaryData& bindata) throw (VariantException);

	Variant& _var;
	tistream& _istrm;

	typedef struct node
	{
		int		tag;
		Variant val;
	} node_t;

	typedef std::vector < node_t > stack_t;
	stack_t _stack;
};

/// -----------------------------
/// class Serializer
/// -----------------------------
class Serializer
{
public:
	Serializer(Variant& var, tostream& ostrm) : _var(var), _ostrm(ostrm) {}
	
	virtual ~Serializer(){}
	virtual void serialize();
	
protected:

	Variant& _var;
	tostream& _ostrm;

};

} // namespace common
} // namespace ZQ

#endif//__ZQ_Common_Variant_H__
