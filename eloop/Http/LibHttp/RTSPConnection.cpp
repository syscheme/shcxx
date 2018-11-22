#include "RTSPConnection.h"
#include "TimeUtil.h"
#include "urlstr.h"
#include <vector>
#include <algorithm>

namespace ZQ {
namespace eloop {

const char* RTSPMessage::methodToStr(RequestMethod method)
{
	switch(method)
	{
	case mtdSETUP: return "SETUP";
	case mtdPLAY: return "PLAY";
	case mtdPAUSE: return "PAUSE";
	case mtdTEARDOWN: return "TEARDOWN";
	case mtdGET_PARAMETER: return "GET_PARAMETER";
	case mtdSET_PARAMETER: return "SET_PARAMETER";
	case mtdDESCRIBE: return "DESCRIBE";
	case mtdANNOUNCE: return "ANNOUNCE";
	case mtdOPTIONS: return "OPTIONS";

	case mtdUNKNOWN:
	default: break;
	}

	return "UNKNOWN";
}

RTSPMessage::RequestMethod RTSPMessage::strToMethod(const char* method)
{
	if (0 == stricmp(method, "SETUP")) return mtdSETUP;
	if (0 == stricmp(method, "PLAY")) return mtdPLAY;
	if (0 == stricmp(method, "PAUSE")) return mtdPAUSE;
	if (0 == stricmp(method, "TEARDOWN")) return mtdTEARDOWN;
	if (0 == stricmp(method, "GET_PARAMETER")) return mtdGET_PARAMETER;
	if (0 == stricmp(method, "SET_PARAMETER")) return mtdSET_PARAMETER;
	if (0 == stricmp(method, "DESCRIBE")) return mtdDESCRIBE;
	if (0 == stricmp(method, "ANNOUNCE")) return mtdANNOUNCE;
	if (0 == stricmp(method, "OPTIONS")) return mtdOPTIONS;

	return mtdUNKNOWN;
}

#define  RTSP_RECV_BUF_SIZE (8*1024)
//-------------------------------------
//	class RTSPConnection
//-------------------------------------
void RTSPConnection::OnConnected(ElpeError status)
{
	if (status != ZQ::eloop::Handle::elpeSuccess)
	{
		std::string desc = "connect error:";
		desc.append(ZQ::eloop::Handle::errDesc(status));
		desc = hint() + desc;
		onError(status,desc.c_str());
		return;
	}

	start();

	for (RTSPMessage::MsgVec::iterator it = _reqList.begin();it != _reqList.end();)
	{
		sendRequest(*it);
		_reqList.erase(it++);
	}

	_reqList.clear();
}

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
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPConnection, "doAllocate() conn[%s] last incomplete message exceed bufsz[%d] from offset[%d], give it up"), hint().c_str(), _recvBuf.len, _byteSeen);
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
		desc = hint() + desc;
		onError(nread, desc.c_str());
		return;
	}

	if (nread == 0)
		return;

	if (TCPConnection::_enableHexDump > 0)
		_logger.hexDump(ZQ::common::Log::L_INFO, _recvBuf.base+_byteSeen, nread, (_connId + hint()+ " OnRead").c_str(),true);


	onDataReceived(nread);

	parse(nread);
}

int RTSPConnection::sendRequest(RTSPMessage::Ptr req, int64 timeout, bool expectResp)
{
	if (!_isConnected)
	{
		_reqList.push_back(req);

		std::string url = req->url();
		ZQ::common::URLStr urlstr(url.c_str());
		const char* host = urlstr.getHost();

		connect4(host,urlstr.getPort());
		return 0;
	}

	uint cseq = lastCSeq();
	req->cSeq(cseq);

	if (expectResp)
	{
		AwaitRequest ar;
		ar.req = req;
		_timeout = (timeout > 0)?timeout:_timeout;
		ar.expiration = ZQ::common::now() + _timeout;

		ZQ::common::MutexGuard g(_lkAwaits);
		_awaits.insert(AwaitRequestMap::value_type(req->cSeq(), ar));
	}

	OnRequestPrepared(req);
	std::string reqStr = req->toRaw();
	int ret = write(reqStr.c_str(), reqStr.size());
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPConnection, "sendRequest() conn[%s] msg[%s]"), hint().c_str(),reqStr.c_str());

	if (ret < 0)
	{
		OnRequestDone(cseq,ret);
		return ret;
	}

	return cseq;
}

