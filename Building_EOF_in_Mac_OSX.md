# Introduction #

Building EOF on Mac is not as simple as on a modern Linux OS, but being a bit Linux-y itself is still easier than building on Windows.  This might possibly only be the case if you have an old enough version of OS X due to the way the operating system deprecates features required by Allegro 4 in ways that Windows and Linux do not.  These steps have been designed and tested on versions of OS X 10.10 and older.  If you can get them to work with or without changes on newer versions of OS X, please let us know and we'll update these instructions accordingly.

# Details #

## Install command line developer tools: ##
An easy way to automate this is go to a terminal and use a command like "gcc" or "make" and the OS will prompt to download and install the tools if they are not already installed.  Allow it to do so if that's the case.

## Get source code for dependencies: ##
This can be done manually or by installing the Homebrew package manager.

To download the source code manually, get libogg, libvorbis and fftw from the following sites:
http://xiph.org/downloads

http://www.fftw.org/

To use Homebrew, go to a terminal and install it with this command:
```
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" < /dev/null 2> /dev/null
```
Then get the source code for the dependencies with:
```
brew fetch libogg libvorbis fftw --build-from-source
```
It will download the source packages to ~/Library/Caches/Homebrew.

Some problems with the current official latest release of the rubberband library (1.8.1) necessitate building from the latest repository version instead:
https://bitbucket.org/breakfastquay/rubberband
You can use Mercurial related tools to download through the repository, but it's much easier to just click the Downloads link on the left side and then click "Download repository" to get the latest source.

Extract all source packages from their compressed formats.  The terminal commands may depend on the format, otherwise just double clicking on each in OS X's graphical interface should extract it to a subfolder.

## Download Xcode 4.3 ##
This involves browsing to https://developer.apple.com/download and creating a free Apple account.  After signing in, click "See more downloads" at the bottom of the page, enter **Xcode 4.3**, and download any of the 4.3 Developer Tools releases from 4.3 up to 4.3.3.  When the roughly 1.8GB download completes, double click on the downloaded DMG file to mount it in OS X.  An Xcode Tools window should appear.  In it, double click on the Packages folder, find the MacOSX10.6.pkg item and double click on it and install it.

## Install cmake ##
Cmake can be manually built from source, but it's not necessary for EOF.  It's easier installed via DMG file or with Homebrew.  To download the DMG file, get it from their website:
http://www.cmake.org/
To install via Homebrew, use the following command:
```
brew install cmake
```
I ran into an issue where it said the 'brew link' step did not complete for the reason that "/usr/local/share/aclocal is not writable".  The following commands fixed that issue:
```
sudo chown -R $(whoami) /usr/local/share/
brew link cmake
```

## Install pkg-config ##
Like Cmake, you can manually build this (https://pkg-config.freedesktop.org/releases/), but if you have Homebrew, it's as easy as
```
brew install pkg-config
```

## Install the other dependencies ##
For each of the libogg, libvorbis and fftw source packages you downloaded earlier, go to its source folder in a terminal and use the following commands:
```
./configure --enable-shared=no --enable-static=yes CFLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.6"
make
sudo make install
```
For fftw, you will also need to build the single precision floating point library (fftw3f).  Do this by still having the terminal at fftw's source folder and using the following commands:
```
./configure --enable-float --enable-sse --enable-shared=no --enable-static=yes CFLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.6"
make
sudo make install
```
All four builds should complete with no problems.

For rubberband, you will need to edit its Makefile.osx makefile:

In the CXX line, remove -stdlib=libc++

In the CXXFLAGS line, change -DHAVE_VDSP to -DHAVE_FFTW3

In the ARCHFLAGS line, change -mmacosx-version-min=10.7 to -mmacosx-version-min=10.6

Save the changes and then go to the rubberband source file in a terminal.  Use the following commands to create a lib folder (to work around a bug in the makefile) and run the makefile with the target to specify just the static library:
```
mkdir lib
make -f Makefile.osx static
```
Use the "ls lib" command to verify that the librubberband.a file got created, and if so, use these commands to install that library and the related headers:
```
sudo cp -a rubberband /usr/local/include
sudo cp lib/librubberband.a /usr/local/lib
```

## Install Allegro 4 ##
Do note that Allegro 5 will not work, it has to be one of the 4.x releases.  Download the latest 4.x release (currently it is 4.4.2) from http://liballeg.org/old.html and extract it.  Verify the path of the SDK you installed from the Xcode package earlier, such as by typing the following command in a terminal:
```
ls /SDKs/
```
It should list something like MacOSX10.6.sdk, making the full path end up as /SDKs/MacOSX10.6.sdk (you will have to use this in a command below, you can type the first half of it and press tab and if it automatically completes the name it means the folder path is there.  It's fine to leave the trailing forward slash on the end of the export command).  Go to the Allegro source folder in a terminal and run the following commands:
```
mkdir build
cd build
export SDK={path to your 10.6 SDK}
cmake .. -DCMAKE_OSX_SYSROOT=$SDK -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 -DAUDIOTOOLBOX_LIBRARY=$SDK/System/Library/Frameworks/AudioToolbox.framework -DAUDIOUNIT_LIBRARY=$SDK/System/Library/Frameworks/AudioUnit.framework -DCARBON_LIBRARY=$SDK/System/Library/Frameworks/Carbon.framework -DCOCOA_LIBRARY=$SDK/System/Library/Frameworks/Cocoa.framework -DCOREAUDIO_LIBRARY=$SDK/System/Library/Frameworks/CoreAudio.framework -DIOKIT_LIBRARY=$SDK/System/Library/Frameworks/IOKit.framework -DWANT_ALLEGROGL=0 -DSHARED=0 -DWANT_EXAMPLES=0
make
sudo make install
```
Some warnings during the make process are fine as long as the output indicates the final build target of afinfo completes.

## Build EOF ##
Download the latest source code from https://github.com/raynebc/editor-on-fire .  You can use git tools if you like, or simply click the "Clone or download" button and pick the option to "Download ZIP" and then extract the source code.

The makefile.macosx file in the downloaded EOF source code may need to be edited.  In its CFLAGS line, edit the -isysroot parameter to have the path to your 10.6 SDK if it is different from /SDKs/MacOSX10.6.sdk 

You can then finally build EOF by opening a terminal to EOF's /src folder and running the makefile:
```
make -f makefile.macosx
```
If the process succeeds, you should get the text "Application bundle created!" in the terminal.  If so, you're ready to run the program.  If not, please review/retry the above steps and contact us if you can't get them to work.

## Optional MP3 support ##
Oggenc and LAME have to be installed in order for EOF to have the ability to decode MP3 and encode OGG files, such as when supplying an MP3 file in the new chart wizard.  You can install them manually or with this package:
http://www.t3-i.com/apps/eof/downloads/eof_utilities.pkg
