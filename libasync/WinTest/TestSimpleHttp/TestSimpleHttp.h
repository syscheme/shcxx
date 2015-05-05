#ifndef _TEST_SIMPLE_HTTP_H
#define _TEST_SIMPLE_HTTP_H
#include <http.h>
#include <FileLog.h>

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

namespace LibAsync{

	typedef std::map<std::string, std::string> AttrMap;

	class TestSimpleHttp : public SimpleHttpClient
	{
	public:
		TestSimpleHttp(ZQ::common::Log& log, const std::string& addr, const unsigned int port = 10080, const int timeout = 10*1000); //timeout default 10s
		virtual ~TestSimpleHttp();

		bool sendLocateRequest(const std::string& url, const std::string& contentName, const std::string& subType, AttrMap& reqProp, AttrMap& respProp);
		bool sendGetRequest(const std::string& url, const std::string& contentName, AttrMap& reqProp, AttrMap& respProp);

	private:
		std::string generateLocateBody(AttrMap& reqProp);

		virtual void OnSimpleHttpResponse( HttpMessagePtr msg, const std::string& body);
		virtual void onSimpleHttpError( int error );

	private:
		ZQ::common::Log			_log;
		std::string				_addr;
		unsigned int			_port;
		std::string				_url;
		std::string				_paid;
		std::string				_pid;
		std::string				_subType;

		int						_timeout; //ms

	};
	typedef ZQ::common::Pointer<TestSimpleHttp> TestSimpleHttpPtr;
}
#endif