
//#pragma warning(disable:4786)
#include "strHelper.h"
#include <stack>


namespace ZQ
{
namespace common
{
namespace stringHelper
{

#define POSOK(x)	(x!=std::string::npos)
	
typedef struct  _STATTR
{
	char	_ch;
	int		_index;
}STATTR;
typedef std::stack<STATTR>	CHSTACK;

bool ZQ_COMMON_API	TrimExtra(std::string& str,const std::string& strExtra)
{
	if(str.length ()<=0)
		return false;
	std::string::size_type	posBegin=0,posEnd=0;
//	std::string::size_type	curPos=0;
	std::string::size_type	length=str.length ();
	std::string	ch;
	//首先查找begin
	while (length>posBegin)
	{
		ch=str.at(posBegin);
		if(!POSOK(strExtra.find(ch)))
			break;
		posBegin++;
	}
	posEnd=length-1;
	while (posEnd>=posBegin)
	{
		ch=str.at(posEnd);
		if(!POSOK(strExtra.find(ch)))
			break;
		posEnd--;
	}
	if(posEnd>=posBegin)
	{
		str=str.substr (posBegin,posEnd-posBegin+1);
		return true;
	}
	return false;
}
bool ZQ_COMMON_API SplitString(const std::string& str ,std::vector<std::string>& result, 
										const std::string& delimiter	,
										const std::string& trimDelimiter,
										const std::string& quote,
										const std::string& escape,
										int maxSection)
{
	result.clear ();
	if(str.empty ())
	{		
		return false;
	}
	int		curPos=0;	//current position
	int		lastPos=0;	//last position
					
	int		length=(int)str.length ();

	std::string			strQuote;
	///现在需要扫描quote以确定是否合法
	if( (quote.length ()-1)/2*2==(quote.length ()/2*2))
	{//odd
		strQuote=quote+quote.at(quote.length ()-1);
	}
	else
	{
		strQuote=quote;
	}	
	
	CHSTACK		stackQuote;			//record current quote stack

	char	ch;
	const char*	pStrPointer=str.c_str();	
	while ( length > curPos && ((maxSection < 0 ) ||( maxSection > 0 && int(result.size()) < maxSection ) ))
	{
		//ch=str.at(curPos);
		ch=pStrPointer[curPos];
		
		if(POSOK(escape.find(ch)))
		{//如果当前字符是转义字符，直接分析下一个
			curPos+=2;
			if( curPos > length )
				curPos=length;
		}
		else
		{
			if(stackQuote.size ()<=0)
			{
				//首先检查这个字符是不是分割符号
				if ( POSOK(delimiter.find (ch)))
				{//如果这个字符是分隔符号
					std::string	strTemp=str.substr (lastPos,curPos-lastPos+1);					
					lastPos=++curPos;
					if(TrimExtra (strTemp,trimDelimiter)&&strTemp.length ()>0)
						result.push_back (strTemp);
				}
				else if(POSOK(strQuote.find (ch)))
				{
					STATTR st;
					st._ch=ch;
					st._index=strQuote.find (ch);
					stackQuote.push (st);
					curPos++;
				}
				else
				{//如果这个字符不是分隔符号
					curPos++;
				}
			}
			else
			{
				//如果当前的stackQuote不为空，那么就代表当前字符串仍然是一个整体
				//首先判断当前的字符是否是quote字符
				std::string::size_type posQuote=strQuote.rfind (ch);
				if(POSOK(posQuote))
				{
					if(stackQuote.top ()._index+1==(int)posQuote)//
					{//匹配quote字符
						//弹出顶层的字符，用以匹配
						stackQuote.pop();
					}
				}
				//当前字符串仍为一个整体
				curPos++;				
			}
		}		
	}
	std::string	strTemp=str.substr ( lastPos );	
	if(TrimExtra (strTemp,trimDelimiter)&&strTemp.length ()>0)
		result.push_back (strTemp);
	return true;
}

std::vector<std::string> ZQ_COMMON_API split(const std::string& src, char sep, size_t counts) {
	std::vector<std::string> result;

	if(src.empty()) {
		return result;
	}

	std::string sub = src;
	std::string::size_type pos = sub.find_first_of(sep);

	size_t cnt = 0;
	while(pos != std::string::npos) {
		result.push_back(sub.substr(0, pos));
		sub = sub.substr(pos+1); 	

		if(++cnt == counts) {
			break;
		}

		pos = sub.find_first_of(sep);
	}
	if(!sub.empty()) {
		result.push_back(sub);
	}

	if(result.empty()) {
		result.push_back(sub);
	}

	return result;
}

std::vector<std::string> ZQ_COMMON_API rsplit(const std::string& src, char sep, size_t counts) {
	std::vector<std::string> result;

	if(src.empty()) {
		return result;
	}

	std::vector<std::string> tmp;

	std::string sub = src;
	std::string::size_type pos = sub.find_last_of(sep);

	size_t cnt = 0;
	while(pos != std::string::npos) {
		tmp.push_back(sub.substr(pos+1));
		sub = sub.substr(0, pos); 	

		if(++cnt == counts) {
			break;
		}

		pos = sub.find_last_of(sep);
	}
	if(!sub.empty()) {
		tmp.push_back(sub);
	}

	if(tmp.empty()) {
		tmp.push_back(sub);
	}

	std::vector<std::string>::reverse_iterator iter = tmp.rbegin();
	for(; iter != tmp.rend(); ++iter) {
		result.push_back(*iter);
	}

	return result;
}

}//namespace stringHelper
}//namespace common
}//namespace ZQ
