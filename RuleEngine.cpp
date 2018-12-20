// ===========================================================================
// Copyright (c) 2006 by
// syscheme, Shanghai,,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
//
// Ident : $Id: RuleEngine.cpp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/RuleEngine.cpp $
// 
// 10    4/10/14 9:41a Hui.shao
// 
// 9     1/24/11 6:39p Fei.huang
// 
// 8     12/30/10 7:17p Fei.huang
// 
// 7     12/30/10 6:42p Fei.huang
// 
// 6     12/30/10 6:29p Hui.shao
// 
// 5     12/29/10 6:42p Hui.shao
// let action[.ToNextAction] first read the parameters from ctx.metadata,
// then overwrite with those of input parameters if they were configured
// 
// 4     10-12-17 12:02 Hui.shao
// added more tracking on rule execution
// 
// 3     10-12-16 14:45 Hui.shao
// added an reserved action ".ToNextAction"
// 
// 2     10-12-15 15:14 Fei.huang
// suppress compiler warnings
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 12    10-10-20 17:52 Fei.huang
// * fix can't build on linux
// 
// 11    10-10-19 16:57 Haoyuan.lu
// 
// 10    10-04-02 13:57 Haoyuan.lu
// 
// 9     10-03-31 16:46 Haoyuan.lu
// 
// 8     10-03-24 13:24 Yixin.tian
// 
// 7     10-03-23 14:15 Yixin.tian
// merge for Linux OS
// 
// 6     10-03-23 12:56 Haoyuan.lu
// 
// 5     10-02-25 17:35 Haoyuan.lu
// 
// 4     09-08-11 10:58 Haoyuan.lu
// 
// 3     09-05-29 11:50 Hui.shao
// 
// 2     09-05-27 18:01 Hui.shao
// add nesting to allow sub rules
// 
// 1     09-05-27 16:11 Hui.shao
// simple rule engine
// ===========================================================================

#include "RuleEngine.h"
#include "TimeUtil.h"
#include "Guid.h"
#include "RuleEngineModule.h"
#include "SystemUtils.h"
#include "strHelper.h"

extern "C"
{
#ifdef ZQ_OS_MSWIN
	#include <io.h>
#else
	#include <glob.h>
#endif
};

namespace ZQ {
namespace common {

#define GREATER_THAN		"greater_than"
#define LESS_THAN			"less_than"
#define TIME_BEFORE			"time_before"
#define TIME_AFTER			"time_after"
#define SUB_STR				"sub_str"
#define MOD					"mod"

typedef bool (*PFucAddr)(const std::string&, const std::vector<std::string>&);

static bool greater_than(const std::string& arg, const std::vector<std::string>& argv)
{
	if(argv.size() < 2)
		return false;
	if(atof(arg.c_str()) <= atof(argv[1].c_str()))
	{
		return false;
	}
	else
		return true;
}

static bool less_than(const std::string& arg, const std::vector<std::string>& argv)
{
	if(argv.size() < 2)
		return false;
	if(atof(arg.c_str()) >= atof(argv[1].c_str()))
	{
		return false;
	}
	else
		return true;
}

static bool time_before(const std::string& arg, const std::vector<std::string>& argv)
{
	if(argv.size() < 2)
		return false;
	time_t timestamp1;
	time_t timestamp2;
	if(TimeUtil::Str2Time(arg.c_str(), timestamp1) && TimeUtil::Str2Time(argv[1].c_str(), timestamp2))
	{
		if(timestamp1 >= timestamp2)
			return false;
	}
	return true;
}

static bool time_after(const std::string& arg, const std::vector<std::string>& argv)
{
	if(argv.size() < 2)
		return false;
	time_t timestamp1;
	time_t timestamp2;
	if(TimeUtil::Str2Time(arg.c_str(), timestamp1) && TimeUtil::Str2Time(argv[1].c_str(), timestamp2))
	{
		if(timestamp1 <= timestamp2)
			return false;
	}
	return true;
}

static bool sub_str(const std::string& arg, const std::vector<std::string>& argv)
{
	if(argv.size() < 2)
		return false;
	for(size_t i = 1; i < argv.size(); i++)
	{
		if(argv[i].find(arg) != std::string::npos)
			return true;
		else
			continue;
	}
	return false;
}

/*
static bool file_exist(const std::string& arg, const std::vector<std::string>& argv)
{
	if(access(arg.c_str(), 0) == 0)
		return true;
	else
		return false;
}
*/

static bool mod(const std::string& arg, const std::vector<std::string>& argv)
{
	if(argv.size() < 2)
		return false;

#ifdef ZQ_OS_MSWIN	
	if( _atoi64(arg.c_str()) % atoi(argv[1].c_str()) == 0)
#else
	if( strtoull(arg.c_str(), (char**)NULL ,10) % atoi(argv[1].c_str()) == 0)
#endif
	{
		return true;
	}
	else
		return false;
}

static std::map<std::string, PFucAddr> FunDict;

/// -----------------------------
/// class XmlImporter
/// -----------------------------
class XmlImporter : public ZQ::common::ExpatBase
{

public:
	XmlImporter(ZQ::common::RuleEngine& engine) : _engine(engine)
	{
	}

