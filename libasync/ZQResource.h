#ifndef __ZQRESOURCE_H__
#define __ZQRESOURCE_H__

#include "../../build/ZQSrcTreeVer.h" // defines the version number of the source tree

	/*#define ZQ_PRODUCT_NAME				"ZQ TianShan Dummy StreamSmith Service"


	#ifdef ZQ_PRODUCT_NAME
		#undef ZQ_PRODUCT_NAME
	#endif*/

	#define ZQ_PRODUCT_NAME			"SeaChange TianShan Architecture"

	#define ISA_TARGET_ENV          	VOD_VERSION_DESC "; ITV " ITV_VERSION

	// the following section are static per-project
	#define ZQ_FILE_DESCRIPTION        	":Async IO lib "
	#ifdef _DEBUG
		#define ZQ_INTERNAL_FILE_NAME      	"libasync_d"
	#else
		#define ZQ_INTERNAL_FILE_NAME      	"libasync"
	#endif // _DEBUG
	#define ZQ_FILE_NAME               	ZQ_INTERNAL_FILE_NAME ".dll"

	// the following section are static per-project, but you can define many SDK involved	



	#define __xN2S2__(x) #x
	#define __xN2S__(x) __xN2S2__(x)

	#define ZQ_PRODUCT_COMMENT          	ZQ_FILE_DESCRIPTION //" " ZQ_PRODUCT_VER_STR3

#endif // __ZQRESOURCE_H__
