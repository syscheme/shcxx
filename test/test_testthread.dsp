# Microsoft Developer Studio Project File - Name="testthread" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=testthread - Win32 Default
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "test_testthread.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "test_testthread.mak" CFG="testthread - Win32 Default"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "testthread - Win32 Default" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release\testthread"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\testthread"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /GR /GX /I "$(ITVSDK)\include\\" /D "WIN32" /D "_CONSOLE" /FD /c
# ADD CPP /nologo /MD /GR /GX /I "$(ITVSDKPATH)\include\\" /D "WIN32" /D "_CONSOLE" /FD /c
# ADD BASE RSC /l 0x405 /i "$(ITVSDK)\include\\" /d "_CONSOLE"
# ADD RSC /l 0x405 /i "$(ITVSDK)\include\\" /d "_CONSOLE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 MtTcpComm_d.lib ScThreadPool_d.lib ws2_32.lib /nologo /subsystem:console /machine:I386 /out:"Release\testthread.exe" /libpath:"$(ITVSDK)\lib\debug"
# ADD LINK32 MtTcpComm_d.lib ScThreadPool_d.lib ws2_32.lib /nologo /subsystem:console /machine:I386 /out:"Release\testthread.exe" /libpath:"$(ITVSDK)\lib\debug"
# Begin Target

# Name "testthread - Win32 Default"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\..\log.cpp
# End Source File
# Begin Source File

SOURCE=.\..\netutil.cpp
# End Source File
# Begin Source File

SOURCE=.\..\sockutil.cpp
# End Source File
# Begin Source File

SOURCE=.\testthread.cpp
# End Source File
# Begin Source File

SOURCE=.\..\thread.cpp
# End Source File
# End Group
# Begin Source File

SOURCE="..\..\..\ITV-V02.0Sdk\include\scthreadpool.h"
# End Source File
# Begin Source File

SOURCE=..\Thread.h
# End Source File
# End Target
# End Project
