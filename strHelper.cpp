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

bool ZQ_COMMON_API	TrimExtra(std::string& str, const std::string& strExtra)
{
	if(str.length ()<=0)
		return false;

	std::string::size_type	posBegin=0,posEnd=0;
//	std::string::size_type	curPos=0;
	std::string::size_type	length=str.length ();
	std::string	ch;
	//���Ȳ���begin
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
										const std::string& delimiter,
										const std::string& trimDelimiter,
										const std::string& quote,
										const std::string& escape,
										int maxSection)
{
	result.clear ();
	if(str.empty ())
		return false;

	int		curPos=0;	//current position
	int		lastPos=0;	//last position
					
	int		length=(int)str.length ();

	std::string			strQuote;
	///������Ҫɨ��quote��ȷ���Ƿ�Ϸ�
	if( (quote.length ()-1)/2*2==(quote.length ()/2*2))
		strQuote=quote+quote.at(quote.length ()-1); //odd
	else
		strQuote=quote;
	
	CHSTACK		stackQuote;			//record current quote stack

	char	ch;
	const char*	pStrPointer=str.c_str();	
	while ( length > curPos && ((maxSection < 0 ) ||( maxSection > 0 && int(result.size()) < maxSection ) ))
	{
		//ch=str.at(curPos);
		ch=pStrPointer[curPos];
		
		if(POSOK(escape.find(ch)))
		{//�����ǰ�ַ���ת���ַ���ֱ�ӷ�����һ��
			curPos+=2;
			if( curPos > length )
				curPos=length;
				
			continue;
		}

		if (stackQuote.size() <= 0)
		{
			//���ȼ������ַ��ǲ��Ƿָ����
			if (POSOK(delimiter.find(ch)))
			{ //�������ַ��Ƿָ����
				std::string strTemp = str.substr(lastPos, curPos - lastPos + 1);
				lastPos = ++curPos;
				if (TrimExtra(strTemp, trimDelimiter) && strTemp.length() > 0)
					result.push_back(strTemp);
			}
			else if (POSOK(strQuote.find(ch)))
			{
				STATTR st;
				st._ch = ch;
				st._index = strQuote.find(ch);
				stackQuote.push(st);
				curPos++;
			}
			else
			{ //�������ַ����Ƿָ����
				curPos++;
			}
		}
		else
		{
			//�����ǰ��stackQuote��Ϊ�գ���ô�ʹ���ǰ�ַ�����Ȼ��һ������
			//�����жϵ�ǰ���ַ��Ƿ���quote�ַ�
			std::string::size_type posQuote = strQuote.rfind(ch);
			if (POSOK(posQuote))
			{
				if (stackQuote.top()._index + 1 == (int)posQuote) //
				{												  //ƥ��quote�ַ�
					//����������ַ�������ƥ��
					stackQuote.pop();
				}
			}
			//��ǰ�ַ�����Ϊһ������
			curPos++;
		}
	}

	std::string	strTemp=str.substr ( lastPos );	
	if(TrimExtra (strTemp,trimDelimiter)&&strTemp.length ()>0)
		result.push_back (strTemp);

	return true;
}

std::vector<std::string> ZQ_COMMON_API split(const std::string& src, char sep, size_t counts) {
	std::vector<std::string> result;

	if(src.empty())
		return result;

	std::string sub = src;
	std::string::size_type pos = sub.find_first_of(sep);

	size_t cnt = 0;
	while(pos != std::string::npos) {
		result.push_back(sub.substr(0, pos));
		sub = sub.substr(pos+1); 	

		if(++cnt == counts)
			break;

		pos = sub.find_first_of(sep);
	}

	if(!sub.empty())
		result.push_back(sub);

	if(result.empty())
		result.push_back(sub);

	return result;
}

