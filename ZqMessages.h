// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
//
// Name  : ZqMessages.mc
// Author : Bernie Zhao (bernie.zhao@i-zq.com  Tianbin Zhao)
// Date  : 2005-2-18
// Desc  : This is the message compiler source file for all ZQ facility
//         messages. (Currently only used by Server Engineer 1)
//
// Revision History:
// ---------------------------------------------------------------------------
// $Log: $
// ===========================================================================
//*******************************************************************************
//*
//*    Header Section
//*
//*  There will be no typedefs for ZQ messages.  The severity names will be the
//*    the default ones: SeverityNames=(Success= 0x0,Informational= 0x1,Warning= 0x2,
//*  Error= 0x3). Language names will likewise default to:
//*    LanguageNames=(English= 1:MSG00001)
//*******************************************************************************
//*******************************************************************************
//*
//*     Generic ZQ messages
//*******************************************************************************
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define ZQShell                          0x6


//
// Define the severity codes
//


//
// MessageId: ZQ_GENERIC_SUCCESS
//
// MessageText:
//
//  ZQ Service General Success:%1.
//
#define ZQ_GENERIC_SUCCESS               0x00001000L

//
// MessageId: ZQ_GENERIC_INFO
//
// MessageText:
//
//  ZQ Service General Information:%1.
//
#define ZQ_GENERIC_INFO                  0x40001001L

//
// MessageId: ZQ_GENERIC_WARNING
//
// MessageText:
//
//  ZQ Service General Warning:%1.
//
#define ZQ_GENERIC_WARNING               0x80001002L

//
// MessageId: ZQ_GENERIC_ERROR
//
// MessageText:
//
//  ZQ Service General Error:%1.
//
#define ZQ_GENERIC_ERROR                 0xC0001003L

//*******************************************************************************
//*
//*     Message Definitions for Facility Service_Shell
//*******************************************************************************
//
// MessageId: SHELL_LOST_SCMGR_CNXN
//
// MessageText:
//
//  (ZQSRVSHELL)Unable to report status to Service Control Manager.
//  Error is %1. State is %2.
//
#define SHELL_LOST_SCMGR_CNXN            0x8006C000L

//
// MessageId: SHELL_STOPPING_SVC
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 stop requested by SCManager.
//
#define SHELL_STOPPING_SVC               0x4006C001L

//
// MessageId: SHELL_CLOSE_ERR
//
// MessageText:
//
//  (ZQSRVSHELL)Error closing handle to %1.
//  Error code is %2.
//
#define SHELL_CLOSE_ERR                  0xC006C002L

//
// MessageId: SHELL_NORMAL_EXIT
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 is now exiting
//
#define SHELL_NORMAL_EXIT                0x4006C003L

//
// MessageId: SHELL_NOKILL
//
// MessageText:
//
//  (ZQSRVSHELL)Service application %1, PID= %2 has failed to shutdown.
//  %3...
//
#define SHELL_NOKILL                     0xC006C004L

//
// MessageId: SHELL_NOSHUTDOWN
//
// MessageText:
//
//  %1 failed.
//  (ZQSRVSHELL)Unable to initiate system shutdown. Error code is %2.
//
#define SHELL_NOSHUTDOWN                 0xC006C005L

//
// MessageId: SHELL_MANINIT_FAIL
//
// MessageText:
//
//  (ZQSRVSHELL)Unable to manage snmp variables. Sts = %1, GetLastError = %2.
//
#define SHELL_MANINIT_FAIL               0x8006C006L

//
// MessageId: APPSHELL_MANINIT_FAIL
//
// MessageText:
//
//  (ZQAPPSHELL)Unable to make timer variables manageable. Error is %1, GetLastError is %2.
//
#define APPSHELL_MANINIT_FAIL            0x8006C007L

//
// MessageId: SHELL_APP_ALREADY_UP
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 wants to create application process, but application is already running.
//  %2...
//
#define SHELL_APP_ALREADY_UP             0xC006C008L

