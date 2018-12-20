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
// Ident : $Id: RedisClient.h,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define RedisClient class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/RedisClient.cpp $
// 
// 7     11/30/15 4:58p Hui.shao
// compile warning
// 
// 6     6/10/15 1:53p Hui.shao
// 
// 5     5/20/15 11:17a Hui.shao
// added DEL/KEYS
// 
// 4     1/22/15 5:06p Hui.shao
// tested 1million multibulks
// 
// 3     1/21/15 5:57p Hui.shao
// tested sadd/srem/smembers
// 
// 2     1/19/15 3:45p Hui.shao
// tested messaging
// 
// 1     1/16/15 2:15p Hui.shao
// created
// ===========================================================================

#include "RedisClient.h"
#include "SystemUtils.h"
#include "CryptoAlgm.h"

#include "libbz/bzlib.h"
#ifndef LIBSUFFIX
#  if defined(DEBUG) || defined(_DEBUG)
#  define LIBSUFFIX "d"
#  else
#  define LIBSUFFIX ""
#  endif
#endif
#pragma comment(lib, "libbz2" LIBSUFFIX)

#define REDIS_NEWLINE              "\r\n"
#define REDIS_LEADINGCH_ERROR      '-'
#define REDIS_LEADINGCH_INLINE     '+'
#define REDIS_LEADINGCH_BULK       '$'
#define REDIS_LEADINGCH_MULTIBULK  '*'
#define REDIS_LEADINGCH_INT        ':'

namespace ZQ {
namespace common {

const char* RedisSink::err2str(Error err)
{
#define CASE_ERR(_ERR)	case rde##_ERR: return #_ERR
	switch(err)
	{
		CASE_ERR(OK);
		CASE_ERR(ClientError);
		CASE_ERR(Nil);
		CASE_ERR(ConnectError);
		CASE_ERR(SendError);
		CASE_ERR(RequestTimeout);
		CASE_ERR(ClientCanceled);
		CASE_ERR(ServerReturnedError);
		CASE_ERR(ProtocolMissed);
	}
#undef CASE_ERR
	return "Unknown";
}

// -----------------------------
// class RedisCommand
// -----------------------------
RedisCommand::RedisCommand(RedisClient& client, const std::string& cmdstr, char replyType, RedisSink::Ptr cbRedirect)
: _client(client), _replyType(replyType), _cbRedirect(cbRedirect)
{
	_stampCreated = ZQ::common::now();
	_stampSent = _stampReceived =0;

	// TODO: fixup the command line
	_command = cmdstr;

	// initialize the reply ctx
	_replyCtx.data.integer = _replyCtx.data.type = 0x00;
	_replyCtx.nBulksLeft =0;
	_replyCtx.isBulkPayload = false;

	if (!_cbRedirect)
	{
		// instance an Event for sync-call
		_pEvent = new Event();
	}
}

bool RedisCommand::wait(int timeout)
{
	if (_stampReceived>0)
		return true;

	if (!_pEvent)
		return false;

	return _pEvent->wait(timeout);
}

void RedisCommand::OnRequestError(RedisClient& client, RedisCommand& cmd, Error errCode, const char* errDesc)
{
	if (_cbRedirect)
		return _cbRedirect->OnRequestError(client, cmd, errCode, errDesc);

	if (_pEvent)
		_pEvent->signal();
}

void RedisCommand::OnReply(RedisClient& client, RedisCommand& cmd, Data& data)
{
	if (_cbRedirect)
		return _cbRedirect->OnReply(client, cmd, data);

	if (_pEvent)
		_pEvent->signal();
}

std::string RedisCommand::desc()
{
	return _command;
}

// -----------------------------
// class RequestErrCmd
// -----------------------------
class RequestErrCmd : public ThreadRequest
{
public:
	RequestErrCmd(RedisClient& client, const RedisCommand::Ptr& pCmd, RedisSink::Error errCode)
		: ThreadRequest(client._thrdpool), _client(client), _pCmd(pCmd), _errCode(errCode)
	{
	}

protected:
	RedisClient&      _client;
	RedisCommand::Ptr _pCmd;
	RedisSink::Error   _errCode;

	virtual int run()
	{
		try {
			if (_pCmd)
				_pCmd->OnRequestError(_client, *_pCmd.get(), _errCode);
		}
		catch(...) {}

		return 0;
	}

	void final(int retcode =0, bool bCancelled =false)
	{
		delete this;
	}
};

// -----------------------------
// class ReplyDispatcher
// -----------------------------
class ReplyDispatcher : public ThreadRequest
{
public:
	ReplyDispatcher(RedisClient& client, const RedisCommand::Ptr& pCmd)
		: ThreadRequest(client._thrdpool), _client(client), _pCmd(pCmd)
	{
	}

protected:
	RedisClient&      _client;
	RedisCommand::Ptr _pCmd;

	virtual int run()
	{
		try {
			if (_pCmd)
				_pCmd->OnReply(_client, *_pCmd.get(), _pCmd->_replyCtx.data);
		}
		catch(...) {}

		return 0;
	}

