// ===========================================================================
// Copyright (c) 2006 by
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
// Ident : $Id: SentryEnv.h $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/SystemInfo.h $
// 
// 10    1/04/15 3:28p Ketao.zhang
// add getCpuCounts() for bug 20492
// 
// 9     3/20/14 10:50a Ketao.zhang
// check in for adding systemInfo to ZQCommon
// 
// 8     3/14/14 5:00p Ketao.zhang
// check in for os struct
// 
// 7     3/07/14 3:47p Ketao.zhang
// check in for MOLOG
// 
// 6     3/03/14 12:38p Ketao.zhang
// check in for systemInfo change
// 
// 5     2/24/14 10:31a Ketao.zhang
// check inf for NIC and Disk
// 
// 4     8/30/13 3:01p Ketao.zhang
// 
// 3     8/29/13 1:09p Ketao.zhang
// 
// 2     8/13/13 2:21p Hui.shao
// 
// 1     8/13/13 2:20p Hui.shao
// created
// ===========================================================================

#ifndef __ZQ_Common_SystemInfo_H__
#define __ZQ_Common_SystemInfo_H__

//#include "TianShanDefines.h"
//#include "NativeThreadPool.h"
//#include "InetAddr.h"
//#include <set>
#include "ZQ_common_conf.h"
#include "Log.h"
#include <string>
#include <vector>


