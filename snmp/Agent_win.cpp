#include "ZQSnmp.h"

#include "SystemUtils.h"
#include "SnmpInteractive.h"

#include "ZQResource.h"
#include "FileLog.h"

#include <algorithm>

// =====================================================================
//  SNMP Extention entries to work as a plugin of Microsoft SNMP service
// =====================================================================
#define SNMPEXT_API              SNMP_FUNC_TYPE // ZQ_SNMP_API
BOOL SNMPEXT_API SnmpExtensionInit(DWORD dwUptimeReference,
                                      HANDLE *phSubagentTrapEvent,
                                      AsnObjectIdentifier *pFirstSupportedRegion);

BOOL SNMPEXT_API SnmpExtensionQuery(BYTE bPduType, SnmpVarBindList *pVarBindList, 
                                       AsnInteger32 *pErrorStatus, AsnInteger32 *pErrorIndex);

BOOL SNMPEXT_API SnmpExtensionTrap(AsnObjectIdentifier *pEnterpriseOid, AsnInteger32 *pGenericTrapId, 
								   AsnInteger32 *pSpecificTrapId, AsnTimeticks *pTimeStamp, SnmpVarBindList *pVarBindList);

VOID SNMP_FUNC_TYPE SnmpExtensionClose();


namespace ZQ {
namespace SNMP {

	static std::string GetPduType(BYTE pdutype)
	{
		switch(pdutype)
		{
		case SNMP_PDU_GET:
			return "GetRequest-PDU";
		case SNMP_PDU_GETNEXT:
			return "GetNextRequest-PDU";
		case SNMP_PDU_SET:
			return "SetRequest-PDU";
		case SNMP_PDU_RESPONSE:
			return "GetResponse-PDU";
		default:
			return "Unsupported PDU";
		}
	}
// -----------------------------
// class SnmpAgent_win
// -----------------------------
// this is a plugin to extend Windows SNMP service
class SnmpAgent_win : public SnmpAgent
{
public:
	SnmpAgent_win();
	virtual ~SnmpAgent_win() { stop(); }

    SNMPError doQuery(uint8 pdutype, SnmpVarBindList* pVbl, AsnInteger32 *pErrStat, AsnInteger32 *pErrIdx);

protected:
	virtual void loadInitConfiguration();
	virtual void loadServiceConfiguration();

	ZQ::common::FileLog _flog;

private:
    struct MibRegion
    {
        UINT m_svcId;
        UINT m_svcProcess;
    };
    
	bool getRegion(const SnmpVarBind *pVb, MibRegion *pRegion);
    bool nextRegion(MibRegion *pRegion);
    SNMPError dispatchRequest(MibRegion targetRegion, BYTE pdutype, SnmpVarBind *pVb, AsnInteger32 *pErrStat);

protected:
	uint32           _cReceived;

};

// -----------------------------
// class SnmpAgent_win
// -----------------------------
SnmpAgent_win::SnmpAgent_win() 
: SnmpAgent(_flog), _cReceived(0)
{
	loadInitConfiguration();

	// open the log file
	std::string fnLog = _logDir + "WinSnmpExt.log";
	try{
		_flog.open(fnLog.c_str(), _loggingMask & 0x0F);
	}
	catch(...) {}

	_log(ZQ::common::Log::L_NOTICE, CLOGFMT(SnmpAgent, "===================== initialize ======================"));

	loadServiceConfiguration();
}

void SnmpAgent_win::loadInitConfiguration()
{
	_logDir =".";
	_loggingMask = DEFAULT_LOG_FLAGS;
	// _baseUdpPort = DEFAULT_BASE_UDP_PORT;
	// _timeoutComm = DEFAULT_COMM_TIMEOUT;

	do {
		static const char* regRoot ="SOFTWARE\\ZQ Interactive\\ZQSnmpExtension";
		HKEY hRoot;
		if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regRoot, 0, KEY_READ, &hRoot))
			break;

