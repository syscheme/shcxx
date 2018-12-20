#include "RuleEngine.h"

using namespace ZQ::common;
class TestAction : public Action
{
public:
	TestAction(RuleEngine& engine, ZQ::common::Log& log) : Action(engine, "TestAction"), _log(log) {}

public:

	virtual Action& operator() (Context& ctx, const Properties& input, Properties& output)
	{
		ctx.statusCode = aSucceedQuit;
		printf("TestAction\n");
		_log(ZQ::common::Log::L_INFO, CLOGFMT(TestAction, "TestAction: %s"), "Test logging");
		return *this;
	}

private:
	ZQ::common::Log& _log;
};
