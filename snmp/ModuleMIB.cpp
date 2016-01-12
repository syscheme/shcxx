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
// Ident : $Id$
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define module wide SNMP exports
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/snmp/ModuleMIB.cpp $
// 
// 36    3/26/15 10:14a Hui.shao
// log by flags
// 
// 35    3/25/15 2:57p Zhiqiang.niu
// 
// 34    3/24/15 5:05p Zhiqiang.niu
// 
// 33    3/24/15 10:03a Hui.shao
// 
// 32    3/19/15 7:01p Zhiqiang.niu
// 
// 31    3/19/15 6:36p Zhiqiang.niu
// 
// 30    3/19/15 6:04p Zhiqiang.niu
// fix set operate
// 
// 29    3/19/15 2:16p Hui.shao
// changed ext of ASN_TYPE flags to 0xff00
// 
// 28    3/19/15 12:29p Hui.shao
// varname with typestr and r/w
// 
// 27    3/18/15 2:32p Build
// 
// 26    3/17/15 5:22p Zhiqiang.niu
// 
// 25    3/17/15 11:42a Hui.shao
// fixed read/write CStr
// 
// 24    3/17/15 11:15a Hui.shao
// fixed nextVar(NilOid) query
// 
// 23    3/17/15 10:24a Hui.shao
// added AsnType_CStr to access NULL-terminated string/buf
// 
// 22    3/16/15 7:04p Hui.shao
// 
// 21    3/16/15 7:00p Hui.shao
// take the first child if GETNEXT with NIL child
// 
// 20    3/13/15 11:14a Hui.shao
// added serviceTypeName indexing
// 
// 19    3/12/15 7:31p Hui.shao
// include the entire mib2ctable into the dll
// 
// 18    3/12/15 12:54p Hui.shao
// 
// 17    3/11/15 3:39p Hui.shao
// log MIB read/write
// 
// 16    3/11/15 2:59p Hui.shao
// 
// 15    3/11/15 2:22p Hui.shao
// SNMPObjectDupValue
// 
// 14    3/10/15 7:03p Zhiqiang.niu
// anonymous objects are "const" at linux, which couldn't pass to
// non-const function argument
// 
// 13    3/10/15 3:38p Hui.shao
// mib2ctbl
// 
// 12    3/09/15 10:18a Hui.shao
// BaseAgent
// 
// 11    3/05/15 5:56p Zhiqiang.niu
// 
// 10    3/05/15 5:54p Hui.shao
// 
// 9     3/05/15 12:50p Hui.shao
// make ZQSnmp as dll
// 
// 8     3/03/15 11:14a Hui.shao
// debugged encodeResp() and decodeReq()
// 
// 7     3/02/15 4:32p Hui.shao
// 
// 6     3/02/15 10:23a Hui.shao
// renamed ModuleMIB to ComponentMID
// 
// 5     3/02/15 9:38a Hui.shao
// SNMPObjectByAPI
// 
// 4     2/28/15 6:52p Hui.shao
// 
// 3     2/28/15 6:04p Hui.shao
// 
// 2     2/28/15 5:24p Hui.shao
// draft of definition
// ===========================================================================

#include "ZQSnmp.h"

#include "mib2ctbl.cpp"