		if (NULL != hRoot)
		{
			DWORD vtype  = REG_NONE;
			DWORD vsize   = 0;
			char strValue[MAX_PATH] = {0};
			DWORD dwValue = 0;

			// _loggingMask
			vsize = sizeof(dwValue);
			if (ERROR_SUCCESS == RegQueryValueEx(hRoot, "LoggingMask", NULL, &vtype, (LPBYTE)&dwValue, &vsize) 
				&& REG_DWORD == vtype)
			{
				_loggingMask = dwValue;
			}

			// _logDir
			vsize = sizeof(strValue);
			if (ERROR_SUCCESS == RegQueryValueEx(hRoot, "LogDir", NULL, &vtype, (LPBYTE)strValue, &vsize) 
				&& REG_SZ == vtype)
			{
				_logDir = strValue;
			}

			// SNMP base port
			vsize = sizeof(dwValue);
			if (ERROR_SUCCESS == RegQueryValueEx(hRoot, "SnmpUdpBasePort", NULL, &vtype, (LPBYTE)dwValue, &vsize) 
				&& REG_DWORD == vtype)
			{
				if (dwValue>100 && dwValue < 65535)
					_baseUdpPort = dwValue;
			}

			// SNMP communication timeout
			vsize = sizeof(dwValue);
			if (ERROR_SUCCESS == RegQueryValueEx(hRoot, "Timeout", NULL, &vtype, (LPBYTE)dwValue, &vsize) 
				&& REG_DWORD == vtype)
			{
					_timeout = dwValue;
			}

			RegCloseKey(hRoot);
			hRoot = NULL;
		}

	} while(0);

	// fixup the result
	if (_logDir.empty())
		_logDir = "." FNSEPS;

	if (FNSEPC != _logDir[_logDir.length()-1])
		_logDir += FNSEPC;

	if (_baseUdpPort <100 || _baseUdpPort > 65000)
		_baseUdpPort = DEFAULT_BASE_UDP_PORT;

	if (_timeout <100 || _timeout > 60*1000)
		_timeout = DEFAULT_COMM_TIMEOUT;

	ZQ::SNMP::ModuleMIB::_flags_VERBOSE = (_loggingMask >>8) & 0xff;
}

void SnmpAgent_win::loadServiceConfiguration()
{
    _serviceList.clear();
    const char* root = "SOFTWARE\\ZQ Interactive\\SNMPOID\\CurrentVersion\\Services";

    HKEY hRoot;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, root, 0, KEY_READ, &hRoot))
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "loadServiceConfiguration() failed to access [%s]"), root);
        return;
    }

	bool ret = true; 

	// enumerate the services
	DWORD keyIndex = 0;
	const int BUFSIZE = 256;
	DWORD type;
	char  value[BUFSIZE];
	char  svcName[BUFSIZE] = {0};
	DWORD nName            = BUFSIZE - 1;
	DWORD nValue           = BUFSIZE - 1;

	BaseAgent::readUdpConfigure(_log, _baseUdpPort, _timeout);

	while(ERROR_SUCCESS == RegEnumKeyEx(hRoot, keyIndex, svcName, &nName, NULL, NULL, NULL, NULL))
	{
		HKEY hSvc;
		if (ERROR_SUCCESS == RegOpenKeyEx(hRoot, svcName, 0, KEY_READ, &hSvc))
		{
			type = 0;
			nValue = BUFSIZE - 1;
			memset(value, 0, sizeof(value));
            // get the service oid
            if (ERROR_SUCCESS == RegQueryValueEx(hSvc, "ServiceOID", NULL, &type, (LPBYTE)value, &nValue) && REG_DWORD == type)
            {
                ServiceInfo svc(*(DWORD*)value, svcName);
                _log(ZQ::common::Log::L_INFO, CLOGFMT(SnmpAgent, "loadServiceConfiguration() OID[%d] for service[%s]"), svc.id, svc.name.c_str());

                // enumerate the modules
                DWORD entryIndex = 0;
                char entry[BUFSIZE];
                entry[BUFSIZE - 1] = '\0';

                DWORD nEntry = BUFSIZE - 1;
                nValue = BUFSIZE - 1;
                while (ERROR_SUCCESS == RegEnumValue(hSvc, entryIndex, entry, &nEntry, NULL, &type, (LPBYTE)&value, &nValue))
                {
                    if (REG_DWORD == type && 0 == strncmp(entry, "mod", 3))
                    { // insert the module in to the proper position
                        DWORD modId = *(DWORD*)value;
                        _log(ZQ::common::Log::L_INFO, CLOGFMT(SnmpAgent, "loadServiceConfiguration() OID[%d] of module[%s] for service[%s]"), modId, entry, svc.name.c_str());
						svc.subModules.push_back(ModuleInfo(modId, entry));
                    }
                    nEntry = BUFSIZE - 1;
                    nValue = BUFSIZE - 1;
                    ++entryIndex;
                }

				// add the default modules
				svc.subModules.push_back(ModuleInfo(1, "shell"));
				svc.subModules.push_back(ModuleInfo(2, "app"));
				svc.subModules.push_back(ModuleInfo(3, "ext"));
				std::sort(svc.subModules.begin(), svc.subModules.end());

                // store the service data
				_serviceList.push_back(svc);
            }
            else
                _log(ZQ::common::Log::L_WARNING, CLOGFMT(SnmpAgent, "loadServiceConfiguration() failed to read OID of service[%s]"), svcName);

			RegCloseKey(hSvc);
        }
        else
            _log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "loadServiceConfiguration() failed to open subkey[%s] under Reg[%s]"), svcName, root);

		// update the parameter
        nName = BUFSIZE - 1;
        ++keyIndex;
    }

    RegCloseKey(hRoot);

	std::sort(_serviceList.begin(), _serviceList.end());
    _log(ZQ::common::Log::L_INFO, CLOGFMT(SnmpAgent, "loadServiceConfiguration() read %d services"), _serviceList.size());
}

