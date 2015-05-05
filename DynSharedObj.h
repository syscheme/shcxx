// ===========================================================================
// Copyright (c) 2004 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// Ident : $Id: DynSharedObj.h,v 1.13 2004/08/09 10:06:56 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define a dynamic shared object / for the plugin architecture
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/DynSharedObj.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 11    09-12-29 18:34 Fei.huang
// + add interface to get error message
// 
// 10    09-04-29 14:50 Hongquan.zhang
// 
// 9     08-12-11 15:48 Yixin.tian
// 
// 8     08-03-06 16:05 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 7     08-03-03 16:43 Yixin.tian
// merged changes for linux
// 
// 6     10/08/06 3:39p Hui.shao
// 
// ===========================================================================

#ifndef	__ZQ_common_DynSharedObj_H__
#define	__ZQ_common_DynSharedObj_H__

#include "ZQ_common_conf.h"
#include <memory>
#include <stdio.h>
#include "Log.h"
#include <string>

#ifdef ZQ_OS_LINUX
extern "C" {
    #include <dlfcn.h>
}
#endif

namespace ZQ {
namespace common {

class ZQ_COMMON_API DynSharedObj;
class ZQ_COMMON_API DynSharedFacet;

// -----------------------------
// class DynSharedObj
// -----------------------------
/// Dynamic shared object, wrapper of DLL on windows or SO on unix
///@sa the example of DynSharedFacet 
class DynSharedObj
{
	friend class DynSharedFacet;

public:

	typedef struct _ImageInfo {
		char	  filename[MAX_PATH];
		char	  verinfo[MAX_PATH];
		char	  vendor[MAX_PATH];
		char	  filedesc[MAX_PATH];
		uint32	  dwVerHi, dwVerLo;

	} ImageInfo;


public:

	/// constructor
	///@param[in] filename the filename of the external dll or so
	DynSharedObj(const char* filename=NULL);

	/// destructor
	virtual ~DynSharedObj();

	/// test if the external image has been loaded successfully
	///@return true if loaded
	virtual bool isLoaded() const;

	/// load a specific external image
	///@param[in] filename the filename of the external dll or so
	///@return true if succeed
	virtual bool load(const char* filename);

	/// unload the external image
	virtual void free();

	/// query the filename of the external image
	///@return a NULL-terminated filename string
	const ImageInfo* getImageInfo() const;

#ifdef ZQ_OS_MSWIN
	/// get the Windows handle of the loaded library
	///@return the Windows handle of the loaded library
	const HINSTANCE getLib() const { return _hLib; }
#else 
	void* getLib() const {return _hLib; }
#endif 

    /// get the error message of last failure
    ///@return the error message
    const char* getErrMsg() const { return _lastErr.c_str(); }

protected:

#ifdef ZQ_OS_MSWIN
	HINSTANCE _hLib;
#endif
#ifdef ZQ_OS_LINUX
    void* _hLib;
#endif
	ImageInfo _imageInfo;

    std::string _lastErr;
};

// -----------------------------
// class DynSharedFacet
// -----------------------------
/// The definition of a base interface exported from dynamic shared object
class DynSharedFacet
{

public:

	/// constructor
	///@param[in] dso the DynSharedObj instance that this facet is exported from
	DynSharedFacet(DynSharedObj& dso,ZQ::common::Log* logger = NULL );

	/// destructor
	virtual ~DynSharedFacet();
	
	/// get the instance of DynSharedObj of this facet
	///@return the instance of DynSharedObj of this facet
	DynSharedObj& getDynSharedObj() const { return _dso; }

	/// test if this facet has been succesfully exported
	///@return true if succeed
	virtual bool isValid();

protected:

//	/// entry routine to map the exported functions from DynSharedFacet
//	bool mapFunc(DynSharedObj& dso);

	DynSharedObj&	_dso;
	bool			_bMapped;

	/// the major routine to map external procedues
	virtual bool		_mapExtProc(bool cleanup =false) { return true; }

protected:

	ZQ::common::Log*		mLogger;

};

#define DECLARE_DSOFACET(myclass, superclass) \
   public: \
   myclass(::ZQ::common::DynSharedObj& dso) : superclass(dso) { if (_dso.isLoaded()) _mapExtProc(); } \
   virtual ~myclass() {}

#define DECLARE_PROC(_RESULT_TYPE, _PROC_NAME, _ARGS) \
    protected: typedef _RESULT_TYPE (*_PROC_NAME##_ProcType) _ARGS; \
    public: \
        _PROC_NAME##_ProcType _PROC_NAME;

#define DECLARE_PROC_WITH_APITYPE(_RESULT_TYPE, _API_TYPE, _PROC_NAME, _ARGS) \
    protected: \
        typedef _RESULT_TYPE (_API_TYPE *_PROC_NAME##_ProcType) _ARGS; \
    public: \
        _PROC_NAME##_ProcType _PROC_NAME;

#define DSOFACET_PROC_BEGIN() \
	protected: \
		virtual bool _mapExtProc(bool cleanup=false) { \
			bool succ = _dso.isLoaded(); if (!succ) return false;

#define DSOFACET_PROC(_PROC_NAME) \
    DSOFACET_PROC_SPECIAL(_PROC_NAME, #_PROC_NAME)

#ifdef ZQ_OS_MSWIN

#define DSOFACET_PROC_SPECIAL(_PROC_NAME, _EXT_PROC_NAME) \
    _PROC_NAME = cleanup ? NULL : (_PROC_NAME##_ProcType) GetProcAddress(_dso.getLib(), _EXT_PROC_NAME); \
	if(!cleanup && _PROC_NAME == NULL) \
	{ if(mLogger) (*mLogger)(ZQ::common::Log::L_ERROR, CLOGFMT(DynSharedFacet, "failed to map \"[%s]%s\" as %s"), _dso.getImageInfo()->filename, _EXT_PROC_NAME, #_PROC_NAME); \
	  succ = false; }
#else

#define DSOFACET_PROC_SPECIAL(_PROC_NAME, _EXT_PROC_NAME) \
    _PROC_NAME = cleanup ? NULL : (_PROC_NAME##_ProcType) dlsym(_dso.getLib(), _EXT_PROC_NAME); \
	if(!cleanup && _PROC_NAME == NULL) \
	{ if(mLogger) (*mLogger)(ZQ::common::Log::L_ERROR, CLOGFMT(DynSharedFacet, "failed to map \"[%s]%s\" as %s"), _dso.getImageInfo()->filename, _EXT_PROC_NAME, #_PROC_NAME); \
	  succ = false; }

#endif

#define DSOFACET_PROC_END()   _bMapped = succ; return succ; }

/*! @class DynSharedFacet
@section HelloFacet_Example Example
Assume there is a dll named "c:\hello.dll" that exports several APIs: \n
@code
...
void __declspec(dllexport) hello(void);
void __declspec(dllexport) HELLO(int aaa, bool bbb);
void __declspec(dllexport) other(int ccc);
...
@endcode
where the API "hello" and "HELLO" are what you needed for a hello purpose, 
You will be able to invoke the dll like the following example:
@code
class HelloFacet : public DynSharedFacet
{
	// declare this Facet object as a child of DynSharedFacet
	DECLARE_DSOFACET(HelloFacet, DynSharedFacet);

	// declare the API protypes
	DECLARE_PROC(void, hello, (void));
	DECLARE_PROC(void, hello2, (int aaa, bool bbb));

	// map the external APIs
	DSOFACET_PROC_BEGIN();
		DSOFACET_PROC(hello);
		DSOFACET_PROC_SPECIAL(hello2, "HELLO");
	DSOFACET_PROC_END();
};

void main()
{
	DynSharedObject dso("c:\\hello.dll");
	HelloFacet	 hf(dso);

	// test if the required APIs have been mapped as expected
	if (!hf.isValid())
		return;

	// now start invoke the API if necessary
	hf.hello();
	hf.hello2(1, true);
}
@endcode
*/

} // namespace common
} // namespace ZQ

#endif // __ZQ_common_DynSharedObj_H__

