#include "SystemInfo.h"
#include "TimeUtil.h"
#include "SystemUtils.h"


#ifdef ZQ_OS_MSWIN
#include <WinSock2.h>
#include <IPHlpApi.h>
#else
#include <sys/utsname.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <net/if.h>  
#include <sys/ioctl.h>  
#include <arpa/inet.h>  
#include <linux/hdreg.h>
#include <sys/statfs.h>
#endif

#define MOLOG			(*_pLog)

namespace ZQ {
	namespace common {
#ifdef ZQ_OS_MSWIN
// -----------------------------
// help buffer class
// -----------------------------	
template <typename _Type>
class dynamic_buffer{
public:
	explicit dynamic_buffer(size_t size = 0)
	{
		init();
		resize(size);
	}
	~dynamic_buffer()
	{
		clear();
	}
	size_t size()
	{
		return m_size;
	}
	_Type * ptr()
	{
		return m_buffer;
	}
	operator _Type*()
	{
		return m_buffer;
	}
	void reset(const _Type &val = _Type())
	{
		for(size_t i = 0; i < m_size; ++i)
			m_buffer[i] = val;
	}
	bool resize(size_t size, bool withOldData = false)
	{
		//only grow the buffer
		if(m_size < size)
		{
			_Type* newbuf = new _Type[size];
			if(newbuf)
			{
				//restore old data
				if(withOldData)
				{
					for(size_t i = 0; i < m_size; ++i)
						newbuf[i] = m_buffer[i];
				}
				clear();
				m_buffer = newbuf;
				m_size = size;
				return true;
			} 
		}
	return false;			//m_size >= size or fail to grow memory
	}
private:
	dynamic_buffer(const dynamic_buffer&);
	dynamic_buffer& operator=(const dynamic_buffer&);
	void init(){
		m_size = 0;
		m_buffer = NULL;
	}
	void clear(){
		if(m_buffer)
			delete []m_buffer;
		init();
	}
private:
	_Type* m_buffer;
	size_t m_size;
};
//----------------------------------
//helper functions to manipulate performance data structure
//---------------------------------
//wstring to string
static void strW2strA(const std::wstring &wstr, std::string &astr)
{
	int requiredBufSize = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
	dynamic_buffer< char > namebuf;
	namebuf.resize(requiredBufSize + 1);
	namebuf.reset();
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), namebuf, (int)namebuf.size(), NULL, NULL);
	astr = namebuf.ptr();
}
// performance object
static PPERF_OBJECT_TYPE FirstObject(PPERF_DATA_BLOCK PerfData)
{
	return( (PPERF_OBJECT_TYPE)((PBYTE)PerfData + 
		PerfData->HeaderLength) );
}

static PPERF_OBJECT_TYPE NextObject(PPERF_OBJECT_TYPE PerfObj)
{
	return( (PPERF_OBJECT_TYPE)((PBYTE)PerfObj + 
		PerfObj->TotalByteLength) );
}

static PPERF_OBJECT_TYPE FindObject(PPERF_DATA_BLOCK PerfData, DWORD nObjNameTitleIndex)
{
	PPERF_OBJECT_TYPE curObj = FirstObject(PerfData);
	for(DWORD iObj = 0; iObj < PerfData->NumObjectTypes; ++iObj)
	{
		if(curObj->ObjectNameTitleIndex == nObjNameTitleIndex)
			return curObj;
		curObj = NextObject(curObj);
	}
	return NULL;
}

// performance counter
static PPERF_COUNTER_DEFINITION FirstCounter(PPERF_OBJECT_TYPE PerfObj)
{
	return( (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj + 
		PerfObj->HeaderLength) );
}

static PPERF_COUNTER_DEFINITION NextCounter(PPERF_COUNTER_DEFINITION PerfCntr)
{
	return( (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr + 
		PerfCntr->ByteLength) );
}

static PPERF_COUNTER_DEFINITION FindCounter(PPERF_OBJECT_TYPE PerfObj, DWORD nCounterNameTitleIndex)
{
	PPERF_COUNTER_DEFINITION curCounter = FirstCounter(PerfObj);
	for(DWORD iCounter = 0; iCounter < PerfObj->NumCounters; ++iCounter)
	{
		if(curCounter->CounterNameTitleIndex == nCounterNameTitleIndex)
			return curCounter;
		curCounter = NextCounter(curCounter);
	}
	return NULL;
}

// instance
static PPERF_INSTANCE_DEFINITION FirstInstance(PPERF_OBJECT_TYPE PerfObj)
{
	return( (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj + 
		PerfObj->DefinitionLength) );
}

static PPERF_INSTANCE_DEFINITION NextInstance(PPERF_INSTANCE_DEFINITION PerfInst)
{
	PPERF_COUNTER_BLOCK PerfCntrBlk;

	PerfCntrBlk = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst + 
		PerfInst->ByteLength);

	return( (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfCntrBlk + 
		PerfCntrBlk->ByteLength) );
}
static LPCBYTE ReadPerfData(PPERF_INSTANCE_DEFINITION PerfInst, PPERF_COUNTER_DEFINITION PerfCntr)
{
	PPERF_COUNTER_BLOCK PerfCntrBlk;

	PerfCntrBlk = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst +
		PerfInst->ByteLength);

	return ((PBYTE)PerfCntrBlk + PerfCntr->CounterOffset);
}
#endif
// -----------------------------
// class SystemInfo
// -----------------------------
SystemInfo::SystemInfo(ZQ::common::Log* log)
		:_pLog(log)
{
	_hostName.clear();
	_osStartup = 0;
	_cpu.clear();
	_cpuCount = 0;
	//_cpuClockMHz = 0;
	 _os.osType.clear();
	 _os.osVersion.clear();
	gatherStaticSystemInfo();
}
SystemInfo::~SystemInfo()
{
}

uint32 SystemInfo::getCpuCounts(ZQ::common::Log* log)
{
	  int cpuCount = 0;
	  SystemInfo* sys = new SystemInfo(log);
	  cpuCount = sys->_cpuCount;
	  delete sys;
	  sys = NULL;
	  if (cpuCount <= 0)
	  {
			(*log)(Log::L_ERROR, CLOGFMT(SystemInfo, "getCpuCounts() get wrong cpu counts[%d]."), cpuCount);
			return 0;
	  }
	  return cpuCount;
}