namespace ZQ {
namespace SNMP {

//////////////////////////////////////////////////////////////////////////
//ZQ service's MIB definition
//          1.3.6.1.4.1(enterprise oid prefix)
//                ____|__
//             ...  |   ...
//                22839(ZQ Interactive)
//          ________|____
//         ...  |       ...(future extension)
//              4 (TianShanComponents)
//             _|__________
//             |       |
//             1       ...
//  (internal service branch)
//        ______|_______
//      ...       |
//           (service id)
//    ____________|______________
//   |                  |       ...(future extension)
//   1(shell process)   2(application process)
//   |                  |
//(variable table)   (variable table)


//////////////////////////////////////////////////////////////////////////
//variable table definition

//
//                         ...(service process)
//      ____________________|_________
//     |           |          |      ...(future extension)
//     1(value)    2(name)    3(access)
//    ...        __|____     ...
//              |  |  | ...
//(instance id) 1  2  3


//////////////////////////////////////////////////////////////////////////
//conceptual table illustration
//    (column)     value       name       access
//                  (1)        (2)         (3)
//(row)            ____________________________
//instance id: 1 |item1.1     item2.1     item3.1
//             2 |item1.2     item2.2     item3.2
//             3 |...         ...         ...
//            ...

//////////////////////////////////////////////////////////////////////////

#define ZQSNMP_OID_OFFSET_VAR_FIELD_ID          (ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY +2) // values of SNMPObject::FieldId

#define ZQSNMP_OID_MODULE_ID_SVCSHELL     (1)
#define ZQSNMP_OID_MODULE_ID_SVCAPP       (2)
// plugins of the application should take 3+

// about the Region definition
#define ZQSNMP_OID_REGION_ID_VAR_VALUE    (1)
#define ZQSNMP_OID_REGION_ID_VAR_DESC     (2)

#define ZQSNMP_OID_OFFSET_VAR_DESC        (ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY +2)
#define ZQSNMP_OID_OFFSET_VAR_VAL         (ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY +3)

static const Oid::I_t OID_TIANSHAN_SVCFAMILY[] = {1,3,6,1,4,1,22839,4,1};

// ----------------------
// ModuleMIB
// ----------------------
const ZQ::SNMP::ModuleMIB::ServiceOidIdx* ZQ::SNMP::ModuleMIB::gMibSvcs = _gMibSvcs; // to export
uint8 ModuleMIB::_flags_VERBOSE = 0xFFFF; // default 0

//const ModuleMIB::MIBE* ModuleMIB::_mibe = NULL;
//void ModuleMIB::setComponentMIBIndices(const MIBE* mibe)
//{ _mibe = mibe; }

Oid ModuleMIB::productRootOid()
{
	static Oid rootOid(OID_TIANSHAN_SVCFAMILY, ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY);
	return rootOid;
}

Oid::I_t ModuleMIB::oidOfServiceType(const char* serviceTypeName)
{
	for (size_t i=0; NULL != gMibSvcs[i].mibsvc; i++)
	{
		if (0 == stricmp(gMibSvcs[i].svcTypeName, serviceTypeName))
			return uint32(gMibSvcs[i].oid /100) *100;
	}

	return 0;
}


ModuleMIB::ModuleMIB(ZQ::common::Log& log, Oid::I_t componentTypeId, Oid::I_t moduleId, uint32 componentInstId)
: _log(log), _moduleOid(OID_TIANSHAN_SVCFAMILY, ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY),
_mibe(NULL)
{
	Oid::I_t componentId = uint32(componentTypeId/100) *100;
	// locate _mibe by component typeId
	for (size_t i=0; NULL != gMibSvcs[i].mibsvc; i++)
	{
		if (gMibSvcs[i].oid == componentId)
		{
			_mibe = gMibSvcs[i].mibsvc;
			break;
		}
	}
	
	componentId += componentInstId*10;

	_moduleOid.append(componentId), _moduleOid.append(moduleId);
	if (_flags_VERBOSE & VFLG_VERBOSE_MIB)
		log(ZQ::common::Log::L_DEBUG, CLOGFMT(ModuleMIB, "instantize ModuleMIB[%s]"), _moduleOid.str().c_str());
}

ModuleMIB::~ModuleMIB()
{
}

Oid::I_t ModuleMIB::componentId() const
{
	return _moduleOid.data()[ZQSNMP_OID_OFFSET_COMPONENT_ID];
}

Oid::I_t ModuleMIB::moduleId() const
{
	return _moduleOid.data()[ZQSNMP_OID_OFFSET_MODULE_ID];
}

bool ModuleMIB::chopOid(const Oid& oidFull, Oid& subOid, Oid::I_t& fieldId)
{
	subOid = Oid();
	fieldId = 1;
	if (_moduleOid.isDescendant(oidFull))
	{
		subOid = oidFull.sub(_moduleOid.length());
		if (subOid.length() >= 1)
		{
			fieldId = subOid[0];
			subOid = subOid.sub(1);
		}

		return true;
	}

	return false;
}

Oid ModuleMIB::buildupOid(const Oid& subOid, Oid::I_t fid) const
{
	Oid fullOid = _moduleOid;
	fullOid.append(fid);
	fullOid.append(subOid);
	return fullOid;
}

ModuleMIB::ChildrenMap::iterator ModuleMIB::_locateForOid(Oid subOid, bool& bfound)
{
	bfound = false;
	// std::string searchKey = subOid.str();
	ChildrenMap::iterator it = _childrenMap.upper_bound(subOid); 
	if (it == _childrenMap.begin())
		return it; 

	if (0 == subOid.compare((--it)->first)) // step back by 1 to compare
		bfound = true;
	else it++; // otherwise back to where upper_bound was

	return it;
}

bool ModuleMIB::addObject(SNMPObject::Ptr obj, const Oid& csubOid)
{
    Oid subOid = csubOid;
	if (NULL == obj)
		return false;

	if (subOid.isNil() && NULL != _mibe)
	{
		for (size_t i = 0; NULL != _mibe[i].varname; i++)
		{
			if (0 == obj->_varname.compare(_mibe[i].varname))
			{
				// subOid = Oid(&_mibe[i].subOid, 1);
				subOid = Oid(_mibe[i].strSubOid);
				break;
			}
		}

		// validate the module Oid
		if (subOid.isNil() || subOid[0] != moduleId())
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "addObject() subOid[%s] for var[%s] mismatches moduleId[%d], ignore"),
						subOid.str().c_str(), obj->_varname.c_str(), (int)moduleId());
			return false;
		}