	void final(int retcode =0, bool bCancelled =false)
	{
		delete this;
	}
};

// -----------------------------
// class RedisEvictor
// -----------------------------
//class EvictorPoller : public ThreadRequest
//{
//public:
//	EvictorPoller(RedisEvictor& evictor, NativeThreadPool& pool)
//		: ThreadRequest(pool), _evictor(evictor)
//	{
//	}
//
//	RedisEvictor& _evictor;
//
//	virtual int run()
//	{
//		_evictor.poll();
//		_evictor.poll();
//
//		return 0;
//	}
//
//	void final(int retcode =0, bool bCancelled =false) { delete this; }
//};

RedisEvictor::RedisEvictor(Log& log, RedisClient::Ptr client, const std::string& name, const Properties& props)
: _client(client), Evictor(log, name, props), _maxValueLen(REDIS_RECV_BUF_SIZE), _recvBuf(NULL)
{ 
	_recvBuf = new uint8[_maxValueLen];
}

RedisEvictor::~RedisEvictor()
{
	if (_recvBuf)
		delete[] _recvBuf;

	_recvBuf = NULL;
}

// save a batch of streamed object to the target object store
int RedisEvictor::saveBatchToStore(StreamedList& batch)
{
	if (!_client)
		return 0;

	int cUpdated =0, cDeleted =0;
	RedisSink::Error rcerr;
	for (StreamedList::iterator it = batch.begin(); it!=batch.end(); it++)
	{
		switch(it->status)
		{
        case Item::created:   // the item is created in the cache, and has not present in the ObjectStore
		case Item::modified:  // the item is modified but not yet flushed to the ObjectStore
			rcerr = _client->SET(it->key, (uint8*)(&it->data[0]), it->data.size());
			if (RedisSink::rdeOK != rcerr)
				Evictor::_log(Log::L_ERROR, CLOGFMT(RedisEvictor, "saveBatchToStore() save[%s] err: %s(%d)"), it->key.c_str(), RedisSink::err2str(rcerr), rcerr);
			else cUpdated++;
			break;

        case Item::destroyed:  // the item is required to destroy but not yet deleted from ObjectStore
            rcerr = _client->DEL(it->key);
			if (RedisSink::rdeOK != rcerr)
				Evictor::_log(Log::L_ERROR, CLOGFMT(RedisEvictor, "saveBatchToStore() del[%s] err: %s(%d)"), it->key.c_str(), RedisSink::err2str(rcerr), rcerr);
			else cDeleted++;
            break;

        default:
            break;
        }
    }

    Evictor::_log(Log::L_DEBUG, CLOGFMT(indexEvictor, "saveBatchToStore() %d updated and %d destroyed"), cUpdated, cDeleted);
    return cUpdated+cDeleted;
}

// load a specified object from the object store
//@ return IOError, NotFound, OK
Evictor::Error RedisEvictor::loadFromStore(const std::string& key, StreamedObject& data)
{
	if (!_client)
		return eeConnectErr;

	uint vlen = _maxValueLen;
	if (NULL == _recvBuf || vlen <=0)
		return eeNoMemory;

	 RedisSink::Error rcerr = _client->GET(key, _recvBuf, vlen);
     switch (rcerr)
     {
     case RedisSink::rdeOK:
         break;

     case RedisSink::rdeNil:
         return eeNotFound;

     case RedisSink::rdeConnectError:
     case RedisSink::rdeSendError:
     case RedisSink::rdeClientError:
     case RedisSink::rdeServerReturnedError:
     case RedisSink::rdeProtocolMissed:
     case RedisSink::rdeClientCanceled:
         return eeConnectErr;

     case RedisSink::rdeRequestTimeout:
         return eeTimeout;
     }

	data.data.assign(_recvBuf, _recvBuf + vlen);
	data.stampAsOf = ZQ::common::now();

	Evictor::_log(Log::L_DEBUG, CLOGFMT(RedisEvictor, "loadFromStore() loaded obj[%s] datasize[%d]"), key.c_str(), vlen);
	return eeOK;
}

// -----------------------------
// class RedisClient
// -----------------------------
// represent a tcp connection to the RTSP server, may be shared by multiple RTSPSessions
RedisClient::RedisClient(Log& log, NativeThreadPool& thrdpool, const std::string& server, tpport_t serverPort, const InetHostAddress& bindAddress, Log::loglevel_t verbosityLevel, tpport_t bindPort)
: TCPClient(bindAddress, bindPort), _serverPort(serverPort), _thrdpool(thrdpool), _log(log, verbosityLevel), _verboseFlags(0), _lastErr(RedisSink::rdeOK),
_inCommingByteSeen(0) 
{
	setClientTimeout(DEFAULT_CONNECT_TIMEOUT, DEFAULT_CLIENT_TIMEOUT);
	do {
		// step 1. parse the URL for the IP and port
		if (!_serverAddress.setAddress(server.c_str()))
			break;

		if (_serverPort <=0)
			_serverPort = REDIS_DEFAULT_PORT;

		setPeer(_serverAddress, _serverPort);
		sendPING();

	} while(0);
}

RedisClient::~RedisClient()
{
}

void RedisClient::setClientTimeout(int32 connectTimeout, int32 messageTimeout)
{
	_connectTimeout = connectTimeout;
	_messageTimeout = messageTimeout;

	if (_messageTimeout <=2) // ensure the value divided by 2 is still valid
		_messageTimeout = DEFAULT_CLIENT_TIMEOUT;

	if (_connectTimeout <=0)
		_connectTimeout = DEFAULT_CLIENT_TIMEOUT;

	TCPSocket::setTimeout(_messageTimeout >>1); // half of _messageTimeout to wake up the socket sleep()
}

// general command entry
// reply=NULL indicates sync-ed command to Redis
RedisCommand::Ptr RedisClient::sendCommand(const std::string& command, char replyType, RedisSink::Ptr cbReply)
{
	if (command.empty())
		return NULL;

	return sendCommand(new RedisCommand(*this, command, replyType, cbReply));
}

RedisCommand::Ptr RedisClient::sendCommand(RedisCommand::Ptr pCmd)
{
	if (!pCmd)
	{
		_log(Log::L_ERROR, CLOGFMT(RedisClient, "sendCommand() failed to instance RedisCommand"));
		return NULL;
	}

	std::string cmddesc = pCmd->desc();
	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "sendCommand() so[%d] cmd len[%d]"), (int) TCPSocket::get(), cmddesc.length());
	_lastErr = RedisSink::rdeSendError;
	// std::string lastErr;

	do // only one round of this loop, program can quit by either "break" or "continue"
	{
		bool connectionIsPending = false;
		{
			MutexGuard g(_lockCommandQueue);

			if (_commandQueueToSend.size() >0 || (Socket::stConnecting == TCPClient::state()))
			{
				// a connection is currently pending with at least one queued request.
				connectionIsPending = true;
			}
			else if (_so < 0 || _so == INVALID_SOCKET)
			{ 
				// need to open a connection
				if (!TCPClient::connect(_connectTimeout))
				{
					_lastErr = RedisSink::rdeConnectError;
					_log(Log::L_ERROR, CLOGFMT(RedisClient, "sendCommand() cmd[%s] error occured at connect()"), cmddesc.c_str());
					break; // an error occurred
				}

				connectionIsPending = (Socket::stConnecting == TCPClient::state());
			}

			if (connectionIsPending)
			{
				_log(Log::L_DEBUG, CLOGFMT(RedisClient, "sendCommand() cmd[%s] connect in progress, wait for next try"), cmddesc.c_str());
				_commandQueueToSend.push(pCmd);

				// give up those expired requests in the queue and prevent this queue from growing too big
				int64 stampExp = ZQ::common::now() - _timeout;
				while (!_commandQueueToSend.empty())
				{
					RedisCommand::Ptr pCmd = _commandQueueToSend.front();

					if (pCmd && (_timeout <=0 || pCmd->_stampCreated > stampExp) && _commandQueueToSend.size() < MAX_PENDING_REQUESTS)
						break;

					_commandQueueToSend.pop();

					if (!pCmd)
						continue;

					try {
						(new RequestErrCmd(*this, pCmd, RedisSink::rdeConnectError))->start();
					}
					catch(...) {}
				}

				return pCmd;
			}
		}

		// The client is currently connected, send the command instantly
		size_t awaitsize = 0, poolsize = _thrdpool.size();
		int64 stampNow = TimeUtil::now();

		if (_messageTimeout >0 && (stampNow - pCmd->_stampCreated) > _messageTimeout)
		{
			_log(Log::L_ERROR, CLOGFMT(RedisClient, "sendCommand() %s timeout prior to sending"), cmddesc.c_str());
			_lastErr = RedisSink::rdeRequestTimeout;
			break;
		}

		MutexGuard g(_lockCommandQueue);
		int ret = _sendLine(pCmd->_command);
		if (ret < 0)
		{
			_lastErr = RedisSink::rdeSendError;
			_log(Log::L_ERROR, CLOGFMT(RedisClient, "failed at _sendLine() %s, ret=%d"), cmddesc.c_str(),  ret);
			break;
		}

		pCmd->_stampSent = stampNow = TimeUtil::now();
		long sendLatency = (long) (stampNow - pCmd->_stampCreated);

		if (_messageTimeout >20 && sendLatency > _messageTimeout/4)
			_log(Log::L_WARNING, CLOGFMT(RedisClient, "sendCommand() %s took %ldmsec, too long until sent"), cmddesc.c_str(), sendLatency);

		_commandQueueToReceive.push(pCmd);
		awaitsize = _commandQueueToReceive.size();
		
		_lastErr = RedisSink::rdeOK; // now up to OnReply() to update the _lastErr

		//TODO: protection if the _commandQueueToReceive is too long

		return pCmd;

	} while(0);

	// An error occurred, so call the response handler immediately (indicating the error)
	(new RequestErrCmd(*this, pCmd, _lastErr))->start();
	return NULL;
}

#define _ISPRINT(_CH) (_CH>=0x20 && _CH<0x7F)