bool SystemInfo::refreshProcessInfo()
{
	if(!collectPerfData())
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo, "refreshProcessInfo() failed to collectPerfData[first]"));
		return false;
	}
	SYS::sleep(500);
	if (!collectPerfData())
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo, "refreshProcessInfo() failed to collectPerfData[second]"));
		return false;
	}
	if(!getProcessData())
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo, "refreshProcessInfo() failed to getProcessData"));
		return false;
	}
	MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo, "refreshProcessInfo() successfully"));
	return true;
}
#ifdef ZQ_OS_MSWIN
#define PERF_DATA_SIZE_BASE     (1024 * 128)
#define PERF_DATA_SIZE_INCR     (1024 * 64)
#define PERF_OBJECT_IDXSTR_PROCESS      "230"
#define PERF_OBJECT_IDX_PROCESS         230 // desc=Process
#define PERF_COUNTER_IDX_CPUTIME          6 // desc=% Processor Time
#define PERF_COUNTER_IDX_MEMUSAGE       180 // desc=Working Set
#define PERF_COUNTER_IDX_VMEMSIZE       184 // desc=Page File Bytes
#define PERF_COUNTER_IDX_THREADCOUNT    680 // desc=Thread Count
#define PERF_COUNTER_IDX_PROCESSID      784 // desc=ID Process
#define PERF_COUNTER_IDX_PARENTPID     1410 // desc=Creating Process ID
#define PERF_COUNTER_IDX_HANDLECOUNT    952 // desc=Handle Count
bool SystemInfo::collectPerfData()
{
	MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"enter collectPerfData()"));
	DWORD nBufSize = PERF_DATA_SIZE_BASE;
	dynamic_buffer< BYTE > PerfDataBuf;
	PerfDataBuf.resize(nBufSize);
	MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"CollectPerfData() init data buffer [bufsize = %u]"), PerfDataBuf.size());
	LONG nRegRet = RegQueryValueEx(
		HKEY_PERFORMANCE_DATA,
		PERF_OBJECT_IDXSTR_PROCESS,
		NULL,
		NULL,
		PerfDataBuf,
		&nBufSize
		);
	while(ERROR_MORE_DATA == nRegRet)
	{
		nBufSize += PERF_DATA_SIZE_INCR;
		PerfDataBuf.resize(nBufSize);
		MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"CollectPerfData() grow data buffer [bufsize = %u]"), PerfDataBuf.size());
		nRegRet = RegQueryValueEx(
			HKEY_PERFORMANCE_DATA,
			PERF_OBJECT_IDXSTR_PROCESS,
			NULL,
			NULL,
			PerfDataBuf,
			&nBufSize
			);
	}
	if(ERROR_SUCCESS != nRegRet)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"CollectPerfData() failed to read registry data [error = %d]"), nRegRet);
		RegCloseKey(HKEY_PERFORMANCE_DATA);
		return false;
	}
	PPERF_DATA_BLOCK PerfData = (PPERF_DATA_BLOCK)(PerfDataBuf.ptr());
	if(!gatherProcessData(PerfData))
	{
		RegCloseKey(HKEY_PERFORMANCE_DATA);
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"CollectPerfData() failed to gatherProcessData"));
		return false;
	}
	RegCloseKey(HKEY_PERFORMANCE_DATA);
	MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"leave CollectPerfData()"));
	return true;
}
bool SystemInfo::gatherProcessData(PPERF_DATA_BLOCK pPerfData)
{
	if(NULL == pPerfData)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() the pPerfData is NULL"));
		return false;
	}
	PPERF_OBJECT_TYPE pProcessObj = FindObject(pPerfData, PERF_OBJECT_IDX_PROCESS);
	if(NULL == pProcessObj)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() failed to FindObject use PERF_OBJECT_IDX_PROCESS"));
		return false;
	}
	PPERF_COUNTER_DEFINITION pCpuTimeCounter = FindCounter(pProcessObj, PERF_COUNTER_IDX_CPUTIME);
	if(NULL == pCpuTimeCounter || PERF_100NSEC_TIMER != pCpuTimeCounter->CounterType)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() failed to FindObject use PERF_COUNTER_IDX_CPUTIME"));
		return false;
	}

	PPERF_COUNTER_DEFINITION pMemUsageCounter = FindCounter(pProcessObj, PERF_COUNTER_IDX_MEMUSAGE);
	if(NULL == pMemUsageCounter || PERF_COUNTER_LARGE_RAWCOUNT != pMemUsageCounter->CounterType)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() failed to FindObject use PERF_COUNTER_IDX_MEMUSAGE"));
		return false;
	}

	PPERF_COUNTER_DEFINITION pVmemSizeCounter = FindCounter(pProcessObj, PERF_COUNTER_IDX_VMEMSIZE);
	if(NULL == pVmemSizeCounter || PERF_COUNTER_LARGE_RAWCOUNT != pVmemSizeCounter->CounterType)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() failed to FindObject use PERF_COUNTER_IDX_VMEMSIZE"));
		return false;
	}

	PPERF_COUNTER_DEFINITION pThreadCountCounter = FindCounter(pProcessObj, PERF_COUNTER_IDX_THREADCOUNT);
	if(NULL == pThreadCountCounter || PERF_COUNTER_RAWCOUNT != pThreadCountCounter->CounterType)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() failed to FindObject use PERF_COUNTER_IDX_THREADCOUNT"));
		return false;
	}
	PPERF_COUNTER_DEFINITION pProcessIdCounter = FindCounter(pProcessObj, PERF_COUNTER_IDX_PROCESSID);
	if(NULL == pProcessIdCounter || PERF_COUNTER_RAWCOUNT != pProcessIdCounter->CounterType)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() failed to FindObject use PERF_COUNTER_IDX_PROCESSID"));
		return false;
	}

	PPERF_COUNTER_DEFINITION pParentPidCounter = FindCounter(pProcessObj, PERF_COUNTER_IDX_PARENTPID);
	if(NULL == pParentPidCounter || PERF_COUNTER_RAWCOUNT != pParentPidCounter->CounterType)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() failed to FindObject use PERF_COUNTER_IDX_PARENTPID"));
		return false;
	}

	PPERF_COUNTER_DEFINITION pHandleCountCounter = FindCounter(pProcessObj, PERF_COUNTER_IDX_HANDLECOUNT);
	if(NULL == pHandleCountCounter || PERF_COUNTER_RAWCOUNT != pHandleCountCounter->CounterType)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherProcessData() failed to FindObject use PERF_COUNTER_IDX_HANDLECOUNT"));
		return false;
	}
	{
		processRawInfo::ProcessState currentState;
		PPERF_INSTANCE_DEFINITION curInstance = FirstInstance(pProcessObj);
		for(LONG iInstance = 0; iInstance < pProcessObj->NumInstances; ++iInstance)
		{
			processRawInfo::RawData processRawData;
			processRawData. initData();
			processRawData.cpuTime100nsec = *(PLARGE_INTEGER)ReadPerfData(curInstance, pCpuTimeCounter);
			processRawData.memUsageByte = *(PLARGE_INTEGER)ReadPerfData(curInstance, pMemUsageCounter);
			processRawData.vmemSizeByte = *(PLARGE_INTEGER)ReadPerfData(curInstance, pVmemSizeCounter);
			processRawData.threadCount = *(PDWORD)ReadPerfData(curInstance, pThreadCountCounter);
			processRawData.processId = *(PDWORD)ReadPerfData(curInstance, pProcessIdCounter);
			processRawData.parentPid = *(PDWORD)ReadPerfData(curInstance, pParentPidCounter);
			processRawData.handleCount = *(PDWORD)ReadPerfData(curInstance, pHandleCountCounter);
			processRawData.imageW = (LPCWSTR)((PBYTE)curInstance + curInstance->NameOffset);
			if(processRawData.imageW.size() != 0)
				currentState.processesRawData.push_back(processRawData);
			curInstance = NextInstance(curInstance);
		}

		// use the _Total's processor time as the base
		if(!currentState.processesRawData.empty())
		{
			// _Total always be the last instance
			std::vector< processRawInfo::RawData >::reverse_iterator rit_proc;
			for(rit_proc = currentState.processesRawData.rbegin(); rit_proc != currentState.processesRawData.rend(); ++rit_proc)
			{
				if(rit_proc->processId == 0 && rit_proc->imageW == L"_Total")
				{
					currentState.sysPerfTime100nsec = rit_proc->cpuTime100nsec;
					break;
				}
			}
		}
		_processRawData1.swap(_processRawData2);
		_processRawData2.swap(currentState);
	}
	MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherProcessData() gather the process data successfully"));
	return true;
}
bool SystemInfo::getProcessData()
{	
	LONGLONG sysPerfTimeInterval100nsec = _processRawData2.sysPerfTime100nsec.QuadPart - _processRawData1.sysPerfTime100nsec.QuadPart;
	if(sysPerfTimeInterval100nsec <= 0)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"getProcessData() failed to get the  cpu interval time"));
		return false;
	}
	MutexGuard g(_lkInfo);
	_listProcessInfo.clear();
	std::vector< processRawInfo::RawData >::const_iterator cit_process2nd;
	for(cit_process2nd = _processRawData2.processesRawData.begin(); cit_process2nd != _processRawData2.processesRawData.end(); ++cit_process2nd)
	{
		//find same process
		std::vector< processRawInfo::RawData >::const_iterator cit_process1st;
		for(cit_process1st = _processRawData1.processesRawData.begin(); cit_process1st != _processRawData1.processesRawData.end(); ++cit_process1st)
		{
			if(cit_process1st->processId == cit_process2nd->processId && cit_process1st->imageW == cit_process2nd->imageW)
				break; // got the process
		}
		if(cit_process1st == _processRawData1.processesRawData.end())
			continue;

		PROCESSINFO pd;
		pd.processId = cit_process2nd->processId;
		pd.parentPid = cit_process2nd->parentPid;
		pd.handleCount = cit_process2nd->handleCount;
		pd.threadCount = cit_process2nd->threadCount;
		pd.physMemUse = (uint32)(cit_process2nd->memUsageByte.QuadPart >> 10);
		pd.virtualMemUse = (uint32)(cit_process2nd->vmemSizeByte.QuadPart >> 10);

		//cpu usage
		LONGLONG curPerfTimeInterval100nsec = cit_process2nd->cpuTime100nsec.QuadPart - cit_process1st->cpuTime100nsec.QuadPart;
		pd.cpuUse = (DWORD)(100 * curPerfTimeInterval100nsec / sysPerfTimeInterval100nsec);
		//image name
		strW2strA(cit_process2nd->imageW, pd.imageName);
		MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"getProcessData() get the process[%s(%d)] CPU usage[%lld/%lld=%.2f] successfully"),pd.imageName.c_str(),pd.processId,curPerfTimeInterval100nsec,sysPerfTimeInterval100nsec,1.0*curPerfTimeInterval100nsec/sysPerfTimeInterval100nsec);
		_listProcessInfo.push_back(pd);
	}
	MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"getProcessData() get the process data successfully"));
	return true;
}
void SystemInfo::gatherStaticSystemInfo()
{
	MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"enter gatherStaticSystemInfo()"));
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(1, 1);
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to call WSAStartup so can't get the hostname"));
		_hostName = "Unknown";
	}
	else
	{
		char  szBuf[MAX_PATH] = {0};
		if (gethostname(szBuf,sizeof(szBuf) - 2) == 0)
		{
			_hostName = szBuf;
			MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the hostName[%s] successfully"),_hostName.c_str());
		}
		else
			_hostName = "Unknown";
	}
	try
	{
		//CPU info
		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);
		switch (sysinfo.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_INTEL:
			_cpuArchitecture += "X86"; break;
		case PROCESSOR_ARCHITECTURE_MIPS:
			_cpuArchitecture += "MIPS"; break;
		case PROCESSOR_ARCHITECTURE_PPC:
			_cpuArchitecture += "PowerPC"; break;
		case PROCESSOR_ARCHITECTURE_IA64:
			_cpuArchitecture += "IA64";break;
		case PROCESSOR_ARCHITECTURE_AMD64:
			_cpuArchitecture += "AMD64"; break;
		case PROCESSOR_ARCHITECTURE_ALPHA64:
			_cpuArchitecture += "ALPHA64"; break;
		case  PROCESSOR_ARCHITECTURE_ALPHA:
			_cpuArchitecture += "ALPHA"; break;
		case PROCESSOR_ARCHITECTURE_ARM:
			_cpuArchitecture += "ARM"; break;
		default:
			_cpuArchitecture += "Unknown"; break;
		}
		//  processor speed info.
		_cpuCount = sysinfo.dwNumberOfProcessors;
		MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the cpuInfo[%s] ,cpuCount[%d] successfully"),_cpuArchitecture.c_str(),_cpuCount);
		
		//get the cpu info
		HKEY hKey;
		MutexGuard g(_lkInfo);
		for (size_t i =0; i < _cpuCount; i++)
		{
			CPUINFO currentCPUInfo;
			if (ERROR_SUCCESS == ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor\\0", i, KEY_QUERY_VALUE, &hKey))
			{
				DWORD dataSize;
				char vendorBuf[MAX_PATH];
				dataSize = sizeof (vendorBuf);
				memset(vendorBuf,0,dataSize);
				if (ERROR_SUCCESS == ::RegQueryValueExA(hKey, "VendorIdentifier", NULL, NULL, (LPBYTE)vendorBuf, &dataSize))
				{
					currentCPUInfo.cpuName = std::string(" vendor=") + vendorBuf ;
					MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the vendor[%s] successfully"),vendorBuf);
				}
				else
					MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to get the vendor"));
				/*char nameBuf[MAX_PATH];
				dataSize = sizeof(nameBuf);
				memset(nameBuf,0,dataSize);
				if (ERROR_SUCCESS == ::RegQueryValueExA(hKey,"ProcessorNameString",NULL,NULL,(LPBYTE)nameBuf,&dataSize))
				{
				_cpu += std::string(" name=") + nameBuf ;
				MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the name[%s] successfully"),nameBuf);
				}
				else
				MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to get the name"));

				char idenifierBuf[MAX_PATH];
				dataSize = sizeof(idenifierBuf);
				memset(idenifierBuf,0,dataSize);
				if (ERROR_SUCCESS == ::RegQueryValueExA(hKey,"Identifier",NULL,NULL,(LPBYTE)idenifierBuf,&dataSize))
				{
				_cpu += std::string(" identifier=") + idenifierBuf ;
				MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the identifier[%s] successfully"),idenifierBuf);
				}
				else
				MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to get the identifier"));
				*/	
				DWORD data;
				dataSize = sizeof(data);
				if (ERROR_SUCCESS == ::RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE)&data, &dataSize))
				{	
					currentCPUInfo.cpuClockMHZ = data;
					MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the cpuClockMHz[%d] successfully"),data);
				}
				else
					MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to get the cpuClockMHz"));
				RegCloseKey (hKey);
				_cpu.push_back(currentCPUInfo);
			}
			else
			{
				RegCloseKey(hKey);
				MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to ::RegOpenKeyExA"));
			}
		}//for
	}//try
	catch(...){}
	//get os info
	try
	{
		// get OS startup time
		LARGE_INTEGER cntr, freq;
		bool retFreq = QueryPerformanceFrequency(&freq);
		bool retCntr = QueryPerformanceCounter(&cntr);
		if (!retFreq || !retCntr)
		{
			//QueryPerformanceFrequency or QueryPerformanceCounter fail
			MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to init the data of OS startup"));
			_osStartup = 0;
		}
		else
		{

			if(0 != freq.QuadPart)
			{
				_osStartup = ZQ::common::TimeUtil::now() - (cntr.QuadPart / freq.QuadPart * 1000);
				char timeBuffer[64];
				memset(timeBuffer, '\0', 64);
				ZQ::common::TimeUtil::TimeToUTC(_osStartup, timeBuffer, sizeof(timeBuffer) - 1);
				MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the OS startup time[%s] successfully"), timeBuffer);
			}
			else
			{
				MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo, "gatherStaticSystemInfo() failed to get performance frequency."));
				_osStartup = 0;
			}
		}
	//get os version
		OSVERSIONINFOEX osinfo;
		memset(&osinfo, 0, sizeof(osinfo));
		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		if (::GetVersionEx((OSVERSIONINFO *)&osinfo))
		{
			char osBuffer[64] = {0};
			sprintf_s(osBuffer,64,"Version %d.%d",osinfo.dwMajorVersion,osinfo.dwMinorVersion);
			//_os.assign(osBuffer);
			_os.osVersion.assign(osBuffer);
			switch(osinfo.dwMajorVersion)
			{
			case 6:
				{
					switch(osinfo.dwMinorVersion)
					{
					case 0:
						if (osinfo.wProductType == VER_NT_WORKSTATION)
							_os.osType = "Windows Vista";
						else
							_os.osType = "Windows Server 2008";
						break;
					case 1:
						if (osinfo.wProductType == VER_NT_WORKSTATION)
							_os.osType = "Windows 7";
						else
							_os.osType = "Windows Server 2008 R2";
						break;
					case 2:
						if (osinfo.wProductType == VER_NT_WORKSTATION)
							_os.osType = "Windows 8";
						else
							_os.osType = "Windows Server 2012";
						break;
					default:
						_os.osType = "Unknown";
						break;
					}
				}
				break;
			case 5:
				{
					switch(osinfo.dwMinorVersion)
					{
					case 0:
						_os.osType = "Windows 2000";
						break;
					case 1:
						_os.osType = "Windows XP";
						break;
					case 2:
						if (::GetSystemMetrics(SM_SERVERR2) == 0)
							_os.osType = "Windows Server 2003";
						else
							_os.osType = "Windows Server 2003 R2";
						break;
					default:
						_os.osType = "Unknown";
						break;
					}
				}
				break;
			default:
				_os.osType = "Unknown";
				break;
			}
		MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo()  get system version[%s] and type [%s] successfully"),_os.osVersion.c_str(), _os.osType.c_str());
		}
		else 
		{
			//GetVersionEx error
			_os.osType = "Unknown";
			_os.osVersion = "Unknown";
			MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to get system verssion and type"));
		}
	}//try
	catch (...){}
}
void SystemInfo::refreshSystemUsage()
{
	_memTotalPhys = 0;
	_memAvailPhys = 0;
	_memTotalVirtual = 0;
	_memAvailVirtual = 0;
	try
	{
		MEMORYSTATUSEX ms;
		memset(&ms, 0x00, sizeof(ms));
		ms.dwLength = sizeof(ms);
		if(!::GlobalMemoryStatusEx(&ms))
		{
			MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"refreshSystemUsage() failed to get the meminfo"));
			//GlobalMemoryStatusEx read the mesmInfo failed
			return;
		}
		_memAvailPhys = (uint32)(ms.ullAvailPhys >>10);
		_memTotalPhys = (uint32)(ms.ullTotalPhys >> 10);
		_memAvailVirtual = (uint32)(ms.ullAvailVirtual >> 10);
		_memTotalVirtual = (uint32)(ms.ullTotalVirtual >> 10);
		MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"refreshSystemUsage()  get the meminfo successfully"));
	}
	catch(...) {}
}
#else
void SystemInfo::gatherStaticSystemInfo()
{
	char  szBuf[MAX_PATH] = {0};
	if (gethostname(szBuf,sizeof(szBuf) - 2) == 0)
	{
		_hostName = szBuf;
		MOLOG(Log::L_DEBUG, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the hostName[%s] successfully"),_hostName.c_str());
	}
	else
	{
		_hostName = "Unknown";
		MOLOG(Log::L_WARNING, CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to get hostName"));
	}
	FILE* fd = fopen("/proc/cpuinfo", "r");
	if(!fd) 
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed  open[/proc/cpuinfo] to read CPU info"));
		return;
	}
	const unsigned SIZE = 1024;
	char buf[SIZE];
	//short resource = 3;
	MutexGuard g(_lkInfo);
	while(!feof(fd))
	{
		CPUINFO currentInfo;
		memset(buf, '\0', SIZE);
		fgets(buf, 1024, fd);
		char* p = strchr(buf, ':');
		if(!p) 
			continue;
		if(strstr(buf, "model name")) 
		{
			std::string cpuName = p+1;
			//_cpu = p+1;
			std::string::size_type pos = cpuName.find_last_of('\n');
			if (pos != std::string::npos)
			{
				cpuName.erase(pos);
			}
			currentInfo.cpuName = cpuName;
			//--resource;
		}
		else if(strstr(buf, "cpu MHz")) 
		{
			currentInfo.cpuClockMHZ = atoi(p+1);
			_cpu.push_back(currentInfo);
			//_cpuClockMHz = atoi(p+1);
			//--resource;
		}
		// get the cpu num 
		else if (strstr(buf,"processor"))
		{
			uint32 cpuCount = atoi(p+1);
			if (cpuCount >= _cpuCount)
				_cpuCount = cpuCount + 1;
		}
		
		//if(!resource) break;
	}//while
	fclose(fd);
	MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the CPU info successfully"));
	//get statup time	
	fd = fopen("/proc/uptime", "r");
	if(!fd)
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed open[/proc/uptime] to read system uptime."));
		return;
	}

	memset(buf, '\0', SIZE);
	fgets(buf, SIZE, fd);
	_osStartup = ZQ::common::TimeUtil::now() - atol(buf)*1000;
	fclose(fd);
	char timeBuffer[64];
	memset(timeBuffer, '\0', 64);
	ZQ::common::TimeUtil::TimeToUTC(_osStartup, timeBuffer, sizeof(timeBuffer) - 1);
	MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the system uptime [%s] successfully"), timeBuffer);
	/*fd = fopen("/etc/redhat-release", "r");
	if(fd)
	{
		memset(buf, '\0', SIZE);
		fgets(buf, SIZE, fd);
		_os.osType = buf;
		std::string::size_type pos = _os.find_last_of('\n');
		if (pos != std::string::npos)
		{
			_os.osType.erase(pos);
		}
		fclose(fd);
		MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the system type[%s] and version [%s] successfully"),_os.osType.c_str(), os.osVersion.c_str());
	}
	else
	{*/
		struct utsname n;
		int res = uname(&n);
		if(res < 0)
		{
			MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"gatherStaticSystemInfo() failed to read system name."));
			return;
		}
		_os.osType = n.nodename;
		_os.osVersion = n.release;
		_cpuArchitecture += n.machine;
		MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"gatherStaticSystemInfo() get the system type [%s] version [%s]  and cpuArchitecture[%s] successfully"),_os.osType.c_str(), _os.osVersion.c_str(), _cpuArchitecture.c_str());
	//}
}

