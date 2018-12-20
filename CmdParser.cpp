// CmdParser.cpp: implementation of the CCmdParser class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)
#include "stdafx.h"
#include "CmdParser.h"
#include <stdio.h>
#include <stdarg.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCmdParser::CCmdParser()
{
	m_bExecute=false;
}

CCmdParser::~CCmdParser()
{

}
//BEGIN_CMDROUTE(CCmdParser,CCmdParser)
//	COMMAND(Help,Help)
//END_CMDROUTE()
void CCmdParser::log(const char*fmt,...)
{
	char msg[2048];
	va_list args;
	
	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);
	MutexGuard gd(m_OutputMutex);
	printf("%s\n",msg);
}
int CCmdParser::ParseCommand(const std::string& strCommand)
{
	std::string	cmd;	
	VARVEC	var;
	PrepareParameter(strCommand,cmd,var);
	RouteMap* pMap=(RouteMap*)GetRouteMap();
	CmdRoute *pRoute=NULL;	
	bool bOK=true;
	if(stricmp("quit",strCommand.c_str())==0)
		return RETERN_QUIT;
	do 
	{
		pRoute=(CmdRoute*)pMap->_route;
		m_bExecute=false;
		if(pRoute )
		{
			 int ret=DoCommand(pRoute,cmd,var);
			 if(m_bExecute)
				return ret;
			 else
				 pMap=(RouteMap*)pMap->_pBaseMap;
		}
		else
		{
			pMap=(RouteMap*)pMap->_pBaseMap;
		}
	} while(pMap!=NULL);
	NoMatchRoute(cmd);	
	return -1;
}
int CCmdParser::DoCommand(CmdRoute* pRoute,std::string cmd,VARVEC& var)
{
	
	while (pRoute->_cmdRoute!=NULL) 
	{
		if(stricmp(pRoute->_cmdString,cmd.c_str())==0)
		{
			m_bExecute=true;
			return (this->*pRoute->_cmdRoute)(var);			
		}
		pRoute++;
	}
	return -1;
}
void CCmdParser::NoMatchRoute(std::string& str)
{
	log("not support command %s\nPlease refer to help command",str.c_str());
}
void CCmdParser::EatWhite(std::string& str)
{
	int iPos=0;
	//look for lead white space
	do
	{
		if(str.length()<=0)
			break;
		if( (iPos=str.find_first_not_of(' ')) !=0)
			str=str.substr(iPos,str.length()-iPos);
		if(str.length()<=0)
			break;
		if((iPos=str.find_first_not_of('\t'))!=0)
			str=str.substr(iPos,str.length()-iPos);
	}while(iPos!=0);

	//look for post white space
	do 
	{		
		if( (iPos=str.find_last_not_of(' ')) != (int)(str.length()-1) )
			str=str.substr(0,iPos+1);
		if(str.length()<=0)
			break;
		if( (iPos=str.find_last_not_of('\t')) != (int) (str.length()-1) )
			str=str.substr(0,iPos+1);
		if(str.length()<=0)
			break;
	} while(str[str.length()-1]==' '|| str[str.length()-1]=='\t');
}
#define	POSOK(x) (x!=std::string::npos)
void CCmdParser::SplitString(std::string& strCmd,VARVEC& var)
{
	EatWhite(strCmd);
	var.clear();
	if(strCmd.length()<=0)
		return;
	int iPosSpace=0,iPosTable=0,iPosQuote=0;
	int iPos=-1;
	bool	bQuoteLast=false;
	std::string	temp;

	do
	{
		iPos=-1;
		//首先查找是否含有"
		if(strCmd.length()<=0)
			break;
		while(strCmd[0]=='"')
		{
			iPosQuote=strCmd.find_first_of('"',1);
			if(POSOK(iPosQuote))
			{
				temp=strCmd.substr(0,iPosQuote+1);
				EatWhite(temp);
				temp.erase(temp.begin());
				temp.erase(temp.end()-1);
				var.push_back(temp);
				strCmd=strCmd.substr(iPosQuote+1,strCmd.length()-iPosQuote);
				EatWhite(strCmd);
			}
			else
			{
				break;
			}
			if(strCmd.length()<=0)
				break;
		} 
		iPosTable=strCmd.find_first_of('\t',iPos+1);
		iPosSpace=strCmd.find_first_of(' ',iPos+1);
		//iPosQuote=strCmd.find_first_of(iPos+1,'"');
		if(POSOK(iPosSpace))
		{
			if(POSOK(iPosTable))
				iPos=iPosSpace>iPosTable ? iPosTable:iPosSpace;
			else
				iPos=iPosSpace;
		}
		else
		{
			if(POSOK(iPosTable))
				iPos=iPosTable;
			else
				iPos=std::string::npos;
		}
		if(POSOK(iPos))
		{
			temp=strCmd.substr(0,iPos+1);
			EatWhite(temp);
			var.push_back(temp);
			strCmd=strCmd.substr(iPos+1,strCmd.length()-iPos);			
		}
		EatWhite(strCmd);
	}while (POSOK(iPos));
	if(strCmd.length()>=1)
	{
		var.push_back(strCmd);
	}
}
void CCmdParser::PrepareParameter(std::string strCommand,std::string& strCMD,VARVEC& var)
{	
	SplitString(strCommand,var);	
	if(var.size()<=0)
		return;
	if(var.size()>=1)
		strCMD=var[0];
	var.erase(var.begin());
}
const CmdRoute	CCmdParser::_messageEntry[]={"",NULL} ;
const RouteMap	CCmdParser::_messageMap={NULL,NULL};
const RouteMap*	CCmdParser::GetRouteMap()
{
	return &CCmdParser::_messageMap;
}