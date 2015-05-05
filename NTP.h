#ifndef __ZQ_COM_NTP_H__
#define __ZQ_COM_NTP_H__

#include "ZQ_common_conf.h"
#include "UDPSocket.h"
#include "TimeUtil.h"
#include "NativeThread.h"
#include "Log.h"


#ifdef ZQ_OS_LINUX 
extern "C"
{
#include <sys/time.h>
#include <arpa/inet.h>
}
#endif

#define DEFAULT_PORT_NTP  (123)
#define NTP_100_NSEC  10000000UL  //100 nanosecond
#define NTP_JAN_1970   2208988800UL	/* 1970 - 1900 in seconds */
#define NTP_FRAC	   4294967296. /* 2^32 as a double */ 


namespace ZQ {
namespace common {

class ZQ_COMMON_API NTPServer;
class ZQ_COMMON_API NTPClient;

// -----------------------------
// class NTPServer
// -----------------------------
class NTPServer : public ZQ::common::NativeThread, public UDPReceive
{
public:
	///constructor
	///@param Pool : thread pool size
	///@param strBindAddr : local address bind with this service, default is 0.0.0.0 which mean any valid address
	///@param sBindPort : local port bind with this service, default is 1230
	NTPServer(ZQ::common::Log* log, const InetAddress &bind, tpport_t sport =123);
	virtual ~NTPServer();

	// NTP timestamps are represented as a 64-bit fixed-point number, in seconds relative to 0000 UT
	// on 1 January 1900.  The integer part is in the first 32 bits and the fraction part in the last
	// 32 bits
	typedef struct _TimeStamp
	{
 		uint32 dwIntSec;  // NTP fixed point Interger part
 		uint32 dwFracSec; // NTP fixed point fraction part
	} TimeStamp;

	typedef struct _Packet
	{
		int32 Control_Word;
		int32 root_delay;
		int32 root_dispersion;
		int32 reference_identifier;

		// stampReference is established by the server or client host as the timestamp (presumably obtained
		// from a reference clock) most recently used to update the local clock.
		// If the local clock has never been synchronized, the value is zero.
		TimeStamp stampReference; 

		// stampOriginate is established by the client host and specifying the local time at which the request
		// departed for the service host.
		TimeStamp stampOriginate;

		// stampReceive is established by the server host and specifying the local time at which the request
		// arrived from the client host.  If no request has ever arrived from the client the value is zero.
		TimeStamp stampReceive;

		// stampTransmit is established by the server host and specifying the local time at which the reply
		// departed for the client host.  If no request has ever arrived from the client the value is zero.
		TimeStamp stampTransmit;

	} Packet;

	void stop();

protected:

	/// accept ntp request and send ntp reply
	///@return the return value will also be passed as the thread exit code
	virtual int run();
public:
	ZQ::common::Log *_log;
	bool _bQuit;

};

// -----------------------------
// class NTPClient
// -----------------------------
class NTPClient : public UDPSocket
{
public:
	/// Create a UDP socket and bind it to a specific interface and port
	/// address so that other UDP sockets may find and send UDP messages
	/// to it. On failure to bind, an exception is thrown.
	/// @param bind address to bind this socket to.
	/// @param port number to bind this socket to.
	NTPClient(ZQ::common::Log* log, const InetAddress &bind, tpport_t bindport=DEFAULT_PORT_NTP,int32 timeout=5000,int version=3);

	/// destructor
	virtual ~NTPClient();

	typedef struct _Txn
	{
		NTPServer::Packet packet;
		NTPServer::TimeStamp stampRespArrive;
	} Txn;

	// query server for the time, value is int64 that refers to ZQ::common::TimeUtils::now() 
	int64 getServerTime(Txn& txn, const InetAddress server, int port=DEFAULT_PORT_NTP);
	//bool getServerTime();
	// set the timeout of the client, in msec
	void setTimeout(int32 timeout=5000);

	// Sets the NTP protocol version number that client sets on request packet communicate with remote host.
	void setVersion(int version);

	// Returns the NTP protocol version number that client sets on request packet that is sent to remote host
	int	getVersion() { return _version; }
	//set the client ntp packet vession
	void setNTPVersion(uint32& control_word);
	//return true if have some error,else return false
	bool checkPacketError(NTPServer::Packet& ntpReply);
	//receive  NTP packet from server 
	int receiveTimeout(void *buf, size_t len, uint32 dwTimeout);

protected: // member variables

	int   _version;
	int32 _timeout;

public: // ntp time conversion
	// convert NTP_Time to int64 that refers to ZQ::common::TimeUtils::now()
	static bool ntp2time(const NTPServer::TimeStamp nt, uint64& time64);

	// convert to NTP_time from int64 that refers to ZQ::common::TimeUtils::now()
	static bool time2ntp(uint64 time64, NTPServer::TimeStamp& nt);

public: // public utility funcs of Txn computing

	// read the round-trip network delay in msec
	static int32 readDelay(Txn& txn);
	//int64 readDelay();

	// read clock offset, in msec, needed to adjust local clock to match remote clock.
	static int32 readOffset(Txn& txn);
	//int64 readOffset();

	// read time at which time message packet was received by local machine.
	static int64 readReturnTime(Txn& txn);
	//int64 readReturnTime(){return _T4;}
      //set the time of local machine
	//void adjustClientClock(int64 nOffset);
private:
	ZQ::common::Log *_log;
	InetAddress  _serveAddress;
      int  _serverPort;
};
/* sample use
NTPClient client(logger, localIP);
NTPClient::Txn txn;
memset(&txn, 0, sizeof(txn));
if (client.getServerTime(txn, "ntp.google.com") >0)
	timediff = NTPClient::readOffset(txn);
else (client.getServerTime(txn, "ntp2.google.com") >0)
	timediff = NTPClient::readOffset(txn);

...
*/

// -----------------------------
// class ClockAdjuster
// -----------------------------
class ClockAdjuster
{
};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COM_NTP_H__
