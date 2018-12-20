
#include "DiskIOPerf.h"
#include "TimeUtil.h"

namespace ZQ {
namespace common{

#define PERF_CALCU(x1,x0,y1,y0,tb) (((double)((x1) - (x0))) /((y1) - (y0)) * (tb))
#define PERF_100NSCALCU(x1,x0,y1,y0) (100*((double)((x1) - (x0)) / ((y1) - (y0))))


#ifdef ZQ_OS_MSWIN
//logical disk object index
#define LOGICALDISK_INDEX		236
//phisical disk object index
#define PHYSICALDISK_INDEX		234

#define DISKINFO_READS			214//counter of reads/sec
#define DISKINFO_WRITES			216//counter of writes/sec
#define DISKINFO_READBYTES		220//counter of read bytes/sec
#define DISKINFO_WRITEBYTES		222//counter of writer bytes/sec
#define DISKINFO_AVGQUEUELEN	1400//counter of average reuest queue size
#define DISKINFO_IDLE			1482//counter of %idle time

#define TB		10000000//time base 100ns
#else

#define DISKINFO_UPTIME   	"/proc/uptime"
#define DISKINFO_DISKSTATS 	"/proc/diskstats"

#endif

DiskIOPerf::DiskIOPerf(bool bLogicalDisks)
	:_stampLastRead(0), _bLogicalDisks(bLogicalDisks)
{
}

DiskIOPerf::~DiskIOPerf()
{
}

DiskIOPerf::IoStatCounters DiskIOPerf::pollIoStats()
{
	if( readDiskstats() )
		_stampLastRead = now();

	return _lastValues;
}

DiskIOPerf::IoStatCounters DiskIOPerf::getLastCounters(int64& stampLast) const
{
	stampLast = _stampLastRead;
	return _lastValues;
}

DiskIOPerf::DiskPerfData DiskIOPerf::computeDiskStats(const IoStatCounters& oldCounters, const IoStatCounters& newCounters)
{
	IoStatCounters tmpCounters = newCounters.empty() ? pollIoStats() : newCounters;
	DiskPerfData ret;

	// do computing and put the result to ret
	IoStatCounters::const_iterator itC,itP;
	for(itC = tmpCounters.begin(); itC != tmpCounters.end(); itC++)
	{
        for( itP = oldCounters.begin(); itP != oldCounters.end(); itP++)
        {
            if( stricmp( itP->dev_name, itC->dev_name) == 0)
                break;
        }
		
		DiskPerfValue perfDatas;
		strcpy(perfDatas.dev_name, itC->dev_name);
		if(itP != oldCounters.end())
        {			
			perfDatas.rdPSec = PERF_CALCU(itC->reads, itP->reads, itC->tickCounts, itP->tickCounts, itC->tickFreq);
			perfDatas.wrPSec = PERF_CALCU(itC->writes, itP->writes, itC->tickCounts, itP->tickCounts, itC->tickFreq);
			perfDatas.rdKBPSec = PERF_CALCU(itC->readBytes, itP->readBytes, itC->tickCounts, itP->tickCounts, itC->tickFreq)/1024;
			perfDatas.wrKBPSec = PERF_CALCU(itC->writeBytes, itP->writeBytes, itC->tickCounts, itP->tickCounts, itC->tickFreq)/1024;
#ifdef ZQ_OS_MSWIN
			perfDatas.avgQueSz = PERF_CALCU(itC->reqQueueLen, itP->reqQueueLen, itC->tickCounts, itP->tickCounts, itC->tickFreq)/TB;
			double utils = double(100.00) - PERF_100NSCALCU(itC->timeUtil, itP->timeUtil, itC->perfStamp, itP->perfStamp);
			perfDatas.util = utils < 0.00 ? 0.0 : utils;
#else
			perfDatas.avgQueSz = PERF_CALCU(itC->reqQueueLen, itP->reqQueueLen, itC->tickCounts, itP->tickCounts, itC->tickFreq)/1000.0;
			perfDatas.util = PERF_CALCU(itC->timeUtil, itP->timeUtil, itC->tickCounts, itP->tickCounts, itC->tickFreq)/10.0;
#endif
		}
		else
		{
			
			perfDatas.rdPSec = PERF_CALCU(itC->reads, 0, itC->tickCounts, 0, itC->tickFreq);
			perfDatas.wrPSec = PERF_CALCU(itC->writes, 0, itC->tickCounts, 0, itC->tickFreq);
			perfDatas.rdKBPSec = PERF_CALCU(itC->readBytes, 0, itC->tickCounts, 0, itC->tickFreq)/1024;
			perfDatas.wrKBPSec = PERF_CALCU(itC->writeBytes, 0, itC->tickCounts, 0, itC->tickFreq)/1024;
#ifdef ZQ_OS_MSWIN
			perfDatas.avgQueSz = PERF_CALCU(itC->reqQueueLen, 0, itC->tickCounts, 0, itC->tickFreq)/TB;
			double utils = double(100.00) - PERF_100NSCALCU(itC->timeUtil, 0, itC->tickCounts, 0) * itC->tickFreq / TB;
			perfDatas.util = utils < 0.00 ? 0.0 : utils;
#else
			perfDatas.avgQueSz = PERF_CALCU(itC->reqQueueLen, 0, itC->tickCounts, 0, itC->tickFreq)/1000.0;
			perfDatas.util = PERF_CALCU(itC->timeUtil, 0, itC->tickCounts, 0, itC->tickFreq)/10.0;
#endif
		}

		ret.push_back(perfDatas);

	}
	return ret;
}

#ifdef ZQ_OS_MSWIN
/*****************************************************************
 *                                                               *
 * Functions used to navigate through the performance data.      *
 *                                                               *
 *****************************************************************/
PPERF_OBJECT_TYPE FirstObject( PPERF_DATA_BLOCK PerfData )
{
    return( (PPERF_OBJECT_TYPE)((PBYTE)PerfData + 
        PerfData->HeaderLength) );
}

PPERF_OBJECT_TYPE NextObject( PPERF_OBJECT_TYPE PerfObj )
{
    return( (PPERF_OBJECT_TYPE)((PBYTE)PerfObj + 
        PerfObj->TotalByteLength) );
}

PPERF_OBJECT_TYPE FindObject(PPERF_DATA_BLOCK PerfData, DWORD nObjNameTitleIndex)
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

PPERF_INSTANCE_DEFINITION FirstInstance( PPERF_OBJECT_TYPE PerfObj )
{
    return( (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj + 
        PerfObj->DefinitionLength) );
}

PPERF_INSTANCE_DEFINITION NextInstance( 
    PPERF_INSTANCE_DEFINITION PerfInst )
{
    PPERF_COUNTER_BLOCK PerfCntrBlk;

    PerfCntrBlk = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst + 
        PerfInst->ByteLength);

