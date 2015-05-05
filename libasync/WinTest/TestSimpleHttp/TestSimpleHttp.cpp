#include "TestSimpleHttp.h"
#include <InetAddr.h>

namespace LibAsync{

	TestSimpleHttp::TestSimpleHttp(ZQ::common::Log& log, const std::string& addr, const unsigned int port, const int timeout)
		:_log(log), _addr(addr), _port(port), _timeout(timeout)
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSimpleHttp, "constructor entry"));
	}

	TestSimpleHttp::~TestSimpleHttp()
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSimpleHttp, "destructor entry"));
	}

	bool TestSimpleHttp::sendLocateRequest(const std::string& url, const std::string& contentName, const std::string& subType, AttrMap& reqProp, AttrMap& respProp)
	{
		_url = url;
		_subType = subType;

		//get paid and pid from content name
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

		HttpMessagePtr locateMsgPtr = new HttpMessage(http_parser_type::HTTP_REQUEST);
		locateMsgPtr->method(http_method::HTTP_POST);
		locateMsgPtr->url(url);

		//add header
		locateMsgPtr->header(C2CLIENT_HOST, host);
		locateMsgPtr->header(C2CLIENT_UserAgent, userAgent);
		locateMsgPtr->header(C2CLIENT_ContentType, contentType);

		//generate body
		std::string bodyMsg = generateLocateBody(reqProp);

		if (!doHttp(locateMsgPtr, bodyMsg, _addr, _port))
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(TestSimpleHttp, "failed to send locate request to [%s:%d]"),_addr.c_str(), _port);
			return false;
		}

		return true;
	}

	std::string TestSimpleHttp::generateLocateBody(AttrMap& reqProp)
	{
		std::ostringstream sendBodyMsg;

		std::string  transferRate = reqProp[C2CLIENT_TransferRate].empty()			\
			? "20000000" : reqProp[C2CLIENT_TransferRate];

		std::string  ingressCapacity = reqProp[C2CLIENT_IngressCapacity].empty()	\
			? "16512000000" : reqProp[C2CLIENT_IngressCapacity];

		std::string strClientTransfer = ZQ::common::InetHostAddress::getLocalAddress().getHostAddress();
		std::string clientTransfer = reqProp[C2CLIENT_ClientTransfer].empty()		\
			? strClientTransfer : reqProp[C2CLIENT_ClientTransfer];

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

		if (transferRate.empty()) return 0;
		sendBodyMsg << "<TransferRate>" << transferRate << "</TransferRate>" << "\r\n";

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

	bool sendGetRequest(const std::string& url, const std::string& contentName, AttrMap& reqProp, AttrMap& respProp)
	{
		return true;
	}

	void TestSimpleHttp::OnSimpleHttpResponse(HttpMessagePtr msg, const std::string& body)
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSimpleHttp, "OnSimpleHttpResponse entry"));
		printf("body:\n%s\n", body.c_str());
	}

	void TestSimpleHttp::onSimpleHttpError( int error )
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSimpleHttp, "onSimpleHttpError entry"));
	}
}