		// cut off the module and attr Oids
		size_t posOid = 1;
		if (subOid.length() >2 && SNMPObject::vf_Value == subOid[1])
			posOid =2;
		subOid = subOid.sub(posOid);
	}

	if (subOid.isNil())
	{
		if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "addObject() NIL subOid for var[%s], ignore"), obj->_varname.c_str());
		return false;
	}

	char buf[16] ="\0";
	snprintf(buf, sizeof(buf)-1, ":%s(%s)", SNMPObject::typestr(obj->_vtype), obj->_readonly?"ro":"rw");
	obj->_varname += buf;

	ZQ::common::MutexGuard g(_lock);
	MAPSET(ChildrenMap, _childrenMap, subOid, obj); 
	if (_flags_VERBOSE & VFLG_VERBOSE_MIB)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ModuleMIB, "addObject() added subOid[%s] for var[%s], moduleId[%d]"),
				subOid.str().c_str(), obj->_varname.c_str(), (int)moduleId());
	return true;
}

SNMPObject::Ptr ModuleMIB::getObject(const Oid& subOid)
{
	if (subOid.isNil())
		return NULL;

	ZQ::common::MutexGuard g(_lock);
	ChildrenMap::iterator it = _childrenMap.find(subOid.str());
	if (_childrenMap.end() == it)
		return NULL;

	return it->second;
}

#define TAG_RESERVED_PREFIX    "$$"
#define TAG_TABLE_NODE         TAG_RESERVED_PREFIX "TABLE"

