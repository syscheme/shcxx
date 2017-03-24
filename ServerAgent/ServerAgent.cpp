#include "ServerAgent.h"
#include "snmp/ZQSnmp.h"
#include <fstream>
#define MAXLENGTH 1024
#define COUNTS    10
namespace ZQ {
namespace eloop {

// -----------------------------
// class SvarQueryCB
// -----------------------------


class ZQSnmpSingleton{
public:
	static ZQ::SNMP::SnmpAgent* getInstance(ZQ::common::Log& log){
		if(_agent == NULL){
			_agent =  new ZQ::SNMP::SnmpAgent(log);
			_agent->start();
			return _agent;
		}else{
			return _agent;
		}
	}

private:
	ZQSnmpSingleton(){}
	~ZQSnmpSingleton(){if(_agent)delete _agent;}
	//把复制构造函数和=操作符也设为私有,防止被复制
	ZQSnmpSingleton(const ZQSnmpSingleton&){}
	ZQSnmpSingleton& operator=(const ZQSnmpSingleton&){}

	static ZQ::SNMP::SnmpAgent* _agent;
	static ZQ::common::Log& _log;
};
ZQ::SNMP::SnmpAgent*  ZQSnmpSingleton::_agent = NULL;


class SvarQueryCB : public ZQ::SNMP::SnmpAgent::QueryCB
{
public:
	typedef ZQ::common::Pointer< SvarQueryCB > Ptr;
	SvarQueryCB(ZQ::SNMP::SnmpAgent& agent, HttpConnection& conn,ZQ::common::Log& log) : _agent(agent), _conn(conn),_log(log) {}

	virtual void OnResult()
	{
		HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::MSG_RESPONSE);
		std::string body;
		if (this->vlist.size() >0 && AsnType_String == this->vlist[0]->type())
		{
			MemRange mr =  this->vlist[0]->getValueByMemRange();
			body.assign((const char*)mr.first, mr.second);
		}
		printf("getJSON %s\n",body.c_str());
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerAgent, "OnResult getJson[%s]"),body.c_str());
		int code = 200;
		outmsg->code(code);
		outmsg->status(HttpMessage::code2status(code));
		outmsg->keepAlive(true);
		outmsg->header("Access-Control-Allow-Origin","*");
		outmsg->header("Access-Control-Allow-Methods","POST,GET");
		outmsg->header("Server","LibAsYnc HtTp SeRVer");
		outmsg->header("Date",HttpMessage::httpdate());
		outmsg->contentLength(body.length());

		std::string head = outmsg->toRaw();
		_conn.write(head.c_str(),head.length());
		_conn.write(body.c_str(),body.length());
		//_conn.setkeepAlive(false);
		return;
	}

	virtual void OnError(int errCode) {
		printf("%d\n",errCode);
	}

	ZQ::SNMP::SnmpAgent&        _agent;
	HttpConnection&             _conn;
	ZQ::common::Log&            _log;
};

// --------------------------------------------------
// class ServerAgent
// --------------------------------------------------
ServerAgent::ServerAgent(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps)
:HttpHandler(conn,logger,dirProps, appProps)
{
	_ag = ZQSnmpSingleton::getInstance(logger); 
}

ServerAgent::~ServerAgent()
{	

}

bool ServerAgent::onHeadersEnd( const HttpMessage::Ptr msg)
{
	HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::MSG_RESPONSE);
	printf("url = %s\n",msg->url().c_str());	
	std::string url = msg->url();
	std::string body = "404 Not Found";
		
	char svar[10] = {0};
	char svcType[100] = {0};
	int  instId = 0;
	char varname[100] = {0};
	char params[100] = {0};
	if (sscanf(msg->url().c_str(), "/%[^/]/%[^/]/%d/%[^?]?%s", svar, svcType, &instId, varname,params) >=4)
	{

		if (_ag->getJSON_async(new SvarQueryCB(*_ag, _conn,_Logger),svcType, varname, instId) <=0)
		{	
			_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(ServerAgent, "onHeadersEnd get Json failed [%s]"),body.c_str());
			int code = 200;
			outmsg->code(code);
			outmsg->status(HttpMessage::code2status(code));
			outmsg->keepAlive(true);
			outmsg->header("Access-Control-Allow-Origin","*");
			outmsg->header("Access-Control-Allow-Methods","POST,GET");
			outmsg->header("Server","LibAsYnc HtTp SeRVer");
			outmsg->header("Date",HttpMessage::httpdate());
			outmsg->contentLength(body.length());

			std::string head = outmsg->toRaw();
			_conn.write(head.c_str(),head.length());
			_conn.write(body.c_str(),body.length());

			//_conn.setkeepAlive(false);
		}
	}
	return true;
}



