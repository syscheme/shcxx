
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */





static const char* const StringData_cxx_Version =
    "$Id: StringData.cxx,v 1.67 2001/06/29 03:56:00 bko Exp $";


//Authors: Sunitha Kumar, Cullen Jennings

#include <cstdio>
#include <ctype.h>

#include "cpLog.h"
#include "StringData.hxx"
#include "DataException.hxx"
#if USE_HASH_MAP
#include <hash_map>
#endif
#include <cctype>
#include <iostream>
using namespace std;

#define INLINE_ inline

static INLINE_ 
bool isIn(char c, const char* match)
{
    char p;

    while((p = *match++))
    {
	if(p == c)
	{
	    return true;
	}
    }

    return false;
}


StringData::StringData():
        hash_val(0),
        hash_valid(false)
{}



StringData::StringData( const char* str, int length ):
        hash_val(0),
        hash_valid(false),
        buf(str, length)
{

}


StringData::StringData( const char* str ):
        hash_val(0),
        hash_valid(false)
{
    buf = str;
}


StringData::StringData( const string& str):
        hash_val(0),
        hash_valid(false)
{
    buf = str;
}


StringData::StringData( const mstring& mstr):
        hash_val(0),
        hash_valid(false)
{
    buf = mstr;
}


StringData::StringData( int value):
        hash_val(0),
        hash_valid(false)
{
    char str[256];
    sprintf(str, "%d", value);
    buf = str;
}


StringData::StringData( const StringData& data )
{
    buf = data.buf;
    hash_val = (data.hash_val);
    hash_valid = (data.hash_valid);
}


StringData&
StringData::operator=( const char* str )
{
    buf = str;
    hash_valid = false;
    return (*this);
}


StringData&
StringData::operator=( const StringData& data )
{
    if (this != &data)
    {
        buf = data.buf;
        hash_valid = false;
    }

    return (*this);
}


const char*
StringData::getData() const
{
    return buf.c_str();
}


const char*
StringData::getDataBuf() const
{
    return buf.c_str();
}


#if 0
void StringData::substr( int start, int length, char* deststr ) const
{
    if ( ( start < 0 ) || (length < 1) )
    {
        cpLog(LOG_ERR, "StringData: substr: incorrect start or length parameters");
        throw DataAccessError(OUT_OF_RANGE);
    }
    else
    {
        char* str = buffer->getBuffer();
        for ( *deststr = *str; *str != '\0'; deststr++, str++, *deststr = *str );

        //assign the null char.
        *deststr = '\0';
    }
}
#endif


char
StringData::getChar(int i) const
{
    if (( i < 0 ) || (i > static_cast < int > (buf.size()) ) )
    {
        cpLog(LOG_ERR, "StringData:getchar: i is out of range.");
        throw DataException(
            "i is out of range",
            __FILE__,
            __LINE__
        );
        return '\0';
    }

    return buf[i];
}


void StringData::setchar(int i, char c)
{
    hash_valid = false;

    if (( i < 0) || (i > static_cast < int > (buf.size()) ) )
    {
        cpLog(LOG_ERR, "StringData:setchar: i is out of range.");
        throw DataException(
            "i is out of range",
            __FILE__,
            __LINE__
        );

    }
    else
    {
        buf[i] = c;
    }
}


char
StringData::operator[](int i)
{
    char ch;
    ch = getChar(i);
    return ch;
}


int
StringData::length() const
{
    return buf.size();
}


bool StringData::operator==(const char* str) const
{
    return ( buf == str );
}


bool StringData::operator==( const StringData& data) const
{
#if 1 
    return ( buf == data.buf );
#else
    if (data.hashfn() == hashfn())
    {
        return ( buf == data.buf );
    }
    else
    {
        return false;
    }
#endif
}


bool StringData::operator!=(const char* str) const
{
    return ( buf != str);
}


bool StringData::operator!=(const StringData& data) const
{
    return ( buf != data.buf );

#if 0
    if (data.hashfn() == hashfn())
    {
        return ( buf != data.buf );
    }
    else
    {
        return true;
    }
#endif
}


bool
StringData::operator>(const StringData& data) const
{
    return ( buf > data.buf );
}


bool
StringData::operator<(const StringData& data) const
{
    return ( buf < data.buf );
}


StringData
StringData::operator+(const StringData& data) const
{
    return ( buf + data.buf );
}


