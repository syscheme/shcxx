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
// Desc  : The communicator to exchange messages with SNMP agent
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/snmp/SubAgent.cpp $
// 
// 32    1/15/16 9:42a Li.huang
// 
// 31    12/29/15 3:20p Li.huang
// using SubAgent instead of  Subagent
// 
// 29    12/28/15 2:29p Li.huang
// using SubAgent instead of  Subagent
// 
// 28    12/28/15 9:19a Li.huang
// 
// 27    12/23/15 4:16p Li.huang
// 
// 26    9/10/15 4:00p Hongquan.zhang
// 
// 25    3/27/15 2:51p Build
// 
// 24    3/27/15 10:49a Hui.shao
// 
// 23    3/26/15 7:08p Hui.shao
// logging
// 
// 22    3/26/15 6:00p Hui.shao
// SubAgent to replace Subagent
// 
// 21    3/26/15 10:14a Hui.shao
// log by flags
// 
// 20    3/12/15 7:31p Hui.shao
// include the entire mib2ctable into the dll
// 
// 19    3/12/15 12:54p Hui.shao
// 
// 17    3/11/15 5:59p Hui.shao
// 
// 16    3/11/15 11:43a Zhiqiang.niu
// 
// 15    3/09/15 10:18a Hui.shao
// BaseAgent
// 
// 14    3/06/15 2:53p Zhiqiang.niu
// 
// 13    3/06/15 2:52p Zhiqiang.niu
// modify for linux
// 
// 12    3/06/15 11:38a Hui.shao
// drafted for future messaging between Agent and SubAgent
// 
// 11    3/05/15 5:54p Hui.shao
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
// 5     3/02/15 10:23a Hui.shao
// renamed ModuleMIB to ComponentMID
// 
// 4     2/28/15 6:52p Hui.shao
// 
// 3     2/28/15 5:24p Hui.shao
// draft of definition
// ===========================================================================

#include "ZQSnmp.h"

#include "UDPSocket.h"
#include "SystemUtils.h"
#include "SnmpInteractive.h"

#ifdef ZQ_OS_LINUX
#include <sstream>
#include <XMLPreferenceEx.h>

extern ZQ::common::XMLPreferenceEx* getPreferenceNode(const std::string& path, ZQ::common::XMLPreferenceDocumentEx& config);
#endif

// #include "ZQSnmpUtil.h"

#define MAX_CSEQ    0x0fffffff

namespace ZQ {
namespace SNMP {

#define SALOG (_log)

// -----------------------------
// class BaseAgent
// -----------------------------
uint BaseAgent::lastCSeq()
{
	int v = _lastCSeq.add(1);
	if (v>0 && v < MAX_CSEQ)
		return (uint) v;

	static ZQ::common::Mutex lock;
	ZQ::common::MutexGuard g(lock);
	v = _lastCSeq.add(1);
	if (v >0 && v < MAX_CSEQ)
		return (uint) v;

	_lastCSeq.set(1);
	v = _lastCSeq.add(1);

	return (uint) v;
}

uint32 BaseAgent::portOfModule(uint32 componentId, uint32 moduleId)
{
	return _baseUdpPort + componentId + moduleId;
}

uint8 BaseAgent::checksum(const uint8* data, int len)
{
	if (NULL == data || len <= 0)
		return 0;

	uint8 checksum = 0;
	for(int i = 0; i < len; ++i)
		checksum ^= data[i];

	return checksum;
}

bool BaseAgent::readUdpConfigure(ZQ::common::Log& log, uint32& baseUdpPort, uint32& timeoutComm)
{
	log(ZQ::common::Log::L_DEBUG, CLOGFMT(Subagent, "readUdpConfigure() reading UDP configuration"));
	bool ret = false;

	baseUdpPort = DEFAULT_BASE_UDP_PORT;
	timeoutComm = (5000);

#if defined(ZQ_OS_MSWIN)

	HKEY        hRoot;
	DWORD       type;
	DWORD       pendType;
	const int   BUFSIZE        = 256;
	char        value[BUFSIZE] = {0};
	DWORD       nValue         = BUFSIZE - 1;
	const char* queryString    = "SnmpUdpBasePort";
	const char* root           = "SOFTWARE\\ZQ Interactive\\SNMPOID\\CurrentVersion\\Services";

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, root, 0, KEY_READ, &hRoot))
	{
		log(ZQ::common::Log::L_ERROR, CLOGFMT(Subagent, "readUdpConfigure() failed to access reg[%s]"), root);
		return ret;
	}