bool SnmpAgent::nextModule(UINT componentOid, UINT& moduleOid) const
{
    for (size_t iSvc = 0; iSvc < _serviceList.size(); iSvc++)
    {
		if (_serviceList[iSvc].id < componentOid)
			continue;

		if (_serviceList[iSvc].id > componentOid)
			return false;

		const ModuleList& ml = _serviceList[iSvc].subModules;
		for (size_t iMod = 0; iMod < ml.size(); iMod++)
		{
			if (ml[iMod].id <= moduleOid)
				continue;

			moduleOid = ml[iMod].id;
			return true;
		}

		break;
    }

    return false;
}

/*
SNMPError SnmpAgent_win::doQuery(BYTE pdutype, SnmpVarBindList* pVbl, AsnInteger32* pErrStat, AsnInteger32* pErrIdx)
{
    if (NULL == pVbl || NULL == pErrStat)
        return se_GenericError;

    *pErrStat = se_NoError;
	*pErrIdx = 0;
	uint32 err = se_NoError;

	for (UINT i = 0; i < pVbl->len; i++)
	{
		SnmpVarBind* pVar = &pVbl->list[i];
		Oid oid(pVar->name.ids, pVar->name.idLength);
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "doQuery() Vars[%u] oid[%s]"), i, oid.str().c_str());

		if (oid.length() < (ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY+2))
		{
			*pErrStat = se_NoSuchName;
			*pErrIdx = i + 1;//
			return se_NoSuchName;
		}

		UINT componentOid = oid[ZQSNMP_OID_OFFSET_COMPONENT_ID];
		UINT moduleOid    = oid[ZQSNMP_OID_OFFSET_MODULE_ID];

		do {
			uint8 msg[ZQSNMP_MSG_LEN_MAX];

			// step 1. componse the request
			Subagent::Msgheader* pHeader = (Subagent::Msgheader*) msg;
			uint8* p = msg + sizeof(Subagent::Msgheader);

			memset(pHeader, 0, sizeof(Subagent::Msgheader));
			pHeader->_agentRecvSeq = _cReceived;

			// step 2. prepare the payload
			SNMPVariable::List vlist;
			SNMPVariable::Ptr var = new SNMPVariable(oid, &pVbl->list[i].value);
			if (NULL == var)
			{
				err = se_GenericError;
				break;
			}

			vlist.push_back(var);

			// put the payload
			p += Subagent::encodeMessage(p, sizeof(msg) - sizeof(Subagent::Msgheader), pdutype, err, vlist);
			int msglen = (int)(p-msg);

			///////////dispatchRequest()////////////////
			//step 3. send the request
			ZQ::common::InetHostAddress serverAddr("127.0.0.1");
			int serverPort = portOfModule(componentOid, moduleOid);

			ZQ::common::UDPSocket   udpComm;
			udpComm.setCompletion(false);

			if (!udpComm.isPending(ZQ::common::Socket::pendingOutput, _timeout))
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "udp timed out[%d] prior to sending"), _timeout);
				err = se_GenericError;
				break;
			}

			if (udpComm.sendto(msg, msglen, serverAddr, serverPort) <=0)
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "failed send to SubAgent[%s/%d] len[%d]"), serverAddr.getHostAddress(), serverPort, msglen);
				err = se_GenericError;
				break;
			}

			if (!udpComm.isPending(ZQ::common::Socket::pendingInput, _timeout))
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "socket busy, output timeout[%d]"), _timeout);
				err = se_GenericError;
				break;
			}
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "query sent to SubAgent[%s/%d] msglen[%d]"), serverAddr.getHostAddress(), serverPort, msglen);

			// step 4. receive the response
			ZQ::common::InetHostAddress peerAddr;
			int peerPort =0;

			int resplen = udpComm.receiveFrom(msg, ZQSNMP_MSG_LEN_MAX, peerAddr, peerPort);

			if (resplen <=sizeof(Subagent::Msgheader) || (pHeader->_agentRecvSeq) != _cReceived)
			{
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(SnmpAgent, "mismatched response from [%s/%d] cseq[%d] but wished[%d]"), peerAddr.getHostAddress(), peerPort, pHeader->_agentRecvSeq, _cReceived);
				err = se_GenericError;
			}

			// step 5. decode the response
			vlist.clear();
			p = msg + sizeof(Subagent::Msgheader);
			uint32 decodeErr = se_NoError;
			if (!Subagent::decodeMessage(p, resplen - sizeof(Subagent::Msgheader), vlist, pdutype, decodeErr) || se_NoError != decodeErr || vlist.size()<=0)
			{
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(SnmpAgent, "failed to decode response from [%s/%d] cseq[%d]"), peerAddr.getHostAddress(), peerPort, pHeader->_agentRecvSeq, _cReceived);
				err = decodeErr;
				break;
			}

			// step 6. copy the result to pVbl[i]
			SnmpUtilVarBindFree(pVar);
			SnmpUtilAsnAnyCpy(&pVar->value, vlist[0]->data());

			oid = vlist[0]->oid();
			UINT oidbuf[ZQSNMP_OID_LEN_MAX];
			AsnObjectIdentifier aoid;
			aoid.ids = oidbuf;
			aoid.idLength = min(ZQSNMP_OID_LEN_MAX, oid.length());
			for (size_t i =0; i < aoid.idLength; i++)
				oidbuf[i] = oid[i];
			SnmpUtilOidCpy(&pVar->name, &aoid);

			/////////////////////////////////////

		} while(0);

		*pErrStat = err;
		*pErrIdx = i + 1;
		if (se_NoError != err)
			break;
	} // for list

#if 0
		do
		{
			//dispatch request
			SNMPError status = dispatchRequest(componentOid, moduleOid, pdutype, &(pVbl->list[i]), pErrStat);
			if (se_NoError != status)
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "dispatchRequest failed at comp[%d] mod[%d]"), componentOid, moduleOid);
				*pErrStat = se_GenericError;
			}

		} while(SNMP_PDU_GETNEXT == pdutype &&  *pErrStat == se_NoSuchName && nextRegion(&region) );

		if (*pErrStat != se_NoError)
		{
			*pErrIdx = i + 1;//
			break;
		}
	} // for list
#endif

	return (SNMPError) err;
}

*/