// this encode is copied from URL by enlarging the allowed characters to all printables except " and %
int RedisClient::encode(std::string& output, const void* source, size_t len)
{
	output = "";

	if (source==NULL)
		return 0;

	if (len <0)
		len = strlen((const char*) source);

#if 1
	// if (_bCompress)
	{
		char page[REDIS_RECV_BUF_SIZE];
		bz_stream bzstrm;
		memset(&bzstrm, 0x00, sizeof(bzstrm));
		if (BZ_OK == BZ2_bzCompressInit(&bzstrm, 9, 0, 250))
		{
			bzstrm.next_in  =(char*) source;
			bzstrm.avail_in =len;
			bzstrm.next_out =(char*) page;
			bzstrm.avail_out=sizeof(page);

            int nRet = BZ2_bzCompress(&bzstrm, BZ_FINISH);
			if (BZ_STREAM_END == nRet)
			{
				output = "bz2:";
				source = page;
				len = sizeof(page) - bzstrm.avail_out;
			}

			BZ2_bzCompressEnd(&bzstrm);
		}
	}
#endif

	output += Base64::encode((const uint8*)source, len);
	return output.size();
}

int RedisClient::decode(const char* source, void* target, size_t maxlen)
{
	uint8 *targ = (uint8 *)target;

	if (targ ==NULL)
		return 0;

	size_t tlen = maxlen;

#if 1
	if (0 == strncmp(source, "bz2:", 4))
	{
        source += 4;
        if (!Base64::decode(source, targ, tlen))
            return 0;

		char page[REDIS_RECV_BUF_SIZE];
		bz_stream bzstrm;
		memset(&bzstrm, 0x00, sizeof(bzstrm));
        int nRet = BZ2_bzDecompressInit(&bzstrm, 0, 0);
		if (BZ_OK != nRet)
			return 0;

		bzstrm.next_in  = (char*)targ;
		bzstrm.avail_in = tlen;
		bzstrm.next_out = page;
		bzstrm.avail_out=sizeof(page);

        nRet = BZ2_bzDecompress(&bzstrm);
		if (BZ_STREAM_END == nRet)
		{
			tlen = sizeof(page) - bzstrm.avail_out;
			//page[tlen] ='\0';
            memcpy(target, (void*)page, tlen);
		}

		BZ2_bzDecompressEnd(&bzstrm);
	}
#endif
	
	return tlen;
}

char* RedisClient::_nextLine(char* startOfLine, int maxByte, int minLen)
{
	// returns the start of the next line, or NULL if none.  Note that this modifies the input string to add '\0' characters.

	// locate the beginning of the line
	for (; (*startOfLine =='\0' || *startOfLine == '\n') && maxByte >0; startOfLine++, maxByte--);

	// locate the end of line
	if (minLen<0)
		minLen =0;

	char* ptr = startOfLine + minLen;
	maxByte -= minLen;
	for (; maxByte >0 && *ptr != '\r' && *ptr != '\n'; ++ptr, maxByte--);

	if (maxByte<=0)
		return NULL;

	// change the "\r\n" as the string NULL
	if (*ptr == '\r')
		*ptr = '\0';

	return startOfLine;
}

int RedisClient::_sendLine(const std::string& cmdline)
{
	if (cmdline.empty())
		return -1;

	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "_sendLine() conn[%s] sending %s"), connDescription(), cmdline.c_str());
	std::string cmdToSend = cmdline + REDIS_NEWLINE;
	int ret = TCPClient::send(cmdToSend.c_str(), cmdToSend.length());

	if (ret < 0)
	{
		if (ret == SOCKET_ERROR)
		{
			int errnum = SYS::getLastErr(SYS::SOCK);
			std::string errMsg = SYS::getErrorMessage(SYS::SOCK);
			_log(Log::L_ERROR, CLOGFMT(RedisClient, "_sendLine() caught socket-err(%d)%s: %s"), errnum, errMsg.c_str(), cmdline.c_str());

#ifdef ZQ_OS_MSWIN
			if (errnum == WSAEWOULDBLOCK)// retry send until reach 5
#else
			if (errnum == EWOULDBLOCK) 
#endif
			{
				int retRetry   =-1;
				int retryTimes = 0;

				for (; retryTimes<5 && retRetry<0; retryTimes++)
				{
					SYS::sleep(200);
					retRetry = TCPClient::send(cmdToSend.c_str(), cmdToSend.length());
				}

				if (retRetry >=0)
				{
                    _log(Log::L_DEBUG, CLOGFMT(RedisClient, "_sendLine() succeeded after %d retrie(s) per EWOULDBLOCK: %s"),
                        retryTimes, cmdline.c_str());
					return 1;
				}

				_log(Log::L_ERROR, CLOGFMT(RedisClient, "_sendLine() retry failed: %s"), cmdline.c_str());
			}
		}

		// send failed, if it reaches MAX_SEND_ERROR_COUNT, close current connection
		// MutexGuard g(_lockQueueToSend); // borrow _lockQueueToSend to avoid thread-unsafe at non-blocking TCPClient::connect()
		int v = _sendErrorCount.add(1);
		if(v > MAX_SEND_ERROR_COUNT)
			OnError();

		_log(Log::L_ERROR, CLOGFMT(RedisClient, "sendCommand() failed to send: %s"), cmdline.c_str());
		return -3;
	}

    if (ret < (int)cmdToSend.length())
    {
        _log(Log::L_WARNING, CLOGFMT(RedisClient, "_sendLine() ret(%d) failed to send full command len(%d): %s"), ret, cmdToSend.length(), cmdline.c_str());
        TCPClient::send(REDIS_NEWLINE REDIS_NEWLINE, strlen(REDIS_NEWLINE REDIS_NEWLINE));
        return -4;
    }

	char sockdesc[100];
	snprintf(sockdesc, sizeof(sockdesc)-2, CLOGFMT(RedisClient, "_sendLine() conn[%08x]"), (uint) TCPSocket::get());

	if (REDIS_VERBOSEFLG_SEND_HEX & _verboseFlags)
		_log.hexDump(Log::L_DEBUG, cmdToSend.c_str(), cmdToSend.length(), sockdesc);
	else _log.hexDump(Log::L_INFO, cmdToSend.c_str(), cmdToSend.length(), sockdesc, true);

	return ret;
}

void RedisClient::_cancelCommands()
{
	while (!_commandQueueToSend.empty())
	{
		RedisCommand::Ptr pCmd = _commandQueueToSend.front();
		_commandQueueToSend.pop();

		try {
			(new RequestErrCmd(*this, pCmd, RedisSink::rdeClientCanceled))->start();
		}
		catch(...) {}
	}

	while (!_commandQueueToReceive.empty())
	{
		RedisCommand::Ptr pCmd = _commandQueueToReceive.front();
		_commandQueueToReceive.pop();

		try {
			(new RequestErrCmd(*this, pCmd, RedisSink::rdeClientCanceled))->start();
		}
		catch(...) {}
	}
}

void RedisClient::disconnect()
{
	ZQ::common::MutexGuard g(_lockCommandQueue);
	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "cancel() drop conn[%s] by cancel %d pending commands and %d await commands"), connDescription(), _commandQueueToSend.size(), _commandQueueToReceive.size());
	_cancelCommands();
	TCPClient::disconnect();
}

