#include "RTSPConnection.h"
#include "TimeUtil.h"

namespace ZQ {
namespace eloop {

#define  RTSP_RECV_BUF_SIZE (8*1024)

//-------------------------------------
//	class RTSPParser
//-------------------------------------
size_t RTSPParser::parse( const char* data, size_t len)
{

}

//-------------------------------------
//	class RTSPConnection
//-------------------------------------
void RTSPConnection::doAllocate(eloop_buf_t* buf, size_t suggested_size)
{
	if (_recvBuf.base == NULL)
	{
		_recvBuf.len = RTSP_RECV_BUF_SIZE;
		_recvBuf.base = (char*)malloc(_recvBuf.len);
	}

	if (_recvBuf.base)
	{

		if ((_recvBuf.len - _byteSeen) <=0)
		{
			_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPConnection, "doAllocate() conn[%s] last incomplete message exceed bufsz[%d] from offset[%d], give it up"), hint(), _recvBuf.len, _byteSeen);
			_byteSeen =0;
			memset(_recvBuf.base,0,_recvBuf.len);
		}

		buf->base = _recvBuf.base + _byteSeen;
		buf->len = _recvBuf.len - _byteSeen;
	}
}

void RTSPConnection::OnRead(ssize_t nread, const char *buf)
{
	if (nread < 0)
	{
		std::string desc = "Read error:";
		desc.append(errDesc(nread));
		onError(nread,desc.c_str());
		return;
	}
	if (nread == 0)
		return;

	_Logger.hexDump(ZQ::common::Log::L_DEBUG, _recvBuf.base+_byteSeen, nread, hint().c_str());

	parse(nread);
}

