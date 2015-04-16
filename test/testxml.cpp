#include "XercesXmlP.h"
#include <iostream>

typedef ZQ::common::XercesXmlDomTree ConfigDoc;
typedef ZQ::common::XercesXmlDomTree::ChildIterator ConfigNode;

void parse_default(ConfigNode node);
void parse_conver(ConfigNode node);

void parse_LocalListenAddress(ConfigNode node);
void parse_LocalSendAddress(ConfigNode node);
void parse_DenyList(ConfigNode node);

int main(int argc, char* argv[])
{
	 ConfigDoc treexml;
	if(!treexml.open("McastFwd.xml"))
		return 1;


	for(ConfigNode nodeit=treexml.begin();nodeit!=treexml.end();nodeit++)
	{
		if(!nodeit.isElementNode()) continue;
		if(nodeit.getName()=="Default") parse_default(nodeit);
		else if(nodeit.getName()=="Conversation") parse_conver(nodeit);
		else std::cout<<"erro,\r\n";
	}
	treexml.close();
	return 0;
}

void parse_default(ConfigNode node)
{
	std::cout<<"begin to parse default node==================\r\n";
	for(ConfigNode nodeit=node.begin();nodeit!=node.end();nodeit++)
	{
		if(!nodeit.isElementNode()) continue;
		if(nodeit.getName()=="LocalListenAddress") parse_LocalListenAddress(nodeit);
		else if (nodeit.getName()=="LocalSendAddress") parse_LocalSendAddress(nodeit);
		else std::cout<<nodeit.getName()<<std::endl;
	}
	std::cout<<"end parse defautl node=======================\r\n\r\n\r\n";
}

void parse_conver(ConfigNode node)
{
	std::cout<<"begin to parse conversion node==================\r\n";
	for(ConfigNode nodeit=node.begin();nodeit!=node.end();nodeit++)
	{
		if(nodeit.isElementNode()) std::cout<<nodeit.getName()<<std::endl;
	}
	std::cout<<"end parse conversion node=======================\r\n\r\n\r\n";
}

void parse_LocalListenAddress(ConfigNode node)
{
	for(ConfigNode nodeit=node.begin();nodeit!=node.end();nodeit++)
	{
		if(!nodeit.isElementNode()) continue;
		if(nodeit.getName()!="Socket") continue;
		std::cout<<nodeit.getName()<<"address is "<<nodeit.getAttribute("address").c_str()<<std::endl;
	}
}

void parse_LocalSendAddress(ConfigNode node)
{
	for(ConfigNode nodeit=node.begin();nodeit!=node.end();nodeit++)
	{
		if(!nodeit.isElementNode()) continue;
		if(nodeit.getName()!="Socket") continue;
		std::cout<<nodeit.getName()<<"address is "<<nodeit.getAttribute("address").c_str()<<"  port is "<<nodeit.getAttribute("port").c_str()<<std::endl;
	}
}

void parse_DenyList(ConfigNode node)
{
}