// ============================================================================================
// Copyright (c) 1997, 1998 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of ZQ Interactive, Inc. Possession, use, duplication or dissemination of the
// software and media is authorized only pursuant to a valid written license from ZQ Interactive,
// Inc.
// This source was copied from shcxx, shcxx's copyright is belong to Hui Shao
//
// This software is furnished under a  license  and  may  be used and copied only in accordance
// with the terms of  such license and with the inclusion of the above copyright notice.  This
// software or any other copies thereof may not be provided or otherwise made available to any
// other person.  No title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and should not be
// construed as a commitment by ZQ Interactive, Inc.
// --------------------------------------------------------------------------------------------
// Author: Hui Shao
// Desc  : define URL string
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/urlstr.h 2     1/16/15 4:33p Zhiqiang.niu $
// $Log: /ZQProjs/Common/urlstr.h $
// 
// 2     1/16/15 4:33p Zhiqiang.niu
// add api getPathAndParam(), return path and param ,such as "
// /path/file?key1=val1&key2=val2#fragment "
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 7     08-01-15 11:15 Li.huang
// Add parser ftp protocol username and password
// 
// 6     07-11-12 10:55 Guan.han
// add function getEnumVars() to gain all the keys and values in the url
// 
// 5     07-07-09 17:04 Yixin.tian
// 
// 4     07-07-09 17:03 Yixin.tian
// 
// 3     07-03-06 12:05 Hongquan.zhang
// 
// 5     4/14/05 10:10a Hui.shao
// 
// 4     4/13/05 6:33p Hui.shao
// changed namespace
// 
// 3     4/12/05 5:18p Hui.shao
// ============================================================================================

#ifndef _URLStr_H_
#define _URLStr_H_

#include "ZQ_common_conf.h"

#include <string>
#include <map>

namespace ZQ {
namespace common {

class ZQ_COMMON_API URLStr;

// -----------------------------
// class URLStr
// -----------------------------
class URLStr
{
  public:
		  URLStr(const char* urlstr=NULL, bool casesensitive=false);
		  ~URLStr();
	  
		  bool parse(const char* urlstr);
	  
		  const char* getProtocol();
		  const char* getHost();
		  const char* getPath();
          const char* getPathAndParam();
		  int		  getPort();
          const char* getUserName();
		  const char* getPwd();

		  void  setProtocol(const char* value);
		  void  setHost(const char* value);
		  void  setPath(const char* value);
		  void	setPort(const int value);
		  void  setUserName(const char* value);
		  void  setPwd(const char* value);

		  const char* getVarname(int idx=0);
		  const char* getVar(const char* var);
		  void  setVar(const char* var, const char* value);
	
		  const char* generate();

		  // to gain all the keys and values in the url.
		  std::map<std::string, std::string> getEnumVars() const;

		  void  clear();

  public:
			static bool encode(const void* source, char* target, int outlen, int len=-1);
			static bool decode(const char* source, void* target, int maxlen=-1);

  protected:
	  		  typedef std::map<std::string, std::string> urlvar_t;
	  		  urlvar_t mVars;
  private:

	  std::string mProtocol, mHost, mPath, mPathAndParam, mContent,mUserName,mPwd;
		  int	mPort;
		  bool bCase;
		  std::string output_str;
}; 

} // namespace common
} // namespace ZQ

#endif // _URLStr_H_

