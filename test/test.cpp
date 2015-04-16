
#include "cmdline.h"

int main(int argc, char* argv[])
{
	ZQ::common::CmdLine op(":a:b:c:d");
	int ch;
	while ((ch = op.ReadOpt(argc, argv)) != EOF)
	{
		switch (ch)
		{
		case '?':
			printf("unknown argment\n");
			break;
		case 'a':
			printf("get Opt -a Arg %s\n", op.ReadArg());
			break;
		case 'b':
			printf("get Opt -b Arg %s\n", op.ReadArg());
			break;
		case 'c':
			printf("get Opt -c Arg %s\n", op.ReadArg());
			break;
		case 'd':
			printf("get Opt -d Arg %s\n", op.ReadArg());
			break;
		}
	}

	system("pause");
	return 0;
}
