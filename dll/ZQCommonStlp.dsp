# Microsoft Developer Studio Project File - Name="ZQCommonStlp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ZQCommonStlp - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ZQCommonStlp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ZQCommonStlp.mak" CFG="ZQCommonStlp - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ZQCommonStlp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ZQCommonStlp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ZQCommonStlp - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ZQCommonStlp - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ZQProjs/Common/dll", INOAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ZQCommonStlp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseStlp"
# PROP BASE Intermediate_Dir "ReleaseStlp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseStlp"
# PROP Intermediate_Dir "ReleaseStlp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /Ot /Oi /Oy /Ob1 /I "." /I "$(STLPORT_ROOT)" /I "$(ExpatPath)/include" /I "$(ICE_ROOT)/include/stlport" /D "_MSC_EXTENSIONS" /D "XML_STATIC" /D "NDEBUG" /D "ZQ_COMMON_DLL" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "ZQCOMMON_DLL" /D "_MBCS" /D "_STLP_NEW_PLATFORM_SDK" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 advapi32.lib $(ExpatPath)\lib\libexpat.lib $(ExpatPath)\lib\libexpatMT.lib /nologo /dll /incremental:yes /map /debug /machine:I386 /nodefaultlib:"LIBCMT.LIB" /libpath:"$(ExpatPath)/lib" /libpath:"$(ICE_ROOT)/lib"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=xcopy           /fy                              ReleaseStlp\ZQCommonStlp.dll                               ..\..\TianShan\bin\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ZQCommonStlp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugStlp"
# PROP BASE Intermediate_Dir "DebugStlp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugStlp"
# PROP Intermediate_Dir "DebugStlp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "$(STLPORT_ROOT)" /I "$(ExpatPath)/include" /D "XML_STATIC" /D "_MSC_EXTENSIONS" /D "ZQCOMMON_DLL" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "_DEBUG" /D "WIN32" /D "_STLP_NEW_PLATFORM_SDK" /D "_STLP_DEBUG" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libcmtd" /out:"DebugStlp/ZQCommonStlp_d.dll" /pdbtype:sept /libpath:"$(EXPATPATH)\lib" /libpath:"$(ICE_ROOT)/lib" /libpath:"$(ExpatPath)/lib"

!ELSEIF  "$(CFG)" == "ZQCommonStlp - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ZQCommonStlp___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "ZQCommonStlp___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Unicode_Debug"
# PROP Intermediate_Dir "Unicode_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "$(STLPORT_ROOT)" /I "$(ExpatPath)/include" /D "XML_STATIC" /D "_MSC_EXTENSIONS" /D "ZQCOMMON_DLL" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "_DEBUG" /D "WIN32" /D "_STLP_NEW_PLATFORM_SDK" /D "_STLP_DEBUG" /FR /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "$(STLPORT_ROOT)" /I "$(ExpatPath)/include" /D "XML_STATIC" /D "_MSC_EXTENSIONS" /D "ZQCOMMON_DLL" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "_DEBUG" /D "WIN32" /D "_STLP_NEW_PLATFORM_SDK" /D "_STLP_DEBUG" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libcmtd" /out:"DebugStlp/ZQCommonStlp_d.dll" /pdbtype:sept /libpath:"$(EXPATPATH)\lib" /libpath:"$(ICE_ROOT)/lib" /libpath:"$(ExpatPath)/lib"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libcmtd" /out:"Unicode_Debug/ZQCommonStlp_d.dll" /pdbtype:sept /libpath:"$(EXPATPATH)\lib" /libpath:"$(ICE_ROOT)/lib" /libpath:"$(ExpatPath)/lib"