StringData
StringData::operator+(const char* str) const
{
    return ( buf + str );
}


void
StringData::operator+=(const StringData& data)
{
    hash_valid = false;
    buf += data.buf;
}


void
StringData::operator+=(const char* str)
{
    hash_valid = false;
    buf += str;
}


StringData::~StringData()
{}



void
StringData::erase()
{
    buf.erase();
    hash_valid = false;
}


StringData::operator string() const
{
    return buf;
}


StringData::operator const char*() const
{
    return buf.c_str();
}


StringData::operator mstring() const
{
    return mstring(buf);
}


StringData::operator int() const
{
    size_t l = buf.length();
    int val = 0;
    size_t p = 0;

    while (l--)
    {
        char c = buf[p++];
        if ((c >= '0') && (c <= '9'))
        {
            val *= 10;
            val += c - '0';
        }
        else
        {
            return val;
        }
    }
    return val;
}

#if USE_HASH_MAP
size_t
StringData::hashfn() const
{
    hash < const char* > x;
    return x(getData());

    if (!hash_valid)
    {
        hash < const char* > x;
        hash_val = x(buf.c_str());
        hash_valid = true;
    }
    return hash_val;
}
#endif
// size_t
// Data::hashfn() const
// {
//     assert (hash_valid == true);
//     return hash_val;
// }


int StringData::find( const StringData& match, int start)
{
    return static_cast < int > (buf.find(match.buf,
                                         static_cast < string::size_type > (start)));
}



int StringData::match( StringData match, StringData* retModifiedData, bool replace, StringData replaceWith)
{
#if 1
    int retVal;
    string::size_type pos = buf.find(match.getData());

    if (pos == string::npos)
    {
        cpLog(LOG_DEBUG_STACK, "Match not found");
        return NOT_FOUND;
    }

    string::size_type replacePos = pos + match.length();
    retVal = FIRST;

    if (retModifiedData)
    {
        (*retModifiedData) = buf.substr(0, pos);
        if (retModifiedData->length()) retVal = FOUND;
    }

    if (replace)
    {
        if (replacePos <= buf.size() )
        {
            hash_valid = false;
            buf.replace(0, replacePos, replaceWith.getData());
        }
        else
        {
            printf("buf =<%s> match =<%s>\n", buf.c_str() , match.getData() );
            printf("pos=%d  match.len=%d replacePos=%d  buf.size()= %d\n",
                   pos,
                   match.length(),
                   replacePos,
                   buf.size() );

        }
    }

    return retVal;
#else

    int pos = static_cast < int > (buf.find(match.getData()));

    if (pos == static_cast < int > (string::npos))
    {
        cpLog(LOG_DEBUG_STACK, "Match not found");
        pos = NOT_FOUND;
    }
    else
    {
        if (data)
        {
            (*data) = buf.substr(0, pos);
        }
        if (replace)
        {
            hash_valid = false;
            buf.replace(0, (pos + match.length()), replaceWith.getData());
        }
        else
        {
        }

        if ( (data->length() == 0) && (pos != ( (int) string::npos) ) )
        {
            //the match string is the first item.
            pos = -2;
            pos = FIRST;
        }

    }
    if ( (pos != NOT_FOUND) && (pos != FIRST) )
    {
        pos = FOUND;
    }
    return pos;
#endif
}


/*
 
int  Data::match( const Data& match, Data* retModifiedData, bool replace, const Data& replaceWith)
{
    int retVal;
    string::size_type pos = buf.find(match.getData());
    
    if (pos == string::npos)
    {
        cpLog(LOG_DEBUG_STACK, "Match not found");
        return NOT_FOUND;
    }
 
    string::size_type replacePos = pos + match.length();
    retVal = FIRST;
 
    if (retModifiedData)
    {
        (*retModifiedData) = buf.substr(0, pos);
	if(retModifiedData->length()) retVal = FOUND;
    }
 
    if (replace)
    {
        buf.replace(0, replacePos, replaceWith.getData());
    }
 
    return retVal;
}
 
*/

bool isEqualNoCase(const StringData& left, const StringData& right )
{
    string::const_iterator leftIter = left.buf.begin();
    string::const_iterator rightIter = right.buf.begin();

    while ( (leftIter != left.buf.end()) && (rightIter != right.buf.end()) )
    {
        if (toupper(*leftIter) != toupper(*rightIter))
        {
            return false;
        }
        ++leftIter;
        ++rightIter;
    }

    if ( (leftIter != left.buf.end()) || (rightIter != right.buf.end()) )
    {
        // since both aren't the same length, they're not equal
        return false;
    }

    return true;
}