void SystemInfo::refreshSystemUsage()
{
	_memTotalPhys = 0;
	_memAvailPhys = 0;
	_memTotalVirtual = 0;
	_memAvailVirtual = 0;
	int fd = open("/proc/meminfo",O_RDONLY);
	if(fd < 0) 
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"refreshSystemUsage() failed open[/proc/meminfo] to read meminfo"));
		return;
	}
	char buf[1024];
	memset(buf, '\0', 1024);
	ssize_t bytes = read(fd, buf, sizeof(buf)-1);
	buf[bytes] = '\0';
	close(fd);

	char* pos = strstr(buf, "MemTotal");
	unsigned total = 0;
	if(pos) {
		pos += 9;
		sscanf(pos, "%u", &total);
	}

	pos = strstr(pos, "MemFree");
	unsigned free = 0;
	if(pos) {
		pos += 8;
		sscanf(pos, "%u", &free);
	}

	pos = strstr(pos, "Buffers");
	unsigned buffers = 0;
	if(pos) {
		pos += 8;
		sscanf(pos, "%u", &buffers);
	}

	pos = strstr(pos, "Cached");
	unsigned cached = 0;
	if(pos) {
		pos += 7;
		sscanf(pos, "%u", &cached);
	}

	pos = strstr(pos, "SwapTotal");
	unsigned vmTotal = 0;
	if(pos) {
		pos += 10;
		sscanf(pos, "%u", &vmTotal);
	}

	pos = strstr(pos, "SwapFree");
	unsigned vmFree = 0;
	if(pos) {
		pos += 12;
		sscanf(pos, "%u", &vmFree);
	}

	_memAvailPhys = (free+buffers+cached);
	_memTotalPhys = total; 
	_memAvailVirtual = vmFree;
	_memTotalVirtual = vmTotal;
	MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"refreshSystemUsage() read meminfo successfully"));
}
bool SystemInfo::readPidStatus(const char* dirName,processRawInfo::RawData& processRawData)
{
	char proPath[128];
	memset(proPath,'\0',128);
	//read the process info 
	snprintf(proPath,sizeof(proPath),"/proc/%s/status" ,dirName);
	FILE* fd = fopen(proPath, "r");
	if(!fd) 
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readPidStatus() failed to open[%s] to read proccess status"),proPath);
	//	fclose(fd);
		return false;
	}
	processRawData.processId = atoi(dirName);
	short resource = 5;
	const unsigned SIZE = 1024;
	char buf[SIZE];
	memset(buf, '\0', SIZE);
	while(!feof(fd))
	{
		fgets(buf, 1024, fd);
		char* p = strchr(buf, ':');
		if(!p) 
			continue;
		if(strstr(buf, "Name")) 
		{
			processRawData.imageName = p+1;
			if(!processRawData.imageName.empty())
			{
				//processRawData.imageName=processRawData.imageName.substr(0,processRawData.imageName.length()-1);
				std::string::size_type pos = processRawData.imageName.find_last_of('\n');
				if (pos != std::string::npos)
				{
					processRawData.imageName.erase(pos);
				}
			//	MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readPidStatus() get the process[%s] image name[%s] successfully"),dirName,processRawData.imageName.c_str());
			}
			--resource;
		}
		else if(strstr(buf, "PPid"))
		{
			processRawData.parentPid = atoi(p+1);
			//MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readPidStatus() get the process[%s] parent ID[%d] successfully"),dirName,processRawData.parentPid);
			--resource;
		}
		else if(strstr(buf, "Threads"))
		{
			processRawData.threadCount = atoi(p+1);
			//MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readPidStatus() get the process[%s] threadCount[%d] successfully"),dirName,processRawData.threadCount);
			--resource;
		}
		else if(strstr(buf, "VmSize"))
		{
			processRawData.vmemSizeByte = atoi(p+1);
			//MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readPidStatus() get the process[%s] vmemSizeByte[%d] successfully"),dirName,processRawData.vmemSizeByte);
			--resource;
		}else if(strstr(buf, "VmRSS"))
		{
			processRawData.memUsageByte = atoi(p+1);
			//MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readPidStatus() get the process[%s] memUsageByte[%d] successfully"),dirName,processRawData.memUsageByte);
			--resource;
		}
		if (!resource) break;
	}  // end of while(!feof(fd))
	fclose(fd);
	MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readPidStatus() get the process[PID:%s Name:%s PPID:%d Threads:%d VmemSize:%d,MemSize:%d] successfully"),dirName,processRawData.imageName.c_str(),processRawData.parentPid,processRawData.threadCount,processRawData.vmemSizeByte,processRawData.memUsageByte);
	return true;
}
int SystemInfo::readFds(const char* dirName)
{
	int fdCount = 0;
	char proPath[128];
	memset(proPath,'\0',128);
	snprintf(proPath,sizeof(proPath),"/proc/%s/fd" ,dirName);
	DIR* fdDir = opendir(proPath); 
	if (NULL == fdDir)
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystermInfo,"readFds() failed open[%s] to read fdcount of the process[%s]"),proPath,dirName);
//		closedir(fdDir);
		return -1;
	}
	else
	{
		struct dirent*  processFd=readdir(fdDir);
		while(processFd != NULL)
		{
			if (DT_LNK == processFd->d_type)
				fdCount ++;
			processFd=readdir(fdDir);
		}
	}
	closedir(fdDir);
	MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readFds() read the fdcount[%d] of process[%s] successfully"),fdCount,dirName);
	return fdCount;
}
bool SystemInfo::readProcCpuTime(const char* dirName,processRawInfo::RawData& processRawData)
{
	char proPath[128];
	memset(proPath,'\0',128);
	snprintf(proPath,sizeof(proPath),"/proc/%s/stat" ,dirName);
	FILE* fd = fopen(proPath, "r");
	if(!fd) 
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readProcCpuTime() failed open[%s] to read cputime of process[%s]"),proPath,dirName);
	//	fclose(fd);
		return false;
	}
	char cpuUseBuf[1024];
	memset(cpuUseBuf,'\0',1024);
	if (fgets(cpuUseBuf,sizeof(cpuUseBuf),fd) != NULL)
	{
		char * t = strchr(cpuUseBuf,')');
		if(NULL != t)
		{
			char pState;
			long long  temp = 0,utime = 0,stime = 0,cutime = 0,cstime = 0;
			int ret = sscanf(t+2,"%c  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",&pState,&temp,&temp,&temp,&temp,&temp,&temp,&temp,&temp,&temp,&temp,&utime,&stime,&cutime,&cstime);
			/*1       2       3      4    5     6     7       8      9    10    11    12     13  14    15	    1           2         3           4         5           6         7         8           9         10         11          12       13         14          15 */           
			if(15 == ret)
			{
				processRawData.cpuTime100nsec = utime+stime+cutime+cstime;
				MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readProcCpuTime()  read cputime[%llu] of process[%s] successfully"),(long long)processRawData.cpuTime100nsec,dirName);
			}
			else
				MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readProcCpuTime()  failed to read cputime of process[%s] "),dirName);
		}
		else
		{
			MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readProcCpuTime()  failed get  valid data from[%s] of the process[%s]"),proPath,dirName);
			fclose(fd);
			return false;
		}

	}
	else
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readProcCpuTime()  failed get  data from[%s] of the process[%s]"),proPath,dirName);
		fclose(fd);
		return false;
	}
	fclose(fd);
	return true;
}
bool SystemInfo::readCpuTime(uint64& sysCpuTime)
{
	FILE* fd = fopen("/proc/stat", "r");
	if(!fd) 
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readCpuTime() failed open[/proc/stat] to read CPU info"));
		//fclose(fd);
		return false;
	}
	char cpuInfoBuf[1024];
	memset(cpuInfoBuf,'\0',1024);
	if (fgets(cpuInfoBuf,sizeof(cpuInfoBuf),fd) != NULL)
	{
		long long  userTime = 0, niceTime =0, systemTime=0, idleTtime = 0, iowaitTime= 0, irqTime = 0, softirqTime = 0, stealTime = 0, guestTime =0;
		char * t = strstr(cpuInfoBuf, "cpu");
		if (t != NULL)
		{
			int ret = sscanf(t+3,"%llu %llu %llu %llu %llu %llu %llu %llu %llu",&userTime,&niceTime,&systemTime,&idleTtime,&iowaitTime,&irqTime,&softirqTime,&stealTime,&guestTime);
			if (ret != 9)
			{
				//_log();
				fclose(fd);
				MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readCpuTime() failed to read CPU time of system"));
				return false;
			}
			sysCpuTime =  userTime +niceTime + systemTime + idleTtime + iowaitTime + irqTime +softirqTime + stealTime + guestTime;
			MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"readCpuTime() read the system CPU time[%llu] successfully"),(long long)sysCpuTime);
		}
		else
		{
			fclose(fd);
			MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readCpuTime()  failed get valid data from[/proc/stat]"));
			return false;
		}
	}
	else
	{
		fclose(fd);
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"readCpuTime()  failed get  data from[/proc/stat]"));
		return false;
	}
	fclose(fd);
	return true;
}
bool SystemInfo::collectPerfData()
{
	processRawInfo::ProcessState currentProcessState;
	DIR* procDir = opendir("/proc");
	if (NULL == procDir)
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"collectPerfData() failed open[/proc] to get processID"));
		//closedir(procDir);
		return false;
	}
	else
	{
		struct dirent*  processDir=readdir(procDir);
		while(processDir != NULL)
		{
			processRawInfo::RawData processRawData;
			processRawData.initData();
			if (DT_DIR == processDir->d_type && isPid(processDir->d_name))
			{
				//read process info from "/proc/pid/status"
				if (!readPidStatus(processDir->d_name,processRawData))
				{
					MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"collectPerfData() failed read status of  process[%s]"),processDir->d_name);
					processDir=readdir(procDir);
					continue;
				}
				//read fd num from "/proc/pid/fd"
				int retReadFd = readFds(processDir->d_name);
				if(retReadFd == -1)
				{
					MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"collectPerfData() failed read fds of  process[%s]"),processDir->d_name);
					processDir=readdir(procDir);
					continue;
				}
				processRawData.handleCount = retReadFd;
				//read the process use cpu info from "/proc/%s/stat"
				if (!readProcCpuTime(processDir->d_name,processRawData))
				{
					MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"collectPerfData() failed read CPU time of  process[%s]"),processDir->d_name);
					processDir=readdir(procDir);
					continue;
				}
			}//end of if(DT_DIR == processDir->d_type && isPid(processDir->d_name))
			if(processRawData.imageName.size() !=0)
				currentProcessState.processesRawData.push_back(processRawData);
			processDir=readdir(procDir);
		} //end of while  (processDir != NULL)
		closedir(procDir);
	}//end of else (NULL != procDir)

	//read the cpu time
	if (!readCpuTime(currentProcessState.sysPerfTime100nsec))
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"collectPerfData() failed read CPU time of system"));
		return false;
	}
	_processRawData1.swap(_processRawData2);
	_processRawData2.swap(currentProcessState);
	return true;
}
bool SystemInfo::getProcessData()
{
	if (_processRawData1.sysPerfTime100nsec <=0 || _processRawData2.sysPerfTime100nsec <= 0)
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"getProcessData() the invalid data of process's using of CPU"));
		return false;
	}
	long long sysPerfTimeInterval100nsec = _processRawData2.sysPerfTime100nsec - _processRawData1.sysPerfTime100nsec;
	if (sysPerfTimeInterval100nsec <= 0)
	{
		MOLOG(Log::L_WARNING,CLOGFMT(SystemInfo,"getProcessData() the invalid data of process's using of CPU"));
		return false;
	}
	MutexGuard g(_lkInfo);
	_listProcessInfo.clear();
	std::vector< processRawInfo::RawData >::const_iterator cit_process2nd;
	for(cit_process2nd = _processRawData2.processesRawData.begin(); cit_process2nd != _processRawData2.processesRawData.end(); ++cit_process2nd)
	{
		//find same process
		std::vector< processRawInfo::RawData >::const_iterator cit_process1st;
		for(cit_process1st = _processRawData1.processesRawData.begin(); cit_process1st != _processRawData1.processesRawData.end(); ++cit_process1st)
		{
			if(cit_process1st->processId == cit_process2nd->processId && cit_process1st->imageName== cit_process2nd->imageName)
				break; // got the process
		}
		if(cit_process1st == _processRawData1.processesRawData.end())
			continue;
		PROCESSINFO pd;
		pd.imageName = cit_process2nd->imageName;
		pd.processId = cit_process2nd->processId;
		pd.parentPid = cit_process2nd->parentPid;
		pd.handleCount = cit_process2nd->handleCount;
		pd.threadCount = cit_process2nd->threadCount;
		pd.physMemUse = cit_process2nd->memUsageByte;
		pd.virtualMemUse = cit_process2nd->vmemSizeByte;

		//cpu usage
		long long curPerfTimeInterval100nsec = cit_process2nd->cpuTime100nsec- cit_process1st->cpuTime100nsec;
 		pd.cpuUse = (uint32)(100 * curPerfTimeInterval100nsec / sysPerfTimeInterval100nsec);
		MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"getProcessData() get the process[%s(%d)] CPU usage[%lld/%lld=%.2f] successfully"),pd.imageName.c_str(),pd.processId,curPerfTimeInterval100nsec,sysPerfTimeInterval100nsec,1.0*curPerfTimeInterval100nsec/sysPerfTimeInterval100nsec);
		_listProcessInfo.push_back(pd);
	}
	MOLOG(Log::L_DEBUG,CLOGFMT(SystemInfo,"getProcessData() get the process data successfully"));
	return true;
}
bool SystemInfo::isPid(std::string dirName)
{
	std::string::iterator sIter = dirName.begin();
	for (; sIter != dirName.end() ; sIter++)
	{
		if (*sIter < '0' || *sIter >'9')
		{
			return false;
		}
	}
	return true;
}
#endif


