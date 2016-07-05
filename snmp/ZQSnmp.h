// ===========================================================================
// Copyright (c) 2015 by
// XOR media, Shanghai, PRC.,
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
// Ident : $Id: ZQSnmp.h,v 1.8 2014/05/26 09:32:35 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define SNMP exports
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/snmp/ZQSnmp.h $
// 
// 51    3/26/15 7:12p Hui.shao
// DEFAULT_LOG_FLAGS for loggingMask
// 
// 50    3/26/15 6:00p Hui.shao
// SubAgent to replace Subagent
// 
// 48    3/26/15 10:14a Hui.shao
// log by flags
// 
// 47    3/25/15 6:59p Hui.shao
// 
// 46    3/19/15 6:36p Zhiqiang.niu
// 
// 45    3/19/15 2:16p Hui.shao
// changed ext of ASN_TYPE flags to 0xff00
// 
// 44    3/19/15 12:29p Hui.shao
// varname with typestr and r/w
// 
// 43    3/19/15 9:46a Hui.shao
// 
// 42    3/18/15 8:29p Zhiqiang.niu
// modify for linux count64, use Gauge instead of Counter64
// 
// 41    3/17/15 5:22p Zhiqiang.niu
// 
// 40    3/17/15 12:03p Hui.shao
// 
// 39    3/17/15 11:15a Hui.shao
// fixed nextVar(NilOid) query
// 
// 38    3/17/15 10:24a Hui.shao
// added AsnType_CStr to access NULL-terminated string/buf
// 
// 37    3/16/15 6:07p Zhiqiang.niu
// 
// 36    3/13/15 5:50p Build
// 
// 35    3/13/15 11:14a Hui.shao
// added serviceTypeName indexing
// 
// 34    3/12/15 7:31p Hui.shao
// include the entire mib2ctable into the dll
// 
// 33    3/12/15 5:43p Zhiqiang.niu
// 
// 32    3/12/15 2:56p Build
// 
// 31    3/12/15 12:54p Hui.shao
// 
// 30    3/12/15 11:00a Zhiqiang.niu
// 
// 29    3/12/15 10:16a Hui.shao
// class SnmpAgent
// 
// 27    3/11/15 5:59p Hui.shao
// 
// 26    3/11/15 2:59p Hui.shao
// 
// 25    3/11/15 2:22p Hui.shao
// SNMPObjectDupValue
// 
// 24    3/10/15 7:03p Zhiqiang.niu
// anonymous objects are "const" at linux, which couldn't pass to
// non-const function argument
// 
// 23    3/10/15 3:38p Hui.shao
// mib2ctbl
// 
// 21    3/09/15 2:25p Zhiqiang.niu
// 
// 20    3/09/15 10:18a Hui.shao
// BaseAgent
// 
// 19    3/06/15 2:52p Zhiqiang.niu
// modify for linux
// 
// 18    3/06/15 11:38a Hui.shao
// drafted for future messaging between Agent and SubAgent
// 
// 17    3/06/15 10:09a Hui.shao
// added Agent extension for Windows SNMP service
// 
// 16    3/05/15 7:30p Hui.shao
// 
// 15    3/05/15 5:54p Hui.shao
// 
// 14    3/05/15 12:50p Hui.shao
// make ZQSnmp as dll
// 
// 13    3/04/15 6:10p Hui.shao
// net-snmp impl-1
// 
// 12    3/04/15 4:54p Zhiqiang.niu
// 
// 11    3/03/15 2:56p Hui.shao
// take MemRange to serialize/unserialize var
// 
// 10    3/03/15 11:14a Hui.shao
// debugged encodeResp() and decodeReq()
// 
// 9     3/02/15 4:32p Hui.shao
// 
// 8     3/02/15 10:23a Hui.shao
// renamed ModuleMIB to ComponentMID
// 
// 6     3/02/15 9:38a Hui.shao
// SNMPObjectByAPI
// 
// 5     2/28/15 6:52p Hui.shao
// 
// 4     2/28/15 6:04p Hui.shao
// 
// 3     2/28/15 5:24p Hui.shao
// draft of definition
// ===========================================================================

