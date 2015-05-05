// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: Agent_netsnmp.cpp,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define plugin entries of net-snmp
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/snmp/Agent_netsnmp.cpp $
// 
// 2     3/16/15 6:07p Zhiqiang.niu
// 
// 1     3/12/15 2:57p Hui.shao
// ===========================================================================

// note: for NetSNMP the entry point will be init_<name of so>,
// So, the void init_ZQSnmp() will be the entrypoint because we name this as ZQSnmp.so 
// it registers the ZQSnmp_handler to net-snmp

#include "ZQSnmp.h"

#include <sstream>
#include <XMLPreferenceEx.h>

extern ZQ::common::XMLPreferenceEx* getPreferenceNode(const std::string& path, ZQ::common::XMLPreferenceDocumentEx& config);

extern "C" void init_ZQSnmp();
#define CONFIG_FILE_INIT  = "/etc/TianShan.xml";

// -----------------------------
// class SnmpAgent_netsnmp()
// -----------------------------
// this is a plugin to extend SNMP service
class SnmpAgent_netsnmp : public ZQ::SNMP::SnmpAgent, public ZQ::common::SharedObject
{
public: // singleton
	typedef ZQ::common::Pointer <SnmpAgent_netsnmp> Ptr;
	static Ptr createAgent()
	{
		if (NULL == _ptr)
			_ptr = new SnmpAgent_netsnmp();
		return _ptr;
	}

	virtual ~SnmpAgent_netsnmp()
	{
		if (_reginfo)
			netsnmp_unregister_handler(_reginfo);

		_reginfo = NULL;
	}

public:

	static int handler(netsnmp_mib_handler* handler,
		netsnmp_handler_registration* reginfo,
		netsnmp_agent_request_info* reqinfo,
		netsnmp_request_info* requests);

	static std::string reqdesc(netsnmp_request_info* request); 

protected:
	SnmpAgent_netsnmp();
	virtual bool loadInitConfiguration();
	virtual void loadServiceConfiguration();

	ZQ::common::FileLog _flog;
    netsnmp_handler_registration* _reginfo;

private:
	static Ptr _ptr;
};

// -----------------------------
// class SnmpAgent_netsnmp
// -----------------------------
SnmpAgent_netsnmp::Ptr SnmpAgent_netsnmp::_ptr = NULL;

SnmpAgent_netsnmp::SnmpAgent_netsnmp()
: SnmpAgent(_flog), _reginfo(NULL)
{
	loadInitConfiguration();

	// open the log file
	std::string fnLog = _logDir + "SnmpAgent.log";
	try{
		_flog.open(fnLog.c_str(), _loggingMask & 0x0F);
	}
	catch(...) {}

	_log(ZQ::common::Log::L_NOTICE, CLOGFMT(SnmpAgent, "===================== initialize ======================"));

	loadServiceConfiguration();

	ZQ::SNMP::Oid rootOid = ZQ::SNMP::ModuleMIB::productRootOid();

	//prepare root oid	
	oid rOid[ZQSNMP_OID_LEN_MAX];
	size_t len =0;
	for (len =0; len < rootOid.length(); len++)
		rOid[len] = rootOid[len];

	_reginfo = netsnmp_create_handler_registration(
		"ZQSnmp", handler,
		rOid, len, HANDLER_CAN_RWRITE);

	if (_reginfo)
	{
		_reginfo->my_reg_void = this;
		netsnmp_register_handler(_reginfo);
	}
}

bool SnmpAgent_netsnmp::loadInitConfiguration()
{
	_logDir =".";
	_loggingMask = 0;

	ZQ::common::XMLPreferenceDocumentEx doc; 
	if (!doc.open(CONFIG_FILE_INIT)) 
	{
		syslog(LOG_ERR, "failed to load configuration file[%s]", CONFIG_FILE_INIT);
		return false;
	}

	ZQ::common::XMLPreferenceEx* node = getPreferenceNode("SNMP", doc);
	if (!node) 
	{
		syslog(LOG_ERR, "failed to find element[SNMP]");
		return false;
	}

	char value[256] ="\0";

	 // loggingMask 
	value[0] = '\0';
	if (node->getAttributeValue("loggingMask", value))
		_loggingMask = atol(value);

	 // logPath 
	value[0] = '\0';
	if (node->getAttributeValue("logPath", value))
		_logDir = value;

	// SnmpUdpBasePort
	value[0] = '\0';
	if (node->getAttributeValue("SnmpUdpBasePort", value))
		_baseUdpPort = atol(value);

	// Timeout
	value[0] = '\0';
	if (node->getAttributeValue("Timeout", value))
		_timeout = atol(value);

	node->free(), node = NULL;
	
	// fixup the configurations
	if (_logDir.empty())
		_logDir = "." FNSEPS;

	if (FNSEPC != _logDir[_logDir.length()-1])
		_logDir += FNSEPC;

	if (_baseUdpPort <100 || _baseUdpPort > 65000)
		_baseUdpPort = DEFAULT_BASE_UDP_PORT;

	if (_timeout <100 || _timeout > 60*1000)
		_timeout = DEFAULT_COMM_TIMEOUT;

	return true;
}