	virtual ~XmlImporter() {}

	virtual int parseFile(const char *szFilename, bool compressed=false)
	{
		if (NULL == szFilename)
			return -1;

#ifndef ENABLE_COMPRESS
		compressed=false;
#endif // ENABLE_COMPRESS

		// Open the specified file
		std::ifstream file(szFilename);
		if (!file.is_open())
			return -2;

		std::istream* pin = &file;

#ifdef ENABLE_COMPRESS
		std::ifstream zfile(szFilename, ::std::ios::in | ::std::ios::binary);
		if (compressed && !zfile.is_open())
			return -3;

		ZQ::common::Bz2InStream unbz2(zfile);
		if (compressed)
			pin = &unbz2;
#endif // ENABLE_COMPRESS

		// Read to EOF
		char szBuffer[8192];
		int cBytes = 0;
		for (bool done = (*pin).eof(); !done;)
		{
			// Read data from the input file; store the bytes read
			(*pin).read(szBuffer, sizeof(szBuffer));
			done = (*pin).eof();
			ZQ::common::ExpatBase::parse(szBuffer, (*pin).gcount(), done);
			cBytes += (*pin).gcount();
		}
		return cBytes;
	}

protected:
	ZQ::common::RuleEngine&    _engine;
	ZQ::common::RuleEngine::Rule _rule;
	ZQ::common::RuleEngine::RuleItem _ruleItem;
	std::string _currentAction, _currentRule;
	std::string _currentCategory, _currentEvent;
	std::vector<std::string> _EventRules;
	typedef ::std::map < ::std::string, ::std::string> Properties;
	Properties _MetaData;

