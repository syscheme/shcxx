#ifndef _X_TCHAR_H_
#define _X_TCHAR_H_

#ifdef UNICODE
#error No implemetion for UNICODE
#else

#ifndef _T
#define _T(x)	(x)
#endif // #ifndef _T

typedef const char* LPCTSTR;
typedef char* LPTSTR;

#define _tcsnicmp			strnicmp
#define _tcsinc(s)			(s+1)
#define _tcsstr				strstr
#define _tcslen(s)			((s) == NULL ? 0 : strlen((s)))
#define _stprintf			sprintf
#define _vstprintf			vsprintf
#define _tcsftime			strftime
#endif // #ifdef UNICODE

#endif // #ifndef _X_TCHAR_H_
