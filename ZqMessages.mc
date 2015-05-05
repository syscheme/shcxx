;// ===========================================================================
;// Copyright (c) 2004 by
;// ZQ Interactive, Inc., Shanghai, PRC.,
;// All Rights Reserved.  Unpublished rights reserved under the copyright
;// laws of the United States.
;// 
;// The software contained  on  this media is proprietary to and embodies the
;// confidential technology of ZQ Interactive, Inc. Possession, use,
;// duplication or dissemination of the software and media is authorized only
;// pursuant to a valid written license from ZQ Interactive, Inc.
;// 
;// This software is furnished under a  license  and  may  be used and copied
;// only in accordance with the terms of  such license and with the inclusion
;// of the above copyright notice.  This software or any other copies thereof
;// may not be provided or otherwise made available to  any other person.  No
;// title to and ownership of the software is hereby transferred.
;//
;// The information in this software is subject to change without notice and
;// should not be construed as a commitment by ZQ Interactive, Inc.
;//
;// Name  : ZqMessages.mc
;// Author : Bernie Zhao (bernie.zhao@i-zq.com  Tianbin Zhao)
;// Date  : 2005-2-18
;// Desc  : This is the message compiler source file for all ZQ facility
;//         messages. (Currently only used by Server Engineer 1)
;//
;// Revision History:
;// ---------------------------------------------------------------------------
;// $Log: $
;// ===========================================================================

;//*******************************************************************************
;//*
;//*    Header Section
;//*
;//*  There will be no typedefs for ZQ messages.  The severity names will be the
;//*    the default ones: SeverityNames=(Success= 0x0,Informational= 0x1,Warning= 0x2,
;//*  Error= 0x3). Language names will likewise default to:
;//*    LanguageNames=(English= 1:MSG00001)
;//*******************************************************************************
FacilityNames = (Service_Shell=0x0006:ZQShell Generic=0x0
   )

;//*******************************************************************************
;//*
;//*     Generic ZQ messages
;//*******************************************************************************
MessageId   =   0x1000
Facility     = Generic
Severity     = Success
SymbolicName = ZQ_GENERIC_SUCCESS
Language     = English
ZQ Service General Success:%1.
.

MessageId   =   0x1001
Facility     = Generic
Severity     = Informational
SymbolicName = ZQ_GENERIC_INFO
Language     = English
ZQ Service General Information:%1.
.

MessageId   =   0x1002
Facility     = Generic
Severity     = Warning
SymbolicName = ZQ_GENERIC_WARNING
Language     = English
ZQ Service General Warning:%1.
.

MessageId   =   0x1003
Facility     = Generic
Severity     = Error
SymbolicName = ZQ_GENERIC_ERROR
Language     = English
ZQ Service General Error:%1.
.



;//*******************************************************************************
;//*
;//*     Message Definitions for Facility Service_Shell
;//*******************************************************************************

MessageId   =   0xC000
Facility = Service_Shell
Severity = Warning
SymbolicName = SHELL_LOST_SCMGR_CNXN
Language=English
(ZQSRVSHELL)Unable to report status to Service Control Manager.
Error is %1. State is %2.
.

MessageId =     0xc001
Severity = Informational
SymbolicName = SHELL_STOPPING_SVC
Language=English
(ZQSRVSHELL)Service %1 stop requested by SCManager.
.

MessageId =     0xc002
Severity = Error
SymbolicName = SHELL_CLOSE_ERR
Language=English
(ZQSRVSHELL)Error closing handle to %1.
Error code is %2.
.

MessageId =     0xc003
Severity = Informational
SymbolicName = SHELL_NORMAL_EXIT
Language=English
(ZQSRVSHELL)Service %1 is now exiting
.

MessageId =     0xc004
Severity = Error
SymbolicName = SHELL_NOKILL
Language=English
(ZQSRVSHELL)Service application %1, PID= %2 has failed to shutdown.
%3...
.

MessageId =     0xc005
Severity = Error
SymbolicName = SHELL_NOSHUTDOWN
Language=English
%1 failed.
(ZQSRVSHELL)Unable to initiate system shutdown. Error code is %2.
.

MessageId =     0xc006
Severity = Warning
SymbolicName = SHELL_MANINIT_FAIL
Language=English
(ZQSRVSHELL)Unable to manage snmp variables. Sts = %1, GetLastError = %2.
.

MessageId =     0xc007
Severity = Warning
SymbolicName = APPSHELL_MANINIT_FAIL
Language=English
(ZQAPPSHELL)Unable to make timer variables manageable. Error is %1, GetLastError is %2.
.

MessageId = 0xc008
Severity = Error
SymbolicName = SHELL_APP_ALREADY_UP
Language=English
(ZQSRVSHELL)Service %1 wants to create application process, but application is already running.
%2...
.

;// --- obsolete for v3.0 -------------
MessageId = 0xC800
Severity = Error
SymbolicName = SHELL_NOSERVICE
Language=English
No service application name on command line. Command line is %1.
Exiting...
.

