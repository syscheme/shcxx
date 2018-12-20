#include "SystemInfoRequestHandler.h"
#include <stdlib.h>
#include <sstream>
#include <string>

SystemInfoRequestHandler::SystemInfoRequestHandler(ZQ::common::FileLog& log)
		:_log(log),
		_info(&_log)
{

}

SystemInfoRequestHandler::~SystemInfoRequestHandler()
{

}

void SystemInfoRequestHandler::onRequest(const CRG::IRequest* req,CRG::IResponse* resp)
{
	resp->setHeader("Access-Control-Allow-Origin","*");
	resp->setHeader("Access-Control-Allow-Methods","POST,GET");

	ZQ::common::SystemInfo _info(&_log);
	std::stringstream ss;
	ss.str("");
	std::string url = req->uri();
	std::string body;
	if (url.find("cpu") != url.npos)
	{
		//body = "{ cpu: { model:intel pentiumn, freq: 2000, usage:50, cores:[60,50,50,40]}}";
		body = "{ \"cpu\": { \"model\":\"intel pentiumn\", \"freq\": 2000, \"usage\":50, \"cores\":[60,50,50,40]}}";
	}
	else if (url.find("ram") != url.npos)
	{
		_info.refreshSystemUsage();
		/*
		ss << "{ \"ram\": {\"total\":";
		ss << _info._memTotalPhys;
		ss << ", \"free\": ";
		ss << _info._memAvailPhys;
		ss << "}}";
		*/
		ss << "{ \"ram\": {\"total\":[";
		ss << _info._memTotalPhys;
		ss << "], \"free\": [";
		ss << _info._memAvailPhys;
		ss << "]}}";
		body = ss.str();
		printf("body = %s\n",body.c_str());
	}
	else if (url.find("nic") != url.npos)
	{
	}
	else
	{
		resp->setStatus(404,"Content Not Found");
		_log(ZQ::common::Log::L_INFO, CLOGFMT(SystemInfoRequestHandler, "errorResponse() StatusCode(404)ReasonPhrase(Content Not Found)"));

	}
	
	printf("receive request uri=%s\n",req->uri());
//	/mvar/cpu => { "cpu": { "model":"intel pentiumn", "freq": 2000, "usage":50, "cores":[60,50,50,40]}}
//	/mvar/ram => { "ram": {"total":20000, "free": 15000,}},
//	/mvar/nic => {"nic": [{"name":"eth0", "ip": "192.168.81.71", "mask":"255....", "bwtotal": 10000, "txbytes": 11111, "rxbytes":22222, "txpackets":222, }, ]}

	resp->setContent(body.c_str(),body.size());


}