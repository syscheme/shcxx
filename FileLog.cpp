#include "ZQ_common_conf.h"

#ifdef ZQ_FILELOG_V2
	#include "FileLogV2.cpp"
#else
	#include "FileLogV1.cpp"
#endif