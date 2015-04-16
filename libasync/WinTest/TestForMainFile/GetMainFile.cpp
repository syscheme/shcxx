#include "GetMainFile.h"
#include <Text.h>
#include "TimeUtil.h"
#include "InetAddr.h"
#include "strHelper.h"
#include <sstream>

namespace ZQ{
namespace StreamService{
static long StringToLong(const std::string& s)
{
#ifdef ZQ_OS_MSWIN
	return ::_strtoi64(s.c_str(), NULL, 10);
#elif defined(__x86_64)
	return strtol(s.c_str(), NULL, 10);
#else
	return strtoll(s.c_str(), NULL, 10);
#endif
}

C2ClientAsync::C2ClientAsync(ZQ::common::Log& log, const std::string upStreamIP, const std::string clientTransfer, const std::string& addr, const unsigned int port, const int timeout)
:LibAsync::HttpClient(), LibAsync::Timer(getLoop()),
_log(log), _upStreamIP(upStreamIP), _clientTransfer(clientTransfer), _addr(addr), _port(port), _timeout(timeout), 
_reqBody(""),
_respBody(""),
_status(ZQ::StreamService::Initialize),
_bBodySend(false),
_bIndex(false)
{
    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "C2ClientAsync() constructor entry [%p]"), this);
    
    for (int i=0; i<8; i++)
    {
        LibAsync::AsyncBuffer buf;
        buf.len = 8* 1024;
        buf.base = (char*)malloc(sizeof(char)* buf.len);
        _recvBufs.push_back(buf);
    }

    if (_clientTransfer.empty())
    {
        _clientTransfer = ZQ::common::InetHostAddress::getLocalAddress().getHostAddress();
        _log(ZQ::common::Log::L_WARNING, CLOGFMT(C2ClientAsync, "client transfer is empty, set it randomly[%s]"), _clientTransfer.c_str());
    }

    if (_upStreamIP.empty())
    {
        _upStreamIP = ZQ::common::InetHostAddress::getLocalAddress().getHostAddress();
        _log(ZQ::common::Log::L_WARNING, CLOGFMT(C2ClientAsync, "up stream ip is empty, set it randomly[%s]"), _upStreamIP.c_str());
    }

    //open file for write
    _file = new std::ofstream();
    _file->open("result", std::ios::trunc|std::ios::binary);
    if (!_file->is_open())
    {
        printf("open file failed\n");
    }

}

C2ClientAsync::~C2ClientAsync()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "C2ClientAsync() destructor entry [%p]"), this);

    LibAsync::AsyncBufferS::iterator it = _recvBufs.begin();
    for (; it != _recvBufs.end(); it++)
    {
        delete it->base;
    }

    _file->close();
    delete _file;
}

std::string C2ClientAsync::generateLocateBody(AttrMap& reqProp)
{
	std::ostringstream sendBodyMsg;

	_transferRate = reqProp[C2CLIENT_TransferRate].empty()			\
		? "20000000" : reqProp[C2CLIENT_TransferRate];

	std::string  ingressCapacity = reqProp[C2CLIENT_IngressCapacity].empty()	\
		? "16512000000" : reqProp[C2CLIENT_IngressCapacity];

    std::string clientTransfer = _clientTransfer;

	std::string exclusionList = reqProp[C2CLIENT_ExclusionList];
	std::string range = reqProp[C2CLIENT_Range];

	std::string transferDelay = reqProp[C2CLIENT_TransferDelay].empty()			\
		? "-2000" : reqProp[C2CLIENT_TransferDelay];

	sendBodyMsg << "<LocateRequest>" << "\r\n";
	sendBodyMsg << "<Object>" << "\r\n";
	sendBodyMsg << "<Name>" << "\r\n";

	if (_paid.empty()) return 0;
	sendBodyMsg << "<AssetID>" << _paid << "</AssetID>" << "\r\n";

	if (_pid.empty()) return 0;
	sendBodyMsg << "<ProviderID>" << _pid << "</ProviderID>" << "\r\n";

	sendBodyMsg << "</Name>" << "\r\n";

	if (_subType.empty()) return 0;
	sendBodyMsg << "<SubType>" << _subType << "</SubType>" << "\r\n";

	sendBodyMsg << "</Object>" << "\r\n";

	if (_transferRate.empty()) return 0;
	sendBodyMsg << "<TransferRate>" << _transferRate << "</TransferRate>" << "\r\n";

	if (ingressCapacity.empty()) return 0;
	sendBodyMsg << "<IngressCapacity>" << ingressCapacity << "</IngressCapacity>" << "\r\n";

	if (clientTransfer.empty()) return 0;
	sendBodyMsg << "<ClientTransfer>" << clientTransfer << "</ClientTransfer>" << "\r\n";

	sendBodyMsg << "<ExclusionList>" << exclusionList << "</ExclusionList>" << "\r\n";

	sendBodyMsg << "<Range>" << range << "</Range>" << "\r\n";

	sendBodyMsg << "<TransferDelay>" << transferDelay << "</TransferDelay>" << "\r\n";

	sendBodyMsg << "</LocateRequest>";

	return sendBodyMsg.str();
}