void RedisClient::OnConnected()
{
	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnConnected() connected to the peer, new conn: %s"), connDescription());
	TCPSocket::setTimeout(_messageTimeout >>1); // half of _messageTimeout to wake up the socket sleep()
	_inCommingByteSeen =0;

	{
		CommandQueue tmpQueue;
		ZQ::common::MutexGuard g(_lockCommandQueue);
		tmpQueue = _commandQueueToSend;
		_commandQueueToSend=CommandQueue(); // clear the original queue

		// Resume sending all pending requests:
		int64 stampNow = TimeUtil::now();
		int cFlushed=0, cExpired=0;
		while (!tmpQueue.empty())
		{
			RedisCommand::Ptr pCmd = tmpQueue.front();
			tmpQueue.pop();
			if (!pCmd)
				continue;

			try {
				if (pCmd->_stampCreated + _timeout < stampNow)
				{
					(new RequestErrCmd(*this, pCmd, RedisSink::rdeRequestTimeout))->start();
					cExpired++;
					continue;
				}

				sendCommand(pCmd);
				cFlushed++;
			}
			catch(...)
			{
				_log(Log::L_INFO, CLOGFMT(RedisClient, "OnConnected() sendRequest catch ..."));
				break;
			}

			pCmd = NULL;
		}

		if (tmpQueue.empty())
		{
			_log(Log::L_INFO, CLOGFMT(RedisClient, "OnConnected() new conn[%s], %d pending request(s) reflushed and %d expired, took %lldmsec"), connDescription(), cFlushed, cExpired, TimeUtil::now() - stampNow);
			return; // successful exit point of the func
		}
	}

	// an error occurred.  tell all pending requests about the error
	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnConnected() new conn[%s] established but failed to send, %d pending request(s) to cancel"), connDescription(), _commandQueueToSend.size());
	disconnect();
}

void RedisClient::OnError()
{
	_log(Log::L_ERROR, CLOGFMT(RedisClient, "OnError() conn[%s] socket error[%d] occurred, canceling all commands..."), connDescription(), checkSoErr());
	disconnect();
}

// overridding of TCPSocket
void RedisClient::OnTimeout()
{
	// _cleanupExpiredAwaitRequests(2, "OnTimeout");

	if (REDIS_VERBOSEFLG_TCPTHREADPOOL & _verboseFlags)
	{
		int poolSize, activeCount, pendingSize=0;
		if (TCPSocket::getThreadPoolStatus(poolSize, activeCount, pendingSize) && pendingSize>0)
			_log(pendingSize>100? Log::L_WARNING :Log::L_DEBUG, CLOGFMT(RedisClient, "OnTimeout() TCP ThreadPool[%d/%d] pending [%d]requests"), activeCount, poolSize, pendingSize);
	}

	if (REDIS_VERBOSEFLG_THREADPOOL & _verboseFlags)
	{
		int poolSize, activeCount, pendingSize=0;
		poolSize    = _thrdpool.size();
		pendingSize = _thrdpool.pendingRequestSize();
		activeCount = _thrdpool.activeCount();

		if (pendingSize >0)
			_log(pendingSize>100? Log::L_WARNING :Log::L_DEBUG, CLOGFMT(RedisClient, "OnTimeout() client ThreadPool[%d/%d] pending [%d]requests"), activeCount, poolSize, pendingSize);
	}
}

