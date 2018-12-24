// ===========================================================================
// Copyright (c) 2006 by
// syscheme, Shanghai
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Poscontention, use,
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
// Ident : $Id: RuleEngine.h $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/RuleEngine.h $
// 
// 3     10-12-17 12:02 Hui.shao
// added more tracking on rule execution
// 
// 2     10-12-16 14:45 Hui.shao
// added an reserved action ".ToNextAction"
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 9     10-04-02 13:57 Haoyuan.lu
// 
// 8     10-03-23 13:50 Haoyuan.lu
// 
// 7     10-03-23 12:56 Haoyuan.lu
// 
// 6     10-02-25 17:35 Haoyuan.lu
// 
// 5     10-02-23 16:41 Haoyuan.lu
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

#ifndef __ZQ_Common_RuleEngine_H__
#define __ZQ_Common_RuleEngine_H__

#include "ZQ_common_conf.h"
#include "Locks.h"
#include "Log.h"
#include "NativeThreadPool.h"
#include "DynSharedObj.h"

#include <map>

#include "expatxx.h"

#include <iostream>
#include <fstream>
//#define ENABLE_COMPRESS

#ifdef ENABLE_COMPRESS
#  include "Bz2Stream.h"
#endif // ENABLE_COMPRESS

#define SYS_PROP_PREFIX  "sys."
#define USER_PROP_PREFIX "user."
#define SYS_PROP(_V) SYS_PROP_PREFIX #_V
#define USER_PROP(_V) USER_PROP_PREFIX #_V

#define STL_MAPSET(_MAPTYPE, _MAP, _KEY, _VAL) if (_MAP.end() ==_MAP.find(_KEY)) _MAP.insert(_MAPTYPE::value_type(_KEY, _VAL)); else _MAP[_KEY] = _VAL

namespace ZQ {
namespace common {
		
class ZQ_COMMON_API Action;
class ZQ_COMMON_API RuleEngine;

// -----------------------------
// class Action
// -----------------------------
///@note any necessary lock must performed outside of state process
class Action
{
	friend class RuleEngine;

public:
	typedef ::std::map < ::std::string, ::std::string> Properties;

	typedef enum _StatusCode
	{
		aSucceed,
		aFailed,
		aSucceedQuit,
		aFailedQuit
	} StatusCode;

	typedef struct _Context
	{
		StatusCode statusCode;
		Properties metaData;
	} Context;

protected:

	Action(RuleEngine& engine, const char* name); // not allow to initialize this base Action directly

public:
	virtual ~Action();
	const char* name() const { return _name.c_str(); }

	// sync-ed call
//	virtual Action& operator() (IActionMgr::Context& ctx, const IActionMgr::Properties& inputMetaData, IActionMgr::Properties& outputMetaData);
	virtual Action& operator() (Context& ctx, const Properties& input, Properties& output);

protected:

	RuleEngine&    _engine;
	::std::string  _name;
};

/* XML based rule definition schema
<Rules>
   <Rule name="Rule1" >
       <Action name="CountSteam" >
	      <!-- stage 2 <Condition ctxMetaData="streamcount" evaluation="VALUE() > 100" /> all condition(s) true to execute the action-->
		  <Arg name="runmode" value="ddd"/ >
	      <ResultMetaData>
		     <result outputName="count" ctxMetaData="streamcount" />
		  </ResultMetaData>
	   </Action>
       <Action name="UpdateStreamCountToMetaData" />
   </Rule>
   <Event category="Stream" event="Created" >
       <Exec rule="Rule1" />
       <Exec rule="Rule2" />
   </Event>
   <Rule name="RuleX" >
         <Action name="Act001" >
			...
		 </Action>
	   <!-- to follow with a sub rule after some actions>
       <Exec rule="Rule1" />
   </Rule>
</Rules>
*/

// -----------------------------
// class RuleEngine
// -----------------------------
class RuleEngine
{
	friend class Action;
	friend class RuleExecCmd;
	friend class RuleExecCmdLocal;

public:

	RuleEngine(Log& log, NativeThreadPool& thpool, int depth); // , const ::std::string& defaultEventChannelEndpoint);
	virtual ~RuleEngine();

