// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: InetAddr.cpp,v 1.22 2004/07/27 04:14:49 bzhao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl Inet address
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/InetAddr.cpp $
// 
// 2     3/11/16 9:54a Dejian.fei
// NDK android : cannot find enterMutex/leaveMutex,maybe 
//  is wrong 
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 9     08-03-06 16:17 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 8     08-03-03 17:10 Yixin.tian
// merged changes for linux
// 
// 7     04-11-18 15:36 Hui.shao
// added MaskAddress constructor to accept leading '1' bits
// 
// 6     04-10-10 18:06 Hui.shao
// added anyaddress checkig
// 
// 5     04-10-09 18:10 Hui.shao
// added scopeid for addr6
// 
// 4     04-09-15 13:47 Hui.shao
// fixed memcpy() destination address in operator=(long)
// 
// 3     04-08-20 11:00 Hui.shao
// Revision 1.22  2004/07/27 04:14:49  bzhao
// deleted dummy ipv4 getlocaladdr part
//
// Revision 1.21  2004/07/27 02:42:23  bzhao
// modified getLocalAddr to compatible with ipV4 when ipV6 enabled
//
// Revision 1.20  2004/07/26 11:04:10  bzhao
// dropped old getLocalAdress() ipv6 part
//
// Revision 1.19  2004/07/26 10:56:35  shao
// adj GETADAPTERSADDRESSES pre-comp definition
//
// Revision 1.18  2004/07/23 07:46:36  bzhao
// corrected some spell error during CVS merge
//
// Revision 1.17  2004/07/23 07:12:08  bzhao
// added GETADAPTERSADDRESSES flag
// and modeified getLocalAddress() for WINXP and WIN2003 to get local IPv6 addresses
// modified getHostAddress() to compatible with IPv6 string output
//
// Revision 1.16  2004/07/20 08:18:06  shao
// index for getHostAddress()
//
// Revision 1.15  2004/07/20 07:31:29  shao
// added InetAddress::operator==(const in/6_addr &a)
//
// Revision 1.13  2004/07/16 07:50:40  bzhao
// Added method to get local host address 'getLocalAddress()'
//
// Revision 1.12  2004/07/07 06:51:26  shao
// added getHostAddress() to return numeric IP address string without query dns
//
// Revision 1.11  2004/06/25 06:51:38  shao
// fixed address compare bug
//
// Revision 1.10  2004/06/24 05:10:15  shao
// setAddress() to public
//
// Revision 1.9  2004/06/22 09:00:08  shao
// InetMcastAddrValidator for IPv6
//
// Revision 1.8  2004/06/18 06:49:25  shao
// supported IPv6
//
// Revision 1.7  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.6  2004/05/09 03:54:31  shao
// no message
//
// Revision 1.5  2004/04/28 06:25:02  shao
// doxy comment format
//
// Revision 1.4  2004/04/27 08:52:12  shao
// no message
//
// Revision 1.3  2004/04/27 02:28:06  shao
// addjusted file header format
//
// Revision 1.2  2004/04/21 04:24:19  shao
// winsock2
//
// ===========================================================================

#include "InetAddr.h"
#include <vector>
#include "Locks.h"

#ifdef ZQ_OS_MSWIN 
#pragma comment(lib, "Ws2_32.lib") // for WSAIoctl

#ifdef IPV6_ENABLED
#	include <Iphlpapi.h>  // for GetAdaptersAddresses
#   pragma comment (lib, "Iphlpapi.lib")
#endif
#else
extern "C" {
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
}
#endif

