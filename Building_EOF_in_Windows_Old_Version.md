# Introduction #

Building EOF from source is much more complicated than just running it (hotfix builds are frequently posted on various related forums), but it's the way to go if you want to experiment with its programming.  It's also the only option if EOF binaries are not being released for your platform (Linux).  The steps for building on Non Windows platforms will likely be different, but should be at least similar.  Please send us your feedback for building on other platforms and we can try to make information available.

Note:  Certain new Linux distributions use Pulse Audio, which messes up the ability of software to use the ALSA sound driver.  If this is preventing sound from working once you build the program, the only workarounds we currently know of are for you to remove the Pulse Audio packages or try another distro.  Unfortunately, how Operating Systems handle their driver and how Allegro developers work around such problems are out of our control.


# Details #

## Building Allegro and other dependencies: ##
First, you need to build Allegro itself, along with some other dependencies that EOF needs.  This procedure should work with 4.4.x versions of Allegro.  At this time EOF will NOT WORK with Allegro 5.  To build EOF from source, you will need:
  1. MinGW (http://sourceforge.net/projects/mingw/)
  1. Allegro source (download from http://liballeg.org/old.html)
  1. DirectX SDK (download from https://liballeg.org/old.html)
  1. zlib and libpng (download from http://tjaden.strangesoft.net/loadpng/mingw.html)
  1. libogg and libvorbis Vorbis packages (download from http://xiph.org/downloads)
  1. MSYS (download from http://www.mingw.org/wiki/msys)
  1. CMake (download from http://www.cmake.org/)
  1. FFTW (download from http://www.fftw.org/install/windows.html)
  1. libsamplerate (download from http://www.mega-nerd.com/SRC/download.html)
  1. libsndfile (download from http://www.mega-nerd.com/libsndfile/)
  1. vamp-sdk (download from http://www.vamp-plugins.org/develop.html)
  1. pkgconfig (download from http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/pkg-config_0.26-1_win32.zip)
  1. GLib (download from http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28/glib_2.28.8-1_win32.zip)
  1. rubberband (download from http://breakfastquay.com/rubberband/)

To get an appropriate MinGW compiler, you can go with the official release (http://sourceforge.net/projects/mingw/) or the TDM release (http://tdm-gcc.tdragon.net/).  Installing MinGW to the root of a file system (ie. c:\mingw) will make things easier, in fact paths with spaces in them aren't supported by MinGW.  Due to changes with how optimizations are done in GCC 4.9, I would recommend not going above GCC version 4.8.x or else there could be some severe stability issues.  With TDM-GCC it's as simple as just downloading the installer for that version, as it supposedly comes with a posix style threads library.  For the official MinGW, you have to use the MinGW-get-setup utility from the command line, install that (ie. to c:\mingw), then go to a command line, change to the MinGW installation's "bin" folder and run the mingw-get utility with these commands to install the appropriate C and C++ compilers:
```
mingw-get.exe install gcc=4.8.*
mingw-get.exe install g++=4.8.*
```
After this, run the mingw-get utility again but with no command line so it displays the GUI.  Select the "mingw32-pthreads-w32" (from All Packages>MinGW>MinGW Base System>MinGW Standard Libraries), "mingw32-make" (from All Packages) and optionally the GDB debugger (from All Packages>MinGW>MinGW Base System>MinGW Source-Level Debugger) for installation.  These packages don't seem to require being an older version like GCC itself.  Ensure that MinGW's "bin" folder is in your PATH environment variable, so you can use gcc and mingw32-make from any current working directory.  Allegro in Windows requires a DirectX SDK.  Get one of them from https://liballeg.org/old.html.  Ie. download dx70_mgw.zip and extract its contents to your MinGW install path (ie. c:\mingw), allowing files to be overwritten.  This must be done in order for Allegro to build later.

Get the zlib and libpng packages (http://tjaden.strangesoft.net/loadpng/mingw.html).  Make sure they install to your MinGW installation directory and NOT to a subfolder, otherwise the source files won't get stored in the right folders and Allegro's CMake will not find them.

Get MSYS (http://www.mingw.org/wiki/msys).  MSYS 1.0.11 is an older version, but it works just fine and includes all parts in one installer, making it very easy to install.  Install it to its default directory (c:\msys\1.0) and allow it to run the post install process.  During this post install process, give it the correct path to your MinGW installation directory.  Remember to use the forward slash / instead of backward slash \ between each directory (ie c:/mingw).

Get the libogg and libvorbis Vorbis libraries (http://xiph.org/downloads).  Extract each someplace WITHOUT spaces in the folder path.  They will each be built using MSYS using the details below.  Libogg has to be built first.  If you extracted libogg to c:\libogg, launch MSYS, change to the right directory with:
```
cd /c/libogg
```
Run the configure script, specifying the path to your MinGW installation.  If MinGW was installed to C:\mingw, that makes it:
```
./configure --prefix=/c/mingw
make
make install
```
If you used the prefix correctly in the call to the configure script above, libogg will be properly built and the files will be correctly copied to within your MinGW folder.  Change directory to the extracted libvorbis files, for example:
```
cd /c/libvorbis
```
And run the same three commands in the same order:
```
./configure --prefix=/c/mingw
make
make install
```
Once all the above steps have been completed, the zlib, libpng, libogg and libvorbis dependencies will have been "built into" your MinGW installation.  That is all we need to use MSYS for, it can now be closed.  The rest of these steps will be done from the regular command window.

Get CMake (http://www.cmake.org).  Install it and ensure that CMake's "bin" folder is in your PATH environment variable, so you can call cmake from any current working directory.

Get Allegro's source (http://liballeg.org/old.html) and extract it someplace easy to get to (such as the root of a filesystem, such as c:\allegro).  Open a command shell, change to the Allegro directory, ie:
```
c:
cd c:\allegro
```
Usually, there won't be a build directory, so make one and change to it:
```
md build
cd build
```
Run CMake to create the appropriate makefiles and ensure dependencies are accounted for:
```
cmake -G "MinGW Makefiles" ..
```
Do include the two periods at the end, which refer cmake to the "CMakeLists.txt" file in the parent folder (c:\allegro).  This should create the files necessary in the build folder to build Allegro itself with make after this step is completed.  If you had tried to cmake Allegro before putting all of these dependencies in place, delete your "CMakeCache.txt" file and try cmake again.  If this CMakeCache.txt file is in the allegro folder or the allegro\build folder, make sure it's deleted, or else CMake won't put the files in the \build folder.  If you installed zlib and libpng to the correct directory, and built the vorbis files correctly, you should see that it says it found ZLIB, PNG and VORBIS among the other libraries that come with Allegro.

If it reports any errors, try running the cmake command again.  I found it would often mention "configuring incomplete, errors occurred" the first time, and running it again would report that it wrote build files to allegro\build.  If it still reports an error, check to make sure your Antivirus software hasn't interfered.  I've noticed Avast would quarantine "conftest.exe" in the libogg and libvorbis folders, and I had to exempt those, re-run the 3 commands each to make libogg and libvorbis and then use cmake to build the Allegro makefile succeeded.

Now Allegro can be built. This process will install files into your MinGW installation, allowing you to compile applications designed to use Allegro (such as EOF). You will need to make sure that you have defined the MINGDIR environment variable to be set to the path of your MinGW installation, and MinGW's \bin\ folder should be in your PATH environment variable, or this next part might fail to work.  To find out, run the "set" command and if those variables don't reflect these requirements, update them in System Properties, close and re-open the command prompt and change to build directory (ie. c:\allegro\build) again.  While still in Allegro's \build directory, run these commands:
```
mingw32-make
mingw32-make install
```

**Note:**
Some earlier versions of Allegro (ie. version 4.22) had some strange problems that would make Allegro fail to CMake and/or make.  Try the latest stable release, which fixes this problem.  Otherwise you may have to update part of the Allegro package (apparently binutils was the culprit).  If you get any strange errors during the make install command, try turning off your antivirus program and trying again.  I found that Avast would prevent the make install from setting the modification time on alleg44.dll and this was causing the process to abort.

Next, the FFTW library needs to be added.  Download the 32 bit precompiled DLL files from FFTW's website (http://www.fftw.org/install/windows.html).  Inside it is several source and binary files.  The one needed for EOF is libfftw3-3.dll.  Copy this file into the "lib" directory in your MinGW installation and rename the file to "libfftw3.dll.a".  This naming convention is probably only one of many ways to do it, but it's how I got it to work as it allows MinGW to be told to link to the "fftw3" library when EOF is built.  Lastly, copy the fftw3.h file from the FFTW precompiled package and paste it into the "include" directory in your MinGW installation.


## Building rubberband: ##
The rubberband library adds several dependencies, but it allows audio to be played back at less than full speed, without lowering the pitch.  It requires a fourier transform library, but luckily is compatible with the FFTW library that was added above.

First, get Vamp (http://www.vamp-plugins.org/develop.html).  It's easiest to download the pre-compiled library (labeled "Windows (MinGW)"), copy libvamp-sdk.a from the zip file into the "lib" directory of your MinGW installation.  The pre-compiled library package does not include the header files, so you will still have to download the source code (labeled "The main SDK"), copy the "vamp", "vamp-hostsdk" and "vamp-sdk" folders of header files into MinGW's "include" directory.

Next, get libsndfile (http://www.mega-nerd.com/libsndfile).  As with Vamp, it's easiest to download the pre-compiled library (the "Win32 installer").  I recommend installing it to its own folder outside your MinGW installation to not clutter MinGW with miscellaneous files.  If you installed it outside of your MinGW installation, copy the libsndfile\bin\libsndfile-1.dll file to the "lib" directory of your MinGW installation and rename it to libsndfile.a, copy the two header files from libsndfile\include into the "include" directory of your MinGW installation and copy the libsndfile\lib\pkgconfig\sndfile.pc file to the "lib\pkgconfig" directory of your MinGW installation.

Next, get libsamplerate (http://www.mega-nerd.com/SRC/download.html).  This doesn't have a pre-compiled library, so it will be built from source code.  Just as with the Vorbis libraries, it's easiest to extract the compressed folder of source files someplace simple like the root of your c drive.  Bring up the MSYS program again and change to the directory with libsamplerate's source files, for example:
```
cd /c/libsamplerate
```
Run the configure script, specifying the path to your MinGW installation.  If MinGW was installed to C:\mingw, that makes it:
```
./configure --prefix=/c/mingw
make
make install
```
If any of these fail keep in mind your antivirus may be interfering again, Avast once again blocked "conftest.exe" during the configure process.  If you used the prefix correctly in the call to the configure script above, libsamplerate will be properly built and the files will be correctly copied to within your MinGW folder.  I saw some errors building some of the example programs that came with the source, but that shouldn't be a problem as long as libsamplerate.a was added to your MinGW "lib" directory and samplerate.h was added to the "include" directory.  I also found that samplerate.pc file that was created during the make wasn't automatically copied to the MinGW's "lib\pkgconfig" directory, and had to manually copy it to the MinGW installation.  Confirm these files were added before continuing.

Next, get pkgconfig (http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/pkg-config_0.26-1_win32.zip).  Extract the pkg-config.exe file from that package's bin folder and copy it to the "bin" folder in your MinGW installation.  While you're here, copy libintl-8.dll and name the copy intl.dll to satisfy a requirement for pkgconfig.

Next, get GLib (http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28/glib_2.28.8-1_win32.zip).  Extract the libglib-2.0-0.dll file from that package's bin folder and copy it to the "bin" folder in your MinGW installation.

I've found that depending on your environment, rubberband may or may not fail to build on Windows with a message along the lines of "cannot find -lpthread", due to missing a Posix style threading library.  If you are using MinGW's GCC distribution, you can install the mingw32-pthreads-w32 package and it seems to work just fine.  Other distributions may require their own equivalent (ie. TDM has a "winpthreads" library).  Most recently, I had installed the TDM release of GCC and it was actually missing the pthreads library so I had to manually get it from the pthreads-win32 website (ftp://sourceware.org/pub/pthreads-win32/dll-latest/dll/x86/) by downloading pthreadGC2.dll, renaming it to libpthread.a and placing it into the "lib" folder of my MinGW installation to satisfy MSYS.

At last, rubberband's dependencies are met and rubberband can be built from source (http://breakfastquay.com/rubberband/).  Extract the source folder someplace convenient like the root of your c drive.  Back in MSYS, change to the directory with rubberband's source files, for example:
```
cd /c/rubberband
```
Run the configure script, specifying the path to your MinGW installation.  Since we didn't build FFTW or Vamp from source, you have to also specify the compiler and linker arguments on the command line.  If MinGW was installed to C:\mingw, that makes it:
```
./configure --prefix=/c/mingw FFTW_CFLAGS="-I/c/mingw/include" FFTW_LIBS="-L/c/mingw/lib -lfftw3 -lm" Vamp_CFLAGS="-I/c/mingw/include" Vamp_LIBS="-L/c/mingw/lib -lvamp-sdk" CFLAGS='-flto' CXXFLAGS='-flto' LDFLAGS='-flto'
```
If everything worked, it will indicate that it got to the point where it was creating the makefile.  If it did, finish up with the make commands:
```
make
make install
```
Don't be surprised if you see errors that indicate it failed (ie. when building libvamp-sdk.a), but even then, check the "lib" directory of the rubberband source folder for librubberband.a.  If it is there, then it worked enough to get the library built, go ahead and copy that file to the "lib" directory of your MinGW installation.  Then copy the "rubberband" headers folder from the rubberband source code folder to the "include" directory of your MinGW installation.


## Building EOF: ##
To build EOF, ensure that you have built Allegro and the other dependencies into your MinGW installation with the instructions given above.  If you are using Allegro 4.4 in Windows, I've found that it may have a different name for the main dll file (alleg.dll).  If so, you need to modify makefile.mingw so that the line that says "LIBS = -lalleg" is changed to "LIBS = -lalleg44.dll".  Before continuing, ensure that the folder containing the \src folder also contains a folder called "bin", or the make will fail.  To build EOF, change directory to EOF's \src folder and run "mingw32-make -f makefile.mingw"

Copy all other resources (graphics, audio files, etc) from the source distrubution to the \bin folder, where the newly-built eof.exe should be.  Also, the following files will probably have to be put in the \bin folder unless you've changed the above steps so that they can be statically linked into EOF:
```
libfftw3-3.dll file (from the FFTW pre-compiled package)
alleg44.dll file (just built from the Allegro source package)
```
And all of the following DLLs from your MinGW installation's "bin" folder:
```
libgcc_s_dw2-1.dll
libsamplerate-0.dll
libvorbisfile-3.dll
libvorbis-0.dll
libogg-0.dll
libstdc++-6.dll
```
The program should now be ready to use.

To build in an IDE instead, create a project and manually add the source files and settings from the makefile:
Code::Blocks instructions, in Windows (other OSs will require some different steps):
  1. Create a new project, select console application, click Go.  Specify it as a C program.  Remove any default source files (main.c) that are added to the project automatically.
  1. Examine the makefile and for each reference to some file file.o, it means that you have to add the appropriate source file to the project (file.c), so it will compile to file.o.  Make sure to pay attention to the paths indicated for each of the object files given.  CodeBlocks will allow you to add all C files within a folder and its subfolders, but some of the example source files in some of the third party modules EOF uses have main() functions declared, so you'd need to remove them from the project or you will have problems during the build process.
  1. Include the file eof.rc to the project.  Code::Blocks should add this to a "Resources" folder in the project so that EOF's program icon can be properly set up.
  1. Open the project's build options, and for the top item listed in the Project build options (should apply to any build targets you created for the project).
  1. Open Compiler Settings.  Under #defines, add "EOF_BUILD" without quotation marks.
  1. Open Linker Settings.  Under "Other Linker Options", add "-lalleg44.dll -logg -lvorbisfile -mwindows -lm -lfftw3 -lrubberband -lsamplerate" without quotation marks.  This adds the required libraries and the necessary "mwindows" parameter required for building EOF in Windows.  Under "Search Directories", add the "...\alogg\include" (within \src directory in the EOF sources package)
  1. Go to Project Properties>Build Targets tab>Output Filename and ensure that you specify where the executable file is placed, otherwise it could end up in some unknown location.


## Adding decoding/encoding capability: ##
During initialization, EOF will check for the existence of the LAME, oggenc2 and oggCat utilities, which are used to convert MP3 to OGG when creating a new chart or when adding leading silence.

You can download LAME here (put lame.exe in EOF's program folder):
http://www.rarewares.org/mp3-lame-bundle.php

You can download oggenc2 here (put oggenc2.exe in EOF's program folder):
http://www.rarewares.org/ogg-oggenc.php
Get the "P4 only" version unless you're running on a processor older than a Pentium 4 or want a less optimized oggenc for some reason.

You can download oggCat as part of the Ogg video tools.  The current latest binary release (0.8a) has a problem where it always re-encodes the OGG files, which defeats EOF's purpose of having it.  Get version 0.8 instead here (put oggCat.exe and bgd.dll in EOF's program folder):
https://sourceforge.net/projects/oggvideotools/files/oggvideotools-win32/oggvideotools-0.8-win32/