// ============================================================================================
// Copyright (c) 1997, 1998 by
// syscheme, Shanghai,,
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of syscheme Possession, use, duplication or dissemination of the
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
// construed as a commitment by syscheme
// --------------------------------------------------------------------------------------------
// Author: Hui Shao
// Desc  : impl URL string
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/urlstr.cpp 9     10/22/15 12:27p Hui.shao $
// $Log: /ZQProjs/Common/urlstr.cpp $
// 
// 9     10/22/15 12:27p Hui.shao
// convert space to %20 instead of +
// 
// 8     3/31/15 5:16p Li.huang
// rollback
// 
// 6     1/16/15 4:33p Zhiqiang.niu
// add api getPathAndParam(), return path and param ,such as "
// /path/file?key1=val1&key2=val2#fragment "
// 
// 5     4/10/13 11:15a Li.huang
// 
// 4     4/03/13 9:42a Hui.shao
// 
// 3     1/15/13 3:41p Li.huang
// add http username and password
// 
// 2     6/07/12 5:51p Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 15    08-12-09 14:35 Yixin.tian
// 
// 14    08-07-14 19:26 Xiaohui.chai
// Increased the internal buffer size to 512 Bytes in the generate()
// 
// 13    08-03-03 18:18 Yixin.tian
// merged changes for linux
// 
// 12    08-02-19 15:57 Hui.shao
// 
// 11    08-01-15 11:15 Li.huang
// Add parser ftp protocol username and password
// 
// 10    07-11-12 10:55 Guan.han
// add function getEnumVars() to gain all the keys and values in the url
// 
// 9     07-09-07 13:52 Xiaohui.chai
// fixed missing separator during generate
// 
// 8     07-06-07 16:31 Hongquan.zhang
// 
// 7     07-03-06 12:05 Hongquan.zhang
// 
// 6     06-10-16 15:23 Yonghua.deng
// 
// 5     06-10-10 13:58 Hongquan.zhang
// 
// 4     10/08/06 11:35a Hui.shao
// 
// 3     4/29/06 8:25p Hui.shao
// 
// 2     06-03-29 15:55 Bernie.zhao
// fixed std::map exception during destructor
// 
// 1     2/20/06 5:59p Hui.shao
// 
// 3     4/13/05 6:33p Hui.shao
// changed namespace
// 
// 2     4/12/05 5:18p Hui.shao
// ============================================================================================

#include "urlstr.h"

#include <algorithm>
#include <locale>

