#pragma once
#ifndef    _SCRIPT_PARSER_H
#define   _SCRIPT_PARSER_H

#include <vector>

#include "expatxx.h"
#include "RuleEngine.h"


using namespace  ZQ::common;
namespace  HttpImpl
{
	  // help struct
typedef struct _logger{
	std::string name;
	ZQ::common::Log::loglevel_t level;
	int size;
	uint count;
} Logger;

typedef struct _item{
	  std::string  name;
	  uint             type;
	  std::string   url;
} Iterm;
typedef std::vector<Iterm> Iterms;

typedef struct _pool{
	  std::string   serverIP;
	  uint				port;
	  uint				clientNum;
	  uint                 interval;
	  uint                 timeout;
	  Iterms				itermS;
} Pool;
typedef  std::vector<Pool> Pools;

typedef struct _sesscontext{
	  Pools         itermPool;
} SessContext;
//
// class ScriptParser using to parser the xml file
//
class ScriptParser : public ZQ::common::ExpatBase
{
public:
	  ScriptParser(const std::string& szFilename);
	  ~ScriptParser(void);

public:

	  static uint to_uint (const std::string& src); 
	  static int64 to_int64(const std::string& src); 
	  static std::string to_string(size_t);

public:
	  virtual void OnStartElement(const XML_Char* name, const XML_Char** atts);
	  virtual void OnEndElement(const XML_Char* name);

	  virtual void processingInstruction(const XML_Char* target, const XML_Char* data){}
	  virtual void defaultHandler(const XML_Char*, int len){}
	  virtual int  notStandaloneHandler(){ return 0;}
	  virtual void unparsedEntityDecl(const XML_Char* entityName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId, const XML_Char* notationName){}
	  virtual void notationDecl(const XML_Char* notationName, const XML_Char* base, const XML_Char* systemId, const XML_Char* publicId){}
	  virtual void OnCharData(const XML_Char*, int len){}
	  virtual void OnStartNamespace(const XML_Char* prefix, const XML_Char* uri){}
	  virtual void OnEndNamespace(const XML_Char*){}
	  virtual void OnLogicalClose(){}

public:
	  //current Node
	  Iterm                                  cIterm;
	  Pool									cPool;

	  //xml data
	  SessContext						_sessContext;

	  Logger                               _loger;




};

}

#endif
