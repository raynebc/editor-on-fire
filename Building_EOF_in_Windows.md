# Introduction #

Building EOF on Windows is fairly straightforward. Here we will provide [a quick setup guide](#quick-setup) as well as [detailed descriptions](#details) of all the necessary steps to build EOF from source code.


# Quick Setup #

If you just want to build EOF, use the following steps (we will break the steps down in the Details section if you want more information):

  * Install MSYS2 (https://www.msys2.org - Follow the installation instructions exactly).
  * Open the MSYS2 MinGW 32-bit terminal ('Start->MSYS2->MSYS2 MinGW 32-bit').
  * Paste the following into the MSYS2 MinGW 32-bit terminal and press enter:

    ```
    pacman -S --needed --noconfirm base-devel mingw-w64-i686-toolchain
    pacman -S --noconfirm unzip
    pacman -S --noconfirm git
    wget https://download.tuxfamily.org/allegro/files/dx80_mgw.zip
    unzip -o dx80_mgw.zip -d /mingw32
    wget https://github.com/liballeg/allegro5/releases/download/4.4.3.1/allegro-4.4.3.1.tar.gz
    tar xzf allegro-4.4.3.1.tar.gz
    cd allegro-4.4.3.1
    mkdir build
    cd build
    cmake .. -DSHARED=OFF -DWANT_DOCS=OFF -DWANT_EXAMPLES=OFF -DWANT_TESTS=OFF -G "MSYS Makefiles"
    make
    make install
    cd ../../
    git clone https://github.com/raynebc/editor-on-fire.git
    cd editor-on-fire
    make
    ```

You should now have a working copy of EOF in the `bin` subdirectory. If you would like to enable the transcoding and audio manipulation features, please see [Adding Decoding/Encoding Capability](#adding-decodingencoding-capability).


# Details #

## Setting Up a Development Environment ##

To build EOF, you will need to have a proper development environment. We will be using MSYS2 with the MinGW32 toolchain. MSYS2 can be downloaded from the following web site:

https://www.msys2.org

First, follow the instructions on the MSYS2 web site to install MSYS2.

You will need the 32-bit toolchain to build EOF, so install that with the following command (run from the MSYS terminal):

`pacman -S --needed base-devel mingw-w64-x86_64-toolchain`

You will also need the unzip utility. Use the following command:

`pacman -S unzip`

You will need git to retrieve the EOF source code. Use the following command:

`pacman -S git`


## Installing EOF Dependencies ##

To build EOF, you will first need to install all of its dependencies. EOF uses Allegro 4 and Rubber Band so we will install those first.

To install Rubber Band, use the following command:

`pacman -S mingw-w64-i686-rubberband`

To install Allegro, you will first need to install the DirectX headers and libraries. To do so, you just need to extract the DirectX headers and libraries to the correct location. Use the following commands to download and install them:

```
wget https://download.tuxfamily.org/allegro/files/dx80_mgw.zip
unzip -o dx80_mgw.zip -d /mingw32
```

Now you need to download the Allegro 4 source code, extract it, build the library, and install it:

```
wget https://github.com/liballeg/allegro5/releases/download/4.4.3.1/allegro-4.4.3.1.tar.gz
tar xzf allegro-4.4.3.1.tar.gz
cd allegro-4.4.3.1
mkdir build
cd build
cmake .. -DSHARED=OFF -DWANT_DOCS=OFF -DWANT_EXAMPLES=OFF -DWANT_TESTS=OFF -G "MSYS Makefiles"
make
make install
```

Finally, you need to download the EOF source code and build EOF:

```
git clone https://github.com/raynebc/editor-on-fire.git
cd editor-on-fire
make
```

The program should now be ready to use.


## Adding Decoding/Encoding Capability ##
During initialization, EOF will check for the existence of the LAME, oggenc2 and oggCat utilities, which are used to convert MP3 to OGG when creating a new chart or when adding leading silence.

You can download LAME here (put lame.exe in EOF's program folder):
http://www.rarewares.org/mp3-lame-bundle.php

You can download oggenc2 here (put oggenc2.exe in EOF's program folder):
http://www.rarewares.org/ogg-oggenc.php
Get the "P4 only" version unless you're running on a processor older than a Pentium 4 or want a less optimized oggenc for some reason.

You can download oggCat as part of the Ogg video tools.  The current latest binary release (0.8a) has a problem where it always re-encodes the OGG files, which defeats EOF's purpose of having it.  Get version 0.8 instead here (put oggCat.exe and bgd.dll in EOF's program folder):
https://sourceforge.net/projects/oggvideotools/files/oggvideotools-win32/oggvideotools-0.8-win32/