void SnmpAgent_netsnmp::loadServiceConfiguration()
{
}

// -----------------------------
// handler ZQSnmp_handler()
// -----------------------------
// When exported function will be called during DLL loading and initialization
int SnmpAgent_netsnmp::handler(netsnmp_mib_handler* handler,
				   netsnmp_handler_registration* reginfo,
				   netsnmp_agent_request_info* reqinfo,
				   netsnmp_request_info* requests)
{
	Ptr agent = _reginfo->my_reg_void;
	if (NULL == agent)
		return SNMP_ERR_RESOURCEUNAVAILABLE;

	for (netsnmp_request_info* req = requests ; req; req = req->next) 
	{
		std::string reqtxn = reqdesc(req);

		// step 1. build up the SNMPVariable::List
		ZQ::SNMP::SNMPVariable::List vlist;
		variable_list* var = NULL;
		for(var = req->requestvb; var; var = var->next_variable)
			vlist.push_back(new ZQ::SNMP::SSNMPVariable(var));

		// step 2. the query parameters
		ZQ::common::InetHostAddress serverAddr("127.0.0.1");
		int serverPort = portOfModule(componentOid, moduleOid);
		ZQ::common::Event::Ptr eventArrived = new ZQ::common::Event();

		uint8 pdu = ZQSNMP_PDU_UNKNOWN;
		switch(reqinfo->mode)
		{
		case MODE_GET:
			pdu = ZQSNMP_PDU_GET;
			break;

		case MODE_GETNEXT:
			pdu = ZQSNMP_PDU_GETNEXT;
			break;

		case MODE_SET_RESERVE1:
		case MODE_SET_RESERVE2:
		case MODE_SET_COMMIT:       
		case MODE_SET_ACTION:       
			pdu = ZQSNMP_PDU_SET;
			break;

		default:
			break;
		}

		if (ZQSNMP_PDU_UNKNOWN == pdu)
		{
			_log(ZQ::common::Log::L_ERROR, "unknown request PDU[%d]", reqinfo->mode);
			return SNMP_ERR_WRONGTYPE;
		}

		// step 3. sending the query
		uint cSeq = sendQuery(serverAddr, serverPort, pdu, vlist, eventArrived);
		if (0 == cSeq)
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "failed send to SubAgent[%s/%d] len[%d]"), serverAddr.getHostAddress(), serverPort);
			//err = se_GenericError;
			break;
		}

		// step 4. receiving the result
		Query result;
		if (!eventArrived->wait(_timeout) || !getResponse(cSeq, result))
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "failed to receive response(%d) from SubAgent[%s/%d] timeout[%d]"), cSeq, serverAddr.getHostAddress(), serverPort, _timeout);
			//err = se_GenericError;
			break;
		}

		// step 5. copy the result.vlist into req->requestvb
		// step 5.1 chain up the result.vlist
		result.vlist[result.vlist.size()-1]->data()->next_variable = NULL;
		for (int i = result.vlist.size()-2; i >=0; i--)
			result.vlist[i]->data()->next_variable = result.vlist[i+1]->data();

		// step 5.2 make the response to clone the list
		req->requestvb = snmp_clone_varbind(result.vlist[0]->data());

		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "txn[%s] cseq(%d) responded %d variables by inquirying %s/%d"), reqtxn.c_str(), cSeq, result.vlist.size(), serverAddr.getHostAddr(), serverPort);
	}

	return SNMP_ERR_NOERROR;
}

// -----------------------------
// entry init_ZQSnmp()
// -----------------------------
void init_ZQSnmp()
{
	SnmpAgent_netsnmp::createAgent();
}

// -----------------------------
// utilities
// -----------------------------
std::string SnmpAgent_netsnmp::reqdesc(netsnmp_request_info* request) 
{
	//#ifdef _DEBUG
	std::string reqstr;
	switch(request->agent_req_info->mode) 
	{
	case MODE_GET:
		reqstr = "GET";
		break;

	case MODE_GETNEXT:
		reqstr = "GETNEXT";
		break;

	case MODE_SET_RESERVE1:
	case MODE_SET_RESERVE2:
	case MODE_SET_COMMIT:       
	case MODE_SET_ACTION:       
		reqstr = "SET";
		break;

	default:
		reqstr = "UNKNOWN";
		break;
	}

	reqstr += " ";
	for (variable_list* var = request->requestvb; var; var= var->next_variable)
	{
		ZQ::SNMP::Oid oid(vars->name, vars->name_length);
		req += oid.str() +", ";
	}

	return reqstr;
}
