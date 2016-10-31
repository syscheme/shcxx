#ifndef __ZQRESOURCE_H__
#define __ZQRESOURCE_H__

#define ZQ_PRODUCT_NAME			"TianShan Architecture"

#define __N2S2__(x) #x
#define __N2S__(x) __N2S2__(x)
// the following section will be replaced with the real value by the ZQAutoBuild process
#define ZQ_PRODUCT_VER_MAJOR		2
#define ZQ_PRODUCT_VER_MINOR		9
#define ZQ_PRODUCT_VER_PATCH		0
#define ZQ_PRODUCT_VER_BUILD		1

// the following section are static per-project
#define ZQ_FILE_DESCRIPTION        	ZQ_PRODUCT_NAME ": libeloop"
#ifdef _DEBUG
#define ZQ_INTERNAL_FILE_NAME      	"libeloop_d"
#else
#define ZQ_INTERNAL_FILE_NAME      	"libeloop"
#endif // _DEBUG
#define ZQ_FILE_NAME               	ZQ_INTERNAL_FILE_NAME ".exe"

// the following section are static per-project, but you can define many SDK involved
#define ZQ_PRODUCT_COMMENT          	ZQ_FILE_DESCRIPTION " " ZQ_PRODUCT_VER_STR3

#endif // __ZQRESOURCE_H__