#ifndef __ZQ_COMMON_SNMP_H__
#define __ZQ_COMMON_SNMP_H__

#include "ZQ_common_conf.h"
#include "Pointer.h"
#include "FileLog.h"
#include "Locks.h"
#include "NativeThread.h"
#include "UDPSocket.h"
#include "LRUMap.h"
#include "TimeUtil.h"

#include <string>
#include <vector>
#include <map>

#define SNMP_VENDOR_NET_SNMP    (1)
#define SNMP_VENDOR_Microsoft   (2)

#ifdef ZQ_OS_MSWIN
#  define SNMP_VENDOR   SNMP_VENDOR_Microsoft
#  ifdef ZQSNMP_EXPORTS
#    define ZQ_SNMP_API __EXPORT
#  else
#    define ZQ_SNMP_API __DLLRTL
#  endif
#else
#  define SNMP_VENDOR   SNMP_VENDOR_NET_SNMP
#  define ZQ_SNMP_API
#endif // OS

#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
#   include <snmp.h>
#   pragma comment(lib,"Snmpapi.lib")
#elif (SNMP_VENDOR == SNMP_VENDOR_NET_SNMP)
#   include <net-snmp/net-snmp-config.h>
#   include <net-snmp/net-snmp-includes.h>
#   include <net-snmp/agent/net-snmp-agent-includes.h>
#else
#   error Unsupported SNMP vendor
#endif // OS

#define DEFAULT_BASE_UDP_PORT  (10000)
#define DEFAULT_COMM_TIMEOUT    (2000) // 2sec

#define ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY       (9)
#define ZQSNMP_OID_OFFSET_COMPONENT_ID          (ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY +0)
#define ZQSNMP_OID_OFFSET_MODULE_ID             (ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY +1) // values of ModuleMIB::ModuleOid_e

#ifndef FLAG
#  define FLAG(_F)    (1<<_F)
#endif

// DEFAULT_LOG_FLAGS for loggingMask
#ifdef _DEBUG
#  define DEFAULT_LOG_FLAGS    (((ModuleMIB::VFLG_VERBOSE_AGENT | ModuleMIB::VFLG_VERBOSE_MIB) <<8) | ZQ::common::Log::L_DEBUG)
#else
#  define DEFAULT_LOG_FLAGS (ZQ::common::Log::L_WARNING)
#endif // DEBUG


namespace ZQ {
namespace SNMP {

class ZQ_SNMP_API Oid; // the abstraction of Windows/netsnmp SNMP variable definition
class ZQ_SNMP_API SNMPVariable; // the abstraction of Windows/netsnmp SNMP variable definition

class ZQ_SNMP_API SNMPObject;   // the wrapper of local address that will be exposed
class ZQ_SNMP_API ModuleMIB;    // the MIB collection of this module
class ZQ_SNMP_API Subagent;     // THIS CLASS SHOULD BE CLEANED
class ZQ_SNMP_API SubAgent;     // the communicator with SNMP agent

#ifdef _DEBUG
class ZQ_SNMP_API BaseAgent;
class ZQ_SNMP_API SnmpAgent;
#endif // _DEBUG

// ---------------------------------------
// Constants per SNMP_VENDOR:
// PDU type, SNMP errors
// ---------------------------------------
#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
#  define ZQSNMP_PDU_GET         SNMP_PDU_GET
#  define ZQSNMP_PDU_GETNEXT     SNMP_PDU_GETNEXT
#  define ZQSNMP_PDU_SET         SNMP_PDU_SET
#  define ZQSNMP_PDU_UNKNOWN     0xff

typedef enum _SNMPError
{
	// v1 status
	se_NoError      = SNMP_ERRORSTATUS_NOERROR,
	se_TooBig       = SNMP_ERRORSTATUS_TOOBIG,
	se_NoSuchName   = SNMP_ERRORSTATUS_NOSUCHNAME,
	se_BadValue     = SNMP_ERRORSTATUS_BADVALUE,
	se_ReadOnly     = SNMP_ERRORSTATUS_READONLY,
	se_GenericError = SNMP_ERRORSTATUS_GENERR
} SNMPError;

#else // (SNMP_VENDOR == SNMP_VENDOR_NET_SNMP)
#  define ZQSNMP_PDU_GET         0x00  // MODE_GET
#  define ZQSNMP_PDU_GETNEXT     0x01  // MODE_GETNEXT
#  define ZQSNMP_PDU_SET         0x02  // MODE_SET_COMMIT
#  define ZQSNMP_PDU_UNKNOWN     0xff

typedef enum _SNMPError
{
	// v1 status
	se_NoError,
	se_TooBig,
	se_NoSuchName,
	se_BadValue,
	se_ReadOnly,
	se_GenericError
} SNMPError;

#endif // SNMP_VENDOR

//////////////////////////////////////////////////////////////////////////
//message
#define ZQSNMP_MSG_LEN_MAX 1024
#define ZQSNMP_OID_LEN_MAX (20)

// -----------------------------
// class Oid
// -----------------------------
class Oid
{
	friend class SNMPVariable;

public:

#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
	typedef uint32 I_t;
#else // (SNMP_VENDOR == SNMP_VENDOR_NET_SNMP)
	typedef oid I_t;
#endif // SNMP_VENDOR