// --- obsolete for v3.0 -------------
//
// MessageId: SHELL_NOSERVICE
//
// MessageText:
//
//  No service application name on command line. Command line is %1.
//  Exiting...
//
#define SHELL_NOSERVICE                  0xC006C800L

//
// MessageId: SHELL_INITFAIL
//
// MessageText:
//
//  (ZQSRVSHELL)Unable to initialize service shell %1. Error is %2. Exiting...
//
#define SHELL_INITFAIL                   0xC006C801L

//
// MessageId: SHELL_ERROR_REBOOTPARMS
//
// MessageText:
//
//  (ZQSRVSHELL)Service shell %1 failed to get configuration values for RebootTimeThreshold/RebootCountLimit from upstream node for SystemType %2
//  %3.
//
#define SHELL_ERROR_REBOOTPARMS          0x8006C803L

//
// MessageId: SHELL_START_PROC_FAIL
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 failed to create application process
//  Command line is %2.
//  Image is %3.
//  Current directory is %4.
//  Error is %5.
//
#define SHELL_START_PROC_FAIL            0xC006CC00L

//
// MessageId: SHELL_UNEXP_EXIT
//
// MessageText:
//
//  (ZQSRVSHELL)Service application %1, PID=%2 exited unexpectedly with status=%3., ChildExit Event occurred
//
#define SHELL_UNEXP_EXIT                 0xC006CC01L

//
// MessageId: SHELL_STARTING_SVC
//
// MessageText:
//
//  (ZQSRVSHELL)Service shell starting service %1 with ProcessId %2.
//
#define SHELL_STARTING_SVC               0x4006CC02L

//
// MessageId: SHELL_TOO_MANY_RESTARTS
//
// MessageText:
//
//  (ZQSRVSHELL)Service application %1 experiencing too many restarts:
//  %2 starts within %3 seconds. %4...
//
#define SHELL_TOO_MANY_RESTARTS          0xC006CC03L

//
// MessageId: SHELL_ALIVE_TIMEOUT
//
// MessageText:
//
//  (ZQSRVSHELL)Alive protocol has timed out for application %1, PID=%2.
//  Will attempt to restart. 
//
#define SHELL_ALIVE_TIMEOUT              0xC006CC04L

//
// MessageId: SHELL_UNKNOWN_WAIT_RESULT
//
// MessageText:
//
//  (ZQSRVSHELL)Unexpected completion from a wait.
//  Completion code is %1. Service %2 exiting...
//
#define SHELL_UNKNOWN_WAIT_RESULT        0xC006CC05L

//
// MessageId: APPSHELL_EVENT_ERROR
//
// MessageText:
//
//  (ZQAPPSHELL)Unable to %1 event %2. Error is %3. Exiting... 
//
#define APPSHELL_EVENT_ERROR             0xC006CC06L

//
// MessageId: APPSHELL_NO_MAIN_THREAD
//
// MessageText:
//
//  (ZQAPPSHELL)Unable to start main thread. Error is %1. Exiting...
//
#define APPSHELL_NO_MAIN_THREAD          0xC006CC07L

//
// MessageId: APPSHELL_ALIVE_TIMEOUT
//
// MessageText:
//
//  (ZQAPPSHELL)Application thread did not set the alive event within %1 sec (AppStartupWait). Exiting...
//
#define APPSHELL_ALIVE_TIMEOUT           0xC006CC08L

//
// MessageId: APPSHELL_UNKNOWN_WAIT_RESULT
//
// MessageText:
//
//  (ZQAPPSHELL)Unexpected completion from a wait.
//  Completion code is %1. Exiting...
//
#define APPSHELL_UNKNOWN_WAIT_RESULT     0xC006CC09L

//
// MessageId: APPSHELL_EXCEPTION
//
// MessageText:
//
//  (ZQAPPSHELL)Exception encountered in application.
//  Exception code is %1. Address is %2.
//  Binary data contain exception record and context. Exiting...
//
#define APPSHELL_EXCEPTION               0xC006CC0AL

