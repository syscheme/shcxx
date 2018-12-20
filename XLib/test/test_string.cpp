// XString.cpp: implementation of the XString class.
//
//////////////////////////////////////////////////////////////////////

#include <xstring.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

typedef ZQ::XString CString;

#include <stdio.h>

int main()
{
	CString x = "   1000";
	x += "2222";
	
	CString s(x);
	x.Empty();
	x.IsEmpty();

	CString s2 = "4444466166       ";

	CString s4;
	s4 += 'c';

	s4 = 'd' + s4;

	s4 = s + s2;

	CString s5;
	s5 = s4.Right(4);
	s5.MakeUpper();
	s5.MakeLower();

	// 100022224444466166
	int n = s4.Find("661");
	n = s4.FindOneOf("65");

	bool b = s4 <= s5;
	
	printf("%s:%d:%c\n", (const char* )s4, s4.GetLength(), s4.GetAt(4));
	s4.TrimRight();
	s4.TrimLeft();
	s5.Format("Hello: %d", 10);
	printf("%s\n", (const char* )s5);
	printf("%s:%d:%c\n", (const char* )s4, s4.GetLength(), s4.GetAt(4));
	CString s6;
	s6.Empty();
	return 0;
}