void StringData::removeSpaces()
{
    hash_valid = false;
    //removes spaces before and after the characters.
    //Leaves the embedded spaces as is.
    /*
     Data beforeSpace="";
     Data afterSpace="";

     int found;

    do
{
        beforeSpace="";
        found = data->match(SPACE, &beforeSpace, false);
        if (found == FIRST)
        {
            //replace with nothing
            data->match(SPACE, &beforeSpace, true);
        }

}while (found == FIRST);

    */

    if (buf.length() == 0)
    {
        return ;
    }

    string::size_type beforeval;
    do
    {
        beforeval = buf.find(SPACE);

        if (beforeval != 0)
        {
            break;
        }

        buf.replace(beforeval, strlen(SPACE) , "");


    }
    while (beforeval == 0);


    string::size_type afterval;
    do
    {
        //afterSpace="";
        //proceed to the last character in Data.
        // string tempstring = buf;
        afterval = buf.rfind(SPACE);

        //if there are chars after val, discard .
        if (afterval + 1 < buf.length())
        {
            break;
        }

        buf.replace(afterval, strlen(SPACE) , "");

        //Data tempdata(tempstring);

        //*data = tempdata;
    }

    while (afterval != string::npos);

}


void
StringData::expand(StringData startFrom, StringData findstr, StringData replstr, StringData delimiter)
{
    hash_valid = false;

    string::size_type startPos = buf.find(startFrom.getData());
    if (startPos < string::npos)
    {
        string::size_type delimPos = buf.find(delimiter.getData(), startPos);
        string::size_type findPos = buf.find( findstr.getData(), startPos);

        while (findPos < delimPos)
        {
            //found replstr, replace
            buf.replace( findPos, strlen(findstr.getData()), replstr.getData());
            //find next.
            //delimPos = buf.find( delimiter.getData(), findPos);
            delimPos = buf.find( delimiter.getData(), findPos + static_cast < string > (replstr.getData()).size() );
            findPos = buf.find( findstr.getData(), findPos);
        }
    }
}



void
StringData::lowercase()
{
    for(string::iterator i = buf.begin() ; 
	i != buf.end() ;
	++i)
    {
#ifndef WIN32
	*i = std::tolower(*i);
#else
	*i = tolower(*i);
#endif
    }
}

void 
StringData::uppercase()
{
    for(string::iterator i = buf.begin() ; 
	i != buf.end() ;
	++i)
    {
#ifndef WIN32
	*i = std::toupper(*i);
#else
	*i = toupper(*i);
#endif
    }
}


StringData 
StringData::parseOutsideQuotes(const char* match, 
			       bool useQuote,
			       bool useAngle,
			       bool* matchFail /* default argument */ )
{
    size_t pos = 0;

    bool inDoubleQuotes = false;
    bool inAngleBrackets = false;

    bool foundAny = false;

    size_t bufsize = buf.length();

    while(!foundAny && (pos < bufsize))
    {
	char p = buf[pos];

        switch (p)
        {
        case '"':
            if(!inAngleBrackets && useQuote)
            {
                inDoubleQuotes = !inDoubleQuotes;
            }
            break;
        case '<':
            if(!inDoubleQuotes && useAngle)
            {
                inAngleBrackets = true;
            }
            break;
        case '>':
            if(!inDoubleQuotes && useAngle)
            {
                inAngleBrackets = false;
            }
            break;
        default:
            break;
        }

	if(!inDoubleQuotes && !inAngleBrackets && isIn(p, match))
	{
	    foundAny = true;
	}
	pos++;
    }

    size_t pos2 = pos;

    while(
	foundAny && 
	(pos2 < bufsize) && 
	isIn(buf[pos2], match)
	)
    {
	pos2++;
    }

    StringData result;

    if(foundAny)
    {
	result = *this;
	result = buf.substr(0, pos - 1);

	buf = buf.substr(pos2, length());
	if(matchFail)
	{
	    *matchFail = false;
	}
    }
    else
    {
	if(matchFail)
	{
	    *matchFail = true;
	}
    }

    return result;
}

