#include "Text.h"
#include <sstream>

namespace ZQ{
namespace common{
namespace Text{
template< class ExceptionT >
void throwf(const char *fmt, ...)
{
    char msg[2048] = {0};
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);
    throw ExceptionT(msg);
}

std::string format(const std::string& fmt, const std::map<std::string, std::string>& parameters) throw (FormattingException)
{
    std::string param; // the current param that need to be interpret
    param.reserve(100); // the parameter shouldn't very long
    std::stringstream buf;
    enum{Begin, InParam, OutParam, End} state = Begin; // the current state
    size_t i = 0;
    char c = 0; // the current character

#define NextChar() if(i < fmt.size()) c = fmt[i++]; else state = End // will be invoked after the current character been processed
    bool bContinue = true;
    while(bContinue)
    {
        switch(state)
        {
        case Begin:
            state = OutParam; // just go into the OutParam state
            NextChar(); // get the first character
            break;
        case InParam:
            if(isalpha(c))
            { // continue the InParam state
                param.push_back(c);
                NextChar();
            }
            else
            { // end of the parameter
                // we may have a parameter, resolve it
                if(!param.empty())
                {
                    std::map<std::string, std::string>::const_iterator it;
                    it = parameters.find(param);
                    if(it != parameters.end())
                    {
                        buf << it->second;
                    }
                    else
                    {
                        throwf<FormattingException>("Parameter [%s] not found. fmt=[%s]", param.c_str(), fmt.c_str());
                    }
                    param.clear();
                }
                else
                {
                    throwf<FormattingException>("Bad format definition at position [%d]. fmt=[%s]", i, fmt.c_str());
                }
                state = OutParam; // the current c haven't been processed yet
            }
            break;
        case OutParam:
            if(c == '$')
            {
                state = InParam; // start of the parameter
            }
            else
            {
                buf << c; // just copy
            }
            NextChar();
            break;
        case End:
            // we may have a parameter, resolve it
            if(!param.empty())
            {
                std::map<std::string, std::string>::const_iterator it;
                it = parameters.find(param);
                if(it != parameters.end())
                {
                    buf << it->second;
                }
                else
                {
                    throwf<FormattingException>("Parameter [%s] not found. fmt=[%s]", param.c_str(), fmt.c_str());
                }
                param.clear();
            }
            bContinue = false; // all work done
            break;
        }
    }
    return buf.str();
}

////////////////////////////////////////////
/// trim function
std::string& trimLeft(std::string& s, const std::string& chs)
{
    std::string::size_type n = s.find_first_not_of(chs);
    if(std::string::npos == n) s.clear(); else s.substr(n).swap(s);
    return s;
}
std::string trimLeftCopy(const std::string& s, const std::string& chs)
{
    std::string::size_type n = s.find_first_not_of(chs);
    return std::string::npos == n ? "" : s.substr(n);
}

std::string& trimRight(std::string& s, const std::string& chs)
{
    std::string::size_type n = s.find_last_not_of(chs);
    s.erase(std::string::npos == n ? 0 : n + 1);
    return s;
}
std::string trimRightCopy(const std::string& s, const std::string& chs)
{
    std::string::size_type n = s.find_last_not_of(chs);
    return s.substr(0, std::string::npos == n ? 0 : n + 1);
}

std::string& trim(std::string& s, const std::string& chs)
{
    trimRight(s, chs);
    trimLeft(s, chs);
    return s;
}
std::string trimCopy(const std::string& s, const std::string& chs)
{
    std::string::size_type nFrom = s.find_first_not_of(chs);
    if(std::string::npos != nFrom)
    {
        std::string::size_type nTo = s.find_last_not_of(chs);
        if(std::string::npos != nTo)
        {
            return s.substr(nFrom, (nTo + 1 - nFrom));
        }
    }
    return "";
}

}}} // namspace ZQ::common::Text
