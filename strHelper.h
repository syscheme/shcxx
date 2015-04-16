
#ifndef _ZQ_COMMON_STRING_HELPER_H__
#define _ZQ_COMMON_STRING_HELPER_H__
/************************************************************************/
/*
This file afford some helpful function operation for string
*/
/************************************************************************/
#include "ZQ_common_conf.h"
#include <string>
#include <vector>
#include <map>
namespace ZQ
{
namespace common
{
namespace stringHelper
{

typedef std::vector<std::string>			STRINGVECTOR;
typedef std::map<std::string,std::string>	STRINGMAP;

///trim the extra character
///@return true if success false if fail
///@param str string to trim
///@param strExtra extra string pattern
bool	ZQ_COMMON_API TrimExtra(std::string& str,const std::string& strExtra=(" \t"));


///split string
///@return true if success false if fail
///@param str the string for splitting
///@param delimiter used for split string
///@param quote which enclosed a whole string
///@param escape escape char
///@param trimDelimiter trim the specified char if it is the leading or trailing character
bool	ZQ_COMMON_API SplitString(const std::string& str ,std::vector<std::string>& result, 
													const std::string& delimiter=(" ,;"),
													const std::string& trimDelimiter=(" ,;\t\""),
													const std::string& quote=("\""),
													const std::string& escape=("\\"),
													int maxSection = -1);


/// split string from left to right
///@return empty vector if failed
///@param src source string
///@param sep seperator defaults to blank space
///@param count count number of splits will occur, defaults to no limit
std::vector<std::string> ZQ_COMMON_API split(const std::string& src, char sep=' ', size_t counts=0); 

/// split string from right to left
std::vector<std::string> ZQ_COMMON_API rsplit(const std::string& src, char sep=' ', size_t counts=0); 


}//stringHelper
}//namespace common
}//namespace ZQ


#endif//_ZQ_COMMON_STRING_HELPER_H__
