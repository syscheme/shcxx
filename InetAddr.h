// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai,,
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
// Ident : $Id: InetAddr.h,v 1.20 2004/07/26 10:56:35 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define Inet address
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/InetAddr.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 11    09-05-11 18:35 Fei.huang
// 
// 10    08-12-09 14:34 Yixin.tian
// 
// 9     08-03-14 12:21 Fei.huang
// alway include socket header on Windows
// 
// 8     08-03-06 16:17 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 7     08-03-03 17:10 Yixin.tian
// merged changes for linux
// 
// 4     04-11-18 15:36 Hui.shao
// added MaskAddress constructor to accept leading '1' bits
// 
// 3     04-10-10 18:06 Hui.shao
// added anyaddress checkig
// 
// 2     04-10-09 18:10 Hui.shao
// added scopeid for addr6
// Revision 1.20  2004/07/26 10:56:35  shao
// adj GETADAPTERSADDRESSES pre-comp definition
//
// Revision 1.19  2004/07/23 07:12:02  bzhao
// added GETADAPTERSADDRESSES flag
// and modeified getLocalAddress() for WINXP and WIN2003 to get local IPv6 addresses
// modified getHostAddress() to compatible with IPv6 string output
//
// Revision 1.18  2004/07/22 09:41:50  shao
// IPV6_ENABLED range bug
//
// Revision 1.17  2004/07/22 06:16:16  shao
// comment on getHostAddress()
//
// Revision 1.16  2004/07/20 08:18:06  shao
// index for getHostAddress()
//
// Revision 1.15  2004/07/20 07:31:29  shao
// added InetAddress::operator==(const in/6_addr &a)
//
// Revision 1.14  2004/07/20 07:29:58  shao
// added InetAddress::operator==(const in/6_addr &a)
//
// Revision 1.13  2004/07/16 07:50:52  bzhao
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
// Revision 1.9  2004/06/18 06:55:59  shao
// reserved byte[] in the address union
//
// Revision 1.8  2004/06/18 06:49:25  shao
// supported IPv6
//
// Revision 1.7  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.6  2004/04/28 05:43:28  shao
// doxy comment format
//
// Revision 1.2  2004/04/21 04:24:19  shao
// winsock2
//
// ===========================================================================

#ifndef	__ZQ_COM_INETADDR_H__
#define	__ZQ_COM_INETADDR_H__

#include "ZQ_common_conf.h"
#include "Exception.h"


#if defined(ZQ_OS_MSWIN)
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	pragma comment(lib, "wsock32.lib") // VC link wsock32
#	ifdef IPV6_ENABLED
#		include <tpipv6.h>  // For IPv6 Tech Preview.
#	endif // IPV6_ENABLED
#elif defined(ZQ_OS_LINUX)
#		include <sys/types.h>
#		include <sys/socket.h>
#		include <netinet/in.h>
#endif // _MSC_VER

#include <string>
#include <exception>


namespace ZQ {
namespace common {


class ZQ_COMMON_API InetAddress;
class ZQ_COMMON_API InetHostAddress;
class ZQ_COMMON_API InetMaskAddress;
class ZQ_COMMON_API BroadcastAddress;
class ZQ_COMMON_API InetMcastAddress;


#define MAX_INTERFACES 32

// global local host address object
//extern ZQ::common::InetLocalHostAddress gLocalHostAddr;


// -----------------------------
// class InetAddrValidator
// -----------------------------
/// Classes derived from InetAddress would require an specific
/// _validator to pass to the InetAddress constructor. This is a base
/// class for classes of function objects used by such derived classes.
class InetAddrValidator 
{
public:

	/// Constructor. Does not deal with any state.
	InetAddrValidator() { };
	virtual ~InetAddrValidator(){}
	/// Pure virtual application operator. Apply the validation
	/// algorithm specific to derived classes.  
	virtual void operator() (const in_addr address) const = 0;
#ifdef IPV6_ENABLED
	virtual void operator() (const in6_addr address) const = 0;
#endif // IPV6_ENABLED
};

// -----------------------------
// class InetAddress
// -----------------------------
/// The network name and address objects are all derived from a common 
/// InetAddress base class. Specific classes, such as InetHostAddress,
/// InetMaskAddress, etc, are defined from InetAddress entirely so that the
/// manner a network address is being used can easily be documented and
/// understood from the code and to avoid common errors and accidental misuse 
/// of the wrong address object.  For example, a "connection" to something
/// that is declared as a "InetHostAddress" can be kept type-safe from a
/// "connection" accidently being made to something that was declared a 
/// "InetBroadcastAddress".
class InetAddress
{
public:
	///
	typedef struct _inetaddr
	{
		uint16 family; // PF_INET or PF_INT6
		union
		{
			uint8  reserved[20];
			struct in_addr a;
#ifdef IPV6_ENABLED
//			struct in_addr6 a6;
			struct in6_addr a6;
#endif // IPV6_ENABLED
		} addr;
#ifdef IPV6_ENABLED
		uint32 scopeid;
#endif // IPV6_ENABLED
	} inetaddr_t;

private:
	/// The _validator given to an InetAddress object must not be a
	/// transient object, but that must exist at least until the
	/// last address object of its kind is deleted. This is an
	/// artifact to be able to do specific checks for derived
	/// classes inside constructors.
	const InetAddrValidator*	_validator;

protected:

