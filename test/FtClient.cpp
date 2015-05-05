#include "../FtTcpComm.h"

#include <tchar.h>
#include <stdio.h>
#include <time.h>

#define CLI_COUNT   1

BOOL g_bReleasing=FALSE;
#ifdef _DEBUG
extern "C" extern DWORD g_dwTraceLevel;
#else
const DWORD             g_dwTraceLevel = SEA_TRACE_NONE;    // release code
#endif

BOOL GetCommand()
{
    BOOL bRet = TRUE;
    char lpsz[64] = {"0"};

    while (99 != atoi(lpsz) )
    {
        printf("\nWelcome to the FtComm Initiator Command Shell\n\n");
        printf(" 1 = List Connections    2 = Stop Connection    3 = Start Connection\n");
        printf("99 = Exit Command Shell\n Enter \"exit\" to leave the program\n");
        scanf("%s",lpsz);

        if (0 == stricmp("exit", lpsz) )
            return FALSE;

        switch (atoi(lpsz) )
        {
        case 1: printf("List Connections\n");
            break;
        case 2: printf("You enter a connection number and I stop that connection\n");
            break;
        case 3: printf("You tell me port number and remote node name and I connect\n");
            break;
        case 99: printf("Bustin'\n");
            break;
        default: printf("Nah, I don't think so.  Try again...\n");
            break;
        }
    }

    return bRet;
}

using namespace ZQ::common;

BOOL WINAPI ConsoleHandler(DWORD CEvent);
bool bQuit = false;

int main(int argc, char** argv )
{
    printf("\n\"Ctrl-C\" at any time followed by a empty line to exit the program.\n\n");
    TCPSTATUS tcpRet = TCPSTATE_SUCCESS;

	if (::SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE)==FALSE)
	{
		// unable to install handler... 
		// display message to the user
		printf("Unable to install handler!\n");
		return -1;
	}

	FtHostAddress local;
	local += "192.168.0.138";
	local += "192.168.80.8";
	FtHostAddress remote;
	remote += "192.168.0.138";
	remote += "192.168.80.8";

	FtTcpConnection conn(22, 1, NULL);

	conn.connect(1970, local, remote);

	char lpsz[100];
	while (!bQuit)
	{
		lpsz[0] = '\0';
		// printf("\nEnter \"cmd\" for a command shell (logging will be turned off).\n\n");
		scanf("%s", lpsz);
		conn.send((BYTE*)lpsz, strlen(lpsz));
		if (0 == stricmp("e", lpsz))
			break;
	}

	conn.close();
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
