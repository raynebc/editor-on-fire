EOF is a song editor for Frets On Fire, Phase Shift, Rock Band, Rocksmith and Clone Hero. The aim of EOF is to provide a simple process with which to create songs. Just provide an OGG file and spend a little time designing note charts and EOF will save files in the appropriate format for immediate use with Frets on Fire, Phase Shift, Clone Hero, Performous and similar games.  Combined with other game-specific tools, you can author charts for games such as Rock Band and Rocksmith.


# Getting EOF #

Binaries are provided for Windows and Mac OS on a somewhat regular basis and can be downloaded from here:\
https://ignition4.customsforge.com/eof

Just click the Apple or Windows logo for your OS and download the relevant files.

The Windows version is distributed as an older release candidate (a full, standalone build) and a "hotfix" containing all files that have changed since that release candidate.  To install the latest release of EOF, extract the release candidate to a folder for which you have write permissions (ie. not within "Program Files" or "Program Files (x86)" ), then extract the hotfix into the release candidate folder (the one that contains eof.exe), allowing it to merge folders and replace existing files.  If you do this correctly, EOF should be able launch and you can verify the hotfix build date by opening its Help menu and selecting About.  For any subsequent hotfix, you can just extract over the previous EOF installation and it will include all updates before and since the previously installed hotfix.

The Mac version is distributed as just the current build.  Do make sure to download and install the Utilities offered on the download page, as this will install the capability to convert chart audio from MP3 and encode OGG audio in various scenarios (converting from MP3 or WAV when creating a new chart, re-encoding audio when adding leading silence, etc).


# Building EOF #

If you're using Linux or if you want to experiment with the source code yourself, there are platform specific build instructions for Windows, Mac OS and Linux available here:\
https://github.com/raynebc/editor-on-fire/tree/wiki

MP3 decoding and OGG encoding capabilities will require LAME and Vorbis Tools to be installed manually, otherwise EOF will warn that applicable features are disabled.
