# Introduction #

We currently do not distribute binary versions of EOF for Linux operating systems. This is mainly due to the Windows XP-style file system conventions used within the program. EOF was originally developed on a Windows XP system and assumes full read/write access to the entire file system. Because of the way it saves certain files, it is not feasible to release a binary version at this time.

Fortunately, building things from source is fairly simple on Linux. Many popular Linux distributions contain everything you need to build EOF within their software repositories. This document will describe what you need to build EOF, as well as how to obtain and build its dependencies if they are not available for your Linux distribution.

# Details #

## Dependencies ##
EOF has several dependencies which need to be met before it can be built.  These are:

  1. The standard GCC compiler, should be readily available in any Unix/Linux OS
  1. Allegro 4.4 source (download from http://alleg.sourceforge.net/download.html)
  1. libogg and libvorbis packages (download from http://xiph.org/downloads)
  1. libfftw (download from http://www.fftw.org)
  1. librubberband (download from http://breakfastquay.com/rubberband)
    * libsamplerate (download from http://www.mega-nerd.com/SRC)
    * libsndfile (download from http://www.mega-nerd.com/libsndfile)
    * vamp-sdk (download from http://sourceforge.net/projects/vamp)
  1. CMake (download from http://www.cmake.org, unless your OS has it already)

**Note:** Allegro 5 does not use the same API as Allegro 4, so you will need to have some version Allegro 4 installed on your system, even if you already have Allegro 5.

## Building Dependencies ##

**Note:** Users of Ubuntu and similar systems can get all of the dependencies with this command:
```
sudo apt-get install build-essential liballegro4-dev libvorbis-dev libfftw3-dev librubberband-dev
```
If this works with your system, you can skip to the Building EOF section. Otherwise, continue reading.

All of EOF's dependencies, with the exception of Allegro 4.4, can be built using the same set of commands. You simply extract the source code to a location of your choosing, change to that directory from the command line (e.g. 'cd ~/libogg'), and type the following commands:
```
./configure
make
sudo make install
```
Make sure to build these five libraries in this order since they depend on each other to function properly:
  1. libfftw
  1. libsamplerate
  1. libsndfile
  1. vamp-sdk
  1. librubberband
The rest of the libraries are independent and can be built in any order.

Allegro 4.4 uses the CMake build system. To build it, extract the source code to a location of your choosing, change to that directory from the command line (e.g. cd ~/allegro), and type the following commands:
```
mkdir build
cd build
cmake .. -G "Unix Makefiles"
make
sudo make install
```
In case you find the above steps don't correctly allow Allegro to be built, please refer to Allegro's documentation for building in a Unix/Linux environment:
  * http://alleg.sourceforge.net/stabledocs/en/build/linux.html
  * http://alleg.sourceforge.net/stabledocs/en/build/unix.html
  * http://alleg.sourceforge.net/stabledocs/en/build/cmake.html

## Building EOF: ##
To build EOF, you must first have all of the dependencies installed (see sections Dependencies and Building Dependencies for more information).  Build EOF with the following command:
```
make -f makefile.linux
```
When the build process completes, you will have an executable file ('eof') in the 'bin' directory along with the binary data that EOF uses. To run EOF, change to the 'bin' directory and run 'eof'.

**Note:** EOF must be built in a part of the file system that you have full read/write access, typically somewhere in your home directory.
