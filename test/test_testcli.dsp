# Microsoft Developer Studio Project File - Name="testcli" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=testcli - Win32 Default
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "test_testcli.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "test_testcli.mak" CFG="testcli - Win32 Default"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "testcli - Win32 Default" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "testcli - Win32 Default"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release\testcli"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\testcli"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /W1 /GR /GX /I "$(ITVSDK)\include\" /D "WIN32" /D "_CONSOLE" /c
# ADD CPP /nologo /FD /MD /W1 /GR /GX /I "$(ITVSDK)\include\" /D "WIN32" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x405 /d "_CONSOLE" /i $(ITVSDK)\include\
# ADD RSC /l 0x405 /d "_CONSOLE" /i $(ITVSDK)\include\
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 MtTcpComm_d.lib ScThreadPool_d.lib ws2_32.lib /nologo /machine:i386 /out:"Release\testcli.exe" /subsystem:console /libpath:"$(ITVSDK)\lib\debug"
# ADD LINK32 MtTcpComm_d.lib ScThreadPool_d.lib ws2_32.lib /nologo /machine:i386 /out:"Release\testcli.exe" /subsystem:console /libpath:"$(ITVSDK)\lib\debug"

!ENDIF

# Begin Target

# Name "testcli - Win32 Default"
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

SOURCE=.\testcli.cpp
# End Source File
# Begin Source File

SOURCE=.\..\thread.cpp
# End Source File
# End Group
# End Target
# End Project

