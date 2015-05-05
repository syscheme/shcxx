#include "../UDPSocket.h"
// #include "../ObjectMap.h"

// ZQ::common::ObjectMap<int, int*> aaa;

void McastSendTest(ZQ::common::InetMcastAddress group, int group_port, ZQ::common::InetHostAddress bind, int bind_port)
{
	static const char msg[] = "McastSendTest::Hello World";

	ZQ::common::UDPMulticast sock(bind, bind_port);
	sock.setGroup(group, group_port);
	sock.setTimeToLive(1);

	for(int i=0; i<5; i++)
		sock.send(msg, sizeof(msg));
}
void McastReceiveTest(ZQ::common::InetMcastAddress group, int group_port, ZQ::common::InetHostAddress bind, int count=5)
{
	char buf[1024];

	ZQ::common::UDPReceive receiver(bind, group_port);
	receiver.setMulticast(true);
	ZQ::common::Socket::Error err = receiver.join(group);

	if (err != ZQ::common::Socket::errSuccess)
	{
		printf("\nunable to join the mcast group");
		return;
	}

	for (int i=0; i<count; i++)
	{
        int len = receiver.receive(buf, sizeof(buf));
		printf("\npack: ");
		for (int j=0; j<len; j++)
			printf("%02x(%c) ", buf[j], (isprint(buf[j])?buf[j]:'.'));
	}
	printf("\n");
}

void UDPUniSendTest(ZQ::common::InetHostAddress dest, int dest_port, ZQ::common::InetHostAddress bind, int bind_port)
{
	static const char msg[] = "UDPUniSendTest::Hello World";

	ZQ::common::UDPMulticast sock(bind, bind_port);
	sock.setPeer(dest, dest_port);
	sock.setTimeToLive(1);

	for(int i=0; i<5; i++)
		sock.send(msg, sizeof(msg));
}



void main()
{

ZQ::common::InetHostAddress localaddr("192.168.0.8");
//ZQ::common::InetHostAddress localaddr6("fe80::20e:a6ff:fe09:2e8");
ZQ::common::InetHostAddress localaddr6("::0");

	McastReceiveTest("ff02::1", 1000, localaddr6, 1000);
// ZQ::common::InetHostAddress localaddr6("::0");
//	McastReceiveTest("225.25.25.25", 7015, localaddr);
//	McastReceiveTest("225.15.15.15", 1000, localaddr, 1000);

/*
McastSendTest("225.15.15.15", 1000, localaddr, 1000);
	McastSendTest("ff02::001", 1000, localaddr6, 1000);

	UDPUniSendTest("192.168.0.1", 1000, localaddr, 1000);
	UDPUniSendTest("fe80::20d:87ff:feac:2285", 1000, localaddr6, 1000);
// */

//	McastSendTest("ff02::1", 1000, localaddr6, 1000);
//	UDPUniSendTest("fe80::20d:87ff:feac:2285", 1000, localaddr6, 1000);
}