#include "eloop.h"
#include <iostream>
#include "SystemUtils.h"
//#include "uv.h"


int main(void)
{
	ZQ::eloop::CpuInfo cpuinfo;
	
	ZQ::eloop::CpuInfo::cpu_info* cpu1 = NULL;
	ZQ::eloop::CpuInfo::cpu_info* cpu2 = NULL;
	double percent;
	int cpuCount = cpuinfo.getCpuCount();

	int idle = 0,total = 0;
	printf("cpu count = %d\n",cpuCount);
	cpu1 = cpuinfo.getCpuInfo();

	while(1)
	{
		cpu2 = cpuinfo.getCpuInfo();
		for(int i = 0;i<cpuCount;i++)
		{
			idle = cpu2[i].cpu_times.idle;
			total = cpu2[i].cpu_times.user + cpu2[i].cpu_times.sys + cpu2[i].cpu_times.nice + cpu2[i].cpu_times.irq + cpu2[i].cpu_times.idle;
			percent =100*(1-(float)idle/total);
			std::cout <<"cpu " << i <<" usage percent: "<< percent << "%"<<std::endl;
		}
		
		SYS::sleep(3000);
		cpu2 = cpuinfo.getCpuInfo();
		std::cout <<"------------------------------------------------------------------------------------"<<std::endl;
	}

	return 0;
}