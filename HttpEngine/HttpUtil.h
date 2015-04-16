#ifndef __ZQ_HttpEngine_Util_H__
#define __ZQ_HttpEngine_Util_H__
#include "ZQ_common_conf.h"
#include <string>
#include <vector>
#include <functional>

namespace ZQHttp{
namespace Util{

void split(std::vector<std::string>& v, const std::string& s, const std::string d = " ");
std::string trim(const std::string& s, const std::string& d = " ");
std::string int2str(int i);

class ICaseLess: std::binary_function<std::string, std::string, bool> 
{
public:
    result_type operator()( const first_argument_type& a, const second_argument_type& b) const
    {
#ifdef ZQ_OS_MSWIN
        return (stricmp(a.c_str(), b.c_str()) < 0);
#else
		return (strcasecmp(a.c_str(), b.c_str()) < 0);
#endif
    }
};

class DataStreamHelper
{
public:
    struct Data
    {
        const char* data;
        size_t size;
        Data():data(NULL), size(0){}
        void clear()
        {
            data = NULL;
            size = 0;
        }
    };
    struct SearchResult
    {
        Data released;
        Data prefix;
        Data locked;
        Data suffix;

        void clear()
        {
            released.clear();
            prefix.clear();
            locked.clear();
            suffix.clear();
        }
    };
public:
    void setTarget(const std::string& target);
    // reset the search state
    void reset();
    // the search result won't include null pointer unless the input data is null.
    // return true for reach the target
    bool search(const char* data, size_t len, SearchResult& result);
private:
    std::string _target;
    std::vector<int> _kmpNext;
    size_t _nLocked;
};

#define Append_Search_Result(d, sr) {\
    d.append(sr.released.data, sr.released.size);\
    d.append(sr.prefix.data, sr.prefix.size);\
}

class LineCache
{
public:
    LineCache();
    const char* getLine(const char* &data, size_t &len);

    void clear();
private:
    std::string _line;
    bool _got;
    DataStreamHelper _dsh;
};

//////////////////////////////////
// helper class for counting
#ifdef ZQ_OS_MSWIN

class AtomicCounter
{
public:
    AtomicCounter():_c(0){}
    long incr() { return ::InterlockedIncrement(&_c); }
    long decr() { return ::InterlockedDecrement(&_c); }
    void reset() { ::InterlockedExchange(&_c, 0); }
    long get() { return _c; }
private:
    LONG _c;
};

#else

using __gnu_cxx::__atomic_add;
using __gnu_cxx::__exchange_and_add;

class AtomicCounter {

public:

    explicit AtomicCounter(long val=0):value_(val) {}
    long incr() { __atomic_add(&value_, 1); return get(); }
    long decr() { return __exchange_and_add(&value_, -1) - 1; return get(); }
    void reset() { __exchange_and_add(&value_, 0); }
    long get() { return __exchange_and_add(&value_, 0); }

private:

    AtomicCounter(AtomicCounter const &);
    AtomicCounter& operator=(AtomicCounter const &);

    mutable _Atomic_word value_;
};

#endif

}}
#endif

