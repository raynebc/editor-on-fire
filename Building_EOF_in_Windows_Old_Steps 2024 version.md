# Introduction #

Building EOF from source is much more complicated than just running it (hotfix builds are frequently posted on various related forums), but it's the way to go if you want to experiment with its programming.  It's also the only option if EOF binaries are not being released for your platform (Linux).  The steps for building on Non Windows platforms will likely be different, but should be at least similar.  Please send us your feedback for building on other platforms and we can try to make information available.

Note:  Certain new Linux distributions use Pulse Audio, which messes up the ability of software to use the ALSA sound driver.  If this is preventing sound from working once you build the program, the only workarounds we currently know of are for you to remove the Pulse Audio packages or try another distro.  Unfortunately, how Operating Systems handle their driver and how Allegro developers work around such problems are out of our control.


# Details #

## Building Allegro and other dependencies: ##
First, you need to build Allegro itself, along with some other dependencies that EOF needs.  This procedure should work with 4.4.x versions of Allegro.  At this time EOF will NOT WORK with Allegro 5.  To build EOF from source, you will need:
  1. MinGW-W64 8.1.0 (download from https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/dwarf/)
  1. Allegro 4.4.3.1 source code (download from http://liballeg.org/old.html)
  1. zlib source code (download from https://www.zlib.net/)
  1. libpng source code (download from http://www.libpng.org/pub/png/libpng.html)
  1. libogg and libvorbis Vorbis source code (download from http://xiph.org/downloads)
  1. MSYS 1.0.11 (download from https://sourceforge.net/projects/mingw/files/MSYS/Base/msys-core/msys-1.0.11/MSYS-1.0.11.exe/download)
  1. CMake (download from https://cmake.org/download/)
  1. FFTW precompiled DLLs (download from http://www.fftw.org/install/windows.html)
  1. libsamplerate (download from http://www.mega-nerd.com/SRC/download.html)
  1. libsndfile (download from http://www.mega-nerd.com/libsndfile/)
  1. vamp-sdk (download from http://www.vamp-plugins.org/develop.html)
  1. pkgconfig (download from http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/pkg-config_0.26-1_win32.zip)
  1. GLib (download from http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28/glib_2.28.8-1_win32.zip)
  1. rubberband 1.9 (download from https://breakfastquay.com/files/releases/rubberband-1.9.0.tar.bz2)

As of 2024, normal MinGW compiler releases are not reliable and I couldn't get it to work, so these instructions will reflect using mingw-w64 i686-8.1.0-release-posix-dwarf-rt_v6-rev0 (https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/dwarf/).  Newer versions of mingw-w64 may or may not work with these instructions.  Extracting the downloaded zip file's mingw32 to the root of a file system (ie. to result in c:\mingw32) will make things easier and will be the assumed location for these instructions.  Edit your PATH environment variable to include the path to the compiler's bin folder (ie. C:\mingw32\bin) and test that it's working by opening a new command window and using this command:
```
gcc --version
```
If it's working, you should see multiple lines of output, or else you'll see an error saying the command isn't recognized.  MinGW-W64 can build 32 and 64 bit programs, but EOF and some dependencies are designed to be 32 bit, so several of the libraries will be added into the 32 bit compiler folder (ie. C:\mingw32\i686-w64-mingw32).  Due to changes with how optimizations are done in GCC 4.9 and newer, it's possible that compiler optimizations may remove some safety checks, in which case the -fno-delete-null-pointer-checks compiler flag will need to be added.

Get MSYS (https://sourceforge.net/projects/mingw/files/MSYS/Base/msys-core/msys-1.0.11/MSYS-1.0.11.exe/download).  MSYS 1.0.11 is an older version, but it works just fine and includes all parts in one installer, making it very easy to install.  You can install it to a directory of your choice or leave it at its default (c:\msys\1.0).  Allow it to run the post install process, but if it fails, you will need to start that manually by opening a command prompt, changing directory to MSYS's postinstall folder (ie. C:\msys\1.0\postinstall) and then running this command:
```
..\bin\sh pi.sh
```
During this post install process, give it the correct path to your MinGW installation directory.  MSYS provides a *nix style environment so within the MSYS shell you must use the forward slash / instead of backward slash \ between each directory (ie c:/mingw32).

Get Cmake (https://cmake.org/download/) and install it with default options, allowing it to add itself to the PATH environment variable.  As with testing gcc, you can confirm the PATH variable was updated properly for cmake if you open a new command prompt and issue this command:
```
cmake --version
```

Get the zlib (https://www.zlib.net/), libpng (http://www.libpng.org/pub/png/libpng.html), libogg and libvorbis Vorbis (http://xiph.org/downloads) source packages.  To keep the path to them short, and to avoid historical problems that the MinGW compiler has had with source code in paths containing spaces, I extracted each of their source folders to c:\dev.  Building them must be done in order to avoid complications.

Zlib will be built first.  Launch MSYS and change to its source directory.  Within MSYS, the C drive is referenced as a folder from the root of the file system, making the command something like this:
```
cd /c/dev/zlib-1.3.1
```
Run the configure script.  To keep things clean, it's recommended to define the prefix path to someplace that isn't the MinGW installation, you can store them in the folder with the source packages, ie. c:\dev.  Then build the source code with the make and make install commands:
```
./configure --prefix=/c/dev
make
make install
```
Libpng won't build using MSYS 1, so we'll use Cmake for it instead.  Launch a normal command window, change directory to its source folder, make a subdirectory called "build", change to that directory and invoke cmake with parameters defining the paths to zlib's library and header files.  The paths MUST use forward slashes instead of backslashes or the make is likely to fail, ie:
```
cd c:\dev\libpng-1.6.44
md build
cd build
cmake -G "MinGW Makefiles" -DZLIB_LIBRARY=c:/dev/lib/libzlib.dll.a -DZLIB_INCLUDE_DIR=c:/dev/include -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=c:/dev ..
mingw32-make
mingw32-make install
```
Libogg and Vorbis must be built in MSYS, and Libogg has to be built before the other.  Also, due to some design limitations, these libraries must install into the MinGW directory.  Go back into MSYS and change to Libogg's source directory, ie:
```
cd /c/dev/libogg-1.3.5/
```
Make a build subdirectory, change to it and run the configure script, specifying the path to your MinGW installation's 32 bit compiler folder, ie:
```
mkdir build
cd build
../configure --prefix=/c/mingw32/i686-w64-mingw32
make
make install
```
If you used the prefix correctly in the call to the configure script above, libogg will be properly built and the files will be correctly copied to within your MinGW folder.  Change directory to the extracted libvorbis files, for example:
```
cd /c/dev/libvorbis-1.3.7
```
And run the same three commands in the same order:
```
mkdir build
cd build
../configure --prefix=/c/mingw32/i686-w64-mingw32
make
make install
```
That is all we need to use MSYS for, it can now be closed.  The rest of these steps will be done from the regular command window.

Define an environment variable called MINGDIR and set it to the path of your 32 bit compiler folder (ie. C:\mingw32\i686-w64-mingw32 ).  It's possible that variable is just used as a default installation path if the install prefix is not defined with cmake below.  Get Allegro 4.4.3.1's source (http://liballeg.org/old.html) and extract it someplace such as c:\dev\allegro).  Open a command shell, change to the Allegro directory, ie:
```
c:
cd c:\dev\allegro
```
Usually, there won't be a build directory, so make one and change to it:
```
md build
cd build
```
Run CMake to create the appropriate makefiles and ensure dependencies are accounted for.  The DirectX related libraries are built into the 32 bit MinGW folder (i686-w64-mingw32).  The various other libraries that were built with the previous instructions are where you specified (ie. c:\dev).  At this step you can define the install prefix parameter to install Allegro into your 32 bit compiler folder or into the folder where the other prerequisites were built.  If you choose the latter, you'll need to edit EOF's makefiles accordingly because they assume Allegro is installed into your compiler's include and lib folders.  Again make sure to use forward slashes, ie:
```
cmake -G "MinGW Makefiles" -DDXGUID_LIBRARY=c:/mingw32/i686-w64-mingw32/lib/libdxguid.a -DDDRAW_INCLUDE_DIR=c:/mingw32/i686-w64-mingw32/include -DDDRAW_LIBRARY=C:/mingw32/i686-w64-mingw32/lib/libddraw.a -DDINPUT_INCLUDE_DIR=c:/mingw32/i686-w64-mingw32/include -DDINPUT_LIBRARY=C:/mingw32/i686-w64-mingw32/lib/libdinput.a -DDSOUND_INCLUDE_DIR=c:/mingw32/i686-w64-mingw32/include -DDSOUND_LIBRARY=C:/mingw32/i686-w64-mingw32/lib/libdsound.a -DZLIB_LIBRARY_DEBUG=C:/dev/lib/libz.a -DZLIB_LIBRARY_RELEASE=C:/dev/lib/libz.a -DPNG_LIBRARY_DEBUG=C:/dev/lib/libpng.a -DPNG_LIBRARY_RELEASE=C:/dev/lib/libpng.a -DPNG_PNG_INCLUDE_DIR=C:/dev/include -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=C:\mingw32\i686-w64-mingw32 ..
```
Do include the two periods at the end, which refer cmake to the "CMakeLists.txt" file in the parent folder (ie. c:\dev\allegro).  This should create the files necessary in the build folder to build Allegro itself with make after this step is completed.  If you had tried to cmake Allegro before putting all of these dependencies in place, delete your "CMakeCache.txt" file and try cmake again.  If this CMakeCache.txt file is in the allegro folder or the allegro\build folder, make sure it's deleted, or else CMake won't put the files in the \build folder.  If you built the prerequisite libraries and defined their paths correctly in the command, you should see that it says it found DXGUID, OpenGL, ZLIB, PNG, VORBIS.  You'll probably see some "The package name...does not match the name of the calling package" that aren't a problem.  If your antivirus quarantines any of the programs invoked during this task (ie. conftest.exe), you may have to exempt the file and try again.  As long as the last line in the output says the build files have been written, proceed to build Allegro with:
```
mingw32-make
mingw32-make install
```

Next, the FFTW library needs to be added.  Download the 32 bit precompiled DLL files from FFTW's website (http://www.fftw.org/install/windows.html).  Inside it is several source and binary files.  The one needed for EOF is libfftw3-3.dll.  Copy this file into the "lib" directory in the 32 bit compiler folder (ie. C:\mingw32\i686-w64-mingw32\lib) and rename the file to "libfftw3.dll.a".  This naming convention is probably only one of many ways to do it, but it's how I got it to work as it allows MinGW to be told to link to the "fftw3" library when EOF is built.  Lastly, copy the fftw3.h file from the FFTW precompiled package and paste it into the "include" directory in your 32 bit compiler folder (ie. C:\mingw32\i686-w64-mingw32\include).


## Building rubberband: ##
The rubberband library adds several dependencies, but it allows audio to be played back at less than full speed, without lowering the pitch.  It requires a fast fourier transform library, but luckily is compatible with the FFTW library that was added above.

First, get the Vamp SDK by downloading and building the source code (http://www.vamp-plugins.org/develop.html) or more easily by downloading the last released pre-compiled binaries for version 2.6 (https://code.soundsoftware.ac.uk/attachments/download/1518/vamp-plugin-sdk-2.6-binaries-win32-mingw.zip).  In the case of the pre-compiled binaries, copy libvamp-sdk.a from the zip file into the "lib" directory of your 32 bit compiler folder (ie. C:\mingw32\i686-w64-mingw32\lib).  The pre-compiled library package does not include the header files, so if using that instead of building the source, you will still have to download the source code (labeled "The main SDK"), copy the "vamp", "vamp-hostsdk" and "vamp-sdk" folders of header files into the 32 bit compiler's "include" directory (ie. C:\mingw32\i686-w64-mingw32\include).

Next, get libsndfile (http://www.mega-nerd.com/libsndfile).  As with Vamp, it's easiest to download the pre-compiled library (the "Win32 installer").  Older releases asked where to install it, and if it asks you I recommend installing it to its own folder outside your MinGW installation to not clutter MinGW with miscellaneous files.  As of the writing of this document, the current pre-compiled release installs to c:\libsndfile automatically without asking.  Copy the libsndfile\bin\libsndfile-1.dll file to the "lib" directory of your 32 bit compiler folder (ie. C:\mingw32\i686-w64-mingw32\lib) and rename it to libsndfile.a, copy the two header files from libsndfile\include into the "include" directory of your 32 bit compiler folder (ie. C:\mingw32\i686-w64-mingw32\include) and copy the libsndfile\lib\pkgconfig\sndfile.pc file to the "lib\pkgconfig" directory of your MinGW installation (ie. C:\mingw32\lib\pkgconfig) as that seems to be where it's looked for when building rubberband.

Next, get libsamplerate (http://www.mega-nerd.com/SRC/download.html).  This doesn't have a pre-compiled library, so it will be built from source code.  Just as with the Vorbis libraries, it's easiest to extract the compressed folder of source files someplace simple like c:\dev.  Bring up the MSYS program again and change to the directory with libsamplerate's source files, for example:
```
cd /c/dev/libsamplerate-0.1.9
```
Run the configure script and make libsamplerate.  At this step you can define the install prefix parameter to install libsamplerate into your 32 bit compiler folder or into a separate folder like c:\dev.  If you choose the latter, you'll need to edit EOF's makefiles accordingly because they assume libsamplerate is installed into your compiler's include and lib folders:
```
./configure --prefix=/c/mingw32/i686-w64-mingw32
make
make install
```
If any of these fail keep in mind your antivirus may be interfering again.  The examples may fail to build, but if you used the prefix correctly in the call to the configure script above, libsamplerate will be properly built, including libsamplerate.a into the destination "lib" directory, samplerate.h into the "include" directory and samplerate.pc into the "lib\pkgconfig" directory.  Confirm these files were added before continuing.

Next, get pkgconfig (http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/pkg-config_0.26-1_win32.zip).  Extract the pkg-config.exe file from that zip file's bin folder and copy it to the "bin" folder in your MinGW installation (ie. C:\mingw32\bin).  Check whether a file called intl.dll (a requirement for pkgconfig) is present in this bin folder.  If not, but there is a libintl-8.dll file, copy that and rename the copy as intl.dll.  If neither file is present, download the gettext runtime (http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-runtime_0.18.1.1-2_win32.zip), extract intl.dll from that zip file's contained bin folder and put it in the mingw32\bin folder.

Next, get GLib (http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28/glib_2.28.8-1_win32.zip) which is a prerequisite for pkgconfig.  Extract the libglib-2.0-0.dll file from that zip file's bin folder and copy it to the "bin" folder in your MinGW installation (ie. C:\mingw32\bin).

Rubberband requires a ladspa.h header file that doesn't come with MinGW, so download it from https://www.ladspa.org/ladspa_sdk/ladspa.h.txt, rename it as ladspa.h and put it in the 32 bit compiler include folder (ie. C:\mingw32\i686-w64-mingw32\include).  At last, rubberband's dependencies are met and rubberband can be built from source (https://breakfastquay.com/files/releases/rubberband-1.9.0.tar.bz2).  Extract the source folder someplace such as c:\dev.  Back in MSYS, change to the directory with rubberband's source files, for example:
```
cd /c/dev/rubberband1.9
```
Run the configure script, specifying the prefix path to the dev folder to reduce clutter in your MinGW installation.  Since we didn't build FFTW or Vamp from source, you have to also specify the compiler and linker arguments on the command line.  Likewise, if libsamplerate (referenced below as "SRC") was installed outside of the compiler folder, it has to have some parameters to define its location.  As with libsamplerate, Allegro's makefiles assume rubberband was installed into the compiler directory but you can optionally change the prefix path, ie:
```
./configure --prefix=/c/mingw32/i686-w64-mingw32 FFTW_CFLAGS="-I/c/mingw32/i686-w64-mingw32/include" FFTW_LIBS="-L/c/mingw32/i686-w64-mingw32/lib -lfftw3 -lm" Vamp_CFLAGS="-I/c/mingw32/i686-w64-mingw32/include" Vamp_LIBS="-L/c/mingw32/i686-w64-mingw32/lib -lvamp-sdk" SRC_CFLAGS="-I/c/mingw32/i686-w64-mingw32/include" SRC_LIBS="-L/c/mingw32/i686-w64-mingw32/lib" CFLAGS='-flto -ffat-lto-objects' CXXFLAGS='-flto -ffat-lto-objects' LDFLAGS='-flto'
```
If everything worked, it will indicate that it got to the point where it was creating the makefile.  If it did, finish up with the make commands:
```
make
make install
```
Even if errors are given during the make commands, check the "lib" directory of the rubberband source folder for librubberband.a.  If it is there, then it worked enough to get the library built, go ahead and copy that file to the "lib" directory of your 32 bit compiler folder (ie. C:\mingw32\i686-w64-mingw32\lib).  Then copy the "rubberband" headers folder from the rubberband source code folder to the "include" directory of your 32 bit compiler (ie. C:\mingw32\i686-w64-mingw32\include).


## Building EOF: ##
To build EOF, ensure that you have built Allegro and the other dependencies into your MinGW installation with the instructions given above.  If you are using Allegro 4.4 in Windows, I've found that it may have a different name for the main dll file (alleg.dll).  If so, you need to modify makefile.mingw so that the line that says "LIBS = -lalleg" is changed to "LIBS = -lalleg44.dll".  Before continuing, ensure that the folder containing the \src folder also contains a folder called "bin", or the make will fail.  To build EOF, change directory to EOF's \src folder and run "mingw32-make -f makefile.mingw"

Copy all other resources (graphics, audio files, etc) from the source distribution to the \bin folder, where the newly-built eof.exe should be.  Also, the following files will probably have to be put in the \bin folder unless you've changed the above steps so that they can be statically linked into EOF:
```
libfftw3-3.dll file (from the FFTW pre-compiled package)
alleg44.dll file (just built from the Allegro source package)
```
And all of the following DLLs from your MinGW installation's "bin" folder:
```
libgcc_s_dw2-1.dll
libstdc++-6.dll
```
And these DLLs from wherever you built the libraries (ie. search for them at c:\dev\bin , C:\dev\libvorbis-1.3.7\build\lib\.libs , C:\dev\libogg-1.3.5\build\src\.libs):
```
libsamplerate-0.dll
libvorbisfile-3.dll
libvorbis-0.dll
libogg-0.dll
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
