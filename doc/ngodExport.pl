#!/usr/bin/perl#
# This is an example of perl module support for the net-snmp agent.
# To load this into a running agent with embedded perl support turned on, 
# simply put the following line (without the leading mark) your /etc/snmp/snmpd.conf file: 
# perl do "/etc/snmp/testagent.pl"; #

BEGIN{ 
 	print STDERR "starting testagent.pl\n"; 
} 
use NetSNMP::OID (':all'); 
use NetSNMP::agent (':all'); 
use NetSNMP::ASN (':all');

print STDERR "ngodExport.pl loaded ok\n";

$rootOID = ".1.3.6.1.4.1.22839.4.100";

@myOIDarray = ('.1.3.6.1.4.1.22839.4.100',
	       '.1.3.6.1.4.1.22839.4.100.1',
	       '.1.3.6.1.4.1.22839.4.100.1.1',
	       '.1.3.6.1.4.1.22839.4.100.1.2',
	       '.1.3.6.1.4.1.22839.4.100.1.3',
	       '.1.3.6.1.4.1.22839.4.100.1.4');
	       
$oidSize = scalar(@myOIDarray);

$LocalIps = "127.0.0.1";
#$ntpServer=`/sbin/ifconfig -a|grep inet|grep -v 127.0.0.1|grep -v inet6`;
#print $ntpServer;
#$ntpServer=~s/.*addr:\([0-9\.]*\).*/\1/g;
#print $ntpServer;

$aaa=`ifconfig |grep 'inet '|grep -v '127.'`;
$IPs="";
while($aaa =~ /([^\n]+)\n?/g){
        my $li = $1;
        $li =~ s/.*addr:([^\s]*).*/$1,/;
        $IPs .= $li;
}
$LocalIps="$IPs";

$ntpServer ="127.0.0.1";
open FH,'<','/opt/TianShan/etc/TianShanDef.xml';
    while(<FH>){
        next unless /\s*<\s*property\s*name\s*=\s*\"TimeServer\"\s*value\s*=\s*\"(\S+)\"\s*\/>/;
     $ntpServer=$1;
    }

sub plog  
{
	open (LOG, ">>/tmp/testagent.log");  
#     print LOG "[$0.";  
#     print LOG __LINE__;  
#     print LOG "]";  
	$i =0;
    foreach $value(@_)  
    {  
        if ($i == 0)  
        {  
            print LOG "[";  
        }  
        elsif ($i == 1)  
        {  
            print LOG ".";  
        }  
        elsif ($i == 2)  
        {  
            print LOG "] ";  
        }  
        else  
        {  
            print LOG " ";  
        }  
        $i++;  
        print LOG $value;  
    }  
    print LOG "\n";  
      
    close(LOG);  
}  

    #
    # Handler routine to deal with SNMP requests
    #
sub myhandler {
	plog($0, __LINE__, "myhandler() enter");
    my ($handler, $registration_info, $request_info, $requests) = @_;

    for ($request = $requests; $request; $request = $request->next()) { 
        #
        #  Work through the list of varbinds
        #
	my $oid = $request->getOID();
	print $oid;
	print "\n";
	plog($0, __LINE__, "myhandler() oid");
   	if ($request_info->getMode() == MODE_GET) {
       		getResult($oid, $request);
   	}
	elsif ($request_info->getMode() == MODE_GETNEXT){
		my $index = -1;
		foreach my $item (0 .. $#myOIDarray){
			if($oid == new NetSNMP::OID($myOIDarray[$item])){
		                $index = $item;
		                break;
		                }
		        }
		print "index: $index\n";
		if($index >= 0 and ($index + 1) < $oidSize)
		{
		   	$nextOID = new NetSNMP::OID($myOIDarray[$index +1]);
           		$request->setOID($nextOID);
           		getResult($nextOID, $request);
		}      	
	}  
    }
}

sub getResult()
{
   my($oid, $request) =@_;
   
   if ($oid == new NetSNMP::OID($rootOID . ".1.1")) {
           		$request->setValue(ASN_OCTET_STR, "0.0.0.0");
   }
   elsif($oid == new NetSNMP::OID($rootOID . ".1.2")) {
           		$request->setValue(ASN_OCTET_STR, $ntpServer);
           }
   elsif($oid == new NetSNMP::OID($rootOID . ".1.3")) {
           		$request->setValue(ASN_OCTET_STR, "0.0.0.0");
           }
   elsif($oid == new NetSNMP::OID($rootOID . ".1.4")) {
           		$request->setValue(ASN_OCTET_STR, $LocalIps);
           }       		    		
}

{
    #
    # Associate the handler with a particular OID tree
    #
	plog("sadfsadfsadf");
    my $regoid = new NetSNMP::OID($rootOID);
	$agent = new NetSNMP::agent('dont_init_agent' => 1,
                            'dont_init_lib' => 1); 
    $agent->register("my_agent_name", $regoid, \&myhandler);
	plog($0, __LINE__, "register() oid");
}

