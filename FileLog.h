#ifndef __ZQ_COMMON_FileLog_H__
#define __ZQ_COMMON_FileLog_H__

#ifdef ZQ_FILELOG_V2
	#include "FileLogV2.h"
#else
	#include "FileLogV1.h"
#endif

#endif // #define __ZQ_COMMON_FileLog_H__