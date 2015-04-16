#ifndef DATA_HXX_
#define DATA_HXX_

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

static const char* const Data_hxx_version =
    "$Id: Data.hxx,v 1.64 2001/04/27 11:02:51 bko Exp $";

//Authors: Sunitha Kumar, Cullen Jennings

#ifdef CODE_OPTIMIZE
// set USE_INLINE_COWDATA to 1 to inline the copy-on-write
// implementation, using thread-safe buffers .
#define USE_INLINE_COWDATA 1
#else
// set USE_INLINE_COWDATA to 0 to use a wrapper class to allow easy
// changes to the code
#define USE_INLINE_COWDATA 0
#endif

#if USE_INLINE_COWDATA

#include "CopyOnWriteData.hxx"
typedef CopyOnWriteData Data;

#else

// this is the slow one

#include <string>
#include "mstring.hxx"
#include "VException.hxx"
#include "DataException.hxx"

/** Class for representing binary data and strings, in a thread-safe
 ** manner.
*/
class Data
{
    public:
        /// Default constructor
        Data( );
        /** constructor for C style strings
	 ** @param str      null-terminated (C style) character array
	 */
        Data( const char* str );
        /** constructor for character arrays with length 
	 **  @param buffer character array 
	 **  @param len    size of buffer
	 */
        Data( const char* buffer, int len );
        /// copy constructor
        Data( const Data& data );
        /** constructor for C++ strings
	 */
        Data( const string& str);
        /** constructor for mstring (a specialization of C++ strings)
	 */
        Data( const mstring& mstr);
        /// constructor that converts an int to a Data
        Data( const int value);

        /// destructor
        ~Data();

        /** compares two Data objects, returning the value of a
	 * dictionary comparison of the two strings
	 */
        bool operator>(const Data& ) const ;
        /** compares two Data objects, returning the value of a
	 * dictionary comparison of the two strings
	 */
        bool operator<(const Data& ) const;
        /** assignment operator
	 ** @param str   C string character array
	 */

        /** compares two Data objects, returning the value of a
	 * dictionary comparison of the two strings
	 */
        bool operator>(const char* ) const ;
        /** compares two Data objects, returning the value of a
	 * dictionary comparison of the two strings
	 */
        bool operator<(const char* ) const;
        /** assignment operator
	 ** @param str   C string character array
	 */

        Data& operator=(const char* str);
        /** assignment operator
	 ** @param data   Data object
	 */
        Data& operator=(const Data& data);

        /// returns a NUL terminated (a C string) buffer
        const char* getData() const;

        /** returns a pointer to the buffer.  Note that this buffer is
         ** NOT NUL terminated (not a C string).
	 */
        const char* getDataBuf() const;

        /** return one character from the string
	 ** @param i   index into the Data object
	 */
        char getChar( int i ) const;  
        /** return one character from the string
	 ** @param i   index into the Data object
	 ** @param c   character to set
	 */
        void setchar( int i, char c );
        /** return one character from the string
	 ** @param i   index into the Data object
	 */
        char operator[]( int i );
        /// length of the Data object
        int length() const;

        /** suggest a size for the underlying buffer (optimizes
         ** performance in some implementations)
         */
        void setBufferSize(int size);

        /** equality operator
	 ** @param str   C string to compare to
	 */
        bool operator==( const char* str ) const;
        /** equality operator
	 ** @param data   Data to compare to
	 */
        bool operator==( const Data& data ) const;
        /// inequality operator
        bool operator!=( const char* str ) const;
        /// inequality operator
        bool operator!=( const Data& data ) const;

        /// friend to compare a c-style string to a data (avoids conversion)
        friend bool operator==( const char* str, const Data& d );

        /// friend to compare a c-style string to a data (avoids conversion)
        friend bool operator!=( const char* str, const Data& d );

        /** concatenate two Data objects together.  Warning -- this
	 * creates an extra copy of the Data object, so it is not
	 * terribly efficient.  If possible, it is better to use +=
	 * instead.
	 */
        Data operator+( const Data& data) const;
        /** concatenate a Data object and a C-style string together.
	 * Warning -- this creates an extra copy of the Data object,
	 * so it is not terribly efficient.  If possible, it is better
	 * to use += instead.
	 */
        Data operator+( const char* str) const;

        /** append a Data object d to this Data.
	 ** this is potentially much more efficient than operator+().
	 */
        void operator+=(const Data& d);
        /** append a string s to this Data.
	 ** this is potentially much more efficient than operator+().
	 */
        void operator+=(const char* s);

	/// convert this Data to lower case
	void lowercase();
	/// convert this Data to upper case
	void uppercase();

        /// erase this object
        void erase();

        /// convert to a string
        operator string() const;

        /// convert to a C style character array
        operator const char*() const;

        /// convert to an mstring
        operator mstring() const;

        /// convert to an int (depreciated)
        operator int() const;

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
	Data parseOutsideQuotes(const char* match, 
                                bool useQuote, 
                                bool useAngle, 
                                bool* matchFail = 0 );
	/** 
	    match (and eat) the first contiguous block of the
	    characters in match. Returned is the data before the
	    matched characters.  If no characters match, return the
	    empty Data.  If matchFail is set to a bool ptr, the bool
	    *matchFail will be set to true if the match fails, and
	    false otherwise.
	*/
	Data parse(const char* match, bool* matchFail = 0);
	/** match (and eat) any one of the characters in match.  If
	    matchedChar points to a char, it will be set to the
	    matching character, or \0 if not matched to anything.
	    Returns characters before the match, or the empty string
	    if no characters match.
	*/
	Data matchChar(const char* match, char* matchedChar = 0);
	/** get the next line in the text, delimited by \r\n or \n .
	    Differs from parse("\r\n", matchFail) in that if there is
	    a blank line (which has the contiguous text \r\n\r\n),
	    parse will merely skip the empty line, while getLine will
	    return the empty line as an empty Data.
	*/
	Data getLine(bool* matchFail = 0);

        /** match the string and return the text prior to the match.
         *  If a match is found, this Data is set to the remainder
         *  after the matched string.
         *
         *  @param match         the string to be matched
         *  @param beforeMatch   the data before the matched string
         *  @param replace       whether to replace the matched data
         *  @param replaceWith   the data to replace the matched data
         */
        int match(const char* match,
                        Data* beforeMatch,
                        bool replace = false,
                        Data replaceWith = "");

        /// removes spaces before and after a string.
        void removeSpaces();

        /// remove leading white space
        void removeLWS();

        /// expand expands headers (depreciated)
        void expand(Data startFrom, Data findstr, Data replstr, Data delimiter);

        /// do a case-insensitive match
        friend bool isEqualNoCase( const Data& left, const Data& right ) ;


        /// do a case-insensitive match
        friend bool isEqualNoCase( const char* left, const Data& right ) ;

	/// output operator
	friend ostream& operator<<(ostream& s, const Data& data);

    private:
        void* dataImpl__;
};


#endif


/* Local Variables: */
/* c-file-style: "stroustrup" */
/* indent-tabs-mode: nil */
/* c-file-offsets: ((access-label . -) (inclass . ++)) */
/* c-basic-offset: 4 */
/* End: */

#endif