namespace ZQ {
namespace common {



//InetLocalHostAddress tmpAddress;
// InetLocalHostAddress gLocalHostAddr;
/// -----------------------------
/// class InetMcastAddrValidator
/// -----------------------------
const InetMcastAddrValidator InetMcastAddress::_validator;

void InetMcastAddrValidator::operator()(const in_addr address) const
{
	// "0.0.0.0" is always accepted, as it is an "empty" address.
	if ((address.s_addr != INADDR_ANY) && 
		 (address.s_addr & MCAST_VALID_MASK) != MCAST_VALID_VALUE)
	{
		throw Exception("Multicast address not in the valid range: from 224.0.0.1 through 239.255.255.255");
	}
}

#ifdef IPV6_ENABLED
void InetMcastAddrValidator::operator()(const in6_addr address) const
{
#ifdef ZQ_OS_MSWIN
	if (address.u.Byte[0] !=0xff)
	{
		int i=1;
		for(; i<sizeof(address.u.Byte); i++)
			if (address.u.Byte[i] !=0)
				throw Exception("Multicast IPv6 address not in the valid range: ff00::1 ~");
	}
#else
	if (address.s6_addr[0] !=0xff)
	{
		int i=1;
		for(; i<int(sizeof(address.s6_addr)); i++)
			if (address.s6_addr[i] !=0)
				throw Exception("Multicast IPv6 address not in the valid range: ff00::1 ~");
	}
#endif
}
#endif // IPV6_ENABLED

/// -----------------------------
/// class InetAddress
/// -----------------------------
#ifndef ZQ_OS_MSWIN
Mutex mutex;
#endif

InetAddress::InetAddress(const InetAddrValidator *_validator) 
	        :_validator(_validator), _ipaddr(NULL), _addr_count(0)
{
	*this = (long unsigned int) INADDR_ANY;
}

InetAddress::InetAddress(const char *address, const InetAddrValidator *_validator)
            :_validator(_validator), _ipaddr(NULL), _addr_count(0)
{
	if (this->_validator)
		this->_validator = _validator;

	if(address == 0 || !strcmp(address, "*"))
		setAddress(NULL);
	else
		setAddress(address);
}

InetAddress::InetAddress(struct in_addr addr, const InetAddrValidator *_validator)
            :_validator(_validator), _ipaddr(NULL)
{
	if (this->_validator)
	{
		this->_validator = _validator;
		(*_validator)(addr);
	}

	_addr_count = 1;
	_ipaddr = new inetaddr_t[1];
	_ipaddr[0].family = PF_INET;
	_ipaddr[0].addr.a = addr;
}

#ifdef IPV6_ENABLED
InetAddress::InetAddress(struct in6_addr addr, const InetAddrValidator *_validator)
            :_validator(_validator), _ipaddr(NULL)
{
	if (this->_validator)
	{
		this->_validator = _validator;
		(*_validator)(addr);
	}

	_addr_count = 1;
	_ipaddr = new inetaddr_t[1];
	_ipaddr[0].family = PF_INET6;
	_ipaddr[0].addr.a6 = addr;
	_ipaddr[0].scopeid = 1; //??
}
#endif // IPV6_ENABLED

InetAddress::InetAddress(const InetAddress &rhs)
            :_addr_count(rhs._addr_count)
{
	_ipaddr = new inetaddr_t [_addr_count];

	memcpy(_ipaddr, rhs._ipaddr, sizeof(inetaddr_t) * _addr_count);
	_validator = rhs._validator;
}

InetAddress::~InetAddress()
{
	if(_ipaddr)
		delete[] _ipaddr;
}

InetAddress::inetaddr_t InetAddress::getAddress(size_t i /*=0*/) const
{
	return (i < _addr_count ? _ipaddr[i] : _ipaddr[0]);
}

const uint16 InetAddress::family(size_t i /* =0*/) const
{
	return (i < _addr_count ? _ipaddr[i].family : _ipaddr[0].family);
}

bool InetAddress::isInetAddress(void) const
{
	struct in_addr addr;
	memset(&addr, 0, sizeof(addr));
	if(memcmp(&addr, &_ipaddr[0].addr.a, sizeof(addr)))
		return true;
	return false;
}

bool InetAddress::isAnyAddress(void) const
{
	if (_ipaddr[0].family == PF_INET)
	{
		struct in_addr addr;
		memset(&addr, 0, sizeof(addr));
		if(memcmp(&addr, &_ipaddr[0].addr.a, sizeof(addr)))
			return false;
	}
#ifdef IPV6_ENABLED
	if (_ipaddr[0].family == PF_INET6)
	{
		struct in6_addr addr;
		memset(&addr, 0, sizeof(addr));
		if(memcmp(&addr, &_ipaddr[0].addr.a6, sizeof(addr)))
			return false;
	}
#endif // IPV6_ENABLED

	return true;
}

InetAddress &InetAddress::operator=(const char *str)
{
	if(str == 0 || !strcmp(str, "*"))
		str = "0.0.0.0";

	setAddress(str);

	return *this;
}

InetAddress &InetAddress::operator=(struct in_addr addr)
{
	if(_ipaddr)
		delete[] _ipaddr;

	if (_validator)
		(*_validator)(addr);

	_addr_count = 1;
	_ipaddr = new inetaddr_t[1];
	_ipaddr[0].family = PF_INET;
	_ipaddr[0].addr.a = addr;
	return *this;
}

InetAddress &InetAddress::operator=(unsigned long addr)
{
	if (_validator)
		(*_validator)(*reinterpret_cast<in_addr*>(&addr));

	if(_ipaddr)
		delete[] _ipaddr;

	_addr_count = 1;
	_ipaddr = new inetaddr_t[1];
	_ipaddr[0].family = PF_INET;
	memcpy(&(_ipaddr[0].addr.a), &addr, sizeof(_ipaddr[0].addr.a));
	
	return *this;        
}

#ifdef IPV6_ENABLED
InetAddress &InetAddress::operator=(struct in6_addr addr)
{
	if(_ipaddr)
		delete[] _ipaddr;

	if (_validator)
		(*_validator)(addr);

	_addr_count = 1;
	_ipaddr = new inetaddr_t[1];
	_ipaddr[0].family = PF_INET6;
	_ipaddr[0].addr.a6 = addr;
	_ipaddr[0].scopeid = 1; //??

	return *this;
}
#endif // IPV6_ENABLED

InetAddress &InetAddress::operator=(const InetAddress &rhs)
{
	if(this == &rhs) return *this;

	_addr_count = rhs._addr_count;
	
	if(_ipaddr)
		delete[] _ipaddr;
	_ipaddr = new inetaddr_t[_addr_count];
	memcpy(_ipaddr, rhs._ipaddr, sizeof(inetaddr_t) * _addr_count);
	
	_validator = rhs._validator;

	return *this;
}

bool InetAddress::operator==(const InetAddress &a) const
{
	const InetAddress *smaller, *larger;
	size_t s, l;

	if(_addr_count > a._addr_count)
	{
		smaller = &a;
		larger  = this;
	}
	else
	{
		smaller = this;
		larger  = &a;
	}

	// Loop through all addr's in the smaller and make sure
	// that they are all in the larger
	for(s = 0; s < smaller->_addr_count; s++)
	{
		/// bool found = false;
		for(l = 0; l < larger->_addr_count; l++)
		{
			if (smaller->_ipaddr[s].family == PF_INET && larger->_ipaddr[l].family == smaller->_ipaddr[s].family)
			{
				if (memcmp((char *)&(smaller->_ipaddr[s].addr.a), (char *)&(larger->_ipaddr[l].addr.a), sizeof(struct in_addr)) ==0)
					return true;
			}

#ifdef IPV6_ENABLED
			if (smaller->_ipaddr[s].family == PF_INET6 && larger->_ipaddr[l].family == smaller->_ipaddr[s].family)
			{
				if (memcmp((char *)&(smaller->_ipaddr[s].addr.a6), (char *)&(larger->_ipaddr[l].addr.a6), sizeof(struct in6_addr)) ==0)
					return true;
			}
#endif // IPV6_ENABLED
		}
	}

	return false;
}

bool InetAddress::operator==(const in_addr &a) const
{
	InetAddress tmpAddr(a);
	return (tmpAddr == *this);
}

#ifdef IPV6_ENABLED
bool InetAddress::operator==(const in6_addr &a) const
{
	InetAddress tmpAddr(a);
	return (tmpAddr == *this);
}
#endif // IPV6_ENABLED

bool InetAddress::operator!=(const InetAddress &a) const
{
	// Impliment in terms of operator==
	return (*this == a ? false : true);
}

#ifdef IPV6_ENABLED

bool InetAddress::setIPAddress(const char *host)
{
	if(!host)
		return false;

    //
    // By not setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
    // indicating that we intend to use the resulting address(es) to connect
    // to a service.  This means that when the Server parameter is NULL,
    // getaddrinfo will return one entry per allowed protocol family
    // containing the loopback address for that family.
#ifdef ZQ_OS_MSWIN
    ADDRINFO hints, *AddrInfo;
#else
	struct addrinfo hints,*AddrInfo;
#endif
	memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC; // accept any protocol family, filter later
    hints.ai_socktype = 0; //accept any socket type, filter later
    if (getaddrinfo(host, NULL, &hints, &AddrInfo) !=0)
		return false;

    // count the addresses getaddrinfo returned
	int c =0;
#ifdef ZQ_OS_MSWIN
	ADDRINFO* ai =NULL;
#else
	struct addrinfo *ai = NULL;
#endif
    for (ai =AddrInfo; ai != NULL; ai = ai->ai_next)
		if (ai->ai_family == PF_INET || ai->ai_family == PF_INET6)
			c ++;

	if (c <=0)
		return false;

	_addr_count = c;
	if(_ipaddr)
		delete[] _ipaddr;
	_ipaddr = new inetaddr_t[_addr_count];

    c=0;
	for (ai = AddrInfo; c < (int) _addr_count && ai != NULL; ai = ai->ai_next, c++)
	{
		_ipaddr[c].family = ai->ai_family;
		if (_ipaddr[c].family == PF_INET)
		{
			const sockaddr_in* sin = reinterpret_cast<const sockaddr_in*>(ai->ai_addr);
			_ipaddr[c].addr.a = sin->sin_addr;
			continue;
		}

		if (_ipaddr[c].family == PF_INET6)
		{
			const sockaddr_in6 * sin = reinterpret_cast<const sockaddr_in6 *>(ai->ai_addr);
			_ipaddr[c].addr.a6 = sin->sin6_addr;
			_ipaddr[c].scopeid = sin->sin6_scope_id;
			continue;
		}
    }

    // We are done with the address info chain, so we can free it.
    freeaddrinfo(AddrInfo);

	return true;
}

#else
bool InetAddress::setIPAddress(const char *host)
{
	if(!host)
		return false;

#if defined(ZQ_OS_MSWIN)
	struct sockaddr_in addr;
	
	addr.sin_addr.s_addr = inet_addr(host);
	if (_validator)
		(*_validator)(addr.sin_addr);
	if(addr.sin_addr.s_addr == INADDR_NONE)
		return false;
	*this = addr.sin_addr.s_addr;
#else
	struct in_addr l_addr;
	
	int ok = inet_aton(host, &l_addr);
	if (_validator)
		(*_validator)(l_addr);
	if (!ok)
		return false;
	*this = l_addr;
#endif
	return true;
}
#endif // IPV6_ENABLED

bool InetAddress::setIPAddress(const sockaddr& host)
{
	if (host.sa_family == PF_INET)
	{
		const sockaddr_in& sin = reinterpret_cast<const sockaddr_in&>(host);
		*this = sin.sin_addr;
		return true;
	}

#ifdef IPV6_ENABLED
	if (host.sa_family == PF_INET6)
	{
		const sockaddr_in6& sin = reinterpret_cast<const sockaddr_in6&>(host);
		in6_addr tmp_addr;
		memcpy(&tmp_addr, sin.sin6_addr.s6_addr, sizeof(tmp_addr));
		*this = tmp_addr;
		return true;
	}
#endif // IPV6_ENABLED

	return false;
}

/// Used to specify a host name or numeric internet address.
bool InetAddress::setAddress(const char *host)
{	
	if(!host)  // The way this is currently used, this can never happen
	{
		*this = (long unsigned int)htonl(INADDR_ANY);
		return false;
	}

	if(!setIPAddress(host))
	{
		struct hostent *hp;
		struct in_addr **bptr;
#if defined(__GLIBC__)
		char   hbuf[8192];
		struct hostent hb;
		int    rtn;

		if(gethostbyname_r(host, &hb, hbuf, sizeof(hbuf), &hp, &rtn))
			hp = NULL;
#elif defined(sun)
		char   hbuf[8192];
		struct hostent hb;
		int    rtn;

		hp = gethostbyname_r(host, &hb, hbuf, sizeof(hbuf), &rtn);
#elif (defined(__osf__) || defined(ZQ_OS_MSWIN))
		hp = gethostbyname(host);
#else
		mutex.enter();
		hp = gethostbyname(host);
		mutex.leave();
#endif
		if(!hp)
		{
			if(_ipaddr)
				delete[] _ipaddr;
			_ipaddr = new inetaddr_t[1];
			memset((void *)&_ipaddr[0], 0, sizeof(inetaddr_t));
			return false;
		}
		
		// Count the number of IP addresses returned
		_addr_count = 0;
		for(bptr = (struct in_addr **)hp->h_addr_list; *bptr != NULL; bptr++)
		{
			_addr_count++;
		}

		// Allocate enough memory
		if(_ipaddr)
			delete[] _ipaddr;	// Cause this was allocated in base
		_ipaddr = new inetaddr_t[_addr_count];

		// Now go through the list again assigning to 
		// the member _ipaddr;
		bptr = (struct in_addr **)hp->h_addr_list;
		for(unsigned int i = 0; i < _addr_count; i++)
		{
			if (_validator)
				(*_validator)(*bptr[i]);
			_ipaddr[i].family = PF_INET;
			_ipaddr[i].addr.a = *bptr[i];
		}
	}
	else if (_validator && _addr_count>0)
	{
		inetaddr_t* old = _ipaddr;
		std::vector<inetaddr_t*> validaddrs;
		unsigned int i;

		// scan and filter out the illegal values
		for (i = 0; i < _addr_count; i++)
		{
			if (_ipaddr[i].family == PF_INET)
			{
				try {
					(*_validator)(_ipaddr[i].addr.a);
					validaddrs.push_back(&_ipaddr[i]);
				} catch(...) {}
			}

#ifdef IPV6_ENABLED
			if (_ipaddr[i].family == PF_INET6)
			{
				try {
					(*_validator)(_ipaddr[i].addr.a6);
					validaddrs.push_back(&_ipaddr[i]);
				} catch(...) {}
			}
#endif // IPV6_ENABLED
		}

		// re-set the _ipaddr array
		_addr_count = validaddrs.size();
		if (_addr_count >0)
		{
			_ipaddr =  new inetaddr_t[_addr_count];
			for (i = 0; i < _addr_count; i++)
				memcpy(&_ipaddr[i], (validaddrs[i]),sizeof(_ipaddr[i]));
		}

		delete[] old;
		validaddrs.clear();
	}

	return (_addr_count >0);
}

const char *InetAddress::getHostname(void) const
{
	struct hostent *hp = NULL;
	struct in_addr addr0;

	memset(&addr0, 0, sizeof(addr0));
	if(!memcmp(&addr0, &_ipaddr[0], sizeof(addr0)))
		return NULL;

#if defined(__GLIBC__)
	char   hbuf[8192];
	struct hostent hb;
	int    rtn;
	if(gethostbyaddr_r((char *)&_ipaddr[0], sizeof(addr0), AF_INET, &hb, hbuf, sizeof(hbuf), &hp, &rtn))
		hp = NULL;
#elif defined(sun)
	char   hbuf[8192];
	struct hostent hb;
	int    rtn;
	hp = gethostbyaddr_r((char *)&_ipaddr[0], (int)sizeof(addr0), (int)AF_INET, &hb, hbuf, (int)sizeof(hbuf), &rtn);
#elif defined(__osf__) || defined(ZQ_OS_MSWIN)
	hp = gethostbyaddr((char *)&_ipaddr[0], sizeof(addr0), AF_INET);
#else
	mutex.enter();
	hp = gethostbyaddr((char *)&_ipaddr[0], sizeof(addr0), AF_INET);
	mutex.leave();
#endif
	if(hp)
		return hp->h_name;

	// numeric address string if failed to assc to domain name
	return getHostAddress();
}

const char *InetAddress::getHostAddress(size_t i /*=0*/) const
{
	if (_addr_count<=0)
		return "";

	i = (i >= _addr_count || i<0)? 0: i;

#ifdef IPV6_ENABLED

	if (_ipaddr[i].family == PF_INET6)
	{
		static char buff[40];
		int j, w=1, len=0, ii=1, lo=0;
		char x[16]={0,};

		for (j=0; j<16; j++) {
			if(j==15 && (!(x[14]) && !(x[13]) && !(x[12]))){
				sprintf(buff+len, "::");
				len+=2; lo++;
			}
#ifdef ZQ_OS_MSWIN
			if(_ipaddr[i].addr.a6.u.Byte[j]){
				ii++; x[j]=1;
				sprintf(buff+len, "%02x", (_ipaddr[i].addr.a6.u.Byte[j]));
				len += 2;
			}
#else
			if(_ipaddr[i].addr.a6.s6_addr[j]){
				ii++; x[j]=1;
				sprintf(buff+len, "%02x", (_ipaddr[i].addr.a6.s6_addr[j]));
				len += 2;
			}
#endif
			if(!(w++%2) && (x[j] || (!(x[j]) && !(x[j-1]) && x[j-2])) && j<15){
				sprintf(buff+len, ":");
				len++;
			}
		}
		if(ii==1 && !lo) sprintf(buff+len, "::");

		return(buff);

	}
#endif // IPV6_ENABLED

	if (_ipaddr[i].family == PF_INET)
		return inet_ntoa(_ipaddr[i].addr.a);

	return "";
}

/// -----------------------------
/// class InetMaskAddress
/// -----------------------------
InetMaskAddress::InetMaskAddress(const char *mask)
{
	unsigned long x = 0xffffffff;
	int l = 32 - atoi(mask);

	if(setIPAddress(mask))
		return;

	if(l < 1 || l > 32)
		throw InetdAddrException("InetMaskAddress out of range");

	*this = htonl(x << l);
}

InetMaskAddress::InetMaskAddress(const uint16 family, const int bits)
{
	inetaddr_t addr;
	memset(&addr,0, sizeof(addr));
	addr.family = family;
	uint8 *p = addr.addr.reserved;

	int i=bits;
	for (; i>=8; i-=8, p++)
		*p = 0xff;

	for (;i>0;i--)
	{
		*p >>=1;
		*p |=0x80;
	}

#ifdef IPV6_ENABLED
	if (family == PF_INET6)
		InetAddress::operator =(addr.addr.a6);
	else
#endif //  IPV6_ENABLED
	InetAddress::operator =(addr.addr.a);
}

InetAddress & InetMaskAddress::operator=(unsigned long addr) 
{
	return InetAddress::operator =(addr);
}


/// -----------------------------
/// class InetHostAddress
/// -----------------------------
InetHostAddress &InetHostAddress::operator&=(const InetMaskAddress &ma)
{
	for(size_t i = 0; i < _addr_count; i++)
	{		
		inetaddr_t mask;
		for (size_t k = 0; k < ma.getAddressCount(); k++)
		{
			mask = ma.getAddress(k);
			if (_ipaddr[i].family == mask.family)
				break;
		}

		if (_ipaddr[i].family !=mask.family)
			continue; // no matched family mask specified, leave the address unchanged

#ifdef IPV6_ENABLED
		int len = sizeof(struct in6_addr);
#else
		int len = sizeof(struct in_addr);
#endif		
		unsigned char *a = (unsigned char *)&_ipaddr[i].addr;
		unsigned char *m = (unsigned char *)&mask.addr;
		
		for(int j = 0; j < len; ++j)
			a[j] &= m[j];
	}

	return *this;
}
	
InetHostAddress::InetHostAddress(struct in_addr addr)
                :InetAddress(addr)
{
}

#ifdef IPV6_ENABLED
InetHostAddress::InetHostAddress(struct in6_addr addr)
                :InetAddress(addr)
{
}
#endif // IPV6_ENABLED

InetHostAddress::InetHostAddress(const char *host)
                :InetAddress(host)
{
	char namebuf[256];

	if(host ==NULL)
	{
		gethostname(namebuf, sizeof(namebuf));
		setAddress(namebuf);
	}
	
}

InetHostAddress InetHostAddress::getLocalAddress()
{
	InetHostAddress localAddr;

#ifdef ZQ_OS_MSWIN

#ifdef GETADAPTERSADDRESSES // is only available on Windows-XP ,Server2003 or above

	DWORD dwStackType;
	IP_ADAPTER_ADDRESSES *pAdapterAddresses, *AI;
	DWORD dwRet, dwSize;
	int i;
	// Obtain the size of the structure
	// Start with IPv6

	dwRet = GetAdaptersAddresses(AF_UNSPEC,0,NULL,NULL,&dwSize);
	if (dwRet != ERROR_BUFFER_OVERFLOW) 
		return NULL;
		
	// Allocate memory (IPv6)
	pAdapterAddresses = (IP_ADAPTER_ADDRESSES *) LocalAlloc(LMEM_ZEROINIT,dwSize);
	if (pAdapterAddresses == NULL) {
		return NULL;
	}
	// Obtain network adapter information (IPv6)
	dwRet = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAdapterAddresses, &dwSize);
	if (dwRet != ERROR_SUCCESS) {
		LocalFree(pAdapterAddresses);
		return NULL;
	} else {
		// scan for ip numbers
		if(localAddr._ipaddr)
			delete[] localAddr._ipaddr;
		localAddr._addr_count = 0;

		for (i = 0, AI = pAdapterAddresses; AI != NULL; AI = AI->Next, i++) {
			if (AI->FirstUnicastAddress != NULL) {
				struct _IP_ADAPTER_UNICAST_ADDRESS* currip;
				for(currip = AI->FirstUnicastAddress; currip !=NULL; currip = currip->Next) {
					localAddr._addr_count++;
				}
			} 
		} // end for
		// allocate enough memory
		localAddr._ipaddr = new inetaddr_t[localAddr._addr_count];

		// now go though the list again and assign ip
		int ipindx=0;
		for (i = 0, AI = pAdapterAddresses; AI != NULL; AI = AI->Next, i++) {
			if (AI->FirstUnicastAddress != NULL) {
				
				struct _IP_ADAPTER_UNICAST_ADDRESS* currip;
				for(currip = AI->FirstUnicastAddress; currip !=NULL; currip = currip->Next) {
					if(currip->Address.lpSockaddr->sa_family == AF_INET6) {
						sockaddr_in6 *in6Address;
						in6Address = (sockaddr_in6*)currip->Address.lpSockaddr;
						in_addr6 localinaddr;
						localinaddr = in6Address->sin6_addr;
						localAddr._ipaddr[ipindx].family = PF_INET6;
						localAddr._ipaddr[ipindx].addr.a6 = localinaddr;
						ipindx++;
					}
					else if(currip->Address.lpSockaddr->sa_family == AF_INET) {
						sockaddr_in *in4Address;
						in4Address = (sockaddr_in*)currip->Address.lpSockaddr;
						in_addr localinaddr;
						localinaddr = in4Address->sin_addr;
						localAddr._ipaddr[ipindx].family = PF_INET;
						localAddr._ipaddr[ipindx].addr.a = localinaddr;
						ipindx++;
					}
				} 
			}
		} // end for
	}