int64 C2ClientAsync::setTimeout(int64 timeout)
{
	_timeout = timeout;
	return _timeout;
}

bool C2ClientAsync::sendLocateRequest(const std::string& url, const std::string& contentName, const std::string& subType, AttrMap& reqProp)
{
    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "sendLocateRequest() entry"));

    _startTime = ZQ::common::TimeUtil::now();
    if (alive())
    {
        close();
    }

    _status = ZQ::StreamService::NotConnected;

    _reqBody  = "";
    _respBody = "";

    if (url.empty())
    {
        _status = ZQ::StreamService::Failure;
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "sendLocateRequest() url is empty"));
        return false;
    }

    if (subType.empty())
    {
        _status = ZQ::StreamService::Failure;
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "sendLocateRequest() subtype is empty"));
        return false;
    }
    if ("index" == subType)
    {
        _bIndex = true;
    }

    _bLocateRequest = true;
    _url	 = url;
    _subType = subType;
    _sendMap = reqProp;

    _contentName = contentName;
    //get paid and pid from content name
    if (contentName.size() <= 20)
    {
        _status = ZQ::StreamService::Failure;
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "sendLocateRequest() Couldn't recognize content name [%s], wrong format"), contentName.c_str());
        return false;
    }
    size_t startAssetID;
    size_t startProvideID;
    size_t lastSlashPos = contentName.find_last_of('/');
    if (std::string::npos == lastSlashPos)
    {//no slash
        startAssetID = 0;
    }
    else{
        startAssetID = lastSlashPos + 1;
    }
    _paid = contentName.substr(startAssetID, 20);

    if ('_' == contentName.at(startAssetID + 20))
    {
        startProvideID = startAssetID + 20 + 1;
    }else{
        startProvideID = startAssetID + 20;
    }
    _pid = contentName.substr(startProvideID);

    std::string host		= reqProp[C2CLIENT_HOST].empty() ? "None" : reqProp[C2CLIENT_HOST];
    std::string userAgent	= reqProp[C2CLIENT_UserAgent].empty() ? "ToInfinityAndBeyond" : reqProp[C2CLIENT_UserAgent];
    std::string contentType = reqProp[C2CLIENT_ContentType].empty() ? "text/xml-external-parsed-entity" : reqProp[C2CLIENT_ContentType];

    LibAsync::HttpMessagePtr locateMsgPtr = new LibAsync::HttpMessage(HTTP_REQUEST);
    locateMsgPtr->method(HTTP_POST);
    locateMsgPtr->url(url);

    //add header
    locateMsgPtr->header(C2CLIENT_HOST, host);
    locateMsgPtr->header(C2CLIENT_UserAgent, userAgent);
    locateMsgPtr->header(C2CLIENT_ContentType, contentType);

    //generate body
    _reqBody = generateLocateBody(reqProp);
    locateMsgPtr->contentLength(_reqBody.size());

    //check if timeout
    if(!checkTimeout())
    {
        return false;
    }

    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "begin send locate request endpoint[%s:%d]"), _addr.c_str(), _port);

    //bind up stream ip
    if (!bind(_upStreamIP, 0))
    {
        _status = ZQ::StreamService::Failure;
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "bind up stream ip[%s] failed"), _upStreamIP.c_str());
        return false;
    }
    
    if (!beginRequest(locateMsgPtr, _addr, _port))
    {
        _status = ZQ::StreamService::Failure;
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "failed to send locate request to [%s:%d]"),_addr.c_str(), _port);
        return false;
    }
    return true;
}

