// RuleEngineTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "RuleEngine.h"
#include <Log.h>
#include <filelog.h>
#include <io.h>
#include <iostream>
#include <vector>
//ZQ::common::Config::Loader< RuleEngineCfg > _config("");
//ZQ::common::FileLog * RuleEngineLog = NULL;
#define  RULEENGINELOGFILE "RuleEngine.log"

class CountStream : public ZQ::common::Action
{
public:
	CountStream(ZQ::common::RuleEngine& engine)
		:Action(engine, "CountStream")
	{
	}

	virtual ZQ::common::Action& operator() (Context& ctx, const Properties& input, Properties& output)
	{
		ctx.statusCode = aSucceed;
		long count = atoi(ctx.metaData["Event.streamcount"].c_str());
		char buf[32];
		snprintf(buf, sizeof(buf)-2, "%d", ++count);
		STL_MAPSET(Properties, output, "count", buf);
		return *this;
	}
};

class UpdateStreamCountToMetaData : public ZQ::common::Action
{
public:
	UpdateStreamCountToMetaData(ZQ::common::RuleEngine& engine)
		:Action(engine, "UpdateStreamCountToMetaData")
	{
	}

	virtual ZQ::common::Action& operator() (Context& ctx, const Properties& input, Properties& output)
	{
		ctx.statusCode = aSucceed;
		printf("UpdateStreamCountToMetaData\n");
		return *this;
	}
};

int _tmain(int argc, _TCHAR* argv[])
{

	char* configfile = "D:\\ZQProjs\\Common\\test\\RuleEngineTest\\";
	char* logFolder = "d:\\log";
	::std::string  strCurDir;
	::std::string  strLogfile;
	char           sModuleName[1025];

	if(_access(logFolder, 0))
	{
		DWORD dSize = GetModuleFileNameA(NULL,sModuleName,1024);
		sModuleName[dSize] = '\0';
		strCurDir = sModuleName;

		int nIndex = strCurDir.rfind('\\');
		strCurDir = strCurDir.substr(0,nIndex);
		nIndex = strCurDir.rfind('\\');
		strCurDir = strCurDir.substr(0,nIndex); //end with "\\"	
		strCurDir = strCurDir + "\\logs\\";
		strLogfile = strCurDir + RULEENGINELOGFILE;
	}
	else
	{
		DWORD dwLength;

		strLogfile = logFolder;
		dwLength = strLogfile.size();
		if(strLogfile[dwLength -1] != '\\')
		{
			strLogfile += "\\";
		}
		strLogfile += RULEENGINELOGFILE;
	}

	ZQ::common::FileLog RuleEngineLog(strLogfile.c_str(), ZQ::common::Log::L_DEBUG);

	char ConfigPath[260];
	strcpy(ConfigPath, configfile);
	strcat(ConfigPath, "\\RuleEngineTest.xml");
//	strcat(ConfigPath, "\\RuleEngine.xml.bz2");
//	strcat(ConfigPath, "\\aaa.xml.bz2");

	ZQ::common::NativeThreadPool _pThreadPool(5);
	ZQ::common::RuleEngine engine(RuleEngineLog, _pThreadPool, 5);
	engine.populate("D:\\ZQProjs\\TianShan\\bin");

	try {
		engine.loadConfig(ConfigPath, false);
		ZQ::common::Action::Properties prop;
		STL_MAPSET(ZQ::common::Action::Properties, prop, "streamcount", "23");
		engine.OnEvent("Stream", "Created", "stampISO8601", "123", prop);
	}
	catch(const ZQ::common::ExpatException& ex)
	{
		(RuleEngineLog)(ZQ::common::Log::L_ERROR, CLOGFMT(RuleEngine, "importXml(%s) caught XML exception: %s"), ConfigPath?ConfigPath:"null", ex.getString());
	}
	catch(...)
	{
		(RuleEngineLog)(ZQ::common::Log::L_ERROR, CLOGFMT(RuleEngine, "importXml(%s) caught unknown exception"), ConfigPath?ConfigPath:"null");
	}

	Sleep(1000*600);

	return 0;
}