SNMPError SnmpAgent_win::doQuery(BYTE pdutype, SnmpVarBindList* pVbl, AsnInteger32* pErrStat, AsnInteger32* pErrIdx)
{
	if (NULL == pVbl || NULL == pErrStat)
		return se_GenericError;

	*pErrStat = se_NoError;
	*pErrIdx = 0;
	uint32 err = se_NoError;

	if (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_VERBOSE_AGENT)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "doQuery() pdu[%02X][%s] for %d vars"), pdutype, GetPduType(pdutype).c_str(), pVbl->len);

	for (UINT i = 0; i < pVbl->len; i++)
	{
		SnmpVarBind* pVar = &pVbl->list[i];
		Oid oid(pVar->name.ids, pVar->name.idLength);
		if (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_VERBOSE_AGENT)
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "doQuery() vars[%u] oid[%s]"), i, oid.str().c_str());

		if (oid.length() < (ZQSNMP_OID_LEN_TIANSHAN_SVCFAMILY+2))
		{
			*pErrStat = se_NoSuchName;
			*pErrIdx = i + 1;//
			return se_NoSuchName;
		}

		UINT componentOid = oid[ZQSNMP_OID_OFFSET_COMPONENT_ID];
		UINT moduleOid    = oid[ZQSNMP_OID_OFFSET_MODULE_ID];
		ZQ::common::InetHostAddress serverAddr("127.0.0.1");
		int serverPort = portOfModule(componentOid, moduleOid);

		do {
			// step 2. prepare the payload
			SNMPVariable::List vlist;
			SNMPVariable::Ptr var = new SNMPVariable(oid, &pVbl->list[i].value);
			if (NULL == var)
			{
				err = se_GenericError;
				break;
			}

			vlist.push_back(var);

			ZQ::common::Event::Ptr eventArrived = new ZQ::common::Event();
			if (NULL == eventArrived)
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "doQuery() failed allocate Event"));
				err = se_GenericError;
				break;
			}

			if (_bQuit)
				break;

			uint cSeq = sendQuery(serverAddr, serverPort, pdutype, vlist, eventArrived);
			if (0 == cSeq)
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "doQuery() failed send to SubAgent[%s/%d]"), serverAddr.getHostAddress(), serverPort);
				err = se_GenericError;
				break;
			}

			Query result;
			if (_bQuit || !eventArrived->wait(_timeout) || !getResponse(cSeq, result))
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "doQuery() failed receive response(%d) from SubAgent[%s/%d] timeout[%d]"), cSeq, serverAddr.getHostAddress(), serverPort, _timeout);
				err = se_GenericError;
				break;
			}

			// step 6. copy the result to pVbl[i]
			SnmpUtilVarBindFree(pVar);
			SnmpUtilAsnAnyCpy(&pVar->value, result.vlist[0]->data());

			oid = result.vlist[0]->oid();
			UINT oidbuf[ZQSNMP_OID_LEN_MAX];
			AsnObjectIdentifier aoid;
			aoid.ids = oidbuf;
			aoid.idLength = min(ZQSNMP_OID_LEN_MAX, oid.length());
			for (size_t i =0; i < aoid.idLength; i++)
				oidbuf[i] = oid[i];
			SnmpUtilOidCpy(&pVar->name, &aoid);

			err = result.header.error;

		} while(0);

		*pErrStat = err;
		*pErrIdx = i;
		if (se_NoError != err)
			break;
	} // for list

	if (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_VERBOSE_AGENT)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "doQuery() pdu[%02X][%s] for %d vars: err(%d) idxErr(%d)"), pdutype, GetPduType(pdutype).c_str(), pVbl->len, pErrStat?*pErrStat:0, pErrIdx?*pErrIdx:0);

	return (SNMPError) err;
}

}} // namespace ZQ::SNMP