	inetaddr_t*		    _ipaddr;
	size_t				_addr_count;

/* TODO:
#if defined(ZQ_OS_MSWIN)
	static MutexCounter counter;
#else
	static Mutex mutex;
#endif
*/

	/// Sets the IP address from a string representation of the
	/// numeric address, ie "127.0.0.1"
	/// For the IPv6 address, the numeric string representation can be
	/// 1) The preferred form is x:x:x:x:x:x:x:x, where the 'x's are the hexadecimal
	/// values of the eight 16-bit pieces of the address. This is the full form.
	/// e.g. 1080:0:0:0:8:800:200C:417A . Note that it is unnecessary to write the
	/// leading zeros in an individual field. However, there must be at least one
	/// numeral in every field, except as described below. 
	/// 2) Due to some methods of allocating certain styles of IPv6 addresses, it
	/// will be common for addresses to contain long strings of zero bits. In order
	/// to make writing addresses containing zero bits easier, a special syntax is
	/// available to compress the zeros. The use of "::" indicates multiple groups
	/// of 16-bits of zeros. The "::" can only appear once in an address. The "::"
	/// can also be used to compress the leading and/or trailing zeros in an
	/// address. e.g. 1080::8:800:200C:417A  
	/// 3) An alternative form that is sometimes more convenient when dealing with
	/// a mixed environment of IPv4 and IPv6 nodes is x:x:x:x:x:x:d.d.d.d, where
	/// the 'x's are the hexadecimal values of the six high-order 16-bit pieces of
	/// the address, and the 'd's are the decimal values of the four low-order
	/// 8-bit pieces of the standard IPv4 representation address,
	/// e.g. ::FFFF:129.144.52.38  and ::129.144.52.38  
	/// where "::FFFF:d.d.d.d" and "::d.d.d.d" are, respectively, the general forms
	/// of an IPv4-mapped IPv6 address and an IPv4-compatible IPv6 address. Note
	/// that the IPv4 portion must be in the "d.d.d.d" form. The following forms
	/// are invalid: ::FFFF:d.d.d , ::FFFF:d.d , ::d.d.d , ::d.d  
	/// The form: ::FFFF:d  is valid, however it is an unconventional
	/// representation of the IPv4-compatible IPv6 address, ::255.255.0.d while
	/// "::d" corresponds to the general IPv6 address "0:0:0:0:0:0:0:d". 
	/// For methods that return a textual representation as output value, the full
	/// form is used. Inet6Address will return the full form because it is
	/// unambiguous when used in combination with other textual data.
	bool setIPAddress(const char *host);

#ifdef IPV6_ENABLED
	bool setIPAddress(const in6_addr& host);
#endif //  IPV6_ENABLED

	bool setIPAddress(const sockaddr& host);

public:

	/// Create an Internet Address object with an empty (0.0.0.0) address.
	InetAddress(const InetAddrValidator *_validator = NULL);

	/// Convert the system internet address data type (struct in_addr)
	/// into an InetAddress object.
	InetAddress(struct in_addr addr, const InetAddrValidator *_validator = NULL);

#ifdef IPV6_ENABLED
	/// Convert the system IPv6 internet address data type (struct in6_addr)
	/// into an InetAddress object.
	InetAddress(struct in6_addr addr, const InetAddrValidator *_validator=NULL);
#endif // IPV6_ENABLED

	/// Convert a null terminated ASCII host address string (example: "127.0.0.1")
	/// or host address name (example: "www.voxilla.org") directly into an
	/// InetAddress object.
	InetAddress(const char *address, const InetAddrValidator *_validator = NULL);

	/// Copy constructor
	InetAddress(const InetAddress &rhs);

	/// Destructor
	virtual ~InetAddress();

	/// Used to specify a host name or numeric internet address.
	bool setAddress(const char *host);

