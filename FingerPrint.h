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
// Ident : $Id: Exception.h,v 1.8 2004/05/26 09:32:35 Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define FingerPrint
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/FingerPrint.h $
// 
// 6     3/17/14 6:32p Ketao.zhang
// check in for get signature data and the license hex file 
// 
// 5     3/11/14 10:23a Ketao.zhang
// check in for privateKey
// 
// 4     3/07/14 3:22p Ketao.zhang
// Check in for hex and char 
// 
// 3     3/03/14 12:38p Ketao.zhang
// check in for systemInfo change
// 
// 2     2/24/14 10:32a Ketao.zhang
// check inf for NIC and Disk
// 
// 1     2/11/14 11:55a Hui.shao
// drafted the definition
// ---------------------------------------------------------------------------

#ifndef	__ZQ_COMMON_FingerPrint_H__
#define	__ZQ_COMMON_FingerPrint_H__

#include "ZQ_common_conf.h"
#include "Log.h"

#include <string>

namespace ZQ {
namespace common {

// -----------------------------
// class MachineFingerPrint
// -----------------------------
class MachineFingerPrint
{
	friend class LicenseGenerater;

public:
	MachineFingerPrint(ZQ::common::Log& log);
	virtual ~MachineFingerPrint();

	/// generate the fingerprint based on the collected local system info
	///@return the fingerprint string ecrypted
	std::string getFingerPrint();

	/// load a license content that has been encrypted
	/// @param license the license content
	/// @return the approval info of the license that has been validated for this local machine
	std::string loadLicense(const std::string& license);

protected:
	ZQ::common::Log& _log;

	std::string _jsonSysInfo; // could be the json object here

	/// collect the system info of the local machine and save in _jsonSysInfo, fomatted in JSON, such as
	/// { "processors": [{"vendor": "intel", "model": "P5", "clock": "1.5G"},],
	///   "nics": [{"name":"Intel(R) 82579LM Gigabit Network Connection", "mac":"00-21-CC-CE-AF-DA"},...],
	/// }
	/// be aware each direct child's value type is array
	virtual bool collectSystemInfo(void);
	
	//use RSA to encrypt the local machineData .
	// @param machineData the machine data of SystemInfo
	// @param publicKey the publicKey of RSA
	std::string encrypt(const std::string& machineData, const std::string& publicKey);
	
	//use RSA to decrypt the Data return from license generater.
	// @param licenseData the license generate data
	// @param publicKey the publicKey of RSA
	std::string decrypt(const std::string& licenseData, const std::string& publicKey);

	static std::string _publicKeyXOR; // the public key of XOR-Media built into the software
};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COMMON_FingerPrint_H__

// the following should be in a separate header file other than FingerPrint.h
#ifndef	__ZQ_COMMON_LicenseIssuer_H__
#define	__ZQ_COMMON_LicenseIssuer_H__

namespace ZQ {
namespace common {

//class LicenseIssuer
class LicenseGenerater
{
public:
	LicenseGenerater(ZQ::common::Log& log, std::string jsonFingerPrintSchema);
	virtual ~LicenseGenerater();

	/// issue a license content with approval and the fingerprint of licensee
	///@param licenseeFingerPrint the fingerprint of the licensee machine reported, which has been encrypted by MachineFingerPrint::getFingerPrint()
	///@param jsonLicense the license infomation 
	std::string issue(const std::string& licenseeFingerPrint, const std::string& jsonLicense);
	
	//getSingData will return the data of signature
	std::string getSigData(const std::string& hexSignature);
	//static bool generateKey();
	static std::string char2Hex(const std::string& encryptData, bool licenseData);
	static std::string hex2Char(const std::string& encryptData, bool licenseData);
protected:
	ZQ::common::Log& _log;
	std::string _jsonFingerPrintSchema; // such as {"processors":1, "nics":2 }
protected:
	//use RSA to encrypt the local licenseData .
	// @param licenseData the license generate data
	// @param privateKey the privateKey of RSA
	std::string encrypt(const std::string& licenseData, const std::string& privateKey);
	//use RSA to decrypt the machineData.
	// @param machineData the machineData from the MachineFingerPrint
	// @param privateKey the privateKey of RSA
	std::string decrypt(const std::string&machineData, const std::string& privateKey);

	static std::string _privateKeyXOR; // the private key of XOR-Media built into the software
	//std::string _privateKeyXOR; // the private key of XOR-Media built into the software
};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COMMON_LicenseIssuer_H__
