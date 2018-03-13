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
namespace eloop {
using namespace ZQ::common;

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

void RedisCommand::OnRequestError(RedisClient& client, RedisCommand& cmd, RedisSink::Error errCode, const char* errDesc)
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

RedisClient::RedisClient(ZQ::eloop::Loop& loop, ZQ::common::Log& log, const std::string& server, unsigned short serverPort, ZQ::common::Log::loglevel_t verbosityLevel)
: _serverIp(server), _serverPort(serverPort), _log(log, verbosityLevel), _verboseFlags(0), _lastErr(RedisSink::rdeOK),
_inCommingByteSeen(0), _tcpStatus(unConnect)
{
    init(loop);
    memset(_connDesc, 0, sizeof(_connDesc));
	setClientTimeout(DEFAULT_CONNECT_TIMEOUT, DEFAULT_CLIENT_TIMEOUT);
	do {
		if (_serverPort <=0)
			_serverPort = REDIS_DEFAULT_PORT;

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

	//TCPSocket::setTimeout(_messageTimeout >>1); // half of _messageTimeout to wake up the socket sleep()
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
	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "sendCommand() cmd len[%d]"), cmddesc.length());
	_lastErr = RedisSink::rdeSendError;
	// std::string lastErr;

	do // only one round of this loop, program can quit by either "break" or "continue"
	{
		bool connectionIsPending = false;
		{
			MutexGuard g(_lockCommandQueue);

			if (_commandQueueToSend.size() >0 || (Connecting == _tcpStatus))
			{
				// a connection is currently pending with at least one queued request.
				connectionIsPending = true;
			}
			else if (_tcpStatus == unConnect)
			{ 
				// need to open a connection
				if (TCP::connect4(_serverIp.c_str(), _serverPort) != 0)
				{
					_lastErr = RedisSink::rdeConnectError;
					_log(Log::L_ERROR, CLOGFMT(RedisClient, "sendCommand() cmd[%s] error occured at connect()"), cmddesc.c_str());
					break; // an error occurred
				}

                _tcpStatus = Connecting;
				connectionIsPending = true;
			}

			if (connectionIsPending)
			{
				_log(Log::L_DEBUG, CLOGFMT(RedisClient, "sendCommand() cmd[%s] connect in progress, wait for next try"), cmddesc.c_str());
				_commandQueueToSend.push(pCmd);

				// give up those expired requests in the queue and prevent this queue from growing too big
				int64 stampExp = ZQ::common::now() - _messageTimeout/2;
				while (!_commandQueueToSend.empty())
				{
					RedisCommand::Ptr pCmd = _commandQueueToSend.front();

					if (pCmd && (_messageTimeout/2 <=0 || pCmd->_stampCreated > stampExp) && _commandQueueToSend.size() < MAX_PENDING_REQUESTS)
						break;

					_commandQueueToSend.pop();

					if (!pCmd)
						continue;

					try {
                        pCmd->OnRequestError(*this, *pCmd, RedisSink::rdeConnectError);
					}
					catch(...) {}
				}

				return pCmd;
			}
		}

		// The client is currently connected, send the command instantly
		size_t awaitsize = 0;
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
			_log(Log::L_WARNING, CLOGFMT(RedisClient, "sendCommand() %s took %dmsec, too long until sent"), cmddesc.c_str(), sendLatency);

		_commandQueueToReceive.push(pCmd);
		awaitsize = _commandQueueToReceive.size();
		
		_lastErr = RedisSink::rdeOK; // now up to OnReply() to update the _lastErr

		//TODO: protection if the _commandQueueToReceive is too long

		return pCmd;

	} while(0);

	// An error occurred, so call the response handler immediately (indicating the error)
    pCmd->OnRequestError(*this, *pCmd, _lastErr);
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

	std::string cmdToSend = cmdline + REDIS_NEWLINE;
    int ret = TCP::write(cmdToSend.c_str(), cmdToSend.length());