//-----------------------
//class deviceInfo
//-----------------------
DeviceInfo::DeviceInfo(ZQ::common::Log* log)
:_pLog(log)
{
	gatherNetAdapterInfo();
#ifdef _DELETDISK	
	gatherDiskInfo();
#endif
}
DeviceInfo::~DeviceInfo()
{
}
#ifdef ZQ_OS_MSWIN
void DeviceInfo::gatherDiskInfo()
{
	MutexGuard g(_lkInfo);
	_disks.clear();
	ReadPhysicalDriveInNT();
	//ReadIdeDriveAsScsiDriveInNT();
}
void DeviceInfo::ReadPhysicalDriveInNT()
{
	for (int drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		HANDLE hPhysicalDriveIOCTL = 0;
		char driveName [256];
		snprintf (driveName, 256,"\\\\.\\PhysicalDrive%d", drive);
		hPhysicalDriveIOCTL = CreateFile (driveName,
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
		if (hPhysicalDriveIOCTL == NULL)
		{
			MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"ReadPhysicalDriveInNT() unable to open physical drive %d, error code: 0x%lX"),drive, GetLastError());
			CloseHandle (hPhysicalDriveIOCTL);
			continue;
		}
		else
		{
			GETVERSIONOUTPARAMS VersionParams;
			DWORD               cbBytesReturned = 0;
			// Get the version, etc of PhysicalDrive IOCTL
			memset ((void*) &VersionParams, 0, sizeof(VersionParams));
			if ( !DeviceIoControl (hPhysicalDriveIOCTL, DFP_GET_VERSION,NULL, 0,&VersionParams,sizeof(VersionParams),&cbBytesReturned, NULL) )
			{         
			//	MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"ReadPhysicalDriveInNT() DeviceIoControl return error for disk [%d] ,with error code 0x%lX"), drive,GetLastError ());
				CloseHandle (hPhysicalDriveIOCTL); 
				continue;
			}

			if (VersionParams.bIDEDeviceMap > 0)
			{
				BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
				SENDCMDINPARAMS  scip;
				//SENDCMDOUTPARAMS OutCmd;
				BYTE IdOutCmd [sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
				bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;
				memset (&scip, 0, sizeof(scip));
				memset (IdOutCmd, 0, sizeof(IdOutCmd));
				if ( DoIDENTIFY (hPhysicalDriveIOCTL, 
					&scip, 
					(PSENDCMDOUTPARAMS)&IdOutCmd, 
					(BYTE) bIDCmd,
					(BYTE) drive,
					&cbBytesReturned))
				{
					DWORD diskdata [256];
					USHORT *pIdSector = (USHORT *)
						((PSENDCMDOUTPARAMS) IdOutCmd) -> bBuffer;
					for (int p = 0; p < 256; p++)
						diskdata [p] = pIdSector [p];
					SetInfo(drive, diskdata);
				}
			}//if (VersionParams.bIDEDeviceMap > 0)
		}//else (hPhysicalDriveIOCTL != NULL)
		 CloseHandle (hPhysicalDriveIOCTL);
	}//for (int drive = 0; drive < MAX_IDE_DRIVES; drive++)
}

