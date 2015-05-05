// CmdParser.h: interface for the CCmdParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMDPARSER_H__F9A634FD_84BE_4D35_A655_5DA6BF1C2BEB__INCLUDED_)
#define AFX_CMDPARSER_H__F9A634FD_84BE_4D35_A655_5DA6BF1C2BEB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <string>
#include <Variant.h>
#include <vector>
#include <Locks.h>

using namespace ZQ::common;
class CCmdParser;
typedef	std::vector<std::string>	VARVEC;
typedef int (CCmdParser::*CMDROUTE)(VARVEC&);

typedef struct _tagCmdRoute 
{
	char*			_cmdString;//string for identity a command
	CMDROUTE		_cmdRoute;
}CmdRoute;

typedef struct _tagRouteMap 
{
	const _tagRouteMap*	_pBaseMap;
	const CmdRoute*		_route;	
}RouteMap;

#define	DECLEAR_CMDROUTE()\
	private:\
	static const CmdRoute	_messageEntry[];\
	static const RouteMap	_messageMap;\
	virtual const RouteMap*	GetRouteMap();
	

#define	BEGIN_CMDROUTE(theclass,baseclass)\
	const RouteMap*		theclass::GetRouteMap(){return &theclass::_messageMap;}\
	const RouteMap		theclass::_messageMap={&baseclass::_messageMap,&theclass::_messageEntry[0] };\
	const CmdRoute theclass::_messageEntry[]=\
	{

#define	END_CMDROUTE()\
	{"",NULL}\
	};
//////////////////////////////////////////////////////////////////////////
//define command call function
#define	COMMAND(cmdString,cmdFunction) {#cmdString, (CMDROUTE)&cmdFunction},
#define		FUNCDEF(x)	int	x(VARVEC& var);

#define		RETERN_QUIT	0xffffffee

class CCmdParser  
{
public:
	CCmdParser();
	virtual ~CCmdParser();
public:
	int			ParseCommand(const std::string& strCommand);
	void		log(const char*fmt,...);
	void		EatWhite(std::string& str);//This function will get rid of lead white space and post white space 
protected:	
	void		PrepareParameter(std::string strCommand,std::string& strCMD,VARVEC& var);
	void		NoMatchRoute(std::string& str);	
private:
	void		SplitString(std::string& strCmd,VARVEC& var);
	int			SplitStringByArgv(std::string& strArgv,VARVEC& var);
	int			DoCommand(CmdRoute* pRoute,std::string cmd,VARVEC& var);
	int			Quit(VARVEC& var);
private:
	bool		m_bExecute;
	Mutex		m_OutputMutex;		
	DECLEAR_CMDROUTE()
};

#endif // !defined(AFX_CMDPARSER_H__F9A634FD_84BE_4D35_A655_5DA6BF1C2BEB__INCLUDED_)