void RedisClient::OnDataArrived()
{
	struct sockaddr_in fromAddress;
	socklen_t addressSize = sizeof(fromAddress);

	CommandQueue completedCmds;
	bool bCancelConnection = false;
	int64 stampNow = TimeUtil::now();

	{
		MutexGuard g(_lockCommandQueue);

		int bytesToRead = sizeof(_inCommingBuffer) - _inCommingByteSeen;
		if (bytesToRead <=0)
		{
			_log(Log::L_WARNING, CLOGFMT(RedisClient, "OnDataArrived() conn[%s] last incomplete message exceed bufsz[%d] from offset[%d], give it up"), connDescription(), sizeof(_inCommingBuffer), _inCommingByteSeen);
			_inCommingByteSeen =0;
			bytesToRead = sizeof(_inCommingBuffer) - _inCommingByteSeen;
		}

		int bytesRead = recv(_so, (char*) &_inCommingBuffer[_inCommingByteSeen], bytesToRead, 0);

		if (bytesRead <= 0)
		{
			int err = errorno();
#ifdef ZQ_OS_MSWIN
			if (WSAEWOULDBLOCK == err || WSAEINPROGRESS == err || WSAEALREADY == err)
#else
			if (EINPROGRESS == err)
#endif // ZQ_OS_MSWIN
				_log(Log::L_WARNING, CLOGFMT(RedisClient, "OnDataArrived() conn[%s] recv() temporary fail[%d/%d], errno[%d]"), connDescription(), bytesRead, bytesToRead, err);
			else
			{
				_log(Log::L_ERROR, CLOGFMT(RedisClient, "OnDataArrived() conn[%s] recv() failed[%d/%d], errno[%d]"), connDescription(), bytesRead, bytesToRead, err);
				OnError();
			}

			return;
		}

		_sendErrorCount.set(0); //current connection is normal, reset _sendErrorCount

		{
			char sockdesc[100];
			snprintf(sockdesc, sizeof(sockdesc)-2, CLOGFMT(RedisClient, "OnDataArrived() conn[%08x]"), TCPSocket::get());

			if (REDIS_VERBOSEFLG_RECV_HEX & _verboseFlags)
				_log.hexDump(Log::L_DEBUG, &_inCommingBuffer[_inCommingByteSeen], bytesRead, sockdesc);
			else
				_log.hexDump(Log::L_INFO, &_inCommingBuffer[_inCommingByteSeen], bytesRead, sockdesc, true);
		}

		// quit if there is no awaiting commands
		if (_commandQueueToReceive.empty())
		{
			_log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnDataArrived() conn[%08x] ignore the receiving since there are no await commands"), TCPSocket::get());
			return;
		}

		if (REDIS_VERBOSEFLG_TCPTHREADPOOL & _verboseFlags)
		{
			int poolSize, activeCount, pendingSize=0;
			if (TCPSocket::getThreadPoolStatus(poolSize, activeCount, pendingSize) && pendingSize>0)
				_log(pendingSize>50*poolSize? Log::L_WARNING :Log::L_DEBUG, CLOGFMT(RedisClient, "OnDataArrived() TCP ThreadPool[%d/%d] pending [%d] requests"), activeCount, poolSize, pendingSize);
		}

		if (REDIS_VERBOSEFLG_THREADPOOL & _verboseFlags)
		{
			int poolSize, activeCount, pendingSize=0;
			poolSize    = _thrdpool.size();
			pendingSize = _thrdpool.pendingRequestSize();
			activeCount = _thrdpool.activeCount();

			if (pendingSize >0)
				_log(pendingSize>100? Log::L_WARNING :Log::L_DEBUG, CLOGFMT(RedisClient, "OnDataArrived() client ThreadPool[%d/%d] pending [%d] requests"), activeCount, poolSize, pendingSize);
		}
		
		if(_commandQueueToReceive.empty())
			return;
		
		RedisCommand::Ptr pCmd = _commandQueueToReceive.front();

		char* pProcessed = _inCommingBuffer, *pEnd = _inCommingBuffer + _inCommingByteSeen + bytesRead;
		bool bFinishedThisDataChuck = false;
		stampNow = TimeUtil::now();
		int minLen =0;

		while ((pProcessed < pEnd && !bCancelConnection) && pCmd) // (pCmd && _pCurrentMsg->contentLenToRead==0))
		{
			// process a line
			char* line = _nextLine(pProcessed, pEnd - pProcessed, minLen);
			if (NULL == line)
			{
				// met an incompleted line, shift it to the beginning of buffer then wait for the next OnDataArrived()
#pragma message ( __MSGLOC__ "TODO: impl here if a single bulk is larger than REDIS_RECV_BUF_SIZE")
				break;
			}

			int len = strlen(line);
			pProcessed += len + sizeof(REDIS_NEWLINE)-1;
			if (len <=0) // ignore empty line
				continue;

			// processing incomplete bulk(s) left from the previous buffer scan
			if (0 != pCmd->_replyCtx.data.type && pCmd->_replyCtx.nBulksLeft > 0) // incomplete bulk(s)
			{
				if (!pCmd->_replyCtx.isBulkPayload)
				{
					if (*(line++) != REDIS_LEADINGCH_BULK)
					{
						pCmd->_replyCtx.errCode = RedisSink::rdeProtocolMissed;
						break;
					}

					if ((minLen =atoi(line)) <0)
					{
						pCmd->_replyCtx.data.bulks.push_back(""); // fill-in an empty line
						pCmd->_replyCtx.nBulksLeft--;
					}
					else pCmd->_replyCtx.isBulkPayload = true;

					continue;
				}

				// the line is a bulk payload
				pCmd->_replyCtx.data.bulks.push_back(line); // fill-in the line
				pCmd->_replyCtx.nBulksLeft--;
				pCmd->_replyCtx.isBulkPayload = false;
				minLen =0;
				continue;
			}

			// a completed reply processed
			if (0 != pCmd->_replyCtx.data.type)
			{
				// put it into the completed list
				if (pCmd)
					completedCmds.push(pCmd);

				// move to next await command
				_commandQueueToReceive.pop();
				pCmd = NULL;
				if(!_commandQueueToReceive.empty())
					pCmd = _commandQueueToReceive.front();

				if (!pCmd)
					break;
			}

			// beginning of a new reply
			pCmd->_replyCtx.data.type = *(line++);
			pCmd->_replyCtx.nBulksLeft = 0;
			pCmd->_replyCtx.isBulkPayload = false;
			minLen =0;

			switch(pCmd->_replyCtx.data.type)
			{
			case REDIS_LEADINGCH_ERROR:
				pCmd->_replyCtx.data.bulks.push_back(line);
				pCmd->_replyCtx.errCode = RedisSink::rdeServerReturnedError;
				break;

			case REDIS_LEADINGCH_INLINE:
				pCmd->_replyCtx.data.bulks.push_back(line);
				pCmd->_replyCtx.errCode = 0;
				break;

			case REDIS_LEADINGCH_INT:
				pCmd->_replyCtx.data.integer = atol(line);
				pCmd->_replyCtx.errCode = 0;
				break;

			case REDIS_LEADINGCH_BULK:
				if ((minLen = atoi(line)) <=0)
				{
					pCmd->_replyCtx.errCode = 0; // empty bulk
					break;
				}

				// except a/the next line is bulk payload
				pCmd->_replyCtx.isBulkPayload = true;
				pCmd->_replyCtx.nBulksLeft = 1; 
				break;

			case REDIS_LEADINGCH_MULTIBULK:
				pCmd->_replyCtx.isBulkPayload = false;
				pCmd->_replyCtx.nBulksLeft = atol(line);
				minLen = 0;
				if (pCmd->_replyCtx.nBulksLeft <= 0)
				{
					// empty bulks
					pCmd->_replyCtx.nBulksLeft = 0;
					pCmd->_replyCtx.errCode = 0;
				}

				break;

			default:
				_log(Log::L_WARNING, CLOGFMT(RedisClient, "OnDataArrived() unknown proto-leading type[%c] received, cancel the connection"), pCmd->_replyCtx.data.type);
				pCmd->_replyCtx.errCode = RedisSink::rdeProtocolMissed;
				break;
			}   

			// determin if needs to give up the connection
			if (RedisSink::rdeProtocolMissed == pCmd->_replyCtx.errCode)
				bCancelConnection = true;
			else if (REDIS_LEADINGCH_ERROR != pCmd->_replyCtx.data.type && pCmd->_replyType != pCmd->_replyCtx.data.type)
				bCancelConnection = true;

		} // end of current buffer reading

		if (pCmd && 0 != pCmd->_replyCtx.data.type && pCmd->_replyCtx.nBulksLeft <= 0)
			completedCmds.push(pCmd);

		// shift the unhandled buffer to the beginning, process with next OnData()
		_log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnDataArrived() conn[%s] received %d bytes, appending to buf[%d], chopped out %d replies, %d incompleted bytes left"), connDescription(), bytesRead, _inCommingByteSeen, completedCmds.size(), (int)(pEnd - pProcessed));
		if (pEnd >= pProcessed)
		{
			_inCommingByteSeen = pEnd - pProcessed;
			memcpy(_inCommingBuffer, pProcessed, _inCommingByteSeen);
		}
	} // end of processing of this ArrivedData

	// fire MessageProcessCmd for the replied commands
	//////////////////////////////////////////////////////////
	stampNow = TimeUtil::now();
	int cReply =0;
	while (!completedCmds.empty())
	{
		RedisCommand::Ptr pCmd = completedCmds.front();
		completedCmds.pop();

		if (!pCmd)
			continue;

		try {
			pCmd->_stampReceived = stampNow;
			(new ReplyDispatcher(*this, pCmd))->start();
			cReply++;
		}
		catch(...) {}
	}

	_log(cReply ? Log::L_INFO: Log::L_DEBUG, CLOGFMT(RedisClient, "OnDataArrived() conn[%s] dispatched, took %dmsec: %d Replies"), connDescription(), (int)(TimeUtil::now() -stampNow), cReply);
}

// async sending RedisCommands
// -----------------------------

RedisCommand::Ptr RedisClient::sendMONITOR(RedisSink::Ptr reply)
{
	return sendCommand("MONITOR", REDIS_LEADINGCH_INLINE, reply);
}

RedisCommand::Ptr RedisClient::sendPING(RedisSink::Ptr reply)
{
	return sendCommand("PING", REDIS_LEADINGCH_INLINE, reply);
}

RedisCommand::Ptr RedisClient::sendINFO(RedisSink::Ptr reply)
{
	return sendCommand("INFO", REDIS_LEADINGCH_BULK, reply);
}

RedisCommand::Ptr RedisClient::sendSLAVEOF(const char *host, int port, RedisSink::Ptr reply)
{

	if (host == NULL || port == 0)
		return sendCommand("SLAVEOF no one", REDIS_LEADINGCH_INLINE, reply);

	char buf[100];
	snprintf(buf, sizeof(buf)-2, "SLAVEOF %s %d", host, port);
	return sendCommand(buf, REDIS_LEADINGCH_INLINE, reply);
}

RedisCommand::Ptr RedisClient::sendSET(const char *key, const uint8* val, int vlen, RedisSink::Ptr reply)
{
	std::string cmdstr;
	int nLen = encode(cmdstr, val, vlen);
	cmdstr = std::string("SET ") + key + " " + cmdstr;

	return sendCommand(cmdstr.c_str(), REDIS_LEADINGCH_INLINE, reply);
}

RedisCommand::Ptr RedisClient::sendDEL(const char *key, RedisSink::Ptr reply)
{
	std::string cmdstr = std::string("DEL ") + key;
	return sendCommand(cmdstr.c_str(), REDIS_LEADINGCH_INLINE, reply);
}

RedisCommand::Ptr RedisClient::sendGETSET(const char *key, const uint8* val, int vlen, RedisSink::Ptr reply)
{
	std::string cmdstr;
	encode(cmdstr, val, vlen);
	cmdstr = std::string("GETSET ") + key + " " + cmdstr;

	return sendCommand(cmdstr.c_str(), REDIS_LEADINGCH_BULK, reply);
}

RedisCommand::Ptr RedisClient::sendGET(const char *key, RedisSink::Ptr reply)
{
	std::string cmdstr = std::string("GET ") + key;
	return sendCommand(cmdstr.c_str(), REDIS_LEADINGCH_BULK, reply);
}