MessageId =     0xc801
Severity = Error
SymbolicName = SHELL_INITFAIL
Language=English
(ZQSRVSHELL)Unable to initialize service shell %1. Error is %2. Exiting...
.

MessageId =     0xc803
Severity = Warning
SymbolicName = SHELL_ERROR_REBOOTPARMS
Language=English
(ZQSRVSHELL)Service shell %1 failed to get configuration values for RebootTimeThreshold/RebootCountLimit from upstream node for SystemType %2
%3.
.

MessageId = 0xCC00
Severity = Error
SymbolicName = SHELL_START_PROC_FAIL
Language=English
(ZQSRVSHELL)Service %1 failed to create application process
Command line is %2.
Image is %3.
Current directory is %4.
Error is %5.
.

MessageId =     0xcc01
Severity = Error
SymbolicName = SHELL_UNEXP_EXIT
Language=English
(ZQSRVSHELL)Service application %1, PID=%2 exited unexpectedly with status=%3., ChildExit Event occurred
.

MessageId =     0xcc02
Severity = Informational
SymbolicName = SHELL_STARTING_SVC
Language=English
(ZQSRVSHELL)Service shell starting service %1 with ProcessId %2.
.

MessageId =     0xcc03
Severity = Error
SymbolicName = SHELL_TOO_MANY_RESTARTS
Language=English
(ZQSRVSHELL)Service application %1 experiencing too many restarts:
%2 starts within %3 seconds. %4...
.

MessageId =     0xcc04
Severity = Error
SymbolicName = SHELL_ALIVE_TIMEOUT
Language=English
(ZQSRVSHELL)Alive protocol has timed out for application %1, PID=%2.
Will attempt to restart. 
.

MessageId =     0xcc05
Severity = Error
SymbolicName = SHELL_UNKNOWN_WAIT_RESULT
Language=English
(ZQSRVSHELL)Unexpected completion from a wait.
Completion code is %1. Service %2 exiting...
.

MessageId = 0xcc06
Severity = Error
SymbolicName = APPSHELL_EVENT_ERROR
Language=English
(ZQAPPSHELL)Unable to %1 event %2. Error is %3. Exiting... 
.

MessageId =     0xcc07
Severity = Error
SymbolicName = APPSHELL_NO_MAIN_THREAD
Language=English
(ZQAPPSHELL)Unable to start main thread. Error is %1. Exiting...
.

MessageId =     0xcc08
Severity = Error
SymbolicName = APPSHELL_ALIVE_TIMEOUT
Language=English
(ZQAPPSHELL)Application thread did not set the alive event within %1 sec (AppStartupWait). Exiting...
.

MessageId =     0xcc09
Severity = Error
SymbolicName = APPSHELL_UNKNOWN_WAIT_RESULT
Language=English
(ZQAPPSHELL)Unexpected completion from a wait.
Completion code is %1. Exiting...
.

MessageId =     0xcc0a
Severity = Error
SymbolicName = APPSHELL_EXCEPTION
Language=English
(ZQAPPSHELL)Exception encountered in application.
Exception code is %1. Address is %2.
Binary data contain exception record and context. Exiting...
.

MessageId =     0xcc0b
Severity = Error
SymbolicName = APPSHELL_DEAD_APPTHREAD
Language=English
(ZQAPPSHELL)The main application thread (app_main) has terminated.  Exiting...
.

MessageId =     0xcc0c
Severity = Error
SymbolicName = APPSHELL_NO_CONTROL_THREAD
Language=English
(ZQAPPSHELL)Unable to start service control thread. Error is %1.
.

MessageId =     0xcc0d
Severity = Informational
SymbolicName = SHELL_PAUSED
Language=English
(ZQSRVSHELL)Application %1 received PAUSE signal.
.

MessageId =     0xcc0e
Severity = Informational
SymbolicName = SHELL_CONTINUED
Language=English
(ZQSRVSHELL)Application %1 received CONTINUE signal.
.

MessageId =     0xcc0f
Severity = Error
SymbolicName = SHELL_EXCEPTION
Language=English
(ZQSRVSHELL)Exception encountered in service shell.
Exception code is %1. Address is %2.
Binary data contain exception record and context. Exiting...
.

MessageId =     0xcc10
Severity = Informational
SymbolicName = SHELL_PAUSING_SVC
Language=English
(ZQSRVSHELL)Service %1 pause requested by SCManager.
.

MessageId =     0xcc11
Severity = Informational
SymbolicName = SHELL_CONTINUING_SVC
Language=English
(ZQSRVSHELL)Service %1 continue requested by SCManager.
.

MessageId =     0xcc12
Severity = Error
SymbolicName = APPSHELL_NO_MONITOR_THREAD
Language=English
(ZQAPPSHELL)Unable to start Shell Monitoring thread. Error is %1.
.

MessageId = 0xcc13
Severity = Error
SymbolicName = APPSHELL_MUTEX_ERROR
Language=English
(ZQAPPSHELL)Unable to open mutex %1. Error is %2. Exiting... 
.