bool ModuleMIB::reserveTable(const std::string& tblname, size_t columns, Oid& subOidTable)
{
	if (subOidTable.isNil() && NULL != _mibe)
	{
		for (size_t i = 0; NULL != _mibe[i].varname; i++)
		{
			if (0 == tblname.compare(_mibe[i].varname))
			{
				subOidTable = Oid(_mibe[i].strSubOid);
				break;
			}
		}

		// validate the module Oid
		if (subOidTable.isNil() || subOidTable[0] != moduleId())
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "reserveTable() subOid[%s] for tbl[%s] mismatches moduleId[%d]"), 
						subOidTable.str().c_str(), tblname.c_str(), (int)moduleId());
			return false;
		}

		// cut off the module and attr Oids
		size_t posOid = 1;
		if (subOidTable.length() >2 && SNMPObject::vf_Value == subOidTable[1])
			posOid =2;
		subOidTable = subOidTable.sub(posOid);
	}

	if (subOidTable.isNil())
	{
		if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "reserveTable() NIL subOid for tbl[%s], ignore"), tblname.c_str());
		return false;
	}

	char buf[32];
	snprintf(buf, sizeof(buf), TAG_TABLE_NODE ".%d", (int)columns);

	SNMPObject::Ptr var = new SNMPObjectDupValue(tblname, buf);

	ZQ::common::MutexGuard g(_lock);
	ChildrenMap::iterator it = _childrenMap.find(subOidTable.str());
	if (_childrenMap.end() != it) // reject if already had such a subOid
		return false;

	// the table node
	MAPSET(ChildrenMap, _childrenMap, subOidTable, var); 
	if (_flags_VERBOSE & VFLG_VERBOSE_MIB)
		_log(ZQ::common::Log::L_INFO, CLOGFMT(ModuleMIB, "reserveTable() table[%s] subOid[%s] reserved with %d columns"), tblname.c_str(), subOidTable.str().c_str(), (int)columns);

	return true;
}

bool ModuleMIB::addTableCell(const Oid& subOidTable, Oid::I_t columnId, Oid::I_t rowIndex, SNMPObject::Ptr var)
{
	if (subOidTable.isNil() || NULL == var)
		return NULL;

	bool bfound = false;

	ZQ::common::MutexGuard g(_lock);
	// validate table exists
	ModuleMIB::ChildrenMap::iterator it = ModuleMIB::_locateForOid(subOidTable, bfound);
	if (!bfound)
		return false;

	if (NULL == it->second->_vaddr || AsnType_String != it->second->type())
	{
		if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "addTableCell() var[%s] into subOid[%s] is not table type"), var->_varname.c_str(), subOidTable.str().c_str());
		return false;
	}

	std::string tbldesc = *(std::string*)it->second->_vaddr;
	if (0 != tbldesc.compare(0, sizeof(TAG_TABLE_NODE), TAG_TABLE_NODE "."))
	{
		if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "addTableCell() var[%s] subOid[%s] is not a table"), var->_varname.c_str(), subOidTable.str().c_str());
		return false;
	}

	int nCols = atoi(tbldesc.substr(sizeof(TAG_TABLE_NODE)).c_str());
	if ((int)columnId > nCols)
	{
		if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "addTableCell() var[%s] column[%d] exceeded reserved size[%d]"), var->_varname.c_str(), (int)columnId, (int)nCols);
		return false;
	}

	// validate column exists
	Oid soid = subOidTable;
	soid.append(1); // xxxTableEntry
	soid.append(columnId);
	// it = ModuleMIB::_locateForOid(soid, bfound);
	// if (!bfound)
	//	 return false;

	// fixup the cell name with row index, type/access
	char buf[16] ="\0";
	if (std::string::npos == var->_varname.find('#'))
	{
		snprintf(buf, sizeof(buf)-2, "#%d", (int)rowIndex);
		var->_varname += buf;
	}

	snprintf(buf, sizeof(buf)-2, ":%s(%s)", SNMPObject::typestr(var->_vtype), var->_readonly?"ro":"rw");
	var->_varname += buf;

	// okay, insert the cell
	soid.append(rowIndex);
	MAPSET(ChildrenMap, _childrenMap, soid, var); 
	if (_flags_VERBOSE & VFLG_VERBOSE_MIB)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ModuleMIB, "addTableCell() table[%s] cell[%s] added"), subOidTable.str().c_str(), var->_varname.c_str());

	return true;
}