RedisCommand::Ptr RedisClient::sendSETAddRm(const char *cmd, const char *key, const char *member, int vlen, RedisSink::Ptr reply)
{
	std::string cmdstr;
	encode(cmdstr, member, vlen);
//	char buf[20];
//	snprintf(buf, sizeof(buf)-2, "%d", cmdstr.length());
	cmdstr = std::string(cmd) + " " + key + " " + cmdstr; // + buf + " " + cmdstr;
	return sendCommand(cmdstr.c_str(), REDIS_LEADINGCH_INT, reply);
}

RedisCommand::Ptr RedisClient::sendSADD(const char *key, const char *member, int vlen, RedisSink::Ptr reply)
{
  return sendSETAddRm("SADD", key, member, vlen, reply);
}

RedisCommand::Ptr RedisClient::sendSREM(const char *key, const char *member, int vlen, RedisSink::Ptr reply)
{
  return sendSETAddRm("SREM", key, member, vlen, reply);
}

RedisCommand::Ptr RedisClient::sendSMEMBERS(const char *key, RedisSink::Ptr reply)
{
	std::vector<std::string> keys;
	keys.push_back(key);
	return sendMultikeyBulkCmd("SMEMBERS", keys, reply);
}

RedisCommand::Ptr RedisClient::sendKEYS(const char *pattern, RedisSink::Ptr reply)
{
	std::vector<std::string> keys;
	keys.push_back(pattern);
	return sendMultikeyBulkCmd("KEYS", keys, reply);
}

RedisCommand::Ptr RedisClient::sendMultikeyBulkCmd(const char *cmd, std::vector<std::string>& keys, RedisSink::Ptr reply)
{
	std::string cmdstr = cmd;
	for (size_t i=0; i < keys.size();i++)
		cmdstr += std::string(" ") + keys[i];
	return sendCommand(cmdstr.c_str(), REDIS_LEADINGCH_MULTIBULK, reply);
}

// sync Redis executions
// -----------------------------
#define SYNC_WAIT_ASSERT_RET() 	if (!pCmd) return (_lastErr = RedisSink::rdeClientError); \
	if (!pCmd->wait(_messageTimeout)) return (_lastErr = RedisSink::rdeRequestTimeout); \
	if (REDIS_LEADINGCH_ERROR == pCmd->_replyCtx.data.type)	return (_lastErr = RedisSink::rdeServerReturnedError); else _lastErr = RedisSink::rdeOK;

RedisSink::Error RedisClient::SET(const std::string& key, const uint8* value, int vlen)
{
	RedisCommand::Ptr pCmd = sendSET(key.c_str(), value, vlen, NULL);
	SYNC_WAIT_ASSERT_RET();
	return RedisSink::rdeOK;
}

RedisSink::Error RedisClient::GET(const std::string& key, uint8* value, uint& vlen)
{
	RedisCommand::Ptr pCmd = sendGET(key.c_str(), NULL);
	SYNC_WAIT_ASSERT_RET();

	if (pCmd->_replyCtx.data.bulks.size() <=0)
	{
		vlen =0;
		return RedisSink::rdeNil;
	}
    vlen = strlen(pCmd->_replyCtx.data.bulks[0].c_str());
	vlen = decode(pCmd->_replyCtx.data.bulks[0].c_str(), value, vlen);
	return RedisSink::rdeOK;
}

RedisSink::Error RedisClient::DEL(const std::string& key)
{
	RedisCommand::Ptr pCmd = sendDEL(key.c_str(), NULL);
	SYNC_WAIT_ASSERT_RET();
	return RedisSink::rdeOK;
}

RedisSink::Error RedisClient::SADD(const std::string& key, const std::string& member)
{
	RedisCommand::Ptr pCmd = sendSADD(key.c_str(), member.c_str(), member.length());
	SYNC_WAIT_ASSERT_RET();

	return RedisSink::rdeOK;
}

RedisSink::Error RedisClient::SREM(const std::string& key, const std::string& member)
{
	RedisCommand::Ptr pCmd = sendSREM(key.c_str(), member.c_str(), member.length());
	SYNC_WAIT_ASSERT_RET();

	return RedisSink::rdeOK;
}

RedisSink::Error RedisClient::SMEMBERS(const std::string& key, StringList& members)
{
	RedisCommand::Ptr pCmd = sendSMEMBERS(key.c_str());
	SYNC_WAIT_ASSERT_RET();

	members.clear();
    members.assign(pCmd->_replyCtx.data.bulks.begin(), pCmd->_replyCtx.data.bulks.end());
	return RedisSink::rdeOK;
}

RedisSink::Error RedisClient::KEYS(const std::string& pattern, StringList& keys)
{
	RedisCommand::Ptr pCmd = sendKEYS(pattern.c_str());
	SYNC_WAIT_ASSERT_RET();

	keys.clear();
    keys.assign(pCmd->_replyCtx.data.bulks.begin(), pCmd->_replyCtx.data.bulks.end());
	return RedisSink::rdeOK;
}

}} // namespaces

