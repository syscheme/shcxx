// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
//
// Ident : $Id: Bz2Stream.h,v 1.13 2004/08/09 10:06:56 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : template of compressed stream
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Bz2Stream.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 1     08-12-23 14:45 Hui.shao
// created
// ===========================================================================

#ifndef	__ZQ_common_Bz2Stream_H__
#define	__ZQ_common_Bz2Stream_H__

#include "ZQ_common_conf.h"
#define BZ_NO_STDIO
#include "libbz/bzlib.h"

#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

#include <vector>
#include <iostream>
#include <algorithm>

#ifndef LIBSUFFIX
#  if defined(DEBUG) || defined(_DEBUG)
#  define LIBSUFFIX "d"
#  else
#  define LIBSUFFIX ""
#  endif
#endif
#pragma comment(lib, "libbz2" LIBSUFFIX)

namespace ZQ {
namespace common {

// -----------------------------
// template class BasicBz2StreamBuf
// -----------------------------
template<typename Elem, typename Tr = std::char_traits<Elem>, typename ElemA = std::allocator<Elem>, typename ByteT = char, typename ByteAT = std::allocator<ByteT> >	
class BasicBz2StreamBuf : public std::basic_streambuf<Elem, Tr> 
{
public:
	typedef std::basic_ostream<Elem, Tr>& ostream_reference;
    typedef ElemA char_allocator_type;
	typedef ByteT byte_type;
    typedef ByteAT byte_allocator_type;
	typedef byte_type* byte_buffer_type;
	typedef std::vector<byte_type, byte_allocator_type > byte_vector_type;
	typedef std::vector<char_type, char_allocator_type > char_vector_type;

    BasicBz2StreamBuf(ostream_reference ostream_, size_t block_size_100k_,  size_t verbosity_, size_t work_factor_, size_t buffer_size_);
	
	~BasicBz2StreamBuf() { flush(); m_ostream.flush(); m_err=BZ2_bzCompressEnd(&m_bzip2_stream);	}

	int sync()	{ if ( pptr() && pptr() > pbase()) { int c = overflow( EOF); if ( c == EOF)	return -1; } return 0; }

	int_type overflow(int_type c)
	{ 
		int w = static_cast<int>(pptr() - pbase());
		if (c != EOF) {	*pptr() = c; ++w; }
		if ( bzip2_to_stream( pbase(), w)) { setp( pbase(), epptr() - 1); return c; } else return EOF;
	}

	std::streamsize flush();
	int get_zerr() const { return m_err; }
	long get_in_size() const { return (m_bzip2_stream.total_in_hi32 << 32) + m_bzip2_stream.total_in_lo32; }
	long get_out_size() const {	return (m_bzip2_stream.total_out_hi32 << 32) + m_bzip2_stream.total_out_lo32; }

private:
	bool bzip2_to_stream( char_type*, std::streamsize);
	size_t fill_input_buffer();

	ostream_reference m_ostream;
	bz_stream m_bzip2_stream;
    int m_err;
	byte_vector_type m_output_buffer;
	char_vector_type m_buffer; 
};

// -----------------------------
// template class BasicUnBz2StreamBuf
// -----------------------------
template<typename Elem, typename Tr = std::char_traits<Elem>, typename ElemA = std::allocator<Elem>, typename ByteT = char, typename ByteAT = std::allocator<ByteT> >
class BasicUnBz2StreamBuf : public std::basic_streambuf<Elem, Tr> 
{
public:
	typedef std::basic_istream<Elem, Tr>& istream_reference;
    typedef ElemA char_allocator_type;
	typedef ByteT byte_type;
    typedef ByteAT byte_allocator_type;
	typedef byte_type* byte_buffer_type;
	typedef std::vector<byte_type, byte_allocator_type > byte_vector_type;
	typedef std::vector<char_type, char_allocator_type > char_vector_type;

    BasicUnBz2StreamBuf(istream_reference istream_, size_t verbosity_, bool small_, size_t read_buffer_size_, size_t input_buffer_size_);
	~BasicUnBz2StreamBuf() { BZ2_bzDecompressEnd(&m_bzip2_stream); }

    int_type underflow();

	istream_reference get_istream()	    { return m_istream; }
	bz_stream& get_bzip2_stream()		{ return m_bzip2_stream; }
	int get_zerr() const			    { return m_err; }

private:
	std::streamsize unbzip2_from_stream( char_type*, std::streamsize);
	void put_back_from_bzip2_stream();
	size_t fill_input_buffer();

