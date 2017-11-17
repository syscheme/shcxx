//#pragma warning(disable:4786)
#include "strHelper.h"
#include <stack>
extern "C" {
#include <errno.h>
}

#include <algorithm>


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

long ZQ_COMMON_API str2long(const char* str)
{	
	if(str== NULL || str[0] == '\0')
		return 0;

	return atol(str);
}

//long ZQ_COMMON_API str2long(const char* str)
//		{	
//			if(str== NULL || str[0] == '\0')
//				return 0;
//
//			char* data =NULL;
//			{
//				std::string tmp = str;
//				TrimExtra(tmp);
//				data = strdup(tmp.c_str());
//			}
//			// data = skipWhiteSpace(data);
//			if(NULL == data || *data == '\0')
//			{
//				free(data);
//				return 0;
//			}
//
//			char* pDigitBegin = data;
//			size_t dotCount = 0;
//			while(*data != '\0')
//			{
//				if(isdigit(*data) || *data == '.') {
//					if(*data == '.') {
//						dotCount++;
//					}
//					data++;						
//					continue;
//				}
//				break;
//			}
//
//			char* pDigitEnd = data;
//
//			if(dotCount > 1)
//			{
//				free(data);
//				errno = ERANGE; //this is the only error we can returned
//				return LONG_MAX;
//			}
//
//			char tmp = *pDigitEnd;
//			*pDigitEnd = '\0';
//			double ret = atof(pDigitBegin);
//			*pDigitEnd = tmp;
//			data = skipWhiteSpace(data);
//			if(*data == '\0')
//			{
//				/*			if(dotCount > 0) {
//				free(pDigitBegin);
//				errno = ERANGE;
//				return LONG_MAX;
//				}		
//				*pDigitEnd = '\0';
//				*/
//				long ret = atol(pDigitBegin);		
//				free(pDigitBegin);
//				return ret;
//			}
//
//			char nextCh = *(data+1);
//			bool f1024Mode = nextCh == 'i' || nextCh == 'I';
//			size_t shiftCount = 0;
//			switch(*data)
//			{
//			case 'k':
//			case 'K':
//				shiftCount = 1;
//				break;
//
//			case 'm':
//			case 'M':
//				shiftCount = 2;
//				break;
//
//			case 'g':
//			case 'G':
//				shiftCount = 3;
//				break;
//
//			default:
//				shiftCount = 0;
//				break;
//			}
//
//			long factor = 1;
//			for(size_t i = 0 ; i < shiftCount; i++ )
//				factor = factor * (f1024Mode ? 1024 : 1000);
//
//			*pDigitEnd = '\0';
//			if(dotCount>0) {
//				//float
//				double dblData = atof(pDigitBegin);
//				dblData = dblData * factor;
//				ret = (long)dblData;
//			} 
//			else {
//				//long		
//				ret = atol(pDigitBegin);
//				ret = ret * factor;		
//			}
//
//			free(pDigitBegin);
//			return (long)ret;
//		}


long ZQ_COMMON_API  str2msec(const char* str)
{	
	// supports format of: [Ndays][Mmin][Xsec], such as 1.5days, 4.0hours, 1.5secs 4.1minutes 1hr50sec123, 1234, 1.234
	if (str== NULL || str[0] == '\0')
		return 0;

	std::string valstr = str;

	size_t pos = valstr.find_first_not_of(" \t\r\n");
	if (std::string::npos != pos)
		valstr.erase(0, pos);

	pos = valstr.find_last_not_of(" \t\r\n");
	if (std::string::npos != pos)
		valstr.erase(pos+1, valstr.length());

	if (std::string::npos == (pos = valstr.find_first_not_of("0123456789")))
		return str2long(valstr.c_str());

	std::transform(valstr.begin(), valstr.end(), valstr.begin(), tolower);

	std::string field;
	double val =0.0f;
	if (std::string::npos != (pos= valstr.find('d'))) // day
	{
		field = valstr.substr(0, pos);
		val += atof(field.c_str());
		valstr = valstr.substr(pos+1);

		if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
			valstr = valstr.substr(pos);
	}
	val*=24;

	if (std::string::npos != (pos= valstr.find('h'))) // hour
	{
		field = valstr.substr(0, pos);
		val += atof(field.c_str());
		valstr = valstr.substr(pos+1);

		if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
			valstr = valstr.substr(pos);
	}
	val*=60;

	if (std::string::npos != (pos= valstr.find('m'))) // minute
	{
		field = valstr.substr(0, pos);
		val += atof(field.c_str());
		valstr = valstr.substr(pos+1);

		if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
			valstr = valstr.substr(pos);
	}
	val*=60;

	field ="";
	if (std::string::npos != (pos= valstr.find('s')))
	{
		field = valstr.substr(0, pos);
		valstr = valstr.substr(pos+1);
	}
	else if (std::string::npos != (pos= valstr.find('.')))
	{
		pos= valstr.find_first_not_of("0123456789", pos+1);
		field = valstr.substr(0, pos);
		valstr = (std::string::npos != pos) ? valstr.substr(pos+1) :"";
	}

	if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
		valstr = valstr.substr(pos);

	if (!field.empty()) // second
		val += atof(field.c_str());

	val*=1000;

	// the millisecond
	val += atol(valstr.c_str());

	return (long) val;
}


}//namespace stringHelper
}//namespace common
}//namespace ZQ