StringData 
StringData::parse(const char* match, bool* matchFail /* default argument */ )
{
    size_t pos = 0;

    bool foundAny = false;

    size_t bufsize = buf.length();

    while(!foundAny && (pos < bufsize))
    {
	char p = buf[pos];
	if(isIn(p, match))
	{
	    foundAny = true;
	}
	pos++;
    }

    size_t pos2 = pos;

    while(
	foundAny && 
	(pos2 < bufsize) && 
	isIn(buf[pos2], match)
	)
    {
	pos2++;
    }

    StringData result;

    if(foundAny)
    {
	result = buf.substr(0, pos - 1);

	buf = buf.substr(pos2, length());
	if(matchFail)
	{
	    *matchFail = false;
	}
    }
    else
    {
	if(matchFail)
	{
	    *matchFail = true;
	}
    }

    return result;
}

StringData 
StringData::matchChar(const char* match, 
		      char* matchedChar /* default argument */)
{
    size_t pos = 0;

    bool foundAny = false;

    size_t bufsize = buf.length();

    while(!foundAny && (pos < bufsize))
    {
	char p = buf[pos];
	if(isIn(p, match))
	{
	    foundAny = true;
	    if(matchedChar)
	    {
		*matchedChar = p;
	    }
	}
	pos++;
    }

    StringData result;

    if(foundAny)
    {
	result.buf = buf.substr(0, pos - 1);

	buf = buf.substr(pos, length());
    }
    else if(matchedChar)
    {
	*matchedChar = '\0';
    }

    return result;
}

StringData 
StringData::getLine(bool* matchFail /* default argument */ )
{
    const int STARTING = 0;
    const int HAS_CR = 1;
    const int HAS_LF = 2;
    const int HAS_CRLF = 3;

    int state = STARTING;
    size_t pos = 0;

    bool foundAny = false;

    size_t bufsize = buf.length();

    while(!foundAny && (pos < bufsize))
    {
	char p = buf[pos];
	if( p == '\r' )
	{
	    state = HAS_CR;
	}
	else if (p == '\n' )
	{
	    if(state == HAS_CR)
	    {
		state = HAS_CRLF;
	    }
	    else
	    {
		state = HAS_LF;
	    }
	    foundAny = true;
	}
	else
	{
	    state = STARTING;
	}
	pos++;
    }

    int pos2 = pos;

    if(state == HAS_CRLF)
    {
	pos--;
    }

    StringData result;

    if(foundAny)
    {
	result.buf = buf.substr(0, pos - 1);

	buf = buf.substr(pos2, buf.length());
	if(matchFail)
	{
	    *matchFail = false;
	}
    }
    else
    {
	if(matchFail)
	{
	    *matchFail = true;
	}
    }

    return result;
}

void 
StringData::removeLWS()
{
    size_t replaceTo;
    size_t pos;

    pos = buf.find ("\r\n");
    if (pos == string::npos)
    {
        pos = find("\n");
    }
    else
    {
	replaceTo = pos + 1; //should end after \r\n
    }
    if (pos == string::npos)
    {
	return;
    }
    else
    {
	replaceTo = pos; //should end after \n
    }
    bool replaceFlag = false;
    do
    {
        while ( (replaceTo + 1 < buf.length()) && 
                ( (buf[replaceTo+1] == '\t') ||
                  (buf[replaceTo+1] == ' ')
                ) 
              )
        {
	    char temp = buf[replaceTo];
            replaceTo++;
	    temp = buf[replaceTo];
            replaceFlag = true;
        }
        if (replaceFlag)
        {
            int replaceFrom = pos;
            while ( (replaceFrom-1 > 0) &&
                    ( (buf[replaceFrom-1] == '\t') ||
                      (buf[replaceFrom-1] == ' ')
                    )
                  )
            {
	        char temp = buf[replaceFrom];
                replaceFrom--;
		temp = buf[replaceFrom];
            }
            int replaceNum = replaceTo - replaceFrom;
            buf.replace(replaceFrom, replaceNum, ""); //replace with nothing
	   
        }
        //remember pos.
        size_t initpos = pos;
        pos = find("\r\n", initpos+2);
	if (pos > buf.length())
	{
	    pos = find("\n", initpos+2);
	    replaceTo = pos;
        }
	else
        {
	    replaceTo = pos+1;
        }
    }
    while (pos < buf.length());
}

ostream& 
operator<<(ostream& s, const StringData& data)
{
    s << data.buf;
    return s;
}
