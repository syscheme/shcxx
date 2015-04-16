#include "../MiniDump.h"

void IllegalOp1()
{
    _asm {cli};
}

void IllegalOp2()
{
	char* p= NULL;

	p[0] = 0;
	int a = 1;
}

ZQ::common::MiniDump dumper("c:\\temp");

void main()
{
	IllegalOp1();
/*	char aa[10], b;
	for (int i=0; 1; i++)
		b= aa[i];*/
}