    typedef std::vector< I_t > Data;
    
	Oid() {}
	Oid(const Oid &other) // copier
	{
		if (this == &other)
			return;

		_data = other._data;
	}

    Oid(const std::string& v) { _data = strToData(v); }
    Oid(const I_t* data, size_t length) { _data.assign(data, data + length); }

    bool isNil() const { return (length()<=0); }
	size_t length() const { return _data.size(); }
	I_t& operator [] (size_t idx) { return _data[idx]; }

	Data data() const { return _data; }
	std::string str() const { return dataToStr(_data); };
    Oid& append(const Oid& other);
	Oid& append(Oid::I_t subOid);

	bool isDescendant(const Oid& other);

	static std::string dataToStr(const Data& data);
	static Data strToData(const std::string& strOid);

	int compare(const Oid& other) const { return compare(other, 0, length()); }
    int compare(const Oid& other, size_t offset, size_t count, size_t otherOff =0, uint32 count2 = uint32(-1)) const;
    
	Oid operator+(const Oid& other) const;
	Oid& operator=(const Oid& other)
	{
		if (this != &other)
            _data = other._data;

		return *this;
	}

    bool operator < (const Oid& other) const { return (compare(other) < 0); }

    Oid sub(size_t offset = 0, size_t count = uint32(-1)) const;

protected:
    Data _data;
};

typedef std::pair<void*, size_t> MemRange;

// -----------------------------
// class SNMPVariable
// -----------------------------
// On Windows, this wrappers Oid+AsnAny
// For net-snmp, this wrappers netsnmp_variable_list(next=NULL)
class SNMPVariable : public ZQ::common::SharedObject
{
	friend class SNMPObject;

public:
	typedef ZQ::common::Pointer< SNMPVariable > Ptr;
	typedef std::vector< Ptr > List;

#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
	typedef uint32 AsnType;
	typedef AsnAny VT;

	SNMPVariable(const Oid& fullOid, VT* value);
#else // (SNMP_VENDOR == SNMP_VENDOR_NET_SNMP)
	typedef u_char AsnType;
	typedef netsnmp_variable_list VT;

	SNMPVariable(VT* var);
#endif // SNMP_VENDOR

    SNMPVariable();
    virtual ~SNMPVariable() { clear(); }

	static void resetValue(VT& value);
	void clear();

    AsnType type() const;
    Oid      oid() const;
	VT*     data() { return &_data; }
	
	MemRange getValueByMemRange() const;
	size_t   setValueByMemRange(AsnType type, MemRange mr);

