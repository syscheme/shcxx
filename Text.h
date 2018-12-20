#ifndef __ZQ_common_Text_H__
#define __ZQ_common_Text_H__
#include <Exception.h>
#include <string>
#include <map>

namespace ZQ{
namespace common{
namespace Text{

typedef std::map<std::string, std::string> Properties;

/// throw exception with formatted message
template< class ExceptionT >
void throwf(const char *fmt, ...) PRINTFLIKE(1, 2);

////////////////////////////////////////////////////
class FormattingException: public ZQ::common::Exception
{
public:
    FormattingException(const std::string& what_arg) throw()
        :ZQ::common::Exception(what_arg)
    {
    }
};

///////////////////////////////////////////////////
/// format a piece or text with $VAR embedded
/// throw FormattingException on failure
///
std::string format(const std::string& fmt, const Properties& parameters) throw(FormattingException);

///////////////////////////////////////////////////
/// trim function
///
std::string& trimLeft(std::string& s, const std::string& chs = " \t\r\n");
std::string trimLeftCopy(const std::string& s, const std::string& chs = " \t\r\n");

std::string& trimRight(std::string& s, const std::string& chs = " \t\r\n");
std::string trimRightCopy(const std::string& s, const std::string& chs = " \t\r\n");

std::string& trim(std::string& s, const std::string& chs = " \t\r\n");
std::string trimCopy(const std::string& s, const std::string& chs = " \t\r\n");

//////////////////////////////////////
/// split function
///
template <class StringColl>
StringColl& split(StringColl& strs, const std::string& s, const std::string& delimiter = " ")
{
    strs.clear();

    std::string::size_type pos_from = 0;
    while((pos_from = s.find_first_not_of(delimiter, pos_from)) != std::string::npos)
    {
        std::string::size_type pos_to = s.find_first_of(delimiter, pos_from);
        if(pos_to != std::string::npos)
        {
            strs.push_back(s.substr(pos_from, pos_to - pos_from));
        }
        else
        {
            strs.push_back(s.substr(pos_from));
            break;
        }
        pos_from = pos_to;
    }
    return strs;
}


//////////////////////////////////////
/// join function
///
template <class StringColl>
std::string& join(std::string& s, const StringColl& strs, const std::string& delimiter = ", ")
{
    s.clear();
    if(strs.empty()) // prevent the string array empty
        return s;

    // compute the total length of the joined string
    size_t totalLength = (strs.size() - 1) * delimiter.size();

    typename StringColl::const_iterator it;
    for(it = strs.begin(); it != strs.end(); ++it)
    {
        totalLength += it->size();
    }

    s.reserve(totalLength);
    // now join the string
    it = strs.begin();
    s.append(*it);
    ++it;
    for(; it != strs.end(); ++it)
    {
        s.append(delimiter);
        s.append(*it);
    }

    return s;
}

template <class StringColl>
std::string join(const StringColl& strs, const std::string& delimiter = ", ")
{
    std::string s;
    return join(s, strs, delimiter);
}
}}} // namspace ZQ::common::Text
#endif