	LocalFree(pAdapterAddresses);
	//return localAddr;

#else
	// GetAdaptersAddresses() method not available

	SOCKET sd;

	sd = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, 0);


	if (sd ==INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		return NULL;
	}

	INTERFACE_INFO InterfaceList[MAX_INTERFACES];
	unsigned long nBytesReturned;

	int re;
	re=WSAIoctl(sd,
		SIO_GET_INTERFACE_LIST,
		NULL,
		0,
		&InterfaceList,
		sizeof(InterfaceList),
		&nBytesReturned,
		NULL,
		NULL);
		
	if(re == SOCKET_ERROR){
		int err2 = WSAGetLastError();
		return NULL;
	}

	int ifcount=nBytesReturned / sizeof(INTERFACE_INFO);

	if (ifcount <=0)
		return NULL;

	// set local ip number
	localAddr._addr_count = ifcount;
	localAddr._ipaddr = new inetaddr_t[localAddr._addr_count];

	for (int i = 0; i < ifcount; ++i)
	{
		sockaddr_in *pAddress;

		pAddress = (sockaddr_in *) & (InterfaceList[i].iiAddress);


		in_addr localinaddr = pAddress->sin_addr;

		localAddr._ipaddr[i].family = PF_INET;
		localAddr._ipaddr[i].addr.a = localinaddr;

	}

