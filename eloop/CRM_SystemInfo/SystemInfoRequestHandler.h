#ifndef __SystemInfoRequestHandler_H__
#define __SystemInfoRequestHandler_H__

#include <CRMInterface.h>
#include <FileLog.h>
#include "SystemInfo.h"

class SystemInfoRequestHandler:public CRG::IContentHandler
{
public:
	SystemInfoRequestHandler(ZQ::common::FileLog& log);
	~SystemInfoRequestHandler();
	virtual void onRequest(const CRG::IRequest* req, CRG::IResponse* resp);

private:
	ZQ::common::FileLog& _log;
	ZQ::common::SystemInfo _info;

};
#endif