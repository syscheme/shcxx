#include "eloop.h"
#include "ZQResource.h"
#include "FileLog.h"

// -----------------------------
// dummy DllMain()
// -----------------------------
BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  uReason,
	LPVOID lpReserved
	)
{
	switch (uReason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