//
// MessageId: APPSHELL_DEAD_APPTHREAD
//
// MessageText:
//
//  (ZQAPPSHELL)The main application thread (app_main) has terminated.  Exiting...
//
#define APPSHELL_DEAD_APPTHREAD          0xC006CC0BL

//
// MessageId: APPSHELL_NO_CONTROL_THREAD
//
// MessageText:
//
//  (ZQAPPSHELL)Unable to start service control thread. Error is %1.
//
#define APPSHELL_NO_CONTROL_THREAD       0xC006CC0CL

//
// MessageId: SHELL_PAUSED
//
// MessageText:
//
//  (ZQSRVSHELL)Application %1 received PAUSE signal.
//
#define SHELL_PAUSED                     0x4006CC0DL

//
// MessageId: SHELL_CONTINUED
//
// MessageText:
//
//  (ZQSRVSHELL)Application %1 received CONTINUE signal.
//
#define SHELL_CONTINUED                  0x4006CC0EL

//
// MessageId: SHELL_EXCEPTION
//
// MessageText:
//
//  (ZQSRVSHELL)Exception encountered in service shell.
//  Exception code is %1. Address is %2.
//  Binary data contain exception record and context. Exiting...
//
#define SHELL_EXCEPTION                  0xC006CC0FL

//
// MessageId: SHELL_PAUSING_SVC
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 pause requested by SCManager.
//
#define SHELL_PAUSING_SVC                0x4006CC10L

//
// MessageId: SHELL_CONTINUING_SVC
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 continue requested by SCManager.
//
#define SHELL_CONTINUING_SVC             0x4006CC11L

//
// MessageId: APPSHELL_NO_MONITOR_THREAD
//
// MessageText:
//
//  (ZQAPPSHELL)Unable to start Shell Monitoring thread. Error is %1.
//
#define APPSHELL_NO_MONITOR_THREAD       0xC006CC12L

//
// MessageId: APPSHELL_MUTEX_ERROR
//
// MessageText:
//
//  (ZQAPPSHELL)Unable to open mutex %1. Error is %2. Exiting... 
//
#define APPSHELL_MUTEX_ERROR             0xC006CC13L

//
// MessageId: APPSHELL_SHELL_NOTALIVE
//
// MessageText:
//
//  (ZQAPPSHELL)Service Shell %1 is dead. Condition is %2. Exiting... 
//
#define APPSHELL_SHELL_NOTALIVE          0xC006CC14L

//
// MessageId: APPSHELL_SERVICE_REQUEST
//
// MessageText:
//
//  (ZQAPPSHELL)Received service shell request to %1. 
//
#define APPSHELL_SERVICE_REQUEST         0x4006CC15L

//
// MessageId: SHELL_REACQUIRE_SHELLALIVE_MUTEX
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 failed to reacquire its ShellAlive mutex from child process, PID=%2.
//  %3...
//
#define SHELL_REACQUIRE_SHELLALIVE_MUTEX 0xC006CC16L

//
// MessageId: SHELL_APP_TERM_OK
//
// MessageText:
//
//  (ZQSRVSHELL)Service application %1, PID=%2, has terminated successfully with status=%3.
//
#define SHELL_APP_TERM_OK                0x4006CC17L

//
// MessageId: SHELL_RELEASING_SHELLALIVE_MUTEX
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 requested its child process %2 to immediately abort
//
#define SHELL_RELEASING_SHELLALIVE_MUTEX 0xC006CC18L

//
// MessageId: SHELL_HELPLESS_STATE
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 in helpless state.  Killing service process...
//
#define SHELL_HELPLESS_STATE             0xC006CC19L

//
// MessageId: SHELL_ABEND
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 terminating abnormally 
//
#define SHELL_ABEND                      0xC006CC1AL