	void setOid(const Oid& fullOid);

	static MemRange _getValueByMemRange(const SNMPVariable::VT& value);
	static size_t _setValueByMemRange(SNMPVariable::VT& value, AsnType type, MemRange mr);

public:
	// static SNMPError value2mem(const SNMPVariable& svar, void* addr);
	// static SNMPError mem2value(SNMPVariable& svar, AsnType type, const void* addr, size_t len);

	//@returns the bytes processed
	virtual size_t unserialize(const uint8* stream, size_t maxlen);
	virtual size_t serialize(uint8* stream, size_t maxlen);

	static size_t unserialize(List& vlist, const uint8* stream, size_t maxlen);
	static size_t serialize(List& vlist, uint8* stream, size_t maxlen);

protected:
 //   SNMPVariable(const SNMPVariable& other);
 //   SNMPVariable& operator=(const SNMPVariable&);

#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
    Oid _oid;
#endif // SNMP_VENDOR
	
	VT  _data;
};

// -----------------------------
// AsnType
// -----------------------------
typedef SNMPVariable::AsnType AsnType;

#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
// supported standard ASN.1 type
const AsnType AsnType_None      = 0x00;
const AsnType AsnType_Int32     = ASN_INTEGER32;
const AsnType AsnType_Int64     = ASN_COUNTER64;
const AsnType AsnType_Oid       = ASN_OBJECTIDENTIFIER;
const AsnType AsnType_String    = ASN_OCTETSTRING; // string, displaystring
#else // (SNMP_VENDOR == SNMP_VENDOR_NET_SNMP)
const AsnType AsnType_None      = 0x00;
const AsnType AsnType_Int32     = ASN_INTEGER;
const AsnType AsnType_Int64     = ASN_UNSIGNED;
const AsnType AsnType_Oid       = ASN_OBJECT_ID;
const AsnType AsnType_String    = ASN_OCTET_STR; // string, displaystring
#endif // SNMP_VENDOR

const AsnType AsnType_CStr      = (0x80|AsnType_String); // externed: NULL-terminated C-style string

// -----------------------------
// class SNMPObject
// -----------------------------
// basic wrapper of the simple data type via direct memory access
class SNMPObject : public ZQ::common::SharedObject
{
	friend class ModuleMIB;

public:
	typedef ZQ::common::Pointer< SNMPObject > Ptr;

	typedef enum _FieldId
	{
		vf_Value   =1,
		vf_VarName =2,
		vf_Access  =3
	} FieldId;

	SNMPObject(const std::string& varname, void* vaddr, AsnType vtype, bool readOnly =true)
		: _varname(varname), _vtype(vtype), _readonly(readOnly), _vaddr(vaddr)
	{}

	SNMPObject(const std::string& varname, int32& vInt32, bool readOnly =true)
		: _varname(varname), _vtype(AsnType_Int32), _readonly(readOnly), _vaddr(&vInt32)
	{}

	SNMPObject(const std::string& varname, uint32& vUInt32, bool readOnly =true)
		: _varname(varname), _vtype(AsnType_Int32), _readonly(readOnly), _vaddr(&vUInt32)
	{}

	SNMPObject(const std::string& varname, int64& vInt64, bool readOnly =true)
		: _varname(varname), _vtype(AsnType_Int64), _readonly(readOnly), _vaddr(&vInt64)
	{}

	SNMPObject(const std::string& varname, uint64& vUInt64, bool readOnly =true)
		: _varname(varname), _vtype(AsnType_Int64), _readonly(readOnly), _vaddr(&vUInt64)
	{}

	SNMPObject(const std::string& varname, std::string& vStdStr, bool readOnly =true)
		: _varname(varname), _vtype(AsnType_String), _readonly(readOnly), _vaddr(&vStdStr)
	{}

	SNMPObject(const std::string& varname, char* vCStr, bool readOnly =true)
		: _varname(varname), _vtype(AsnType_CStr), _readonly(readOnly), _vaddr(vCStr)
	{}

