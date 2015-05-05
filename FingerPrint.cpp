#include "FingerPrint.h"
#include "SystemInfo.h"
#include "TimeUtil.h"

#include <json/json.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SIG_HEAD ">>>>xor-media.com SIGNATURE BEGIN>>>>"
#define SIG_TAIL ">>>>xor-media.com SIGNATURE END>>>>"

#define LIC_HEAD ">>>>xor-media.com LICENSE BEGIN>>>>"
#define LIC_TAIL ">>>>xor-media.com LICENSE END>>>>"

#define  ONELINE  32
#define MOLOG   (_log)


//class MachineFingerPrint
namespace ZQ {
namespace common {

const static std::string publicKey =
"-----BEGIN RSA PUBLIC KEY-----\n"
"MIGJAoGBAOic4KHu6OGrph4ZdyfWrJN6yh9+j3gN1Khis8nThlI0B31fPnfPP6+O\n"
"Ybwmny4eeM4glHf++U2j0HQbUfM8XATh7Tvgnim822I4lKIkjN/EZGubYhyxcRn/\n"
"dk/I6WEeJQAQwPsEP9sFOPlhRBt8R4KSvwNEEAL9pZIPaZaeCpkvAgMBAAE=\n"
"-----END RSA PUBLIC KEY-----";

std::string MachineFingerPrint::_publicKeyXOR = publicKey;
MachineFingerPrint::MachineFingerPrint(ZQ::common::Log& log)
	:_log(log)
{

}

MachineFingerPrint::~MachineFingerPrint()
{

}

std::string MachineFingerPrint::getFingerPrint()
{
	// step 1. duplicate the _jsonSysInfo
	if (_jsonSysInfo.empty() && !collectSystemInfo())
		return "";

	std::string fingerPrint = _jsonSysInfo;
	char buffer[64] = {0};
	ZQ::common::TimeUtil::TimeToUTC(ZQ::common::TimeUtil::now(),buffer,sizeof(buffer) - 1);

	// step 2. append with current timestamp
	Json::Value   fingerPrintValue;
	Json::FastWriter writer;
	Json::Reader reader;
	reader.parse(fingerPrint,fingerPrintValue);
	fingerPrintValue["stampAsOf"] = std::string (buffer);
	fingerPrint = writer.write(fingerPrintValue);
	//fingerPrint.append("stampAsOf",buffer);

	// step 3. encrypt the fingerPrint with XOR's public key
	fingerPrint = encrypt(fingerPrint, _publicKeyXOR);
	fingerPrint = LicenseGenerater::char2Hex(fingerPrint, false);

	return fingerPrint;
}

std::string MachineFingerPrint::loadLicense(const std::string& license)
{
	// step 1. ensure _jsonSysInfo is collected
	if (_jsonSysInfo.empty() && !collectSystemInfo())
		return false;

	// step 2. encrypt the license with XOR's public key
	std::string licenseData = LicenseGenerater::hex2Char(license, true);
	std::string strLic = decrypt(licenseData, _publicKeyXOR);
	if (strLic.empty())
		return "";

	//test log 
	//MOLOG(Log::L_ERROR,CLOGFMT(MachineFingerPrint,"loadLicense() license file:[## %s ##]"),strLic.c_str());

	// step 3. read the begining JSON struct of decrypted strLic as the JSON object of finger print
	Json::Reader reader;
	Json::Value licenseValue;
	reader.parse(strLic,licenseValue);

	// step 4. step into jsonFP, see if the listed hardware models matches that in _jsonSysInfo
	// return "" if any item in jsonFP doesn't included in _jsonSysInfo
	if (!licenseValue.isMember("system"))
	{
		MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "loadLicense(),the system information mismatched"));
		return "";
	}