void RTSPConnection::parse(ssize_t bytesRead)
{
	RTSPMessage::MsgVec receivedResps, receivedReqs;

	char* pProcessed = _recvBuf.base, *pEnd = _recvBuf.base + _byteSeen + bytesRead;
	bool bFinishedThisDataChuck = false;
	int64 stampNow = ZQ::common::now();

	while ((pProcessed < pEnd && !bFinishedThisDataChuck) || (_currentParseMsg.headerCompleted && _currentParseMsg.pMsg->contentLength()==0))
	{
		if (_currentParseMsg.headerCompleted)
		{
			// read the data as the content body of the current message
			// step 1. determin the length to read
			int64 len = 0;
			if (_currentParseMsg.pMsg->contentLength() >0) 
				len = _currentParseMsg.pMsg->contentLength() - _currentParseMsg.contentBodyRead;

			if (len > pEnd - pProcessed)
				len = pEnd - pProcessed;

			//			if (pEnd - pProcessed < len)
			//			{
			//				bFinishedThisDataChuck = true;
			//				break;
			//			}

			_currentParseMsg.pMsg->appendBody(pProcessed,len);
			pProcessed += len;
			_currentParseMsg.contentBodyRead += len;

			if (_currentParseMsg.contentBodyRead < _currentParseMsg.pMsg->contentLength())
			{
				_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPConnection, "parse() conn[%s] incompleted message left, appended[%lld], Content-Length[%lld/%lld]"), hint().c_str(), len, _currentParseMsg.contentBodyRead, _currentParseMsg.pMsg->contentLength());
				continue;
			}

			// the current message has been read completely when reach here

			// check the header CSeq
			if (_currentParseMsg.pMsg->cSeq() <= 0)
			{
				_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPConnection, "parse() conn[%s] ignore illegal response withno CSeq"), hint().c_str());
				_currentParseMsg.reset();
				continue;
			}

			if (_currentParseMsg.pMsg->getMsgType() == RTSPMessage::RTSP_MSG_REQUEST)
				receivedReqs.push_back(_currentParseMsg.pMsg);
			else
				receivedResps.push_back(_currentParseMsg.pMsg);

			_currentParseMsg.reset();
			continue;
		}

		// beginning of header reading
		while (!bFinishedThisDataChuck && pProcessed < pEnd)
		{
			char* line = nextLine(pProcessed, pEnd - pProcessed);
			if (NULL == line)
			{
				// met an incompleted line, shift it to the beginning of buffer then wait for the next OnDataArrived()
				bFinishedThisDataChuck = true;
				break;
			}

			int len = strlen(line);
			pProcessed += (len + 2); // skip /r/n

			if (len <=0) // an empty line
			{
				if (!_currentParseMsg.startLine.empty())
				{
					_currentParseMsg.headerCompleted = true;
					// finished this header reading
					break;
				}

				continue; // sounds like a bad line here
			}

			if (_currentParseMsg.startLine.empty())
			{
				_currentParseMsg.startLine = line;
				//				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [%s]"), connDescription(), _pCurrentMsg->startLine.c_str());

				char* delim = " \r\n";
				char* token = strtok(line, delim);
				if (strncmp(token, "RTSP", 4) == 0)
				{
					_currentParseMsg.pMsg->setMsgType(RTSPMessage::RTSP_MSG_RESPONSE);
					_currentParseMsg.pMsg->version(token);
					token = strtok(NULL, delim);
					_currentParseMsg.pMsg->code(atoi(token));
					token = strtok(NULL, delim);
					_currentParseMsg.pMsg->status(token);

				}
				else
				{
					_currentParseMsg.pMsg->setMsgType(RTSPMessage::RTSP_MSG_REQUEST);
					_currentParseMsg.pMsg->method(token);
					token = strtok(NULL, delim);
					_currentParseMsg.pMsg->url(token);
					token = strtok(NULL, delim);
					_currentParseMsg.pMsg->version(token);
				}
				continue;
			}

			std::string header, value;
			char* pos = strchr(line, ':');
			if (NULL ==pos)
				continue; // illegal header

			*pos = '\0';
			header = RTSPConnection::trim(line);
			value = RTSPConnection::trim(pos+1);
			_currentParseMsg.pMsg->header(header,value);


			if (0 == header.compare("Content-Length"))
				_currentParseMsg.pMsg->contentLength(atol(value.c_str()));
			// 			else if(0 == header.compare("CSeq"))
			// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [CSeq: %s]"), connDescription(), _pCurrentMsg->headers["CSeq"].c_str());
			// 			else if(0 == header.compare("Method-Code"))
			// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [Method-Code: %s]"), connDescription(), _pCurrentMsg->headers["Method-Code"].c_str());
			// 			else if(0 == header.compare("Session"))
			// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [Session: %s]"), connDescription(), _pCurrentMsg->headers["Session"].c_str());
		}; // end of header reading;
	}

	if (pEnd >= pProcessed)
	{
		_byteSeen = pEnd - pProcessed;
		memcpy(_recvBuf.base, pProcessed, _byteSeen);
	}
}

std::string RTSPConnection::trim(char const* str)
{
	if (NULL == str)
		return "";
	int len = strlen(str);
	// The line begins with the desired header name.  Trim off any whitespace
	const char* t =str + len;
	for (; *str == ' ' || *str == '\t'; str++);
	for (; *(t-1) == ' ' || *(t-1) == '\t'; t--);
	return std::string(str, t-str);
}

char* RTSPConnection::nextLine(char* startOfLine, int maxByte)
{
	// returns the start of the next line, or NULL if none.  Note that this modifies the input string to add '\0' characters.
	// locate the beginning of the line
	for (; (*startOfLine =='\0' || *startOfLine == '\n') && maxByte >0; startOfLine++, maxByte--);

	// locate the end of line
	char* ptr = startOfLine;
	for (; maxByte >0 && *ptr != '\r' && *ptr != '\n'; ++ptr, maxByte--);

	if (maxByte<=0)
		return NULL;

	// change the "\r\n" as the string NULL
	if (*ptr == '\r')
		*ptr = '\0';

	return startOfLine;
}

void RTSPConnection::OnWrote(int status)
{
	if (status != elpeSuccess)
	{
		std::string desc = "send error:";
		desc.append(errDesc(status));
		onError(status,desc.c_str());
		return;
	}

//	onHttpDataSent(status);
}

} }//namespace ZQ::eloop
