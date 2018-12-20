// ===========================================================================
// Copyright (c) 2015 by
// XOR media, Shanghai
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
// Ident : $Id$
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define SNMP variables to exchange with SNMP agent
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/snmp/SnmpVar.cpp $
// 
// 31    2/09/17 4:03p Hui.shao
// 
// 30    12/26/16 2:49p Hui.shao
// 
// 29    11/30/15 4:54p Ketao.zhang
// check in  merge git
// 
// 28    3/27/15 2:51p Build
// 
// 27    3/19/15 6:04p Zhiqiang.niu
// fix set operate
// 
// 26    3/18/15 8:29p Zhiqiang.niu
// modify for linux count64, use Gauge instead of Counter64
// 
// 25    3/18/15 11:50a Zhiqiang.niu
// 
// 24    3/17/15 11:42a Hui.shao
// fixed read/write CStr
// 
// 23    3/16/15 6:07p Zhiqiang.niu
// 
// 22    3/13/15 10:11a Zhiqiang.niu
// 
// 21    3/13/15 9:32a Hui.shao
// 
// 20    3/12/15 5:43p Zhiqiang.niu
// 
// 19    3/11/15 7:38p Zhiqiang.niu
// 
// 18    3/11/15 7:10p Zhiqiang.niu
// 
// 17    3/11/15 5:52p Zhiqiang.niu
// 
// 16    3/10/15 7:04p Zhiqiang.niu
// fix a bug which crashed program
// 
// 15    3/10/15 10:45a Zhiqiang.niu
// 
// 14    3/06/15 2:50p Zhiqiang.niu
// modify for linux
// 
// 13    3/05/15 2:30p Hui.shao
// 
// 12    3/05/15 2:24p Zhiqiang.niu
// 
// 11    3/05/15 12:58p Hui.shao
// 
// 10    3/05/15 12:50p Hui.shao
// make ZQSnmp as dll
// 
// 9     3/04/15 6:10p Hui.shao
// net-snmp impl-1
// 
// 8     3/03/15 2:56p Hui.shao
// take MemRange to serialize/unserialize var
// 
// 7     3/03/15 11:14a Hui.shao
// debugged encodeResp() and decodeReq()
// 
// 6     3/02/15 4:32p Hui.shao
// 
// 5     3/02/15 9:38a Hui.shao
// SNMPObjectByAPI
// 
// 4     2/28/15 6:52p Hui.shao
// 
// 3     2/28/15 5:24p Hui.shao
// draft of definition
// ===========================================================================

#include "ZQSnmp.h"
// #include "ZQSnmpUtil.h"