bool ModuleMIB::removeSubtree(const Oid& subOid)
{
	bool bfound = false;

	ZQ::common::MutexGuard g(_lock);
	// validate table exists
	ChildrenMap::iterator it = ModuleMIB::_locateForOid(subOid, bfound);
	if (!bfound)
		return false;

	std::string range = std::string("[") + it->first.str() + ", ";

	ChildrenMap::iterator itEnd = _childrenMap.upper_bound(subOid.str() +'z');
	if (_childrenMap.end() != itEnd)
		range += itEnd->first.str();
	range += ")";

	_childrenMap.erase(it, itEnd);

	if (_flags_VERBOSE & VFLG_VERBOSE_MIB)
		_log(ZQ::common::Log::L_INFO, CLOGFMT(ModuleMIB, "removeSubtree() removing subOid range %s"), range.c_str());

	return true;
}

Oid ModuleMIB::nextOid(const Oid& subOidFrom)
{
	Oid subOidNext;
	bool bfound = false;

	ZQ::common::MutexGuard g(_lock);
	ChildrenMap::iterator it = ModuleMIB::_locateForOid(subOidFrom, bfound);
	if (bfound)
		it++; // step to the next

	// skip those reserved node
	while (_childrenMap.end() != it)
	{
		if (AsnType_String != it->second->type())
			break;
		std::string v = *((std::string*)it->second->_vaddr);
		if (0 != v.compare(0, sizeof(TAG_RESERVED_PREFIX)-1, TAG_RESERVED_PREFIX))
			break;

		it++;
	}

	if (_childrenMap.end() != it)
		subOidNext = Oid(it->first);

	return subOidNext;
}

Oid ModuleMIB::firstOid()
{
	Oid subOidNext;
	ZQ::common::MutexGuard g(_lock);
	ChildrenMap::iterator it = _childrenMap.begin();

	// skip those reserved node
	while (_childrenMap.end() != it)
	{
		if (AsnType_String != it->second->type())
			break;
		std::string v = *((std::string*)it->second->_vaddr);
		if (0 != v.compare(0, sizeof(TAG_RESERVED_PREFIX)-1, TAG_RESERVED_PREFIX))
			break;

		it++;
	}

	if (_childrenMap.end() != it)
		subOidNext = Oid(it->first);

	return subOidNext;
}

// access by SNMPVarList
SNMPError ModuleMIB::readVars(SNMPVariable::List& vlist)
{
	SNMPError err = se_NoError;
	Oid::I_t fieldId= (Oid::I_t) SNMPObject::vf_Value;

	std::string subOidList;

	for (size_t i =0; i < vlist.size(); i++)
	{
		if (NULL == vlist[i])
			continue;

		Oid sOid;
		if (!chopOid(vlist[i]->oid(), sOid, fieldId))
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "readVars() invalid oid[%s] requested"), vlist[i]->oid().str().c_str());
			return se_NoSuchName;
		}

		ZQ::common::MutexGuard g(_lock);
		
		SNMPObject::Ptr obj = getObject(sOid);
		if (NULL == obj)
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "readVars() NULL object of oid[%s] requested"), vlist[i]->oid().str().c_str());
			return se_NoSuchName;
		}

		err = obj->read(*vlist[i], fieldId);
		if (se_NoError!= err)
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "readVars() read object[%s] failed: err(%d)"), vlist[i]->oid().str().c_str(), err);
			return err;
		}

		subOidList += sOid.str() + ", ";
	}

	if (_flags_VERBOSE & VFLG_VERBOSE_MIB)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ModuleMIB, "readVars() subOid[%s] has been read, requested %d items"), subOidList.c_str(), (int)vlist.size());
	return se_NoError;
}