	if (ERROR_SUCCESS == RegQueryValueEx(hRoot, queryString, NULL, &type, (LPBYTE)value, &nValue) 
		&& REG_DWORD == type)
	{
		ret = true;
		if (*(uint32 *)value < 65535)
			baseUdpPort = *(uint32 *)value;
	}

	nValue = BUFSIZE - 1;
	memset(value, 0, sizeof(value));
	if (ERROR_SUCCESS == RegQueryValueEx(hRoot, "SnmpAgentPendTimeOut", NULL, &pendType, (LPBYTE)value, &nValue) 
		&& REG_DWORD == pendType)
	{
		if (*(uint32 *)value >0 && *(uint32 *)value < 60*1000*30)
			timeoutComm = *(uint32 *)value;
	}
	else
		log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "readUdpConfigure() failed to read Reg[SnmpAgentPendTimeOut]"));

	RegCloseKey(hRoot);

#else

	std::istringstream       is;
    ZQ::common::XMLPreferenceDocumentEx  doc;
	char                     value[256]  =  {0};
	const char*              CONFIG      = "/etc/TianShan.xml";

	if (!doc.open(CONFIG)) 
	{
		log(ZQ::common::Log::L_ERROR, CLOGFMT(Subagent, "readUdpConfigure() failed to load configuration[%s]"), CONFIG);
		return ret;
	}

	ZQ::common::XMLPreferenceEx * node = NULL;
	node = getPreferenceNode("SNMP", doc);
	if (!node) 
	{
		log(ZQ::common::Log::L_ERROR, CLOGFMT(Subagent, "readUdpConfigure() no element SNMP in [%s]"), CONFIG);
		return ret;
	}

	baseUdpPort = DEFAULT_BASE_UDP_PORT;
	if (node->getAttributeValue("SnmpUdpBasePort", value)) 
	{
		uint32 port = atol(value);
		if (port < 65535)
			baseUdpPort = port;
		ret = true;
	}

#endif//ZQ_OS_MSWIN

	log(ZQ::common::Log::L_NOTICE, CLOGFMT(Subagent, "readUdpConfigure() taking baseUdpPort[%d] timeout[%d]msec"), baseUdpPort, timeoutComm);
	return ret;
}

bool BaseAgent::decodeMessage(const uint8* stream, int len, Msgheader& header, SNMPVariable::List& vlist)
{
	memset(&header, 0, sizeof(header));

	const uint8* p = stream;
	if (NULL == p || len <sizeof(header))
		return false;

	memcpy(&header, p, sizeof(header)), p+= sizeof(header);
	
	// validate the payload length
	if (((size_t)(stream +len -p)) < header.payloadLen)
		return false;

	// validate the checksum
	if (header.checksum != checksum(p, header.payloadLen))
		return false;

	// step. read the variables
	if (SNMPVariable::unserialize(vlist, p, header.payloadLen) <=0)
		return false;

	return true;
}

// size_t BaseAgent::encodeMessage(uint8* stream, size_t maxlen, uint8 pduType, SNMPVariable::List& vlist, int cSeq)
size_t BaseAgent::encodeMessage(uint8* stream, size_t maxlen, Msgheader& header, SNMPVariable::List& vlist)
{
	if (NULL == stream || maxlen < sizeof(Msgheader) || vlist.size() <=0)
		return 0;

	if (0 == header.cseq)
		header.cseq = lastCSeq();

	// the payload of the variable list
	uint8* p = stream +sizeof(Msgheader);
	header.payloadLen = (uint32) SNMPVariable::serialize(vlist, p, stream +maxlen -p);
	if (header.payloadLen <=0)
		return 0;

	// update the checksum
	header.checksum = BaseAgent::checksum(p, header.payloadLen);

	// fill the header
	memcpy(stream, &header, sizeof(header));
	
	return sizeof(Msgheader) + header.payloadLen;
}

