// this file is converted from .MIB file by mibparse.py
// ====================================================

// -------------------------------------
// service bcastChannel oid[1900]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_bcastChannel[] = {
	{".2", "bcastChApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).bcastChannel(1900).bcastChApp(2)
	{ NULL, NULL } };

// -------------------------------------
// service c2fe oid[2500]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_c2fe[] = {
	{".2", "c2feApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).c2fe(2500).c2feApp(2)
	{".2.1", "c2feAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).c2fe(2500).c2feApp(2).c2feAttr(1)
	{ NULL, NULL } };

// -------------------------------------
// service cdnCS oid[2000]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_cdnCS[] = {
	{".2", "cdnCSSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnCS(2000).cdnCSSvcApp(2)
	{ NULL, NULL } };

// -------------------------------------
// service cdnss oid[2100]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_cdnss[] = {
	{".2", "cdnssSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2)
	{".2.1", "cdnssAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1)
	{".2.1.100", "hotContentsTbl" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).hotContentsTbl(100)
	{".2.1.100.1", "hotContent" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).hotContentsTbl(100).hotContent(1)
	{".2.1.100.1.1", "hcContentName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).hotContentsTbl(100).hotContent(1).hcContentName(1)
	{".2.1.100.1.2", "hcAccessCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).hotContentsTbl(100).hotContent(1).hcAccessCount(2)
	{".2.1.100.1.3", "hcStampSince" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).hotContentsTbl(100).hotContent(1).hcStampSince(3)
	{".2.1.100.1.4", "hcStampLatest" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).hotContentsTbl(100).hotContent(1).hcStampLatest(4)
	{".2.1.101", "missedContentTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).missedContentTable(101)
	{".2.1.101.1", "missedContent" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).missedContentTable(101).missedContent(1)
	{".2.1.101.1.1", "mcContentName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).missedContentTable(101).missedContent(1).mcContentName(1)
	{".2.1.101.1.2", "mcAccessCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).missedContentTable(101).missedContent(1).mcAccessCount(2)
	{".2.1.101.1.3", "mcStampSince" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).missedContentTable(101).missedContent(1).mcStampSince(3)
	{".2.1.101.1.4", "mcStampLatest" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).missedContentTable(101).missedContent(1).mcStampLatest(4)
	{".2.1.102", "cacheStoreNeighborTbl" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102)
	{".2.1.102.1", "cacheStoreNeighbor" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1)
	{".2.1.102.1.1", "csnNetId" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1).csnNetId(1)
	{".2.1.102.1.2", "csnState" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1).csnState(2)
	{".2.1.102.1.3", "csnLoad" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1).csnLoad(3)
	{".2.1.102.1.4", "csnLoadImport" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1).csnLoadImport(4)
	{".2.1.102.1.5", "csnLoadCacheWrite" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1).csnLoadCacheWrite(5)
	{".2.1.102.1.6", "csnStampAsOf" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1).csnStampAsOf(6)
	{".2.1.102.1.7", "csnEndpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1).csnEndpoint(7)
	{".2.1.102.1.8", "csnSessionInterface" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreNeighborTbl(102).cacheStoreNeighbor(1).csnSessionInterface(8)
	{".2.1.103", "cacheStoreStreamCounters" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreStreamCounters(103)
	{".2.1.103.1", "cacheStoreStreamCtr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreStreamCounters(103).cacheStoreStreamCtr(1)
	{".2.1.103.1.1", "scrName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreStreamCounters(103).cacheStoreStreamCtr(1).scrName(1)
	{".2.1.103.1.2", "scrCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreStreamCounters(103).cacheStoreStreamCtr(1).scrCount(2)
	{".2.1.103.1.3", "scrFailCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreStreamCounters(103).cacheStoreStreamCtr(1).scrFailCount(3)
	{".2.1.103.1.4", "scrLatencyAvg" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreStreamCounters(103).cacheStoreStreamCtr(1).scrLatencyAvg(4)
	{".2.1.103.1.5", "scrLatencyMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cacheStoreStreamCounters(103).cacheStoreStreamCtr(1).scrLatencyMax(5)
	{".2.1.110", "cdnssStat-cache-Measure-Reset" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cdnssStat-cache-Measure-Reset(110)
	{".2.1.111", "cdnssStat-cache-Measure-Since" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cdnssStat-cache-Measure-Since(111)
	{".2.1.20", "cdnssStat-cache-missedSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cdnssStat-cache-missedSize(20)
	{".2.1.21", "cdnssStat-cache-hotLocalsSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cdnssStat-cache-hotLocalsSize(21)
	{".2.1.22", "cdnssStat-cache-requestsInTimeWin" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cdnssStat-cache-requestsInTimeWin(22)
	{".2.1.23", "cdnssStat-cache-hitInTimeWin" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cdnss(2100).cdnssSvcApp(2).cdnssAttr(1).cdnssStat-cache-hitInTimeWin(23)
	{ NULL, NULL } };

// -------------------------------------
// service channelOnDemand oid[800]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_channelOnDemand[] = {
	{".2", "chodSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).channelOnDemand(800).chodSvcApp(2)
	{ NULL, NULL } };

// -------------------------------------
// service cpeSvc oid[700]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_cpeSvc[] = {
	{".2", "cpesvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2)
	{".2.1", "cpesvcAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2).cpesvcAttr(1)
	{".2.1.100", "contentProvSessionTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2).cpesvcAttr(1).contentProvSessionTable(100)
	{".2.1.100.1", "contentProvSessionEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2).cpesvcAttr(1).contentProvSessionTable(100).contentProvSessionEntry(1)
	{".2.1.100.1.1", "cpeMethod" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2).cpesvcAttr(1).contentProvSessionTable(100).contentProvSessionEntry(1).cpeMethod(1)
	{".2.1.100.1.2", "cpeSessSubtotal" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2).cpesvcAttr(1).contentProvSessionTable(100).contentProvSessionEntry(1).cpeSessSubtotal(2)
	{".2.1.100.1.3", "cpeBwSubtotal" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2).cpesvcAttr(1).contentProvSessionTable(100).contentProvSessionEntry(1).cpeBwSubtotal(3)
	{".2.1.100.1.4", "cpeSessMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2).cpesvcAttr(1).contentProvSessionTable(100).contentProvSessionEntry(1).cpeSessMax(4)
	{".2.1.100.1.5", "cpeBwMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).cpeSvc(700).cpesvcApp(2).cpesvcAttr(1).contentProvSessionTable(100).contentProvSessionEntry(1).cpeBwMax(5)
	{ NULL, NULL } };

// -------------------------------------
// service dataOnDemand oid[1600]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_dataOnDemand[] = {
	{".2", "dodApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dataOnDemand(1600).dodApp(2)
	{ NULL, NULL } };

// -------------------------------------
// service dataStream oid[1800]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_dataStream[] = {
	{".2", "dataStreamApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dataStream(1800).dataStreamApp(2)
	{ NULL, NULL } };

// -------------------------------------
// service dodCS oid[1700]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_dodCS[] = {
	{".2", "dodCSApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dodCS(1700).dodCSApp(2)
	{ NULL, NULL } };

// -------------------------------------
// service dsmccCRG oid[2700]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_dsmccCRG[] = {
	{".2", "dsmccCRGSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2)
	{".2.1", "dsmccCRGAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1)
	{".2.1.100", "dsmccCrStat" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100)
	{".2.1.100.1", "reqStatRow" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1)
	{".2.1.100.1.1", "rowName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).rowName(1)
	{".2.1.100.1.2", "countSubTotal" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).countSubTotal(2)
	{".2.1.100.1.3", "latencyMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).latencyMax(3)
	{".2.1.100.1.4", "latencyAvg" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).latencyAvg(4)
	{".2.1.100.1.5", "countOK" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).countOK(5)
	{".2.1.100.1.6", "countNoSess" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).countNoSess(6)
	{".2.1.100.1.7", "countBadRequest" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).countBadRequest(7)
	{".2.1.100.1.8", "countInvalidMethod" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).countInvalidMethod(8)
	{".2.1.100.1.9", "countServerError" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccCrStat(100).reqStatRow(1).countServerError(9)
	{".2.1.101", "dsmccStat-Measure-Reset" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccStat-Measure-Reset(101)
	{".2.1.102", "dsmccStat-Measured-Since" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccStat-Measured-Since(102)
	{".2.1.105", "dsmccStat-SessionCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccStat-SessionCount(105)
	{".2.1.106", "dsmccStat-PendingSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccStat-PendingSize(106)
	{".2.1.107", "dsmccStat-BusyThreads" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccStat-BusyThreads(107)
	{".2.1.108", "dsmccStat-ThreadPoolSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).dsmccCRG(2700).dsmccCRGSvcApp(2).dsmccCRGAttr(1).dsmccStat-ThreadPoolSize(108)
	{ NULL, NULL } };

// -------------------------------------
// service edgeRM oid[2300]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_edgeRM[] = {
	{".2", "edgeRMApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2)
	{".2.1", "edgeRMAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1)
	{".2.1.200", "ermDevices" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200)
	{".2.1.200.1", "devicesTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1)
	{".2.1.200.1.1", "devicesEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1).devicesEntry(1)
	{".2.1.200.1.1.1", "edZone" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1).devicesEntry(1).edZone(1)
	{".2.1.200.1.1.2", "edDeviceName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1).devicesEntry(1).edDeviceName(2)
	{".2.1.200.1.1.3", "edVendor" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1).devicesEntry(1).edVendor(3)
	{".2.1.200.1.1.4", "edModel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1).devicesEntry(1).edModel(4)
	{".2.1.200.1.1.5", "edDescription" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1).devicesEntry(1).edDescription(5)
	{".2.1.200.1.1.6", "edTftp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1).devicesEntry(1).edTftp(6)
	{".2.1.200.1.1.7", "edAdminUrl" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermDevices(200).devicesTable(1).devicesEntry(1).edAdminUrl(7)
	{".2.1.201", "ermPorts" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201)
	{".2.1.201.1", "portsTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1)
	{".2.1.201.1.1", "portsEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1).portsEntry(1)
	{".2.1.201.1.1.1", "epPortID" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1).portsEntry(1).epPortID(1)
	{".2.1.201.1.1.2", "epPowerLevel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1).portsEntry(1).epPowerLevel(2)
	{".2.1.201.1.1.3", "epModulationFormat" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1).portsEntry(1).epModulationFormat(3)
	{".2.1.201.1.1.4", "epInterLeaverMode" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1).portsEntry(1).epInterLeaverMode(4)
	{".2.1.201.1.1.5", "epFEC" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1).portsEntry(1).epFEC(5)
	{".2.1.201.1.1.6", "epDeviceIP" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1).portsEntry(1).epDeviceIP(6)
	{".2.1.201.1.1.7", "epDeviceGroup" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermPorts(201).portsTable(1).portsEntry(1).epDeviceGroup(7)
	{".2.1.202", "ermChannels" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202)
	{".2.1.202.1", "channelsTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1)
	{".2.1.202.1.1", "channelsEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1)
	{".2.1.202.1.1.1", "ecName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecName(1)
	{".2.1.202.1.1.10", "ecLowBandwidthUtilization" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecLowBandwidthUtilization(10)
	{".2.1.202.1.1.11", "ecHighBandwidthUtilization" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecHighBandwidthUtilization(11)
	{".2.1.202.1.1.12", "ecMaxSessions" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecMaxSessions(12)
	{".2.1.202.1.1.13", "ecIntervalPAT" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecIntervalPAT(13)
	{".2.1.202.1.1.14", "ecIntervalPMT" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecIntervalPMT(14)
	{".2.1.202.1.1.15", "ecSymbolRate" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecSymbolRate(15)
	{".2.1.202.1.1.16", "ecAllocations" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecAllocations(16)
	{".2.1.202.1.1.2", "ecStampLastUpdated" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecStampLastUpdated(2)
	{".2.1.202.1.1.3", "ecEnabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecEnabled(3)
	{".2.1.202.1.1.4", "ecFreqRF" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecFreqRF(4)
	{".2.1.202.1.1.5", "ecTSID" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecTSID(5)
	{".2.1.202.1.1.6", "ecNITPID" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecNITPID(6)
	{".2.1.202.1.1.7", "ecStartUDPPort" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecStartUDPPort(7)
	{".2.1.202.1.1.8", "ecUdpPortStepByPn" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecUdpPortStepByPn(8)
	{".2.1.202.1.1.9", "ecStartProgramNumber" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).edgeRM(2300).edgeRMApp(2).edgeRMAttr(1).ermChannels(202).channelsTable(1).channelsEntry(1).ecStartProgramNumber(9)
	{ NULL, NULL } };

// -------------------------------------
// service eventChannel oid[1300]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_eventChannel[] = {
	{".2", "eventChApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventChannel(1300).eventChApp(2)
	{".2.1", "eventChAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventChannel(1300).eventChApp(2).eventChAttr(1)
	{".2.1.20", "eventCh-endpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventChannel(1300).eventChApp(2).eventChAttr(1).eventCh-endpoint(20)
	{".2.1.21", "eventCh-llevelIce" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventChannel(1300).eventChApp(2).eventChAttr(1).eventCh-llevelIce(21)
	{ NULL, NULL } };

// -------------------------------------
// service eventGateway oid[1200]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_eventGateway[] = {
	{".2", "eventGwSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventGateway(1200).eventGwSvcApp(2)
	{".2.1", "eventGwAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventGateway(1200).eventGwSvcApp(2).eventGwAttr(1)
	{".2.1.20", "eventGw-eventChEp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventGateway(1200).eventGwSvcApp(2).eventGwAttr(1).eventGw-eventChEp(20)
	{".2.1.21", "eventGw-ConnCheckIntv" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventGateway(1200).eventGwSvcApp(2).eventGwAttr(1).eventGw-ConnCheckIntv(21)
	{".2.1.23", "eventGw-llevelIce" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).eventGateway(1200).eventGwSvcApp(2).eventGwAttr(1).eventGw-llevelIce(23)
	{ NULL, NULL } };

// -------------------------------------
// service httpCRG oid[2200]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_httpCRG[] = {
	{".2", "httpSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2)
	{".2.1", "httpCrgAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1)
	{".2.1.1", "httpCRG-Version" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpCRG-Version(1)
	{".2.1.10", "crgConfigDir" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).crgConfigDir(10)
	{".2.1.100", "httpReqTbl" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100)
	{".2.1.100.1", "httpReqEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1)
	{".2.1.100.1.1", "reqMethod" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).reqMethod(1)
	{".2.1.100.1.2", "reqCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).reqCount(2)
	{".2.1.100.1.3", "reqLatencyAvg" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).reqLatencyAvg(3)
	{".2.1.100.1.4", "reqLatencyMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).reqLatencyMax(4)
	{".2.1.100.1.5", "resp2xx" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp2xx(5)
	{".2.1.100.1.6", "resp400" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp400(6)
	{".2.1.100.1.7", "resp404" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp404(7)
	{".2.1.100.1.8", "resp500" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp500(8)
	{".2.1.100.1.9", "resp503" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp503(9)
	{".2.1.101", "httpStat-Since" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpStat-Since(101)
	{".2.1.102", "httpStat-Measure-Reset" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpStat-Measure-Reset(102)
	{".2.1.2", "crgSnmpLoggingMask" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).crgSnmpLoggingMask(2)
	{".2.1.3", "crgLogDir" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).crgLogDir(3)
	{".2.1.6", "crgLogFileSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).crgLogFileSize(6)
	{".2.1.7", "crgLogTimeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).crgLogTimeout(7)
	{".2.1.8", "crgLogBufferSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).crgLogBufferSize(8)
	{".2.1.9", "crgLogLevel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).crgLogLevel(9)
	{".3", "extC2loc" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3)
	{".3.1", "c2locAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1)
	{".3.1.1", "c2locLogLevel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locLogLevel(1)
	{".3.1.100", "c2locPortTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100)
	{".3.1.100.1", "c2locPortEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1)
	{".3.1.100.1.1", "c2locPortIndex" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortIndex(1)
	{".3.1.100.1.10", "c2locPortErrorRatePercent" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortErrorRatePercent(10)
	{".3.1.100.1.2", "c2locPortName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortName(2)
	{".3.1.100.1.3", "c2locPortSS" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortSS(3)
	{".3.1.100.1.4", "c2locPortCapacityKbps" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortCapacityKbps(4)
	{".3.1.100.1.5", "c2locPortUsedBwKbps" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortUsedBwKbps(5)
	{".3.1.100.1.6", "c2locPortTransferCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortTransferCount(6)
	{".3.1.100.1.7", "c2locPortEnabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortEnabled(7)
	{".3.1.100.1.8", "c2locPortPenalty" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortPenalty(8)
	{".3.1.100.1.9", "c2locPortAddresses" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortTable(100).c2locPortEntry(1).c2locPortAddresses(9)
	{".3.1.101", "c2locPortCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locPortCount(101)
	{".3.1.102", "clientTransferTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).clientTransferTable(102)
	{".3.1.102.1", "clientTransferEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).clientTransferTable(102).clientTransferEntry(1)
	{".3.1.102.1.1", "ctName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).clientTransferTable(102).clientTransferEntry(1).ctName(1)
	{".3.1.102.1.2", "ctSession" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).clientTransferTable(102).clientTransferEntry(1).ctSession(2)
	{".3.1.102.1.3", "ctBwSubtotal" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).clientTransferTable(102).clientTransferEntry(1).ctBwSubtotal(3)
	{".3.1.102.1.4", "ctBwMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).clientTransferTable(102).clientTransferEntry(1).ctBwMax(4)
	{".3.1.105", "c2locHitRatePercent" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locHitRatePercent(105)
	{".3.1.105", "transferPortTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105)
	{".3.1.105.1", "transferPortEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1)
	{".3.1.105.1.1", "tpName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1).tpName(1)
	{".3.1.105.1.2", "tpStatus" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1).tpStatus(2)
	{".3.1.105.1.3", "tpSession" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1).tpSession(3)
	{".3.1.105.1.4", "tpBwSubtotal" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1).tpBwSubtotal(4)
	{".3.1.105.1.5", "tpBwMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1).tpBwMax(5)
	{".3.1.105.1.6", "tpEnabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1).tpEnabled(6)
	{".3.1.105.1.7", "tpPenalty" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1).tpPenalty(7)
	{".3.1.105.1.8", "tpAddress" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).transferPortTable(105).transferPortEntry(1).tpAddress(8)
	{".3.1.106", "c2locHitRateReset" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locHitRateReset(106)
	{".3.1.15", "c2locLogIce" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locLogIce(15)
	{".3.1.2", "c2locLogSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locLogSize(2)
	{".3.1.3", "c2locLamIf" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locLamIf(3)
	{".3.1.4", "c2locForwardLocator" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locForwardLocator(4)
	{".3.1.5", "c2locOptSelectionRetryMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locOptSelectionRetryMax(5)
	{".3.1.6", "c2locOptIdxRate" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locOptIdxRate(6)
	{".3.1.7", "c2locTotalBwKbps" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locTotalBwKbps(7)
	{".3.1.8", "c2locActiveBwKbps" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locActiveBwKbps(8)
	{".3.1.9", "c2locTransferCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).extC2loc(3).c2locAttr(1).c2locTransferCount(9)
	{ NULL, NULL } };

// -------------------------------------
// service mediaClusterCS oid[1500]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_mediaClusterCS[] = {
	{".2", "mediaClusterCSApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2)
	{".2.1", "mediaClusterCSAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1)
	{".2.1.1", "mediaClusterCS-Version" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-Version(1)
	{".2.1.10", "mediaClusterCS-configDir" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-configDir(10)
	{".2.1.11", "mediaClusterCS-netId" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-netId(11)
	{".2.1.12", "mediaClusterCS-cacheMode" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-cacheMode(12)
	{".2.1.13", "mediaClusterCS-cacheLevel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-cacheLevel(13)
	{".2.1.14", "mediaClusterCS-threads" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-threads(14)
	{".2.1.15", "mediaClusterCS-Bind-endpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-Bind-endpoint(15)
	{".2.1.16", "mediaClusterCS-Bind-dispatchSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-Bind-dispatchSize(16)
	{".2.1.17", "mediaClusterCS-Bind-dispatchMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-Bind-dispatchMax(17)
	{".2.1.18", "mediaClusterCS-Provision-defaultBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-Provision-defaultBandwidth(18)
	{".2.1.19", "mediaClusterCS-Provision-trickSpeeds" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-Provision-trickSpeeds(19)
	{".2.1.2", "mediaClusterCS-SnmpLoggingMask" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-SnmpLoggingMask(2)
	{".2.1.20", "mediaClusterCS-DatabaseCache-volumeSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-DatabaseCache-volumeSize(20)
	{".2.1.21", "mediaClusterCS-DatabaseCache-contentSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-DatabaseCache-contentSize(21)
	{".2.1.22", "mediaClusterCS-DatabaseCache-contentSavePeriod" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-DatabaseCache-contentSavePeriod(22)
	{".2.1.23", "mediaClusterCS-DatabaseCache-contentSaveSizeTrigger" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-DatabaseCache-contentSaveSizeTrigger(23)
	{".2.1.24", "mediaClusterCS-CPC-Bind-endpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-CPC-Bind-endpoint(24)
	{".2.1.3", "mediaClusterCS-LogDir" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-LogDir(3)
	{".2.1.4", "mediaClusterCS-KeepAliveIntervals" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-KeepAliveIntervals(4)
	{".2.1.5", "mediaClusterCS-ShutdownWaitTime" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-ShutdownWaitTime(5)
	{".2.1.6", "mediaClusterCS-LogFileSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-LogFileSize(6)
	{".2.1.7", "mediaClusterCS-LogWriteTimeOut" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-LogWriteTimeOut(7)
	{".2.1.8", "mediaClusterCS-LogBufferSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-LogBufferSize(8)
	{".2.1.9", "mediaClusterCS-LogLevel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).mediaClusterCS(1500).mediaClusterCSApp(2).mediaClusterCSAttr(1).mediaClusterCS-LogLevel(9)
	{ NULL, NULL } };

// -------------------------------------
// service modSvc oid[900]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_modSvc[] = {
	{".2", "modSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).modSvc(900).modSvcApp(2)
	{ NULL, NULL } };

// -------------------------------------
// service nss oid[1400]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_nss[] = {
	{".2", "nssSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).nss(1400).nssSvcApp(2)
	{".2.1", "nssAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).nss(1400).nssSvcApp(2).nssAttr(1)
	{".2.1.100", "nssStat-cSessions" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).nss(1400).nssSvcApp(2).nssAttr(1).nssStat-cSessions(100)
	{ NULL, NULL } };

// -------------------------------------
// service rtspProxy oid[1000]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_rtspProxy[] = {
	{".2", "rtspSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2)
	{".2.1", "rtspProxyAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1)
	{".2.1.1", "rtspProxy-Version" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Version(1)
	{".2.1.10", "rtspProxy-configDir" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-configDir(10)
	{".2.1.100", "rtspProxy-Statistics-455-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-455-per-10min(100)
	{".2.1.101", "rtspProxy-Statistics-OtherErrs-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-OtherErrs-per-10min(101)
	{".2.1.2", "rtspProxy-SnmpLoggingMask" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-SnmpLoggingMask(2)
	{".2.1.200", "rtspCrStat" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspCrStat(200)
	{".2.1.200.2", "rtspCrStat-SessionCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspCrStat(200).rtspCrStat-SessionCount(2)
	{".2.1.200.3", "rtspCrStat-PendingSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspCrStat(200).rtspCrStat-PendingSize(3)
	{".2.1.200.4", "rtspCrStat-BusyThreads" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspCrStat(200).rtspCrStat-BusyThreads(4)
	{".2.1.200.5", "rtspCrStat-ThreadPoolSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspCrStat(200).rtspCrStat-ThreadPoolSize(5)
	{".2.1.21", "rtspProxy-RequestProcess-threads" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-RequestProcess-threads(21)
	{".2.1.22", "rtspProxy-RequestProcess-maxPendingRequest" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-RequestProcess-maxPendingRequest(22)
	{".2.1.23", "rtspProxy-EventPublisher-timeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-EventPublisher-timeout(23)
	{".2.1.24", "rtspProxy-SocketServer-rtspPort" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-SocketServer-rtspPort(24)
	{".2.1.25", "rtspProxy-SocketServer-maxConnections" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-SocketServer-maxConnections(25)
	{".2.1.26", "rtspProxy-SocketServer-threads" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-SocketServer-threads(26)
	{".2.1.27", "rtspProxy-SocketServer-idleScanInterval" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-SocketServer-idleScanInterval(27)
	{".2.1.28", "rtspProxy-SocketServer-maxSessions" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-SocketServer-maxSessions(28)
	{".2.1.29", "rtspProxy-SocketServer-IncomingMessage-maxLen" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-SocketServer-IncomingMessage-maxLen(29)
	{".2.1.3", "rtspProxy-LogDir" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-LogDir(3)
	{".2.1.300", "rtspProxy-Statistics-ANNOUNCE-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-ANNOUNCE-Count(300)
	{".2.1.40", "rtspProxy-Statistics-Measure-Reset" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Measure-Reset(40)
	{".2.1.41", "rtspProxy-Statistics-Measured-Since" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Measured-Since(41)
	{".2.1.42", "rtspProxy-Statistics-Request-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Request-Count(42)
	{".2.1.43", "rtspProxy-Statistics-Total-Succeeded-Request-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Total-Succeeded-Request-Count(43)
	{".2.1.44", "rtspProxy-Statistics-Average-Process-Latency" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Average-Process-Latency(44)
	{".2.1.45", "rtspProxy-Statistics-Max-Process-Latency" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Max-Process-Latency(45)
	{".2.1.46", "rtspProxy-Statistics-Average-Latency" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Average-Latency(46)
	{".2.1.47", "rtspProxy-Statistics-Max-Latency" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Max-Latency(47)
	{".2.1.48", "rtspProxy-Statistics-Skip-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Skip-Count(48)
	{".2.1.49", "rtspProxy-Statistics-SETUP-Latency-Max" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETUP-Latency-Max(49)
	{".2.1.50", "rtspProxy-Statistics-SETUP-Latency-Min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETUP-Latency-Min(50)
	{".2.1.51", "rtspProxy-Statistics-SETUP-Latency-Average" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETUP-Latency-Average(51)
	{".2.1.52", "rtspProxy-Statistics-SETUP-Request-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETUP-Request-Count(52)
	{".2.1.53", "rtspProxy-Statistics-SETUP-Successed-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETUP-Successed-Count(53)
	{".2.1.54", "rtspProxy-Statistics-PLAY-Latency-Max" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PLAY-Latency-Max(54)
	{".2.1.55", "rtspProxy-Statistics-PLAY-Latency-Min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PLAY-Latency-Min(55)
	{".2.1.56", "rtspProxy-Statistics-PLAY-Latency-Average" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PLAY-Latency-Average(56)
	{".2.1.57", "rtspProxy-Statistics-PLAY-Request-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PLAY-Request-Count(57)
	{".2.1.58", "rtspProxy-Statistics-PLAY-Succeeded-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PLAY-Succeeded-Count(58)
	{".2.1.59", "rtspProxy-Statistics-GETPARAMETER-Latency-Max" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-GETPARAMETER-Latency-Max(59)
	{".2.1.6", "rtspProxy-LogFileSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-LogFileSize(6)
	{".2.1.60", "rtspProxy-Statistics-GETPARAMETER-Latency-Min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-GETPARAMETER-Latency-Min(60)
	{".2.1.61", "rtspProxy-Statistics-GETPARAMETER-Latency-Average" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-GETPARAMETER-Latency-Average(61)
	{".2.1.62", "rtspProxy-Statistics-GETPARAMETER-Request-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-GETPARAMETER-Request-Count(62)
	{".2.1.63", "rtspProxy-Statistics-GETPARAMETER-Succeeded-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-GETPARAMETER-Succeeded-Count(63)
	{".2.1.64", "rtspProxy-Statistics-SETPARAMETER-Latency-Max" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETPARAMETER-Latency-Max(64)
	{".2.1.65", "rtspProxy-Statistics-SETPARAMETER-Latency-Min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETPARAMETER-Latency-Min(65)
	{".2.1.66", "rtspProxy-Statistics-SETPARAMETER-Latency-Average" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETPARAMETER-Latency-Average(66)
	{".2.1.67", "rtspProxy-Statistics-SETPARAMETER-Request-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETPARAMETER-Request-Count(67)
	{".2.1.68", "rtspProxy-Statistics-SETPARAMETER-Succeeded-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETPARAMETER-Succeeded-Count(68)
	{".2.1.69", "rtspProxy-Statistics-OPTIONS-Latency-Max" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-OPTIONS-Latency-Max(69)
	{".2.1.70", "rtspProxy-Statistics-OPTIONS-Latency-Min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-OPTIONS-Latency-Min(70)
	{".2.1.71", "rtspProxy-Statistics-OPTIONS-Latency-Average" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-OPTIONS-Latency-Average(71)
	{".2.1.72", "rtspProxy-Statistics-OPTIONS-Request-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-OPTIONS-Request-Count(72)
	{".2.1.73", "rtspProxy-Statistics-OPTIONS-Succeeded-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-OPTIONS-Succeeded-Count(73)
	{".2.1.74", "rtspProxy-Statistics-TEARDOWN-Latency-Max" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-TEARDOWN-Latency-Max(74)
	{".2.1.75", "rtspProxy-Statistics-TEARDOWN-Latency-Min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-TEARDOWN-Latency-Min(75)
	{".2.1.76", "rtspProxy-Statistics-TEARDOWN-Latency-Average" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-TEARDOWN-Latency-Average(76)
	{".2.1.77", "rtspProxy-Statistics-TEARDOWN-Request-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-TEARDOWN-Request-Count(77)
	{".2.1.78", "rtspProxy-Statistics-TEARDOWN-Succeeded-Count" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-TEARDOWN-Succeeded-Count(78)
	{".2.1.82", "rtspProxy-Statistics-Responses-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-Responses-per-10min(82)
	{".2.1.83", "rtspProxy-Statistics-SETUP-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETUP-per-10min(83)
	{".2.1.84", "rtspProxy-Statistics-PLAY-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PLAY-per-10min(84)
	{".2.1.85", "rtspProxy-Statistics-PAUSE-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PAUSE-per-10min(85)
	{".2.1.86", "rtspProxy-Statistics-TEARDOWN-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-TEARDOWN-per-10min(86)
	{".2.1.87", "rtspProxy-Statistics-GET_PARAMETER-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-GET_PARAMETER-per-10min(87)
	{".2.1.88", "rtspProxy-Statistics-SETUP500-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETUP500-per-10min(88)
	{".2.1.89", "rtspProxy-Statistics-PLAY500-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PLAY500-per-10min(89)
	{".2.1.9", "rtspProxy-LogLevel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-LogLevel(9)
	{".2.1.90", "rtspProxy-Statistics-PAUSE500-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PAUSE500-per-10min(90)
	{".2.1.91", "rtspProxy-Statistics-TEARDOWN500-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-TEARDOWN500-per-10min(91)
	{".2.1.92", "rtspProxy-Statistics-GET_PARAMETER500-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-GET_PARAMETER500-per-10min(92)
	{".2.1.93", "rtspProxy-Statistics-SETUP503-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-SETUP503-per-10min(93)
	{".2.1.94", "rtspProxy-Statistics-PLAY503-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PLAY503-per-10min(94)
	{".2.1.95", "rtspProxy-Statistics-PAUSE503-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-PAUSE503-per-10min(95)
	{".2.1.96", "rtspProxy-Statistics-TEARDOWN503-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-TEARDOWN503-per-10min(96)
	{".2.1.97", "rtspProxy-Statistics-GET_PARAMETER503-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-GET_PARAMETER503-per-10min(97)
	{".2.1.98", "rtspProxy-Statistics-404-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-404-per-10min(98)
	{".2.1.99", "rtspProxy-Statistics-454-per-10min" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspSvcApp(2).rtspProxyAttr(1).rtspProxy-Statistics-454-per-10min(99)
	{".3", "rtspExtNGOD" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3)
	{".3.1", "rtspNGODAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1)
	{".3.1.1", "sopTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1)
	{".3.1.1.1", "sopEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1)
	{".3.1.1.1.1", "sopIndex" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopIndex(1)
	{".3.1.1.1.10", "sopUsedBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopUsedBandwidth(10)
	{".3.1.1.1.11", "sopMaxBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopMaxBandwidth(11)
	{".3.1.1.1.12", "sopActiveSession" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopActiveSession(12)
	{".3.1.1.1.13", "sopMaxSession" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopMaxSession(13)
	{".3.1.1.1.14", "sopLocalSession" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopLocalSession(14)
	{".3.1.1.1.15", "sopVolume" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopVolume(15)
	{".3.1.1.1.2", "sopName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopName(2)
	{".3.1.1.1.3", "sopStreamer" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopStreamer(3)
	{".3.1.1.1.4", "sopStreamService" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopStreamService(4)
	{".3.1.1.1.5", "sopStatus" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopStatus(5)
	{".3.1.1.1.6", "sopPenalty" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopPenalty(6)
	{".3.1.1.1.7", "sopSessionUsed" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopSessionUsed(7)
	{".3.1.1.1.8", "sopSessionFailed" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopSessionFailed(8)
	{".3.1.1.1.9", "sopErrorRate" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopTable(1).sopEntry(1).sopErrorRate(9)
	{".3.1.10", "ngod2-RTSPSession-timeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-RTSPSession-timeout(10)
	{".3.1.101", "sopCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopCount(101)
	{".3.1.102", "sopReset" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).sopReset(102)
	{".3.1.105", "icTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icTable(105)
	{".3.1.105.1", "icEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icTable(105).icEntry(1)
	{".3.1.105.1.1", "icIndex" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icTable(105).icEntry(1).icIndex(1)
	{".3.1.105.1.2", "icChannelName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icTable(105).icEntry(1).icChannelName(2)
	{".3.1.105.1.3", "icUsedBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icTable(105).icEntry(1).icUsedBandwidth(3)
	{".3.1.105.1.4", "icTotalBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icTable(105).icEntry(1).icTotalBandwidth(4)
	{".3.1.105.1.5", "icRunningSessCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icTable(105).icEntry(1).icRunningSessCount(5)
	{".3.1.105.1.6", "icStatus" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icTable(105).icEntry(1).icStatus(6)
	{".3.1.106", "icCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).icCount(106)
	{".3.1.11", "ngod2-RTSPSession-cacheSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-RTSPSession-cacheSize(11)
	{".3.1.12", "ngod2-RTSPSession-defaultServiceGroup" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-RTSPSession-defaultServiceGroup(12)
	{".3.1.13", "ngod2-Database-path" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-Database-path(13)
	{".3.1.14", "ngod2-Announce-useGlobalCSeq" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-Announce-useGlobalCSeq(14)
	{".3.1.15", "ngod2-Response-setupFailureWithSessId" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-Response-setupFailureWithSessId(15)
	{".3.1.16", "ngod2-Response-streamCtrlPort" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-Response-streamCtrlPort(16)
	{".3.1.17", "ngod2-LAM-TestMode-enabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-LAM-TestMode-enabled(17)
	{".3.1.18", "ngod2-playlistControl-enableEOT" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-playlistControl-enableEOT(18)
	{".3.1.3", "rtspLogNgod2" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).rtspLogNgod2(3)
	{".3.1.4", "rtspLogNgod2Ice" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).rtspLogNgod2Ice(4)
	{".3.1.5", "rtspLogNgod2Event" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).rtspLogNgod2Event(5)
	{".3.1.6", "ngod2-LogFile-size" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-LogFile-size(6)
	{".3.1.7", "ngod2-LogFile-level" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-LogFile-level(7)
	{".3.1.8", "ngod2-eventChannel-endpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-eventChannel-endpoint(8)
	{".3.1.9", "ngod2-Bind-endpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspExtNGOD(3).rtspNGODAttr(1).ngod2-Bind-endpoint(9)
	{".4", "rtspOpenVBO" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4)
	{".4.1", "rtspVboAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1)
	{".4.1.100", "vboStrTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100)
	{".4.1.100.1", "vboStrEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1)
	{".4.1.100.1.1", "vboStrIndex" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrIndex(1)
	{".4.1.100.1.10", "vboStrMaxBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrMaxBandwidth(10)
	{".4.1.100.1.11", "vboStrActiveSession" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrActiveSession(11)
	{".4.1.100.1.12", "vboStrMaxSession" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrMaxSession(12)
	{".4.1.100.1.13", "vboStrLocalSession" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrLocalSession(13)
	{".4.1.100.1.14", "vboStrVolume" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrVolume(14)
	{".4.1.100.1.2", "vboStrNetId" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrNetId(2)
	{".4.1.100.1.3", "vboStrSourceAddr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrSourceAddr(3)
	{".4.1.100.1.4", "vboStrEndpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrEndpoint(4)
	{".4.1.100.1.5", "vboStrStatus" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrStatus(5)
	{".4.1.100.1.6", "vboStrPenalty" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrPenalty(6)
	{".4.1.100.1.7", "vboStrSessionUsed" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrSessionUsed(7)
	{".4.1.100.1.8", "vboStrSessionFailed" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrSessionFailed(8)
	{".4.1.100.1.9", "vboStrUsedBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStrEntry(1).vboStrUsedBandwidth(9)
	{".4.1.100.102", "vboStatReset" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrTable(100).vboStatReset(102)
	{".4.1.101", "vboStrCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboStrCount(101)
	{".4.1.105", "vboIcTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboIcTable(105)
	{".4.1.105.1", "vboIcEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboIcTable(105).vboIcEntry(1)
	{".4.1.105.1.1", "vboIcIndex" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboIcTable(105).vboIcEntry(1).vboIcIndex(1)
	{".4.1.105.1.2", "vboIcChannelName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboIcTable(105).vboIcEntry(1).vboIcChannelName(2)
	{".4.1.105.1.3", "vboIcUsedBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboIcTable(105).vboIcEntry(1).vboIcUsedBandwidth(3)
	{".4.1.105.1.4", "vboIcTotalBandwidth" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboIcTable(105).vboIcEntry(1).vboIcTotalBandwidth(4)
	{".4.1.105.1.5", "vboIcRunningSessCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboIcTable(105).vboIcEntry(1).vboIcRunningSessCount(5)
	{".4.1.106", "vboIcCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspOpenVBO(4).rtspVboAttr(1).vboIcCount(106)
	{".6", "rtspS1" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspS1(6)
	{".6.1", "rtspS1Attr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspS1(6).rtspS1Attr(1)
	{".6.1.1", "rtspLogS1" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspS1(6).rtspS1Attr(1).rtspLogS1(1)
	{".6.1.2", "rtspLogIcetrace" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).rtspProxy(1000).rtspS1(6).rtspS1Attr(1).rtspLogIcetrace(2)
	{ NULL, NULL } };

// -------------------------------------
// service sentry oid[1100]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_sentry[] = {
	{".2", "sentrySvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2)
	{".2.1", "sentryAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1)
	{".2.1.100", "tsModulesTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100)
	{".2.1.100.1", "tsModulesEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100).tsModulesEntry(1)
	{".2.1.100.1.1", "hostName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100).tsModulesEntry(1).hostName(1)
	{".2.1.100.1.2", "interface" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100).tsModulesEntry(1).interface(2)
	{".2.1.100.1.3", "adapter" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100).tsModulesEntry(1).adapter(3)
	{".2.1.100.1.4", "endpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100).tsModulesEntry(1).endpoint(4)
	{".2.1.100.1.5", "pid" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100).tsModulesEntry(1).pid(5)
	{".2.1.100.1.6", "activated" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100).tsModulesEntry(1).activated(6)
	{".2.1.100.1.7", "ip" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsModulesTable(100).tsModulesEntry(1).ip(7)
	{".2.1.101", "tsNeighborTable" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101)
	{".2.1.101.1", "tsNeighbor" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101).tsNeighbor(1)
	{".2.1.101.1.1", "neighborName" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101).tsNeighbor(1).neighborName(1)
	{".2.1.101.1.2", "nodeId" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101).tsNeighbor(1).nodeId(2)
	{".2.1.101.1.3", "rootURL" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101).tsNeighbor(1).rootURL(3)
	{".2.1.101.1.4", "processor" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101).tsNeighbor(1).processor(4)
	{".2.1.101.1.5", "memory" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101).tsNeighbor(1).memory(5)
	{".2.1.101.1.6", "os" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101).tsNeighbor(1).os(6)
	{".2.1.101.1.7", "startup" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).sentry(1100).sentrySvcApp(2).sentryAttr(1).tsNeighborTable(101).tsNeighbor(1).startup(7)
	{ NULL, NULL } };

// -------------------------------------
// service siteAdmin oid[300]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_siteAdmin[] = {
	{".2", "siteAdminSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).siteAdmin(300).siteAdminSvcApp(2)
	{ NULL, NULL } };

// -------------------------------------
// service streamSegementer oid[2400]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_streamSegementer[] = {
	{".2", "ssegApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSegementer(2400).ssegApp(2)
	{".2.1", "ssegAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSegementer(2400).ssegApp(2).ssegAttr(1)
	{ NULL, NULL } };

// -------------------------------------
// service streamSmith oid[100]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_streamSmith[] = {
	{".2", "streamSmithSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2)
	{".2.1", "streamSmithAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1)
	{".2.1.1", "streamSmith-Version" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Version(1)
	{".2.1.10", "streamSmith-configDir" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-configDir(10)
	{".2.1.11", "streamSmith-default-eventChannel-endpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-default-eventChannel-endpoint(11)
	{".2.1.12", "streamSmith-default-CrashDump-path" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-default-CrashDump-path(12)
	{".2.1.13", "streamSmith-default-CrashDump-enabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-default-CrashDump-enabled(13)
	{".2.1.14", "streamSmith-default-IceTrace-enabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-default-IceTrace-enabled(14)
	{".2.1.15", "streamSmith-default-IceTrace-level" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-default-IceTrace-level(15)
	{".2.1.16", "streamSmith-default-IceTrace-size" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-default-IceTrace-size(16)
	{".2.1.17", "streamSmith-default-Database-path" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-default-Database-path(17)
	{".2.1.18", "streamSmith-netId" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-netId(18)
	{".2.1.19", "streamSmith-mode" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-mode(19)
	{".2.1.2", "streamSmith-SnmpLoggingMask" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SnmpLoggingMask(2)
	{".2.1.20", "streamSmith-SuperPlugin-path" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SuperPlugin-path(20)
	{".2.1.21", "streamSmith-SuperPlugin-enabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SuperPlugin-enabled(21)
	{".2.1.22", "streamSmith-SuperPlugin-eventSinkTimeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SuperPlugin-eventSinkTimeout(22)
	{".2.1.23", "streamSmith-SuperPlugin-enableShowDetail" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SuperPlugin-enableShowDetail(23)
	{".2.1.24", "streamSmith-Bind-endpoint" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Bind-endpoint(24)
	{".2.1.25", "streamSmith-Bind-dispatchSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Bind-dispatchSize(25)
	{".2.1.26", "streamSmith-Bind-dispatchMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Bind-dispatchMax(26)
	{".2.1.27", "streamSmith-RequestProcess-threads" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-RequestProcess-threads(27)
	{".2.1.28", "streamSmith-LocalResource-configuration" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-LocalResource-configuration(28)
	{".2.1.29", "streamSmith-LocalResource-Streamer-defaultSpigotId" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-LocalResource-Streamer-defaultSpigotId(29)
	{".2.1.3", "streamSmith-LogDir" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-LogDir(3)
	{".2.1.33", "streamSmith-DatabaseCache-playlistSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-DatabaseCache-playlistSize(33)
	{".2.1.34", "streamSmith-DatabaseCache-dbHealthCheckThreshold" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-DatabaseCache-dbHealthCheckThreshold(34)
	{".2.1.35", "streamSmith-DatabaseCache-itemSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-DatabaseCache-itemSize(35)
	{".2.1.36", "streamSmith-Playlist-pauseWhenUnload" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-pauseWhenUnload(36)
	{".2.1.37", "streamSmith-Playlist-sessionScanInterval" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-sessionScanInterval(37)
	{".2.1.38", "streamSmith-Playlist-bandwidthUsageScanInterval" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-bandwidthUsageScanInterval(38)
	{".2.1.39", "streamSmith-Playlist-timeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-timeout(39)
	{".2.1.4", "streamSmith-KeepAliveIntervals" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-KeepAliveIntervals(4)
	{".2.1.40", "streamSmith-Playlist-keepOnPause" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-keepOnPause(40)
	{".2.1.41", "streamSmith-Playlist-QueryLastItemPlayTime" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-QueryLastItemPlayTime(41)
	{".2.1.42", "streamSmith-Playlist-progressInterval" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-progressInterval(42)
	{".2.1.43", "streamSmith-Playlist-EOTsize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-EOTsize(43)
	{".2.1.44", "streamSmith-Playlist-PreLoadTime" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-PreLoadTime(44)
	{".2.1.45", "streamSmith-Playlist-ForceNormalTimeBeforeEnd" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-ForceNormalTimeBeforeEnd(45)
	{".2.1.46", "streamSmith-Playlist-delayedCleanup" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-delayedCleanup(46)
	{".2.1.47", "streamSmith-Playlist-repositionInaccuracy" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Playlist-repositionInaccuracy(47)
	{".2.1.48", "streamSmith-CriticalStartPlayWait-enabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-CriticalStartPlayWait-enabled(48)
	{".2.1.49", "streamSmith-CriticalStartPlayWait-timeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-CriticalStartPlayWait-timeout(49)
	{".2.1.5", "streamSmith-ShutdownWaitTime" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-ShutdownWaitTime(5)
	{".2.1.50", "streamSmith-MotoPreEncryption-cycle1" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-MotoPreEncryption-cycle1(50)
	{".2.1.51", "streamSmith-MotoPreEncryption-freq1" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-MotoPreEncryption-freq1(51)
	{".2.1.52", "streamSmith-MotoPreEncryption-cycle2" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-MotoPreEncryption-cycle2(52)
	{".2.1.53", "streamSmith-MotoPreEncryption-freq2" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-MotoPreEncryption-freq2(53)
	{".2.1.54", "streamSmith-Plugin-log-level" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Plugin-log-level(54)
	{".2.1.55", "streamSmith-Plugin-log-fileSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Plugin-log-fileSize(55)
	{".2.1.56", "streamSmith-Plugin-log-buffer" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Plugin-log-buffer(56)
	{".2.1.57", "streamSmith-Plugin-log-flushTimeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-Plugin-log-flushTimeout(57)
	{".2.1.58", "streamSmith-SessionMonitor-log-level" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SessionMonitor-log-level(58)
	{".2.1.59", "streamSmith-SessionMonitor-log-fileSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SessionMonitor-log-fileSize(59)
	{".2.1.6", "streamSmith-LogFileSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-LogFileSize(6)
	{".2.1.60", "streamSmith-SessionMonitor-log-buffer" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SessionMonitor-log-buffer(60)
	{".2.1.61", "streamSmith-SessionMonitor-log-flushTimeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SessionMonitor-log-flushTimeout(61)
	{".2.1.62", "streamSmith-PerfTune-IceFreezeEnvironment-CheckpointPeriod" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-PerfTune-IceFreezeEnvironment-CheckpointPeriod(62)
	{".2.1.63", "streamSmith-PerfTune-IceFreezeEnvironment-DbRecoverFatal" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-PerfTune-IceFreezeEnvironment-DbRecoverFatal(63)
	{".2.1.64", "streamSmith-PerformanceTune-playlist-SavePeriod" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-PerformanceTune-playlist-SavePeriod(64)
	{".2.1.65", "streamSmith-PerformanceTune-playlist-SaveSizeTrigger" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-PerformanceTune-playlist-SaveSizeTrigger(65)
	{".2.1.66", "streamSmith-PerformanceTune-item-SavePeriod" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-PerformanceTune-item-SavePeriod(66)
	{".2.1.67", "streamSmith-PerformanceTune-item-SaveSizeTrigger" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-PerformanceTune-item-SaveSizeTrigger(67)
	{".2.1.68", "streamSmith-SocketServer-enabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-enabled(68)
	{".2.1.69", "streamSmith-SocketServer-rtspPort" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-rtspPort(69)
	{".2.1.7", "streamSmith-LogWriteTimeOut" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-LogWriteTimeOut(7)
	{".2.1.70", "streamSmith-SocketServer-lscpPort" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-lscpPort(70)
	{".2.1.71", "streamSmith-SocketServer-maxConnections" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-maxConnections(71)
	{".2.1.72", "streamSmith-SocketServer-threads" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-threads(72)
	{".2.1.73", "streamSmith-SocketServer-threadPriority" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-threadPriority(73)
	{".2.1.74", "streamSmith-SocketServer-debugLevel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-debugLevel(74)
	{".2.1.75", "streamSmith-SocketServer-debugDetail" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-debugDetail(75)
	{".2.1.76", "streamSmith-SocketServer-idleTimeout" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-idleTimeout(76)
	{".2.1.77", "streamSmith-SocketServer-idleScanInterval" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-idleScanInterval(77)
	{".2.1.78", "streamSmith-SocketServer-SSL-enabled" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-SSL-enabled(78)
	{".2.1.8", "streamSmith-LogBufferSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-LogBufferSize(8)
	{".2.1.84", "streamSmith-SocketServer-IncomingMessage-maxLen" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-SocketServer-IncomingMessage-maxLen(84)
	{".2.1.9", "streamSmith-LogLevel" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).streamSmith(100).streamSmithSvcApp(2).streamSmithAttr(1).streamSmith-LogLevel(9)
	{ NULL, NULL } };

// -------------------------------------
// service weiwoo oid[200]
// -------------------------------------
const ZQ::SNMP::ModuleMIB::MIBE gTblMib_weiwoo[] = {
	{".2", "weiwooSvcApp" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).weiwoo(200).weiwooSvcApp(2)
	{".2.1", "weiwooAttr" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).weiwoo(200).weiwooSvcApp(2).weiwooAttr(1)
	{".2.1.100", "weiwooStat" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).weiwoo(200).weiwooSvcApp(2).weiwooAttr(1).weiwooStat(100)
	{".2.1.100.2", "weiwooStat-cSessions" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).weiwoo(200).weiwooSvcApp(2).weiwooAttr(1).weiwooStat(100).weiwooStat-cSessions(2)
	{".2.1.100.3", "weiwooStat-cPending" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).weiwoo(200).weiwooSvcApp(2).weiwooAttr(1).weiwooStat(100).weiwooStat-cPending(3)
	{".2.1.100.4", "weiwooStat-BusyThreads" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).weiwoo(200).weiwooSvcApp(2).weiwooAttr(1).weiwooStat(100).weiwooStat-BusyThreads(4)
	{".2.1.100.5", "weiwooStat-ThreadPoolSize" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).weiwoo(200).weiwooSvcApp(2).weiwooAttr(1).weiwooStat(100).weiwooStat-ThreadPoolSize(5)
	{ NULL, NULL } };

// -------------------------------------
// Idx by services
// -------------------------------------
const ZQ::SNMP::ModuleMIB::ServiceOidIdx  _gMibSvcs[] = {
	{   100, "streamSmith", gTblMib_streamSmith },
	{   200, "weiwoo", gTblMib_weiwoo },
	{   300, "siteAdmin", gTblMib_siteAdmin },
	{   700, "cpeSvc", gTblMib_cpeSvc },
	{   800, "channelOnDemand", gTblMib_channelOnDemand },
	{   900, "modSvc", gTblMib_modSvc },
	{  1000, "rtspProxy", gTblMib_rtspProxy },
	{  1100, "sentry", gTblMib_sentry },
	{  1200, "eventGateway", gTblMib_eventGateway },
	{  1300, "eventChannel", gTblMib_eventChannel },
	{  1400, "nss", gTblMib_nss },
	{  1500, "mediaClusterCS", gTblMib_mediaClusterCS },
	{  1600, "dataOnDemand", gTblMib_dataOnDemand },
	{  1700, "dodCS", gTblMib_dodCS },
	{  1800, "dataStream", gTblMib_dataStream },
	{  1900, "bcastChannel", gTblMib_bcastChannel },
	{  2000, "cdnCS", gTblMib_cdnCS },
	{  2100, "cdnss", gTblMib_cdnss },
	{  2200, "httpCRG", gTblMib_httpCRG },
	{  2300, "edgeRM", gTblMib_edgeRM },
	{  2400, "streamSegementer", gTblMib_streamSegementer },
	{  2500, "c2fe", gTblMib_c2fe },
	{  2700, "dsmccCRG", gTblMib_dsmccCRG },
	{ 0, NULL, NULL } };
const ZQ::SNMP::ModuleMIB::ServiceOidIdx* gMibSvcs = _gMibSvcs; // to export

