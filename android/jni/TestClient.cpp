#include <iostream>
#include <signal.h>
#include "RTSPClient.h"
#include "FileLog.h"
#include <string>
#include <map>
#include <algorithm>
using namespace ZQ::common;
using namespace std;

class MySession : public RTSPSession
{
	public:
		MySession(Log& log, NativeThreadPool& thrdpool, const char* streamDestUrl, const char* filePath=NULL, Log::loglevel_t verbosityLevel=Log::L_WARNING, int timeout=600000, const char* sessGuid=NULL)
			: RTSPSession(log,thrdpool,streamDestUrl,filePath,verbosityLevel,timeout)
		{}

		virtual ~MySession()
		{
			destroy();
		}

		void destroy()
		{
			RTSPSession::destroy();
			_log(Log::L_INFO, CLOGFMT(NGODSession, "Session[%s, %s] destroyed"), _sessGuid.c_str(), _sessionId.c_str());
		}


	protected:
		// overwrites of RTSPSession
		virtual void OnResponse_SETUP(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, ZQ::common::RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
		{
			RTSPSession::OnResponse_SETUP(rtspClient, pReq, pResp, resultCode, resultString);
			_log(Log::L_DEBUG, CLOGFMT(MySession, "Session[%s, %s, %d] OnResponse_SETUP(), return [%d %s]"), _sessGuid.c_str(), _sessionId.c_str(), pResp->cSeq, resultCode, resultString);
			_resultCode = resultCode;
			_stampSetup = now();
			if (resultCode != RTSPSink::rcOK)
				return;

			// TODO: add your code to process SETUP response
		}	

		virtual void OnResponse_PLAY(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
		{
			_log(Log::L_DEBUG, CLOGFMT(MySession, "Session[%s, %d] OnResponse_PLAY(), return [%d %s]"), _sessGuid.c_str(), pResp->cSeq, resultCode, resultString);
			_resultCode = resultCode;
			if(resultCode == RTSPSink::rcSessNotFound)
			{
				destroy();
				return;
			}

			if(resultCode != RTSPSink::rcOK)
				return;

			// TODO: add your code to process PLAY response
		}

		virtual void OnResponse_TEARDOWN(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RTSPMessage::Ptr& pResp, uint resultCode, const char* resultString)
		{
			_log(Log::L_DEBUG, CLOGFMT(NGODSession, "Session[%s, %d] OnResponse_TEARDOWN(), return [%d %s]"), _sessGuid.c_str(), pResp->cSeq, resultCode, resultString);
			_resultCode = resultCode;
			_sessionHistory = pResp->contentBody;
			if (resultCode == RTSPSink::rcSessNotFound)
				destroy();
		}

		virtual void OnRequestError(RTSPClient& rtspClient, RTSPRequest::Ptr& pReq, RequestError errCode, const char* errDesc=NULL)
		{
			RTSPSession::OnRequestError(rtspClient, pReq, errCode, errDesc);

			if (!pReq)
				return;

			switch(errCode)
			{
				case Err_InvalidParams:
					_resultCode = rcBadParameter;
					break;

				case Err_RequestTimeout:
					_resultCode = rcRequestTimeout;
					break;

				case Err_ConnectionLost:
					_resultCode = Err_ConnectionLost;
					break;
				case Err_SendFailed:
					_resultCode = Err_SendFailed;
					break;
				default:
					_resultCode = rcServiceUnavail;
			}
		}

	public:
		typedef Pointer < MySession > Ptr;	
		uint			 _resultCode;
		std::string     _sessionHistory;
};

struct Options
{
	std::string RtspUrl;
	std::string bitrate;
	std::string ServiceGroup;
	std::string IPport;
	std::string appdata;
} options =
{
	"rtsp://10.15.10.73:554",
	"375000",
	"1004",
	"",
	"smartcard-id=1370495919;device-id=1370495919;home-id=35355793;purchase-id=7204763;"
};
void sig_handler(int signo)
{
	if (signo == SIGPIPE) 
	{
		//printf("received SIGPIPE\n");
	}
	/*
	   else if(signo == SIGINT)
	   {
	//printf("received SIGINT\n");
	}
	else if(signo == SIGKILL)
	{
	//printf("received SIGKILL\n");
	}*/
}

void ignoreSigPipe() 
{
	if(signal(SIGPIPE, sig_handler) == SIG_ERR) 
	{
		printf("failed to register sig_handler\n");
	} 
	else 
	{
		//	printf("register sig_handler ok\n");
	}
}

void usage()
{	
	//urlStr = std::string("rtsp://") + axiomUrl.getHost() + "/" + pPath + "?asset=" + pPID+"#"+ pPAID;
	std::cout 
		<< "RtspClient -u \"<RTSP URL>\" [--b <bitrate>] {-g <ServiceGroup> | -d <IP>:<port>} [-a <appdata>]\n"
		<< "Options:\n"
		<< "    -u <RTSP URL> the full URL to the RTSP server  <= example: rtsp://10.15.10.73:554/60010001?assetUID=3B9AD0DA\n"
		<< "    -b <bitrate>                                   <= example: bandwidth=375000\n"
		<< "    -g <ServiceGroup>                              <= example: 1001\n"
		<< "    -d <IP>:<port>                                 <= IP mode:\n"
		<< "    -a <appdata>                                   <= appdata: smartcard-id=1370495919;device-id=1370495919;home-id=35355793;purchase-id=7204763; \n"
		<< std::endl;
}

void getopts(int argc, char** argv)
{
	int count = 1;
	while (count < argc)
	{
		if (strcmp(argv[count], "-u") == 0)
		{
			if (++count < argc)
			{

				options.RtspUrl = argv[count];
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "-b") == 0)
		{
			if (++count < argc)
			{
				options.bitrate = argv[count];
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "-g") == 0)
		{
			if (++count < argc)
			{
				options.ServiceGroup = argv[count];
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "-d") == 0)
		{
			if (++count < argc)
			{
				options.IPport = argv[count];
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "-a") == 0)
		{
			if (++count < argc)
			{
				options.appdata = argv[count];
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--help") == 0 || 
				(strcmp(argv[count], "-h") == 0))
		{
			usage();
		}
		else
		{
			printf("Arguments is Invalid\n");
		}
		count++;
	}
}
// brief usage:
//    RTSPClient client(logger, threadpool, bindAddress, "rtsp://192.168.11.22");
//    RTSPSession::Ptr sess = new RTSPSession(logger, threadpool, "udp://@223.12.12.12:1234", "/aaa.mpg");
//    client.sendSETUP(*sess);
//    ... wait for the overwritten of RTSPSession::OnResponse_SETUP()
//    client.sendPLAY(*sess);
//    ... wait
//    client.sendTEARDOWN(*sess);
//    sess->destroy(); //must call to destroy the session


// RtspClient -u "<RTSP URL>" [-b <bitrate>] {-g <ServiceGroup> | -d <IP>:<port>} [-a <appdata>]
// -u <RTSP URL> the full URL to the RTSP server  <= 例如上例子rtsp://192.168.11.22/aaa.mpg
// -b <bitrate> 
// -g <ServiceGroup> 走QAM/DVBC模式
// -d <IP>:<port> 走IP模式
// -a <appdata>  应用相关的参数



int main(int argc,char** argv)
{
	getopts(argc, argv);
	if(argc < 3)
	{
		printf("Please -h/--help for help\n");
		return 0;
	}
	ZQ::common::FileLog  logger("./log/TestClient.log",ZQ::common::Log::L_DEBUG);
	ZQ::common::NativeThreadPool thrdpool(10);
	ZQ::common::InetHostAddress bindAddress("0.0.0.0");
	std::string Url,Asset;
	size_t pos = options.RtspUrl.rfind("/");
	if(pos != std::string::npos)
	{
		Asset = options.RtspUrl.substr(pos+1);
		Url = options.RtspUrl.substr(0,pos);
	}
	RTSPClient_sync client(logger, thrdpool, bindAddress, Url.c_str(), "ITVLibrary 1.0; amino",ZQ::common::Log::L_DEBUG);
	client.setClientTimeout(30000,60000);
	MySession::Ptr sess = new MySession(logger, thrdpool, "",Asset.c_str(),ZQ::common::Log::L_DEBUG);


	RTSPMessage::AttrMap headers;
	//case 1. QAM mode
	//SETUP rtsp://172.16.88.5:554/60010001?asset=10048#GTIT0120151203200219&folderId=GCNN7137 RTSP/1.0
	//Transport: MP2T/DVBC/QAM;unicast
	//TianShan-ServiceGroup: 102
	//TianShan-AppData: smartcard-id=1370495919;device-id=1370495919;home-id=35355793;purchase-id=7204763;
	std::string destination, port;
	pos = options.IPport.find(":");
	if(pos != std::string::npos)
	{
		port = options.IPport.substr(pos+1);
		destination = options.IPport.substr(0,pos);
	}
	char transport[2048];
	memset(transport,sizeof(transport),0);
	if(options.IPport.empty())
	{
		snprintf(transport,sizeof(transport)-1,"MP2T/DVBC/QAM;unicast;bandwidth=%s" ,options.bitrate.c_str());
		logger(ZQ::common::Log::L_DEBUG, CLOGFMT(testClient, "Now is MP2T/DVBC/QAM mode"));
	}
	else
	{
		snprintf(transport,sizeof(transport)-1,"MP2T/AVP/UDP;unicast;bandwidth=%s;destination=%s;client_port=%s" ,options.bitrate.c_str(),destination.c_str(),port.c_str());
		logger(ZQ::common::Log::L_DEBUG, CLOGFMT(testClient, "Now is MP2T/AVP/UDP mode"));
	}

	headers["Transport"] = transport;
	headers["TianShan-ServiceGroup"] = options.ServiceGroup;
	if (!options.appdata.empty())
	{
		headers["TianShan-AppData"] = options.appdata;
	}

	int64 stampNow = ZQ::common::now();
	int cseq = client.sendSETUP(*sess, "", NULL, headers);
	ignoreSigPipe();	
	if (cseq <= 0 || !client.waitForResponse(cseq))
	{
		logger(ZQ::common::Log::L_ERROR, CLOGFMT(testClient, "SETUP request failed/timeout [%lld]msec"), ZQ::common::now() -stampNow);
		return -1;
	}

	logger(ZQ::common::Log::L_DEBUG, CLOGFMT(testClient, "SETUP resultCode: %d"),sess->_resultCode);
	printf("SETUP    : %d\n",sess->_resultCode);
	if(RTSPSink::rcOK != sess->_resultCode) //if SETUP fair, not need PLAY TEARDOWN
	{
		sess->destroy(); //must call to destroy the session
		return 0;
	}


	//PLAY rtsp://172.16.88.5:554/60010001?asset=10048#GTIT0120151203200219&folderId=GCNN7137 RTSP/1.0
	//CSeq: ?
	//Session: ????
	//User-Agent: ITVLibrary 1.0; amino
	//Range: npt=0-
	//Scale: 1.0
	headers.clear();
	headers["Range"] = "npt=0-";
	headers["Scale"] = "1.0";
	stampNow = ZQ::common::now();
	cseq = client.sendPLAY(*sess, 0.0, 20.0, 1.0);
	if (cseq <= 0 || !client.waitForResponse(cseq))
	{
		logger(ZQ::common::Log::L_ERROR, CLOGFMT(testClient, "PLAY request failed/timeout [%lld]msec"), ZQ::common::now() -stampNow);
		return -2;
	}

	logger(ZQ::common::Log::L_DEBUG, CLOGFMT(testClient, "PLAY resultCode: %d"),sess->_resultCode);
	printf("PLAY     : %d\n",sess->_resultCode);

	//PAUSE * RTSP/1.0
	//CSeq: 14
	//User-Agent: ITVLibrary 1.0; amino
	//Session:  ????
	headers.clear();
	headers["Range"] = "npt=now-";
	headers["x-reason"] = "User Requested Pause";
	stampNow = ZQ::common::now();
	cseq = client.sendPAUSE(*sess, NULL, headers);
	if (cseq <= 0 || !client.waitForResponse(cseq))
	{
		logger(ZQ::common::Log::L_ERROR, CLOGFMT(testClient, "PAUSE request failed/timeout [%lld]msec"), ZQ::common::now() -stampNow);
		return -3;
	}
	logger(ZQ::common::Log::L_DEBUG, CLOGFMT(testClient, "PAUSE resultCode: %d"),sess->_resultCode);
	printf("PAUSE    : %d\n",sess->_resultCode);

	//PLAY * RTSP/1.0
	//CSeq: ?
	//Session:  ????
	//User-Agent: ITVLibrary 1.0; amino
	//Scale: 7.500000
	headers.clear();
	headers["Scale"] = "7.5";
	stampNow = ZQ::common::now();
	cseq = client.sendPLAY(*sess, -1.0f, -1.0f, 7.5, NULL, headers);
	if (cseq <= 0 || !client.waitForResponse(cseq))
	{
		logger(ZQ::common::Log::L_ERROR, CLOGFMT(testClient, "FASTWARD request failed/timeout [%lld]msec"), ZQ::common::now() -stampNow);
		return -4;
	}

	logger(ZQ::common::Log::L_DEBUG, CLOGFMT(testClient, "FASTWARD resultCode: %d"),sess->_resultCode);
	printf("FASTWARD : %d\n",sess->_resultCode);

	//TEARDOWN * RTSP/1.0
	//CSeq: ?
	//User-Agent: ITVLibrary 1.0; amino
	//x-reason: User Requested Teardown
	//RTSPMessage::AttrMap headers;
	headers.clear();
	headers["x-reason"] = "User Requested Teardown";
	cseq = client.sendTEARDOWN(*sess);
	if (cseq <= 0 || !client.waitForResponse(cseq))
	{
		logger(ZQ::common::Log::L_ERROR, CLOGFMT(testClient, "TEARDOWN request failed/timeout [%lld]msec"), ZQ::common::now() -stampNow);
		return -5;
	}

	logger(ZQ::common::Log::L_DEBUG, CLOGFMT(testClient, "TEARDOWN resultCode: %d"),sess->_resultCode);
	printf("TEARDOWN : %d\n",sess->_resultCode);

	sess->destroy(); //must call to destroy the session

	return 0;
}