	typedef struct _RuleItem
	{
		::std::string actionName;
		bool          isASubRule;

		// map of context metadata to condition statement, multiple conditions are under and-calculation
		// only exec this action when ALL the conditions are true
		Action::Properties conditions;

		/// per-rule-item input arguments
		Action::Properties inputArgs; 

		/// after an operation is executed, this map guides the rule engine to copy the output params of the operation into
		/// the operation context's metadata as the input for the next operation(s) in the stack
		Action::Properties output2ctx; 

	} RuleItem;

	typedef ::std::vector < RuleItem > Rule;

public:	// methods to rule management
	
	virtual void subscribeEvents(const ::std::string& topic);

	///@note empty Rule indicates to remove the rule with given ruleName
	virtual bool registerRule(const ::std::string& ruleName, const Rule& Rule);
	///@note empty ruleNames indicates to remote the event-to-rules association
	virtual bool applyRulesToEvent(const ::std::string& category, const ::std::string& eventName, const ::std::vector< ::std::string> ruleNames);
	virtual const ::std::vector< ::std::string>& getRulesOfEvent(const ::std::string& category, const ::std::string& eventName);

public: // entry for events

	virtual void OnEvent(const std::string& category, const std::string& eventName, const std::string& stampISO8601, const std::string& sourceNetId, const Action::Properties& params, const std::string& userTxnId="");
	virtual void OnRuleExecuted(const std::string& ruleName, uint64 execId, const Action::Context& outputCtx, const std::string& userTxnId="") {}

	void maxDepth(const Rule& rule, int& currentDepth, int& currentMax, const ::std::string& ruleName, const Rule& registerRule);

	bool load(const char* filename);
	bool loadConfig(const char* filename, bool compressed=false);
	int populate(const char* path);

protected: // implementation of IActionMgr
	void getPathes(::std::string& configFolder, ::std::string& logFolder) const;
	virtual Log& ctxlog();

	virtual bool invoke(Action::Context& ctx, const ::std::string& actionName, const Action::Properties& input, Action::Properties& output, const long timeout =2000);
	///@note when alias is given, the rule engine takes alias to map the Action
	virtual bool registerAction(const ::std::string& actionName, Action& act, const ::std::string& alias="");
	virtual bool unregisterAction(const ::std::string& actionSearchName);

protected:

	typedef ::std::map < ::std::string, Rule > RuleMap; ///< a map from rule-name to rule
	typedef ::std::map < ::std::string, ::std::vector< ::std::string> > EventToRulesMap; ///< a map from "category$eventName" to rule stack map
	typedef ::std::map < ::std::string, Action* > ActionMap; ///< a map from rulename/alias to Action map
	
	EventToRulesMap    _eventToRules;
	Mutex              _lkEventToRules;

	RuleMap            _ruleMap;
	Mutex              _lkRuleMap;

	ActionMap          _actions;
	Mutex              _lkActions;

	NativeThreadPool&  _thpool;
	Log&               _log;
	long               _invokeTimeout;
	int				   _depth;

	typedef std::map< std::string, ZQ::common::DynSharedObj* > MoudleObjMap; ///< a map of link-type to StorageLinkHelper object
	MoudleObjMap  _ModuleObjMap;
	ZQ::common::Mutex   _lockModuleObjMap;
};

// In order to support async execution of an action in the rule, the rule engine reserved ".ToNextAction" registered by default
// It takes two input parameters:
//   @param ".ToNextAction.skipSize" - an integer to indicate how many action in the rule should be skipped, default 0.
//   @param ".ToNextAction.ActionName" - an string name of registered action to search for after skipping some actions, default ""
//                                       to take any action name followed
// @note the caller may refer to context[SYS_PROP(LastActionIdx)] to determine the value of ".ToNextAction.skipSize"
// -----------------------------
// class ToNextAction
// -----------------------------
///@note any necessary lock must performed outside of state process
//class ToNextAction : public Action
//{
//	ToNextAction(RuleEngine& engine, int skipSize=0, const char* lookforActionName=NULL)
//		: Action(engine, ".ToNextAction") {}
//};


// -----------------------------
// class QuitAction
// -----------------------------
///@note any necessary lock must performed outside of state process
class QuitAction : public Action
{
	QuitAction(RuleEngine& engine) : Action(engine, "QuitAction") {}

public:

	virtual Action& operator() (Context& ctx, const Properties& input, Properties& output)
	{
		ctx.statusCode = aSucceedQuit;
		return *this;
	}
};

}} // namespace

#endif // __ZQ_Common_RuleEngine_H__
