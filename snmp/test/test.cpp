#include "snmp/ZQSnmp.h"
#include "FileLog.h"
#include "SystemUtils.h"

using namespace ZQ::common;
using namespace ZQ::SNMP;
void hexPrint(void* v, int len)
{
	char tbl[] = "0123456789abcdef";
	const unsigned char* bs = (const unsigned char*)v;
	for(int i = 0; i < len; ++i) {
		unsigned char b = bs[i];
		printf("%c%c ", tbl[(b >> 4) & 0x0F], tbl[b & 0x0F]);
	}
}

extern const ZQ::SNMP::ModuleMIB::MIBE* gSvcMib_rtspProxy;
#include "../mib2ctbl.cpp"

class VarContainer
{
public:
	VarContainer() :_s("svalue") { }
	
	uint32 getInt() { return 1223;}
	void setInt(const uint32&) {}
	
	std::string getStr() { return _s; }
	void setStr(const std::string& s) { _s=s; }

	std::string _s;
};

VarContainer vc;

int SNMP_unitTest()
{
	FileLog log("SnmpTrace.log", Log::L_DEBUG);
	// ZQ::SNMP::ModuleMIB::setComponentMIBIndices(gSvcMib_rtspProxy);

	u_char buf[1024];
	size_t buflen = 1024;

	Oid oid(".1.3.6.1.4.1.22839.4.1.1000"), a=oid;
	oid.append(3);
	std::string str = oid.str();
	bool b = a.isDescendant(oid);
	b= a.isDescendant(a);
	b= a.isDescendant(Oid(".1.3.6.1.4.1"));
	b= a.isDescendant(Oid(".1.3.6.1.4.1.22839.4.1.1001"));

	SNMPVariable var;
	var.setOid(a);
	// var.serialize(buf, sizeof(buf));

	ModuleMIB mmib3(log, 1000, 3);
	Oid subOidTable;
 	mmib3.reserveTable("sopTable", 15, subOidTable);
	mmib3.addTableCell(subOidTable, 1, 1, new SNMPObjectDupValue("sopIndex", (int32)11));
	mmib3.addTableCell(subOidTable, 2, 1, new SNMPObjectDupValue("sopName",  "name"));
	mmib3.addTableCell(subOidTable, 3, 1, new SNMPObjectDupValue("sopStreamer", "streamer"));
	mmib3.addTableCell(subOidTable, 4, 1, new SNMPObjectDupValue("sopStreamService", "service"));
	mmib3.addTableCell(subOidTable, 5, 1, new SNMPObjectDupValue("sopStatus", "status"));
	//mmib3.addTableCell(subOidTable, 6, 1, new SNMPObjectDupValue("sopPenalty", (int32)12));
	//mmib3.addTableCell(subOidTable, 7, 1, new SNMPObjectDupValue("sopSessionUsed", (int32)123));
	//mmib3.addTableCell(subOidTable, 8, 1, new SNMPObjectDupValue("sopSessionFailed", (int32)32));
	//mmib3.addTableCell(subOidTable, 9, 1, new SNMPObjectDupValue("sopErrorRate", (int32)5));
	//mmib3.addTableCell(subOidTable, 10, 1, new SNMPObjectDupValue("sopUsedBandwidth", (int64)1234567890));
	//mmib3.addTableCell(subOidTable, 11, 1, new SNMPObjectDupValue("sopMaxBandwidth", (int32)5));
	//mmib3.addTableCell(subOidTable, 15, 1, new SNMPObjectDupValue("sopVolume", "volume"));
	mmib3.addTableCell(subOidTable, 1, 2, new SNMPObjectDupValue("sopIndex", (int32)22));
	mmib3.addTableCell(subOidTable, 2, 2, new SNMPObjectDupValue("sopName",  "N2"));
	mmib3.addTableCell(subOidTable, 3, 2, new SNMPObjectDupValue("sopStreamer", "S2"));
	mmib3.addTableCell(subOidTable, 4, 2, new SNMPObjectDupValue("sopStreamService", "SVC2"));
	mmib3.addTableCell(subOidTable, 5, 2, new SNMPObjectDupValue("sopStatus", "ST2"));

	mmib3.addObject(new SNMPObjectDupValue("XXXX1", "XXX1"), ".5"); 
	mmib3.addObject(new SNMPObjectDupValue("XXXX2", "XXX2"), ".5.1");
	mmib3.addObject(new SNMPObjectDupValue("XXXX3", "XXX3"), ".5.2");
	mmib3.addObject(new SNMPObjectDupValue("XXXX4", "XXX4"), ".5.3");
	// XXXX1: {"_v":XXX1, XXXX2:XXX2, XXXX3:XXX3}

	Oid next = mmib3.nextOid(Oid(".2"));
	Oid first = mmib3.firstOid();

	ModuleMIB mmib(log, 1000);

	uint32 var_uint32 = 32;
    SNMPObject::Ptr obj = new SNMPObject("abc", var_uint32, false);
	mmib.addObject(obj, Oid(".2"));
	
	obj = new SNMPObjectByAPI<VarContainer, uint32>("abe", vc, AsnType_Int32, &VarContainer::getInt);
	
	SNMPVariable value;
	obj->read(value);
	obj = new SNMPObjectByAPI<VarContainer, std::string>("abd", vc, AsnType_String, &VarContainer::getStr, &VarContainer::setStr);
	obj->read(value);
	obj->read(value, SNMPObject::vf_VarName);
	obj->write(value);
	
	mmib.addObject(new SNMPObjectByAPI<VarContainer, uint32>("rtspProxy-RequestProcess-threads", vc, AsnType_Int32, &VarContainer::getInt));
	mmib.addObject(new SNMPObjectByAPI<VarContainer, std::string>("efgh", vc, AsnType_String, &VarContainer::getStr, &VarContainer::setStr), ".4");

	SNMPVariable::List vlist;
	SNMPVariable::VT   v;
#ifdef ZQ_OS_MSWIN
	vlist.push_back(new SNMPVariable(mmib.buildupOid(Oid(".3")), &v));
#else
    Oid::I_t name[] = {1, 3, 6, 1, 4, 1, 22839, 4, 1, 1000, 2, 2, 3};
    v.name = name;
    v.name_length = 13;
    vlist.push_back(new SNMPVariable(&v));
#endif
	mmib.readVars(vlist);
	mmib.nextVar(vlist);

	vlist.clear();
#ifdef ZQ_OS_MSWIN
	vlist.push_back(new SNMPVariable(mmib.buildupOid(Oid(".3"), SNMPObject::vf_VarName), &v));
#else
    vlist.push_back(new SNMPVariable(&v));
#endif
	mmib.readVars(vlist);

	vlist[0]->setOid(mmib.buildupOid(Oid(".4")));
	mmib.writeVars(vlist);
	//mmib.vars2Json(vlist);
    int64 var_int64 = 64;
    Oid oid5(".5");
    mmib.addObject(new SNMPObject("testInt64", var_int64, false), oid5);

    std::string var_string = "string";
    Oid oid6(".6");
    mmib.addObject(new SNMPObject("testString", var_string, true), oid6);

    char var_cstr7[200] = "cstringAAA";
    char var_cstr8[200] = "cstringBBB";
    mmib.addObject(new SNMPObject("var_cstr7", var_cstr7, false), Oid(".7"));
    mmib.addObject(new SNMPObject("var_cstr8", var_cstr8, false), Oid(".8"));

	SubAgent ag(log, mmib, 5000);

	vlist.clear();
/*
#ifdef ZQ_OS_MSWIN
    vlist.push_back(new SNMPVariable(mmib.buildupOid(Oid(".3")), &v));
    vlist.push_back(new SNMPVariable(mmib.buildupOid(Oid(".4")), &v));
#else
    vlist.push_back(new SNMPVariable(&v));
    vlist.push_back(new SNMPVariable(&v));
#endif
*/
    vlist.push_back(new SNMPVariable(mmib.buildupOid(Oid(".7")), &v));
	mmib.readVars(vlist);
	vlist[0]->setOid(mmib3.buildupOid(subOidTable));
	// vlist[0]->setOid(mmib3.buildupOid(Oid(".5.2")));
	// mmib.writeVars(vlist);
	mmib3.vars2Json(vlist);

	BaseAgent::Msgheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.pdu = ZQSNMP_PDU_GETJSON;
	hdr.error = se_NoError;
	size_t len = ag.encodeMessage(buf, sizeof(buf), hdr, vlist);
	vlist.clear();
	ag.decodeMessage(buf, len, hdr, vlist);
	
	ag.start();
    SYS::sleep(60*60*1000);

	return 0;
};