	Json::Value systemValue = licenseValue["system"];
	//the system
	if (systemValue.isMember("hostName"))
	{
		std::string hostName = Json::FastWriter().write(systemValue["hostName"]);
		if (std::string::npos == _jsonSysInfo.find(hostName))
		{
			MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "loadLicense() hostName mismatched"));
			return "";
		}
	}
	//
	if (systemValue.isMember("OS"))
	{
		Json::Value osValue = systemValue["OS"];
		std::string osInfo = Json::FastWriter().write(osValue);
		if (std::string::npos == _jsonSysInfo.find(osInfo))
		{
			MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "loadLicense() osName mismatched"));
			return "";
		}
	}
	//the cpu info
	if (systemValue.isMember("CPU"))
	{
		Json::Value cpuValue = systemValue["CPU"];
		int num = cpuValue.size();
		for (int i = 0; i < num; i++)
		{
			Json::Value currentCpu = cpuValue[i];
			Json::FastWriter cpuWriter;
			std::string cpuInfo = cpuWriter.write(currentCpu);
			if (std::string::npos == _jsonSysInfo.find(cpuInfo))
			{
				MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "loadLicense() cpuInfo mismatched"));
				return "";
			}
		}
	}
	//the nic inof
	if (systemValue.isMember("NIC"))
	{
		Json::Value nicValue = systemValue["NIC"];
		int num = nicValue.size();
		for (int i = 0; i < num; i++)
		{
			Json::Value currentNic = nicValue[i];
			std::string nicInfo = Json::FastWriter().write(currentNic);
			if (std::string::npos == _jsonSysInfo.find(nicInfo))
			{
				MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "loadLicense() nicInfo mismatched"));
				return "";
			}
		}
	}
	//the disk info
	if (systemValue.isMember("DISK"))
	{
		Json::Value diskValue = systemValue["DISK"];
		int num = diskValue.size();
		for (int i = 0; i < num; i++)
		{
			Json::Value currentDisk = diskValue[i];
			Json::FastWriter diskWriter;
			std::string diskInfo = diskWriter.write(currentDisk);
			if (std::string::npos == _jsonSysInfo.find(diskInfo))
			{
				MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "loadLicense() diskInfo mismatched"));
				return "";
			}
		}
	}

//	jsonFP = ..(strLic, offset=0, &posEnd);

	// step 5. read the next json struct as the approval license
	if (!licenseValue.isMember("license"))
	{
		MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "loadLicense(),the license data mismatched"));
		return "";
	}

	std::string jsonLic = licenseValue["license"].asString();
	return jsonLic;
	//jsonLic = ...(strLic, offset=posEnd)
}

bool MachineFingerPrint::collectSystemInfo(void)
{
	ZQ::common::SystemInfo sysInfo(&_log);
	ZQ::common::DeviceInfo deviceInfo(&_log);
	//deviceInfo.gatherDiskInfo();
	//sysInfo.refreshSystemUsage();
	Json::Value sysValue;
	sysValue["HostName"] = sysInfo._hostName;
	
	Json::Value osValue;
	osValue["type"] = sysInfo._os.osType;
	osValue["version"] = sysInfo._os.osVersion;
	sysValue["OS"] = osValue;
	
	//sysValue["CPUNum"] = sysInfo._cpuCount;
	Json::Value cpus;
	//Json::Value cpuValue;
	for(int i = 0; i < sysInfo._cpu.size(); i++)
	{
		Json::Value cpuInfo;
		//cpuInfo["architecture"] = sysInfo._cpuArchitecture;
		cpuInfo["name"] = sysInfo._cpu[i].cpuName;
		cpuInfo["clockMHZ"] = sysInfo._cpu[i].cpuClockMHZ;
		cpus.append(cpuInfo);
	}
	sysValue["CPU"] = cpus;

	//sysValue["NICNum"] = (int)deviceInfo._netInterfaceCard.size();
	Json::Value NICs;
	for(int i =0; i< deviceInfo._netInterfaceCard.size(); i++)
	{
		Json::Value nicInfo;
		nicInfo["mac"] = deviceInfo._netInterfaceCard[i].macAddress;
		nicInfo["name"] = deviceInfo._netInterfaceCard[i].netCardName;
		nicInfo["description"] = deviceInfo._netInterfaceCard[i].cardDescription;
		NICs[i] = nicInfo;
	}
	sysValue["NIC"] = NICs;

	//sysValue["DiskNum"] = (int)deviceInfo._disk.size();
	Json::Value Disks;
	for (int i = 0; i < deviceInfo._disk.size(); i++)
	{
		Json::Value diskInfo;
		diskInfo["id"] = deviceInfo._disk[i].diskSeque;
		diskInfo["model"] = deviceInfo._disk[i].diskModel;
		Disks[i] = diskInfo;
	}
	sysValue["DISK"] = Disks;

	Json::FastWriter writer;
	_jsonSysInfo = writer.write(sysValue);
	//test log 
	//MOLOG(Log::L_ERROR,CLOGFMT(MachineFingerPrint,"collectSystemInfo() systemInfo:[## %s ##]"),_jsonSysInfo.c_str());
	return true;
}