bool C2ClientAsync::sendGetRequest(const std::string& url, const std::string& contentName, AttrMap& prop, std::string addr, unsigned int port)
{
    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "sendGetRequest() entry, TransferID[%s] Content Name[%s] Transfer endpoint[%s:%d], client[%p]"), url.c_str(), contentName.c_str(), addr.c_str(), port, this);

    if (alive())
    {
        close();
    }

    _status = ZQ::StreamService::NotConnected;

    _bLocateRequest = false;
    _reqBody  = "";
    _respBody = "";

    if (url.empty())
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "sendGetRequest() url is empty"));
        _status = ZQ::StreamService::Failure;
        return false;
    }

    _contentName = contentName;

    //get paid and pid from content name
    if (contentName.size() <= 20)
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "sendGetRequest() Couldn't recognize content name [%s], wrong format"), contentName.c_str());
        return false;
    }
    size_t startAssetID;
    size_t startProvideID;
    size_t lastSlashPos = contentName.find_last_of('/');
    if (std::string::npos == lastSlashPos)
    {//no slash
        startAssetID = 0;
    }
    else{
        startAssetID = lastSlashPos + 1;
    }
    _paid = contentName.substr(startAssetID, 20);

    if ('_' == contentName.at(startAssetID + 20))
    {
        startProvideID = startAssetID + 20 + 1;
    }else{
        startProvideID = startAssetID + 20;
    }
    _pid = contentName.substr(startProvideID);

    LibAsync::HttpMessagePtr getMsgPtr = new LibAsync::HttpMessage(HTTP_REQUEST);
    getMsgPtr->method(HTTP_GET);

    std::string strUrl = url;
    if (strUrl.at(0) != '/')
    {
        strUrl = "/" + strUrl;
    }
    getMsgPtr->url(strUrl);

    getMsgPtr->header(C2CLIENT_Range, _sendMap[C2CLIENT_Range]);
    getMsgPtr->header(C2CLIENT_UserAgent, _sendMap[C2CLIENT_UserAgent]);
    getMsgPtr->header(C2CLIENT_HOST, _sendMap[C2CLIENT_HOST]);
    getMsgPtr->header("Transfer-Delay", _sendMap[C2CLIENT_TransferDelay]);
    getMsgPtr->header("Ingress-Capacity", _sendMap[C2CLIENT_IngressCapacity]);

    //check if timeout
    if(!checkTimeout())
    {
        return false;
    }

    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "begin send get request endpoint[%s:%d]"), addr.c_str(), port);

    //bind up stream ip
    if (!bind(_upStreamIP, 0))
    {
        _status = ZQ::StreamService::Failure;
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "bind up stream ip[%s] failed"), _upStreamIP.c_str());
        return false;
    }

    if (!beginRequest(getMsgPtr, addr, port))
    {
        _status = ZQ::StreamService::Failure;
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "failed to send get request to [%s:%d]"),addr.c_str(), port);
        return false;
    }

    return true;
}

bool C2ClientAsync::checkTimeout()
{
    int64 now = ZQ::common::TimeUtil::now();
    int64 remainTime = _timeout - (now - _startTime);

    if (remainTime > 0)
    {
        update(remainTime);
        return true;
    }
    else{
        onTimer();
        return false;
    }
}

bool C2ClientAsync::parseIndex(std::string& contentName, const char* indexData, size_t dataSize, ZQ::IdxParser::IndexData& idxData)
{
	ZQ::IdxParser::IdxParserEnv			idxParserEnv;
	idxParserEnv.AttchLogger(&_log);
	ZQ::IdxParser::IndexFileParser		idxParser(idxParserEnv);

	if(!idxParser.ParseIndexFromMemory( contentName, idxData, indexData, dataSize ) ) 
	{
        _status = ZQ::StreamService::Failure;
        _log(ZQ::common::Log::L_ERROR,CLOGFMT(C2ClientAsync,"parseIndex() failed to parse index data for[%s], data size[%u]"),
			contentName.c_str(), (uint32)dataSize);
		return false;
	}
	return true;
}

