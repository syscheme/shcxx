#ifndef _RULEENGINE_MODULE_H__
#define _RULEENGINE_MODULE_H__
#include "ZQ_common_conf.h"
#include "RuleEngine.h"
#include "DynSharedObj.h"
namespace ZQ {
	namespace common{

		#define Module_PREFIX "Action_"

		class ActionFacet : public DynSharedFacet
		{
			// declare this Facet object as a child of DynSharedFacet
			DECLARE_DSOFACET(ActionFacet, DynSharedFacet);

			// declare the API prototypes
			DECLARE_PROC(bool, Initialize, (RuleEngine* engine, ZQ::common::Log* log));
			DECLARE_PROC(bool, UnInitialize, (void));

			// map the external APIs
			DSOFACET_PROC_BEGIN();
			DSOFACET_PROC(Initialize);
			DSOFACET_PROC(UnInitialize);
			DSOFACET_PROC_END();
		};
	}
}//namespace
#endif//_RULEENGINE_MODULE_H__

