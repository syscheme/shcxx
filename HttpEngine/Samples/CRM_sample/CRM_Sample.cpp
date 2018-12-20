// CRM_Sample.cpp : Defines the entry point for the DLL application.
//

#include <CRMInterface.h>
#include <windows.h>
#include <sstream>

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

class EchoPage: public CRG::IContentHandler
{
public:
    virtual void onRequest(const CRG::IRequest* req, CRG::IResponse* resp)
    {
        std::ostringstream buf;
        buf << "<HTML><HEAD><TITLE>Echo Page</TITLE></HEAD><BODY><div>";
        buf << "request page: " << req->uri() << "</div></BODY></HTML>";
        std::string content = buf.str();
        resp->setContent(content.data(), content.size());
    }
};
EchoPage gEchoPage;
extern "C"
{
    __declspec(dllexport) bool CRM_Entry_Init(CRG::ICRMManager* mgr)
    {
        mgr->registerContentHandler("/", &gEchoPage);
        return true;
    }

    __declspec(dllexport) void CRM_Entry_Uninit(CRG::ICRMManager* mgr)
    {
        mgr->unregisterContentHandler("/", &gEchoPage);
    }
}
#ifdef _MANAGED
#pragma managed(pop)
#endif