void DeviceInfo::ReadIdeDriveAsScsiDriveInNT()
{
	for (int controller = 0; controller < 2; controller++)
	{
		HANDLE hScsiDriveIOCTL = 0;
		char   driveName [256];
		memset(driveName,'\0',256);
		snprintf (driveName, 256,"\\\\.\\Scsi%d:", controller);
		hScsiDriveIOCTL = CreateFile (driveName,
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
		if (hScsiDriveIOCTL == INVALID_HANDLE_VALUE)
		{
			MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"ReadIdeDriveAsScsiDriveInNT() unable to open physical drive %d, error code: 0x%lX\n"),controller, GetLastError ());
			CloseHandle (hScsiDriveIOCTL);
			continue;
		}
		else
		{
			for (int drive = 0; drive < 2; drive++)
			{
				char buffer [sizeof (SRB_IO_CONTROL) + SENDIDLENGTH];
				SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) buffer;
				SENDCMDINPARAMS *pin = (SENDCMDINPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
				DWORD dummy;
				memset (buffer, 0, sizeof (buffer));
				p -> HeaderLength = sizeof (SRB_IO_CONTROL);
				p -> Timeout = 10000;
				p -> Length = SENDIDLENGTH;
				p -> ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
				strncpy ((char *) p -> Signature, "SCSIDISK", 8);
				pin -> irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
				pin -> bDriveNumber = drive;
				if (DeviceIoControl (hScsiDriveIOCTL, IOCTL_SCSI_MINIPORT, buffer,sizeof (SRB_IO_CONTROL) +sizeof (SENDCMDINPARAMS) - 1,buffer,sizeof (SRB_IO_CONTROL) + SENDIDLENGTH,&dummy, NULL))
				{
					SENDCMDOUTPARAMS *pOut =(SENDCMDOUTPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
					IDSECTOR *pId = (IDSECTOR *) (pOut -> bBuffer);
					if (pId -> sModelNumber [0])
					{
						DWORD diskdata [256];
						USHORT *pIdSector = (USHORT *) pId;
						for (int p = 0; p < 256; p++)
							diskdata [p] = pIdSector [p];
						SetInfo(controller * 2 + drive, diskdata);
					}
				}
			}//for (drive = 0; drive < 2; drive++)
		}//else  (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
		CloseHandle (hScsiDriveIOCTL);
	}//for (int controller = 0; controller < 2; controller++)
}