	/// Provide a string representation of the value (Internet Address)
	/// held in the InetAddress object.
	/// note: the return value will be overwritten if you call next getHostname()
	///       or getHostAddress()
	const char *getHostname(void) const;

	/// Provide a numeric string representation of the value (Internet Address)
	/// held in the InetAddress object.
	/// note: the return value will be overwritten if you call next getHostname()
	///       or getHostAddress()
	const char *getHostAddress(size_t i =0) const;

	/// May be used to verify if a given InetAddress returned
	/// by another function contains a "valid" address, or "0.0.0.0"
	/// which is often used to mark "invalid" InetAddress values.
	bool isInetAddress(void) const;

	bool isAnyAddress(void) const;

	/// Provide a low level system usable struct in_addr object from
	/// the contents of InetAddress.  This is needed for services such
	/// as bind() and connect().
	/// @param i for InetAddresses with multiple addresses, returns the
	///	address at this index.  User should call getAddressCount() 
	///	to determine the number of address the object contains.
	/// @return system binary coded internet address.  If parameter i is
	///	out of range, the first address is returned.
	inetaddr_t getAddress(size_t i =0) const;

	/// Returns the number of internet addresses that an InetAddress object
	/// contains.  This usually only happens with InetHostAddress objects
	/// where multiple IP addresses are returned for a DNS lookup
	size_t getAddressCount() const { return _addr_count; }

	const uint16 family(size_t i =0) const; 

	InetAddress &operator=(const char *str);
	InetAddress &operator=(struct in_addr addr);
	InetAddress &operator=(const InetAddress &rhs);
	InetAddress &operator=(struct in6_addr addr);

    /// Allows assignment from the return of functions like
	/// inet_addr() or htonl() 
	InetAddress &operator=(unsigned long addr);

	inline bool operator!() const {return !isInetAddress();};

	/// Compare two internet addresses to see if they are equal
	/// (if they specify the physical address of the same internet host).
	/// 
	/// If there is more than one IP address in either InetAddress object,
	/// this will return true if all of the IP addresses in the smaller
	/// are in the larger in any order.
	bool operator==(const InetAddress &a) const;

	/// If there is more than one IP address in either InetAddress object,
	/// this will return true if all of the IP addresses in the smaller
	/// are in the larger in any order.
	bool operator==(const in_addr &a) const;

#ifdef IPV6_ENABLED
	/// If there is more than one IP address in either InetAddress object,
	/// this will return true if all of the IP addresses in the smaller
	/// are in the larger in any order.
	bool operator==(const in6_addr &a) const;
#endif// IPV6_ENABLED

	/// Compare two internet addresses to see if they are not
	/// equal (if they each refer to unique and different physical
	/// ip addresses).
	bool operator!=(const InetAddress &a) const;
};	

// -----------------------------
// class InetMcastAddrValidator
// -----------------------------
/// Class for the function object that validates multicast addresses.
/// Implements a specific application operator to validate multicast
/// addresses.
class InetMcastAddrValidator : public InetAddrValidator
{
public:

	/// Constructor. Does not deal with any state.
	InetMcastAddrValidator(){};
	virtual ~InetMcastAddrValidator(){}
	
	/// Application operator. Apply the validation algorithm
	/// specific to multicast addresses
	void operator()(const in_addr address) const; 
#ifdef IPV6_ENABLED
	void operator()(const in6_addr address) const; 
#endif // IPV6_ENABLED

private:

#if __BYTE_ORDER == __BIG_ENDIAN
	enum { MCAST_VALID_MASK = 0xF0000000, MCAST_VALID_VALUE = 0xE0000000 };
#else
	enum { MCAST_VALID_MASK = 0x000000F0, MCAST_VALID_VALUE = 0x000000E0 };
#endif

};

// -----------------------------
// class InetMaskAddress
// -----------------------------
/// The seperate mask class is used so that C++ type casting can automatically
/// determine when an InetAddress object is really a mask address object
/// rather than simply using the base class.  This also allows manipulative
/// operators for address masking to operate only when presented with a
/// Masked address as well as providing cleaner and safer source.
class InetMaskAddress : public InetAddress
{
public:

	/// Create the mask from a null terminated ASCII string such as
	/// "255.255.255.128".
	/// @param mask null terminated ASCII mask string.
	InetMaskAddress(const char *mask="255.255.255.255");

	/// Create the mask by specify the number of leading bits of '1'
	/// @param family specify the address family
	/// @param bits   number of the leading '1' bits
	InetMaskAddress(const uint16 family, const int bits);