std::string MachineFingerPrint::encrypt(const std::string& machineData, const std::string& publicKey)
{
	OpenSSL_add_all_algorithms();
	std::string encryptedData;
	encryptedData.clear();
	BIO *pBio = BIO_new( BIO_s_mem() );
	BIO_write(pBio,publicKey.c_str(),publicKey.length()*sizeof(char) );
	RSA* rsaK = PEM_read_bio_RSAPublicKey(pBio, NULL, NULL, NULL);
	BIO_free_all(pBio);  
	if(NULL ==rsaK)
	{
		MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "encrypt() use the public key to generate the RSA failed"));
		RSA_free(rsaK); 
		CRYPTO_cleanup_all_ex_data(); 
		return "";
	}

	int nRsaSize = RSA_size(rsaK);
	int nLen = nRsaSize -11;
	char* buffer = new char[nLen + 1];
	memset(buffer,'\0', nLen + 1);
	char *pEncode = new char[nRsaSize + 1];
	memset(pEncode,'\0',nRsaSize + 1);

	int pos = 0;
	do 
	{
		snprintf(buffer, nLen + 1, "%s", machineData.substr(pos,nLen).c_str());
		pos += nLen;
		int ret = RSA_public_encrypt(nLen,(const unsigned char*)buffer,  
			(unsigned char *)pEncode,rsaK,RSA_PKCS1_PADDING);
		if(ret != nRsaSize)
		{
			delete[] pEncode;
			delete[]buffer;
			RSA_free(rsaK);
			CRYPTO_cleanup_all_ex_data(); 
			MOLOG(Log::L_ERROR,CLOGFMT(MachineFingerPrint,"encrypt() encrypt the data[%s] failed."), machineData.c_str());
			return "";
		}
		std::string result;
		result.assign(pEncode,ret);
		encryptedData.append(result);
		memset(buffer,'\0', nLen + 1);
		memset(pEncode,'\0',nRsaSize + 1);
	} while (pos < machineData.length());

	delete[] pEncode;
	delete[]buffer;

	RSA_free(rsaK);
	CRYPTO_cleanup_all_ex_data();

	MOLOG(Log::L_INFO,CLOGFMT(MachineFingerPrint,"encrypt() encrypt the data successful"));
	return encryptedData;
}
std::string MachineFingerPrint::decrypt(const std::string& licenseData, const std::string& publicKey)
{
	std::string decryptedData;
	decryptedData.clear();
	OpenSSL_add_all_algorithms();
	BIO *pBio = BIO_new( BIO_s_mem() );
	BIO_write(pBio,publicKey.c_str(),publicKey.length()*sizeof(char) );
	RSA* rsaK = PEM_read_bio_RSAPublicKey(pBio, NULL, NULL, NULL);
	BIO_free_all(pBio);

	if(NULL ==rsaK)
	{
		MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "decrypt() use the public key to generate the RSA failed"));
		RSA_free(rsaK); 
		CRYPTO_cleanup_all_ex_data(); 
		return "";
	}

	int nRsaSize = RSA_size(rsaK);
	int nLen = nRsaSize;