MessageId = 0xcc14
Severity = Error
SymbolicName = APPSHELL_SHELL_NOTALIVE
Language=English
(ZQAPPSHELL)Service Shell %1 is dead. Condition is %2. Exiting... 
.

MessageId = 0xcc15
Severity = Informational
SymbolicName = APPSHELL_SERVICE_REQUEST
Language=English
(ZQAPPSHELL)Received service shell request to %1. 
.

MessageId = 0xcc16
Severity = Error
SymbolicName = SHELL_REACQUIRE_SHELLALIVE_MUTEX
Language=English
(ZQSRVSHELL)Service %1 failed to reacquire its ShellAlive mutex from child process, PID=%2.
%3...
.

MessageId = 0xcc17
Severity = Informational
SymbolicName = SHELL_APP_TERM_OK
Language=English
(ZQSRVSHELL)Service application %1, PID=%2, has terminated successfully with status=%3.
.

MessageId = 0xcc18
Severity = Error
SymbolicName = SHELL_RELEASING_SHELLALIVE_MUTEX
Language=English
(ZQSRVSHELL)Service %1 requested its child process %2 to immediately abort
.

MessageId = 0xcc19
Severity = Error
SymbolicName = SHELL_HELPLESS_STATE
Language=English
(ZQSRVSHELL)Service %1 in helpless state.  Killing service process...
.

MessageId = 0xcc1a
Severity = Error
SymbolicName = SHELL_ABEND
Language=English
(ZQSRVSHELL)Service %1 terminating abnormally 
.

MessageId =     0xcc1b
Severity = Error
SymbolicName = SHELL_TOO_MANY_REBOOTS
Language=English
(ZQSRVSHELL)Service %1 experiencing too many reboots:
%2 starts within %3 seconds. Exiting...
.

MessageId =     0xcc1c
Severity = Error
SymbolicName = SHELL_EXITTING_NEED_REBOOT
Language=English
(ZQAPPSHELL)Service terminating.  Need system reboot to recover..
.

MessageId =     0xcc1d
Severity = Error
SymbolicName = SHELL_EXITTING_NEED_RESTART
Language=English
(ZQAPPSHELL)Service terminating.  Need process restart to recover.
.

MessageId =     0xcc1e
Severity = Error
SymbolicName = SHELL_EXITTING_NEED_INTERVENTION
Language=English
(ZQAPPSHELL)Service terminating.  Need user intervention to recover.
.

MessageId =     0xcc1f
Severity = Error
SymbolicName = SHELL_REBOOTING
Language=English
(ZQSRVSHELL)Service initiating system shutdown.
.
MessageId   =   0xcc20
Severity = Error
SymbolicName = SHELL_SCMGR_RPC_BUSY
Language=English
(ZQSRVSHELL)Unable to report status to Service Control Manager.
After trying several times, the RPC Server is still too busy.
Service %1 is terminating.
.
MessageId =     0xcc21
Severity = Informational
SymbolicName = SHELL_REBOOT_DELAY
Language=English
(ZQSRVSHELL)Service initiating system shutdown.%1
.
MessageId =     0xcc22
Severity = Warning
SymbolicName = SHELL_NO_PRODUCT
Language=English
(ZQSRVSHELL)Unable to find ProductName value in SYSTEM...Services...%1.  NT Error = %2.
Assuming CDCI...
.
MessageId =     0xcc23
Severity = Warning
SymbolicName = SHELL_NO_PORT
Language=English
(ZQSRVSHELL)Unable to find the management port for %1.  Error=%2.
Continuing without management...
.
MessageId =     0xcc24
Severity = Warning
SymbolicName = APPSHELL_ON_BACKUP
Language=English
(ZQAPPSHELL)Service %1 still running on its backup node.
.

MessageId =     0xcc25
Severity = Error
SymbolicName = APPSHELL_NO_WARNING_THREAD
Language=English
(ZQAPPSHELL)Unable to start backup warning thread. Error is %1. Exiting...
.

MessageId =     0xcc26
Severity = Error
SymbolicName = APPSHELL_TO_BACKUP
Language=English
(ZQAPPSHELL)Service %1 started on its backup node.
.

MessageId =     0xcc27
Severity = Error
SymbolicName = SHELL_KILL_FAILED
Language=English
(ZQSRVSHELL)Service application, unable to kill the process %1, PID= %2. Attempt to kill failed with an Win32 error %3 :-.
.

MessageId =     0xcc50
Severity = Error
Facility=Service_Shell
SymbolicName = SERVICE_LOG_ERROR
Language=English
%1
.

MessageId =     0xcc51
Severity = Warning
Facility=Service_Shell
SymbolicName = SERVICE_LOG_WARNING
Language=English
%1
.

MessageId =     0xcc52
Severity = Informational
Facility=Service_Shell
SymbolicName = SERVICE_LOG_INFO
Language=English
%1
.