!ELSEIF  "$(CFG)" == "ZQCommonStlp - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ZQCommonStlp___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "ZQCommonStlp___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Unicode_Release"
# PROP Intermediate_Dir "Unicode_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /I "." /I "$(STLPORT_ROOT)" /I "$(ExpatPath)/include" /I "$(ICE_ROOT)/include/stlport" /D "_MSC_EXTENSIONS" /D "XML_STATIC" /D "NDEBUG" /D "ZQ_COMMON_DLL" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "ZQCOMMON_DLL" /D "_MBCS" /D "_STLP_NEW_PLATFORM_SDK" /FD /c
# SUBTRACT BASE CPP /Fr /YX
# ADD CPP /nologo /MD /W3 /GX /ZI /Od /I "." /I "$(STLPORT_ROOT)" /I "$(ExpatPath)/include" /I "$(ICE_ROOT)/include/stlport" /D "_MSC_EXTENSIONS" /D "XML_STATIC" /D "NDEBUG" /D "ZQ_COMMON_DLL" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "ZQCOMMON_EXPORTS" /D "ZQCOMMON_DLL" /D "_MBCS" /D "_STLP_NEW_PLATFORM_SDK" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib $(ExpatPath)\lib\libexpat.lib $(ExpatPath)\lib\libexpatMT.lib /nologo /dll /map /debug /machine:I386 /nodefaultlib:"LIBCMT.LIB" /libpath:"$(ExpatPath)/lib" /libpath:"$(ICE_ROOT)/lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 advapi32.lib $(ExpatPath)\lib\libexpat.lib $(ExpatPath)\lib\libexpatMT.lib /nologo /dll /incremental:yes /map /debug /machine:I386 /nodefaultlib:"LIBCMT.LIB" /libpath:"$(ExpatPath)/lib" /libpath:"$(ICE_ROOT)/lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ZQCommonStlp - Win32 Release"
# Name "ZQCommonStlp - Win32 Debug"
# Name "ZQCommonStlp - Win32 Unicode Debug"
# Name "ZQCommonStlp - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\DynSharedObj.cpp
# End Source File
# Begin Source File

SOURCE=..\Exception.cpp
# End Source File
# Begin Source File

SOURCE=..\ExpatMap.cpp
# End Source File
# Begin Source File

SOURCE=..\expatxx.cpp
# End Source File
# Begin Source File

SOURCE=..\expatxxx.cpp
# End Source File
# Begin Source File

SOURCE=..\FileLog.cpp
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

SOURCE=..\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\MD5CheckSumUtil.cpp
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

SOURCE=..\Scheduler.cpp
# End Source File
# Begin Source File

SOURCE=..\Semaphore.cpp
# End Source File
# Begin Source File

SOURCE=..\Socket.cpp
# End Source File
# Begin Source File

SOURCE=..\strHelper.cpp
# End Source File
# Begin Source File

SOURCE=..\SvcLog.cpp
# End Source File
# Begin Source File

SOURCE=..\TimeUtil.cpp
# End Source File
# Begin Source File

SOURCE=..\UDPDuplex.cpp
# End Source File
# Begin Source File

SOURCE=..\UDPSocket.cpp
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

SOURCE=..\XMLPreferenceEx.cpp
# End Source File
# Begin Source File

SOURCE=..\XmlRpcSerializer.cpp
# End Source File
# Begin Source File

SOURCE=..\ZQThreadPool.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\DynSharedObj.h
# End Source File
# Begin Source File

SOURCE=..\Exception.h
# End Source File
# Begin Source File

SOURCE=..\ExpatMap.h
# End Source File
# Begin Source File

SOURCE=..\expatxx.h
# End Source File
# Begin Source File

SOURCE=..\expatxxx.h
# End Source File
# Begin Source File

SOURCE=..\FileLog.h
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

SOURCE=..\md5.h
# End Source File
# Begin Source File

SOURCE=..\MD5CheckSumUtil.h
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

SOURCE=..\Scheduler.h
# End Source File
# Begin Source File

SOURCE=..\Socket.h
# End Source File
# Begin Source File

SOURCE=..\strHelper.h
# End Source File
# Begin Source File

SOURCE=..\SvcLog.h
# End Source File
# Begin Source File

SOURCE=..\SyncUtil.h
# End Source File
# Begin Source File

SOURCE=..\TimeUtil.h
# End Source File
# Begin Source File

SOURCE=..\UDPDuplex.h
# End Source File
# Begin Source File

SOURCE=..\UDPSocket.h
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

SOURCE=..\XMLPreferenceEx.h
# End Source File
# Begin Source File

SOURCE=..\XmlRpcSerializer.h
# End Source File
# Begin Source File

SOURCE=..\ZQ_common_conf.h
# End Source File
# Begin Source File

SOURCE=..\ZQThreadPool.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ZQCommonStlp.rc
# ADD BASE RSC /l 0x804
# ADD RSC /l 0x804 /i "." /i "$(ZQPROJSPATH)/build"
# End Source File
# End Group
# End Target
# End Project