SNMPError ModuleMIB::nextVar(SNMPVariable::List& vlist)
{
	Oid::I_t fieldId= (Oid::I_t) SNMPObject::vf_Value;

	if (vlist.size() <=0)
		return se_NoError; 

	SNMPVariable::Ptr var = vlist[0];
	vlist.clear();
	if (NULL == var)
	{
		if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(ModuleMIB, "nextVar() NULL object requested"));
		return se_NoError;
	}

	std::string requestedOid = var->oid().str();
	Oid tmpOid;
	if (!chopOid(var->oid(), tmpOid, fieldId))
	{
        vlist.push_back(var);
		if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "nextVar() oid[%s] reached end"), requestedOid.c_str());
		return se_NoSuchName;
	}

	Oid nxOid  = tmpOid.isNil() ? firstOid() : nextOid(tmpOid);

    if (nxOid.isNil())
    {
        vlist.push_back(var);
        return se_NoSuchName;
    }

	// build up the full Oid
	tmpOid = buildupOid(nxOid, fieldId);
	var->setOid(tmpOid);

	vlist.push_back(var);

	if (_flags_VERBOSE & VFLG_VERBOSE_MIB)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ModuleMIB, "nextVar() Oid[%s] is next to [%s]"), tmpOid.str().c_str(), requestedOid.c_str());
	return ModuleMIB::readVars(vlist);
}

SNMPError ModuleMIB::writeVars(SNMPVariable::List& vlist)
{
	SNMPError err = se_NoError;
	Oid::I_t fieldId= (Oid::I_t) SNMPObject::vf_Value;

	std::string subOidList;
	for (size_t i =0; i < vlist.size(); i++)
	{
		if (NULL == vlist[i])
			continue;

		Oid sOid;
		if (!chopOid(vlist[i]->oid(), sOid, fieldId))
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "writeVars() invalid oid[%s] requested"), vlist[i]->oid().str().c_str());
			return se_NoSuchName;
		}

		if (fieldId != (Oid::I_t) SNMPObject::vf_Value)
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(ModuleMIB, "writeVars() non-value field[%d] of oid[%s] requested to write"), (int)fieldId, vlist[i]->oid().str().c_str());
			return se_ReadOnly;
		}

		ZQ::common::MutexGuard g(_lock);

		SNMPObject::Ptr obj = getObject(sOid);
		if (NULL == obj)
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "writeVars() NULL object of oid[%s] requested"), vlist[i]->oid().str().c_str());
			return se_NoSuchName;
		}

		err = obj->write(*vlist[i]);
		if (se_NoError!= err)
		{
			if (0 == (_flags_VERBOSE & VFLG_MUTE_ERRS_MIB))
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(ModuleMIB, "writeVars() write object[%s] failed: err(%d)"), vlist[i]->oid().str().c_str(), err);
			return err;
		}

		subOidList += sOid.str() + ", ";
	}

	if (_flags_VERBOSE & VFLG_VERBOSE_MIB)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ModuleMIB, "writeVars() subOid[%s] has been written, requested %d items"), subOidList.c_str(), (int)vlist.size());
	return se_NoError;
}

// -----------------------------
// class SNMPObject
// -----------------------------
const char* SNMPObject::typestr(AsnType at)
{
	switch(at & 0x7f)
	{
	case AsnType_None:     return "NON";
	case AsnType_Int32:    return "INT";
	case AsnType_Int64:    return "I64";
	case AsnType_String:   return "STR";
	case AsnType_Oid:      return "OID";
	}
	
	return "UKW";
}

std::string SNMPObject::name() const
{ 
	size_t pos = _varname.find(':');
	return (std::string::npos == pos) ? _varname : _varname.substr(0, pos);
}

