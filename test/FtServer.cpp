#include "../FtTcpComm.h"

#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <stdio.h>

#define TYPE 22
#define INST 1
#define PORT 1970

using namespace ZQ::common;

BOOL WINAPI ConsoleHandler(DWORD CEvent);
bool bQuit = false;

int main( )
{
    printf("\n\"Ctrl-C\" at any time to exit the program.\n\n");
    TCPSTATUS tcpRet = TCPSTATE_SUCCESS;

	if (::SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE)==FALSE)
	{
		// unable to install handler... 
		// display message to the user
		printf("Unable to install handler!\n");
		return -1;
	}

	FtHostAddress laddr;
	laddr += "192.168.0.138";
	laddr += "192.168.80.8";

	FtTcpListener Srv(TYPE, INST);

	Srv.listen(PORT, laddr);

// Sleep until I come along and stop the program
    while (!bQuit)
    {
		::Sleep(200);
    }

	Srv.close();

    return 0;
}

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
    switch(CEvent)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
		bQuit = true;
        break;

    }
    return TRUE;
}
