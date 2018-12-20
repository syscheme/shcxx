# Microsoft Developer Studio Project File - Name="ZQCommon_stlp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ZQCommon_stlp - Win32 Debug_Local
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ZQCommon_stlp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ZQCommon_stlp.mak" CFG="ZQCommon_stlp - Win32 Debug_Local"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ZQCommon_stlp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ZQCommon_stlp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ZQCommon_stlp - Win32 Debug_Local" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ZQProjs/Common/dll", INOAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ZQCommon_stlp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /Od /I "$(STLPORT_ROOT)\\" /I "$(STLPORT_ROOT)/stlport" /D "NDEBUG" /D "ZQ_COMMON_DLL" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "ZQCOMMON_DLL" /D "_STLP_NEW_PLATFORM_SDK" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /machine:I386 /libpath:"$(STLPORT_ROOT)/lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=./PostBuild.bat Release
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ZQCommon_stlp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ICE_ROOT)/include/stlport" /I "$(ExpatPath)/include" /D "_DEBUG" /D "ZQCOMMON_DLL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "_STLP_NEW_PLATFORM_SDK" /D "XML_STATIC" /FD /GZ /c
# SUBTRACT CPP /X /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libexpatMT.lib /nologo /dll /pdb:"stlp_Debug/ZQCommon_stlp_d.pdb" /debug /machine:I386 /out:"stlp_Debug/ZQCommon_stlp_d.dll" /implib:"stlp_Debug/ZQCommon_stlp_d.lib" /pdbtype:sept /libpath:"$(ICE_ROOT)/lib" /libpath:"$(ExpatPath)/lib/"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetDir=.\stlp_Debug
SOURCE="$(InputPath)"
PostBuild_Cmds=$(TARGETDIR)\PostBuild.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ZQCommon_stlp - Win32 Debug_Local"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ZQCommon___Win32_Debug_Local0"
# PROP BASE Intermediate_Dir "ZQCommon___Win32_Debug_Local0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ZQCommon___Win32_Debug_Local0"
# PROP Intermediate_Dir "ZQCommon___Win32_Debug_Local0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "ZQCOMMON_DLL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(STLPORT_ROOT)\\" /I "$(STLPORT_ROOT)/stlport" /D "_DEBUG" /D "ZQCOMMON_DLL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "_STLP_NEW_PLATFORM_SDK" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug/ZQCommon_stlp_d.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"stlp_Debug/ZQCommon_stlp_d.dll" /pdbtype:sept /libpath:"$(ICE_ROOT)/lib"

!ENDIF 

# Begin Target

# Name "ZQCommon_stlp - Win32 Release"
# Name "ZQCommon_stlp - Win32 Debug"
# Name "ZQCommon_stlp - Win32 Debug_Local"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Exception.cpp
# End Source File
# Begin Source File

SOURCE=..\expatxx.cpp
# End Source File
# Begin Source File

SOURCE=..\Guid.cpp
# End Source File
# Begin Source File

SOURCE=..\InetAddr.cpp
# End Source File
# Begin Source File

SOURCE=..\Log.cpp
# End Source File
# Begin Source File

SOURCE=..\NativeThread.cpp
# End Source File
# Begin Source File

SOURCE=..\NativeThreadPool.cpp
# End Source File
# Begin Source File

SOURCE=..\PollingTimer.cpp
# End Source File
# Begin Source File

SOURCE=..\rwlock_Win32.cpp
# End Source File
# Begin Source File

SOURCE=..\Semaphore.cpp
# End Source File
# Begin Source File

SOURCE=..\Socket.cpp
# End Source File
# Begin Source File

SOURCE=..\urlstr.cpp
# End Source File
# Begin Source File

SOURCE=..\Variant.cpp
# End Source File
# Begin Source File

SOURCE=..\XMLPreference.cpp
# End Source File
# Begin Source File

SOURCE=..\XmlRpcSerializer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Exception.h
# End Source File
# Begin Source File

SOURCE=..\..\..\sdk\3rdPartyKits\expat\include\expat.h
# End Source File
# Begin Source File

SOURCE=..\expatxx.h
# End Source File
# Begin Source File

SOURCE=..\Guid.h
# End Source File
# Begin Source File

SOURCE=..\InetAddr.h
# End Source File
# Begin Source File

SOURCE=..\IPreference.h
# End Source File
# Begin Source File

SOURCE=..\Locks.h
# End Source File
# Begin Source File

SOURCE=..\Log.h
# End Source File
# Begin Source File

SOURCE=..\NativeThread.h
# End Source File
# Begin Source File

SOURCE=..\NativeThreadPool.h
# End Source File
# Begin Source File

SOURCE=..\PollingTimer.h
# End Source File
# Begin Source File

SOURCE=..\rwlock.h
# End Source File
# Begin Source File

SOURCE=..\Socket.h
# End Source File
# Begin Source File

SOURCE=..\urlstr.h
# End Source File
# Begin Source File

SOURCE=..\Variant.h
# End Source File
# Begin Source File

SOURCE=..\XMLPreference.h
# End Source File
# Begin Source File

SOURCE=..\XmlRpcSerializer.h
# End Source File
# Begin Source File

SOURCE=..\ZQ_common_conf.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