	if (ret < 0)
	{
		if (ret == SOCKET_ERROR)
		{
			int errnum = SYS::getLastErr(SYS::SOCK);
			std::string errMsg = SYS::getErrorMessage(SYS::SOCK);
			_log(Log::L_ERROR, CLOGFMT(RedisClient, "_sendLine() caught socket-err(%d)%s: %s"), errnum, errMsg.c_str(), cmdline.c_str());
		}

		// send failed, if it reaches MAX_SEND_ERROR_COUNT, close current connection
		// MutexGuard g(_lockQueueToSend); // borrow _lockQueueToSend to avoid thread-unsafe at non-blocking TCPClient::connect()
		int v = _sendErrorCount.add(1);
		if(v > MAX_SEND_ERROR_COUNT)
			OnError();

		_log(Log::L_ERROR, CLOGFMT(RedisClient, "sendCommand() failed to send: %s"), cmdline.c_str());
		return -3;
	}

	char sockdesc[100];
	snprintf(sockdesc, sizeof(sockdesc)-2, CLOGFMT(RedisClient, "_sendLine() conn[%s]"), _connDesc);

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
            pCmd->OnRequestError(*this, *pCmd, RedisSink::rdeClientCanceled);
		}
		catch(...) {}
	}

	while (!_commandQueueToReceive.empty())
	{
		RedisCommand::Ptr pCmd = _commandQueueToReceive.front();
		_commandQueueToReceive.pop();

		try {
            pCmd->OnRequestError(*this, *pCmd, RedisSink::rdeClientCanceled);
		}
		catch(...) {}
	}
}

void RedisClient::disconnect()
{
	ZQ::common::MutexGuard g(_lockCommandQueue);
	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "cancel() drop conn[%s] by cancel %d pending commands and %d await commands"), _connDesc, _commandQueueToSend.size(), _commandQueueToReceive.size());
	_cancelCommands();
	shutdown();
    _tcpStatus = unConnect;
}

void RedisClient::OnConnected(ElpeError status)
{
    char localIp[17] = {0}, serverIp[17] = {0};
    int localPort = 0, serverPort = 0;
    getlocaleIpPort(localIp, localPort);
    getpeerIpPort(serverIp, serverPort);
    snprintf(_connDesc, sizeof(_connDesc) -2, "%s/%d->%s/%d", localIp, localPort, serverIp, serverPort);

    if (status != elpeSuccess)
    {
        _tcpStatus = unConnect;
        std::string desc = "on_connect error:";
        desc.append(errDesc(status));
        _log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnConnected() %s Connected has error: %s"), _connDesc, desc.c_str());
        return;
    }
    _tcpStatus = Connected;
	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnConnected() connected to the peer, new conn: %s"), _connDesc);
	set_blocking(1);
    read_start();
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
				if (pCmd->_stampCreated + _messageTimeout/2 < stampNow)
				{
                    pCmd->OnRequestError(*this, *pCmd, RedisSink::rdeRequestTimeout);
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
			_log(Log::L_INFO, CLOGFMT(RedisClient, "OnConnected() new conn[%s], %d pending request(s) reflushed and %d expired, took %lldmsec"), _connDesc, cFlushed, cExpired, TimeUtil::now() - stampNow);
			return; // successful exit point of the func
		}
	}

	// an error occurred.  tell all pending requests about the error
	_log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnConnected() new conn[%s] established but failed to send, %d pending request(s) to cancel"), _connDesc, _commandQueueToSend.size());
	disconnect();
}

void RedisClient::OnError()
{
	_log(Log::L_ERROR, CLOGFMT(RedisClient, "OnError() conn[%s] socket error[] occurred, canceling all commands..."), _connDesc);
	disconnect();
}