namespace ZQ {
namespace common {

#define ISDIGIT(_C) (_C>='0' && _C<='9')
#define ISALPHA(_C) ((_C>='A' && _C<='Z') || (_C>='a' && _C<='z'))

bool URLStr::encode(const void* source, char* target, int outlen, int len/*=-1*/)
{
	if (source==NULL || outlen<=0)
		return false;

	if (len<0)
		len = strlen((char*) source);

	const int8 *sptr=(const int8 *)source;

	std::string ret;

	for(int i=0; i<len; i++)
	{
		// The ASCII characters digits or letters, and ".", "-", "*", "_"
		// remain the same
		if (ISDIGIT(sptr[i]) || ISALPHA(sptr[i])
			|| sptr[i]=='.' || sptr[i]=='-' ||	sptr[i]=='*' || sptr[i]=='_' )
		{
			ret += (char) sptr[i];
			continue;
		}

		// The space character ' ' is converted into a plus	sign '+'
		if (sptr[i]==' ')
		{
			ret += "%20"; // ret += '+';
			continue;
		}

		//All other characters are converted into the 3-character string "%xy",
		// where xy is the two-digit hexadecimal representation of the lower
		// 8-bits of the character
		unsigned int hi, lo;
		hi= ((unsigned int)sptr[i] & 0xf0) / 0x10;
		lo= (unsigned int) sptr[i] % 0x10;

		hi+=(hi<10)? '0' : ('a' -10);
		lo+=(lo<10)? '0' : ('a' -10);

		ret += '%';
		ret += (char) (hi &0xff);
		ret += (char) (lo &0xff);
	}

	if((int)ret.size() < outlen)
	{
		strncpy(target, ret.c_str(), outlen);
		return true;
	}

	return false;
}

bool URLStr::decode(const char* source, void* target, int maxlen)
{
	int slen=strlen(source);
	uint8 *targ = (uint8 *)target;

	if (targ ==NULL)
		return false;

	int s, t;
	for(s=0, t=0; s<slen && (t<maxlen || maxlen<0); s++, t++)
	{
		// a plus sign '+' should be convert back to space ' '
		if (source[s]=='+')
		{
			targ[t]=' ';
			continue;
		}

		// the 3-character string "%xy", where xy is the
		// two-digit hexadecimal representation should be char

		if (source[s]=='%')
		{
			unsigned int hi, lo;

			hi=(unsigned int) source[++s];
			lo=(unsigned int) source[++s];

			hi -= ( isxdigit(hi)  ? ( isalpha(hi) ? (isupper(hi)?('A' -10):('a'-10)) : '0' ) : '0' );
			lo -= ( isxdigit(lo)  ? ( isalpha(lo) ? (isupper(lo)?('A' -10):('a'-10)) : '0' ) : '0' );

			//			hi -=(isxdigit(hi) ? ('a' -10) : '0' ) ;
			//			lo -=(isxdigit(lo) ? ('a' -10) : '0' ) ;

			if ((hi & 0xf0)|| (lo &0xf0))
				return false;

			targ[t]=(hi*0x10 +lo) &0xff;
			continue;
		}

		// The ASCII characters 'a' through 'z', 'A' through
		// 'Z', '0' through '9', and ".", "-", "*", "_" remain the same
		targ[t]= source[s];
	}

	if (t<maxlen || maxlen<0)
		targ[t]=0x00;

	return true;
}

URLStr::URLStr(const char* urlstr, bool caseSensitiveVars)
: _bCaseSensitiveVars(caseSensitiveVars), mPort(0)
{
	parse(urlstr);
}

URLStr::~URLStr()
{
	mVars.clear();
}

#define TOLOWER(_S) \
	std::transform(_S.begin(), _S.end(), _S.begin(), (int(*)(int)) tolower)

// https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers
static struct {
	const char* prot; int port;
} 
KNOWN_PORTS[] = {
	{"http",        80 },
	{"ftp",         21 },
	{"rtsp",        554 },
	{"ssh",         22 },
	{"telnet",      23 },
	{"smtp",        25 },
	{"dns",         53 },
	{"gopher",      34},
	{"tftp",        69},
	{"rtelnet",     107},
	{"oncrpc",      111},
	{"sftp",        115},
	{"ntp",         123},
	{"snmp",        161},
	{"snmptrap",    162},
	{"ldap",        389},
	{"https",       443 },
	{"samba",       445 }, // also for active directory
	{"kerberos",    464 },
	{"modbus",      502 },
	{"syslog",      514 },
	{"uucp",        540 },
	{"ftps",        990 }, // the control connection, data will be on 989
	{"docker",      2375 }, // docker REST API
	{"dockers",     2376 }, // docker REST API(SSL)
	{"redis",       6379 },
	{NULL, 0}
};

bool URLStr::parse(const char* urlstr)
{
	if (urlstr==NULL)
		return false;

	std::string wkurl = urlstr;
	if (wkurl.empty())
		return true;

	int qpos=wkurl.find('?');
	int cpos=wkurl.find(':');
	int spos=wkurl.find('/');
	//	int epos=wkurl.find('=');
	int upos=wkurl.find('@');
	std::string surl, searchurl;
	if (cpos>0 && cpos <spos)
	{
		// has protocol and server
		mProtocol = wkurl.substr(0, cpos);
		surl = wkurl.substr(cpos+1, qpos-cpos-1);
	}
	else searchurl = wkurl;

	//for path and param, such as /path/file?key1=val1&key2=val2#fragment
	int startPathPos = wkurl.find_first_of('/', cpos + 3);
	if (startPathPos != wkurl.npos)
		mPathAndParam = wkurl.substr(startPathPos);

	if (qpos>=0)
		searchurl = wkurl.substr(qpos+1);

	if (upos > 0 && !surl.empty())
	{
		int pos = surl.find_first_of(":");

		upos = surl.find_first_of("@");

		if(pos > 0)
		{
			mUserName = surl.substr(2,pos - 2);

			if(pos +1 < upos)
				mPwd = surl.substr(pos +1, upos - pos -1);
			else
				mPwd = "";
		}
		else
		{
			mUserName = surl.substr(2,upos - 2);
			mPwd = "";
		}

		surl = surl.substr(upos +1);
	}

	if (!surl.empty())
	{
		int pos = surl.find_first_not_of("/");

		if (3 ==pos)
		{
			// met a triple slash, for example file:/// means localhost
			mHost = "localhost";
			mPath = surl.substr(pos);
		}
		else
		{
			surl = (pos>=0) ? surl.substr(pos): surl;

			pos = surl.find_first_of("/");
			if (pos>0)
			{
				mHost = surl.substr(0, pos);
				mPath = surl.substr(pos+1);
			}
			else
			{
				mHost = surl;
				mPath = "";
			}
		}

		pos = mHost.find_last_of(":");
		if (pos>0)
		{
			mPort = atoi(mHost.substr(pos+1).c_str());
			mHost = mHost.substr(0, pos);
		}
	}

	TOLOWER(mHost);

	mContent=searchurl;
	searchurl +="&";
	for (int pos = searchurl.find("&");
		pos>=0 && (size_t) pos <searchurl.length();
		searchurl = searchurl.substr(pos+1), pos = searchurl.find("&"))
	{
		std::string wkexpress = searchurl.substr(0, pos);
		if (wkexpress.empty())
			continue;

		int qpos=wkexpress.find("=");
		std::string var, val;

		var = (qpos>0) ? wkexpress.substr(0, qpos):wkexpress;
		val = (qpos>0) ? wkexpress.substr(qpos+1) : "";

		char* buf = new char[wkexpress.length()+2];
		decode(var.c_str(), buf);
		var = buf;

		decode(val.c_str(), buf);
		val = buf;

		//		ZeroMemory(buf, wkexpress.length()+2);
		memset(buf, 0,wkexpress.length()+2);
		delete [] buf;
		buf = NULL;

		if (!_bCaseSensitiveVars)
			TOLOWER(var);

		mVars.insert(urlvar_t::value_type(var, val));
	}

	if (mPort <=0) // fill the default port number of known protocols
	{
		std::string tmpProt = mProtocol;
		TOLOWER(tmpProt);

		for (int i=0; KNOWN_PORTS[i].prot; i++)
		{
			if (tmpProt != KNOWN_PORTS[i].prot)
				continue;

			mPort =KNOWN_PORTS[i].port;
			break;
		}
	}

	return true;
}

const char* URLStr::getVarname(int idx)
{
	int j =idx;
	urlvar_t::iterator i;
	for ( i = mVars.begin(); i!= mVars.end() && j>0; i++, j--)
		;
	return (i== mVars.end()) ? NULL : i->first.c_str();
}

std::map<std::string, std::string> URLStr::getEnumVars() const
{
	return mVars;
}

const char* URLStr::getVar(const char* var)
{
	if(var==NULL)
		return mContent.c_str();

	if (mVars.find(var) == mVars.end())
		return "";

	return mVars[var].c_str();
}

void URLStr::setVar(const char* var, const char* value)
{
	if (var==NULL || *var==0x00)
		return;
	mVars[var]=(value==NULL)?"" : value;
}

void URLStr::setProtocol(const char* value)
{
	if (value==NULL)
		return;

	mProtocol=value;
}

void URLStr::setHost(const char* value)
{
	if (value==NULL)
		return;
	mHost=value;
}

void URLStr::setPort(const int value)
{
	if(value==0)
		return;
	mPort=value;
}

void URLStr::setPath(const char* value)
{
	if (value==NULL)
		return;
	mPath=value;
}

void  URLStr::setUserName(const char* value)
{
	if (value == NULL)
		return;
	mUserName = value;
}

void  URLStr::setPwd(const char* value)
{
	if (value == NULL)
		return;
	mPwd = value;	
}
const char* URLStr::generate()
{
	char portBuff[16];
	itoa(mPort, portBuff, 10);
	if(!mUserName.empty() && (mProtocol == "ftp" || mProtocol == "http" || mProtocol == "https" || mProtocol == "cifs" || mProtocol == "file"))
	{
		if(!mPwd.empty())
			output_str = mProtocol + "://" +mUserName + ":" + mPwd + "@" + mHost + ":" + portBuff +"/" + mPath;
		else
			output_str = mProtocol + "://" +mUserName + "@" + mHost + ":" + portBuff +"/" + mPath;		
	}
	else
		output_str = mProtocol + "://" +mHost + ":" + portBuff +"/" + mPath;

	if (mVars.size()>0)
		output_str += "?";

#define VAR_BUF_SIZE 512
	char varBuff[VAR_BUF_SIZE];
	for (urlvar_t::iterator i = mVars.begin(); i!= mVars.end(); i++)
	{
		//added by xiaohui.chai >>
		if(i != mVars.begin())
			output_str += "&";
		//added by xiaohui.chai ||
		memset(varBuff,0,VAR_BUF_SIZE);
		if(!encode((void*)i->first.c_str(), varBuff, VAR_BUF_SIZE))
			return NULL;

		output_str += varBuff;

		output_str += "=";

		memset(varBuff,0,VAR_BUF_SIZE);
		if(!encode((void*)i->second.c_str(), varBuff, VAR_BUF_SIZE))
			return NULL;

		output_str += varBuff;
	}

	return output_str.c_str();
}

void URLStr::clear()
{
	mProtocol=mHost=mPath="";
	mVars.clear();
}

const char* URLStr::getProtocol()
{
	return mProtocol.c_str();
}

const char* URLStr::getHost()
{
	return mHost.c_str();
}

const char* URLStr::getPath()
{
	return mPath.c_str();
}

const char* URLStr::getPathAndParam()
{
	return mPathAndParam.c_str();
}

int URLStr::getPort()
{
	return mPort;
}

const char* URLStr::getUserName()
{
	return mUserName.c_str();
}

const char* URLStr::getPwd()
{
	return mPwd.c_str();
}

}} // namespaces