void BaseAgent::stop()
{	
	_bQuit = true;
	SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(BaseAgent, "stop() SNMPv3 udp-endpoint[%s/%d]"), _bindAddr.c_str(), _bindPort);
	waitHandle(500);
}

bool BaseAgent::init(void)
{
	SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(BaseAgent, "BaseAgent::init()"));

	if (_bindPort >0)
		_soUdp.bind(ZQ::common::InetHostAddress(_bindAddr.c_str()), _bindPort);

	ZQ::common::Socket::Error se = _soUdp.getErrorNumber();
	if (ZQ::common::Socket::errSuccess != se)
	{
		SALOG(ZQ::common::Log::L_ERROR, CLOGFMT(BaseAgent, "SNMPv3 udp-endpoint failed to bind [%s/%d], error[%d]"), _bindAddr.c_str(), _bindPort, se);
		return false;
	}

	_soUdp.setCompletion(false);
	SALOG(ZQ::common::Log::L_INFO, CLOGFMT(BaseAgent, "SNMPv3 udp-endpoint listens at [%s/%d], timeout[%d]msec"), _bindAddr.c_str(), _bindPort, _timeout);
		
	return true;
}

int BaseAgent::run()
{
	SALOG(ZQ::common::Log::L_INFO, CLOGFMT(BaseAgent, "SNMPv3 udp-endpoint[%s/%d] BaseAgent::run() enter, _bQuit[%d]"), _bindAddr.c_str(), _bindPort, _bQuit);

	while(!_bQuit)
	{
		int64 stampIdleStart = ZQ::common::now();
		for (int nLoop = 1; !_bQuit; ++nLoop)
		{
			if (_soUdp.isPending(ZQ::common::Socket::pendingInput, _timeout))
			{
				if (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_VERBOSE_AGENT)
					SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(BaseAgent, "SNMPv3 udp-endpoint[%s/%d], msg received after idle %dsec"),_bindAddr.c_str(), _bindPort, (int)(ZQ::common::now() -stampIdleStart)/1000);
				break;
			}
		}

		if (_bindPort <=0)
		{
			ZQ::common::tpport_t bport =0;
			_bindAddr = _soUdp.getLocal(&bport).getHostAddress();
			_bindPort = bport;
			SALOG(ZQ::common::Log::L_INFO, CLOGFMT(BaseAgent, "BaseAgent::run() bindPort[%d]"), _bindPort);
		}

		if (_bQuit)
			break;
	
		_log.flush();

		uint8 msg[ZQSNMP_MSG_LEN_MAX];
		ZQ::common::InetHostAddress  peerAddr;
		int peerPort = 0;
		int msglen = _soUdp.receiveFrom((void *)msg, sizeof(msg), peerAddr, peerPort);

		if(msglen <= 0)
			continue;
		
		int respLen = (int)processMessage(peerAddr, peerPort, msg, msglen, sizeof(msg));
		if (respLen <=0)
			continue;

		if (_bQuit)
			break;

		if (_soUdp.sendto(msg, respLen, peerAddr, peerPort) <=0)
		{
			if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
				SALOG(ZQ::common::Log::L_ERROR, CLOGFMT(BaseAgent, "SNMPv3 udp-endpoint[%s/%d] failed to send msg to peer[%s/%d]: %s"), _bindAddr.c_str(), _bindPort, peerAddr.getHostAddress(), peerPort, strerror(errno));
			continue;
		}
	}

	SALOG(ZQ::common::Log::L_INFO, CLOGFMT(BaseAgent, "SNMPv3 udp-endpoint[%s/%d] quits"), _bindAddr.c_str(), _bindPort);
	_log.flush();
	return 0;
}