void C2ClientAsync::onReqMsgSent( size_t size)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "entry onReqMsgSent(), send size[%ld], client[%p]"), size, this);
	
	_status = ZQ::StreamService::HeaderSendCompleted;

    cancel();

	if(!_bBodySend && !_reqBody.empty()) {
		LibAsync::AsyncBuffer buf;
		buf.base = (char*)_reqBody.c_str();
		buf.len = _reqBody.length();
		_bBodySend = true;
       
        //check if timeout
        if(!checkTimeout())
        {
            return;
        }
        
		sendReqBody(buf);
        _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "start send body, client[%p]"), this);
	} else if(_bBodySend && !_reqBody.empty()) {
		_reqBody.clear();
		//mRequest = NULL;
        //check if timeout
        if(!checkTimeout())
        {
            return;
        }
		endRequest();
        _startCalcLatency = ZQ::common::TimeUtil::now();
        _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "end send body, client[%p]"), this);
	} else {
		_status = ZQ::StreamService::BodySendCompleted;
        //check if timeout
        if(!checkTimeout())
        {
            return;
        }
		getResponse();
        _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "get response, client[%p]"), this);
	}
}

void C2ClientAsync::onHttpDataReceived( size_t size )
{
	//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "entry onHttpDataReceived(), received size[%d]"), size);

    cancel();

	if (ZQ::StreamService::Completed != _status)
	{
        //check if timeout
        update(_timeout);

		recvRespBody(_recvBufs);
	}
    else{
        //received successful
        if (!_bLocateRequest)
        {
            _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "received all data"));
        }
        int statusCode = _headerMsg->code();	

        if (2 != statusCode/100)
        {
            _status = ZQ::StreamService::Failure;
            std::string phaseName = _bLocateRequest ? "locate" : "get";
            _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "send %s request failure, error[%d:%s]"), phaseName.c_str(), statusCode, _headerMsg->status().c_str());		

            printf("request failure: %d %s", statusCode, _headerMsg->status().c_str());
            return;
        }

        if (_bLocateRequest) //locate request successfully
        {
            _log(ZQ::common::Log::L_INFO, CLOGFMT(C2ClientAsync, "locate request latency[%ld]"), ZQ::common::TimeUtil::now()-_startCalcLatency);

            LocateResponseData respData;
            SimpleXMLParser parser;
            try
            {
                parser.parse(_respBody.data(), _respBody.size(), 1);
            }
            catch (const ZQ::common::ExpatException& e)
            {
                _status = ZQ::StreamService::Failure;
                _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parse response body catch exception, ExpatException: [%s] during parsing http response body"), e.getString());

                return;
            }


            std::string error;
            error.reserve(2048);
            if(!parseResponse(&parser.document(), respData, error))
            {//error
                _status = ZQ::StreamService::Failure;
                _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientSync, "parse response body failure, %s"), error.c_str());
                return;
            }

            //for read, send get request
            _sessionID = respData.transferId;
            std::string addr  = respData.transferPort;
            unsigned int port;
            if (respData.portNum.empty())
            {
                _log(ZQ::common::Log::L_INFO, CLOGFMT(C2ClientAsync, "can't find 'PortNum' from locate response, try to use default port[%d]"), _getRequestPort);
                port = _getRequestPort;
            }else{
                port = atoi(respData.portNum.c_str());
            }

            printf("locate request successfully\n");

            AttrMap prop;
            _log(ZQ::common::Log::L_INFO, CLOGFMT(C2ClientAsync, "begin send get request\n"));
            if (!sendGetRequest(_sessionID, _contentName, prop, addr, port))
            {
                // TODO: return error code
                _status = ZQ::StreamService::Failure;
                printf("send get request failure\n");
            }
            printf("receiving data ... \n");

        }
        else //get request successfully
        {
            _log(ZQ::common::Log::L_INFO, CLOGFMT(C2ClientAsync, "get request latency[%ld]"), ZQ::common::TimeUtil::now()-_startCalcLatency);

            if (_bIndex)
            {
                ZQ::IdxParser::IndexData idxData;
                bool result = parseIndex(_contentName, _respBody.c_str(), _respBody.size(), idxData);
                if (result)
                {
                    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "parse index success"));

                    //set response map
                    _recvMap[C2CLIENT_AssetID]		= _paid;
                    _recvMap[C2CLIENT_ProviderID]	= _pid;

                    std::ostringstream playTimeOSS;
                    playTimeOSS<<idxData.getPlayTime();
                    if (!playTimeOSS.str().empty())
                    {
                        _recvMap[C2CLIENT_PlayTime]			=	playTimeOSS.str();
                    }

                    std::ostringstream muxBitRateOSS;
                    muxBitRateOSS<<idxData.getMuxBitrate();
                    if (!muxBitRateOSS.str().empty())
                    {
                        _recvMap[C2CLIENT_MuxBitrate]		=	muxBitRateOSS.str();
                    }

                    std::string extName = idxData.getSubFileName(0);
                    if (!extName.empty())
                    {
                        size_t dotPos = extName.find_first_of('.');
                        if (dotPos == extName.npos)
                        {
                            _recvMap[C2CLIENT_ExtName]			=	extName;
                        }
                        else{
                            _recvMap[C2CLIENT_ExtName]			=	extName.substr(dotPos + 1);
                        }
                    }

                    ZQ::IdxParser::IndexData::SubFileInformation info;
                    if (idxData.getSubFileInfo(0, info))
                    {
                        std::ostringstream startOffsetOSS;
                        uint64 startOffset =info.startingByte;
                        startOffsetOSS<<startOffset;
                        if (!startOffsetOSS.str().empty())
                        {
                            _recvMap[C2CLIENT_StartOffset]		=	startOffsetOSS.str();
                        }

                        std::ostringstream endOffsetOSS;
                        uint64 endOffset   = info.endingByte;
                        endOffsetOSS<<endOffset;
                        if (!endOffsetOSS.str().empty())
                        {
                            _recvMap[C2CLIENT_EndOffset]		=	endOffsetOSS.str();
                        }
                    }

                }
                else
                {
                    _status = ZQ::StreamService::Failure;
                    _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parse index failure"));
                    return;
                }
            }else{
                // get main file
            }

            printf("get request successfully\n");
        }
    }
}

