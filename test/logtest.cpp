#include "../Log.h"
#include "../ScLog.h"

using namespace ZQ::common;
void main()
{
	ZQ::common::ScLog sclog("sclog.log");
	for(int i=0;i<100;i++)
	{
		sclog(Log::L_CRIT,"%s:error code = %d, event loop = %s",__FUNCTION__,1,"faint");
	}
}
