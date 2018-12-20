#ifndef __Algorithm_Selector_H__
#define __Algorithm_Selector_H__
#include <list>
#include <string>
#include <boost/regex.hpp>

///////////////////////////////////////////////
// special map with regular expression support

template <class ValType>
class selector
{
public:
    bool set(const std::string& syntax, ValType* val)
    {
        if(syntax.empty()) // bad expression
            return false;

        typename Rules::const_iterator it;
        for(it = _rules.begin(); it != _rules.end(); ++it)
        {
            if(syntax == it->syntax)
            { // the syntax has already been registered
                return false;
            }
        }

        Rule rule;
        rule.syntax = syntax;
        rule.value = val;
        try
        {
            rule.matcher.assign(syntax);
        }
        catch( const boost::bad_expression&)
        { // bad expression
            return false;
        }

        _rules.push_back(rule);
        return true;
    }

    ValType* get(const std::string& key)
    {
        typename Rules::const_iterator it;
        for(it = _rules.begin(); it != _rules.end(); ++it)
        {
            boost::smatch m;
            if(boost::regex_match(key, m, it->matcher))
            {
                return it->value;
            }
        }
        return NULL;
    }

    bool remove(const std::string& syntax)
    {
        typename Rules::iterator it;
        for(it = _rules.begin(); it != _rules.end(); ++it)
        {
            if(syntax == it->syntax)
            {
                _rules.erase(it);
                return true;
            }
        }
        return false;
    }

    void clear()
    {
        _rules.clear();
    }
private:
    struct Rule
    {
        std::string syntax;
        boost::regex matcher;
        ValType* value;
        Rule():value(NULL){}
    };
    typedef std::list<Rule> Rules;
    Rules _rules;
};
#endif
