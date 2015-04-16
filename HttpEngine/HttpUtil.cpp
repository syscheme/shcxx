#include "HttpUtil.h"
#include <boost/algorithm/string/trim.hpp>

namespace ZQHttp{
namespace Util{
void split(std::vector<std::string>& v, const std::string& s, const std::string d)
{
    v.clear();

    std::string::size_type pos_from = 0;
    while((pos_from = s.find_first_not_of(d, pos_from)) != std::string::npos)
    {
        std::string::size_type pos_to = s.find_first_of(d, pos_from);
        if(pos_to != std::string::npos)
        {
            v.push_back(s.substr(pos_from, pos_to - pos_from));
        }
        else
        {
            v.push_back(s.substr(pos_from));
            break;
        }
        pos_from = pos_to;
    }
}

std::string trim(const std::string& s, const std::string& d)
{
    return boost::trim_copy_if(s, boost::is_any_of(d));
}

std::string int2str(int i)
{
    char buf[12] = {0};
	sprintf(buf,"%d",i);
	return buf;
//    return itoa(i, buf, 10);
}

static void kmpPreprocess(const std::vector<char>& x, std::vector<int>& kmpNext);
void DataStreamHelper::setTarget(const std::string& target)
{
    _nLocked = 0;
    _target = target;
    kmpPreprocess(std::vector<char>(_target.begin(), _target.end()), _kmpNext);
}
// reset the search state
void DataStreamHelper::reset()
{
    _nLocked = 0;
}
// the search result won't include null pointer unless the input data is null.
// return true for reach the target
bool DataStreamHelper::search(const char* data, size_t len, SearchResult& result)
{
    int m = _target.size();
    int i = _nLocked;
    size_t j = 0;
    // kmp algorithm
    while(j < len)
    {
        while(i > -1 && _target[i] != data[j])
            i = _kmpNext[i];
        ++i;
        ++j;
        if(i >= m)
        { // found
            break;
        }
    }

    // not found
    int vPos = _nLocked + j - i;
    if(vPos > (int)_nLocked)
    { // all locked part released
        result.released.data = _target.data();
        result.released.size = _nLocked;
        result.prefix.data = data;
        result.prefix.size = j - i;
    }
    else
    { // only first bytes of the locked part released
        result.released.data = _target.data();
        result.released.size = vPos;
        result.prefix.data = data;
        result.prefix.size = 0;
    }
    result.suffix.data = data + j;
    result.suffix.size = len - j;

    result.locked.data = _target.data();
    result.locked.size = i;

    if(i >= m)
    { // found
        _nLocked = 0; // discard the locked part
        return true;
    }
    else
    { // not found
        _nLocked = i;
        return false;
    }
}

LineCache::LineCache()
  : _got(false)
{
    _dsh.setTarget("\r\n");
}
const char* LineCache::getLine(const char* &data, size_t &len)
{
    if(_got)
    {
        return _line.c_str();
    }

    DataStreamHelper::SearchResult sr;
    _got = _dsh.search(data, len, sr);
    Append_Search_Result(_line, sr);
    data = sr.suffix.data;
    len = sr.suffix.size;
    return _got ? _line.c_str() : NULL;
}
void LineCache::clear()
{
    _line.clear();
    _got = false;
    _dsh.reset();
}

// implement the Knuth-Morris-Pratt algorithm
static void kmpPreprocess(const std::vector<char>& x, std::vector<int>& kmpNext)
{
    size_t m = x.size();
    kmpNext.clear();
    kmpNext.resize(m + 1);

    size_t i = 0;
    int j = kmpNext[0] = -1;

    while(i < m)
    {
        while(j > -1 && x[i] != x[j])
            j = kmpNext[j];
        ++i;
        ++j;
        if(i < m && x[i] == x[j])
            kmpNext[i] = kmpNext[j];
        else
            kmpNext[i] = j;
    }
}
}}

