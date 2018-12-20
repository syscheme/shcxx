#include <sstream>

#include "ScriptParser.h"

namespace HttpImpl{
ScriptParser::ScriptParser(const std::string& szFilename)
{
	  parse(szFilename.c_str());
}

ScriptParser::~ScriptParser(void)
{
}

uint ScriptParser::to_uint (const std::string& src)
{
	  uint tmp;
	  std::istringstream iss(src);
	  iss >> tmp;
	  return tmp;
}

int64 ScriptParser::to_int64(const std::string& src)
{
	  int64 tmp;
	  std::istringstream iss(src);
	  iss >> tmp;
	  return tmp;
}

std::string ScriptParser::to_string(size_t value)
{
	  std::ostringstream oss;
	  oss << value;
	  return oss.str();
}

void ScriptParser::OnStartElement(const XML_Char* name, const XML_Char** atts)
{
	  ZQ::common::Action::Properties attrs;
	  /* keep all attributes for current node */
	  std::string hierarchyName = getHiberarchyName();
	  for(short i = 0; atts[i]; i+=2) {
			::std::string key_v = "";
			key_v.assign( (char *)atts[i]);
			::std::string value_v = "";
			value_v.assign((char*)atts[i+1]);
			attrs[key_v] = value_v;
	  }
	  
	   if (hierarchyName == "/ClientTest")
			 return;

	  if (hierarchyName == "/ClientTest/Log")
	  {
			_loger.level = (ZQ::common::Log::loglevel_t)to_uint(attrs["level"]);
			_loger.name = attrs["filename"];
			_loger.size = to_uint(attrs["size"]);
			_loger.count = to_uint(attrs["count"]);
			return;
	  }

	  if (hierarchyName == "/ClientTest/Session")
			return;

	  if( hierarchyName == "/ClientTest/Session/Pool")
	  {
			cPool.serverIP = attrs["serverIP"];
			cPool.port = to_uint(attrs["port"]);
			cPool.interval= to_uint(attrs["interval"]);
			cPool.timeout= to_uint(attrs["timeout"]);
			cPool.clientNum = to_uint(attrs["Maxclient"]);
	  }
	  if( hierarchyName == "/ClientTest/Session/Pool/Item")
	  {
			cIterm.name = attrs["name"];
			cIterm.type = to_uint(attrs["type"]);
			cIterm.url = attrs["url"];
			return;
	  }
	  return;
}

void ScriptParser::OnEndElement(const XML_Char* name)
{
	  std::string hierarchyName = getHiberarchyName();
	  if (hierarchyName == "/ClientTest")
			return;
	  if (hierarchyName == "/ClientTest/Log")
			return;
	  if (hierarchyName == "/ClientTest/Session")
			return;
	  if( hierarchyName == "/ClientTest/Session/Pool")
	  {
			_sessContext.itermPool.push_back(cPool);
			cPool.clientNum = 0;
			cPool.serverIP.clear();
			cPool.port = 0;
			cPool.itermS.clear();
			return;
	  }
	  if( hierarchyName == "/ClientTest/Session/Pool/Item")
	  {
			cPool.itermS.push_back(cIterm);
			cIterm.name.clear();
			cIterm.type = 0;
			return;
	  }

}

}