namespace ZQ {
namespace SNMP {

// -----------------------------
// class Oid
// -----------------------------
std::string Oid::dataToStr(const Oid::Data& data)
{
	std::string ret;
	char buf[16];
	for (size_t i=0; i< data.size(); i++)
	{
		snprintf(buf, sizeof(buf)-2, ".%d", (uint32) data[i]);
		ret += buf;
	}

	return ret;
}

Oid::Data Oid::strToData(const std::string& strOid)
{
	Oid::Data data;
	if (strOid.empty())
		return data;
	std::string str = strOid;
	size_t i=0;
	for (i =0; !str.empty(); i++)
	{
		size_t pos = str.find_first_of("0123456789");
		if (std::string::npos != pos)
			str = str.substr(pos);
		data.push_back(atol(str.c_str()));
		
		pos = str.find_first_of(".,");
		if (std::string::npos != pos)
			str = str.substr(pos+1);
		else break;
	}

	return data;
}

int Oid::compare(const Oid& other, size_t offset, size_t count, size_t otherOff, uint32 otherCount) const
{
    Data::const_iterator beg = (offset < _data.size() ? _data.begin() + offset : _data.end());
    Data::const_iterator end = (offset < _data.size() && count < (_data.size() - offset) ? _data.begin() + offset + count : _data.end());

	Data::const_iterator beg2 = (otherOff < other._data.size() ? other._data.begin() + otherOff : other._data.end());
    Data::const_iterator end2 = (otherOff < other._data.size() && otherCount < (other._data.size() - otherOff) ? other._data.begin() + otherOff + otherCount : other._data.end());

    if (lexicographical_compare(beg, end, beg2, end2))
        return -1;
    else if (lexicographical_compare(beg2, end2, beg, end))
        return 1;
    else
        return 0;
}

Oid& Oid::append(const Oid& other)
{
	for (size_t i =0; i< other._data.size(); i++)
		_data.push_back(other._data[i]);

	return (*this);
}

bool Oid::isDescendant(const Oid& other)
{
	return 0 == compare(other, 0, _data.size(), 0, _data.size());
}


Oid& Oid::append(Oid::I_t subOid)
{
	_data.push_back(subOid);

	return (*this);
}

Oid Oid::operator+(const Oid& other) const
{
    Oid result = *this;
	result.append(other);
	return result;
}

Oid Oid::sub(size_t offset, size_t count) const
{
    if (offset < _data.size() && count != 0)
        return Oid(&_data[offset], MIN(count, (uint32)(_data.size()) - offset));

	return Oid();
}

// -----------------------------
// class SNMPVariable
// -----------------------------
SNMPVariable::SNMPVariable()
{ 
	memset(&_data, 0, sizeof(_data)); 
}

MemRange SNMPVariable::getValueByMemRange() const
{
	return _getValueByMemRange(_data);
}

size_t SNMPVariable::setValueByMemRange(AsnType type, MemRange mr)
{ 
	return _setValueByMemRange(_data, type, mr);
}


size_t SNMPVariable::unserialize(List& vlist, const uint8* stream, size_t maxlen)
{
	const uint8* p = stream;
	size_t n =0;
	vlist.clear();

	do {
		SNMPVariable::Ptr pv = new SNMPVariable();
		if (NULL == pv)
			break;
		n = pv->unserialize(p, stream + maxlen -p);
		p +=n;
		vlist.push_back(pv);

	} while (p < stream+maxlen && n>0);

	return (size_t) (p - stream);
}

size_t SNMPVariable::serialize(List& vlist, uint8* stream, size_t maxlen)
{
	uint8* p = stream;
	size_t n =0;

	for (size_t i =0; i < vlist.size(); i++)
	{
		if (NULL == vlist[i])
			continue;

		n = vlist[i]->serialize(p, stream + maxlen -p);
		if (n >0)
			p+=n;
		else
			break;
	}

	return (size_t) (p - stream);
}

#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
// -----------------------------
// class SNMPVariable for Windows
// -----------------------------
SNMPVariable::SNMPVariable(const Oid& fullOid, VT* value)
: _oid(fullOid)
{ 
	memset(&_data, 0, sizeof(_data));
	SnmpUtilAsnAnyCpy(&_data, value); 
}

void SNMPVariable::resetValue(VT& value) 
{ 
	SnmpUtilAsnAnyFree(&value);
	memset(&value, 0, sizeof(value));
}

void SNMPVariable::clear() 
{ 
	resetValue(_data);
	_oid._data.clear();
}

SNMPVariable::AsnType SNMPVariable::type() const
{
	return _data.asnType; 
}

Oid SNMPVariable::oid() const
{ 
	return _oid; 
}

void SNMPVariable::setOid(const Oid& fullOid)
{
	_oid = fullOid;
}

// from ZQSnmpUtil///////////////////////
static size_t encode(uint8* stream, int maxLen, const uint8* value, const uint32 vlen)
{
	if (NULL == stream || NULL == value || maxLen < (int)(sizeof(uint32) + vlen))
		return 0;

	uint8* p = stream;
	*((uint32*) p) = vlen, p += sizeof(uint32);

	memcpy(p, value, vlen), p +=vlen;

	return (size_t) (p - stream);
}

static size_t decode(const uint8* stream, int maxLen, uint8* value, uint32& vlen)
{
	if (NULL == stream || NULL == value || maxLen < (sizeof(uint32)))
		return 0;

	const uint8* p = stream;
	uint32 slen = *((uint32*) p);

	if (slen > vlen)
		return 0; // not matched or overflow

	vlen = slen;
	p += sizeof(uint32);
	memcpy(value, p, vlen), p+= vlen;

	return (size_t) (p - stream);
}

static size_t encode(uint8* stream, int maxLen, const int32 value)
{
	return encode(stream, maxLen, (uint8*)&value, sizeof(value));
}

static size_t decode(const uint8* stream, int maxLen, int32& value)
{
	uint32 vlen = sizeof(value);
	return decode(stream, maxLen, (uint8*)&value, vlen);
}

static size_t encode(uint8* stream, int maxLen, const uint32 value)
{
	return encode(stream, maxLen, (uint8*)&value, sizeof(value));
}

static size_t decode(const uint8* stream, int maxLen, uint32& value)
{
	uint32 vlen = sizeof(value);
	return decode(stream, maxLen, (uint8*)&value, vlen);
}

static size_t encode(uint8* stream, int maxLen, const uint64 value)
{
	return encode(stream, maxLen, (uint8*)&value, sizeof(value));
}

static size_t decode(const uint8* stream, int maxLen, uint64& value)
{
	uint32 vlen = sizeof(value);
	return decode(stream, maxLen, (uint8*)&value, vlen);
}

static size_t encodeAny0(uint8* stream, int maxLen, const AsnAny& av)
{
    if (NULL == stream || maxLen <=0)
        return 0;

	uint8* p = stream;
	*p++ = av.asnType;

    switch(av.asnType)
    {
    case ASN_NULL:
		p += encode(p, (int)(stream +maxLen -p), (const uint8*) "", 0);
		break;

	case ASN_INTEGER32:
		p += encode(p, (int)(stream +maxLen -p), (int32) av.asnValue.number);
		break;

	case ASN_UNSIGNED32:
    case ASN_COUNTER32:
    case ASN_GAUGE32:
    case ASN_TIMETICKS:
		p += encode(p, (int)(stream +maxLen -p), (uint32) av.asnValue.unsigned32);
		break;

	case ASN_COUNTER64:
		p += encode(p, (int)(stream +maxLen -p), (uint64) av.asnValue.counter64.QuadPart);
		break;

	case ASN_OCTETSTRING:
	case ASN_BITS:
	case ASN_SEQUENCE:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:
		p += encode(p, (int)(stream +maxLen -p), (uint8*) av.asnValue.string.stream, av.asnValue.string.length);
		break;

	case ASN_OBJECTIDENTIFIER:
		p += encode(p, (int)(stream +maxLen -p), (uint8*) av.asnValue.object.ids, (uint32) av.asnValue.object.idLength *sizeof(UINT));
		break;
    };

	return (size_t)(p-stream);
}

static size_t encodeAny(uint8* stream, int maxLen, const AsnAny& av)
{
    if (NULL == stream || maxLen <=0)
        return 0;

	uint8* p = stream;
	*p++ = av.asnType;
	MemRange mr = SNMPVariable::_getValueByMemRange(av);
	if (NULL == mr.first)
		return 0;

	*((uint32*) p) = (uint32)mr.second, p+= sizeof(uint32);
	memcpy(p, mr.first, mr.second), p += mr.second;

	return (size_t)(p-stream);
}

static size_t decodeAny(const uint8* stream, int maxLen, AsnAny& av)
{
    if (NULL == stream || maxLen <=1)
        return 0;

	const uint8* p = stream;
	uint8 type = *p++;
	uint32 vlen = *((uint32*) p); p +=sizeof(uint32);
	if (vlen > (stream + maxLen -p))
		return 0;

	MemRange mr((void*) p, vlen);
	p += SNMPVariable::_setValueByMemRange(av, type, mr);
	return (size_t) (p -stream);
}

//@returns the bytes processed
size_t SNMPVariable::unserialize(const uint8* stream, size_t maxlen)
{
	size_t n =0;
	const uint8* p = stream;
	clear(); 

	if (NULL == p || (stream+maxlen -p) <= sizeof(AsnUnsigned32))
		return 0;

	do {
		// step 1. parse the oid
		// AsnObjectName name;
		// n = ZQAsnUtil::decodeOID(p, &name);
		Oid::I_t oidbuf[ZQSNMP_OID_LEN_MAX];
		uint32 vlen = sizeof(oidbuf);
		n = decode(p, (int)(stream + maxlen -p), (uint8*)oidbuf, vlen);

		if (n < sizeof(AsnUnsigned32))
			break;
		p += n;

		//_oid = Oid(name.ids, name.idLength);
		// SnmpUtilOidFree(&name); //cleanup
		_oid = Oid(oidbuf, vlen /sizeof(Oid::I_t));

		// step.5 read the value
		if ((stream+maxlen -p) < sizeof(AsnUnsigned32))
			break;

		//VT value;
		//n = ZQAsnUtil::decodeAny(p, &value);
		//if (n < sizeof(AsnUnsigned32))
		//	break;
		//p += n;

		//SnmpUtilAsnAnyCpy(&_data, &value);
		//SnmpUtilAsnAnyFree(&value); // clean up
		n = decodeAny(p, int(stream+maxlen -p), _data);
		if (n < sizeof(AsnUnsigned32))
			break;
		p += n;

	} while(0);

	return (size_t) (p - stream);
}

size_t SNMPVariable::serialize(uint8* stream, size_t maxlen)
{
	uint8* p = stream;
	size_t n =0;
	if (NULL == stream || maxlen < sizeof(Oid::I_t)*_oid.length())
		return 0;

	Oid::I_t oidbuf[ZQSNMP_OID_LEN_MAX];
	for (size_t i=0; i < _oid.length(); i++)
		oidbuf[i] = _oid[i];

	// n = encode(p, stream + maxlen -p, (const uint8*)oidbuf, _oid.length() * sizeof(Oid::I_t));
	n = encode(p, int(stream + maxlen -p), (const uint8*)oidbuf, (uint32)(_oid.length() * sizeof(Oid::I_t)));

	if (n <=0)
		return 0;
	p+=n;

	p += encodeAny(p, int(stream + maxlen -p), _data);

	return (size_t) (p-stream);
}

MemRange SNMPVariable::_getValueByMemRange(const SNMPVariable::VT& value)
{
	MemRange mr(NULL, 0);

	switch (value.asnType)
	{
	case AsnType_None:
    case ASN_NULL:
		mr.first  = "";
		mr.second = 0;
		break;

	case ASN_INTEGER32:  // mixed int32 and uint32
	case ASN_UNSIGNED32: // AsnType_Int32:
	case ASN_COUNTER32:
	case ASN_GAUGE32:
	case ASN_TIMETICKS:
		mr.first  = (void*) &value.asnValue.number;
		mr.second = sizeof(uint32);
		break;

	case ASN_COUNTER64: //AsnType_Int64
		mr.first  = (void*) &value.asnValue.number;
		mr.second = sizeof(uint64);
		break;

	case ASN_OCTETSTRING: //AsnType_String
	case ASN_BITS:
	case ASN_SEQUENCE:
	case ASN_IPADDRESS:
	case ASN_OPAQUE:
		mr.first  = (void*) value.asnValue.string.stream;
		mr.second = value.asnValue.string.length;
		break;

	case ASN_OBJECTIDENTIFIER:  // AsnType_Oid:
		mr.first  = (void*) value.asnValue.object.ids;
		mr.second = value.asnValue.object.idLength * sizeof(Oid::I_t);
		break;
	}

	return mr;
}

size_t SNMPVariable::_setValueByMemRange(SNMPVariable::VT& value, AsnType type, MemRange mr)
{
	if (NULL == mr.first)
		return 0;

	resetValue(value);

	switch (type)
	{
	case AsnType_None:
    case ASN_NULL:
		value.asnValue.number = 0;
		value.asnType = ASN_NULL;
		return 0;
	
	// case AsnType_Int32:
	case ASN_INTEGER32: // mixed int32 and uint32
	case ASN_UNSIGNED32: // AsnType_Int32:
	case ASN_COUNTER32:
	case ASN_GAUGE32:
	case ASN_TIMETICKS:
		if (mr.second < sizeof(uint32))
			return 0;

		value.asnValue.number = *((uint32*) mr.first);
		value.asnType = ASN_INTEGER32;
		return sizeof(uint32);
	
	// case AsnType_Int64:
	case ASN_COUNTER64: //AsnType_Int64
		if (mr.second < sizeof(uint64))
			return 0;

		value.asnValue.counter64.QuadPart = *((uint64*) mr.first);
		value.asnType = ASN_COUNTER64;
		return sizeof(uint64);

	// case AsnType_String:
	case ASN_OCTETSTRING: //AsnType_String
	case ASN_BITS:
	case ASN_SEQUENCE:
	case ASN_IPADDRESS:
	case ASN_OPAQUE:
	case AsnType_CStr: // this is a extended type
		value.asnType = ASN_OCTETSTRING;
		value.asnValue.string.length = (UINT) mr.second;
		value.asnValue.string.dynamic = FALSE;

		if (value.asnValue.string.length >0)
		{
			if (NULL != (value.asnValue.string.stream = (BYTE *)SnmpUtilMemAlloc(value.asnValue.string.length)))
			{
				memcpy(value.asnValue.string.stream, mr.first, value.asnValue.string.length);
				value.asnValue.string.dynamic = TRUE;
			}
			else 
            {
                value.asnValue.string.stream = (BYTE *)"";
                value.asnValue.string.length=0;
            }
        }
		else
		{
            value.asnValue.string.stream = (BYTE *)"";
            value.asnValue.string.length=0;
        }

		return (size_t) value.asnValue.string.length;

	// case AsnType_Oid:
	case ASN_OBJECTIDENTIFIER:  // AsnType_Oid:
		value.asnType = ASN_OBJECTIDENTIFIER;
		value.asnValue.object.ids = NULL;
		value.asnValue.object.idLength = (UINT) mr.second / sizeof(Oid::I_t); // ???_varname.length();
		if (value.asnValue.object.idLength >0)
		{
			if (NULL == (value.asnValue.object.ids = (UINT *)SnmpUtilMemAlloc(sizeof(UINT) * value.asnValue.object.idLength)))
				value.asnValue.object.idLength = 0;
			else 
				memcpy(value.asnValue.object.ids, mr.first, sizeof(UINT) * value.asnValue.object.idLength);
		}
		
		return sizeof(UINT) * value.asnValue.object.idLength;
	
	}

	return 0;
}

#else // (SNMP_VENDOR == SNMP_VENDOR_NET_SNMP)
// -----------------------------
// class SNMPVariable for netsnmp
// -----------------------------
//netsnmp SNMPVariable::VT = netsnmp_variable_list
SNMPVariable::SNMPVariable(SNMPVariable::VT* var)
{ 
	memset(&_data, 0, sizeof(_data));
	if (var)
		snmp_clone_var(var, &_data);
}

void SNMPVariable::resetValue(SNMPVariable::VT& value) 
{ 
	snmp_reset_var_buffers(&value);
	//memset(&value, 0, sizeof(value));
}

void SNMPVariable::clear() 
{
	resetValue(_data);
	//memset(&_data, 0, sizeof(_data));
}

SNMPVariable::AsnType SNMPVariable::type() const
{
	return _data.type; 
}

Oid SNMPVariable::oid() const
{
	return Oid(_data.name, _data.name_length);
}

void SNMPVariable::setOid(const Oid& fullOid)
{
    Oid::I_t buf[ZQSNMP_OID_LEN_MAX];
	size_t len = fullOid.length();

	for (size_t i =0; i < len; i++)
		buf[i] = fullOid._data[i];

	snmp_set_var_objid(&_data, buf, len);
}


static size_t decodeAny(uint8* data, size_t datalength,  uint8& type, netsnmp_variable_list& var)
{
	uint8  vbuf[1024]; 
    uint8* p = NULL;
	size_t vlen =0;

	switch(type)
	{
    case ASN_INTEGER:
        vlen = sizeof(long);
        p = asn_parse_int(data, &datalength, &type, (long *) vbuf, vlen);
        break;

    case ASN_COUNTER:
    case ASN_UNSIGNED:
    case ASN_TIMETICKS:
    case ASN_UINTEGER:
		vlen = sizeof(long);
        p = asn_parse_unsigned_int(data, &datalength, &type, (u_long *)vbuf, vlen);
        break;

	case ASN_COUNTER64:
		vlen = sizeof(struct counter64);
        p = asn_parse_unsigned_int64(data, &datalength, &type, (struct counter64 *) vbuf, sizeof(struct counter64));
        break;

    case ASN_OCTET_STR:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:
    case ASN_NSAP:
        vlen = sizeof(vbuf);
        p = asn_parse_string(data, &datalength, &type, vbuf, &vlen);
        break;

    case ASN_OBJECT_ID:
        vlen = sizeof(vbuf) / sizeof(oid);
        p = asn_parse_objid(data, &datalength, &type, (oid*)vbuf, &vlen);
        vlen *= sizeof(oid);
        break;

    case ASN_NULL:
		vlen = 0;
        p = asn_parse_null(data, &datalength, &type);
        break;

    case ASN_BIT_STR:
        vlen = sizeof(vbuf);
        p = asn_parse_bitstring(data, &datalength, &type, vbuf, &vlen);
        break;
    }

    if (NULL == p)
		return 0;
	
	if (0 != snmp_set_var_typed_value(&var, type, vbuf, vlen))
		return 0;

	var.next_variable = NULL;
	return (size_t) (p-data);
}


//@returns the bytes processed
size_t SNMPVariable::unserialize(const uint8* stream, size_t maxlen)
{
	size_t n =0;
	const u_char* p = stream;
	clear(); 

	if (NULL == p || (stream+maxlen -p) <= sizeof(unsigned long))
		return 0;

	do {
		// step 1. parse the oid
		Oid::I_t oidbuf[ZQSNMP_OID_LEN_MAX];
		size_t oidlen = 0, vallen = 0;
		uint8 type = 0, *val = NULL;

		//u_char * snmp_parse_var_op(
		//u_char *data              IN - pointer to the start of object
		//oid *var_name             OUT - object id of variable 
		//int *var_name_len         IN/OUT - length of variable name 
		//u_char *var_val_type      OUT - type of variable (int or octet string) (one byte) 
		//int *var_val_len          OUT - length of variable 
		//u_char **var_val          OUT - pointer to ASN1 encoded value of variable 
		//int *listlength           IN/OUT - number of valid bytes left in var_op_list 
		// p = snmp_parse_var_op(p, tmpoid, &oidlen, &type, &vallen, &val, &maxlen);
		p = snmp_parse_var_op((u_char*)p, oidbuf, &oidlen, &type, &vallen, &val, &maxlen);

		if (NULL == p || (oidlen + vallen) <= 0)
			return 0;

        size_t var_len = p - val;
        if(decodeAny(val, var_len, type, _data)) {
            if(snmp_set_var_objid(&_data, oidbuf, oidlen)) {
                snmp_set_var_typed_value(&_data, ASN_NULL, NULL, 0); // clear data
            }
        }
        
/*
		_oid = Oid(_data.name, _data.name_length /sizeof(Oid::I_t));

		// step.2 read the value
		if ((stream+maxlen -p) < sizeof(AsnUnsigned32))
			break;

		if (decodeAny(p, stream+maxlen -p, _data) <=0)
			break;
*/

	} while(0);

	return (size_t) (p - stream);
}

size_t SNMPVariable::serialize(uint8* stream, size_t maxlen)
{
	size_t len = maxlen;

	// u_char * snmp_build_var_op(
	// u_char *data         IN - pointer to the beginning of the output buffer
	// oid *var_name        IN - object id of variable 
	// int *var_name_len    IN - length of object id 
	// u_char var_val_type  IN - type of variable 
	// int    var_val_len   IN - length of variable 
	// u_char *var_val      IN - value of variable 
	// int *listlength      IN/OUT - number of valid bytes left in output buffer 
    if (snmp_build_var_op((u_char*)stream, _data.name, &_data.name_length, _data.type, _data.val_len, _data.val.string, &len))
        return maxlen - len;

	return 0;
}

MemRange SNMPVariable::_getValueByMemRange(const SNMPVariable::VT& value)
{
    MemRange mr(NULL, 0);

    switch (value.type)
    {
    case ASN_NULL:
        mr.first = (u_char*)"";
        mr.second = 0;
        break;
    case ASN_BOOLEAN:
    case ASN_UNSIGNED:
    case ASN_INTEGER:
        mr.first = value.val.integer;
        mr.second = value.val_len;
        break;

    case ASN_BIT_STR:
        mr.first = value.val.bitstring;
        mr.second = value.val_len;
        break;
    case ASN_OCTET_STR:
        mr.first = value.val.string;
        mr.second = value.val_len;
        break;
    case ASN_OBJECT_ID:
        mr.first = value.val.objid;
        mr.second = value.val_len;
        break;
    case ASN_SEQUENCE:
    case ASN_SET:
        mr.first = (u_char*)value.buf;
        mr.second = value.val_len;
        break;
    }

    return mr;
}

size_t SNMPVariable::_setValueByMemRange(SNMPVariable::VT& value, AsnType type, MemRange mr)
{
    if (NULL == mr.first)
        return 0;

    resetValue(value);

    switch (type)
    {
    case ASN_NULL:
        if (0 != snmp_set_var_typed_value(&value, ASN_NULL, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;
    case ASN_BOOLEAN:
        if (0 != snmp_set_var_typed_value(&value, ASN_BOOLEAN, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;
    case ASN_INTEGER:
        if (0 != snmp_set_var_typed_value(&value, ASN_INTEGER, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;
    case ASN_UNSIGNED:
        if (0 != snmp_set_var_typed_value(&value, ASN_UNSIGNED, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;

    case ASN_BIT_STR:
        if (0 != snmp_set_var_typed_value(&value, ASN_BIT_STR, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;

    case ASN_OCTET_STR:
        if (0 != snmp_set_var_typed_value(&value, ASN_OCTET_STR, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;

    case ASN_OBJECT_ID:
        if (0 != snmp_set_var_typed_value(&value, ASN_OBJECT_ID, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;
    case ASN_SEQUENCE:
        if (0 != snmp_set_var_typed_value(&value, ASN_SEQUENCE, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;
    case ASN_SET:
        if (0 != snmp_set_var_typed_value(&value, ASN_SET, (u_char*)mr.first, mr.second))
        {
            // set value failed
        }
        break;
    default:
        return 0;
    }
    return value.val_len;
}

#endif // SNMP_VENDOR

}} // namespace ZQ::Snmp