// 	char* buffer = new char[nLen + 1];
// 	memset(buffer,'\0', nLen + 1);
	char *pEncode = new char[nRsaSize + 1];
	memset(pEncode,'\0',nRsaSize + 1);

	int pos = 0;
	do 
	{
		std::string decryptData = licenseData.substr(pos,nLen);
		if (decryptData.length() != nLen)
		{
			MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "decrypt() the licenseData was not effective data"));
			delete []pEncode;
			//delete []buffer;
			RSA_free(rsaK); 
			CRYPTO_cleanup_all_ex_data(); 
			return "";
		}

		//snprintf(buffer, nLen + 1, "%s", decryptData.c_str());
		pos += nLen;
		int ret =  RSA_public_decrypt(nLen,(const unsigned char*)decryptData.c_str(),(unsigned char *)pEncode,rsaK,RSA_PKCS1_PADDING);  
		if(ret < 0)
		{
			MOLOG(Log::L_ERROR, CLOGFMT(MachineFingerPrint, "decrypt() decrypt the licenseData failed"));
			delete []pEncode;
			//delete []buffer;
			RSA_free(rsaK); 
			CRYPTO_cleanup_all_ex_data(); 
			return "";
		}

		std::string result;
		result.assign(pEncode,ret);
		decryptedData.append(result);
		//memset(buffer,'\0', nLen + 1);
		memset(pEncode,'\0',nRsaSize + 1);
	} while (pos < licenseData.length());

	delete []pEncode;
	//delete []buffer;
	RSA_free(rsaK); 
	CRYPTO_cleanup_all_ex_data();

	MOLOG(Log::L_INFO,CLOGFMT(MachineFingerPrint,"decrypt() decrypt the data successful"));
	return decryptedData;
}

//std::string LicenseGenerater::_privateKeyXOR = privateKey;
LicenseGenerater::LicenseGenerater(ZQ::common::Log& log, std::string jsonFingerPrintSchema)
	:_log(log), _jsonFingerPrintSchema(jsonFingerPrintSchema)
{
}

LicenseGenerater::~LicenseGenerater()
{
}

