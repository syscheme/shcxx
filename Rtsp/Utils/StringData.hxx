#ifndef STRINGDATA_H_
#define STRINGDATA_H_

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

static const char* const StringDataHeaderVersion =
    "$Id: StringData.hxx,v 1.56 2001/06/27 22:56:20 bko Exp $";

//Authors: Sunitha Kumar, Cullen Jennings

#include <cstring>
#include <string>
#include "mstring.hxx"
#include "VException.hxx"
#include "DataException.hxx"

/** This will eventually implemented the copy on write, reference
    counted smart buffer scheme
*/
class StringData
{

    public:
        // creation members.
        ///
        //StringData( const string& str);
        ///
        StringData( );
        ///
        StringData( const char* str );
        ///
        StringData( const char* buffer, int length );
        ///
        StringData( const StringData& data );
        ///
        StringData( const string& str);
        ///
        StringData( const mstring& mstr);
        ///
        StringData( const int value);

        ///
        ~StringData();

        ///
        bool operator>(const StringData& ) const ;
        ///
        bool operator<(const StringData& ) const;

        ///
        StringData& operator=(const char* str);
        ///
        StringData& operator=(const StringData& data);
        ///
        //Data& operator=(const string& str);

        //access members
        ///
        //string getData() const;


        /// getData returns a NUL terminated (e.g. a C string) buffer
        const char* getData() const;

        /** getDataBuf differs from getData in that the resultant buffer is NOT
        NUL terminated */
        const char* getDataBuf() const;

        ///
        //void substr( int start, int length , char* str) const;
        //char* substr( int start, int length ) const;
        ///
        char getChar( int i ) const;  //return the i'th char of string.
        ///
        void setchar( int i, char c );  //write to the i'th char of string.
        ///
        char operator[]( int i );  //return the i'th char of string.
        ///
        int length() const;
        ///
        bool operator==( const char* str ) const;
        ///
        bool operator==( const StringData& data ) const;
        ///
        //bool operator==(const string& second );
        ///
        bool operator!=( const char* str ) const;
        ///
        bool operator!=( const StringData& data ) const;
        ///
        //bool operator!=(const string& str);
        ///
        StringData operator+( const StringData& data) const;
        ///
        StringData operator+( const char* str) const;
        ///
        //Data& operator+(const Data& firstsrc, const string& secondsrc);

        //modification members
        ///
        void operator+=(const StringData&);
        ///
        void operator+=(const char*);

        ///
        void erase();
        ///
        //Data& operator+=(const string&);

        ///
        //size_t hashfn() ;

        ///
        size_t hashfn() const;

        // conversion operators
        ///
        operator string() const;

        ///
        operator const char*() const;

        ///
        operator mstring() const;

        ///
        operator int() const;
        //destruction

        /// match
        int match(StringData match,
                              StringData* data,
                              bool replace = false,
                              StringData replaceWith = "");

        /*
          ///
          //returns the starting pos of match in this data item.
          // if replace is true, then, the matched string is replaced with the 
          //replaceWith string.
          //if match string is not found, then MATCH_NOT_FOUND exception is thrown.
          int  match( const Data& match, 
          Data* retModifiedData, 
          bool replace=false, 
          const Data& replaceWith="");
        */

        /// removes spaces before and after a string.
        void removeSpaces();

        ///expand requird for expandin headers
        void expand(StringData startFrom, StringData findstr, StringData replstr, StringData delimiter);

        /// do a case-insensitive match
        friend bool isEqualNoCase( const StringData& left, const StringData& right ) ;

        ///
        void deepCopy (const StringData &src, char ** bufPtr = 0, int *bufLenPtr = 0);

        ///
        int find( const StringData& match, int start = 0 );

	/// convert this Data to lower case
	void lowercase();
	/// convert this Data to upper case
	void uppercase();

	/** 
	    match (and eat) the first contiguous block composed of the
	    characters in match, which is outside of double quotes <">
	    and angle brackets "<" and ">". Returned is the data
	    before the matched characters.  If no characters match,
	    return the empty Data.  If matchFail is set to a bool ptr,
	    the bool *matchFail will be set to true if the match
	    fails, and false otherwise.

            This is designed for use in separating a list of
            parameters at the commas (e.g. Contact:)
	*/
	StringData parseOutsideQuotes(const char* match, 
				      bool useQuote,
				      bool useAngle,
				      bool* matchFail = 0 );

	/** 
	    match (and eat) the first contiguous block composed of the
	    characters in match. Returned is the data before the
	    matched characters.  If no characters match, return the
	    empty Data.  If matchFail is set to a bool ptr, the bool
	    *matchFail will be set to true if the match fails, and
	    false otherwise.
	*/
	StringData parse(const char* match, bool* matchFail = 0 );
	/** match (and eat) any one of the characters in match.  If
	    matchedChar points to a char, it will be set to the
	    matching character, or \0 if not matched to anything.
	    Returns characters before the match, or the empty string
	    if no characters match.
	*/
	StringData matchChar(const char* match, char* matchedChar = 0);
	/** get the next line in the text, delimited by \r\n or \n .
	    Differs from parse("\r\n", matchFail) in that if there is
	    a blank line (which has the contiguous text \r\n\r\n),
	    parse will merely skip the empty line, while getLine will
	    return the empty line as an empty Data.
	*/
	StringData getLine(bool* matchFail = 0 );

        /// remove leading white space.
        void removeLWS();

        /** suggest a size for the underlying buffer (optimizes
         ** performance in some implementations)
         */
        void setBufferSize(int size)
	{
	}

	/// output operator 
	friend ostream& operator<<(ostream& s, const StringData& data);


    private:

        //mutable string buf;
        mutable size_t hash_val;
        mutable bool hash_valid;
        string buf;
};

#endif