	/// Masks are usually used to coerce host addresses into a specific
	/// router or class domain.  This can be done by taking the Inet
	/// Host Address object and "and"ing it with an address mask.  This
	/// operation can be directly expressed in C++ through the & operator.
	/// @return a internet host address that has been masked.
	/// @param addr host address to be masked by subnet.
	/// @param mask inetnet mask address object to mask by.
	friend InetHostAddress operator&(const InetHostAddress &addr, 
					 const InetMaskAddress &mask);

    /// Allows assignment from the return of functions like
    /// inet_addr() or htonl()
	InetAddress &operator=(unsigned long addr); 
};


// -----------------------------
// class InetHostAddress
// -----------------------------
/// This object is used to hold the actual and valid internet address of a 
/// specific host machine that will be accessed through a socket.
class InetHostAddress : public InetAddress
{
public: 
	/// Create a new host address for a specific internet host.  The
	/// internet host can be specified in a null terminated ASCII
	/// string and include either the physical host address or the
	/// DNS name of a host machine.  Hence, an InetHostAddress
	/// ("www.voxilla.org") can be directly declaired in this manner.

	/// Defaults to the IP address that represents the interface matching
	/// "gethostname()".
	/// @param host dns or physical address of an Internet host.
	InetHostAddress(const char *host = NULL);

	/// Convert a system socket binary address such as may be returned
	/// through the accept() call or getsockpeer() into an internet host
	/// address object.
	/// @param addr binary address of internet host.
	InetHostAddress(struct in_addr addr);
#ifdef IPV6_ENABLED
	InetHostAddress(struct in6_addr addr);
#endif // IPV6_ENABLED

	/// get the local host address object
	///@return local host address
	static InetHostAddress getLocalAddress();

	/// Allows assignment from the return of functions like
	/// inet_addr() or htonl() 
	InetAddress &operator=(unsigned long addr); 

	/// Mask the internet host address object with a network mask address.
	/// This is commonly used to coerce an address by subnet.
	InetHostAddress &operator&=(const InetMaskAddress &mask);

	friend class InetMaskAddress;
	friend InetHostAddress operator&(const InetHostAddress &addr, 
					                 const InetMaskAddress &mask);
	
};

/*
/// -----------------------------
// class InetLocalHostAddress
// -----------------------------
class InetLocalHostAddress: public InetHostAddress
{
public:
	// constructor
	InetLocalHostAddress();

	/// Determine whether the host is local host
	///@return determine result
	virtual bool isLocal() { return true; }

};
*/

// -----------------------------
// class BroadcastAddress
// -----------------------------
/// The broadcast address object is used to store the broadcast address for
/// a specific subnet.  This is commonly used for UDP broadcast operations.
class BroadcastAddress : public InetAddress
{
public:
	/// Specify the physical broadcast address to use and create a new
	/// broadcast address object based on a null terminated ASCII string.
	/// @param net null terminated ASCII network address.
	BroadcastAddress(const char *net = "255.255.255.255");
};


// -----------------------------
// class InetMcastAddress
// -----------------------------
/// A specialization of InetAddress that provides address validation
/// for multicast addresses. Whenever its value changes the new value
/// is checked to be in the range from 224.0.0.1 through 239.255.255.255.
/// If it is not, an exception is thrown.
class InetMcastAddress: public InetAddress
{
public:
	 
	/// Create an Internet Multicast Address object with an empty address. 
	InetMcastAddress();

	/// Convert the system internet address data type (struct in_addr)
	/// into a Common C++ InetMcastAddress object.
	/// @param address struct of system used binary internet address.
	InetMcastAddress(const struct in_addr address);
#ifdef IPV6_ENABLED
	InetMcastAddress(const struct in6_addr address);
#endif // IPV6_ENABLED

	/// Convert a null terminated ASCII multicast address string
	/// (example: "224.0.0.1") or multicast name string (example:
	/// "sap.mcast.net") directly into an InetMcastAddress object.
	/// @param address null terminated C string. 
	InetMcastAddress(const char *address);

private:

	/// Check the address in <code>addr<code> is a valid multicast
	/// address. In case not, throws an exception.
	/// @param address a system network address 
	/// @return true if validation succeeded
	static const InetMcastAddrValidator _validator;
};

// -----------------------------
// class InetdAddrException
// -----------------------------
/// A sub-hierarchy for Exception classes.
class InetdAddrException : public Exception
{
public:
	InetdAddrException(const std::string &what_arg) throw();
};


} // namespace common
} // namespace ZQ


#endif // __ZQ_COM_INETADDR_H__
