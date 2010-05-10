EOF Build Instructions
----------------------

This file will guide you through the process of building EOF from the source
package. If a binary is available for the platform you are using it is
recommended that you acquire and use that package instead of trying to build it
yourself. Please visit http://www.t3-i.com/eof.htm for links to all available
packages.


What You Will Need
------------------

EOF is written in C so you will need a C compiler.

EOF uses the Allegro game programming library so you will need to install that
before you do anything else. Obtain Allegro at http://alleg.sourceforge.net/
and follow the installation instructions therein.


How to Build
------------

Open up a shell and switch to the directory where you extracted the source
package. List the files and make sure all these files are there:

./bin      (binary files)
./src      (source code)
./Makefile (build script)

Run "make" to start the build process.


How to Launch
-------------

Once EOF is built you can launch it by switching to the "bin" directory and
running the EOF executable (usually "./eof").

To enable MP3 support you will need to have LAME and Vorbis Tools installed. If
either of these tools aren't installed and located in the system's path, MP3
support will be disabled until they are installed.