bool C2ClientAsync::onHttpMessage( const LibAsync::HttpMessagePtr msg)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "onHttpMessage(), received http header, client[%p]"), this);
	_headerMsg = msg;

	return true;
}

bool C2ClientAsync::onHttpBody( const char* data, size_t size)
{
	//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "entry onHttpBody(), received data size[%d]"), size);

    _status = ZQ::StreamService::Receiving;
    if (_bLocateRequest)
    {
        _respBody.append(data, size);
    }else{
        if (_bIndex)
        {
            _respBody.append(data, size);
        }
        _file->write(data, size);
        _file->flush();
    }

	return true;
}
void C2ClientAsync::onHttpComplete()
{
    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "onHttpComplete() entry"));
	_status = ZQ::StreamService::Completed;
}

void C2ClientAsync::onHttpError( int error )
{
	_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "onHttpError() error[%d], client[%p]"), error, this);
	cancel();

    _status = ZQ::StreamService::Failure;

    printf("http error occured, error code %d\n", error);
}

void C2ClientAsync::onTimer()
{
    cancel();
    _status = ZQ::StreamService::Timeout;
    std::string err = _bLocateRequest ? "locate" : "get";
	_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "timeout when %s, client[%p]"), err.c_str(), this);
    printf("timeout when %s request\n", err.c_str());

}