    return( (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfCntrBlk + 
        PerfCntrBlk->ByteLength) );
}

PPERF_COUNTER_DEFINITION FirstCounter( PPERF_OBJECT_TYPE PerfObj )
{
    return( (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj + 
        PerfObj->HeaderLength) );
}

PPERF_COUNTER_DEFINITION NextCounter( 
    PPERF_COUNTER_DEFINITION PerfCntr )
{
    return( (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr + 
        PerfCntr->ByteLength) );
}

PPERF_COUNTER_DEFINITION FindCounter(PPERF_OBJECT_TYPE PerfObj, DWORD nCounterNameTitleIndex)
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

LPCBYTE ReadPerfData(PPERF_INSTANCE_DEFINITION PerfInst, PPERF_COUNTER_DEFINITION PerfCntr)
{
    PPERF_COUNTER_BLOCK PerfCntrBlk;

    PerfCntrBlk = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst +
        PerfInst->ByteLength);

    return ((PBYTE)PerfCntrBlk + PerfCntr->CounterOffset);
}

bool DiskIOPerf::readDiskstats()
{
#define TOTALBYTES    8192
#define BYTEINCREMENT 1024

	DWORD BufferSize = TOTALBYTES;
	PPERF_DATA_BLOCK pPerfData = (PPERF_DATA_BLOCK) malloc( BufferSize );

    if( NULL == pPerfData )
       return false;
	
	int nIndex = PHYSICALDISK_INDEX;
	if( _bLogicalDisks )
		nIndex = LOGICALDISK_INDEX;

	char chIndex[10] = {0};
	sprintf(chIndex, "%d", nIndex);
    while( RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                               chIndex,
                               NULL,
                               NULL,
                               (LPBYTE) pPerfData,
                               &BufferSize ) == ERROR_MORE_DATA )
    {
		// Get a buffer that is big enough
        BufferSize += BYTEINCREMENT;
        pPerfData = (PPERF_DATA_BLOCK) realloc( pPerfData, BufferSize );
    }
	
	//some othe data
	DWORD64 perTime = pPerfData->PerfTime.QuadPart;
	DWORD64 perFreq = pPerfData->PerfFreq.QuadPart;

	PPERF_OBJECT_TYPE pLDObj = FindObject(pPerfData, nIndex);
    if(NULL == pLDObj)
	{
		free( pPerfData );
		return false;
	}

	PPERF_INSTANCE_DEFINITION pPerfInst = FirstInstance( pLDObj );
	if(pPerfInst == NULL)
	{
		free( pPerfData );
		return false;
	}

	//specified counters
	PPERF_COUNTER_DEFINITION pReads = FindCounter(pLDObj, DISKINFO_READS);
    if(NULL == pReads  || pReads->CounterType != PERF_COUNTER_COUNTER)
	{
		free( pPerfData );
		return false;
	}

	PPERF_COUNTER_DEFINITION pWrites = FindCounter(pLDObj, DISKINFO_WRITES);
	if(NULL == pWrites || pWrites->CounterType != PERF_COUNTER_COUNTER)
	{
		free( pPerfData );
		return false;
	}

	PPERF_COUNTER_DEFINITION pReadBytes = FindCounter(pLDObj, DISKINFO_READBYTES);
	if(NULL == pReadBytes || pReadBytes->CounterType != PERF_COUNTER_BULK_COUNT)
	{
		free( pPerfData );
		return false;
	}

	PPERF_COUNTER_DEFINITION pWriteBytes = FindCounter(pLDObj, DISKINFO_WRITEBYTES);
	if(NULL == pWriteBytes || pWriteBytes->CounterType != PERF_COUNTER_BULK_COUNT)
	{
		free( pPerfData );
		return false;
	}

	PPERF_COUNTER_DEFINITION pQueueLen = FindCounter(pLDObj, DISKINFO_AVGQUEUELEN);
	if(NULL == pQueueLen || pQueueLen->CounterType != PERF_COUNTER_100NS_QUEUELEN_TYPE)
	{
		free( pPerfData );
		return false;
	}

	PPERF_COUNTER_DEFINITION pIdle = FindCounter(pLDObj, DISKINFO_IDLE);
	if(NULL == pIdle || pIdle->CounterType != PERF_PRECISION_100NS_TIMER)
	{
		free( pPerfData );
		return false;
	}

	PPERF_COUNTER_DEFINITION pIdleStamp = NextCounter(pIdle);
	if(NULL == pIdleStamp || pIdleStamp->CounterType != PERF_LARGE_RAW_BASE)
	{
		free( pPerfData );
		return false;
	}
	
	//clear old value
	_stampLastRead = 0;
	_lastValues.clear();
    for(LONG k=0; k < pLDObj->NumInstances; k++ )
    {		
		//specified counters
		IoStatCounter info;
		memset(&info, 0, sizeof(IoStatCounter));

		//get counters value
		info.tickCounts = perTime;
		info.tickFreq = perFreq;
		info.reads = *(PDWORD)ReadPerfData(pPerfInst, pReads);
		info.writes = *(PDWORD)ReadPerfData(pPerfInst, pWrites);
		info.readBytes = *(PDWORD64)ReadPerfData(pPerfInst, pReadBytes);
		info.writeBytes = *(PDWORD64)ReadPerfData(pPerfInst, pWriteBytes);
		info.reqQueueLen = *(PDWORD64)ReadPerfData(pPerfInst, pQueueLen);
		info.timeUtil = *(PDWORD64)ReadPerfData(pPerfInst, pIdle);		
		info.perfStamp = *(PDWORD64)ReadPerfData(pPerfInst, pIdleStamp);
		if( pPerfInst->NameLength != 0 )
		{
			std::wstring wstrN = (LPCWSTR)((PBYTE)pPerfInst + pPerfInst->NameOffset);
			WideCharToMultiByte(CP_ACP, 0, wstrN.c_str(), wstrN.size(), info.dev_name, sizeof(info.dev_name), NULL, NULL);
		}

		_lastValues.push_back(info);
        // Get the next instance.        
		pPerfInst = NextInstance( pPerfInst );
    	
	}

    free( pPerfData );
	return true;
}


