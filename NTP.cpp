#include "NTP.h"

namespace ZQ {
namespace common {

// -----------------------------
// class NTPServer
// -----------------------------
NTPServer::NTPServer(ZQ::common::Log& log, const InetAddress &bind, tpport_t sport /* =123 */)
	:_log(log),_bQuit(false),UDPReceive(bind,sport)
{
	_log(ZQ::common::Log::L_INFO,CLOGFMT(NTPServer, "NTPServer() start NTP Server..."));
}

NTPServer::~NTPServer()
{
	_log(ZQ::common::Log::L_INFO,CLOGFMT(NTPServer, "~NTPServer() Quiting NTP Server..."));
}
void NTPServer::stop()
{
	_bQuit=true;
}

int NTPServer::run()
{
  if(INVALID_SOCKET == get())
  {
	_log(ZQ::common::Log::L_INFO,CLOGFMT(NTPServer, "NTPServer() Fail to start NTP Server..."));
	_bQuit=true;
  }

  while(!_bQuit)
  {
	Packet ntp_ServerPacket;
	ZQ::common::InetHostAddress cAddress;
	int cPort;
	memset(&ntp_ServerPacket, 0, sizeof(ntp_ServerPacket));
	int nReceive=receiveFrom(&ntp_ServerPacket,sizeof(ntp_ServerPacket),cAddress,cPort);
	if (nReceive != sizeof(ntp_ServerPacket))
	{
		if (_bQuit)
			break;

#ifdef ZQ_OS_MSWIN
		if (SOCKET_ERROR == nReceive )
			_log(ZQ::common::Log::L_ERROR,CLOGFMT(NTPServer,"run() receiveFrom client error"));
#else
		if (nReceive < 0)
			_log(ZQ::common::Log::L_ERROR,CLOGFMT(NTPServer,"run() receiveFrom client error"));
#endif
		continue;
	}

	TimeStamp pServerTime;
	int64 intServerTime=ZQ::common::TimeUtil::now();
	NTPClient::time2ntp(intServerTime*10000,pServerTime);
	int32 nControlWord = 0;
	nControlWord = ntohl(ntp_ServerPacket.Control_Word) & (0x3800FF00);
	ntp_ServerPacket.Control_Word = htonl(nControlWord | 0x04010090);
	ntp_ServerPacket.root_delay = htonl(0x00000000); // root_delay = 0
	ntp_ServerPacket.root_dispersion = htonl(0x00000000); // root_diespersion = 0
	ntp_ServerPacket.reference_identifier = htonl(0x47505300); // GPS = 0x47505300
	ntp_ServerPacket.stampReceive= pServerTime;
	intServerTime=ZQ::common::TimeUtil::now();
	NTPClient::time2ntp(intServerTime*10000,pServerTime);
	ntp_ServerPacket.stampTransmit=pServerTime;
	//send response

	setPeer(cAddress,cPort);

	int nSend=send(&ntp_ServerPacket,sizeof(ntp_ServerPacket));
	if (nSend != sizeof(ntp_ServerPacket))
	{
		if (_bQuit)
			break;

		_log(Log::L_ERROR, CLOGFMT(NTPServer, "run() Fail to send ntp reply to address [%s] at port [%d]"),cAddress.getHostAddress(), cPort);
		continue;
	}

	_log(Log::L_INFO, CLOGFMT(NTPServer, "run() Success to send ntp reply to address [%s] at port [%d]"), cAddress.getHostAddress(), cPort);
  }

  return 0;
}

// -----------------------------
// class NTPClient
// -----------------------------
NTPClient::NTPClient(ZQ::common::Log& log, const InetAddress &bind, tpport_t bindport/* =DEFAULT_PORT_NTP */,int32 timeout/*=5000*/,int version/*=3*/)
:_log(log),_serveAddress(bind),_serverPort(bindport),_timeout(timeout),_version(version)
{
}

NTPClient::~NTPClient()
{
}

int64 NTPClient::getServerTime(Txn& txn, const InetAddress server, int port/*=DEFAULT_PORT_NTP*/)
{
	NTPServer::Packet _ntpClientPacket;
	memset(&_ntpClientPacket,0,sizeof(_ntpClientPacket));
	uint32 control_word=0x0B000000;
	setNTPVersion(control_word);
	_ntpClientPacket.Control_Word = htonl(control_word);
	NTPServer::TimeStamp clientOriginateTime;
	int64 intClientOriginateTime=ZQ::common::TimeUtil::now();
	time2ntp(intClientOriginateTime*10000,clientOriginateTime);
	_ntpClientPacket.stampOriginate=clientOriginateTime;
	setPeer(server,port);

	int nSend = send(&_ntpClientPacket,sizeof(_ntpClientPacket));
	if (nSend != sizeof(_ntpClientPacket))
	{
		_log(ZQ::common::Log::L_ERROR,CLOGFMT(NTPClient, "getServerTime() send the data error"));
		return -1;
	}

	int nReceive = receiveTimeout(&_ntpClientPacket,sizeof(_ntpClientPacket),_timeout);
	if (nReceive != sizeof(_ntpClientPacket))
	{
		_log(ZQ::common::Log::L_ERROR,CLOGFMT(NTPClient, "getServerTime() receive the data error [%d]"),nReceive);
		return -1;
	}

	if (checkPacketError(_ntpClientPacket))
	{
		_log(ZQ::common::Log::L_ERROR,CLOGFMT(NTPClient, "getServerTime() receive the data error"));
		//_T1=_T2=_T3=_T4=0;
		return -1;
	}

	NTPServer::TimeStamp clientRespArrive;
	int64 intclientRespArrive=ZQ::common::TimeUtil::now();
	NTPClient::time2ntp(intclientRespArrive*10000,clientRespArrive);
	_ntpClientPacket.stampOriginate=clientOriginateTime;
	txn.packet=_ntpClientPacket;
	txn.stampRespArrive=clientRespArrive;
	uint64 T5;
	NTPClient::ntp2time(_ntpClientPacket.stampTransmit,T5);
	return T5;
}

int NTPClient::receiveTimeout(void *buf, size_t len, uint32 dwTimeout)
{
	SOCKET _so=get();
	fd_set setCheck;
	timeval timeout;
	timeout.tv_sec = dwTimeout/1000; 
	timeout.tv_usec = dwTimeout%1000;
	FD_ZERO(&setCheck);
	FD_SET( _so, &setCheck );
	select(_so+1, &setCheck, 0, 0, &timeout);
	if(FD_ISSET(_so,&setCheck))
		return receive(buf,len);

	return 0;
}

bool NTPClient::checkPacketError(NTPServer::Packet& ntpReply)
{
	int32 nControlWord = ntohl(ntpReply.Control_Word);
	// check LI field
	int32 nLI = (nControlWord >> 30) & 0x00000003;
	if (3 == nLI)
		return true;

	// check stratum field
	int32 nStratum = (nControlWord >> 16) & 0x000000FF;
	if (nStratum < 1 || nStratum >15)
		return true;

	NTPServer::TimeStamp transmitTime;
	transmitTime.dwIntSec = ntohl(ntpReply.stampTransmit.dwIntSec);
	transmitTime.dwFracSec = ntohl(ntpReply.stampTransmit.dwFracSec);
	if (0 == transmitTime.dwIntSec && 0 == transmitTime.dwFracSec)
		return true;

	return false;
}

bool NTPClient::ntp2time(const NTPServer::TimeStamp nt, uint64& time64)
{
#ifdef ZQ_OS_MSWIN
	double dTimeStamp = (double) ntohl(nt.dwIntSec);
	dTimeStamp += ((double)ntohl(nt.dwFracSec))/(double)NTP_FRAC ;	/* 2^32 as a double */
	dTimeStamp *= NTP_100_NSEC;
	ULONGLONG T0 = (ULONGLONG) dTimeStamp;
	FILETIME ft2;
	SYSTEMTIME st;
	st.wYear = 1900;
	st.wMonth = 1;
	st.wDayOfWeek = 0;
	st.wDay = 1;
	st.wHour = 0;
	st.wMinute = 0;
	st.wSecond = 0;
	st.wMilliseconds = 0;
	SystemTimeToFileTime(&st, &ft2);  // ft is the file time for Jan 1 1900.
	ULONGLONG T1;
	T1 = (((ULONGLONG) ft2.dwHighDateTime) << 32) + ft2.dwLowDateTime;
	T0 += T1; // adjust the offset.
	time64=T0;
#else
	uint32 ntpsec = ntohl(nt.dwIntSec);
	uint32 ntpfrac = ntohl(nt.dwFracSec);
	//time64 =((uint64)(ntpsec<<32))+ntpfrac;
	ntpfrac=(ntpfrac/NTP_FRAC)*NTP_100_NSEC;
	time64=ntpsec*NTP_100_NSEC+ntpfrac;
	time64 -=NTP_JAN_1970*NTP_100_NSEC;
#endif

	return true;
}

bool NTPClient::time2ntp(uint64 time64, NTPServer::TimeStamp& nt)
{
#ifdef ZQ_OS_MSWIN
	FILETIME ft2;
	SYSTEMTIME st;
	st.wYear = 1900;
	st.wMonth = 1;
	st.wDayOfWeek = 0;
	st.wDay = 1;
	st.wHour = 0;
	st.wMinute = 0;
	st.wSecond = 0;
	st.wMilliseconds = 0;
	SystemTimeToFileTime(&st, &ft2);  // ft is the file time for Jan 1 1900.
	ULONGLONG T1;
	T1 = (((ULONGLONG) ft2.dwHighDateTime) << 32) + ft2.dwLowDateTime;
	time64-=T1;
#else
	time64 += NTP_JAN_1970*NTP_100_NSEC;
#endif

	double d1 = (double)( time64/NTP_100_NSEC); // get the integer part.
	double d2 = (double)(time64%NTP_100_NSEC);  // get the fraction.
	double d4 = (d2/(double)NTP_100_NSEC) * (double)NTP_FRAC;
	nt.dwIntSec = htonl((uint32)d1);
	nt.dwFracSec = htonl((uint32)d4);

	return true;
}

void NTPClient::setTimeout(int32 timeout/* =5000 */)
{
	_timeout=timeout;
}

void NTPClient::setVersion(int version)
{
	_version=version;
}

void NTPClient::setNTPVersion(uint32& control_word)
{
	uint32 temp=_version;
	control_word = control_word & 0xC7FFFFFF;
	for (int i=4;i<31;i++)
		temp=temp << 1;

	control_word = control_word | temp;
}

int32 NTPClient::readDelay(Txn& txn)
{
	uint64 T1=0,T2=0,T3=0,T4=0;
	NTPClient::ntp2time(txn.packet.stampOriginate,T1);
	NTPClient::ntp2time(txn.packet.stampReceive,T2);
	NTPClient::ntp2time(txn.packet.stampTransmit,T3);
	NTPClient::ntp2time(txn.stampRespArrive,T4);
	int64 delayTime = (T4 - T1) - (T3 - T2);

	return (int32)(delayTime/10000);
}

int32 NTPClient::readOffset(Txn& txn)
{
	uint64 T1=0,T2=0,T3=0,T4=0;
	NTPClient::ntp2time(txn.packet.stampOriginate,T1);
	NTPClient::ntp2time(txn.packet.stampReceive,T2);
	NTPClient::ntp2time(txn.packet.stampTransmit,T3);
	NTPClient::ntp2time(txn.stampRespArrive,T4);
	int64 tx = T2 - T1;
	int64 ty = T3 - T4;
	int64 nOffset = (tx + ty) /2;

	return (int32) (nOffset /10000);
}

int64 NTPClient::readReturnTime(Txn& txn)
{
	uint64 T=0;
	NTPClient::ntp2time(txn.stampRespArrive,T);
	return T;
}

}} //namespaces