/*
	/////////////////////////////////////////
	int credis_info(REDIS rhnd, REDIS_INFO *info)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "INFO\r\n");

  if (rc == 0) {
    char role;
    memset(info, 0, sizeof(REDIS_INFO));
    cr_parseinfo(rhnd->reply.bulk, "redis_version", "%"CR_VERSION_STRING_SIZE_STR"s\r\n", &(info->redis_version));
    cr_parseinfo(rhnd->reply.bulk, "arch_bits", "%d", &(info->arch_bits));
    cr_parseinfo(rhnd->reply.bulk, "multiplexing_api", "%"CR_MULTIPLEXING_API_SIZE_STR"s\r\n", &(info->multiplexing_api));
    cr_parseinfo(rhnd->reply.bulk, "process_id", "%ld", &(info->process_id));
    cr_parseinfo(rhnd->reply.bulk, "uptime_in_seconds", "%ld", &(info->uptime_in_seconds));
    cr_parseinfo(rhnd->reply.bulk, "uptime_in_days", "%ld", &(info->uptime_in_days));
    cr_parseinfo(rhnd->reply.bulk, "connected_clients", "%d", &(info->connected_clients));
    cr_parseinfo(rhnd->reply.bulk, "connected_slaves", "%d", &(info->connected_slaves));
    cr_parseinfo(rhnd->reply.bulk, "blocked_clients", "%d", &(info->blocked_clients));
    cr_parseinfo(rhnd->reply.bulk, "used_memory", "%zu", &(info->used_memory));
    cr_parseinfo(rhnd->reply.bulk, "used_memory_human", "%"CR_USED_MEMORY_HUMAN_SIZE_STR"s", &(info->used_memory_human));
    cr_parseinfo(rhnd->reply.bulk, "changes_since_last_save", "%lld", &(info->changes_since_last_save));
    cr_parseinfo(rhnd->reply.bulk, "bgsave_in_progress", "%d", &(info->bgsave_in_progress));
    cr_parseinfo(rhnd->reply.bulk, "last_save_time", "%ld", &(info->last_save_time));
    cr_parseinfo(rhnd->reply.bulk, "bgrewriteaof_in_progress", "%d", &(info->bgrewriteaof_in_progress));
    cr_parseinfo(rhnd->reply.bulk, "total_connections_received", "%lld", &(info->total_connections_received));
    cr_parseinfo(rhnd->reply.bulk, "total_commands_processed", "%lld", &(info->total_commands_processed));
    cr_parseinfo(rhnd->reply.bulk, "expired_keys", "%lld", &(info->expired_keys));
    cr_parseinfo(rhnd->reply.bulk, "hash_max_zipmap_entries", "%zu", &(info->hash_max_zipmap_entries));
    cr_parseinfo(rhnd->reply.bulk, "hash_max_zipmap_value", "%zu", &(info->hash_max_zipmap_value));
    cr_parseinfo(rhnd->reply.bulk, "pubsub_channels", "%ld", &(info->pubsub_channels));
    cr_parseinfo(rhnd->reply.bulk, "pubsub_patterns", "%u", &(info->pubsub_patterns));
    cr_parseinfo(rhnd->reply.bulk, "vm_enabled", "%d", &(info->vm_enabled));
    cr_parseinfo(rhnd->reply.bulk, "role", "%c", &role);

    info->role = ((role=='m')?CREDIS_SERVER_MASTER:CREDIS_SERVER_SLAVE);
  }
  
  return rc;
}

int credis_monitor(REDIS rhnd)
{
  return cr_sendfandreceive(rhnd, CR_INLINE, "MONITOR\r\n");
}

int credis_slaveof(REDIS rhnd, const char *host, int port)
{
  if (host == NULL || port == 0)
    return cr_sendfandreceive(rhnd, CR_INLINE, "SLAVEOF no one\r\n");
  else
    return cr_sendfandreceive(rhnd, CR_INLINE, "SLAVEOF %s %d\r\n", host, port);
}

static int cr_setaddrem(REDIS rhnd, const char *cmd, const char *key, const char *member)
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "%s %s %s\r\n", // "%s %s %zu\r\n%s\r\n", 
                              cmd, key, member); //cmd, key, strlen(member), member);

  if (rc == 0 && rhnd->reply.integer == 0)
    rc = -1;

  return rc;
}

int credis_sadd(REDIS rhnd, const char *key, const char *member)
{
  return cr_setaddrem(rhnd, "SADD", key, member);
}

int credis_srem(REDIS rhnd, const char *key, const char *member)
{
  return cr_setaddrem(rhnd, "SREM", key, member);
}

int credis_spop(REDIS rhnd, const char *key, char **member)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "SPOP %s\r\n", key);

  if (rc == 0 && (*member = rhnd->reply.bulk) == NULL)
    rc = -1;

  return rc;
}

int credis_smove(REDIS rhnd, const char *sourcekey, const char *destkey, 
                 const char *member)
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "SMOVE %s %s %s\r\n", 
                              sourcekey, destkey, member);

  if (rc == 0 && rhnd->reply.integer == 0)
    rc = -1;

  return rc;
}

int credis_scard(REDIS rhnd, const char *key) 
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "SCARD %s\r\n", key);

  if (rc == 0)
    rc = rhnd->reply.integer;

  return rc;
}

int credis_sinter(REDIS rhnd, int keyc, const char **keyv, char ***members)
{
  return cr_multikeybulkcommand(rhnd, "SINTER", keyc, keyv, members);
}

int credis_sunion(REDIS rhnd, int keyc, const char **keyv, char ***members)
{
  return cr_multikeybulkcommand(rhnd, "SUNION", keyc, keyv, members);
}

int credis_sdiff(REDIS rhnd, int keyc, const char **keyv, char ***members)
{
  return cr_multikeybulkcommand(rhnd, "SDIFF", keyc, keyv, members);
}

int credis_sinterstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv)
{
  return cr_multikeystorecommand(rhnd, "SINTERSTORE", destkey, keyc, keyv);
}

int credis_sunionstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv)
{
  return cr_multikeystorecommand(rhnd, "SUNIONSTORE", destkey, keyc, keyv);
}

int credis_sdiffstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv)
{
  return cr_multikeystorecommand(rhnd, "SDIFFSTORE", destkey, keyc, keyv);
}

int credis_sismember(REDIS rhnd, const char *key, const char *member)
{
  return cr_setaddrem(rhnd, "SISMEMBER", key, member);
}

int credis_smembers(REDIS rhnd, const char *key, char ***members)
{
  return cr_multikeybulkcommand(rhnd, "SMEMBERS", 1, &key, members);
}

int credis_zadd(REDIS rhnd, const char *key, double score, const char *member)
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZADD %s %f %s\r\n", // "ZADD %s %f %zu\r\n%s\r\n", 
                              key, score, member); // key, score, strlen(member), member);

  if (rc == 0 && rhnd->reply.integer == 0)
    rc = -1;

  return rc;
}

int credis_zrem(REDIS rhnd, const char *key, const char *member)
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZREM %s %s\r\n",  // "ZREM %s %zu\r\n%s\r\n",
                              key, member);

  if (rc == 0 && rhnd->reply.integer == 0)
    rc = -1;

  return rc;
}

// TODO what does Redis return if member is not member of set?
int credis_zincrby(REDIS rhnd, const char *key, double incr_score, const char *member, double *new_score)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "ZINCRBY %s %f %s\r\n",  // "ZINCRBY %s %f %zu\r\n%s\r\n",
                              key, incr_score, member); // key, incr_score, strlen(member), member); 

  if (rc == 0 && new_score)
    *new_score = strtod(rhnd->reply.bulk, NULL);

  return rc;
}

// TODO what does Redis return if member is not member of set?
static int cr_zrank(REDIS rhnd, int reverse, const char *key, const char *member)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "%s %s %s\r\n", // "%s %s %zu\r\n%s\r\n", 
                              reverse==1?"ZREVRANK":"ZRANK", key, member); // key, strlen(member), member);

  if (rc == 0)
    rc = atoi(rhnd->reply.bulk);

  return rc;
}

int credis_zrank(REDIS rhnd, const char *key, const char *member)
{
  return cr_zrank(rhnd, 0, key, member);
}

int credis_zrevrank(REDIS rhnd, const char *key, const char *member)
{
  return cr_zrank(rhnd, 1, key, member);
}

int cr_zrange(REDIS rhnd, int reverse, const char *key, int start, int end, char ***elementv)
{
  int rc = cr_sendfandreceive(rhnd, CR_MULTIBULK, "%s %s %d %d\r\n",
                              reverse==1?"ZREVRANGE":"ZRANGE", key, start, end);

  if (rc == 0) {
    *elementv = rhnd->reply.multibulk.bulks;
    rc = rhnd->reply.multibulk.len;
  }

  return rc;
}

int credis_zrange(REDIS rhnd, const char *key, int start, int end, char ***elementv)
{
  return cr_zrange(rhnd, 0, key, start, end, elementv);
}

int credis_zrevrange(REDIS rhnd, const char *key, int start, int end, char ***elementv)
{
  return cr_zrange(rhnd, 1, key, start, end, elementv);
}

int credis_zcard(REDIS rhnd, const char *key)
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZCARD %s\r\n", key);

  if (rc == 0) {
    if (rhnd->reply.integer == 0)
      rc = -1;
    else
      rc = rhnd->reply.integer;
  }

  return rc;
}

int credis_zscore(REDIS rhnd, const char *key, const char *member, double *score)
{
  int rc = cr_sendfandreceive(rhnd, CR_BULK, "ZSCORE %s %s\r\n",  // "ZSCORE %s %zu\r\n%s\r\n", 
                              key, member); // key, strlen(member), member);

  if (rc == 0) {
    if (!rhnd->reply.bulk)
      rc = -1;
    else if (score)
      *score = strtod(rhnd->reply.bulk, NULL);
  }

  return rc;
}

int credis_zremrangebyscore(REDIS rhnd, const char *key, double min, double max)
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZREMRANGEBYSCORE %s %f %f\r\n", 
                              key, min, max);

  if (rc == 0)
    rc = rhnd->reply.integer;

  return rc;
}

int credis_zremrangebyrank(REDIS rhnd, const char *key, int start, int end)
{
  int rc = cr_sendfandreceive(rhnd, CR_INT, "ZREMRANGEBYRANK %s %d %d\r\n", 
                              key, start, end);

  if (rc == 0)
    rc = rhnd->reply.integer;

  return rc;
}

// TODO add writev() support instead and push strings to send onto a vector of
// strings to send instead...
static int cr_zstore(REDIS rhnd, int inter, const char *destkey, int keyc, const char **keyv, 
                     const int *weightv, REDIS_AGGREGATE aggregate)
{
  cr_buffer *buf = &(rhnd->buf);
  int rc, i;

  buf->len = 0;
  
  if ((rc = cr_appendstrf(buf, "%s %s %d ", inter?"ZINTERSTORE":"ZUNIONSTORE", destkey, keyc)) != 0)
    return rc;
  if ((rc = cr_appendstrarray(buf, keyc, keyv, 0)) != 0)
    return rc;
  if (weightv != NULL)
    for (i = 0; i < keyc; i++)
      if ((rc = cr_appendstrf(buf, " %d", weightv[i])) != 0)
        return rc;

  switch (aggregate) {
  case SUM: 
    rc = cr_appendstr(buf, "AGGREGATE SUM", 0);
    break;
  case MIN:
    rc = cr_appendstr(buf, "AGGREGATE MIN", 0);
    break;
  case MAX:
    rc = cr_appendstr(buf, "AGGREGATE MAX", 0);
    break;
  case NONE:
    ; // avoiding compiler warning
  }
  if (rc != 0)
    return rc;

  if ((rc = cr_appendstr(buf, "\r\n", 0)) != 0)
    return rc;

  if ((rc = cr_sendandreceive(rhnd, CR_INT)) == 0) 
    rc = rhnd->reply.integer;

  return rc;
}

int credis_zinterstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv, 
                       const int *weightv, REDIS_AGGREGATE aggregate)
{
  return cr_zstore(rhnd, 1, destkey, keyc, keyv, weightv, aggregate);
}

int credis_zunionstore(REDIS rhnd, const char *destkey, int keyc, const char **keyv, 
                       const int *weightv, REDIS_AGGREGATE aggregate)
{
  return cr_zstore(rhnd, 0, destkey, keyc, keyv, weightv, aggregate);
}

/////////////////////////////////////////
	// issues a RTSP "ANNOUNCE" command (with "sdpDescription" as parameter),
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param sdpDescription to specify the SDP description if needed
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendANNOUNCE(const char* sdpDescription, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "SET_PARAMETER" command on "connection", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param paramMap a <key, value> map to specify paramters to set
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendSET_PARAMETER(const RedisCommand::AttrMap& paramMap, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "GET_PARAMETER" command on "connection", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param parameterNames a list of keynames of parameters that wish to query for
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendGET_PARAMETER(const RedisCommand::AttrList& parameterNames, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues a RTSP "SETUP" command, then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to setup, user must instantize an RTSPSession object before SETUP
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendSETUP(RTSPSession& session, const char* SDP=NULL, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "PLAY" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendPLAY(RTSPSession& session, double start = 0.0f, double end = -1.0f, float scale = 1.0f, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "PAUSE" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendPAUSE(RTSPSession& session, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "RECORD" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendRECORD(RTSPSession& session, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "TEARDOWN" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendTEARDOWN(RTSPSession& session, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	// issues an aggregate RTSP "SET_PARAMETER" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param paramMap a <key, value> map to specify paramters to set
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendSET_PARAMETER(RTSPSession& session, const RedisCommand::AttrMap& paramMap, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	/// issues an aggregate RTSP "GET_PARAMETER" command on "session", then returns the "CSeq" sequence number that was used in the command.
	/// the overwriteable entry OnResponse() would be triggered if a response was received from the server, or
	/// the overwriteable entry OnRequestError() would be triggered
	///@param session is the RTSPSession to operate on
	///@param parameterNames a list of keynames of parameters that wish to query for
	///@param authenticator, (optional) is used for access control.  If you have username and password strings, you can use this by
	///       passing an actual parameter that you created by creating an "Authenticator(username, password) object".
	///@param headerToOverride, (optional) is used to customize or append the RTSP headers that prepared by this class by default
	///@return the cseq number if the request has been sent or queued, the cseq num would be in the range of [1,MAX_CLIENT_CSEQ)
	///        otherwise a negative return value indicates the invocation failed
	int sendGET_PARAMETER(RTSPSession& session, const RedisCommand::AttrList& parameterNames, Authenticator* authenticator =NULL, const RTSPMessage::AttrMap& headerToOverride =RTSPMessage::AttrMap());

	void setClientTimeout(int32 connectTimeout =DEFAULT_CONNECT_TIMEOUT, int32 messageTimeout =DEFAULT_CLIENT_TIMEOUT);

protected:
	NativeThreadPool&  _thrdpool;
	LogWrapper		  _log;

	std::string       _lastErr;

	// about the connection
	InetHostAddress  _serverAddress;
	tpport_t         _serverPort;
	int32            _connectTimeout;
	int32            _messageTimeout;

	// about the common variable of the client
	std::string      _userAgent;
	std::string		 _url;
	std::string      _serverType;

	uint32			 _cTcpStreams;
	AtomicInt        _lastCSeq;
	AtomicInt        _sendErrorCount;

	// about the request and pending queue
	typedef std::queue < RedisCommand::Ptr > RequestQueue;
	RequestQueue _requestsQueueToSend;
	Mutex        _lockQueueToSend;

	// about the requests that is waiting for responses
	typedef std::map <uint, RedisCommand::Ptr> CSeqToRedisCommandMap;
	CSeqToRedisCommandMap _requestsAwaitResponse;
	Mutex				_lockAwaitResponse;

	// send a request thru the connection
	//@return CSeq num in [1, MAX_CLIENT_CSEQ) if succeeded, or negative if failed
	int  sendCommand(RedisCommand::Ptr pRequest, const RTSPMessage::AttrMap& headerToOverride);

	int  sendMessage(RTSPMessage::Ptr pMessage, const RTSPMessage::AttrMap& headerToOverride, const char* msgDesc =NULL);

	RTSPMessage::Ptr _pCurrentMsg;
	char         _inCommingBuffer[RTSP_MSG_BUF_SIZE];
	int	     _inCommingByteSeen;
	Mutex	     _lockInCommingMsg;

	/// increase last CSeq and then return the new CSeq in the range [1, MAX_CLIENT_CSEQ)
	uint lastCSeq();

private:
	void _cleanupExpiredAwaitRequests(uint8 multiplyTimeout=1, char* func=""); // private use
	static uint16 _verboseFlags;
};

#ifndef MAPSET
#  define MAPSET(_MAPTYPE, _MAP, _KEY, _VAL) if (_MAP.end() ==_MAP.find(_KEY)) _MAP.insert(_MAPTYPE::value_type(_KEY, _VAL)); else _MAP[_KEY] = _VAL
#endif // MAPSET

}}//endof namespace

// brief usage:
//    RedisClient client(logger, threadpool, bindAddress, "rtsp://192.168.11.22");
//    RTSPSession::Ptr sess = new RTSPSession(logger, threadpool, "udp://@223.12.12.12:1234", "/aaa.mpg");
//    client.sendSETUP(*sess);
//    ... wait for the overwritten of RTSPSession::OnResponse_SETUP()
//    client.sendPLAY(*sess);
//    ... wait
//    client.sendTEARDOWN(*sess);
//    sess->destroy(); //must call to destroy the session

#endif // __RedisClient_H__
*/
