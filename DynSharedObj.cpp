// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
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
// Ident : $Id: DynSharedObj.cpp,v 1.13 2004/08/09 10:06:56 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl a dynamic shared object / for the plugin architecture
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/DynSharedObj.cpp $
// 
// 2     4/10/14 9:40a Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 16    09-12-29 18:35 Fei.huang
// * don't throw exception in ctor, keep error message instead
// 
// 15    09-12-29 14:41 Fei.huang
// * use common interface to get error message
// 
// 14    09-12-29 11:31 Fei.huang
// + throw exception if library not loaded 
// 
// 13    09-06-04 13:42 Yixin.tian
// 
// 12    09-04-29 14:50 Hongquan.zhang
// 
// 11    08-12-11 15:48 Yixin.tian
// 
// 10    08-03-06 16:11 Hui.shao
// 
// 9     08-03-06 16:05 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 8     08-03-03 16:43 Yixin.tian
// merged changes for linux
// 
// 7     10/08/06 3:39p Hui.shao
// 
// 6     06-09-05 18:57 Hui.shao
// 
// 5     06-08-02 18:46 Hui.shao
// 
// 4     06-08-02 17:56 Hui.shao
// 
// 3     06-08-01 18:23 Hui.shao
// 
// 2     06-08-01 18:19 Hui.shao
// 
// 1     06-08-01 17:49 Hui.shao
// 
// ===========================================================================

#include "DynSharedObj.h"
#include "SystemUtils.h"

#ifdef ZQ_OS_MSWIN
#pragma comment (lib, "version") // for GetFileVersionInfo
#endif

namespace ZQ {
namespace common {

DynSharedObj::DynSharedObj(const char* filename):
_hLib(NULL) {

	memset(&_imageInfo, 0, sizeof(_imageInfo));
	if (!filename) {
        return;
    } 
    load(filename);
}

DynSharedObj::~DynSharedObj() {
	if(isLoaded()) 
		free();
}

bool DynSharedObj::load(const char* filename) { 
	if(isLoaded()) 
		return false;
	
#ifdef ZQ_OS_MSWIN
	_hLib = ::LoadLibrary(filename);
#endif

#ifdef ZQ_OS_LINUX
    _hLib = dlopen(filename, RTLD_LAZY);
#endif
	
	if (_hLib == NULL) {
        _lastErr =  SYS::getErrorMessage(SYS::RTLD);
		return false;
    }
    _lastErr.clear();
	
#ifdef ZQ_OS_MSWIN

	::GetModuleFileName(_hLib, _imageInfo.filename, sizeof(_imageInfo.filename)-2);

    DWORD dwHandle;
    DWORD cchver = ::GetFileVersionInfoSize(_imageInfo.filename, &dwHandle);

	VS_FIXEDFILEINFO filever;
	WIN32_FILE_ATTRIBUTE_DATA fileattrs;
	
    if (cchver > 0)
	{
		char* pver = new char[cchver];
		UINT uLen;
		void *pbuf;

		if (NULL != pver && ::GetFileVersionInfo(_imageInfo.filename, dwHandle, cchver, pver))
		{
			if (::VerQueryValue(pver,"\\",&pbuf,&uLen))
				memcpy(&filever, pbuf,sizeof(filever));
			
			char querykey[MAX_PATH];

			DWORD translation = 0;

			if (::VerQueryValue(pver,"\\VarFileInfo\\Translation", &pbuf,&uLen) && uLen >= sizeof(translation)) 
				memcpy(&translation, (void*) pbuf, sizeof(translation));

			sprintf(querykey, "\\StringFileInfo\\%04x%04x\\CompanyName", LOWORD(translation), HIWORD(translation));
			if (::VerQueryValue(pver, querykey, &pbuf, &uLen))
				strcpy(_imageInfo.vendor, (char*) pbuf);

			sprintf(querykey, "\\StringFileInfo\\%04x%04x\\FileDescription", LOWORD(translation), HIWORD(translation));
			if (::VerQueryValue(pver, querykey, &pbuf, &uLen))
				strcpy(_imageInfo.filedesc, (char*) pbuf);
		}

		if (NULL != pver)
			delete[] pver;
	}

	::GetFileAttributesEx(_imageInfo.filename, GetFileExInfoStandard, &fileattrs);

	//change filetime to systime.
	SYSTEMTIME systime;
	::FileTimeToSystemTime(&fileattrs.ftCreationTime, &systime);
	__int64 filesize=fileattrs.nFileSizeHigh;
	filesize <<=32;
	filesize += fileattrs.nFileSizeLow;
	sprintf(_imageInfo.verinfo, "ProdVer: %d.%d.%d.%d; FileVer: %d.%d.%d.%d; CreateTime: %04d%02d%02dT%02d%02d%02dZ; Length: %lld bytes",
			HIWORD(filever.dwProductVersionMS), LOWORD(filever.dwProductVersionMS), HIWORD(filever.dwProductVersionLS), LOWORD(filever.dwProductVersionLS),
			HIWORD(filever.dwFileVersionMS), LOWORD(filever.dwFileVersionMS), HIWORD(filever.dwFileVersionLS), LOWORD(filever.dwFileVersionLS),
			systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond,
			filesize);
	_imageInfo.dwVerHi = filever.dwProductVersionMS; _imageInfo.dwVerLo = filever.dwProductVersionLS;

#endif // ZQ_OS_MSWIN

#ifdef ZQ_OS_LINUX
//	#pragma message ( __MSGLOC__ "TODO: impl here for non-Win32")
    /* TODO */
	strcpy(_imageInfo.filename,filename);
//    int res = readlink("/proc/self/exe", _imageInfo.filename, sizeof(_imageInfo.filename)-2);
//	if (res < 0 || res > int(sizeof(_imageInfo.filename)-2))
//		strcpy(_imageInfo.filename, "DynSharedObj");
#endif

	return true;
}

void DynSharedObj::free()
{
	if(_hLib != NULL)
#ifdef ZQ_OS_MSWIN
		::FreeLibrary(_hLib);
#endif
#ifdef ZQ_OS_LINUX
        if(dlclose(_hLib)) {
        }
#endif
	_hLib = NULL;
}

bool DynSharedObj::isLoaded() const {
	return _hLib != NULL;
}

const DynSharedObj::ImageInfo* DynSharedObj::getImageInfo() const {
	return &_imageInfo;
}

DynSharedFacet::DynSharedFacet(DynSharedObj& dso , ZQ::common::Log* logger)
         : _dso(dso), _bMapped(false),mLogger(logger)
{
	if (!_dso.isLoaded())
		return;

	_mapExtProc();
}

DynSharedFacet::~DynSharedFacet()
{
	if (!isValid())
		return;

	_mapExtProc(true);
}

/*
bool DynSharedFacet::mapFunc(DynSharedObj& dso)
{
	if (isValid())
		return false;

	_dso=dso;

	if (!_dso.isLoaded())
		return false;

	if (_mapExtProc(false))
	{
		_bMapped = true;
		return true;
	}

	return false;
}
*/

bool DynSharedFacet::isValid()
{
	return (_bMapped && _dso.isLoaded());
}

} // namespace common
} // namespace ZQ
