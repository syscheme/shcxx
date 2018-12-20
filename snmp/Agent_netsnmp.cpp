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
// Ident : $Id: Agent_netsnmp.cpp,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define plugin entries of net-snmp
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/snmp/Agent_netsnmp.cpp $
// 
// 6     2/13/17 5:26p Dejian.fei
// snmp  Async
// 
// 5     12/26/16 2:49p Hui.shao
// 
// 4     1/15/16 9:42a Li.huang
// 
// 3     12/30/15 4:34p Li.huang
// 
// 2     3/16/15 6:07p Zhiqiang.niu
// 
// 1     3/12/15 2:57p Hui.shao
// ===========================================================================

// note: for NetSNMP the entry point will be init_<name of so>,
// So, the void init_ZQSnmp() will be the entrypoint because we name this as ZQSnmp.so 
// it registers the ZQSnmp_handler to net-snmp

#include "ZQSnmp.h"
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <sstream>
#include "XMLPreferenceEx.h"

extern ZQ::common::XMLPreferenceEx* getPreferenceNode(const std::string& path, ZQ::common::XMLPreferenceDocumentEx& config);

extern "C" void init_ZQSnmp();
#define CONFIG_FILE_INIT   "/etc/TianShan.xml"
#define FILE_LOG_NAME      "SnmpAgent.log"

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
		_log(ZQ::common::Log::L_INFO, CLOGFMT(SnmpAgent, "~SnmpAgent_netsnmp()"));

		_log.flush();

		if (_reginfo)
			netsnmp_unregister_handler(_reginfo);

		stop();
		_reginfo = NULL;
	}

	SnmpAgent_netsnmp();

public:

	static int handler(netsnmp_mib_handler* handler,
		netsnmp_handler_registration* reginfo,
		netsnmp_agent_request_info* reqinfo,
		netsnmp_request_info* requests);

	static std::string reqdesc(netsnmp_request_info* request); 

	ZQ::common::FileLog& getLog(){return _flog;};
protected:

//	SnmpAgent_netsnmp();
	virtual bool loadInitConfiguration();
	virtual void loadServiceConfiguration();


    netsnmp_handler_registration* _reginfo;
	ZQ::common::FileLog _flog;

private:
	static Ptr _ptr;
};

// -----------------------------
// class SnmpAgent_netsnmp
// -----------------------------
SnmpAgent_netsnmp::Ptr SnmpAgent_netsnmp::_ptr;
SnmpAgent_netsnmp* gSnmp= NULL;

SnmpAgent_netsnmp::SnmpAgent_netsnmp()
: SnmpAgent(_flog), _reginfo(NULL)
{
	loadInitConfiguration();

	std::string fnLog = _logDir + FILE_LOG_NAME;
	try{
		_flog.open(fnLog.c_str(), _loggingMask & 0x0F);
	}
	catch(...) {}

	_log(ZQ::common::Log::L_NOTICE, CLOGFMT(SnmpAgent, "===================== initialize ======================"));

	loadServiceConfiguration();
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

	ZQ::SNMP::ModuleMIB::_flags_VERBOSE = (_loggingMask >>8) & 0xff;

	return true;
}

void SnmpAgent_netsnmp::loadServiceConfiguration()
{
}

static std::string GetPduType(uint8 pdutype)
{
	switch(pdutype)
	{
	case ZQSNMP_PDU_GET:
		return "GetRequest-PDU";
	case ZQSNMP_PDU_GETNEXT:
		return "GetNextRequest-PDU";
	case ZQSNMP_PDU_SET:
		return "SetRequest-PDU";
	default:
		return "Unsupported PDU";
	}
}

bool copyVarlist(netsnmp_variable_list* destList, netsnmp_variable_list *srcList)
{
	for (; destList && srcList; destList = destList->next_variable, srcList= srcList->next_variable)
	{
		destList->next_variable = NULL;
		destList->name_length = srcList->name_length;
		//copy oid
		memcpy(destList->name, srcList->name, sizeof(oid) * srcList->name_length);
		//copy type;
		destList->type = srcList->type;
		destList->val_len = srcList->val_len;

		if (0 != snmp_set_var_typed_value(destList, srcList->type, srcList->val.bitstring, srcList->val_len))
			return false;
	}
	return true;
}
// -----------------------------
// class SyncQueryCB
// -----------------------------
//  public ZQ::SNMP::SnmpAgent
class SyncQueryCB : public ZQ::SNMP::SnmpAgent::QueryCB
{
public:
	typedef ZQ::common::Pointer< SyncQueryCB > Ptr;
	SyncQueryCB(ZQ::SNMP::SnmpAgent& agent) : _agent(agent) {}
	bool wait(timeout_t timeout=TIMEOUT_INF) { return _event.wait(timeout); }

	virtual void OnResult() { _event.signal(); }
	virtual void OnError(int errCode) {}

