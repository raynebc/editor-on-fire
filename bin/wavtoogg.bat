@echo off
@echo Using OggEnc to convert from WAV to OGG.
@echo.
@echo Input:  %1
@echo Output: %3
@echo.
@echo This may take a minute or two. Please be patient.
@oggenc2 --quiet -q %2 --resample 44100 -s 0 %1 -o %3