#else
bool getClockTicks(uint64& nTicks)
{
	long lticks;
	if ((lticks = sysconf(_SC_CLK_TCK)) == -1) 
		return false;
	nTicks = lticks;
	return true;
}
//read uptime to ticks
bool readUptimeToTicks(const uint64& nHZ, uint64& uptime)
{
	FILE *fp;
	char line[128];
	unsigned long up_sec, up_cent;

	if ((fp = fopen(DISKINFO_UPTIME, "r")) == NULL)
		return false;

	if (fgets(line, 128, fp) == NULL)
	{
		fclose(fp);
		return false;
	}
	sscanf(line, "%lu.%lu", &up_sec, &up_cent);
	uptime = (unsigned long long) up_sec * nHZ  + (unsigned long long) up_cent * nHZ / 100;

	fclose(fp);
	return true;
}

bool DiskIOPerf::readDiskstats()
{
	uint64 nHZ = 0;
	if( !getClockTicks(nHZ) )
		return false;

	uint64 tickCounts = 0;
	if( !readUptimeToTicks(nHZ, tickCounts) )
		return false;

	FILE *fp;
	char line[256], dev_name[128];

	unsigned long rd_ios, rd_merges_or_rd_sec, rd_ticks_or_wr_sec, wr_ios;
	unsigned long ios_pgr, tot_ticks, rq_ticks, wr_merges, wr_ticks;
	unsigned long long rd_sec, wr_sec;
	unsigned int major, minor;

	if ((fp = fopen(DISKINFO_DISKSTATS, "r")) == NULL)
		return false;
	
	//clear old value
	_stampLastRead = 0;
	_lastValues.clear();
	while (fgets(line, 256, fp) != NULL) 
	{
		memset(dev_name, 0, sizeof(dev_name));
		/* major minor name rio rmerge rsect ruse wio wmerge wsect wuse running use aveq */
		int i = sscanf(line, "%u %u %s %lu %lu %llu %lu %lu %lu %llu %lu %lu %lu %lu",
			   &major, &minor, dev_name,
			   &rd_ios, &rd_merges_or_rd_sec, &rd_sec, &rd_ticks_or_wr_sec,
			   &wr_ios, &wr_merges, &wr_sec, &wr_ticks, &ios_pgr, &tot_ticks, &rq_ticks);

		if (i == 14) 
		{
			/* Device or partition */
			if ( (!rd_ios && !wr_ios) || !strlen(dev_name))
				continue;
			
			IoStatCounter sdev;
			memset(&sdev, 0, sizeof(IoStatCounter));
			sdev.tickFreq = nHZ;
			sdev.tickCounts = tickCounts;
			strcpy(sdev.dev_name, dev_name);
			sdev.reads  = rd_ios;
			sdev.readBytes = rd_sec*512;
			sdev.writes     = wr_ios;
			sdev.writeBytes = wr_sec*512;
			sdev.reqQueueLen   = rq_ticks;
			sdev.timeUtil  = tot_ticks;			
			
			_lastValues.push_back(sdev);
		}
	}

	fclose(fp);
	return true;
}

#endif




}}


