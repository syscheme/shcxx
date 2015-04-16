# Microsoft Developer Studio Project File - Name="ServiceFrame" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ServiceFrame - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ServiceFrame.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ServiceFrame.mak" CFG="ServiceFrame - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ServiceFrame - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ServiceFrame - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ZQProjs/Common/ServiceFrame/Lib", IKRAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ServiceFrame - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /I "$(ICE_ROOT)/include/stlport" /I "../../ssllib" /I "$(OPENSSLPATH)\include" /I "$(ZQPROJSPATH)\Common" /I "$(ExpatPath)/include" /D "_MBCS" /D "MBCS" /D "_LIB" /D "_MT" /D "_SPEED_VERSION" /D "_WIN32_SPECIAL_VERSION" /D "NDEBUG" /D "WIN32" /D "_STLP_NEW_PLATFORM_SDK" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "ServiceFrame - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "$(ZQPROJSPATH)\Common" /I "$(ICE_ROOT)/include/stlport" /I "$(OPENSSLPATH)\include" /I "../../ssllib" /D _WIN32_WINNT=0x400 /D "_MBCS" /D "_LIB" /D "_MT" /D "_SPEED_VERSION" /D "_STLP_DEBUG" /D "_WIN32_SPECIAL_VERSION" /D "_DEBUG" /D "WIN32" /D "_STLP_NEW_PLATFORM_SDK" /FD /GZ /c
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

# Name "ServiceFrame - Win32 Release"
# Name "ServiceFrame - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\debug.cpp
# End Source File
# Begin Source File

SOURCE=..\ServiceFrame.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SSLLIB\ssllib.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\debug.h
# End Source File
# Begin Source File

SOURCE=..\ServiceFrame.h
# End Source File
# Begin Source File

SOURCE=..\..\SSLLIB\ssllib.h
# End Source File
# End Group
# End Target
# End Project
