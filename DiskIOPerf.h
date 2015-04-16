
#ifndef __DISK_IO_PERF_H__
#define __DISK_IO_PERF_H__


#include "ZQ_common_conf.h"
#include <vector>

namespace ZQ {
namespace common{


class ZQ_COMMON_API DiskIOPerf
{
public:
	DiskIOPerf(bool bLogicalDisks=false);
	~DiskIOPerf();	

typedef struct _IoStatCounter
{
	uint32  reads;
	uint32  writes;
	uint64  readBytes;
	uint64  writeBytes;
	uint64  reqQueueLen;
	uint64  timeUtil;
	uint64  perfStamp;
	uint64	tickFreq;
	uint64  tickCounts;
	char    dev_name[256];
} IoStatCounter;
typedef std::vector<IoStatCounter> IoStatCounters;

typedef struct _DiskPerfValue
{
	double	rdPSec;
	double	wrPSec;
	double	rdKBPSec;
	double	wrKBPSec;
	double	avgQueSz;
	double	util;
	char    dev_name[256];
} DiskPerfValue;

typedef std::vector< DiskPerfValue > DiskPerfData;

public:
	//get disk io performance counters value
	IoStatCounters pollIoStats();
	//get member values
	IoStatCounters getLastCounters(int64& stampLast) const;
	//compute disk io performance,if newCounters is empty it fill with current counters value
	//if oldCounters is empty the result is computed from being monitored
	DiskPerfData computeDiskStats(const IoStatCounters& oldCounters, const IoStatCounters& newCounters = IoStatCounters());

private:
	bool readDiskstats();

protected:
	IoStatCounters _lastValues;
	int64	_stampLastRead;
	bool	_bLogicalDisks;
};

}}

#endif //__DISK_IO_PERF_H__

