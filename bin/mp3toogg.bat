@echo off
@echo Using LAME and OggEnc to convert from MP3 to OGG.
@echo.
@echo Input:  %4
@echo Output: %3
@echo.
@echo This may take a minute or two. Please be patient.
@lame --quiet --decode %1 temp.wav
@oggenc2 --quiet -q %2 --resample 44100 -s 0 temp.wav -o %3
@del temp.wav