// --------------------------------------------------
// class LoadFile
// --------------------------------------------------
LoadFile::LoadFile(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps)
:HttpHandler(conn,logger,dirProps, appProps),_curfile(NULL)
{
	Properties::const_iterator it = appProps.find("homedir");
	if( it != appProps.end()){
		_homedir = it->second;
	}
	it = appProps.find("sourcedir");
	if( it != appProps.end()){
		_sourcedir = it->second;
	}
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerAgent, "set home [%s] _sourcedir [%s]"),_homedir.c_str(),_sourcedir.c_str());
}

LoadFile::~LoadFile()
{	
	if(_curfile){
		fclose(_curfile);
		_curfile = NULL;
	}
}

bool LoadFile::onHeadersEnd( const HttpMessage::Ptr msg)
{
	HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::MSG_RESPONSE);
	printf("url = %s\n",msg->url().c_str());
	std::string url = msg->url();
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerAgent, "current url [%s]!"),url.c_str());
	std::string body = "404 Not found";
	//head
	int code = 200;
	int fileSize = 0;
	outmsg->code(code);
	outmsg->status(HttpMessage::code2status(code));
	outmsg->keepAlive(true);
	outmsg->header("Access-Control-Allow-Origin","*");
	outmsg->header("Access-Control-Allow-Methods","POST,GET");
	outmsg->header("Server","LibAsYnc HtTp SeRVer");
	outmsg->header("Date",HttpMessage::httpdate());

	char getway[10] = {0};
	char filepath[200] ={0};
	char prefix[100] = {0};
	char suffix[100] = {0};
	/*
		/xxxx/xx/xxx?xxxx
		getway + prefix + suffix
	*/
	sscanf(url.c_str(), "/%[^/]/%s", getway,filepath);
	sscanf(filepath, "%[^?]?%s", prefix,suffix);
	//get file
	if(0 == std::string(getway).find("fvar"))
	{
		char path[200] = {0};
		char file[200] = {0};
		if (sscanf(prefix, "%[^/]%s", path,file) >=0)
		{
			std::string filePath = "";
			std::string filePath2 = "";
			if(!strlen(suffix)){				
				filePath = _homedir + path + file;
				filePath2 = _sourcedir + path + file;
			}else{
				filePath = _homedir + path + file + "/" +suffix;
				filePath2 = _sourcedir + path + file + "/" +suffix;
			}
			_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerAgent, "file path1 [%s] path2 [%s]"),filePath.c_str(),filePath2.c_str());
			_curfile = fopen(filePath.c_str(), "rb");
			if(_curfile == NULL){ 
				_curfile = fopen(filePath2.c_str(), "rb");
			}
			if(_curfile != NULL){
				fseek(_curfile,0,SEEK_END);     //定位到文件末   
				fileSize = ftell(_curfile);
				rewind(_curfile);
			}else{
				_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(ServerAgent, "open file [%s] is failed!"),filePath2.c_str());
			}
		}
	}else
	{
		body += "unkown message type: " + std::string(getway);
		_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(ServerAgent, "unkown message type:"),body.c_str());
	}
	outmsg->contentLength(fileSize);
	std::string head = outmsg->toRaw();
	_conn.write(head.c_str(),head.length());

	return true;
}

void LoadFile::onHttpDataSent()
{
	char buf[MAXLENGTH*COUNTS];
	memset(buf,0,MAXLENGTH*COUNTS);
	/*
	if(_curfile){
		int ret = fread(buf,1,COUNTS*MAXLENGTH,_curfile);
		if(ret)
		{
			printf("[%d] [%s]\n",ret,buf);
			_conn.write(buf,ret);
		}else{
			fclose(_curfile);
			_curfile = NULL;
		}
	}*/
	if(_curfile){
		fgets(buf,MAXLENGTH*COUNTS,_curfile);
		if(strlen(buf)){
			//printf("[%d] [%s]\n",strlen(buf),buf);
			_conn.write(buf,strlen(buf));
		} else{
			fclose(_curfile);
			_curfile = NULL;
		}
	}
	//_conn.setkeepAlive(false);
}


}}//namespace ZQ::eloop