	std::string name() const;
	AsnType     type() const { return (NULL == _vaddr) ? AsnType_None:_vtype; }
	static const char* typestr(AsnType at);

	virtual SNMPError write(const SNMPVariable& value);
    virtual SNMPError read(SNMPVariable& value, Oid::I_t field =vf_Value) const;

protected:

	static MemRange getDataMemRange(uint32& i) { return MemRange(&i, sizeof(i)); }
	static MemRange getDataMemRange(uint64& i) { return MemRange(&i, sizeof(i)); }
	static MemRange getDataMemRange(std::string& s) { return MemRange((void*)s.c_str(), s.length()); }
	static MemRange getDataMemRange(char* s)        { return MemRange((void*)s, s?strlen(s):0); }
	static void setDataMemRange(uint32& i, MemRange mr) { i = *((uint32*)mr.first); }
	static void setDataMemRange(uint64& i, MemRange mr) { i = *((uint32*)mr.first); }
	static void setDataMemRange(std::string& s, MemRange mr) { s.assign((char*)mr.first, mr.second); }
    static void setDataMemRange(char* s, MemRange mr) { strncpy(s, (char*)mr.first, MIN(mr.second, strlen((char*)mr.first))); }

protected:
	std::string _varname;
	AsnType     _vtype;
	bool        _readonly;

private:
	void*       _vaddr;
};

// -----------------------------
// class SNMPObjectByAPI
// -----------------------------
// object to access value via API invocation
template <class OBJT, typename BaseT >
class SNMPObjectByAPI : public SNMPObject
{
public:
	typedef OBJT ObjectType;
	typedef BaseT (OBJT::*MethodGet)();
	typedef void (OBJT::*MethodSet)(const BaseT&);

	SNMPObjectByAPI(const std::string& varname, ObjectType& obj, AsnType vtype, MethodGet methodGet, MethodSet methodSet=NULL)
		: SNMPObject(varname, NULL, vtype, methodSet?false:true), 
		_methodGet(methodGet), _methodSet(methodSet), _obj(obj)
	{
	}

	virtual ~SNMPObjectByAPI() {}

	virtual SNMPError write(const SNMPVariable& value)
	{
		if (NULL == _methodSet)
			return se_ReadOnly;

		if (value.type() != this->_vtype)
			return se_BadValue;
		
		MemRange mr = value.getValueByMemRange();
		if (NULL == mr.first || mr.second <=0)
			return se_BadValue;

		BaseT vv;
		setDataMemRange(vv, mr);
		(_obj.*_methodSet)(vv);
		return se_NoError;
	}

	virtual SNMPError read(SNMPVariable& value, Oid::I_t field =vf_Value) const
	{
		if (vf_Value != field)
			return SNMPObject::read(value, field);

		if (NULL == _methodGet)
			return se_BadValue;

//do NOT validate here
//      if (value.type() != this->_vtype)
//			return se_BadValue;

		BaseT vv = (_obj.*_methodGet)();
		MemRange mr = getDataMemRange(vv);
		value.setValueByMemRange(_vtype, mr);
		return se_NoError;
	}

private:
	MethodGet _methodGet;
	MethodSet _methodSet;
	ObjectType& _obj;
};

// -----------------------------
// class SNMPObjectDupValue
// -----------------------------
// object by duplicate the value instead of accessing memory or invoking API
class SNMPObjectDupValue : public SNMPObject
{
public:
	SNMPObjectDupValue(const std::string& varname, int32 value)
		: SNMPObject(varname, &_vInt32, AsnType_Int32, true), _vInt32(value)
	{
	}

	SNMPObjectDupValue(const std::string& varname, int64 value)
		: SNMPObject(varname, &_vInt64, AsnType_Int64, true), _vInt64(value)
	{
	}

	SNMPObjectDupValue(const std::string& varname, const std::string& value)
		: SNMPObject(varname, &_vString, AsnType_String, true), _vString(value)
	{
	}