	ZQ::SNMP::SnmpAgent&        _agent;
	ZQ::common::Event _event;
};
// -----------------------------
// handler ZQSnmp_handler()
// -----------------------------
// When exported function will be called during DLL loading and initialization
 int SnmpAgent_netsnmp::handler(netsnmp_mib_handler* handler,
								 netsnmp_handler_registration* reginfo,
								 netsnmp_agent_request_info* reqinfo,
								 netsnmp_request_info* requests) 
 {
	uint32 err = SNMP_ERR_NOERROR;

	if(reginfo->my_reg_void == NULL)
	{
		gSnmp = new SnmpAgent_netsnmp();

		if(NULL == gSnmp)
			return SNMP_ERR_RESOURCEUNAVAILABLE;

		gSnmp->start();
		reginfo->my_reg_void =  gSnmp;
		gSnmp->_reginfo = reginfo;
		SnmpAgent_netsnmp::_ptr = gSnmp;
	}

	SnmpAgent_netsnmp* agent = (SnmpAgent_netsnmp*)(reginfo->my_reg_void);

	if (NULL == agent)
		return SNMP_ERR_RESOURCEUNAVAILABLE;

	ZQ::common::FileLog& log = agent->getLog();

	log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent_netsnmp,"handler() enter, handler[%p]"), handler);									 

	for (netsnmp_request_info* req = requests ; req; req = req->next) 
	{
		std::string reqtxn = reqdesc(req);

		bool bOid = false;

		// step 1. build up the SNMPVariable::List
		ZQ::SNMP::SNMPVariable::List vlist;
		int serverPort;
		uint32 componentOid;
		uint32 moduleOid;
		ZQ::common::InetHostAddress serverAddr("127.0.0.1");

		for(variable_list* vars = req->requestvb; vars; vars = vars->next_variable)
		{
			if(!bOid)
			{
				ZQ::SNMP::Oid oidTemp(vars->name, vars->name_length);
				if (oidTemp.length() < (ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY+2))
				{
					return SNMP_ERR_NOSUCHNAME;
				}
				componentOid = oidTemp[ZQSNMP_OID_OFFSET_COMPONENT_ID];
				moduleOid    = oidTemp[ZQSNMP_OID_OFFSET_MODULE_ID];
				// step 2. the query parameters
				serverPort = agent->portOfModule(componentOid, moduleOid);
				bOid = true;
			}

			vlist.push_back(new ZQ::SNMP::SNMPVariable(vars));	
		}

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
			log(ZQ::common::Log::L_ERROR,CLOGFMT(SnmpAgent_netsnmp, "handler[%p] unknown request PDU[%d]"), handler, reqinfo->mode);
			return SNMP_ERR_WRONGTYPE;
		}

		log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent_netsnmp, "handler[%p] read vars[%d]from [%s:%d] PDU[%s]"), handler, vlist.size(),serverAddr.getHostAddress(), serverPort, GetPduType(pdu).c_str());

		// step 3. sending the query
		SyncQueryCB::Ptr cbQuery = new SyncQueryCB(*agent);
		if (NULL == cbQuery)
		{
			log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "handler() failed allocate cbQuery"));
			return SNMP_ERR_RESOURCEUNAVAILABLE;
		}

		uint cSeq = agent->sendQuery(cbQuery,serverAddr, serverPort, pdu, vlist);
		if (0 == cSeq)
		{
			log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent_netsnmp, "failed send to SubAgent[%s/%d]"), serverAddr.getHostAddress(), serverPort);
			return SNMP_ERR_RESOURCEUNAVAILABLE;
		}

		// step 3. wait for response
		// step 4. receiving the cbQuery
		if (!cbQuery->wait(agent->getTimeout()))
		{
			log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent_netsnmp, "failed to receive response(%d) from SubAgent[%s/%d] timeout[%d]"), cSeq, serverAddr.getHostAddress(), serverPort, agent->getTimeout());
			log.flush();
			return SNMP_ERR_RESOURCEUNAVAILABLE;
		}


		// step 5. copy the  cbQuery->vlist into req->requestvb
		// step 5.1 chain up the  cbQuery->vlist
		cbQuery->vlist[cbQuery->vlist.size()-1]->data()->next_variable = NULL;
		for (int i =  cbQuery->vlist.size()-2; i >=0; i--)
			cbQuery->vlist[i]->data()->next_variable =  cbQuery->vlist[i+1]->data();

		// step 5.2 make the response to clone the list
//		req->requestvb = snmp_clone_varbind(cbQuery->vlist[0]->data());

		copyVarlist(req->requestvb,  cbQuery->vlist[0]->data());
  
		std::string respStr = reqdesc(req);
		log(ZQ::common::Log::L_INFO, CLOGFMT(SnmpAgent_netsnmp, "handler[%p]txn[%s]response[%s] cseq(%d) responded [%d] errorcode[%d]variables by inquirying %s/%d"),handler, reqtxn.c_str(), respStr.c_str(),cSeq, (int)cbQuery->vlist.size(), err,serverAddr.getHostAddress(), serverPort);
		log.flush();

		err = cbQuery->header.error;
		if (SNMP_ERR_NOERROR != err)
		{
			log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent_netsnmp, "txn[%s]error[%u]"), reqtxn.c_str(),err);
			log.flush();
			return err;
		}
	}