std::vector<std::string> ZQ_COMMON_API rsplit(const std::string& src, char sep, size_t counts) {
	std::vector<std::string> result;

	if(src.empty())
		return result;

	std::vector<std::string> tmp;

	std::string sub = src;
	std::string::size_type pos = sub.find_last_of(sep);

	size_t cnt = 0;
	while(pos != std::string::npos) {
		tmp.push_back(sub.substr(pos+1));
		sub = sub.substr(0, pos); 	

		if(++cnt == counts)
			break;

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
	
	std::string valstr = str;

	size_t pos = valstr.find_first_not_of(" \t\r\n");
	if (std::string::npos != pos)
		valstr.erase(0, pos);

	pos = valstr.find_last_not_of(" \t\r\n");
	if (std::string::npos != pos)
		valstr.erase(pos+1, valstr.length());

	std::transform(valstr.begin(), valstr.end(), valstr.begin(), tolower);

	std::string field;
	double val =0.0f;
	//type 1.Hex transform,if str contain illegal word ,return 0
	if (!strncmp(&valstr[0],"0x",2))//Hex,supports format of 0x[NUM],such as: 0xFFF,0X123
	{	
		pos= valstr.find("0x");
		//step 1. remove "0x" in str
		valstr = valstr.substr(pos+2);
		//step 2. start transform
		return strtol(valstr.c_str(), NULL ,16);
	}
	//type 2.Dec transform,inlclude 1000 or 1024
	if (std::string::npos != (pos= valstr.find_first_of("kmg")))//Dec,supports format of [NUM]g[NUM]m[NUM]k,such as: 1.2g2m5k, 3kB
	{	
		//step 1. confirm 1000 or 1024
		int level = 1000;
		if (!strncmp(&valstr[valstr.size()-1],"b",1))
		{
			valstr.erase(valstr.size()-1, 1);
			level = 1024;
		}
		//step 2. start transform
		if (std::string::npos != (pos= valstr.find('g')))
		{
			//get the str before the unit
			field = valstr.substr(0, pos);
			//transform
			val += atof(field.c_str());
			valstr = valstr.substr(pos+1);

			if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
				valstr = valstr.substr(pos);
		}
		val*=level;
		if (std::string::npos != (pos= valstr.find('m')))
		{
			//get the str before the unit
			field = valstr.substr(0, pos);
			//transform
			val += atof(field.c_str());
			valstr = valstr.substr(pos+1);

			if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
				valstr = valstr.substr(pos);
		}
		val*=level;
		if (std::string::npos != (pos= valstr.find('k')))
		{
			//get the str before the unit
			field = valstr.substr(0, pos);
			//transform
			val += atof(field.c_str());
			valstr = valstr.substr(pos+1);

			if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
				valstr = valstr.substr(pos);
		}
		val*=level;
		//after transfer g,m,k,use the standard strtolong
		if(!valstr.empty())
		{
			if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
				valstr = valstr.substr(pos);
			val+=atof(valstr.c_str());
		}

		return (long)val;
	}
	
	//type 3.standard strtolong
	return atol(str);
}

bool associateMacros(std::string &str, std::map<std::string, std::string> macros)
{
	size_t nResolved =0, nResolvedOfRound =0;

	do {
		nResolved += nResolvedOfRound;
		nResolvedOfRound =0;
		if((nResolved) > 256)
		{
			// PPLOG(ZQ::common::Log::L_ERROR, CLOGFMT(Preprocessor, "macro[%s] nested too much"), str.c_str());
			// throwf<PreprocessException>(EXFMT(PreprocessException, "macro[%s] nested too much"), str.c_str());
			return false;
		}

		std::string::size_type pos_macro_begin =0, pos_macro_end =0;
		for (pos_macro_begin =0; std::string::npos !=(pos_macro_begin = str.find("${", pos_macro_begin)); pos_macro_begin = pos_macro_end + 1)
		{
			// get macro string
			if(std::string::npos == (pos_macro_end = str.find_first_of('}', pos_macro_begin))) // not a valid macro reference
			{
				// PPLOG(ZQ::common::Log::L_WARNING, CLOGFMT(Preprocessor, "unpaired brackets in macro[%s]"), str.c_str());
				break;
			}

			std::string macro = str.substr(pos_macro_begin, pos_macro_end + 1 - pos_macro_begin); // include the end '}'

			// try to fixup the macro
			std::map< std::string, std::string >::const_iterator cit_macro = macros.find(macro);
			if(macros.end() == cit_macro)
			{
				// PPLOG(ZQ::common::Log::L_WARNING, CLOGFMT(Preprocessor, "unassociated macro[%s] referenced"), macro.c_str());
				continue;
			}

			str.replace(pos_macro_begin, macro.size(), cit_macro->second);
			nResolvedOfRound++;
		}

	} while(nResolvedOfRound >0);

	return true;
}

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

bool keywordReduce(const std::vector< std::string >& texts, std::vector< std::string >& keywords, const char* wordDelimitor, const char* kwDelimitor)
{
	keywords.clear();
	std::vector< std::vector<std::string> > tokens;
	for (size_t i=0; i < texts.size(); i++)
	{
		std::vector<std::string> strtkn;
		std::string trimchs = std::string(" \t\n\r") + (wordDelimitor?wordDelimitor:"");
		SplitString(texts[i], strtkn, wordDelimitor, trimchs.c_str());
		tokens.push_back(strtkn);
	}

	size_t flags =0;
	std::vector<std::string> filter;
	for (int i=0; i < tokens.size(); i++)
	{
		std::vector<std::string>& st = tokens[i];
		for (size_t j =0; j < st.size(); j++)
		{
			if (filter.size() <=j)
			{
				filter.push_back(st[j]);
				continue;
			}

			size_t flag = (1<<j);
			if (0 == (flags & flag) && filter[j].compare(st[j]))
				flags |= (1<<j);
		}
	}

	if (filter.size() <=0 || 0==flags)
		return false;

	std::vector< std::string > result;

	for (int i=0; i < tokens.size(); i++)
	{
		std::vector<std::string>& st = tokens[i];
		std::string key;
		for (size_t j =0; j < st.size(); j++)
		{
			if (0 == ((1<<j) & flags))
				continue;

			key += st[j] + kwDelimitor;
		}

		if (key.length()<2)
		{
			char buf[20]; sprintf(buf, "kw%d", i);
			key = buf;
		}
		else key = key.substr(0, key.length() -strlen(kwDelimitor));

		keywords.push_back(key);
	}

	return true;
}

}//namespace stringHelper
}//namespace common
}//namespace ZQ
