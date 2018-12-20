import sys
import os
import re

class Mib2CTable:
    """iSCSI target administration using ietadm."""

    def __init__(self):
        self._mibdict = dict()
    
    def load(self, filename) :
        nodeName =""
        
        # fn = argv[1]
        if (len(filename) <=0) :
            filename = "../../TianShan/TianShan.MIB"
        fh = open(filename)

        for line in fh.readlines() :
            # m =re.match('[\s]*::=[\s]*\{[\s]*([^\s]+)[\s]+([\d]+)[\s]*\}.*', line)
            m =re.match('[\s]*([^\s]*).*::=[\s]*\{[\s]*([^\s]+)[\s]+([\d]+)[\s]*\}.*', line)
            if (m) :
                # dict[nodeName] = '{%s}.%s' % (m.groups()[0], m.groups()[1])
                if (len(m.groups()[0]) >0):
                    nodeName = m.groups()[0]
                  
                parentName = m.groups()[1]
                nodeId = m.groups()[2]
                node ={'p': '{%s}' % parentName, 'i':nodeId}
                self._mibdict[nodeName] = node
                if (parentName in self._mibdict) :
                    np = self._mibdict[parentName]
                    node = {'p':'%s.%s(%s)' % (np['p'], parentName, np['i']), 'i':nodeId}
                    self._mibdict[nodeName] = node
      
            m = re.match('[\s]*([^\s]+)[\s]*OBJECT[\s]+IDENTIFIER[\s]*$', line)
            if (m) :
                nodeName = m.groups()[0]
                continue
            
            m = re.match('[\s]*([^\s]+)[\s]*OBJECT-TYPE[\s]*$', line)
            if (m) :
                nodeName = m.groups()[0]
                continue
                        
            m = re.match('[\s]*([^\s]+)[\s]*MODULE-IDENTITY[\s]*$', line)
            if (m) :
                nodeName = m.groups()[0]
                continue
        fh.close()
 
    def list(self) :
        for i in self._mibdict.keys() :
            print '%s.%s(%s), "%s"\n' % (self._mibdict[i]['p'], i, self._mibdict[i]['i'], i)
           
    def outputCArray(self, filename="") :
        if (len(filename)<=0) :
            filename = "./mib2ctbl.cpp"
            
        svcTbl = dict()
        for i in self._mibdict.keys() :
            sNum=""
            sOid = '%s.%s(%s)' % (self._mibdict[i]['p'], i, self._mibdict[i]['i'])
            stks = sOid.split('.')
            # {enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).cloc(3).clocPortMgr(12).clocPortTable(1).clocPortEntry(1).clocPortUsedBwKbps(5)
            if (len(stks) <6 or stks[3] != "tianShanService(1)") :
                continue
            
            service = stks[4]
            svcOid =""
            del stks[:5]
            posB = service.find('(')
            if (posB >0) :
                svcOid = service[posB+1:len(service)-1]
                service = service[0:posB]
                 
            if (service not in svcTbl) :
                svcTbl[service] = {'oid':svcOid, 'lines':[]}
            # stks.shift()
            for tk in stks :
                posB = tk.find('(')
                posE = tk.find(')')
                if (posB <0 or posE <0) :
                    continue
                v = tk[posB+1:posE]
                sNum += '.%s' % v
                
            line = '{"%s", "%s" }, //%s' % (sNum, i, sOid)
            svcTbl[service]['lines'].append(line)
                
        fh = open(filename, "w")
        fh.write('// this file is converted from .MIB file by mibparse.py\n// ====================================================\n\n')
        svcList = svcTbl.keys()
        svcList.sort()
        svcMibList =[]
        for s in svcList :
            fh.write('// -------------------------------------\n// service %s oid[%s]\n// -------------------------------------\n' % (s, svcTbl[s]['oid']))
            fh.write('const ZQ::SNMP::ModuleMIB::MIBE gTblMib_%s[] = {\n' % s)
            svcTbl[s]['lines'].sort()
            for l in svcTbl[s]['lines'] :
                fh.write('\t%s\n' % l)
            fh.write('\t{ NULL, NULL } };\n\n')
            # fh.write('const ZQ::SNMP::ModuleMIB::MIBE* gSvcMib_%s = gTblMib_%s; // to export\n\n' %(s,s))
            svcMibList.append('\t{%6s, "%s", gTblMib_%s },\n' % (svcTbl[s]['oid'], s, s))
        fh.write('// -------------------------------------\n// Idx by services\n// -------------------------------------\n')
        fh.write('const ZQ::SNMP::ModuleMIB::ServiceOidIdx  _gMibSvcs[] = {\n')
        svcMibList.sort()
        for l in svcMibList :
            fh.write(l)
        fh.write('\t{ 0, NULL, NULL } };\nconst ZQ::SNMP::ModuleMIB::ServiceOidIdx* gMibSvcs = _gMibSvcs; // to export\n\n')

        fh.close();
        
    def dupService(self, filename, svcName, maxCount=10) :
        if (len(filename) <=0) :
            filename = "../../TianShan/TianShan.MIB"
        fnOut = '%s.new' % filename
        fin = open(filename)
        fout = open(fnOut, "w")
        svclines = []
        bInService = 0
        svcOid =0
        for line in fin.readlines() :
            mS = re.match('[\s]*([^\s]*).*::=[\s]*\{[\s]*tianShanService[\s]+([\d]+)[\s]*\}.*', line)
            mE = re.match('[\s]*END[\s]*.*', line)
            
            if (mS) :
                if mS.groups()[0] == svcName :
                    svcOid = int(mS.groups()[1])
                    bInService = 1
                else :
                    bInService = 0
            elif mE and len(svclines)>0:
                fout.write('\n-- !!!!!!!!!!!!!!! Duplicated Service[%s] Begin !!!!!!!!!!!!!!!\n' %svcName)
                fout.write(  '-- *********************************************************************\n')
                for i in range(1, maxCount, 1) :
                    newSvcDecl = '%s%d  OBJECT IDENTIFIER  ::= { tianShanService %d }' % (svcName, i, int(svcOid/100)*100 +10*i)
                    fout.write('%s\n' % newSvcDecl)
                    for sl in svclines :
                        slnew = sl.replace(svcName, '%s%d' %(svcName, i))
                        fout.write(slnew)
                    fout.write('\n')
            elif bInService and not re.match('[\s]*\--.*', line) :
                svclines.append(line)
                
            fout.write(line)
        
        fin.close()
        fout.close()

def main(argv=None):
    if argv is None:
        argv = sys.argv

    #if len(argv) < 2:
    #    usage()
    
    ca = Mib2CTable()
    # ca.dupService(filename = "", svcName='nss')
    
    ca.load(filename = "")
    # ca.list()
    ca.outputCArray()
        
    
 
if __name__ == '__main__':
    sys.exit(main())