/*
// -----------------------------
// class Subagent
// -----------------------------
void Subagent::final(void)
{
	SALOG(ZQ::common::Log::L_INFO, CLOGFMT(Subagent, "message processing thread %d is stopped"), NativeThread::id());
}

void Subagent::stop()
{
    if (isRunning())
    {
		SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(Subagent, "stopping the message processing thread %d..."), NativeThread::id());
		_bQuit = true;
		waitHandle(_timeout);
    }
}

/////////////////////////////////////////
bool Subagent::refreshBasePort(void)
{
	return BaseAgent::readUdpConfigure(_log, _snmpUdpBasePort, _timeout);
}


int Subagent::run()
{
	refreshBasePort();
	uint32 componentId = _mmib.componentId();
	uint32 moduleId = _mmib.moduleId();
    // uint32 snmpServerPort = (serviceId_ / 100 ) * 100 + componentId_ + _snmpUdpBasePort + serviceInstanceId;
    uint32 snmpServerPort = _snmpUdpBasePort + componentId + moduleId;

	uint32         msgid = 0;
	unsigned long  lastLossCount   = 0;
	//unsigned long  requestLinePos  = (unsigned long) &((((struct InterActive*) 0)->_content).request);
	unsigned long  requestLinePos  = sizeof(struct InterActiveHead);

	ZQ::common::UDPSocket udpServer(ZQ::common::InetAddress("127.0.0.1"), snmpServerPort);

	ZQ::common::Socket::Error se = udpServer.getErrorNumber();
	if (ZQ::common::Socket::errSuccess != se)
	{
		SALOG(ZQ::common::Log::L_ERROR, CLOGFMT(Subagent, "SNMPv3 udp-endpoint failed to bind port[%d(%d+%d+%d)], error[%d]"), 
			snmpServerPort, _snmpUdpBasePort, componentId, moduleId, se);

		return false;
	}

	udpServer.setCompletion(false);
	SALOG(ZQ::common::Log::L_INFO, CLOGFMT(Subagent, "SNMPv3 udp-endpoint listens at port[%d(%d+%d+%d)], timeout[%d]msec"), 
		snmpServerPort, _snmpUdpBasePort, componentId, moduleId, _timeout);

	while(!_bQuit)
	{
		timeout_t timeout = _timeout;
		for(int nLoop = 1; !_bQuit; ++nLoop)
		{
			if (udpServer.isPending(ZQ::common::Socket::pendingInput, timeout))
			{
				SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(Subagent, "SNMPv3 udp-endpoint[%d], incoming msg detected after idle %dmsec x%d"), snmpServerPort, timeout, nLoop);
				break;
			}
		}

		if (_bQuit)
			break;
		
		ZQ::common::InetHostAddress  udpClientAddress;
		int              peerPort = 0;
		std::string      response;
		InterActive      requestFromAgent = {0};
		int              bytes = udpServer.receiveFrom((void *)&requestFromAgent, sizeof(InterActive), udpClientAddress, peerPort);
		if (bytes <=0)
		{
			SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(Subagent, "SNMPv3 udp-endpoint[%d] failed to receive from client[%s/%d]: msgid[%d] lastLossCount[%d]"), snmpServerPort, udpClientAddress.getHostAddress(), peerPort, msgid, lastLossCount);
			continue;
		}		

		SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(Subagent, "SNMPv3 udp-endpoint[%d] received from client[%s/%d]: response[%d], lastLossCount[%d], agentRecvCount[%d]"), snmpServerPort, udpClientAddress.getHostAddress(), peerPort, msgid, lastLossCount, (requestFromAgent._head._agentRecvSeq));
		bool status = processMessage((requestFromAgent._content.request), bytes - requestLinePos, response);

		if (_bQuit)
			break;

		++msgid;
		if (!status)
		{
			if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
				SALOG(ZQ::common::Log::L_WARNING, CLOGFMT(Subagent, "SNMPv3 udp-endpoint[%d] failed to process msg from client[%s/%d] msgid[%d]"), snmpServerPort, udpClientAddress.getHostAddress(), peerPort, msgid);
			continue;
		}

		if (response.size() > ZQSNMP_MSG_LEN_MAX)
		{
			if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
				SALOG(ZQ::common::Log::L_WARNING, CLOGFMT(Subagent, "response[%u] size[%u] exceeded max len, dropped"), msgid, response.size()); 
			continue;
		}

		// ?????
		if (!udpServer.isPending(ZQ::common::Socket::pendingOutput, timeout))
		{
			SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(Subagent, "processing response[%d] thread[%d], SNMPv3 udp-endpoint[%d], Output timeout[%d]"), msgid, NativeThread::id(), snmpServerPort, timeout);
			continue;
		}

		SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(Subagent, "request[%u] processMessage successfully, sending response %uB to client[%s/%d]"), msgid, response.size(), udpClientAddress.getHostAddress(), peerPort);
		InterActive sendToAgnet = {0};
		sendToAgnet._head._agentRecvSeq     = requestFromAgent._head._agentRecvSeq;
		lastLossCount                       = msgid - requestFromAgent._head._agentRecvSeq;
		sendToAgnet._head._serviceSendSeq   = msgid;
		sendToAgnet._head._lastLossCount    = lastLossCount;
		memcpy((sendToAgnet._content.request), response.data(), response.size());
		int sentState = udpServer.sendto((void *)&sendToAgnet, response.size() + requestLinePos, udpClientAddress, peerPort);
		if (0 >= sentState)
		{
			if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
				SALOG(ZQ::common::Log::L_ERROR, CLOGFMT(Subagent, "SNMPv3 udp-endpoint[%d] failed to send response[%u] to client[%s/%d]: %s"), snmpServerPort, msgid, udpClientAddress.getHostAddress(), peerPort, strerror(errno));
			continue;
		}

		// SALOG(ZQ::common::Log::L_DEBUG, CLOGFMT(Subagent, "client[%s], peerPort[%d]  response[%u] %u bytes send, lastLossCount[%d], agentRecvCount[%d]"), udpClientAddress.getHostAddress(), peerPort, msgid, response.size(), lastLossCount, (sendToAgnet._head._agentRecvSeq));
	}

	SALOG(ZQ::common::Log::L_INFO, CLOGFMT(Subagent, "SNMPv3 udp-endpoint[%d] by thread[%d] quits: msgid[%d], lastLossCount[%d]"), snmpServerPort, NativeThread::id(), msgid, lastLossCount);
	return 0;
}

bool Subagent::decodeMessage(const uint8* stream, int len, SNMPVariable::List& vlist, uint8& pduType, uint32& err)
{
	const uint8* p = stream;

	if (NULL == p || len <2)
		return false;

	// vlist.clear();

#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
	// step.1 validate the checksum
	if ((*p++) != BaseAgent::checksum(stream +1, len -1))
		return false;
#endif // SNMP_VENDOR

	// step.2 read the pduType
	pduType = *p++;
	if ((stream +len -p) <= 0)
		return false;

	// step.3 read the errstat
	err = *((uint32*) p), p+=sizeof(uint32);
	if ((stream +len -p) <= 0)
		return false;

	// step.4 read the variables
	return (SNMPVariable::unserialize(vlist, p, stream +len -p) >0);
}

size_t Subagent::encodeMessage(uint8* stream, size_t maxlen, uint8 pduType, uint32 err, SNMPVariable::List& vlist)
{
	uint8* p = stream;
	size_t n =0;

	if (NULL == p || vlist.size() <=0 || maxlen < 6)
		return 0;

	do {
#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
		p++; // skip the byte0 for the checksum
#endif // SNMP_VENDOR

		*p++ = pduType; // 1-byte PDU
		*((uint32*)p) = err, p+=sizeof(uint32); // 4-byte errstat

		// the variable list
		n = SNMPVariable::serialize(vlist, p, stream +maxlen -p);
		if (n <=0)
			break;
		p += n;

#if (SNMP_VENDOR == SNMP_VENDOR_Microsoft)
		// set the checksum
		*stream = BaseAgent::checksum(stream +1, (int)(p -stream -1));
#endif // SNMP_VENDOR

		return (size_t) (p -stream);
	
	} while(0);

	return 0;
}

bool Subagent::processMessage(const uint8* request, int len, std::string& responseMsg)
{
    if (NULL == request || len <= 0)
        return false;
	
	uint8 pduType = ZQSNMP_PDU_UNKNOWN;
	uint32 err = (uint32) se_NoError;
	SNMPVariable::List vlist;

	if (!decodeMessage(request, len, vlist, pduType, err))
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			SALOG(ZQ::common::Log::L_ERROR, CLOGFMT(Subagent, "decodeRequest() failed, err(%d)"), err);
        return false;
	}

	err = (uint32) se_NoError;
    switch (pduType)
    {
    case ZQSNMP_PDU_GET:
         err = (uint32) _mmib.readVars(vlist);
		 break;

    case ZQSNMP_PDU_GETNEXT:
         err = (uint32) _mmib.nextVar(vlist);
		 break;
        break;

	case ZQSNMP_PDU_SET:
		err = (uint32) _mmib.writeVars(vlist);
		break;

	default:
        // method not support
		err = (uint32) se_GenericError;
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			SALOG(ZQ::common::Log::L_WARNING, CLOGFMT(Subagent, "Unknown pduType[%u]"), pduType);
        break;
    }

	uint8 resp[ZQSNMP_MSG_LEN_MAX];
	memset(resp, 0, sizeof(resp));
	size_t msglen = encodeMessage(resp, ZQSNMP_MSG_LEN_MAX, pduType, err, vlist);
	if (msglen>0)
	{
		responseMsg.assign((const char*)resp, msglen);
		return true;
	}

	if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
		SALOG(ZQ::common::Log::L_ERROR, CLOGFMT(Subagent, "encodeResponse() failed."));
	return false;
}
*/
// -----------------------------
// class SnmpAgent
// -----------------------------
SnmpAgent::SnmpAgent(ZQ::common::Log& log)
: BaseAgent(log)
{
	_bindPort =0;
}

