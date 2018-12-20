
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

static const char* const Data_cxx_Version =
    "$Id: Data.cxx,v 1.74 2001/06/27 22:56:20 bko Exp $";


//Authors: Sunitha Kumar, Cullen Jennings

//#include <cstdio>
//#include <ctype.h>

//#include "cpLog.h"
#include "Data.hxx"
#include <iostream>
#if !USE_INLINE_COWDATA

// define this to use the StringData.hxx (using C++ strings) as the
// underlying mechanism
//#ifdef WIN32
#define USE_STRING_DATA 1
#define USE_COPY_ON_WRITE_DATA 0
//#else
//#define USE_STRING_DATA 0


// define this to use the CopyOnWriteData.hxx (thread-safe internal
// buffer representation) as the underlying mechanism
//#define USE_COPY_ON_WRITE_DATA 1
//#endif //WIN32

#if USE_STRING_DATA
# include "StringData.hxx"
typedef StringData DataImplType;
#else
# if USE_COPY_ON_WRITE_DATA
# include "CopyOnWriteData.hxx"
typedef CopyOnWriteData DataImplType;
# else
# include "NewData.hxx"
typedef NewData DataImplType;
# endif 
#endif

#include "DataException.hxx"


Data::Data()
{
    dataImpl__ = new DataImplType;
}


Data::Data( const char* str, int length )
{
    dataImpl__ = new DataImplType(str, length);
}


Data::Data( const char* str )
{
    dataImpl__ = new DataImplType(str);
}


Data::Data( const string& str)
{
    dataImpl__ = new DataImplType(str);
}


Data::Data( const mstring& mstr)
{
    dataImpl__ = new DataImplType(mstr);
}


Data::Data( int value)
{
    dataImpl__ = new DataImplType(value);
}


Data::Data( const Data& data )
{
    dataImpl__ 
	= new DataImplType( *(static_cast < DataImplType* > (data.dataImpl__)) );
}


Data&
Data::operator=( const char* str )
{
    static_cast < DataImplType* > (dataImpl__)->operator=(str);
    return *this;
}


Data&
Data::operator=( const Data& data )
{
    static_cast < DataImplType* > (dataImpl__)->
	operator=(*(static_cast < DataImplType* > (data.dataImpl__)));
    return *this;
}


const char*
Data::getData() const
{
    return static_cast < DataImplType* > (dataImpl__)->getData();
}


const char*
Data::getDataBuf() const
{
    return static_cast < DataImplType* > (dataImpl__)->getDataBuf();
}


char
Data::getChar(int i) const
{
    return static_cast < DataImplType* > (dataImpl__)->getChar(i);
}


void
Data::setchar(int i, char c)
{
    static_cast < DataImplType* > (dataImpl__)->setchar(i, c);
}


char
Data::operator[](int i)
{
    return static_cast < DataImplType* > (dataImpl__)->operator[](i);
}


int
Data::length() const
{
    return static_cast < DataImplType* > (dataImpl__)->length();
}


void
Data::setBufferSize(int size)
{
    static_cast < DataImplType* > (dataImpl__)->setBufferSize(size);
}


bool
Data::operator==(const char* str) const
{
    return static_cast < DataImplType* > (dataImpl__)->operator==(str);
}


bool
Data::operator==( const Data& data) const
{
    return static_cast < DataImplType* > (dataImpl__)->
	operator==(*(static_cast < DataImplType* > (data.dataImpl__)));
}


bool
Data::operator!=(const char* str) const
{
    return static_cast < DataImplType* > (dataImpl__)->operator!=(str);
}


bool
Data::operator!=(const Data& data) const
{
    return static_cast < DataImplType* > (dataImpl__)->operator!=(*(static_cast < DataImplType* > (data.dataImpl__)));
}


bool
operator==(const char* s, const Data& data)
{
    return static_cast < DataImplType* > (data.dataImpl__)->operator==(s);
}


bool
operator!=(const char* s, const Data& data)
{
    return static_cast < DataImplType* > (data.dataImpl__)->operator!=(s);
}


bool
Data::operator>(const Data& data) const
{
    return static_cast < DataImplType* > (dataImpl__)->
	operator>(*(static_cast < DataImplType* > (data.dataImpl__)));
}


bool
Data::operator<(const Data& data) const
{
    return static_cast < DataImplType* > (dataImpl__)->
	operator<(*(static_cast < DataImplType* > (data.dataImpl__)));
}


bool
Data::operator>(const char* data) const
{
    return static_cast < DataImplType* > (dataImpl__)->
	operator>(data);
}


bool
Data::operator<(const char* data) const
{
    return static_cast < DataImplType* > (dataImpl__)->
	operator<(data);
}


