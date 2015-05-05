
#include "tailtrigger.h"
#include <iostream>
#include <windows.h>
#include <stdlib.h>

class TailTriggerTest: public ZQ::common::TailTrigger
{
public:
	virtual void OnAction(const char* strFileName, const char* strSyntax, const char* strLine, const std::vector<std::string>& arrParam)
	{
		std::cout<<"file:"<<strFileName<<"syntax:"<<strSyntax<<"line:"<<strLine<<std::endl;

		for (int i = 0; i < arrParam.size(); ++i)
		{
			std::cout<<arrParam[i]<<std::endl;
		}
	}
};

int main(void)
{
	TailTriggerTest ttt;

	ttt.setLogFile("c:\\icm.log");

	ttt.push_back("(([0-9]+)/([0-9]+) ([0-9]+):([0-9]+):([0-9]+):([0-9.]+)) (.*)");

	ttt.run();

	system("pause");
	return 0;
}