	istream_reference m_istream;
	bz_stream m_bzip2_stream;
    int m_err;
	byte_vector_type m_input_buffer;
	char_vector_type m_buffer; 
};

// -----------------------------
// template class BasicBz2OutStreamBase
// -----------------------------
template<typename Elem, typename Tr = std::char_traits<Elem>, typename ElemA = std::allocator<Elem>, typename ByteT = char, typename ByteAT = std::allocator<ByteT> >	
class BasicBz2OutStreamBase : virtual public std::basic_ios<Elem,Tr>
{
public:
	typedef std::basic_ostream< Elem, Tr >& ostream_reference;
	typedef BasicBz2StreamBuf< Elem,Tr,ElemA,ByteT,ByteAT > bzip2_streambuf_type;

	BasicBz2OutStreamBase(ostream_reference ostream_, size_t block_size_100k_, size_t verbosity_, size_t work_factor_, size_t buffer_size_)
		: m_buf(ostream_,block_size_100k_, verbosity_, work_factor_, buffer_size_)
	{ init(&m_buf ); }
	
	bzip2_streambuf_type* rdbuf() { return &m_buf; }

private:
	bzip2_streambuf_type m_buf;
};

// -----------------------------
// template class BasicBz2InStreamBase
// -----------------------------
template<typename Elem, typename Tr = std::char_traits<Elem>, typename ElemA = std::allocator<Elem>, typename ByteT = char, typename ByteAT = std::allocator<ByteT> >	
class BasicBz2InStreamBase : virtual public std::basic_ios<Elem,Tr>
{
public:
	typedef std::basic_istream<Elem, Tr>& istream_reference;
	typedef BasicUnBz2StreamBuf<
        Elem,Tr,ElemA,ByteT,ByteAT> unbzip2_streambuf_type;

	BasicBz2InStreamBase(istream_reference ostream_, size_t verbosity_, bool small_, size_t read_buffer_size_, size_t input_buffer_size_)
		: m_buf(ostream_, verbosity_, small_, read_buffer_size_, input_buffer_size_ )
	{ init(&m_buf ); }
	
	unbzip2_streambuf_type* rdbuf() { return &m_buf; };

private:
	unbzip2_streambuf_type m_buf;
};

// -----------------------------
// template class BasicBz2OutStream
// -----------------------------
template< typename Elem, typename Tr = std::char_traits<Elem>, typename ElemA = std::allocator<Elem>, typename ByteT = char, typename ByteAT = std::allocator<ByteT> >
class BasicBz2OutStream : public BasicBz2OutStreamBase<Elem,Tr,ElemA,ByteT,ByteAT>, public std::basic_ostream<Elem,Tr>
{
public:
	typedef BasicBz2OutStreamBase< Elem,Tr,ElemA,ByteT,ByteAT> Bz2OSB;
	typedef std::basic_ostream<Elem,Tr> ostream_type;

	BasicBz2OutStream(ostream_reference ostream_, size_t block_size_100k_ = 9, size_t verbosity_ = 0, size_t work_factor_ = 30, size_t buffer_size_ = 32)
	: Bz2OSB(ostream_,block_size_100k_, verbosity_, work_factor_,buffer_size_), ostream_type(rdbuf())
	{
	};
	
	BasicBz2OutStream& add_header();
	BasicBz2OutStream& zflush()	{	flush(); rdbuf()->flush(); return *this; }
};

// -----------------------------
// template class BasicBz2OutStream
// -----------------------------
template< typename Elem, typename Tr = std::char_traits<Elem>, typename ElemA = std::allocator<Elem>, typename ByteT = char, typename ByteAT = std::allocator<ByteT> >	
class BasicBz2InStream : public BasicBz2InStreamBase<Elem,Tr,ElemA,ByteT,ByteAT>, public std::basic_istream<Elem,Tr>
{
public:
	typedef BasicBz2InStreamBase< Elem,Tr,ElemA,ByteT,ByteAT> Bz2ISB;
	typedef std::basic_istream<Elem,Tr> istream_type;
	typedef unsigned char byte_type;