#include "../Agent_win.cpp"

int testAgent()
{
	FileLog log("SubAgent.log", Log::L_DEBUG);
	ModuleMIB mmib(log, 1000);

	uint32 var_uint32 = 32, var2=57, var3=9033;
	mmib.addObject(new SNMPObject("abc", var_uint32, false), Oid(".1"));
	mmib.addObject(new SNMPObject("var2", var2, false),      Oid(".2"));
	mmib.addObject(new SNMPObject("var3", var3, false),      Oid(".3"));

	SubAgent ag(log, mmib, 5000);
	ag.start();
    SYS::sleep(1000);

	SnmpExtensionInit(0, NULL, NULL);

	SnmpVarBind vbs[3];
//	memset(vbs, 0, sizeof(vbs));
	size_t i =0, j;
	Oid fullOid = mmib.buildupOid(Oid(".1"));
	for (j=0; j<3; j++)
	{
		vbs[j].name.idLength = fullOid.length();
		vbs[j].name.ids = (UINT *)SnmpUtilMemAlloc(sizeof(UINT) * vbs[j].name.idLength);
		for (i =0; i < vbs[0].name.idLength; i++)
			vbs[j].name.ids[i] = fullOid[i];
	}
	vbs[0].name.ids[fullOid.length()-1] =1;
	vbs[1].name.ids[fullOid.length()-1] =2;
	vbs[2].name.ids[fullOid.length()-1] =3;

	SnmpVarBindList vbl = { vbs, sizeof(vbs)/sizeof(vbs[0]) };
	AsnInteger32 errstatus, errIdx;

	for (int i=0; i< 10; i++)
		SnmpExtensionQuery(SNMP_PDU_GET, &vbl, &errstatus, &errIdx);

	SnmpExtensionClose();

	ag.stop();
	return 0;
}

int main()
{
	return SNMP_unitTest();
}