std::string LicenseGenerater::issue(const std::string& licenseeFingerPrint, const std::string& jsonLicense)
{
	std::string sigData = LicenseGenerater::hex2Char(licenseeFingerPrint, false);
	std::string licContent;

	// step 1. decrypt the licenseeFingerPrint with the private key of XOR
	std::string liceeFP = decrypt(sigData, _privateKeyXOR);
	if (liceeFP.empty())
		return "";

	//test log 
	//printf("the signature data is :[%s]\n", liceeFP.c_str());
	MOLOG(Log::L_INFO,CLOGFMT(LicenseGenerater,"issue() get the sigNature Data:[%s] successful"),liceeFP.c_str());

	// step 2. steps each listed in _jsonFingerPrintSchema to check if liceeFP provided
	// return "" if not provided, otherwise
	// copy as many as _jsonFingerPrintSchema required from the value array of liceeFP.
	// if the count of value array in liceeFP is less than required, include the whole array in liceeFP
	// save the result into the beginning of licContent
	Json ::Value sysValue;
	Json::Value machineValue;
	Json::Value FingerPrintValue;
	Json::Reader jsonReader;
	jsonReader.parse(liceeFP, machineValue);
	jsonReader.parse(_jsonFingerPrintSchema, FingerPrintValue);
	//get the hostName
	if (machineValue.isMember("HostName"))
		sysValue["HostName"] = machineValue["HostName"].asString();
	if (machineValue.isMember("OS"))
		sysValue["OS"] = machineValue["OS"];

	bool deviceInfo = false;
	//get the cpu info
	if (FingerPrintValue.isMember("CPU"))
	{
		Json::Value cpuSchemaValue = FingerPrintValue["CPU"];
		Json::Value cpus;
		Json::Value cpuValue;
		if (machineValue.isMember("CPU"))
			cpuValue = machineValue["CPU"];

		for (int i = 0; i < cpuSchemaValue.size(); i++)
		{
			try{
				int cPos = cpuSchemaValue[i].asInt();
				if(cPos < cpuValue.size())
				{
					Json::Value currentCPUValue = cpuValue[cPos];
					cpus.append(currentCPUValue);
				}
			}
			catch(...)
			{
				MOLOG(Log::L_ERROR,CLOGFMT(LicenseGenerater,"issue() the schema data[%s] is invalid, the data of cpu pos should be integer."), _jsonFingerPrintSchema.c_str());
			}
		}

		if ( !cpus.empty())
		{
			deviceInfo = true;
			sysValue["CPU"] = cpus;
		}
	}

	//get the net interface card info
	if (FingerPrintValue.isMember("NIC"))
	{
		Json::Value nicSchemaValue = FingerPrintValue["NIC"];
		Json::Value nics;
		Json::Value nicValue;
		if (machineValue.isMember("NIC"))
			nicValue = machineValue["NIC"];
		for (int i = 0; i< nicSchemaValue.size(); i++)
		{
			try{
				int nPos = nicSchemaValue[i].asInt();
				if (nPos < nicValue.size())
				{
					Json::Value currentNICValue = nicValue[nPos];
					nics.append(currentNICValue);
				}
			}
			catch(...)
			{
				MOLOG(Log::L_ERROR,CLOGFMT(LicenseGenerater,"issue() the schema data[%s] is invalid, the data of nic pos should be integer."), _jsonFingerPrintSchema.c_str());
			}
		}

		if ( !nics.empty() )
		{
			deviceInfo = true;
			sysValue["NIC"] = nics;
		}
	}

	//get the disks info
	if (FingerPrintValue.isMember("DISK"))
	{
		Json::Value diskSchemaValue = FingerPrintValue["DISK"];
		Json::Value disks;
		Json::Value diskValue;
		if (machineValue.isMember("DISK"))
			diskValue = machineValue["DISK"];

		for (int i = 0; i < diskSchemaValue.size(); i++)
		{
			try{
				int dPos = diskSchemaValue[i].asInt();
				if (dPos < diskValue.size())
				{
					Json::Value currentDiskValue = diskValue[dPos];
					disks.append(currentDiskValue);
				}
			}
			catch(...)
			{
				MOLOG(Log::L_ERROR,CLOGFMT(LicenseGenerater,"issue() the schema data[%s] is invalid, the data of disk pos should be integer."), _jsonFingerPrintSchema.c_str());
			}
		}

		if ( !disks.empty())
		{
			deviceInfo = true;
			sysValue["DISK"] = disks;
		}
	}

	if ( !deviceInfo)
	{
		MOLOG(Log::L_ERROR,CLOGFMT(LicenseGenerater,"issue() the schema data[%s]  or the singnature data [%s] is invalid, the data of disk pos should be integer."), _jsonFingerPrintSchema.c_str(), liceeFP.c_str());
		return "";
	}

	//licContent =....;
	Json::Value resValue;
	resValue["system"] = sysValue;

	// step 3 append jsonLicense with current time
	char buffer[64];
	memset(buffer,'\0',64);
	ZQ::common::TimeUtil::TimeToUTC(ZQ::common::TimeUtil::now(),buffer,sizeof(buffer) - 1);
	resValue["stampIssued"] = std::string (buffer);

	// step 4 append licContent with jsonLicense
	resValue["license"] = jsonLicense;
	Json::FastWriter writer;
	licContent = writer.write(resValue);

	//licContent += jsonLicense;
	//test log 
	//printf("the license data is : [%s]\n", licContent.c_str());
	MOLOG(Log::L_INFO,CLOGFMT(LicenseGenerater,"issue() generate the license data:[%s] successful."),licContent.c_str());
	
	// step 5. encrypt the licContent with the privateKey of XOR-Media
	licContent = encrypt(licContent, _privateKeyXOR);
	licContent = LicenseGenerater::char2Hex(licContent, true);
	return licContent;
}

