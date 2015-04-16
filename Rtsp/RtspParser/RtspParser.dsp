# Microsoft Developer Studio Project File - Name="RtspParser" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=RtspParser - Win32 Debug_Local
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RtspParser.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RtspParser.mak" CFG="RtspParser - Win32 Debug_Local"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RtspParser - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "RtspParser - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "RtspParser - Win32 Debug_Local" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ZQProjs/Common/Rtsp/RtspParser", JSRAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RtspParser - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /O2 /I "../Utils/" /I "$(ICE_ROOT)/include/stlport" /D "_LIB" /D "_UNICODE" /D "UNICODE" /D "NDEBUG" /D "WIN32" /D "_STLP_NEW_PLATFORM_SDK" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "RtspParser - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "../utils" /I "$(ICE_ROOT)/include/stlport" /D "_STLP_DEBUG" /D "_MBCS" /D "_LIB" /D "_DEBUG" /D "WIN32" /D "_STLP_NEW_PLATFORM_SDK" /FD /GZ /FI "ForceInc.h" /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "RtspParser - Win32 Debug_Local"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "RtspParser___Win32_Debug_Local0"
# PROP BASE Intermediate_Dir "RtspParser___Win32_Debug_Local0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "RtspParser___Win32_Debug_Local0"
# PROP Intermediate_Dir "RtspParser___Win32_Debug_Local0"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "../utils" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /FI "ForceInc.h" /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "../utils/" /D "_LIB" /D "_DEBUG" /D "WIN32" /D "_MBCS" /FD /GZ /FI "ForceInc.h" /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "RtspParser - Win32 Release"
# Name "RtspParser - Win32 Debug"
# Name "RtspParser - Win32 Debug_Local"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CharData.cxx
# End Source File
# Begin Source File

SOURCE=.\CharDataParser.cxx
# End Source File
# Begin Source File

SOURCE=..\Utils\cpLog.cpp
# End Source File
# Begin Source File

SOURCE=..\Utils\Data.cxx
# End Source File
# Begin Source File

SOURCE=..\Utils\DataException.cxx
# End Source File
# Begin Source File

SOURCE=..\Utils\mstring.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspAnnounceMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspDescribeMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspLocationHdr.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspMsgParser.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspOptionsMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspPauseMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspPlayMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspRangeHdr.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspRecordMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspRequest.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspResponse.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspRtpInfoHdr.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspSetParameterMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspSetupMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspTeardownMsg.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspTransportHdr.cxx
# End Source File
# Begin Source File

SOURCE=.\RtspUtil.cxx
# End Source File
# Begin Source File

SOURCE=..\Utils\Sptr.cxx
# End Source File
# Begin Source File

SOURCE=..\Utils\StringData.cxx
# End Source File
# Begin Source File

SOURCE=..\Utils\VException.cxx
# End Source File
# Begin Source File

SOURCE=..\Utils\VMutex_WIN32.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CharData.hxx
# End Source File
# Begin Source File

SOURCE=.\CharDataParser.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\CountSemaphore.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\cpLog.h
# End Source File
# Begin Source File

SOURCE=..\Utils\Data.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\DataException.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\ForceInc.h
# End Source File
# Begin Source File

SOURCE=..\Utils\global.h
# End Source File
# Begin Source File

SOURCE=..\Utils\mstring.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspAnnounceMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspBadDataException.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspDescribeMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspLocationHdr.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspMsgParser.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspOptionsMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspPauseMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspPlayMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspRangeHdr.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspRecordMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspRequest.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspResponse.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspRtpInfoHdr.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspSetParameterMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspSetupMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspTeardownMsg.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspTransportHdr.hxx
# End Source File
# Begin Source File

SOURCE=.\RtspUtil.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\Sptr.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\SptrRefCount.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\StringData.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\substring.h
# End Source File
# Begin Source File

SOURCE=..\Utils\VException.hxx
# End Source File
# Begin Source File

SOURCE=..\Utils\VMutex.h
# End Source File
# Begin Source File

SOURCE=..\Utils\vtypes.h
# End Source File
# End Group
# End Target
# End Project