// =====================================================================
//  SNMP Extention entries to work as a plugin of Microsoft SNMP service
// =====================================================================

static ZQ::SNMP::SnmpAgent_win* gAgent = NULL;
/*
// -----------------------------
// class Logger
// -----------------------------
ZQ::common::Log     NullLog;
ZQ::common::FileLog MainLog;

void InitLogger()
{
	std::string logDir =".";
	DWORD loggingMask = 0;

	do {
		static const char* regRoot ="SOFTWARE\\ZQ Interactive\\%s\\CurrentVersion\\Services\\ZQSnmpExtension";
		HKEY hRoot;
		if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regRoot, 0, KEY_READ, &hRoot))
			break;

		if (NULL != hRoot)
		{
			DWORD vtype  = REG_NONE;
			DWORD vsize   = 0;
			char strValue[MAX_PATH] = {0};
			DWORD dwValue = 0;

			// _loggingMask
			vsize = sizeof(dwValue);
			if (ERROR_SUCCESS == RegQueryValueEx(hRoot, "LoggingMask", NULL, &vtype, (LPBYTE)&dwValue, &vsize) 
				&& REG_DWORD == vtype)
			{
				loggingMask = dwValue;
			}

			// _logDir
			vsize = sizeof(strValue);
			if (ERROR_SUCCESS == RegQueryValueEx(hRoot, "LogDir", NULL, &vtype, (LPBYTE)strValue, &vsize) 
				&& REG_SZ == vtype)
			{
				logDir = strValue;
			}

			RegCloseKey(hRoot);
			hRoot = NULL;
		}

	} while(0);

	// fixup the result
	if (logDir.empty())
		logDir = "." FNSEPS;

	if (FNSEPC != logDir[logDir.length()-1])
		logDir += FNSEPC;

		// open the log file
	std::string fnLog = logDir + "WinSnmpExt.log";
	try{
		MainLog.open(fnLog.c_str(), loggingMask & 0x0F);
	}
	catch(...)
	{
	}
}
*/

