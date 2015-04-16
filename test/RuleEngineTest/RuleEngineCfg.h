


#include "RuleEngine.h"


/*
struct ActionCfg{
	std::string name;
	std::string outputName;
	std::string ctxMetaData;
	static void structure(ZQ::common::Config::Holder< ActionCfg > &holder)
	{
		holder.addDetail("", "name", &ActionCfg::name, NULL);
		holder.addDetail("ResultMetaData/result", "outputName", &ActionCfg::outputName, NULL);
		holder.addDetail("ResultMetaData/result", "ctxMetaData", &ActionCfg::ctxMetaData, NULL);
	}
};

struct RuleCfg{
	std::string name;
	typedef std::map<std::string, ZQ::common::Config::Holder< ActionCfg > > ActionsCfg;
	ActionsCfg Actions;
	static void structure(ZQ::common::Config::Holder< RuleCfg > &holder)
	{
		holder.addDetail("", "name", &RuleCfg::name, NULL);
		holder.addDetail("Action", &RuleCfg::readAction, &RuleCfg::registerAction);
	}

	void readAction(ZQ::common::XMLUtil::XmlNode node, const ZQ::common::Preprocessor* hPP)
	{
		ZQ::common::Config::Holder< ActionCfg > ActionHolder("name");
		ActionHolder.read(node, hPP);
		Actions[ActionHolder.name] = ActionHolder;
	}
	void registerAction(const std::string &full_path)
	{
	}	
};

struct ExecCfg{
	std::string rule;
	static void structure(ZQ::common::Config::Holder< ExecCfg > &holder)
	{
		holder.addDetail("", "rule", &ExecCfg::rule, NULL);
	}
};

struct EventCfg{
	std::string category;
	std::string event;
	typedef std::map<std::string, ZQ::common::Config::Holder< ExecCfg > > ExecCfgs;
	ExecCfgs Execs;
	static void structure(ZQ::common::Config::Holder< EventCfg > &holder)
	{
		holder.addDetail("", "category", &EventCfg::category, NULL);
		holder.addDetail("", "event", &EventCfg::event, NULL);
		holder.addDetail("Exec", &EventCfg::readExec, &EventCfg::registerExec);
	}

	void readExec(ZQ::common::XMLUtil::XmlNode node, const ZQ::common::Preprocessor* hPP)
	{
		ZQ::common::Config::Holder< ExecCfg > ExecHolder("name");
		ExecHolder.read(node, hPP);
		Execs[ExecHolder.rule] = ExecHolder;
	}
	void registerExec(const std::string &full_path)
	{
	}	
};



struct RuleEngineCfg{

	typedef std::map<std::string, ZQ::common::Config::Holder< RuleCfg > > Rules;
	Rules rules;
	typedef std::map<std::string, ZQ::common::Config::Holder< EventCfg > > EventCfgs;
	EventCfgs Events;
	static void structure(ZQ::common::Config::Holder< RuleEngineCfg > &holder)
	{
		holder.addDetail("Rule", &RuleEngineCfg::readRule, &RuleEngineCfg::registerRule);	
		holder.addDetail("Event", &RuleEngineCfg::readEvent, &RuleEngineCfg::registerEvent);
	}

	void readRule(ZQ::common::XMLUtil::XmlNode node, const ZQ::common::Preprocessor* hPP)
	{
		ZQ::common::Config::Holder< RuleCfg > RuleHolder("name");
		RuleHolder.read(node, hPP);
		rules[RuleHolder.name] = RuleHolder;
	}
	void registerRule(const std::string &full_path)
	{
	}	

	void readEvent(ZQ::common::XMLUtil::XmlNode node, const ZQ::common::Preprocessor* hPP)
	{
		ZQ::common::Config::Holder< EventCfg > EventHolder("name");
		EventHolder.read(node, hPP);
		Events[EventHolder.category+EventHolder.event] = EventHolder;
	}
	void registerEvent(const std::string &full_path)
	{
	}	
};
*/
