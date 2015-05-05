// ===========================================================================
// Copyright (c) 2004 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// Ident : $Id: SchangeServiceAppMain.cpp,v 1.1 2004/08/05 07:08:07 wli Exp $
// Branch: $Name:  $
// Author: Kaliven.lee
// Desc  : Define entry point of Schange Service application 
//
// Revision History: 
// ---------------------------------------------------------------------------
// log definition
//
// ===========================================================================


#include <stdlib.h>
#include <crtdbg.h>
#include <memory>

#include "baseschangeserviceapplication.h"
#include "appshell.h"



using ZQ::common::BaseSchangeServiceApplication;

extern ZQ::common::BaseSchangeServiceApplication* Application;



/////////////////////////////////////////////////////////////////////////////
// Main routine
/////////////////////////////////////////////////////////////////////////////

// To run under control of the SeaChange service shell, the main routine of
// the service is named app_main(), rather than main(). The service shell
// process is started by NT as the actual service. The service shell creates
// a process to run this program. This program is linked with the appshell.lib
// module, which defines the main() routine to be executed at process startup.
// The main() routine within appshell creates a thread to run this app_main()
// routine.

// Event called by service stop entry point to exit main loop

extern "C" void app_main(int argc, char* argv[])
{
//	_CrtSetBreakAlloc(2003);	
	HRESULT hr = S_OK;

	gbAppShellSetKeepAlive = true;
	{
		//
		// As soon as possible, the app must set the APPALIVE event to tell
		// the service shell that the app has started successfully. The
		// service shell uses this to tell the NT service manager that the
		// app has successfully started.
		//
		SetEvent(handle_array[APPALIVE]);
		hr = Application->init();
		if (hr!=S_OK)
		{
			Application->unInit();
			return;
		}	

		// service app block here until recieve a stop message from appshell
		hr = Application->startService();
	}

	Application->unInit();


	return;
} // end app_main


////////////////////////////////////////////////////////////////////////////////
// app_service_control()
//
// This routine is called by the appshell when various service control manager
// events are signalled. This program, for now, does not process any of these
// events.
//
extern "C"
void app_service_control(int SCode)
{
	switch (SCode) 
	{
	case SCMgrSTOP: 
		{
			Application->OnStop();
			break;
		}
	case SCMgrPAUSE: 
		{
			Application->OnPause();
			break;
		}
	case SCMgrCONTINUE: 
		{
			Application->OnContinue();
			break;
		}
	default: 
		{
			break;
		}
	}
}