SnmpAgent::~SnmpAgent()
{
	_log(ZQ::common::Log::L_INFO, CLOGFMT(SnmpAgent, "~SnmpAgent()"));

}

size_t SnmpAgent::processMessage(const ZQ::common::InetHostAddress& peerAddr, uint32 peerPort, uint8* msgbuf, int nbyteIn, int maxlen)
{
	Msgheader header;
	SNMPVariable::List vlist;

	if (!decodeMessage(msgbuf, nbyteIn, header, vlist))
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "processMessage() failed to decode message(%d) from peer[%s/%d]"), header.cseq, peerAddr.getHostAddress(), peerPort);
		return 0;
	}

	if (_bQuit)
		return 0;

	OnQueryResult(peerAddr, peerPort, header, vlist);
	return 0; // always no response
}

void SnmpAgent::OnQueryResult(const ZQ::common::InetHostAddress& serverAddr, int serverPort, BaseAgent::Msgheader header, const SNMPVariable::List& vlist)
{
	ZQ::common::MutexGuard g(_awaitLock);
	AwaitMap::iterator it = _awaitMap.find(header.cseq);
	if (_awaitMap.end() == it)
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(SnmpAgent, "OnQueryResult() ignored result(%d) from SubAgent[%s/%d]"), header.cseq, serverAddr.getHostAddress(), serverPort);
		return;
	}
    it->second.header = header;
	it->second.stampResponsed = ZQ::common::now();
	it->second.vlist = vlist;

	if (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_VERBOSE_AGENT)
		_log(ZQ::common::Log::L_INFO, CLOGFMT(SnmpAgent, "OnQueryResult() processed result(%d) from SubAgent[%s/%d] took %dmsec"), header.cseq, serverAddr.getHostAddress(), serverPort, (int)(it->second.stampResponsed - it->second.stampRequested));

	if (NULL == it->second.pEvent)
		return;

	it->second.pEvent->signal();
}