void RTSPConnection::parse(ssize_t bytesRead)
{
	RTSPMessage::MsgVec receivedResps, receivedReqs;

	char* pProcessed = _recvBuf.base, *pEnd = _recvBuf.base + _byteSeen + bytesRead;
	bool bFinishedThisDataChuck = false;
	int64 stampNow = ZQ::common::now();
	int64 size = _byteSeen + bytesRead;

	while (((pProcessed < pEnd) && !bFinishedThisDataChuck) || (_currentParseMsg.headerCompleted && _currentParseMsg.pMsg->contentLength()==0))
	{

		if (_currentParseMsg.headerCompleted)
		{
			// read the data as the content body of the current message
			// step 1. determin the length to read
			int len = 0;
			if (_currentParseMsg.pMsg->contentLength() >0) 
				len = _currentParseMsg.pMsg->contentLength() - _currentParseMsg.contentBodyRead;

			if (pEnd <= pProcessed)
				len = 0;
			else
			{
				if (len > (pEnd - pProcessed))
					len = (int)(pEnd - pProcessed);
			}

			//			if (pEnd - pProcessed < len)
			//			{
			//				bFinishedThisDataChuck = true;
			//				break;
			//			}

			_currentParseMsg.pMsg->appendBody(pProcessed, len);
			pProcessed += len;
			_currentParseMsg.contentBodyRead += len;

			if (_currentParseMsg.contentBodyRead < _currentParseMsg.pMsg->contentLength())
			{
				_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPConnection, "parse() conn[%s] incompleted message left, appended[%zd], Content-Length[%d/%d]"), hint().c_str(), len, (int)_currentParseMsg.contentBodyRead, _currentParseMsg.pMsg->contentLength());
				continue;
			}

			// the current message has been read completely when reach here

			// check the header CSeq
			if (_currentParseMsg.pMsg->cSeq() <= 0)
			{
				_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPConnection, "parse() conn[%s] ignore illegal response withno CSeq"), hint().c_str());
				_currentParseMsg.reset();
				continue;
			}

			if (!parseStartLine(_currentParseMsg.startLine, _currentParseMsg.pMsg))
			{
				std::string desc = "parse start line error:";
				if (_currentParseMsg.startLine.empty())
					desc = "start line is null:";

				char descBuf[4096];
				unsigned char* data = (unsigned char* )_recvBuf.base;
				int bufPos = 0;
				bufPos += sprintf(&descBuf[bufPos], "%s %s startLine:%s msg:",hint().c_str(), desc.c_str(), _currentParseMsg.startLine.c_str());

				int bufLeft = sizeof(descBuf) - bufPos - 8;

				for (int i = 0; i <size	&& i < bufLeft; i++)
					descBuf[bufPos ++] = (' '==data[i] || isgraph(data[i])) ? data[i] : '.';
				descBuf[bufPos++] = '\0';

				onError(ParseStartLineError, descBuf);
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
			char* line = nextLine(pProcessed, (int)(pEnd - pProcessed));
			if (NULL == line)
			{
				// met an incompleted line, shift it to the beginning of buffer then wait for the next OnDataArrived()
				bFinishedThisDataChuck = true;
				break;
			}

			size_t len = strlen(line);
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
				_currentParseMsg.pMsg->_stampCreated = ZQ::common::now();
				_currentParseMsg.startLine = line;
/*				//				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [%s]"), connDescription(), _pCurrentMsg->startLine.c_str());

				char* delim = " \r\n";
				char* token = strtok(line, delim);
				if (strncmp(token, "RTSP", 4) == 0)
				{
					_currentParseMsg.pMsg->setMsgType(RTSPMessage::RTSP_MSG_RESPONSE);
					_currentParseMsg.pMsg->version(token);
					token = strtok(NULL, delim);
					if (token == NULL)
					{
						std::string desc = "parse start line step1 error:";
						desc +=_currentParseMsg.startLine;
						onError(ParseStartLineError, desc.c_str());
						continue;
					}

					_currentParseMsg.pMsg->code(atoi(token));
					token = strtok(NULL, delim);
					if (token == NULL)
					{
						std::string desc = "parse start line step2 error:";
						desc +=_currentParseMsg.startLine;
						onError(ParseStartLineError, desc.c_str());
						continue;
					}

					_currentParseMsg.pMsg->status(token);

				}
				else
				{
					if (token == NULL)
					{
						std::string desc = "parse start line step3 error:";
						desc +=_currentParseMsg.startLine;
						onError(ParseStartLineError, desc.c_str());
						continue;
					}
					_currentParseMsg.pMsg->setMsgType(RTSPMessage::RTSP_MSG_REQUEST);
					_currentParseMsg.pMsg->method(token);
					token = strtok(NULL, delim);
					if (token == NULL)
					{
						std::string desc = "parse start line step4 error:";
						desc +=_currentParseMsg.startLine;
						onError(ParseStartLineError, desc.c_str());
						continue;
					}
					_currentParseMsg.pMsg->url(token);
					token = strtok(NULL, delim);
					if (token == NULL)
					{
						std::string desc = "parse start line step5 error:";
						desc +=_currentParseMsg.startLine;
						onError(ParseStartLineError, desc.c_str());
						continue;
					}
					_currentParseMsg.pMsg->version(token);
				}
*/
				continue;
			}

			std::string header, value;
			char* pos = strchr(line, ':');
			if (NULL ==pos)
				continue; // illegal header

			*pos = '\0';
			header = RTSPConnection::trim(line);
			value = RTSPConnection::trim(pos+1);
			if (!header.empty() && !value.empty())
				_currentParseMsg.pMsg->header(header,value);

			if (0 == header.compare(Header_ContentLength))
				_currentParseMsg.pMsg->contentLength(atol(value.c_str()));

			if (0 == header.compare(Header_CSeq))
				_currentParseMsg.pMsg->cSeq(atol(value.c_str()));
			// 			else if(0 == header.compare("CSeq"))
			// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [CSeq: %s]"), connDescription(), _pCurrentMsg->headers["CSeq"].c_str());
			// 			else if(0 == header.compare("Method-Code"))
			// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [Method-Code: %s]"), connDescription(), _pCurrentMsg->headers["Method-Code"].c_str());
			// 			else if(0 == header.compare("Session"))
			// 				_log(Log::L_DEBUG, CLOGFMT(RTSPClient, "OnDataArrived() conn[%s] received data [Session: %s]"), connDescription(), _pCurrentMsg->headers["Session"].c_str());
		}// end of header reading;
	}

	if (pEnd > pProcessed)
	{
		_byteSeen = int(pEnd - pProcessed);
		memcpy(_recvBuf.base, pProcessed, _byteSeen);
	}
	else
	{
		_byteSeen = 0;
		memset(_recvBuf.base,0,_recvBuf.len);
	}

	// notifying the sink
	if (receivedResps.size() > 0)
	{
		::std::sort(receivedResps.begin(), receivedResps.end(), RTSPMessage::less);
		for (RTSPMessage::MsgVec::iterator itResp = receivedResps.begin(); itResp != receivedResps.end(); itResp++)
		{
			RTSPMessage::Ptr resp = (*itResp);
			if (!resp)
			{
				_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPConnection, "resp is NULL"));
				continue;
			}

			resp->setConnId(_connId);
			int cseq = resp->cSeq();
			ZQ::common::MutexGuard g(_lkAwaits);
			AwaitRequestMap::iterator itW = _awaits.find(cseq);
			if (_awaits.end() == itW) // unknown request or it has been previously expired
			{
				_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPConnection, "unknown request or it has been previously expired. cseq(%d)"),cseq);
				continue;
			}

			int64 stampNow = ZQ::common::now();
			OnResponse(resp);
			int elapsed = (int) (ZQ::common::now() - stampNow);
			int err = 200;
			if (resp)
				err = resp->code();

			_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPConnection, "OnResponse() %s(%d) ret(%d) took %dmsec triggered, cleaning from await list"), RTSPMessage::methodToStr(resp->method()), cseq, err, elapsed);

			OnRequestDone(cseq, err);
			_awaits.erase(cseq);
		}
	}

	for (int i=0; i< receivedReqs.size(); i++)
	{
		receivedReqs[i]->setConnId(_connId);
		OnRequest(receivedReqs[i]);
	}
}