std::string LicenseGenerater::encrypt(const std::string& licenseData, const std::string& privateKey)	
{
	std::string encryptedData;
	encryptedData.clear();
	OpenSSL_add_all_algorithms();  
	BIO* bp = BIO_new( BIO_s_mem() );  
	BIO_write(bp,privateKey.c_str(),privateKey.length() * sizeof(char));
	unsigned char passwd[] = "hellose1";
	RSA* rsaK = PEM_read_bio_RSAPrivateKey(bp, NULL, NULL,passwd); 
	BIO_free_all( bp );   
	
	if( NULL == rsaK )
	{
		MOLOG(Log::L_ERROR, CLOGFMT(LicenseGenerater, "encrypt() use the private key to generate the RSA failed"));
		RSA_free(rsaK); 
		CRYPTO_cleanup_all_ex_data(); 
		return "";
	}

	int nRsaSize = RSA_size(rsaK);
	int nLen = nRsaSize - 11;
	char* buffer = new char[nLen + 1];
	memset(buffer,'\0', nLen + 1);
	char *pEncode = new char[nRsaSize + 1];
	memset(pEncode,'\0',nRsaSize + 1);

	int pos = 0;
	do 
	{
		snprintf(buffer, nLen + 1, "%s", licenseData.substr(pos,nLen).c_str());
		pos += nLen;
		int ret = RSA_private_encrypt(nLen,(const unsigned char*)buffer,  
			(unsigned char *)pEncode,rsaK,RSA_PKCS1_PADDING);

		if(ret != nRsaSize)
		{
			delete[] pEncode;
			delete[]buffer;
			RSA_free(rsaK);
			CRYPTO_cleanup_all_ex_data(); 
			MOLOG(Log::L_ERROR,CLOGFMT(LicenseGenerater,"encrypt() encrypt the data[%s] failed."), licenseData.c_str());
			return "";
		}

		std::string result;
		result.assign(pEncode,ret);
		encryptedData.append(result);
		memset(buffer,'\0', nLen + 1);
		memset(pEncode,'\0',nRsaSize + 1);
	} while (pos < licenseData.length());

	delete[] pEncode;
	delete[]buffer;
	RSA_free(rsaK);
	CRYPTO_cleanup_all_ex_data();

	MOLOG(Log::L_INFO,CLOGFMT(LicenseGenerater,"encrypt() encrypt the data successful"));
	return encryptedData;
}

std::string LicenseGenerater::decrypt(const std::string&machineData, const std::string& privateKey)
{
	std::string decryptedData;
	decryptedData.clear();
	OpenSSL_add_all_algorithms();  
	BIO* bp = BIO_new( BIO_s_mem() );  
	BIO_write(bp,privateKey.c_str(),privateKey.length() * sizeof(char));
	unsigned char passwd[] = "hellose1";
	RSA* rsaK = PEM_read_bio_RSAPrivateKey(bp, NULL, NULL,passwd); 
	BIO_free_all( bp ); 

	if( NULL == rsaK )
	{
		MOLOG(Log::L_ERROR, CLOGFMT(LicenseGenerater, "decrypt() use the private keyto generate the RSA failed"));
		RSA_free(rsaK); 
		ERR_print_errors_fp(stdout);
		CRYPTO_cleanup_all_ex_data(); 
		return "";
	}

	int nRsaSize = RSA_size(rsaK);
	int nLen = nRsaSize;
// 	char* buffer = new char[nLen + 1];
// 	memset(buffer,'\0', nLen + 1);
	char *pEncode = new char[nRsaSize + 1];
	memset(pEncode,'\0',nRsaSize + 1);
	int pos = 0;
	do 
	{
		std::string decryptData = machineData.substr(pos,nLen);
		if (decryptData.length() != nLen)
		{
			MOLOG(Log::L_ERROR,CLOGFMT(LicenseGenerater,"decrypt() the data was not effective data"));
			delete []pEncode;
			//delete []buffer;
			RSA_free(rsaK);
			CRYPTO_cleanup_all_ex_data();
			return "";
		}

		//snprintf(buffer, nLen + 1, "%s", decryptData.c_str());
		pos += nLen;
		int ret =  RSA_private_decrypt(nLen,(const unsigned char*)decryptData.c_str(),(unsigned char *)pEncode,rsaK,RSA_PKCS1_PADDING);  
		if(ret < 0)
		{
			MOLOG(Log::L_ERROR,CLOGFMT(LicenseGenerater,"decrypt() decrypt the data failed"));
			delete []pEncode;
			//delete []buffer;
			RSA_free(rsaK);
			CRYPTO_cleanup_all_ex_data();
			return "";
		}

		std::string result;
		result.assign(pEncode,ret);
		decryptedData.append(result);
		//memset(buffer,'\0', nLen + 1);
		memset(pEncode,'\0',nRsaSize + 1);
	} while (pos < machineData.length());

	MOLOG(Log::L_INFO,CLOGFMT(LicenseGenerater,"decrypt() decrypt the data successful"));
	delete []pEncode;
	//delete []buffer;
	RSA_free(rsaK);
	CRYPTO_cleanup_all_ex_data();
	return decryptedData;
}	