// return cSeq
uint32 SnmpAgent::sendQuery(const ZQ::common::InetHostAddress& serverAddr, int serverPort, uint8 pdu, SNMPVariable::List& vlist, ZQ::common::Event::Ptr eventArrived)
{
	Query aq;
	memset(&aq.header, 0, sizeof(aq.header));
	aq.header.pdu = pdu;
	aq.header.cseq = lastCSeq();
	aq.stampRequested = ZQ::common::now();
	aq.pEvent = eventArrived;

	uint8 msg[ZQSNMP_MSG_LEN_MAX];
	memset(msg, 0, sizeof(msg));

	// step 1. compose the request
	size_t msglen = encodeMessage(msg, sizeof(msg), aq.header, vlist);
	if (msglen <=0)
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "sendQuery() failed to compose out-going request"));
		return 0;
	}

	if (!_soUdp.isPending(ZQ::common::Socket::pendingOutput, _timeout))
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "sendQuery() udp timed out[%d] prior to sending"), _timeout);
		return 0;
	}

	ZQ::common::MutexGuard g(_awaitLock);
	_awaitMap[aq.header.cseq] = aq;

	if (_soUdp.sendto(msg, msglen, serverAddr, serverPort) <=0)
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "sendQuery() failed send to SubAgent[%s/%d] len[%d]"), serverAddr.getHostAddress(), serverPort, msglen);
		return 0;
	}