	shutdown(sd, 2);
	closesocket(sd);
#endif //GETADAPTERSADDRESSES

#else
	int             sockfd;
	struct ifconf   ifc;
	struct ifreq    ifrlist[MAX_INTERFACES];

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	ifc.ifc_len = sizeof(ifrlist);
	ifc.ifc_req = ifrlist;

	if(ioctl(sockfd, SIOCGIFCONF, &ifc) != 0)
		return localAddr;

	int ifcount= ifc.ifc_len / sizeof(struct ifreq);

	// set local ip number
	localAddr._addr_count = ifcount;
	localAddr._ipaddr = new inetaddr_t[localAddr._addr_count];

	for (int i=0; i < ifcount; i++)
	{
//		int len = sizeof(struct ifreq);
//		struct nif_t aNif;

//		struct ifreq        ifreq_rec;
//		struct sockaddr_in *sinptr;

		#ifdef  HAVE_SOCKADDR_SA_LEN
		len =(len < ifrlist[i].ifr_addr.sa_len) ?
		ifrlist[i].ifr_addr.sa_len : len;     // length > 16
		#endif

//		aNif.family= ifrlist[i].ifr_addr.sa_family;
//		aNif.flags = ifrlist[i].ifr_flags;

		switch (ifrlist[i].ifr_addr.sa_family)
		{
		case AF_INET:
		{			
			localAddr._ipaddr[i].family = AF_INET;
			struct sockaddr_in* paddr = NULL;
			paddr = (struct sockaddr_in*)&(ifrlist[i].ifr_addr); 
			localAddr._ipaddr[i].addr.a = (paddr->sin_addr);
//			aNif.name = ifrlist[i].ifr_name;
//			sinptr = (struct sockaddr_in *) &ifrlist[i].ifr_addr;
//			aNif.addr = inet_ntoa(sinptr->sin_addr);
//
//			ifreq_rec = ifrlist[i]
//			ioctl(sockfd, SIOCGIFNETMASK, &ifreq_rec);
//			sinptr = (struct sockaddr_in*) &ifreq_rec.ifr_broadaddr;
//			aNif.netmask = inet_ntoa(sinptr->sin_addr);
//
//			if (aNif.flags & IFF_BROADCAST)
//			{
//				ifreq_rec = ifrlist[i]
//				ioctl(sockfd, SIOCGIFBRDADDR, &ifreq_rec);
//				sinptr = (struct sockaddr_in*)
//				&ifreq_rec.ifr_broadaddr;
//
//				aNif.broadaddr = inet_ntoa(sinptr->sin_addr);
//			}

			break;
		}
		default:
			printf("%s\n", ifrlist[i].ifr_name);
		}

//		mNetIFs.push_back(aNif);
	}