bool RTSPConnection::parseStartLine(const std::string& startLine, RTSPMessage::Ptr& pMsg)
{
	if (startLine.empty())
		return false;

	char* buf = new char[startLine.length()+2];
	strcpy(buf, startLine.c_str());
	bool bSucc =true;

	do 
	{
		char* p = buf, *q=p;
		while (*p == ' ' || *p == '\t')	p++; // skip the leading white spaces
		for (q=p; *q && *q!= ' ' && *q != '\t'; q++); // find the next white spaces

		if (*q)
			*q++ = '\0';

		std::string cmdName = trim(p);
		if (NULL == (p = strstr(q, "RTSP/")))	//resp
		{
			if (cmdName.find("RTSP/") <= 0)
			{
				bSucc = false;
				break;
			}

			for (p=q; *p && *p!= ' ' && *p != '\t'; p++); // find the next white spaces
			std::string temp(q, p-q);
			uint resultCode=atoi(temp.c_str());
			std::string resultStr = trim(p);

			pMsg->setMsgType(RTSPMessage::RTSP_MSG_RESPONSE);
			pMsg->status(resultStr);
			pMsg->code(resultCode);
			pMsg->version(cmdName);
		}
		else		//req
		{
			*(p-1) = '\0';

			std::string url = trim(q);
			std::string proto = trim(p);

			pMsg->setMsgType(RTSPMessage::RTSP_MSG_REQUEST);
			pMsg->method(RTSPMessage::strToMethod(cmdName.c_str()));
			pMsg->url(url);
			pMsg->version(proto);
		}
	} while (0);

	delete [] buf;
	return bSucc;

/*
	char* delim = " \r\n";
	char* token = strtok((char*)startLine.c_str(), delim);
	if (token == NULL)
		return false;
	if (strncmp(token, "RTSP", 4) == 0)
	{
		pMsg->setMsgType(RTSPMessage::RTSP_MSG_RESPONSE);
		pMsg->version(token);
		token = strtok(NULL, delim);
		if (token == NULL)
			return false;

		pMsg->code(atoi(token));
		token = strtok(NULL, delim);
		if (token == NULL)
			return false;

		pMsg->status(token);

	}
	else
	{
		pMsg->setMsgType(RTSPMessage::RTSP_MSG_REQUEST);
		pMsg->method(token);
		token = strtok(NULL, delim);
		if (token == NULL)
			return false;
		
		pMsg->url(token);
		token = strtok(NULL, delim);
		if (token == NULL)
			return false;
		pMsg->version(token);
	}
	return true;
*/
}