namespace ZQ {
namespace common {

class ZQ_COMMON_API SystemInfo;
class ZQ_COMMON_API DeviceInfo;
namespace processRawInfo{
// -----------------------------
// struct RawData 
//use to save the raw process data
// -----------------------------
#ifdef ZQ_OS_MSWIN
struct RawData
{
	std::wstring    imageW;
	DWORD           processId;
	DWORD           parentPid;
	DWORD           handleCount;
	DWORD           threadCount;
	LARGE_INTEGER   cpuTime100nsec;
	LARGE_INTEGER   memUsageByte;
	LARGE_INTEGER   vmemSizeByte;
public:
	void initData()
	{
		imageW = std::wstring(L"");
		processId = 0;
		parentPid = 0;
		handleCount = 0;
		threadCount = 0;
		cpuTime100nsec.QuadPart = 0;
		memUsageByte.QuadPart = 0;
		vmemSizeByte.QuadPart = 0;
	}
};
#else
struct RawData
{
	std::string    imageName;
	uint32          processId;
	uint32           parentPid;
	uint32           handleCount;
	uint32           threadCount;
	uint32   cpuTime100nsec;
	uint32   memUsageByte;
	uint32   vmemSizeByte;
public:
	void initData()
	{
		imageName = "";
		processId = 0;
		parentPid = 0;
		handleCount = 0;
		threadCount = 0;
		cpuTime100nsec = 0;
		memUsageByte = 0;
		vmemSizeByte = 0;
	}
};
#endif
// -----------------------------
// struct RawData 
//use to save the raw process data
// -----------------------------
struct ProcessState
{
#ifdef ZQ_OS_MSWIN
	LARGE_INTEGER sysPerfTime100nsec;
#else
	uint64 sysPerfTime100nsec;
#endif
	std::vector< RawData > processesRawData;
	ProcessState()
	{
#ifdef ZQ_OS_MSWIN
		sysPerfTime100nsec.QuadPart = 0;
#else
		sysPerfTime100nsec =0;
#endif
	}
	void swap(ProcessState& other){
		std::swap(sysPerfTime100nsec, other.sysPerfTime100nsec);
		processesRawData.swap(other.processesRawData);
	}
};
}//processRawInfo 

// -----------------------------
// class SystemInfo
// -----------------------------
// re-implement of Sentry Neighborhood.cpp
class SystemInfo
{
public:
	SystemInfo(ZQ::common::Log* log);
	virtual ~SystemInfo();
	void refreshSystemUsage();
	bool refreshProcessInfo();
	static uint32 getCpuCounts(ZQ::common::Log* log);
	typedef struct _ProcessInfo
	{
		std::string imageName;
		uint32 processId;
		uint32 parentPid;
		uint32 cpuUse;			//the percent cpu of the process use
		uint32 physMemUse;	     //the memory of the process use (KB)
		uint32 virtualMemUse; //the virtual memory or the process use (KB)
		uint32 handleCount;
		uint32 threadCount;
	}PROCESSINFO;
	typedef std::vector<PROCESSINFO> ProcessesInfo;
	typedef struct _cpuInfo 
	{
		std::string cpuName;
		uint32	    cpuClockMHZ;
	}CPUINFO;
	typedef std::vector<CPUINFO> CPUInfo;
	typedef struct _osInfo
	{
		std::string osType;
		std::string osVersion;
	}OSINFO;
	
public:	// exports all the members
	std::string _hostName;
	int64       _osStartup;
	//std::string _cpu;
	CPUInfo _cpu;
	std::string _cpuArchitecture;
	uint32		_cpuCount;// _cpuClockMHz;
	OSINFO _os;  // http://msdn.microsoft.com/en-us/library/windows/desktop/ms724834(v=vs.85).aspx
	uint32		_memTotalPhys, _memAvailPhys;
	uint32		_memTotalVirtual, _memAvailVirtual;
	ProcessesInfo _listProcessInfo;
protected:
	processRawInfo::ProcessState _processRawData1;
	processRawInfo::ProcessState _processRawData2;
private:
	ZQ::common::Log *_pLog;
protected:
	void gatherStaticSystemInfo();
	//collect the performance processes data
	bool collectPerfData();
	//get the process info from the raw data
	bool getProcessData();
#ifdef ZQ_OS_MSWIN
protected:
//gather the raw process data through the performance data
	bool gatherProcessData(PPERF_DATA_BLOCK pPerfData);
#else
	bool isPid(std::string dirName);
	bool readPidStatus(const char* dirName,processRawInfo::RawData& processRawData);
	int  readFds(const char* dirName);
	bool readProcCpuTime(const char* dirName,processRawInfo::RawData& processRawData);
	bool readCpuTime(uint64& sysCpuTime);
#endif

};


//------------------------------
//class DeviceInfo
//------------------------------
#ifdef ZQ_OS_MSWIN
#define  MAX_IDE_DRIVES  4
#define  IDENTIFY_BUFFER_SIZE  512

#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA

#define  DFP_GET_VERSION          0x00074080
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

#define  SENDIDLENGTH  sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE
#define  FILE_DEVICE_SCSI              0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT 0x0004D008 

#ifndef SM_SERVERR2
#define SM_SERVERR2             89
#endif

#endif

class DeviceInfo
{
public:
#ifdef ZQ_OS_MSWIN
	//help struct 
	typedef struct _DRIVERSTATUS
	{
		BYTE  bDriverError;  //  Error code from driver, or 0 if no error.
		BYTE  bIDEStatus;    //  Contents of IDE Error register.
		//  Only valid when bDriverError is SMART_IDE_ERROR.
		BYTE  bReserved[2];  //  Reserved for future expansion.
		DWORD  dwReserved[2];  //  Reserved for future expansion.
	} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;
	typedef struct _SENDCMDOUTPARAMS
	{
		DWORD         cBufferSize;   //  Size of bBuffer in bytes
		DRIVERSTATUS  DriverStatus;  //  Driver status structure.
		BYTE          bBuffer[1];    //  Buffer of arbitrary length in which to store the data read from the                                                       // drive.
	} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;