SNMPError SNMPObject::read(SNMPVariable& value, Oid::I_t fieldId) const
{
	// case 1. read the variable name
	if (vf_VarName == fieldId)
	{
		value.setValueByMemRange(AsnType_String, MemRange((void*)_varname.c_str(), _varname.length())); // SNMPVariable::mem2value(value, AsnType_String, _varname.c_str(), _varname.length());
		return se_NoError;
	}

	// case 2. read the access mode
	if (vf_Access == fieldId)
	{
		uint32 ro = _readonly ? 0:1; 
		value.setValueByMemRange(AsnType_Int32, MemRange((void*)&ro, sizeof(ro)));
		return se_NoError;
	}

    if (NULL == _vaddr)
        return se_NoSuchName;

	// case 3. read the value
	switch (_vtype)
	{
	case AsnType_Int32:
		// return SNMPVariable::mem2value(value, ASN_INTEGER32, _vaddr, sizeof(uint32));
		return value.setValueByMemRange(AsnType_Int32, MemRange((void*)_vaddr, sizeof(uint32)))?se_NoError:se_BadValue;
	
	case AsnType_Int64:
		// return SNMPVariable::mem2value(value, ASN_COUNTER64, _vaddr, sizeof(uint64));
		return value.setValueByMemRange(AsnType_Int64, MemRange((void*)_vaddr, sizeof(uint64)))?se_NoError:se_BadValue;

	case AsnType_String:
		{
			std::string* pstr = (std::string*) _vaddr;
			// return SNMPVariable::mem2value(value, ASN_OCTETSTRING, pstr->c_str(), pstr->length());
			value.setValueByMemRange(AsnType_String, MemRange((void*)pstr->c_str(), pstr->length()));
		}
		return se_NoError;

	case AsnType_CStr:
		value.setValueByMemRange(AsnType_CStr, MemRange((void*)_vaddr, _vaddr?strlen((const char*)_vaddr):0));
		return se_NoError;

	/* what's the hell for 	case AsnType_Oid:
		Oid* pOid = (Oid*)  _vaddr;
		return SNMPVariable::mem2value(value, ASN_OCTETSTRING, pOid->_data ->c_str(), pstr->length());

		pVal->asnType = ASN_OBJECTIDENTIFIER;
		pVal->asnValue.object.ids = NULL;
		pVal->asnValue.object.idLength = 0; // ???_varname.length();
		if (pVal->asnValue.object.idLength >0)
		{
			if (NULL == (pVal->asnValue.object.ids = (UINT *)SnmpUtilMemAlloc(sizeof(UINT) * pVal->asnValue.object.idLength)))
				pVal->asnValue.object.idLength = 0;
			else 
			{
				// memcpy()
			}
		}
		break;
*/
	}

	return se_NoError;
}

SNMPError SNMPObject::write(const SNMPVariable& value)
{
    if (NULL == _vaddr)
        return se_NoSuchName;

	if (_readonly)
        return se_ReadOnly;

    // step 3: check type
	if ((value.type() & 0x7f) != (_vtype &0x7f))
		return se_BadValue;

	MemRange mr = value.getValueByMemRange();
	if (NULL == mr.first)
		return se_BadValue;

	switch (_vtype)
	{
	case AsnType_Int32:
		if (mr.second < sizeof(uint32))
			return se_BadValue;
		setDataMemRange((*(uint32*)_vaddr), mr);
		break;
			
	case AsnType_Int64:
		if (mr.second < sizeof(uint64))
			return se_BadValue;
		setDataMemRange((*(uint64*)_vaddr), mr);
		break;

	case AsnType_String:
		setDataMemRange((*(std::string*)_vaddr), mr);
		break;

	case AsnType_CStr:
		setDataMemRange((char*)_vaddr, mr);
		break;
	}

	return se_NoError;

	// return SNMPVariable::value2mem(value, _vaddr);
}

// extern const ZQ::SNMP::ModuleMIB::MIBE* gTblMib_RtspProxy;
// const ZQ::SNMP::ModuleMIB::MIBE* ModuleMIB::_mibe = gTblMib_RtspProxy;


}} // namespace ZQ::Snmp