	virtual ~SNMPObjectDupValue() {}

protected:
	int32 _vInt32;
	int64 _vInt64;
	std::string _vString;
};

// ----------------------
// ModuleMIB
// ----------------------
class ModuleMIB
{
public:
	// about the verbose logging
	typedef enum {
		VFLG_VERBOSE_MIB         =FLAG(0),
		VFLG_VERBOSE_AGENT       =FLAG(1),
		VFLG_MUTE_ERRS_MIB       =FLAG(2),
		VFLG_MUTE_ERRS_AGENT     =FLAG(3),
	} VFLG_e;

	static uint8 _flags_VERBOSE; // default 0

	// about the Module OID definition to fill fullOid[ZQSNMP_OID_OFFSET_MODULE_ID]
	typedef enum {
		ModuleOid_SvcShell       =1,
		ModuleOid_Application    =2,
		ModuleOid_AppPluginStart,
	} ModuleOid_e;

	// used for the index converted from MIB file
	typedef struct _MIBE
	{
		char* strSubOid;
		char* varname;
		// Oid::I_t subOid;
	} MIBE;

	typedef struct _ServiceOidIdx
	{ 
		uint32 oid;
		const char* svcTypeName;
		const ZQ::SNMP::ModuleMIB::MIBE* mibsvc; 
	} ServiceOidIdx;

public:
	ModuleMIB(ZQ::common::Log& log, Oid::I_t componentTypeId, Oid::I_t moduleId=ModuleOid_Application, uint32 componetInstId=0);
	virtual ~ModuleMIB();

	// static void setComponentMIBIndices(const MIBE* mibe);
	static Oid productRootOid();
	static Oid::I_t oidOfServiceType(const char* serviceTypeName);

	Oid moduleOid() const { return _moduleOid; }
	Oid::I_t componentId() const;
	Oid::I_t moduleId() const;

	//@return Nil Oid if not a descendant
	bool chopOid(const Oid& oidFull, Oid& subOid, Oid::I_t& fieldId);
	Oid buildupOid(const Oid& subOid, Oid::I_t fid = SNMPObject::vf_Value) const;

	//@param[in] subOid the subOid of the object, NIL to be up to the _mibe table
	virtual bool addObject(SNMPObject::Ptr obj, const Oid& subOid = Oid());
	virtual SNMPObject::Ptr getObject(const Oid& subOid);

	bool addObject(SNMPObject::Ptr obj, const std::string& strSubOid) { return addObject(obj, Oid(strSubOid)); }
	SNMPObject::Ptr getObject(const std::string& strSubOid) { return getObject(Oid(strSubOid)); }

	//@param[in, out] subOidTable the subOid of the table, input NIL to search _mibe table 
	virtual bool reserveTable(const std::string& tblname, size_t columns, Oid& subOidTable);
	virtual bool addTableCell(const Oid& subOidTable, Oid::I_t columnId, Oid::I_t rowIndex, SNMPObject::Ptr var);

	//@removes the entire subtree
	virtual bool removeSubtree(const Oid& subOid);

	// lookup the subOid
	Oid nextOid(const Oid& subOidFrom);
	Oid firstOid();

	// access by SNMPVarList
	SNMPError readVars(SNMPVariable::List& vlist);
	SNMPError nextVar(SNMPVariable::List& vlist);
	SNMPError writeVars(SNMPVariable::List& vlist);

protected:
	ZQ::common::Log& _log;

	typedef std::map< Oid, SNMPObject::Ptr> ChildrenMap; // map of Oid to SNMPObject::Ptr
    ZQ::common::Mutex _lock;
	ChildrenMap _childrenMap;

	Oid _moduleOid;

protected:
	ChildrenMap::iterator _locateForOid(Oid subOid, bool& bfound);

private:
	static const ServiceOidIdx* gMibSvcs;
	const ZQ::SNMP::ModuleMIB::MIBE* _mibe; 
};

// -----------------------------
// class BaseAgent
// -----------------------------
// BaseAgent about the communication between ext of SNMP service and per-app SubAgent
class BaseAgent : public ZQ::common::NativeThread
{
public:

	BaseAgent(ZQ::common::Log& log)
		: _log(log), _bindAddr("127.0.0.1"), _bindPort(0), 
		_timeout(DEFAULT_COMM_TIMEOUT), _baseUdpPort(DEFAULT_BASE_UDP_PORT),
		_bQuit(false)
	{}

	virtual ~BaseAgent() {}

	uint lastCSeq();

	void stop();

	uint32 portOfModule(uint32 componentId, uint32 moduleId);

	static uint8 checksum(const uint8* data, int len);
	static bool readUdpConfigure(ZQ::common::Log& log, uint32& baseUdpPort, uint32& timeoutComm);

	typedef struct _Msgheader 
	{
		uint32  version;
		uint32  cseq;
		uint8   pdu;
		uint8   checksum;
		uint32  error;
		uint32  payloadLen;
	} Msgheader;

	size_t encodeMessage(uint8* stream, size_t maxlen, Msgheader& header, SNMPVariable::List& vlist);
	// size_t encodeMessage(uint8* stream, size_t maxlen, uint8 pduType, SNMPVariable::List& vlist, int cSeq =0);
	bool   decodeMessage(const uint8* stream, int len, Msgheader& header, SNMPVariable::List& vlist);

protected: // impl of NativeThread
	virtual bool init(void);
	virtual int  run();
	virtual void final(void) {}

protected:
	// prototype on message processing
	//@param msgbuf IN/OUT the message buffer that contains the incoming message, also can be take to fill out-going message
	//@param nbyteIn IN the byte-len of the incoming message
	//@param maxlen  the maximal byte-len of the msgbuf that can be filled for the out-going message
	//@return the yte-len of the out-going message that has been filled into msgbuf
	virtual size_t processMessage(const ZQ::common::InetHostAddress& peerAddr, uint32 peerPort, uint8* msgbuf, int nbyteIn, int maxlen) =0;

protected:
	ZQ::common::Log& _log;
	ZQ::common::UDPSocket _soUdp;


	std::string _bindAddr;
	uint32      _bindPort;

	uint32      _timeout;
	uint32      _baseUdpPort;

	ZQ::common::AtomicInt  _lastCSeq;     
	bool       _bQuit;
};

// -----------------------------
// class SnmpAgent
// -----------------------------
// this is a plugin to extend SNMP service
class SnmpAgent : public BaseAgent
{
public:
	SnmpAgent(ZQ::common::Log& log);
	virtual ~SnmpAgent();

protected: // impl of BaseAgent
	virtual size_t processMessage(const ZQ::common::InetHostAddress& peerAddr, uint32 peerPort, uint8* msgbuf, int nbyteIn, int maxlen);

	std::string _logDir;
	uint32      _loggingMask;

protected:
	typedef struct _Query
	{
		BaseAgent::Msgheader header;
		int64 stampRequested, stampResponsed;
		ZQ::common::Event::Ptr pEvent;
		SNMPVariable::List vlist;
	} Query;

	typedef ZQ::common::LRUMap< uint32, Query > AwaitMap;
	AwaitMap _awaitMap;
	ZQ::common::Mutex _awaitLock;

	void OnQueryResult(const ZQ::common::InetHostAddress& serverAddr, int serverPort, BaseAgent::Msgheader header, const SNMPVariable::List& vlist);
	
	//@ return cSeq
	uint32 sendQuery(const ZQ::common::InetHostAddress& serverAddr, int serverPort, uint8 pdu, SNMPVariable::List& vlist, ZQ::common::Event::Ptr eventArrived=NULL);
	bool getResponse(uint32 cSeq, Query& query);

protected:
	typedef struct _ModuleInfo
	{
		uint id;
		std::string name;
		_ModuleInfo(uint mid, std::string mname) : id(mid), name(mname) {}
		bool operator < (const struct _ModuleInfo& o)const { return id < o.id; };
	} ModuleInfo;

	typedef std::vector< ModuleInfo > ModuleList;