void RedisClient::OnRead(ssize_t nread, const char *buf)
{
    _log(Log::L_WARNING, CLOGFMT(RedisClient, "OnRead() conn[%s] read size[%d]"), _connDesc, nread);

    CommandQueue completedCmds;
    bool bCancelConnection = false;
    int64 stampNow = TimeUtil::now();

    {
        MutexGuard g(_lockCommandQueue);

        int bytesToRead = sizeof(_inCommingBuffer) - _inCommingByteSeen;
        if (bytesToRead <=0)
        {
            _log(Log::L_WARNING, CLOGFMT(RedisClient, "OnRead() conn[%s] last incomplete message exceed bufsz[%d] from offset[%d], give it up"), _connDesc, sizeof(_inCommingBuffer), _inCommingByteSeen);
            _inCommingByteSeen =0;
            bytesToRead = sizeof(_inCommingBuffer) - _inCommingByteSeen;
        }

        if (nread <= 0)
        {
            int err = errorno();
#ifdef ZQ_OS_MSWIN
            if (WSAEWOULDBLOCK == err || WSAEINPROGRESS == err || WSAEALREADY == err)
#else
            if (EINPROGRESS == err)
#endif // ZQ_OS_MSWIN
                _log(Log::L_WARNING, CLOGFMT(RedisClient, "OnRead() conn[%s] recv() temporary fail[%d/%d], errno[%d]"), _connDesc, nread, bytesToRead, err);
            else
            {
                _log(Log::L_ERROR, CLOGFMT(RedisClient, "OnRead() conn[%s] recv() failed[%d/%d], errno[%d]"), _connDesc, nread, bytesToRead, err);
                OnError();
            }

            return;
        }

        int nCopySize = 0, nLeftSize = 0;
        if (bytesToRead < nread)
        {
            memcpy((char*) &_inCommingBuffer[_inCommingByteSeen], buf, bytesToRead);
            nCopySize = bytesToRead;
            nLeftSize = nread - bytesToRead;
        }
        else
        {
            memcpy((char*) &_inCommingBuffer[_inCommingByteSeen], buf, nread);
            nCopySize = nread;
        }

        _sendErrorCount.set(0); //current connection is normal, reset _sendErrorCount

        {
            char sockdesc[100];
            snprintf(sockdesc, sizeof(sockdesc)-2, CLOGFMT(RedisClient, "OnRead() conn[%s]"), _connDesc);

            if (REDIS_VERBOSEFLG_RECV_HEX & _verboseFlags)
                _log.hexDump(Log::L_DEBUG, &_inCommingBuffer[_inCommingByteSeen], nCopySize, sockdesc);
            else
                _log.hexDump(Log::L_INFO, &_inCommingBuffer[_inCommingByteSeen], nCopySize, sockdesc, true);
        }

        // quit if there is no awaiting commands
        if (_commandQueueToReceive.empty())
        {
            _log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnRead() conn[%s] ignore the receiving since there are no await commands"), _connDesc);
            return;
        }

        if(_commandQueueToReceive.empty())
            return;

        RedisCommand::Ptr pCmd = _commandQueueToReceive.front();

        char* pProcessed = _inCommingBuffer, *pEnd = _inCommingBuffer + _inCommingByteSeen + nCopySize;
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
            _log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnRead() conn[%s] line:lenth[%s:%d] cmd[%s]"), _connDesc, line, len, pCmd->desc().c_str());
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
                {
                    completedCmds.push(pCmd);
                }

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
                _log(Log::L_WARNING, CLOGFMT(RedisClient, "OnRead() unknown proto-leading type[%c] received, cancel the connection"), pCmd->_replyCtx.data.type);
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
        {
            completedCmds.push(pCmd);
            _commandQueueToReceive.pop();
        }

        // shift the unhandled buffer to the beginning, process with next OnData()
        _log(Log::L_DEBUG, CLOGFMT(RedisClient, "OnRead() conn[%s] received %d bytes, appending to buf[%d], chopped out %d replies, %d incompleted bytes left"), _connDesc, nCopySize, _inCommingByteSeen, completedCmds.size(), pEnd - pProcessed);
        if (pEnd >= pProcessed)
        {
            _inCommingByteSeen = pEnd - pProcessed;
            memcpy(_inCommingBuffer, pProcessed, _inCommingByteSeen);
        }

        if (nLeftSize > 0)
        {
            _inCommingByteSeen += nLeftSize;
            buf += nLeftSize;
            memcpy((char*)&_inCommingBuffer[_inCommingByteSeen], buf, nLeftSize);
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
            pCmd->OnReply(*this, *pCmd.get(), pCmd->_replyCtx.data);
            //(new ReplyDispatcher(*this, pCmd))->start();
            cReply++;
        }
        catch(...) {}
    }

    _log(cReply ? Log::L_INFO: Log::L_DEBUG, CLOGFMT(RedisClient, "OnDataArrived() conn[%s] dispatched, took %dmsec: %d Replies"), _connDesc, (int)(TimeUtil::now() -stampNow), cReply);
}