std::string LicenseGenerater::getSigData(const std::string& hexSignature)
{
	std::string sigData = LicenseGenerater::hex2Char(hexSignature, false);
	return decrypt(sigData, _privateKeyXOR);
}

std::string LicenseGenerater::char2Hex(const std::string& encryptData, bool licenseData)
{
	int bitlen = encryptData.length();
	std::string hexStr = "";
	if (licenseData)
		hexStr = LIC_HEAD;
	else
		hexStr = SIG_HEAD;
	//std::string hexStr(DATA_HEAD);
	hexStr.append("\n");
	int  i;
	char szTmp[3];
	for( i = 0; i < bitlen; i++ )
	{
		memset(szTmp, 0, 3);
		snprintf( szTmp, 3, "%02X", (unsigned char) encryptData[i] );
		hexStr.append(szTmp);
		if ( (ONELINE - 1) == (i%ONELINE))
			hexStr.append("\n");
		else
			hexStr.append(" ");
	}

	if( 0 != bitlen%ONELINE)
		hexStr.append("\n");

	//hexStr.append(DATA_TAIL);
	if (licenseData)
		hexStr.append(LIC_TAIL);
	else
		hexStr.append(SIG_TAIL);

	return hexStr;
}

std::string LicenseGenerater::hex2Char(const std::string& encryptData, bool licenseData)
{
	std::string data = encryptData;
	std::string resultStr;
	int findPos;
	if(licenseData)
	{
		if ((findPos = data.find(LIC_HEAD,0)) != std::string::npos)
			data.erase(findPos,strlen(LIC_HEAD));
		if ((findPos =data.find(LIC_TAIL,0)) != std::string::npos)
			data.erase(findPos, strlen(LIC_TAIL));
	}
	else
	{
		if ((findPos = data.find(SIG_HEAD,0)) != std::string::npos)
			data.erase(findPos,strlen(SIG_HEAD));
		if ((findPos =data.find(SIG_TAIL,0)) != std::string::npos)
			data.erase(findPos, strlen(SIG_TAIL));
	}

	int hexlen = data.length();
	for (int i = 0; i < hexlen; )
	{
		if ( data[i] == ' ' ||  data[i] == '\n' || data[i] == '\r')
		{
			i ++ ;
			continue;
		}

		char hexData[3];
		memset(hexData,0,3);
		hexData[0] =  data[i];
		if (++i >= hexlen)
			break;
		hexData[1] = data[i];
		if (++i >= hexlen)
			break;
		char charBuffer[2];
		memset(charBuffer, '\0', 2);
		sscanf(hexData,"%02X",charBuffer);
		resultStr.append(charBuffer, 1);
	}

	return resultStr;
}
/*
bool LicenseGenerater::generateKey()
{
	RSA*  rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);
	char buffer[2048];
	memset(buffer,'\0',2048);
	BIO *bp = BIO_new( BIO_s_mem());  
	PEM_write_bio_RSAPublicKey(bp, rsa); 
	//BIO_get_mem_ptr(bp,buffer);
	BIO_read(bp,buffer,sizeof(buffer));
	BIO_free_all( bp ); 
	MachineFingerPrint::_publicKeyXOR.assign(buffer);
	FILE* file = fopen("publicKey","w");
	fprintf(file,"%s",buffer);
	fclose(file);
	//Éú³ÉË½Ô¿
	memset(buffer,'\0',2048);
	unsigned char passwd[] = "hellose1";
	BIO * bp2 = BIO_new( BIO_s_mem());  
	PEM_write_bio_RSAPrivateKey(bp2,rsa,EVP_des_cfb64(),passwd, 8, NULL, NULL);  
	//BIO_get_mem_ptr(bp,buffer);
	BIO_read(bp2,buffer,sizeof(buffer));
	BIO_free_all( bp2 ); 
//	LicenseGenerater::_privateKeyXOR.assign(buffer);
	file = fopen("privateKey","w");
	fprintf(file,"%s",buffer);
	fclose(file);
	RSA_free(rsa);
	return true;
}
*/
}}//namespace