	//  IDE registers
	typedef struct _IDEREGS
	{
		BYTE bFeaturesReg;       // Used for specifying SMART "commands".
		BYTE bSectorCountReg;    // IDE sector count register
		BYTE bSectorNumberReg;   // IDE sector number register
		BYTE bCylLowReg;         // IDE low order cylinder value
		BYTE bCylHighReg;        // IDE high order cylinder value
		BYTE bDriveHeadReg;      // IDE drive/head register
		BYTE bCommandReg;        // Actual IDE command.
		BYTE bReserved;          // reserved for future use.  Must be zero.
	} IDEREGS, *PIDEREGS, *LPIDEREGS;
	typedef struct _GETVERSIONOUTPARAMS
	{
		BYTE bVersion;      // Binary driver version.
		BYTE bRevision;     // Binary driver revision.
		BYTE bReserved;     // Not used.
		BYTE bIDEDeviceMap; // Bit map of IDE devices.
		DWORD fCapabilities; // Bit mask of driver capabilities.
		DWORD dwReserved[4]; // For future use.
	} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;
	typedef struct _SENDCMDINPARAMS
	{
		DWORD     cBufferSize;   //  Buffer size in bytes
		IDEREGS   irDriveRegs;   //  Structure with drive register values.
		BYTE bDriveNumber;       //  Physical drive number to send 
		//  command to (0,1,2,3).
		BYTE bReserved[3];       //  Reserved for future expansion.
		DWORD     dwReserved[4]; //  For future use.
		BYTE      bBuffer[1];    //  Input buffer.
	} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;
	typedef struct _SRB_IO_CONTROL
	{
		ULONG HeaderLength;
		UCHAR Signature[8];
		ULONG Timeout;
		ULONG ControlCode;
		ULONG ReturnCode;
		ULONG Length;
	} SRB_IO_CONTROL, *PSRB_IO_CONTROL;
	typedef struct _IDSECTOR
	{
		USHORT  wGenConfig;
		USHORT  wNumCyls;
		USHORT  wReserved;
		USHORT  wNumHeads;
		USHORT  wBytesPerTrack;
		USHORT  wBytesPerSector;
		USHORT  wSectorsPerTrack;
		USHORT  wVendorUnique[3];
		CHAR    sSerialNumber[20];
		USHORT  wBufferType;
		USHORT  wBufferSize;
		USHORT  wECCSize;
		CHAR    sFirmwareRev[8];
		CHAR    sModelNumber[40];
		USHORT  wMoreVendorUnique;
		USHORT  wDoubleWordIO;
		USHORT  wCapabilities;
		USHORT  wReserved1;
		USHORT  wPIOTiming;
		USHORT  wDMATiming;
		USHORT  wBS;
		USHORT  wNumCurrentCyls;
		USHORT  wNumCurrentHeads;
		USHORT  wNumCurrentSectorsPerTrack;
		ULONG   ulCurrentSectorCapacity;
		USHORT  wMultSectorStuff;
		ULONG   ulTotalAddressableSectors;
		USHORT  wSingleWordDMA;
		USHORT  wMultiWordDMA;
		BYTE    bReserved[128];
	} IDSECTOR, *PIDSECTOR;
#endif
	typedef struct _diskInfo
	{
		std::string  diskModel; //the model of the disk
		std::string diskSeque; //the sequence num of the disk
	//	int diskSize;  //the total size of the disk (G)  
	}DISKINFO;
	typedef std::vector<DISKINFO> DiskInfo;
	typedef struct _netCard
	{
		std::string netCardName;
		std::string macAddress;
		std::string cardDescription;
		std::vector<std::string> IPs;
	}NETCARD;
	typedef std::vector<NETCARD> NetCard;
public:
	DeviceInfo(ZQ::common::Log* log);
	~DeviceInfo();

	void gatherDiskInfo();
	void gatherNetAdapterInfo();
private:
#ifdef ZQ_OS_MSWIN
	void  ReadPhysicalDriveInNT(void);
	void  ReadIdeDriveAsScsiDriveInNT(void);
	bool  DoIDENTIFY(HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,PDWORD lpcbBytesReturned);
	void  SetInfo(int drive, DWORD diskdata [256]);
	bool  ConvertToString(DWORD diskdata [256], int firstIndex, int lastIndex,char* res);
#else
	bool getdiskInfo(const std::string& path);
#endif
public:
	DiskInfo _disk;
	NetCard _netInterfaceCard;
private:
	ZQ::common::Log *_pLog;


};
}} // namespace

#endif // __ZQ_Common_SystemInfo_H__