void RedisClient::OnWrote( int status )
{
    if (status != elpeSuccess)
    {
        _log(Log::L_WARNING, CLOGFMT(RedisClient, "OnWrote() conn[%s] failed"), _connDesc);
    }
    _log(Log::L_WARNING, CLOGFMT(RedisClient, "OnWrote() conn[%s] success"), _connDesc);
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

ZQ::eloop::RedisCommand::Ptr RedisClient::sendEXPIRE( const char *key, int seconds, RedisSink::Ptr reply/*=NULL*/ )
{
    char ttls[32];
    snprintf(ttls, sizeof(ttls) - 1, " %d", seconds);
    std::string cmdstr = std::string("EXPIRE ") + key + ttls;
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

#define ident_cstr(IDENT) Evictor::identToStr(IDENT).c_str()
RedisEvictor::RedisEvictor(Sink* sink, Log& log, RedisClient::Ptr client, const std::string& name, const Properties& props)
:_sink(sink), _client(client), Evictor(log, name, props), _maxValueLen(REDIS_RECV_BUF_SIZE)
{ 
    _recvBuf = new uint8[_maxValueLen];
}

RedisEvictor::~RedisEvictor()
{
    if (_recvBuf)
    {
        delete[] _recvBuf;
        _recvBuf = NULL;
    }
}

// save a batch of streamed object to the target object store
int RedisEvictor::saveBatchToStore(StreamedList& batch)
{
    if (!_client)
        return 0;

    int cUpdated =0, cDeleted =0;
    RedisSink::Error rcerr;
    RedisCommand::Ptr cmdPtr = NULL;
    for (StreamedList::iterator it = batch.begin(); it!=batch.end(); it++)
    {
        switch(it->status)
        {
        case Item::created:   // the item is created in the cache, and has not present in the ObjectStore
        case Item::modified:  // the item is modified but not yet flushed to the ObjectStore
            cmdPtr = _client->sendSET(it->key.c_str(), (uint8*)(&it->data[0]), it->data.size());
            if (!cmdPtr) 
                Evictor::_log(Log::L_ERROR, CLOGFMT(RedisEvictor, "saveBatchToStore() save[%s] err: %s(%d)"), it->key.c_str(), RedisSink::err2str(RedisSink::rdeClientError), RedisSink::rdeClientError);
            else cUpdated++;
            break;

        case Item::destroyed:  // the item is required to destroy but not yet deleted from ObjectStore
            cmdPtr = _client->sendDEL(it->key.c_str());
            if (!cmdPtr)
                Evictor::_log(Log::L_ERROR, CLOGFMT(RedisEvictor, "saveBatchToStore() del[%s] err: %s(%d)"), it->key.c_str(), RedisSink::err2str(RedisSink::rdeClientError), RedisSink::rdeClientError);
            else cDeleted++;
            break;

        default:
            break;
        }
    }

    Evictor::_log(Log::L_DEBUG, CLOGFMT(RedisEvictor, "saveBatchToStore() %d updated and %d destroyed"), cUpdated, cDeleted);
    return cUpdated+cDeleted;
}

Evictor::Item::Ptr RedisEvictor::add( const ZQ::common::Evictor::Item::ObjectPtr& obj, const ZQ::common::Evictor::Ident& ident )
{
    Evictor::Item::Ptr item = NULL;

    // Create a new entry
    item = new Item(*this, ident);
    item->_data.status = Evictor::Item::dead;

    {
        MutexGuard lk(*item);
        item->_data.status = Evictor::Item::created;
        item->_data.servant = obj;
        item->_data.stampCreated = now();
        item->_data.stampLastSave = 0;
        // item->_data.avgSaveTime = 0;

        MutexGuard g(_lkEvictor);
        _queueModified(item); // this is only be called while item is locked
        _requeue(item); // this is only be called while item is locked
    }

    _log(Log::L_DEBUG, CLOGFMT(Evictor, "added object[%s](%s)"), ident_cstr(ident), Evictor::Item::stateToString(item->_data.status));

    return item;
}

Evictor::Item::ObjectPtr RedisEvictor::locate( const Ident& ident )
{
    {
        MutexGuard g(_lkEvictor);
        Map::iterator itCache = _cache.find(ident);
        if (_cache.end() != itCache)
        {
            Item::Ptr item = itCache->second;
            if (_sink)
            {
                _sink->OnDataResponse(ident, item->_data.servant);
                return NULL;
            }
        }
    }

    if (!_client)
    {
        if (_sink)
            _sink->OnError(ident, Evictor::eeConnectErr, "redis client is NULL");
        return NULL;
    }       

    std::string cmdstr = std::string("GET ") + ident_cstr(ident);
    ZQ::eloop::RedisCommand::Ptr pCmd = new ZQ::eloop::RedisCommand(*_client, cmdstr, REDIS_LEADINGCH_BULK, this);
    _client->sendCommand(pCmd);
    return NULL;
}

void RedisEvictor::OnRequestError( RedisClient& client, RedisCommand& cmd, RedisSink::Error errCode, const char* errDesc/*=NULL*/ )
{
    Evictor::Ident ident;
    cmdStrToIdent(ident, cmd.desc());
    if (_sink)
        _sink->OnError(ident, (int)errCode, errDesc);
}

void RedisEvictor::OnReply( RedisClient& client, RedisCommand& cmd, RedisSink::Data& data )
{
    Evictor::Ident ident;
    cmdStrToIdent(ident, cmd.desc());
    if (!_sink)
    {
        _log(Log::L_ERROR, CLOGFMT(RedisEvictor, "sink is null"));
        return;
    }

    std::vector<uint8> temp;
    uint vlen = 0;
    if (data.bulks.size() <=0)
    {
        _sink->OnError(ident, 404, "not Found");
        return ;
    }
    vlen = strlen(data.bulks[0].c_str());
    {
        ZQ::common::MutexGuard g(_lockBuf);
        vlen = RedisClient::decode(data.bulks[0].c_str(), _recvBuf, vlen);
        temp.assign(_recvBuf, _recvBuf + vlen);
    }

    Item::Ptr item = new Item(*this, ident);
    if (!unmarshal("indexHeader", item->_data, temp))
    {
        _sink->OnError(ident, 400, "unmarshal failed");
        return ;
    }
    _sink->OnDataResponse(ident, item->_data.servant);
    if (ident.category != "indexHeader")
    {
        MutexGuard g(_lkEvictor);
        _cache.insert(Evictor::Map::value_type(ident, item));
        _evictorList.push_front(ident);
        item->_pos = _evictorList.begin();
        item->_orphan = false;
    }

    Evictor::_log(Log::L_DEBUG, CLOGFMT(RedisEvictor, "loadFromStore() %d recieve len"), vlen);
}


void RedisEvictor::cmdStrToIdent( ZQ::common::Evictor::Ident& ident, const std::string& cmd )
{
    std::string strCmd = cmd;
    size_t pos = strCmd.find_first_of(" ");
    strCmd = strCmd.substr(pos+1, strCmd.length() - pos);
    pos = strCmd.find_last_of('@');
    ident.name = strCmd.substr(0, pos);
    ident.category = strCmd.substr(pos+1);
}

}} // namespaces