std::string RTSPConnection::trim(char const* str)
{
	if (NULL == str)
		return "";

	size_t len = strlen(str);
	// The line begins with the desired header name.  Trim off any whitespace
	const char* t =str + len;
	for (; *str == ' ' || *str == '\t'; str++);
	for (; *(t-1) == ' ' || *(t-1) == '\t'; t--);

	if (t <= str)
		return "";

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
	if (*ptr == '\r' || *ptr == '\n')
		*ptr = '\0';

	return startOfLine;
}

void RTSPConnection::OnWrote(int status)
{
	if (status != elpeSuccess)
	{
		std::string desc = std::string("send failed: ") + hint() + " " + errDesc(status);
		onError(status,desc.c_str());
		return;
	}

	onDataSent(status);
}

void RTSPConnection::OnTimer()
{
	std::vector<uint> expiredList;

	{
		ZQ::common::MutexGuard g(_lkAwaits);
		for (AwaitRequestMap::iterator itW = _awaits.begin(); itW != _awaits.end();)
		{
			if (itW->second.expiration < ZQ::common::now())
			{
				expiredList.push_back(itW->first);
				_awaits.erase(itW++);
				continue;
			}
			itW++;
		}
	} // end of _lkAwaits

	for (size_t i =0; i < expiredList.size(); i++)
		OnRequestDone(expiredList[i], RtspReqTimeout);
}

} }//namespace ZQ::eloop
