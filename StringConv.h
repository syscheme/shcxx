
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
// Dev  : Microsoft Developer Studio
// Name  : ZqStringConv.h
// Author: XiaoBai (daniel.wang@i-zq.com  Wang YuanOu)
// Date  : 2005-4-20
// Desc  : converting string between unicode format and ansi format
//
// Revision History:
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/StringConv.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 8     05-04-20 20:10 Daniel.wang
// 
// 7     05-04-20 20:08 Daniel.wang
// 
// 6     05-04-20 19:59 Daniel.wang
// 
// 5     05-04-20 18:38 Daniel.wang
// 
// 4     05-04-20 17:35 Daniel.wang
// 
// 3     05-04-20 17:33 Daniel.wang
// ===========================================================================

#ifndef _ZQ_STRING_CONV_H_
#define _ZQ_STRING_CONV_H_

#include <assert.h>
#include "ZQ_common_conf.h"

		//convert character from wide-char to ansi-char
		inline char wctoac(wchar_t c)
		{
			if (c >= 0x00ff)
			{
				return char(0x7f);
			}
			return char(c);
		}

		//convert character from ansi-char to wide-char
		inline wchar_t actowc(char c)
		{
			return wchar_t(c);
		}

		//convert string from wide-string to ansi-string
		inline std::string wstoas(const wchar_t* str)
		{
			assert(str);

			std::string strResult;
			for (const wchar_t* pStr = str; *pStr != 0; ++pStr)
			{
				strResult.append(1, wctoac(*pStr));
			}
			return strResult;
		}

		//convert string from ansi-string to wide-string
		inline std::wstring astows(const char* str)
		{
			assert(str);
			
			std::wstring strResult;
			for (const char* pStr = str; *pStr != 0; ++pStr)
			{
				strResult.append(1, actowc(*pStr));
			}
			return strResult;
		}
		
		namespace ZQ
		{
			namespace common
			{
				inline std::string Ws2As(const wchar_t* str)
				{
					return wstoas(str);
				}

				inline std::wstring AS2Ws(const char* str)
				{
					return astows(str);
				}
			}
		}
		
#ifdef _UNICODE
#	define cctoac(c) (wctoac(c))
#	define cctowc(c) (c)
#	define actocc(c) (actowc(c))
#	define wctocc(c) (c)
#else
#	define cctoac(c) (c)
#	define cctowc(c) (actowc(c))
#	define actocc(c) (c)
#	define wctocc(c) (wctoac(c))
#endif//_UNICODE
		
#ifdef _UNICODE
#	define cstoas(str) wstoas(str)
#	define cstows(str) (str)
#	define astocs(str) astows(str)
#	define wstocs(str) (str)
#else
#	define cstoas(str) (str)
#	define cstows(str) astows(str)
#	define astocs(str) (str)
#	define wstocs(str) wstoas(str)
#endif//_UNICODE


#ifdef _UNICODE
		typedef wchar_t char_t;
#else
		typedef char char_t;
#endif//_UNICODE

typedef std::basic_string<char_t> string_t;

#ifdef _UNICODE
#	define f_strlen wcslen
#	define f_sprintf swprintf
#	define f_strcat wcscat
#	define f_strchr wcschr
#	define f_strcmp wcscmp
#	define f_strcoll wcscoll
#	define f_stricoll _wcsicoll
#	define f_strncoll _wcsncoll
#	define f_strnicoll _wcsnicoll
#	define f_strcpy wcscpy
#	define f_strcspn wcscspn
#	define f_strdup _wcsdup
#	define f_stricmp _wcsicmp
#	define f_strlwr _wcslwr
#	define f_strncat wcsncat
#	define f_strncmp wcsncmp
#	define f_strncpy wcsncpy
#	define f_strnicmp _wcsnicmp
#	define f_strnset _wcsnset
#	define f_strpbrk wcspbrk
#	define f_strrchr wcsrchr
#	define f_strrev _wcsrev
#	define f_strset _wcsset
#	define f_strspn wcsspn
#	define f_strstr wcsstr
#	define f_strtok wcstok
#	define f_strupr _wcsupr
#	define f_strxfrm wcsxfrm
#	define f_vsprintf vswprintf
#	define f_fprintf fwprintf
#	define f_fdopen _wfdopen
#	define f_fgetc fgetwc
#	define f_fgetchar _fgetwchar
#	define f_fgets fgetws
#	define f_fopen _wfopen
#	define f_fputc fputwc
#	define f_fputchar _fputwchar
#	define f_fputs fputws
#	define f_freopen _wfreopen
#	define f_fscanf fwscanf
#	define f_fsopen _wfsopen
#	define f_getc getwc
#	define f_getchar getwchar
#	define f_gets _getws
#	define f_printf wprintf
#	define f_putc putwc
#	define f_putchar putwchar
#	define f_puts _putws
#	define f_scanf wscanf
#	define f_snprintf _snwprintf
#	define f_sscanf swscanf
#	define f_tempnam _wtempnam
#	define f_tmpnam _wtmpnam
#	define f_ungetc ungetwc
#	define f_vfprintf vfwprintf
#	define f_vprintf vwprintf
#	define f_vsnprintf _vsnwprintf
#	define f_snprintf _snwprintf
#else
#	define f_fputs fputs
#	define f_fputchar _fputchar
#	define f_fprintf fprintf
#	define f_fdopen _fdopen
#	define f_fgetc fgetc
#	define f_fgetchar _fgetchar
#	define f_fgets fgets
#	define f_fopen fopen
#	define f_fputc fputc
#	define f_freopen freopen
#	define f_fscanf fscanf
#	define f_fsopen _fsopen
#	define f_getc getc
#	define f_getchar getchar
#	define f_gets gets
#	define f_printf printf
#	define f_putc putc
#	define f_putchar putchar
#	define f_puts puts
#	define f_scanf scanf
#	define f_snprintf _snprintf
#	define f_sscanf sscanf
#	define f_tempnam _tempnam
#	define f_tmpnam tmpnam
#	define f_ungetc ungetc
#	define f_vfprintf vfprintf
#	define f_vprintf vprintf
#	define f_vsnprintf _vsnprintf
#	define f_strlen strlen
#	define f_sprintf sprintf
#	define f_strncat strncat
#	define f_strlwr _strlwr
#	define f_stricmp _stricmp
#	define f_strdup _strdup
#	define f_strcspn strcspn
#	define f_strcpy strcpy
#	define f_strnicoll _strnicoll
#	define f_strncoll _strncoll
#	define f_stricoll _stricoll
#	define f_strcoll strcoll
#	define f_strcmp strcmp
#	define f_strchr strchr
#	define f_strcat strcat
#	define f_strrchr strrchr
#	define f_strpbrk strpbrk
#	define f_strnset _strnset
#	define f_strncpy strncpy
#	define f_strncmp strncmp
#	define f_strnicmp _strnicmp
#	define f_strrev _strrev
#	define f_strset _strset
#	define f_strspn strspn
#	define f_strstr strstr
#	define f_strtok strtok
#	define f_vsprintf vsprintf
#	define f_strupr _strupr
#	define f_strxfrm strxfrm
#endif//_UNICODE

#endif//_ZQ_STRING_CONV_H_