bool DeviceInfo::DoIDENTIFY(HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,PDWORD lpcbBytesReturned)
{
	// Set up data structures for IDENTIFY command.
	pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
	pSCIP -> irDriveRegs.bFeaturesReg = 0;
	pSCIP -> irDriveRegs.bSectorCountReg = 1;
	pSCIP -> irDriveRegs.bSectorNumberReg = 1;
	pSCIP -> irDriveRegs.bCylLowReg = 0;
	pSCIP -> irDriveRegs.bCylHighReg = 0;
	// Compute the drive number.
	pSCIP -> irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);
	// The command can either be IDE identify or ATAPI identify.
	pSCIP -> irDriveRegs.bCommandReg = bIDCmd;
	pSCIP -> bDriveNumber = bDriveNum;
	pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
	return( DeviceIoControl (hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA,
		(LPVOID) pSCIP,
		sizeof(SENDCMDINPARAMS) - 1,
		(LPVOID) pSCOP,
		sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
		lpcbBytesReturned, NULL) );
}

bool DeviceInfo::ConvertToString(DWORD diskdata [256], int firstIndex, int lastIndex,char* string)
{
	int index = 0;
	int position = 0;
	//  each integer has two characters stored in it backwards
	for (index = firstIndex; index <= lastIndex; index++)
	{
		//  get high byte for 1st character
		string [position] = (char) (diskdata [index] / 256);
		position++;
		//  get low byte for 2nd character
		string [position] = (char) (diskdata [index] % 256);
		position++;
	}
	//  end the string 
	string [position] = '\0';
	//  cut off the trailing blanks
	for (index = position - 1; index > 0 && ' ' == string [index]; index--)
		string [index] = '\0';
	return true;

}

