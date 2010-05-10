# Microsoft Developer Studio Project File - Name="alogg_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=alogg_dll - Win32 MD
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "alogg_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "alogg_dll.mak" CFG="alogg_dll - Win32 MD"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "alogg_dll - Win32 MD" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MD"
# PROP BASE Intermediate_Dir "MD"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj/msvc/dll"
# PROP Intermediate_Dir "obj/msvc/dll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ALOGG_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W1 /GX /O2 /I "include" /I "decoder" /I "decoder/libogg/include" /I "decoder/libvorbis/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ALOGG_DLL_EXPORTS" /D "ALOGG_DLL" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40a /d "NDEBUG"
# ADD RSC /l 0x40a /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib alleg.lib /nologo /dll /pdb:none /machine:I386 /out:"lib/msvc/alogg.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy import library
PostBuild_Cmds=copy obj\msvc\dll\alogg.lib lib\msvc\aloggdll.lib
# End Special Build Tool
# Begin Target

# Name "alogg_dll - Win32 MD"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\alogg.c
# ADD CPP /W3
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\analysis.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\bitrate.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libogg\src\bitwise.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\block.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\codebook.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\envelope.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\floor0.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\floor1.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libogg\src\framing.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\info.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\lpc.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\lsp.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\mapping0.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\mdct.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\psy.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\registry.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\res0.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\sharedbook.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\smallft.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\synthesis.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\vorbisfile.c
# End Source File
# Begin Source File

SOURCE=.\decoder\libvorbis\lib\window.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\include\alogg.h
# End Source File
# Begin Source File

SOURCE=..\alm\include\aloggdll.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