//
// MessageId: SHELL_TOO_MANY_REBOOTS
//
// MessageText:
//
//  (ZQSRVSHELL)Service %1 experiencing too many reboots:
//  %2 starts within %3 seconds. Exiting...
//
#define SHELL_TOO_MANY_REBOOTS           0xC006CC1BL

//
// MessageId: SHELL_EXITTING_NEED_REBOOT
//
// MessageText:
//
//  (ZQAPPSHELL)Service terminating.  Need system reboot to recover..
//
#define SHELL_EXITTING_NEED_REBOOT       0xC006CC1CL

//
// MessageId: SHELL_EXITTING_NEED_RESTART
//
// MessageText:
//
//  (ZQAPPSHELL)Service terminating.  Need process restart to recover.
//
#define SHELL_EXITTING_NEED_RESTART      0xC006CC1DL

//
// MessageId: SHELL_EXITTING_NEED_INTERVENTION
//
// MessageText:
//
//  (ZQAPPSHELL)Service terminating.  Need user intervention to recover.
//
#define SHELL_EXITTING_NEED_INTERVENTION 0xC006CC1EL

//
// MessageId: SHELL_REBOOTING
//
// MessageText:
//
//  (ZQSRVSHELL)Service initiating system shutdown.
//
#define SHELL_REBOOTING                  0xC006CC1FL

//
// MessageId: SHELL_SCMGR_RPC_BUSY
//
// MessageText:
//
//  (ZQSRVSHELL)Unable to report status to Service Control Manager.
//  After trying several times, the RPC Server is still too busy.
//  Service %1 is terminating.
//
#define SHELL_SCMGR_RPC_BUSY             0xC006CC20L

//
// MessageId: SHELL_REBOOT_DELAY
//
// MessageText:
//
//  (ZQSRVSHELL)Service initiating system shutdown.%1
//
#define SHELL_REBOOT_DELAY               0x4006CC21L

//
// MessageId: SHELL_NO_PRODUCT
//
// MessageText:
//
//  (ZQSRVSHELL)Unable to find ProductName value in SYSTEM...Services...%1.  NT Error = %2.
//  Assuming CDCI...
//
#define SHELL_NO_PRODUCT                 0x8006CC22L

//
// MessageId: SHELL_NO_PORT
//
// MessageText:
//
//  (ZQSRVSHELL)Unable to find the management port for %1.  Error=%2.
//  Continuing without management...
//
#define SHELL_NO_PORT                    0x8006CC23L

//
// MessageId: APPSHELL_ON_BACKUP
//
// MessageText:
//
//  (ZQAPPSHELL)Service %1 still running on its backup node.
//
#define APPSHELL_ON_BACKUP               0x8006CC24L

//
// MessageId: APPSHELL_NO_WARNING_THREAD
//
// MessageText:
//
//  (ZQAPPSHELL)Unable to start backup warning thread. Error is %1. Exiting...
//
#define APPSHELL_NO_WARNING_THREAD       0xC006CC25L

//
// MessageId: APPSHELL_TO_BACKUP
//
// MessageText:
//
//  (ZQAPPSHELL)Service %1 started on its backup node.
//
#define APPSHELL_TO_BACKUP               0xC006CC26L

//
// MessageId: SHELL_KILL_FAILED
//
// MessageText:
//
//  (ZQSRVSHELL)Service application, unable to kill the process %1, PID= %2. Attempt to kill failed with an Win32 error %3 :-.
//
#define SHELL_KILL_FAILED                0xC006CC27L

//
// MessageId: SERVICE_LOG_ERROR
//
// MessageText:
//
//  %1
//
#define SERVICE_LOG_ERROR                0xC006CC50L

//
// MessageId: SERVICE_LOG_WARNING
//
// MessageText:
//
//  %1
//
#define SERVICE_LOG_WARNING              0x8006CC51L

//
// MessageId: SERVICE_LOG_INFO
//
// MessageText:
//
//  %1
//
#define SERVICE_LOG_INFO                 0x4006CC52L

