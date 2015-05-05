#ifndef _GETMAINFILE_H
#define _GETMAINFILE_H

#include <ZQ_common_conf.h>
#include <libasync/http.h>
#include "SimpleXMLParser.h"
#include <HttpClient.h>
#include <IndexFileParser.h>
#include <fstream>

#ifdef ZQ_OS_MSWIN
#pragma comment(lib, "ws2_32.lib")
#endif//ZQ_OS_MSWIN

//error code for response
#define C2_ERROR_BAD_RESPONSE_CODE					-1
#define C2_ERROR_BAD_RESPONSE_STRING				"Bad Response"
#define C2_ERROR_LOCATE_FAILED_CODE					-2
#define C2_ERROR_LOCATE_FAILED_STRING			    "Locate Request Failed"
#define C2_ERROR_GET_FAILED_CODE					-3
#define C2_ERROR_GET_FAILED_STRING					"Get Request Failed"
#define C2_ERROR_TIMEOUT_CODE						-4
#define C2_ERROR_TIMEOUT_STRING						"Timeout"

#define C2CLIENT_URI								"URI"
#define C2CLIENT_HOST								"HOST"
#define C2CLIENT_UserAgent							"User-Agent"
#define C2CLIENT_ContentType						"Content-Type"
#define C2CLIENT_ContentLength						"Content-Length"
#define C2CLIENT_TransferPort						"TransferPort"
#define C2CLIENT_TransferID							"TransferID"
#define C2CLIENT_PortNum							"PortNum"
#define C2CLIENT_TransferTimeout					"TransferTimeout"
#define C2CLIENT_AvailableRange						"AvailableRange"
#define C2CLIENT_OpenForWrite						"OpenForWrite"
#define C2CLIENT_AssetID							"AssetID"
#define C2CLIENT_ProviderID							"ProviderID"
#define C2CLIENT_SubType							"SubType"
#define C2CLIENT_ObjectID							"ObjectID"
#define C2CLIENT_TransferRate						"TransferRate"
#define C2CLIENT_IngressCapacity					"IngressCapacity"
#define C2CLIENT_ClientTransfer						"ClientTransfer"
#define C2CLIENT_ExclusionList						"ExclusionList"
#define C2CLIENT_Range								"Range"
#define C2CLIENT_TransferDelay						"TransferDelay"

#define C2CLIENT_Recording							"recording"
#define C2CLIENT_PlayTime							"playTime"
#define C2CLIENT_MuxBitrate							"muxBitrate"
#define C2CLIENT_ExtName							"extName"
#define C2CLIENT_StartOffset						"startOffset"
#define C2CLIENT_EndOffset							"endOffset"

#define C2CLIENT_GET_REQUEST_Transfer_Delay			"Transfer-Delay"
#define C2CLIENT_GET_REQUEST_Ingress_Capacity		"Ingress-Capacity"

namespace ZQ{
    namespace StreamService{
        typedef std::map<std::string, std::string> AttrMap;

        typedef enum _TransferStatus{
            Initialize,
            NotConnected,
            Connected,
            HeaderSendCompleted,
            BodySendCompleted,
            Receiving,
            WaitNewBuffer,
            Completed,
            Failure,
            Timeout
        }TransferStatus;

        struct LocateRequestData
        {
            std::string objectId; // for the object identifier mode in comcast spec
            std::string assetId;
            std::string providerId;
            std::string subType;
            std::string extension; // file extension of the member file
            long		transferRate;

            std::string clientTransfer;
            long		ingressCapacity;

            std::vector<std::string> exclusionList;
            std::string range;
            long		transferDelay;
            LocateRequestData() {
                transferRate = 0;
                ingressCapacity = 0;
                transferDelay = 0;
            }
        };

        struct LocateResponseData
        {
            std::string transferPort;
            std::string transferId;
            std::string openForWrite;
            std::string availableRange;
            std::string portNum; // listen port
            std::string idxContentGeneric;
            std::string idxContentSubfiles;
            std::string recording;
            std::string startOffset;
            std::string endOffset;
            std::string extName;
            std::string playTime;
            std::string muxBitrate;
            long		transferTimeout;

            std::string reqSubType;//not for response output
            int32		exposeAssetIndexData;
            LocateResponseData():transferTimeout(-1),exposeAssetIndexData(0) {}
        };

        class C2ClientAsync;
        typedef ZQ::common::Pointer<C2ClientAsync> C2ClientAsyncPtr;
        class C2ClientAsync : public LibAsync::HttpClient, public LibAsync::Timer
        {
        public:
            C2ClientAsync(ZQ::common::Log& log, const std::string upStreamIP, const std::string clientTransfer, const std::string& addr, const unsigned int port, const int timeout = 10*1000);

            friend class AsyncReadInitialize;

            virtual ~C2ClientAsync();

            bool	    sendLocateRequest(const std::string& url, const std::string& contentName, const std::string& subType, AttrMap& prop);
            bool	    sendGetRequest(const std::string& url, const std::string& contentName, AttrMap& prop, std::string addr, unsigned int port);

        private:
            std::string generateLocateBody(AttrMap& reqProp);

            int64		setTimeout(int64 timeout = 10*1000); //default timeout 10s
            bool		parseResponse(const SimpleXMLParser::Node* root, LocateResponseData& respData, std::string& error);
            bool		parseIndex(std::string& contentName, const char* indexData, size_t dataSize, ZQ::IdxParser::IndexData& idxData);
            bool		setRevcMap(LocateResponseData& respData);

            bool        checkTimeout();

        private:

            virtual void	onReqMsgSent( size_t size);
            virtual void	onHttpDataReceived( size_t size );
            virtual bool	onHttpMessage( const LibAsync::HttpMessagePtr msg);
            virtual bool	onHttpBody( const char* data, size_t size);
            virtual void	onHttpComplete();
            virtual void	onHttpError( int error );

            virtual void	onTimer();

        private:
            ZQ::common::Log&			_log;
            std::string					_url;
            std::string                 _upStreamIP;
            std::string                 _clientTransfer;
            std::string					_addr;
            unsigned int				_port;
            unsigned int                _getRequestPort;
            std::string					_paid;
            std::string					_pid;
            std::string					_contentName;
            std::string					_subType;
            std::string					_transferRate;
            int							_timeout; //ms
            int64                       _startTime;
            int64                       _startCalcLatency;

            LibAsync::AsyncBufferS		_recvBufs;
            LibAsync::HttpMessagePtr	_headerMsg;
            std::string					_reqBody;
            std::string					_respBody;
            TransferStatus				_status;
            bool						_bBodySend;

            //for C2SS
            AttrMap						_sendMap;
            AttrMap						_recvMap;
            bool                        _bIndex;

            std::string                 _sessionID;
            bool                        _bLocateRequest;

            std::ofstream*                   _file;

        };
    } // namespace StreamService
} // namespace ZQ
#endif //_GETMAINFILE_H