//	log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent_netsnmp, "handler() leave, handler[%p]"), handler);
//	log.flush();
	return SNMP_ERR_NOERROR;
}

// -----------------------------
// entry init_ZQSnmp()
// -----------------------------

void init_ZQSnmp()
{
	std::string logDir =".";
	uint32 loggingMask = 0;

	ZQ::common::XMLPreferenceDocumentEx doc; 
	if (!doc.open(CONFIG_FILE_INIT)) 
	{
		syslog(LOG_ERR, "failed to load configuration file[%s]", CONFIG_FILE_INIT);
		return ;
	}

	ZQ::common::XMLPreferenceEx* node = getPreferenceNode("SNMP", doc);
	if (!node) 
	{
		syslog(LOG_ERR, "failed to find element[SNMP]");
		return ;
	}

	char value[256] ="\0";

	// loggingMask 
	value[0] = '\0';
	if (node->getAttributeValue("loggingMask", value))
		loggingMask = atol(value);

	// logPath 
	value[0] = '\0';
	if (node->getAttributeValue("logPath", value))
		logDir = value;

	// fixup the configurations
	if (logDir.empty())
		logDir = "." FNSEPS;

	if (FNSEPC != logDir[logDir.length()-1])
		logDir += FNSEPC;

	// open the log file
	std::string fnLog = logDir + FILE_LOG_NAME;
	ZQ::common::FileLog filelog;
	try{
		filelog.open(fnLog.c_str(), loggingMask & 0x0F);
	}
	catch(...)
	{	
		syslog(LOG_ERR, "failed to create logfile");
		return;
	}

	ZQ::SNMP::Oid rootOid = ZQ::SNMP::ModuleMIB::productRootOid();

	//prepare root oid	
	oid rOid[ZQSNMP_OID_LEN_MAX];
	size_t len =0;
	for (len =0; len < rootOid.length(); len++)
		rOid[len] = rootOid[len];

	netsnmp_handler_registration* reginfo = netsnmp_create_handler_registration(
					"ZQSnmp", SnmpAgent_netsnmp::handler,rOid, len, HANDLER_CAN_RWRITE);

	if (!reginfo)
	{
		filelog(ZQ::common::Log::L_ERROR, "SnmpAgent failed to create netsnmp handler");									 
		return;
	}
	reginfo->my_reg_void = NULL;
	int ret = netsnmp_register_handler(reginfo);

	if(ret != SNMPERR_SUCCESS )
	{
		filelog(ZQ::common::Log::L_ERROR, "SnmpAgent netsnmp failed to register handler[%p] return[%d]", reginfo, ret);
		return ;
	}

	filelog(ZQ::common::Log::L_INFO, "SnmpAgent initialized: oid[%s],  register handler[%p]", (rootOid.str()).c_str(), reginfo);									 
	filelog.flush();
}

void deinit_ZQSnmp()
{
	if(gSnmp) 
	{
		ZQ::common::FileLog& log = gSnmp->getLog();
		log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "deinit_myAgent()"));
		log.flush();
		delete gSnmp;
		gSnmp = NULL;
	}
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
	for (variable_list* vars = request->requestvb; vars; vars= vars->next_variable)
	{
		ZQ::SNMP::Oid oid1(vars->name, vars->name_length);
		char buf[2056]= "";
		std::string oidstr=  oid1.str();

		sprintf(buf,"Oid:[%s],type:[%u]varlen[%d]nextVariable[%p]", 
			oidstr.c_str(), vars->type,vars->val_len, vars->next_variable);
		reqstr +=std::string(buf);

		unsigned char bitstring = 0;
		unsigned long high = 0;
		unsigned long low = 0;
		long interger = 0;
		oid objid = 0;
		unsigned char str = 0;

		if( NULL != vars->val.bitstring)
			bitstring = *(vars->val.bitstring);
		if( NULL != vars->val.counter64)
		{
			high = vars->val.counter64->high;
			low = vars->val.counter64->low;
		}
		if(NULL != vars->val.integer)
			interger = *(vars->val.integer);
		if(NULL != vars->val.objid)
			objid = *(vars->val.objid);
		if( NULL != vars->val.string)
			str = *(vars->val.string);

		memset(buf, 0, sizeof(buf));

//		sprintf(buf,"val.bitstring[%p=%u],varsTemp->val.counter64[%p=high:%lld,low:%lld],varsTemp->val.integer[%p=%lld],varsTemp->val.objid[%p=%u]varsTemp->val.string[%p=%u] ", 
//			vars->val.bitstring, bitstring, vars->val.counter64, high, low, vars->val.integer, interger, vars->val.objid, objid, vars->val.string, str);

		reqstr +=std::string(buf);
	}
	return reqstr;
}