void DeviceInfo::SetInfo(int drive, DWORD diskdata [256])
{
	char buffer[1024];
	memset(buffer,'\0',1024);
	ConvertToString (diskdata, 0, 100,buffer);
	char diskId[32];
	memset(diskId,'\0',32);
	for(int i = 0; i<32; i++)
	{
		if (buffer[i+32] == 0)
			break;
		diskId[i] = buffer[i+32];
	}
	char diskModel[32];
	memset(diskModel,'\0',32);
	for (int i = 0; i < 32; i++)
	{
		if (buffer[i + 54] == 0 || buffer[i + 54] == ' ')
			break;
		diskModel[i] = buffer[i + 54];
	}
	{	
		MutexGuard g(_lkInfo);
		DiskInfo theDisk;
		theDisk.diskSeque.assign(diskId);
		theDisk.diskModel.assign(diskModel);
		_disks.push_back(theDisk);
	}
	MOLOG(Log::L_INFO, CLOGFMT(DeviceInfo,"get the disk id[%s] and model [%s] successful"),diskId, diskModel);
     return;
}

void DeviceInfo::gatherNetAdapterInfo()
{
	// get the net interface card info
	PIP_ADAPTER_INFO pAdapterInfo = NULL;  
	ULONG uLen = 0; 

	//allocate the memory buffer for PIP_ADAPTER_INFOs
	::GetAdaptersInfo(pAdapterInfo, &uLen);  
	pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, uLen);
	if (NULL == pAdapterInfo)
		return;

	do {
		// get the infomation of the local NIC
		DWORD dwRes = ::GetAdaptersInfo(pAdapterInfo, &uLen);  
		if(ERROR_SUCCESS != dwRes) 
			break;

		ZQ::common::MutexGuard g(_lkInfo);
		_NICs.clear();

		for (; pAdapterInfo; pAdapterInfo = pAdapterInfo->Next)
		{
			NicInfo netCard;
			netCard.netCardName.assign(pAdapterInfo->AdapterName);
			netCard.cardDescription.assign(pAdapterInfo->Description);

			// the MAC address
			uint32 macLen = pAdapterInfo->AddressLength;
			char buffer[128]="";
			for (uint i =0; i < macLen; i++)
				snprintf(buffer+3*i, sizeof(buffer) -3*i -2,"%02X-", pAdapterInfo->Address[i]);
			macLen = strlen(buffer);
			if (macLen >3)
				netCard.macAddress.assign(buffer, macLen-1);

			// the IP address
			for (IP_ADDR_STRING *pAddrString = &(pAdapterInfo->IpAddressList); pAddrString; pAddrString = pAddrString->Next)
			{  
				//snprintf(buffer, sizeof(buffer)-2, "%s", pAddrString->IpAddress.String);
				//std::string ipAdd(buffer);
				netCard.IPs.push_back(pAddrString->IpAddress.String);
				netCard.IpMasks.push_back(pAddrString->IpMask.String);
			}  
			_NICs.push_back(netCard);
			MOLOG(Log::L_DEBUG, CLOGFMT(DeviceInfo,"got info of NIC[%s]: mac[%s] %s"), netCard.netCardName.c_str(), netCard.macAddress.c_str(), netCard.cardDescription.c_str());
		}
	} while(0);

	GlobalFree(pAdapterInfo);
}

