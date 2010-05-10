# Microsoft Developer Studio Project File - Name="alogg_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=alogg_static - Win32 MD lib
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "alogg_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "alogg_static.mak" CFG="alogg_static - Win32 MD lib"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "alogg_static - Win32 MD lib" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "alogg_static___Win32_MD_lib"
# PROP BASE Intermediate_Dir "alogg_static___Win32_MD_lib"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj\msvc\static"
# PROP Intermediate_Dir "obj\msvc\static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "include" /I "decoder/src" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_M_IX_86" /YX /FD /c
# ADD CPP /nologo /MD /GX /O2 /I "include" /I "decoder" /I "decoder/libogg/include" /I "decoder/libvorbis/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /Z<none> /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib/alogg.lib"
# ADD LIB32 /nologo /out:"lib\msvc\alogg.lib"
# Begin Target

# Name "alogg_static - Win32 MD lib"
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

SOURCE=.\include\aloggdll.h
# End Source File
# End Group
# End Target
# End Project
