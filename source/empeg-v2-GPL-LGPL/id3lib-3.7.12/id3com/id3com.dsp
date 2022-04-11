# Microsoft Developer Studio Project File - Name="id3com" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=id3com - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "id3com.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "id3com.mak" CFG="id3com - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "id3com - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "id3com - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "id3com - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\\" /I "..\include" /I "..\include\id3" /I "..\zlib\include" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "ID3LIB_COMPILATION" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\zlib\zlibD.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\id3com.dll
InputPath=.\Debug\id3com.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "id3com - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "id3com___Win32_Release"
# PROP BASE Intermediate_Dir "id3com___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /I ".\\" /I "..\\" /I "..\include" /I "..\include\id3" /I "..\zlib\include" /D "NDEBUG" /D "_MBCS" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I ".\\" /I "..\\" /I "..\include" /I "..\include\id3" /I "..\zlib\include" /D "NDEBUG" /D "_MBCS" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "ID3LIB_COMPILATION" /FR /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\zlib\zlib.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build - Performing registration
OutDir=.\Release
TargetPath=.\Release\id3com.dll
InputPath=.\Release\id3com.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "id3com - Win32 Debug"
# Name "id3com - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\EnumFields.cpp
# End Source File
# Begin Source File

SOURCE=.\id3com.cpp
# End Source File
# Begin Source File

SOURCE=.\id3com.def
# End Source File
# Begin Source File

SOURCE=.\id3com.idl
# ADD MTL /tlb ".\id3com.tlb" /h "id3com.h" /iid "id3com_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\id3com.rc
# End Source File
# Begin Source File

SOURCE=.\ID3Field.cpp
# End Source File
# Begin Source File

SOURCE=.\ID3Frame.cpp
# End Source File
# Begin Source File

SOURCE=.\ID3Tag.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\EnumFields.h
# End Source File
# Begin Source File

SOURCE=.\ID3Field.h
# End Source File
# Begin Source File

SOURCE=.\ID3Frame.h
# End Source File
# Begin Source File

SOURCE=.\ID3Tag.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ID3Field.rgs
# End Source File
# Begin Source File

SOURCE=.\ID3Frame.rgs
# End Source File
# Begin Source File

SOURCE=.\ID3Tag.rgs
# End Source File
# End Group
# Begin Group "id3lib"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=..\src\c_wrapper.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Config.h
# End Source File
# Begin Source File

SOURCE=..\src\error.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\Error.h
# End Source File
# Begin Source File

SOURCE=..\src\field.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\field.h
# End Source File
# Begin Source File

SOURCE=..\src\field_binary.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\field_integer.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\field_string_ascii.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\field_string_unicode.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\Flags.h
# End Source File
# Begin Source File

SOURCE=..\src\frame.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\frame.h
# End Source File
# Begin Source File

SOURCE=..\src\frame_parse.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\frame_render.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\globals.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\Globals.h
# End Source File
# Begin Source File

SOURCE=..\src\header.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\Header.h
# End Source File
# Begin Source File

SOURCE=..\src\header_frame.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\header_frame.h
# End Source File
# Begin Source File

SOURCE=..\src\header_tag.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\header_tag.h
# End Source File
# Begin Source File

SOURCE=..\Include\Id3.h
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\Int28.h
# End Source File
# Begin Source File

SOURCE=..\src\misc_support.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\misc_support.h
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\sized_types.h
# End Source File
# Begin Source File

SOURCE=..\src\spec.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\Spec.h
# End Source File
# Begin Source File

SOURCE=..\src\tag.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\Tag.h
# End Source File
# Begin Source File

SOURCE=..\src\tag_file.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\tag_find.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\tag_parse.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\tag_parse_lyrics3.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\tag_parse_musicmatch.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\tag_parse_v1.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\tag_render.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\tag_sync.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\src\uint28.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\uint28.h
# End Source File
# Begin Source File

SOURCE=..\src\utils.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Include\Id3\Utils.h
# End Source File
# Begin Source File

SOURCE=..\zlib\include\zconf.h
# End Source File
# Begin Source File

SOURCE=..\zlib\include\zlib.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\ChangeLog
# End Source File
# Begin Source File

SOURCE=..\News
# End Source File
# Begin Source File

SOURCE=..\Todo
# End Source File
# End Target
# End Project