#else
void DeviceInfo::gatherNetAdapterInfo()
{
	int fd;  
	int interfaceNum = 0;  

	struct ifreq buf[16];  
	struct ifconf ifc;  
	//struct ifreq ifrcopy;  

	char mac[64];
	memset(mac,'\0',64);
	char ip[32];
	memset(ip,'\0',32);
	char broadAddr[32];
	memset(broadAddr,'\0',32);
	char subnetMask[32];
	memset(subnetMask,'\0',32); 

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  
	{  
		MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"gatherNetAdapterInfo() create socket failed"));
		close(fd);
		return ;
	}
	 ifc.ifc_len = sizeof(buf);
	 ifc.ifc_buf = (caddr_t)buf;
	 if (0 != ioctl(fd, SIOCGIFCONF, (char *)&ifc))
		MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"gatherNetAdapterInfo() list NICs failed"));
	 else
	 {	
		 MutexGuard g(_lkInfo);
		 interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
		 while (interfaceNum-- > 0)
		 {
			 NicInfo theCard;
			 theCard.netCardName = buf[interfaceNum].ifr_name;
			 if (0 != ioctl(0 != fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum])))
			 {
				 MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"gatherNetAdapterInfo() get the mac of NIC failed"));
				 continue;
			 }

			 memset(mac, '\0', sizeof(mac));
			 snprintf(mac, sizeof(mac), "%02x-%02x-%02x-%02x-%02x-%02x",  
				 (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],  
				 (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],  
				 (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],  
				 (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],  
				 (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],  
				 (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
			 theCard.macAddress = mac;

			 //get the IP of this interface  
			 if (0 != ioctl(fd, SIOCGIFADDR, (char *)&buf[interfaceNum]))  
			 {  
				 MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"gatherNetAdapterInfo() get the ip NIC failed"));
				 continue;		  
			 }

			 memset(ip,'\0',32);
			 snprintf(ip, sizeof(ip), "%s", (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_addr))->sin_addr));  
			 std::string strIp = std::string(ip);
			 theCard.IPs.push_back(strIp);

			 _NICs.push_back(theCard);
			 MOLOG(Log::L_INFO, CLOGFMT(DeviceInfo,"gatherNetAdapterInfo() get the NICInfo successful decription[%s] mac[%s] name[%s] "),theCard.cardDescription.c_str(), theCard.macAddress.c_str(), theCard.netCardName.c_str());
		 }//while
	 }

	 close(fd); 
	 return;
}
bool DeviceInfo::getdiskInfo(const std::string& path)
{
	DiskInfo currentDisk;
	currentDisk.diskSeque.clear();
	int fd;
	struct hd_driveid hid = {0};
	fd = open (path.c_str(), O_RDONLY);
	if (fd < 0)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"getdiskInfo() open file %s failed"),path.c_str());
		return false;
	}

	if (ioctl(fd, HDIO_GET_IDENTITY, &hid) < 0)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"getdiskInfo() read the diskId of [%s]failed with error[%d]"),path.c_str(), errno);
		return false;
	}

	close (fd);
	char buffer[128];
	memset(buffer,'\0',128);
	snprintf(buffer, 128,"%s", hid.serial_no);
	std::string diskSeque(buffer);
	size_t pos = diskSeque.find_first_not_of(' ');
	if(pos != std::string::npos)
		diskSeque.erase(0,pos);
	pos = diskSeque.find_first_of(' ');
	if(pos != std::string::npos)
		diskSeque.erase(pos);
	currentDisk.diskSeque = diskSeque;//.assign(buffer);
	memset(buffer,'\0',128);
	snprintf(buffer, 128,"%s", hid.model);
	std::string diskModel(buffer);
	pos = diskModel.find_first_not_of(' ');
	if(pos != std::string::npos)
		diskModel.erase(0,pos);
	pos = diskModel.find_first_of(' ');
	if(pos != std::string::npos)
		diskModel.erase(pos);
	currentDisk.diskModel = diskModel;//.assign(buffer);
	{
		MutexGuard g(_lkInfo);
		_disks.push_back(currentDisk);
	}
	MOLOG(Log::L_INFO, CLOGFMT(DeviceInfo,"getdiskInfo() get the disk path[%s] id[%s] and model [%s] successful"),path.c_str(),currentDisk.diskSeque.c_str(), currentDisk.diskModel.c_str());
	return true;
}

void DeviceInfo::gatherDiskInfo()
{
	DIR *dp;
	struct dirent *entry;
	if((dp = opendir("/dev/disk/by-path/")) == NULL)
	{
		MOLOG(Log::L_WARNING, CLOGFMT(DeviceInfo,"gatherDiskInfo() open file dir [/dev/disk/by-path/] failed"));
		return;
	}

	MutexGuard g(_lkInfo);
	_disks.clear();
	while((entry = readdir(dp)) != NULL)
	{
		if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0)
			continue;
		std::string devPath = std::string(entry->d_name);
		if (devPath.find("part") == std::string::npos)
		{
			devPath = "/dev/disk/by-path/" + devPath;
			getdiskInfo(devPath);
		}
	}

	closedir(dp);
	return ;
}

#endif
	}//common
}//ZQ