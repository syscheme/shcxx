#include <ClientRequestGw.h>
#include <FileLog.h>

static void showUsage()
{
    printf("usage: crg help\n");
    printf("       crg load <mod list>\n");
}
static HANDLE _hStop;
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    switch(dwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        {
            SetEvent(_hStop);
        }
    }
    return true;
}

int
main(int argc, char* argv[])
{
    _hStop = CreateEvent(NULL, false, false, NULL);
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    if(argc < 2)
    {
        showUsage();
        return 1;
    }
    else
    {
        if(
            0 == stricmp(argv[1], "help") ||
            0 != stricmp(argv[1], "load") ||
            argc == 2
            )
        {
            showUsage();
            return 1;
        }
       
    }

    ZQ::common::FileLog* pLog = new ZQ::common::FileLog("CRG.log", ZQ::common::Log::L_DEBUG);
    CRG::CRGateway* svr = new CRG::CRGateway(*pLog);
    svr->setCapacity(10);
    svr->setModEnv("C:\\TianShan\\etc", "C:\\TianShan\\Logs");
    for(int iMod = 2; iMod < argc; ++iMod)
        svr->addModule(argv[iMod]);
    svr->start();
    WaitForSingleObject(_hStop, 10000);
    svr->stop();
    return 0;
}