	typedef struct _ServiceInfo
	{
		uint id;
		std::string name;
		ModuleList subModules;
		_ServiceInfo(uint sid, std::string sname) : id(sid), name(sname) {}
		bool operator < (const struct _ServiceInfo& o)const { return id < o.id; };
	} ServiceInfo;

	typedef std::vector< ServiceInfo > ServiceInfoList;
	ServiceInfoList _serviceList;

	bool nextModule(uint componentOid, uint& moduleOid) const;

};

// !!!!!!!!!!!!!!!!!!!   THIS CLASS SHOULD BE CLEANED !!!!!!!!!!!!!!!!!!!!
// -----------------------------
// class Subagent
// -----------------------------
// Subagent on the application/service-side that process the message communication with SnmpAgent
//#pragma message ( __MSGLOC__ "TODO: should inherit from BaseAgent when the communication is replaced")
class Subagent: public ZQ::common::NativeThread
{
public:

	Subagent(ZQ::common::Log& log, ModuleMIB& mmib, timeout_t timeout)
		: _log(log), _mmib(mmib), _timeout(timeout), _bQuit(false)
	{}

	virtual ~Subagent() { stop(); }

	ModuleMIB& mmib() { return _mmib; }

	void stop();

protected: // impl of NativeThread
	virtual int run();
	virtual void final(void);

public:
	bool refreshBasePort(void); // TODO be removed

	typedef struct _Msgheader // TODO should be gotten rid of
	{
		unsigned long  _serviceSendSeq;
		unsigned long  _agentRecvSeq;
		unsigned long  _lastLossCount; //last time: _lastLossCount = _serviceSendSeq - _agentRecvSeq
	} Msgheader;

	virtual bool processMessage(const uint8* request, int len, std::string& responseMsg); // TODO be removed

//pragma message ( __MSGLOC__ "TODO: should call BaseAgent's encode/decodeMessage()")
	static bool decodeMessage(const uint8* request, int len, SNMPVariable::List& vlist, uint8& pduType, uint32& err);
	static size_t encodeMessage(uint8* stream, size_t maxlen, uint8 pduType, uint32 err, SNMPVariable::List& vlist);

private:
	ZQ::common::Log& _log; // TODO be removed

	ModuleMIB& _mmib;
	uint32     _snmpUdpBasePort; // TODO be removed
	uint32     _timeout; // TODO be removed
	bool       _bQuit;
};
// !!!!!!!!!!!!!!!!!!!   THIS CLASS SHOULD BE CLEANED !!!!!!!!!!!!!!!!!!!!

// -----------------------------
// class SubAgent
// -----------------------------
// SubAgent instanced in the application/service-side that process the message communication with SnmpAgent
class SubAgent: public BaseAgent
{
public:
	SubAgent(ZQ::common::Log& log, ModuleMIB& mmib, timeout_t timeout);
	virtual ~SubAgent() { stop(); }

	ModuleMIB& mmib() { return _mmib; }

protected: // impl of BaseAgent 
	// prototype on message processing
	//@param msgbuf IN/OUT the message buffer that contains the incoming message, also can be take to fill out-going message
	//@param nbyteIn IN the byte-len of the incoming message
	//@param maxlen  the maximal byte-len of the msgbuf that can be filled for the out-going message
	//@return the yte-len of the out-going message that has been filled into msgbuf
	virtual size_t processMessage(const ZQ::common::InetHostAddress& peerAddr, uint32 peerPort, uint8* msgbuf, int nbyteIn, int maxlen);

protected:
	ModuleMIB& _mmib;
};

}} // namespace ZQ::SNMP

#ifndef MAPSET
#  define MAPSET(_MAPTYPE, _MAP, _KEY, _VAL) if (_MAP.end() ==_MAP.find(_KEY)) _MAP.insert(_MAPTYPE::value_type(_KEY, _VAL)); else _MAP[_KEY] = _VAL
#endif // MAPSET

#endif // __ZQ_COMMON_SNMP_H__