Data
Data::operator+(const Data& data) const
{
    Data tmp;
    *(static_cast < DataImplType* > (tmp.dataImpl__))
    =
        static_cast < DataImplType* > (dataImpl__)->operator+(
            *(static_cast < DataImplType* > (data.dataImpl__)));
    return ( tmp );
}


Data
Data::operator+(const char* str) const
{
    Data tmp;
    *(static_cast < DataImplType* > (tmp.dataImpl__))
    =
        static_cast < DataImplType* > (dataImpl__)->operator+(str);
    return ( tmp );
}


void
Data::operator+=(const Data& data)
{
    static_cast < DataImplType* > (dataImpl__)->
	operator+=(*(static_cast < DataImplType* > (data.dataImpl__)));
}


void
Data::operator+=(const char* str)
{
    static_cast < DataImplType* > (dataImpl__)->operator+=(str);
}


Data::~Data()
{
    delete static_cast < DataImplType* > (dataImpl__);
}


void
Data::lowercase()
{
    static_cast < DataImplType* > (dataImpl__)->lowercase();
}


void
Data::uppercase()
{
    static_cast < DataImplType* > (dataImpl__)->uppercase();
}


void
Data::erase()
{
    static_cast < DataImplType* > (dataImpl__)->erase();
}


Data::operator string() const
{
    return static_cast < DataImplType* > (dataImpl__)->operator string();
}


Data::operator const char*() const
{
    return static_cast < DataImplType* > (dataImpl__)->operator const char*();
}


Data::operator mstring() const
{
    return static_cast < DataImplType* > (dataImpl__)->operator mstring();
}


Data::operator int() const
{
    return static_cast < DataImplType* > (dataImpl__)->operator int();
}


int Data::match( const char* match,
                 Data* retModifiedData,
                 bool replace,
                 Data replaceWith)
{
    return static_cast < DataImplType* > (dataImpl__)->
        match(match,
              static_cast < DataImplType* > (retModifiedData->dataImpl__),
              replace,
              *(static_cast < DataImplType* > (replaceWith.dataImpl__)));
}


Data Data::parse(const char* match, bool* matchFail )
{
    Data x;
    *( static_cast < DataImplType* > (x.dataImpl__)) 
	= static_cast < DataImplType* > (dataImpl__)->parse(match, matchFail);
    return x;
}


Data Data::parseOutsideQuotes(const char* match, bool useQuote, bool useAngle, bool* matchFail )
{
    Data x;
    *( static_cast < DataImplType* > (x.dataImpl__)) 
	= static_cast < DataImplType* > (dataImpl__)->
        parseOutsideQuotes(match, 
                           useQuote, 
                           useAngle, 
                           matchFail);
    return x;
}


Data Data::matchChar(const char* match, char* matchedChar )
{
    Data x;
    *( static_cast < DataImplType* > (x.dataImpl__)) 
	= static_cast < DataImplType* > (dataImpl__)->matchChar(match, matchedChar);
    return x;
}


Data Data::getLine(bool* matchFail )
{
    Data x;
    *( static_cast < DataImplType* > (x.dataImpl__)) 
	= static_cast < DataImplType* > (dataImpl__)->getLine(matchFail);
    return x;
}


bool isEqualNoCase(const Data& left, const Data& right )
{
    return isEqualNoCase(
        *(static_cast < DataImplType* > (left.dataImpl__)),
        *(static_cast < DataImplType* > (right.dataImpl__)));
}



bool isEqualNoCase(const char* left, const Data& right )
{
    return isEqualNoCase(
        left,
        *(static_cast < DataImplType* > (right.dataImpl__)));
}



void Data::removeSpaces()
{
    static_cast < DataImplType* > (dataImpl__)->removeSpaces();
}
    
    
void Data::removeLWS()
{
#if !USE_STRING_DATA
    static_cast < DataImplType* > (dataImpl__)->removeLWS();
#endif
}


void
Data::expand(Data startFrom, Data findstr, Data replstr, Data delimiter)
{
    static_cast < DataImplType* > (dataImpl__)->expand(
        *static_cast < DataImplType* > (startFrom.dataImpl__),
        *static_cast < DataImplType* > (findstr.dataImpl__),
        *static_cast < DataImplType* > (replstr.dataImpl__),
        *static_cast < DataImplType* > (delimiter.dataImpl__));
}

ostream& operator<< (std::ostream& s, const Data& data)
{	
    s << (*(static_cast < DataImplType* > (data.dataImpl__)));
    return s;
}

/* Local Variables: */
/* c-file-style:"stroustrup" */
/* c-basic-offset:4 */
/* c-file-offsets:((inclass . ++)) */
/* indent-tabs-mode:nil */
/* End: */

#endif