/*	char buf[1024];
	memset(buf , 0, 1024);

	 ZQ::common::InetHostAddress tempAddr;
	 int port;

	 int msgLength = _soUdp.receiveFrom(buf, 1024, tempAddr, port);

	 _log(ZQ::common::Log::L_ERROR, CLOGFMT(SnmpAgent, "receiveFrom() failed receiver[%d]"),msgLength);
*/
	if (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_VERBOSE_AGENT)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SnmpAgent, "sendQuery() query(%d) sent to SubAgent[%s/%d] msglen[%d]"), aq.header.cseq, serverAddr.getHostAddress(), serverPort, msglen);
	return aq.header.cseq;
}

bool SnmpAgent::getResponse(uint32 cSeq, Query& query)
{
	ZQ::common::MutexGuard g(_awaitLock);
	AwaitMap::iterator it = _awaitMap.find(cSeq);
	if (_awaitMap.end() == it)
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(SnmpAgent, "getResponse() query[%d] is not in the await list"), cSeq);
		return false;
	}

	query = it->second;
	return true;
}

// -----------------------------
// class SubAgent
// -----------------------------
SubAgent::SubAgent(ZQ::common::Log& log, ModuleMIB& mmib, timeout_t timeout)
: BaseAgent(log), _mmib(mmib)
{
	_bindPort = portOfModule(_mmib.componentId(), _mmib.moduleId());
}


size_t SubAgent::processMessage(const ZQ::common::InetHostAddress& peerAddr, uint32 peerPort, uint8* msgbuf, int nbyteIn, int maxlen)
{
	Msgheader header;
	SNMPVariable::List vlist;

	if (!decodeMessage(msgbuf, nbyteIn, header, vlist))
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SubAgent, "processMessage() failed to decode message(%d) from peer[%s/%d]"), header.cseq, peerAddr.getHostAddress(), peerPort);
		return 0;
	}

	header.error = (uint32) se_NoError;
	switch (header.pdu)
    {
    case ZQSNMP_PDU_GET:
         header.error = (uint32) _mmib.readVars(vlist);
		 break;

    case ZQSNMP_PDU_GETNEXT:
         header.error = (uint32) _mmib.nextVar(vlist);
        break;

	case ZQSNMP_PDU_SET:
		header.error = (uint32) _mmib.writeVars(vlist);
		break;

	case ZQSNMP_PDU_GETJSON:
		header.error = (uint32) _mmib.vars2Json(vlist);
		// TODO: index in the MIB table, then format the output in JSON
		// step 1. search the MIB table for the SNMPVar
		// step 2. format SNMPVar to JSON as a SNMPVariable(type=string)
		// step 3. vlist.push_back(jsonstr)
		break;

	default:
        // method not support
		header.error = (uint32) se_GenericError;
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			SALOG(ZQ::common::Log::L_WARNING, CLOGFMT(SubAgent, "processMessage() unknown pduType[%u]"), header.pdu);
        break;
    }

	// step x. compose the response
	size_t msglen = encodeMessage(msgbuf, maxlen, header, vlist);
	if (msglen <=0)
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SubAgent, "processMessage() failed to compose response(%d)"), header.cseq);
		return 0;
	}

	if (!_soUdp.isPending(ZQ::common::Socket::pendingOutput, _timeout))
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SubAgent, "processMessage() udp timedout[%d] prior to sending"), _timeout);
		return 0;
	}

	if (_soUdp.sendto(msgbuf, msglen, peerAddr, peerPort) <=0)
	{
		if (0 == (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_MUTE_ERRS_AGENT))
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(SubAgent, "processMessage() failed to send resp(%d) to Agent[%s/%d] len[%d]"), header.cseq, peerAddr.getHostAddress(), peerPort, msglen);
		return 0;
	}

	if (ModuleMIB::_flags_VERBOSE & ModuleMIB::VFLG_VERBOSE_AGENT)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SubAgent, "processMessage() resp(%d) sent to Agent[%s/%d] len[%d]"), header.cseq, peerAddr.getHostAddress(), peerPort, msglen);

	return msglen;
}

}} // namespace ZQ::SNMP