bool C2ClientAsync::parseResponse(const SimpleXMLParser::Node* root, LocateResponseData& respData, std::string& error)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "parseResponse() entry"));

	typedef SimpleXMLParser::Node Node;
	// step 1: check the content and extract the response data
	const Node* locNode = findNode(root, "LocateResponse");
	if(NULL == locNode)
	{ // bad xml content
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <LocateResponse>"));
		return false;
	}

	const Node* transferPortNode = findNode(locNode, "TransferPort");
	if(!transferPortNode)
	{ // parameter missed
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <TransferPort>"));
		return false;
	}
	respData.transferPort = transferPortNode->content;

	const Node* transferIDNode = findNode(locNode, "TransferID");
	if (!transferIDNode)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <TransferID>"));
		return false;
	}
	respData.transferId = transferIDNode->content;

	const Node* transferTimeoutNode = findNode(locNode, "TransferTimeout");
	if (!transferTimeoutNode)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <TransferTimeout>"));
		return false;
	}
	respData.transferTimeout = StringToLong(transferTimeoutNode->content);

	const Node* availableRangeNode = findNode(locNode, "AvailableRange");
	if (!availableRangeNode)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <AvailableRange>"));
		return false;
	}
	respData.availableRange = availableRangeNode->content;

	const Node* openForWriteNode = findNode(locNode, "OpenForWrite");
	if (!openForWriteNode)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <OpenForWrite>"));
		return false;
	}
	respData.openForWrite = openForWriteNode->content;

	const Node* portNumNode = findNode(locNode, "PortNum");
	if (portNumNode)
	{
        respData.portNum = portNumNode->content;
	}

	if (_bIndex) //if call read(), with no need for these properties
	{
        const Node* residentialNode = findNode(locNode, "ClipInfo/Residential");
        if (!residentialNode)
        {
            _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <Residential>"));
            return false;
        }
        std::map<std::string, std::string> residentialNodeAttrs = residentialNode->attrs;
        respData.recording = residentialNodeAttrs["recording"];

        const Node* encodingInfoNode = findNode(locNode, "ClipInfo/EncodingInfo");
        if (!encodingInfoNode)
        {
            _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <EncodingInfo>"));
            return false;
        }
        std::map<std::string, std::string> encodingInfoNodeAttrs = encodingInfoNode->attrs;
        respData.playTime = encodingInfoNodeAttrs["playTime"];
        respData.muxBitrate = encodingInfoNodeAttrs["muxBitrate"];

        const Node* membersNode = findNode(locNode, "ClipInfo/Members");
        if (!membersNode)
        {
            _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() XML element missed: <MembersNode>"));
            return false;
        }
        std::list<Node>::const_iterator it = membersNode->children.begin();
        if (it == membersNode->children.end())
        {
            _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2ClientAsync, "parseResponse() ClipInfo/Members is empty "));
            return false;
        }

        std::map<std::string, std::string> subFileAttrs = it->attrs;
        std::string ext = subFileAttrs["extName"];

        respData.extName = ext;
        respData.startOffset = subFileAttrs["startOffset"];
        respData.endOffset = subFileAttrs["endOffset"];
	}

	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "parseResponse() parse response success"));
	return true;
}

bool C2ClientAsync::setRevcMap(LocateResponseData& respData)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2ClientAsync, "setRevcMap() entry"));

	if (!&respData)
	{
		return false;
	}

	_recvMap[C2CLIENT_TransferPort]		=	respData.transferPort;
	_recvMap[C2CLIENT_TransferID]		=	respData.transferId;
	_recvMap[C2CLIENT_PortNum]			=	respData.portNum.empty() ? "12000" : respData.portNum;

	char timeout[8];
	itoa(respData.transferTimeout, timeout, 10);
	std::string strTimeout = timeout;
	_recvMap[C2CLIENT_TransferTimeout]	=	timeout;

	_recvMap[C2CLIENT_AvailableRange]	=	respData.availableRange;
	_recvMap[C2CLIENT_OpenForWrite]		=	respData.openForWrite;

	_recvMap[C2CLIENT_Recording]		=	respData.recording;
	_recvMap[C2CLIENT_PlayTime]			=	respData.playTime;
	_recvMap[C2CLIENT_MuxBitrate]		=	respData.muxBitrate;
	_recvMap[C2CLIENT_ExtName]			=	respData.extName;
	_recvMap[C2CLIENT_StartOffset]		=	respData.startOffset;
	_recvMap[C2CLIENT_EndOffset]		=	respData.endOffset;

	return true;
}
} // namespace StreamService
} // namespace ZQ