	shutdown(sockfd, 2);
	close(sockfd);

#endif //ZQ_OS_MSWIN

	
	return localAddr;
}


InetAddress & InetHostAddress::operator=(unsigned long addr) 
{
	return InetAddress::operator =(addr);
}

// friend func
InetHostAddress operator&(const InetHostAddress &addr, 
			  const InetMaskAddress &mask)
{
	InetHostAddress temp = addr;
	temp &= mask;
	return temp;
}



/// -----------------------------
/// class InetHostAddress
/// -----------------------------

BroadcastAddress::BroadcastAddress(const char *net)
                 :InetAddress(net)
{
}

/// -----------------------------	
/// class InetMcastAddress
/// -----------------------------
InetMcastAddress::InetMcastAddress() : 
	InetAddress(&_validator)
{
}

InetMcastAddress::InetMcastAddress(const struct in_addr address) : 
	InetAddress(address,&_validator)
{
}

#ifdef IPV6_ENABLED
InetMcastAddress::InetMcastAddress(const struct in6_addr address) : 
	InetAddress(address,&_validator)
{
}
#endif // IPV6_ENABLED

InetMcastAddress::InetMcastAddress(const char *address) :
	InetAddress(address,&_validator)
{
}



/// -----------------------------	
/// class InetdAddrException
/// -----------------------------
InetdAddrException::InetdAddrException(const std::string &what_arg) throw()
                   :Exception(what_arg)
{
}



} // namespace common
} // namespace ZQ
