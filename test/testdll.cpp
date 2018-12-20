// testdll.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void test(void)
{
	VariantXml var;

	var.setStruct("first", "hello");
	var.setStruct("second", 5);

	char strBuffer[256] = {0};
	//var.getString(strBuffer, 256);
	//std::cout<<strBuffer<<std::endl;

	var.toXml(strBuffer, 256);
	std::cout<<strBuffer<<std::endl;
}

int main(int argc, char* argv[])
{
	test();

	system("pause");
	return 0;
}