	// overrideable callbacks, from ExpatBase
	virtual void OnStartElement(const XML_Char* name, const XML_Char** atts)
	{
		::std::string hiberarchyName = getHiberarchyName();
		Properties attrMap;
		for (int n = 0; atts[n]; n += 2)
			STL_MAPSET(Properties, attrMap, atts[n], atts[ n + 1 ]);

		if (0 == hiberarchyName.compare("/Rules/Rule"))
		{
			_currentRule = "";
			_currentRule = attrMap["name"];
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action"))
		{
			_currentAction = "";
			_currentAction = attrMap["name"];
			_ruleItem.actionName = attrMap["name"];
			_ruleItem.isASubRule = false;
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Exec"))
		{
			_ruleItem.isASubRule = true;
			_ruleItem.actionName = attrMap["rule"];
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action/Condition") && !_currentAction.empty())
		{
			STL_MAPSET(Properties, _ruleItem.conditions, attrMap["ctxMetaData"], attrMap["evaluation"]);
//			STL_MAPSET(Properties, _ruleItem.conditions, SYS_PROP(value), attrMap["evaluation"]);
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action/arg") && !_currentAction.empty())
		{
			STL_MAPSET(Properties, _ruleItem.inputArgs, attrMap["name"], attrMap["value"]);
//			STL_MAPSET(Properties, _ruleItem.inputArgs, SYS_PROP(value), attrMap["value"]);
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action/ResultMetaData") && !_currentAction.empty())
		{
			_MetaData.clear();
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action/ResultMetaData/result") && !_currentAction.empty())
		{
			STL_MAPSET(Properties, _MetaData, attrMap["outputName"], attrMap["ctxMetaData"]);
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Event"))
		{
			_EventRules.clear();
			_currentCategory = "";
			_currentEvent = "";
			_currentCategory = attrMap["category"];
			_currentEvent = attrMap["event"];
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Event/Exec") && !_currentEvent.empty())
		{
			_EventRules.push_back(attrMap["rule"]);
			return;
		}

	}

	virtual void OnEndElement(const XML_Char*)
	{
		::std::string hiberarchyName = getHiberarchyName();
		if (0 == hiberarchyName.compare("/Rules/Rule") && !_currentRule.empty())
		{
			//create a new rule
			_engine.registerRule(_currentRule, _rule);
			_rule.clear();
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action") && !_currentAction.empty() && !_currentRule.empty())
		{
			//create a new action for current rule
			_rule.push_back(_ruleItem);
			//clear Rule Item
			_ruleItem.actionName = "";
			_ruleItem.conditions.clear();
			_ruleItem.inputArgs.clear();
			_ruleItem.output2ctx.clear();
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Exec"))
		{
			_rule.push_back(_ruleItem);
			_ruleItem.actionName = "";
			_ruleItem.conditions.clear();
			_ruleItem.inputArgs.clear();
			_ruleItem.output2ctx.clear();
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action/Condition") && !_currentAction.empty())
		{
			_MetaData.clear();
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action/arg") && !_currentAction.empty())
		{
//			_ruleItem.inputArgs.clear();
			_MetaData.clear();
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action/ResultMetaData") && !_currentAction.empty())
		{
			_ruleItem.output2ctx = _MetaData;
			_MetaData.clear();
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Rule/Action/ResultMetaData/result") && !_currentAction.empty())
		{
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Event"))
		{
			_engine.applyRulesToEvent(_currentCategory, _currentEvent, _EventRules);
			return;
		}

		if (0 == hiberarchyName.compare("/Rules/Event/Exec") && !_currentEvent.empty())
		{
			return;
		}
	}

	virtual void OnCharData(const XML_Char*, int len) {}
	virtual void OnLogicalClose() {}
};


// -----------------------------
// class RuleExecCmd
// -----------------------------
class RuleExecCmd : public ThreadRequest
{
public:
	typedef void (*cbRuleExecFinished) (RuleExecCmd* pExec);

	/// constructor
	///@note no direct instantiation of ProvisionCmd is allowed
	RuleExecCmd(RuleEngine& engine, const RuleEngine::Rule ruleStack, const std::string& ruleName, const std::string& userTxnId="", uint depth=0, cbRuleExecFinished cbFinish=NULL)
		: ThreadRequest(engine._thpool), _engine(engine), _cbFinish(cbFinish),
		  _rule(ruleStack), _ruleName(ruleName), _userTxnId(userTxnId), _depth(depth)
	{
		if (_userTxnId != "")
		{
			ZQ::common::MutexGuard g(_lkUserTxnMap);
			Map::iterator it = _userTxnMap.find(_userTxnId);
			if (_userTxnMap.end() != it && this != it->second)
				throw ZQ::common::Exception("a RuleExecCmd of the same request is queued, ignore this post");

			_userTxnMap.insert(Map::value_type(_userTxnId, this));
		}

		_ctx.statusCode = Action::aSucceed;
		_itAction = _rule.begin();
		_stampCreated = now();
	}

	virtual ~RuleExecCmd()
	{
	}

protected:
	/// constructor to start a branch for a sub-rule
	RuleExecCmd(const RuleExecCmd& parent, const RuleEngine::Rule subRule, const std::string& subRuleName, uint branchId =0)
		: ThreadRequest(parent._engine._thpool), _ctx(parent._ctx), _engine(parent._engine), _cbFinish(parent._cbFinish),
		  _rule(subRule), _ruleName(subRuleName), _depth(parent._depth+1)
	{
		if (!parent._userTxnId.empty())
		{
			char buf[16];
			snprintf(buf, sizeof(buf)-2, "#%d", branchId);
			_userTxnId = parent._userTxnId + buf;
		}

		_itAction = _rule.begin();
		_stampCreated = now();
	}

protected: // impls of ThreadRequest

	virtual bool init(void)
	{ return _itAction < _rule.end(); }

	virtual void final(int retcode =0, bool bCancelled =false)
	{ delete this; }

	virtual int run(void)
	{
		{
			ZQ::common::MutexGuard g(_lkUserTxnMap);
			Map::iterator it = _userTxnMap.find(_userTxnId);
			if (_userTxnMap.end() != it)
				_userTxnMap.erase(it);
		}

		_engine.ctxlog()(Log::L_DEBUG, CLOGFMT(RuleEngine, "RuleExecCmd[%p] run()"), this);
		uint64 stampNow = now();
		uint64 stampLast = stampNow;

		for (; _itAction < _rule.end(); _itAction++)
		{
			// if the iteration is a sub rule, take a new RuleExecCmd to execute it
			if (_itAction->isASubRule)
			{
				MutexGuard g(_engine._lkRuleMap);
				RuleEngine::RuleMap::const_iterator itSubRule = _engine._ruleMap.find(_itAction->actionName);

				if (_engine._ruleMap.end() == itSubRule)
					continue;

				try {
					
					_engine.ctxlog()(Log::L_DEBUG, CLOGFMT(RuleEngine, "RuleExecCmd[%p] starting subrule[%s]"), this, _itAction->actionName.c_str());
					(new RuleExecCmd(*this, itSubRule->second, _itAction->actionName, int(_itAction - _rule.begin())))->start();
				}
				catch (...) {}

				continue;
			}

			// step 1 TODO: test the condition if need if any condition returns false
			bool bMeetAll =  true;
			for (Action::Properties::const_iterator itCondition = _itAction->conditions.begin(); itCondition != _itAction->conditions.end(); itCondition++)
			{
				std::string arg;
				if(_ctx.metaData.end() == _ctx.metaData.find(itCondition->first))
				{
					bMeetAll =  false;
					_engine.ctxlog()(Log::L_ERROR, CLOGFMT(RuleEngine, "RuleExecCmd[%p] MetaData[%s] not found when starting action[%s]"), this, itCondition->first.c_str(), _itAction->actionName.c_str());
					break;
				}
				else
				{
					arg = _ctx.metaData[itCondition->first];
				}
				if (itCondition->second.empty())
					continue;
				else
				{
					::std::vector< ::std::string> argv = ZQ::common::stringHelper::split(itCondition->second, ' ');
					if(FunDict.end() != FunDict.find(argv[0]))
					{
						PFucAddr f = FunDict[argv[0]];
						if(f)
						{
							bMeetAll = f(arg, argv);
							if(!bMeetAll)
								break;
						}
					}
				}
			}		
			if(! bMeetAll)	// if any condition returns false, skip current action
				continue;
			_engine.ctxlog()(Log::L_DEBUG, CLOGFMT(RuleEngine, "RuleExecCmd[%p] executing action[%s]"), this, _itAction->actionName.c_str());

			// step 2 find the action by name
			Action*  pAction =NULL;
			char buf[64];
			snprintf(buf, sizeof(buf)-2, "%d", int(_itAction - _rule.begin()));
			STL_MAPSET(Action::Properties, _ctx.metaData, SYS_PROP(LastActionName), _itAction->actionName);
			STL_MAPSET(Action::Properties, _ctx.metaData, SYS_PROP(LastActionIdx), buf);

			// the reserved action: ".ToNextAction"
			if (0 == _itAction->actionName.compare(".ToNextAction"))
			{
				size_t skipSize =0;
				std::string lookforActionName;

				// read the ctx metadata[".ToNextAction.skipSize"] and metadata[".ToNextAction.ActionName"] if available
				Action::Properties::const_iterator it = _ctx.metaData.find(".ToNextAction.skipSize");
				if (_ctx.metaData.end() != it)
					skipSize = atoi(it->second.c_str());

				it = _ctx.metaData.find(".ToNextAction.ActionName");
				if (_ctx.metaData.end() != it)
					lookforActionName = it->second;

				// read the input argument ".ToNextAction.skipSize" and overwrite that of ctx.metadata if it has been set
				it = _itAction->inputArgs.find(".ToNextAction.skipSize");
				if (_itAction->inputArgs.end() != it)
					skipSize = atoi(it->second.c_str());

				if (skipSize >0)
					_itAction += skipSize;

				// read the input argument ".ToNextAction.ActionName" and overwrite that of ctx.metadata if it has been set, search for the next matched action name if yes
				it = _itAction->inputArgs.find(".ToNextAction.ActionName");
				if (_itAction->inputArgs.end() != it && !it->second.empty())
					lookforActionName = it->second;

				if (!lookforActionName.empty())
				{
					RuleEngine::Rule::iterator itNextAct = _itAction+1;
					while (itNextAct < _rule.end() && 0 != itNextAct->actionName.compare(lookforActionName))
						itNextAct++;

					_itAction = itNextAct-1;
				}

				continue; // end of executing ".ToNextAction"
			}

			{
				MutexGuard g(_engine._lkActions);
				RuleEngine::ActionMap::const_iterator it = _engine._actions.find(_itAction->actionName);
				if (_engine._actions.end() != it)
					pAction = it->second;

				if (NULL == pAction)
				{
					_engine.ctxlog()(Log::L_ERROR, CLOGFMT(RuleEngine, "RuleExecCmd[%p] action[%s] not found, invoke failed, given up the following action in the rule"), this, _itAction->actionName.c_str());
					_ctx.statusCode = Action::aFailedQuit;
					break;
				}
			}

			// step 3. execute the action
			try{
				_lastOutput.clear(); 
				(*pAction)(_ctx, _itAction->inputArgs, _lastOutput);
			}
			catch(...)
			{
				_engine.ctxlog()(Log::L_ERROR, CLOGFMT(RuleEngine, "RuleExecCmd[%p] action[%s] caught exception, given up the following action in the rule"), this, _itAction->actionName.c_str());
				_ctx.statusCode = Action::aFailedQuit;
				break;
			}

			stampNow = now();
			_engine.ctxlog()(Log::L_DEBUG, CLOGFMT(RuleEngine, "RuleExecCmd[%p] action[%s] took %lldmsec returned (%d) on the following actions"), this, _itAction->actionName.c_str(), stampNow - stampLast, _ctx.statusCode);
			stampLast = stampNow;
			
			if (Action::aFailed == _ctx.statusCode)
				continue;
			
			if (Action::aFailedQuit == _ctx.statusCode)
				break;

			// step 4 copy the needed output parameter into the context
			for (Action::Properties::const_iterator itOut = _itAction->output2ctx.begin(); itOut != _itAction->output2ctx.end(); itOut++)
			{
				if (itOut->second.empty() || _lastOutput.end() == _lastOutput.find(itOut->first))
					continue;

				STL_MAPSET(Action::Properties, _ctx.metaData, itOut->second, _lastOutput[itOut->first]);
			}

			if (Action::aSucceedQuit == _ctx.statusCode)
				break;
		}

		_engine.ctxlog()(Log::L_INFO, CLOGFMT(RuleEngine, "RuleExecCmd[%p] reached the end of actions with (%d), took %lldmsec"), this, _ctx.statusCode, now() - _stampCreated);
		if (NULL != _cbFinish)
		{
			try {
				_engine.ctxlog()(Log::L_DEBUG, CLOGFMT(RuleEngine, "RuleExecCmd[%p] calling finish callback[%p]"), this, _cbFinish);
				_cbFinish(this);
			}
			catch(...)
			{
				_engine.ctxlog()(Log::L_ERROR, CLOGFMT(RuleEngine, "RuleExecCmd[%p] caught exception on callback[%p]"), this, _cbFinish);
			}
		}

		try {
			if (_depth >0) // skip the event dispather cmd
				_engine.OnRuleExecuted(_ruleName, uint64 ((void*)this), _ctx, _userTxnId);
		}
		catch(...)
		{
			_engine.ctxlog()(Log::L_ERROR, CLOGFMT(RuleEngine, "RuleExecCmd[%p] caught exception on callback[%p]"), this, _cbFinish);
		}

		return 0;
	}

public:
	Action::Context    _ctx;
	Action::Properties _lastOutput; 

protected:

	RuleEngine&    _engine;
	uint64         _stampCreated;
	cbRuleExecFinished _cbFinish;

	RuleEngine::Rule   _rule;
	RuleEngine::Rule::iterator	_itAction;

	std::string _ruleName,_userTxnId; 
	uint		_depth;

	typedef std::map<std::string, RuleExecCmd*> Map; // user's transaction id to exec to avoid duplicate posts
	static Map _userTxnMap;
	static ZQ::common::Mutex _lkUserTxnMap;
};

ZQ::common::Mutex RuleExecCmd::_lkUserTxnMap;
RuleExecCmd::Map RuleExecCmd::_userTxnMap;

// -----------------------------
// class RuleExecCmdLocal
// -----------------------------
class RuleExecCmdLocal : public RuleExecCmd
{
public:
	RuleExecCmdLocal(RuleEngine& engine, const RuleEngine::Rule ruleStack, const std::string& ruleName, std::string userTxnId="", uint depth=0)
	: RuleExecCmd(engine, ruleStack, ruleName, userTxnId, depth, _cbRuleExecFinished)
	{
	}

	virtual ~RuleExecCmdLocal()
	{
	}

	bool tryWait(long timeout = -1)
	{
		if (_engine._invokeTimeout >0 && (timeout <0 || timeout >_engine._invokeTimeout))
		{
			_engine.ctxlog()(Log::L_ERROR, CLOGFMT(RuleExecCmdLocal, "[%p] timeout %dmsec is adjusted to max %dmsec"), this, timeout, _engine._invokeTimeout);
			timeout = _engine._invokeTimeout;
		}

		if (SYS::SingleObject::TIMEDOUT == _hWakeup.wait(timeout))
			return true;

		return false;
	}

protected:
	void final(int retcode =0, bool bCancelled =false)
	{ } // do not delete self

private:
	static void _cbRuleExecFinished (RuleExecCmd* pExec)
	{
		RuleExecCmdLocal* pThis = (RuleExecCmdLocal*) (pExec);

		if (NULL != pThis)
			pThis->_hWakeup.signal();
	}

	SYS::SingleObject _hWakeup;
};

// -----------------------------
// class RuleEngine
// -----------------------------
RuleEngine::RuleEngine(Log& log, NativeThreadPool& thpool, int depth) //, const ::std::string& defaultEventChannelEndpoint)
: _thpool(thpool), _log(log), _invokeTimeout(20000), _depth(depth) // , _eventchEndpoint(defaultEventChannelEndpoint)
{
	FunDict.insert(make_pair(GREATER_THAN, greater_than));
	FunDict.insert(make_pair(LESS_THAN, less_than));
	FunDict.insert(make_pair(TIME_BEFORE, time_before));
	FunDict.insert(make_pair(TIME_AFTER, time_after));
	FunDict.insert(make_pair(SUB_STR, sub_str));
	FunDict.insert(make_pair(MOD, mod));
}

RuleEngine::~RuleEngine()
{
	ZQ::common::MutexGuard sync(_lockModuleObjMap);
	MoudleObjMap::iterator it = _ModuleObjMap.begin();
	for (; _ModuleObjMap.end() != it; it++)
	{
		if (!it->second)
			continue;

		try {
			ActionFacet	af(*(it->second));
			af.UnInitialize();
		}catch(...) {}

		try{delete it->second;}catch (...) {}
	}
	_ModuleObjMap.clear();
}

int RuleEngine::populate(const char* path)
{
	if (NULL == path)
		return _ModuleObjMap.size();

	std::string wkpath = path;
	if (wkpath[wkpath.length()-1] != FNSEPC)
		wkpath +=FNSEPS;

#ifdef ZQ_OS_MSWIN
	std::string filespec = wkpath + Module_PREFIX "*.dll";

	long hFile;
	struct _finddata_t c_file;

	for (bool moreAddin= ((hFile = ::_findfirst(filespec.c_str(), &c_file)) !=-1L);
		moreAddin;
		moreAddin =(_findnext(hFile, &c_file) ==0))
	{
		std::string phofile = wkpath;
		phofile += c_file.name;
		bool ret = load(phofile.c_str());
		if(ret)
			_log(Log::L_INFO, CLOGFMT(RuleEngine, "populate() load  library file [%s] success"), phofile.c_str());
		else
			_log(Log::L_ERROR, CLOGFMT(RuleEngine, "populate() load  library file [%s] failed"), phofile.c_str());
			
	}

	_findclose(hFile);

#else
	std::string filespec = wkpath + "lib" Module_PREFIX "*.so";

    glob_t gl;
    if(!glob(filespec.c_str(), GLOB_PERIOD|GLOB_TILDE, 0, &gl))
    {
        for(size_t i = 0; i < gl.gl_pathc; ++i)
        {
            bool ret = load(gl.gl_pathv[i]);
			if(ret)
				_log(Log::L_INFO, CLOGFMT(RuleEngine, "populate() load  library file [%s] success"), gl.gl_pathv[i]);
			else
				_log(Log::L_ERROR, CLOGFMT(RuleEngine, "populate() load  library file [%s] failed"), gl.gl_pathv[i]);
        }
    }
	else
		_log(Log::L_ERROR, CLOGFMT(RuleEngine, "populate() get library file pattern [%s] failed"), filespec.c_str());
		
#endif

	return _ModuleObjMap.size();
}

bool RuleEngine::load(const char* filename)
{
	if (NULL == filename)
		return false;

	ZQ::common::DynSharedObj* dso = new ZQ::common::DynSharedObj(filename);

	ZQ::common::MutexGuard sync(_lockModuleObjMap);
	MoudleObjMap::iterator it = _ModuleObjMap.find((dso->getImageInfo())->filename);
	if (_ModuleObjMap.end() != it)
	{
		return false;
	}

	try
	{
		ActionFacet	 af(*dso);

		// test if the required APIs have been mapped as expected
		if (!af.isValid())
			return false;

		// now start invoke the API if necessary
		af.Initialize(this, &_log);

		_ModuleObjMap[(dso->getImageInfo())->filename] = dso;
	}
	catch(...)
	{
		_log(Log::L_ERROR, CLOGFMT(RuleEngine, "load() failed to load %s"), (dso->getImageInfo())->filename);
		return false;
	}

	return true;
}

void RuleEngine::subscribeEvents(const ::std::string& topic)
{
}

bool RuleEngine::registerRule(const ::std::string& ruleName, const Rule& Rule)
{
	if (ruleName.empty())
		return false;

	if (Rule.size() <=0)
	{
		MutexGuard g(_lkRuleMap);
		_ruleMap.erase(ruleName);
		return true;
	}

	bool hasSubRule = false;

	{
		MutexGuard g(_lkActions);
		for (size_t i = 0; i < Rule.size(); i++)
		{
			if (Rule[i].isASubRule)
			{
				hasSubRule = true;
				continue;
			}

			static const char* reservedActNames[] = 
			{
				".ToNextAction",
				NULL,
			};

			if (Rule[i].actionName.empty() || _actions.end() == _actions.find(Rule[i].actionName))
			{
				int j=0;
				while (reservedActNames[j] && 0 != Rule[i].actionName.compare(reservedActNames[j]))
					j++;

				if (!reservedActNames[j])
				{
					_log(Log::L_ERROR, CLOGFMT(RuleEngine, "registerRule() No.%d action[%s] not recoganized, register failed"), i, Rule[i].actionName.c_str());
					return false;
				}
			}
		}
	}

	MutexGuard g(_lkRuleMap);
	if (hasSubRule)
	{
		int currentDepth = 0;
		int currentMax = 0;
		maxDepth(Rule, currentDepth, currentMax, ruleName, Rule);
		if(currentMax > _depth)
		{
			_log(Log::L_ERROR, CLOGFMT(RuleEngine, "registerRule(%s) failed, too deep"), ruleName.c_str());
			return false;
		}
	}

	STL_MAPSET(RuleMap, _ruleMap, ruleName, Rule);
	return true;
}

bool RuleEngine::applyRulesToEvent(const ::std::string& category, const ::std::string& eventName, const ::std::vector< ::std::string> ruleNames)
{
	if (category.empty() || eventName.empty())
		return false;

	std::string searchKey = category + "$" + eventName;
	if (searchKey.length() <3)
		return false;

	::std::vector< ::std::string> ruleNamesToApply;
	for (::std::vector< ::std::string>::const_iterator it = ruleNames.begin(); it < ruleNames.end(); it++)
		if (!it->empty())
			ruleNamesToApply.push_back(*it);

	if (ruleNamesToApply.size() <=0)
	{
		MutexGuard g(_lkEventToRules);
		_eventToRules.erase(searchKey);
		return true;
	}

	MutexGuard g(_lkEventToRules);
	STL_MAPSET(EventToRulesMap, _eventToRules, searchKey, ruleNamesToApply);

	return true;
}

const ::std::vector< ::std::string>& RuleEngine::getRulesOfEvent(const ::std::string& category, const ::std::string& eventName)
{
	static const ::std::vector< ::std::string> NilStack;
	std::string searchKey = category + "$" + eventName;

	MutexGuard g(_lkEventToRules);
	EventToRulesMap::iterator it = _eventToRules.find(searchKey);

	if (_eventToRules.end() == it)
		return NilStack;

	return it->second;
}

void RuleEngine::OnEvent(const std::string& category, const std::string& eventName, const std::string& stampISO8601, const std::string& sourceNetId, const Action::Properties& params, const std::string& userTxnId)
{
	Rule rule;
	std::string searchKey = category + "$" + eventName;

	{
		MutexGuard g(_lkEventToRules);
		EventToRulesMap::iterator it = _eventToRules.find(searchKey);

		if (_eventToRules.end() == it || it->second.size() <=0)
			return; // do nothing if the event is not attended

		for (::std::vector< ::std::string>::const_iterator itRule = it->second.begin(); itRule != it->second.end(); itRule ++)
		{
			RuleItem ri;
			ri.actionName = *itRule;
			ri.isASubRule = true;
			ri.inputArgs.insert(Action::Properties::value_type("Invoke.type", "event"));

			rule.push_back(ri);
		}
	}

	try {
		RuleExecCmd* pCmd = new RuleExecCmd(*this, rule, std::string("dispathEvent(") + searchKey +")", userTxnId, 0);
		if (NULL == pCmd)
		{
			_log(Log::L_DEBUG, CLOGFMT(RuleEngine, "OnEvent() failed to allocate RuleExecCmd for the event[%s::%s] from[%s] asof[%s]"), category.c_str(), eventName.c_str(), sourceNetId.c_str(), stampISO8601.c_str());
			return;
		}

		Action::Context& ctx = pCmd->_ctx;
		ctx.metaData.insert(Action::Properties::value_type("Event.category", category));
		ctx.metaData.insert(Action::Properties::value_type("Event.name", eventName));
		ctx.metaData.insert(Action::Properties::value_type("Event.sourceNetId", sourceNetId));
		ctx.metaData.insert(Action::Properties::value_type("Event.stampISO8601", stampISO8601));

		for (Action::Properties::const_iterator it = params.begin(); it != params.end(); it++)
			ctx.metaData.insert(Action::Properties::value_type(std::string("Event.") + it->first, it->second));

		_log(Log::L_DEBUG, CLOGFMT(RuleEngine, "OnEvent() starting RuleExecCmd[%p] for the event[%s::%s] from[%s] asof[%s]"), pCmd, category.c_str(), eventName.c_str(), sourceNetId.c_str(), stampISO8601.c_str());
		pCmd->start();
	}
	catch(const ZQ::common::Exception& ex)
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(RuleEngine,"OnEvent() caugh exception: %s"), ex.getString());
	}
	catch(...)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RuleEngine, "OnEvent() caught exception when add RuleExecCmd"));
	}
}


Log& RuleEngine::ctxlog()
{
	return _log;
}

/*
MetaDataLibraryPtr& RuleEngine::metadataLib(const ::std::string& libname)
{
}
*/

bool RuleEngine::invoke(Action::Context& context, const ::std::string& actionName, const Action::Properties& input, Action::Properties& output, const long timeout)
{
	RuleExecCmdLocal* pCmd = NULL;
	{
		Rule rule;
		RuleItem ri;
		ri.actionName = actionName;
		ri.isASubRule = false;
		ri.inputArgs = input;
		ri.inputArgs.insert(Action::Properties::value_type("Invoke.type", "internal"));
		ri.output2ctx.insert(Action::Properties::value_type("*", ""));

		rule.push_back(ri);
		pCmd = new RuleExecCmdLocal(*this, rule, std::string("#") + actionName);
	}

	if (NULL == pCmd)
	{
		_log(Log::L_DEBUG, CLOGFMT(RuleEngine, "invoke() failed to initiate RuleExecCmdLocal action[%s]"), actionName.c_str());
		return false;
	}

	_log(Log::L_DEBUG, CLOGFMT(RuleEngine, "invoke() starting RuleExecCmdLocal[%p] to call action[%s]"), pCmd, actionName.c_str());
	pCmd->_ctx = context;
	pCmd->start();

	if (!pCmd->tryWait(timeout) || Action::aFailedQuit == pCmd->_ctx.statusCode && Action::aFailed == pCmd->_ctx.statusCode)
	{
		delete pCmd;
		return false;
	}

	// flush back the context after run to the input context
	context = pCmd->_ctx;
	// copy the output of invocation to return
	output =  pCmd->_lastOutput;

	delete pCmd;
	return true;
}

bool RuleEngine::registerAction(const ::std::string& actionName, Action& act, const ::std::string& alias)
{
	::std::string actSearchName = !alias.empty() ? alias : actionName;
	if (actSearchName.empty())
		return false;

	MutexGuard g(_lkActions);
	STL_MAPSET(ActionMap, _actions, actSearchName, &act); 
	_log(Log::L_DEBUG, CLOGFMT(RuleEngine, "registerAction() action[%s] registered, original name[%s]"), actSearchName.c_str(), actionName.c_str());
	return true;
}

bool RuleEngine::unregisterAction(const ::std::string& actionSearchName)
{
	if (actionSearchName.empty())
		return false;

	MutexGuard g(_lkActions);
	ActionMap::iterator it = _actions.find(actionSearchName);
	if (_actions.end() == it) // || &act != it->second)
		return false;

	_actions.erase(it);
	_log(Log::L_DEBUG, CLOGFMT(RuleEngine, "unregisterAction() action[%s] unregistered"), actionSearchName.c_str());
	return true;
}

void RuleEngine::maxDepth(const Rule& rule, int& currentDepth, int& currentMax, const ::std::string& ruleName, const Rule& registerRule)
{
	if(currentMax > _depth || currentDepth > _depth)
		return;
	for (size_t i = 0; i < rule.size(); i++)
	{
		if (rule[i].isASubRule)
		{
			currentDepth++;

			if(rule[i].actionName == ruleName)
			{
				maxDepth(registerRule, currentDepth, currentMax, ruleName, registerRule);			
			}
			else
			{
				MutexGuard g(_lkRuleMap);
				RuleEngine::RuleMap::const_iterator itSubRule = _ruleMap.find(rule[i].actionName);

				if (_ruleMap.end() == itSubRule)
				{
					if(currentDepth > currentMax)
						currentMax = currentDepth;
					currentDepth = 0;
					continue;
				}
				else
				{
					maxDepth(itSubRule->second, currentDepth, currentMax, ruleName, registerRule);
				}
			}
		}
	}
	if(currentDepth > currentMax)
		currentMax = currentDepth;
	currentDepth = 0;
}

bool RuleEngine::loadConfig(const char* filename, bool compressed)
{
	ZQ::common::XmlImporter importer(*this);
	int size = importer.parseFile(filename, compressed);
	_log(Log::L_DEBUG, CLOGFMT(RuleEngine, "importXml(%s) processed, %d bytes"), filename?filename:"null", size);
	if (size > 0)
		return true;
	else
		return false;
}

// -----------------------------
// class Action
// -----------------------------
Action::Action(RuleEngine& engine, const char* name)
: _engine(engine)
{
	_name = (NULL == name)? "NULL" : name;
	_engine.registerAction(_name, *this);
}

Action::~Action()
{
	_engine.unregisterAction(_name);
}	

Action& Action::operator() (Context& ctx, const Properties& input, Properties& output)
{
	ctx.statusCode = aSucceed;
	output.clear();
	return *this;
}

}}