// -----------------------------
// dummy DllMain()
// -----------------------------
BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  uReason, 
                      LPVOID lpReserved
                      )
{
    switch (uReason)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

	case DLL_PROCESS_DETACH:
		SnmpExtensionClose();
		break;
    }

	return TRUE;
}
static UINT m_ZQTianShanOidPrefixData[ZQSNMP_OID_LEN_MAX];
static AsnObjectIdentifier  m_ZQTianShanOidPrefix;
// -----------------------------
// entry SnmpExtensionInit()
// -----------------------------
// When exported function will be called during DLL loading and initialization
BOOL SNMP_FUNC_TYPE SnmpExtensionInit(DWORD dwUptimeReference,
									  HANDLE *phSubagentTrapEvent,
									  AsnObjectIdentifier *pFirstSupportedRegion)
{
	gAgent = new ZQ::SNMP::SnmpAgent_win();
	if (NULL == gAgent)
		return FALSE;

	gAgent->start();

	ZQ::SNMP::Oid rootOid = ZQ::SNMP::ModuleMIB::productRootOid();

	//prepare root oid	
	size_t len =0;
	for (len =0; len < rootOid.length() && len < ZQSNMP_OID_LEN_MAX; len++)
		m_ZQTianShanOidPrefixData[len] = rootOid[len];

	m_ZQTianShanOidPrefix.idLength = len;
	m_ZQTianShanOidPrefix.ids =  m_ZQTianShanOidPrefixData;

	if(phSubagentTrapEvent)
		*phSubagentTrapEvent = NULL;
	if(pFirstSupportedRegion)
		*pFirstSupportedRegion = m_ZQTianShanOidPrefix;
	return TRUE;
}

// -----------------------------
// entry SnmpExtensionQuery()
// -----------------------------
// this export is to query the MIB table and fields
BOOL SNMP_FUNC_TYPE SnmpExtensionQuery(BYTE bPduType, 
                                       SnmpVarBindList *pVarBindList, 
                                       AsnInteger32 *pErrorStatus, AsnInteger32 *pErrorIndex)
{
    // _log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "SnmpExtensionQuery... [pdutype = %s], [VarBindList: length = %u]"), ZQSnmpUtil::GetPduType(bPduType).c_str(), pVarBindList->len);
    if (!gAgent)
		return FALSE;

	ZQ::SNMP::SNMPError err = gAgent->doQuery(bPduType, pVarBindList, pErrorStatus, pErrorIndex);

	return (ZQ::SNMP::se_NoError == err) ? TRUE :FALSE;
}

// -----------------------------
// entry SnmpExtensionTrap()
// -----------------------------
// this function just simulate traps
//   Traps just a 2 variables value from MIB
//  Trap is kind of event from server to client
///         When ever the event is set service will call this function and gets the parameters filled.
//       After filling the parameters service willsend the trap to all the client connected
BOOL SNMP_FUNC_TYPE SnmpExtensionTrap(AsnObjectIdentifier *pEnterpriseOid, AsnInteger32 *pGenericTrapId, AsnInteger32 *pSpecificTrapId, AsnTimeticks *pTimeStamp, SnmpVarBindList *pVarBindList)
{
    return FALSE;
}
 
// -----------------------------
// entry SnmpExtensionClose()
// -----------------------------
VOID SNMP_FUNC_TYPE SnmpExtensionClose()
{
	if (gAgent)
	{
		gAgent->stop();
		Sleep(500);
		delete gAgent;
	}

	gAgent = NULL;
}