	BasicBz2InStream(istream_reference istream_, size_t verbosity_ = 0, bool small_ = false, size_t read_buffer_size_ = 1024, size_t input_buffer_size_ = 1024)
	  : Bz2ISB(istream_,verbosity_, small_, read_buffer_size_, input_buffer_size_), istream_type(rdbuf())
	{}
};

// -----------------------------
// template class bzip2_iostream
// -----------------------------
typedef BasicBz2OutStream<char> Bz2OutStream;
typedef BasicBz2OutStream<wchar_t> Bz2WOutSstream;
typedef BasicBz2InStream<char> Bz2InStream;
typedef BasicBz2InStream<wchar_t> Bz2WInStream;

/////////////////////////////////////////////////////////////////////////////

// -----------------------------
// impl of class BasicBz2StreamBuf
// -----------------------------
template<typename Elem, typename Tr, typename ElemA, typename ByteT, typename ByteAT >
BasicBz2StreamBuf< Elem,Tr,ElemA,ByteT,ByteAT>:: BasicBz2StreamBuf(ostream_reference ostream_, size_t block_size_100k_, size_t verbosity_, size_t work_factor_, size_t buffer_size_)
	: m_ostream(ostream_), m_output_buffer(buffer_size_,0), m_buffer(buffer_size_,0)
{
		m_bzip2_stream.bzalloc=NULL;
		m_bzip2_stream.bzfree=NULL;

		m_bzip2_stream.next_in=NULL;
		m_bzip2_stream.avail_in=0;
		m_bzip2_stream.avail_out=0;
		m_bzip2_stream.next_out=NULL;

		m_err=BZ2_bzCompressInit(&m_bzip2_stream, std::min( 9, static_cast<int>(block_size_100k_) ), std::min( 4, static_cast<int>(verbosity_) ), std::min( 250, static_cast<int>(work_factor_) ) );

		setp( &(m_buffer[0]), &(m_buffer[m_buffer.size()-1]));
}

template<typename Elem, typename Tr, typename ElemA, typename ByteT, typename ByteAT>
bool BasicBz2StreamBuf<Elem,Tr,ElemA,ByteT,ByteAT >::bzip2_to_stream( typename BasicBz2StreamBuf<Elem,Tr,ElemA,ByteT,ByteAT >::char_type* buffer_, std::streamsize buffer_size_ )
{
	std::streamsize written_byte_size=0, total_written_byte_size = 0;

	m_bzip2_stream.next_in=(byte_buffer_type)buffer_;
	m_bzip2_stream.avail_in=buffer_size_*sizeof(char_type);
	m_bzip2_stream.avail_out=static_cast<unsigned int>(m_output_buffer.size());
	m_bzip2_stream.next_out=&(m_output_buffer[0]);
	size_t remainder=0;

	do
	{
		m_err = BZ2_bzCompress (&m_bzip2_stream, BZ_RUN );

		if (m_err == BZ_RUN_OK  || m_err == BZ_STREAM_END)
		{
			written_byte_size= static_cast<std::streamsize>(m_output_buffer.size()) - m_bzip2_stream.avail_out;
			total_written_byte_size+=written_byte_size;
			// ouput buffer is full, dumping to ostream
			m_ostream.write( 
				(const char_type*) &(m_output_buffer[0]), 
				static_cast<std::streamsize>( written_byte_size/sizeof(char_type) )
				);

			// checking if some bytes were not written.
			if ( (remainder = written_byte_size%sizeof(char_type))!=0)
			{
				// copy to the beginning of the stream
				memcpy(
					&(m_output_buffer[0]), 
					&(m_output_buffer[written_byte_size-remainder]),
					remainder);

			}

			m_bzip2_stream.avail_out=static_cast<unsigned int>(m_output_buffer.size()-remainder);
			m_bzip2_stream.next_out=&m_output_buffer[remainder];
		}
	} 
	while (m_bzip2_stream.avail_in != 0 && m_err == BZ_RUN_OK);

	return m_err == BZ_RUN_OK || m_err == BZ_FLUSH_OK;
}

template<typename Elem, typename Tr,  typename ElemA, typename ByteT, typename ByteAT >
std::streamsize BasicBz2StreamBuf<Elem,Tr,ElemA,ByteT,ByteAT >::flush()
{
	std::streamsize written_byte_size=0, total_written_byte_size=0;

	size_t remainder=0;

	do
	{
		m_err = BZ2_bzCompress (&m_bzip2_stream, BZ_FINISH);
		if (m_err == BZ_FINISH_OK || m_err == BZ_STREAM_END)
		{
			written_byte_size=
				static_cast<std::streamsize>(m_output_buffer.size()) 
				- m_bzip2_stream.avail_out;
			total_written_byte_size+=written_byte_size;
			// ouput buffer is full, dumping to ostream
			m_ostream.write( 
				(const char_type*) &(m_output_buffer[0]), 
				static_cast<std::streamsize>( written_byte_size/sizeof(char_type)*sizeof(char) )
				);

			// checking if some bytes were not written.
			if ( (remainder = written_byte_size%sizeof(char_type))!=0)
			{
				// copy to the beginning of the stream
				memcpy(
					&(m_output_buffer[0]), 
					&(m_output_buffer[written_byte_size-remainder]),
					remainder);

			}

			m_bzip2_stream.avail_out=
				static_cast<unsigned int>(m_output_buffer.size()-remainder);
			m_bzip2_stream.next_out=&(m_output_buffer[remainder]);
		}
	} while (m_err == BZ_FINISH_OK);

	m_ostream.flush();

	return total_written_byte_size;
}

// -----------------------------
// impl of class BasicUnBz2StreamBuf
// -----------------------------
template<typename Elem, typename Tr, typename ElemA, typename ByteT, typename ByteAT >
BasicUnBz2StreamBuf<Elem,Tr,ElemA,ByteT,ByteAT >::BasicUnBz2StreamBuf(istream_reference istream_, size_t verbosity_, bool small_, size_t read_buffer_size_, size_t input_buffer_size_ )
: m_istream(istream_), m_input_buffer(input_buffer_size_), m_buffer(read_buffer_size_)
{
	// setting zalloc, zfree and opaque
	m_bzip2_stream.bzalloc=NULL;
	m_bzip2_stream.bzfree=NULL;

	m_bzip2_stream.next_in=NULL;
	m_bzip2_stream.avail_in=0;
	m_bzip2_stream.avail_out=0;
	m_bzip2_stream.next_out=NULL;


	m_err=BZ2_bzDecompressInit (
		&m_bzip2_stream,
		std::min(4, static_cast<int>(verbosity_)),
		static_cast<int>(small_)
		);

	setg( &(m_buffer[0])+4,     // beginning of putback area
		&(m_buffer[0])+4,     // read position
		&(m_buffer[0])+4);    // end position    
}

template<typename Elem, typename Tr, typename ElemA, typename ByteT, typename ByteAT >
size_t BasicUnBz2StreamBuf<	Elem,Tr,ElemA,ByteT,ByteAT	>::fill_input_buffer()
{
	m_bzip2_stream.next_in=&(m_input_buffer[0]);
	m_istream.read( 
		(char_type*)(&(m_input_buffer[0])), 
		static_cast<std::streamsize>(m_input_buffer.size()/sizeof(char_type)) 
		); 
	return m_bzip2_stream.avail_in=m_istream.gcount()*sizeof(char_type);
}

template<typename Elem, typename Tr, typename ElemA, typename ByteT, typename ByteAT >
void BasicUnBz2StreamBuf<Elem,Tr,ElemA,ByteT,ByteAT >::put_back_from_bzip2_stream()
{
	if (m_bzip2_stream.avail_in==0)
		return;

	m_istream.clear( ::std::ios::goodbit );
	m_istream.seekg(
		-static_cast<int>(m_bzip2_stream.avail_in),
		::std::ios_base::cur
		);

	m_bzip2_stream.avail_in=0;
}

template<typename Elem, typename Tr, typename ElemA, typename ByteT, typename ByteAT >
typename BasicUnBz2StreamBuf<Elem,Tr,ElemA,ByteT,ByteAT>::int_type BasicUnBz2StreamBuf<Elem,Tr,ElemA,ByteT,ByteAT>::underflow() 
{ 
	if ( gptr() && ( gptr() < egptr()))
		return * reinterpret_cast<unsigned char *>( gptr());

	int n_putback = static_cast<int>(gptr() - eback());
	if ( n_putback > 4)
		n_putback = 4;
	memcpy( 
		&(m_buffer[0]) + (4 - n_putback), 
		gptr() - n_putback, 
		n_putback*sizeof(char_type)
		);

	int num = unbzip2_from_stream( 
		&(m_buffer[0])+4, 
		static_cast<std::streamsize>((m_buffer.size()-4)*sizeof(char_type))
		);
	if (num <= 0) // ERROR or EOF
		return EOF;

	// reset buffer pointers
	setg( &(m_buffer[0]) + (4 - n_putback),   // beginning of putback area
		&(m_buffer[0]) + 4,                 // read position
		&(m_buffer[0]) + 4 + num);          // end of buffer

	// return next character
	return* reinterpret_cast<unsigned char *>( gptr());    
}

template<typename Elem, typename Tr, typename ElemA, typename ByteT, typename ByteAT>
std::streamsize BasicUnBz2StreamBuf<Elem,Tr,ElemA,ByteT,ByteAT>::unbzip2_from_stream(char_type* buffer_, std::streamsize buffer_size_)
{
	m_bzip2_stream.next_out=(byte_buffer_type)buffer_;
	m_bzip2_stream.avail_out=buffer_size_*sizeof(char_type);
	size_t count =m_bzip2_stream.avail_in;

	do
	{
		if (m_bzip2_stream.avail_in==0)
			count=fill_input_buffer();

		if (m_bzip2_stream.avail_in)
		{
			m_err = BZ2_bzDecompress( &m_bzip2_stream );
		}
	} while (m_err==BZ_OK && m_bzip2_stream.avail_out != 0 && count != 0);

	if (m_err == BZ_STREAM_END)
		put_back_from_bzip2_stream();

	return buffer_size_ - m_bzip2_stream.avail_out/sizeof(char_type);
}

}} // namespaces

#endif // __ZQ_common_Bz2Stream_H__

