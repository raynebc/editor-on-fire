/*	Changelog: 1.96 to 2.0
//		Improved parameter conflict detection
//		Improved readme file
//		Added the -notenames parameter, allowing the exported pitched lyric file to be written in note names instead of numbers (ie. A#7 instead of 106)
//		Reorganized program for inclusion in EOF
//		Corrected grouping logic for KAR import
//		Removed the extra "#newline" markup that was at the end of script export if -marklines was specified
//		Pitchless formats (Script, VL, LRC and Soft Karaoke) are now stored internally as having no defined pitch instead of a generic pitch
//		Fixed relative UltraStar import logic
//		Fixed a bug with the UltraStar trailing whitespace logic that could cause grouping status to be lost
//		Added logic to better allow pitch shift lyrics to be converted to "~" when hyphens weren't suppressed during UltraStar export
//		Added the -relative parameter, allowing export of UltraStar lyrics in relative timing instead of absolute timing
//		Fixed a bug that would cause the program to crash if exporting to a MIDI format, using a source MIDI, when the import file was not a MIDI based format
//		Fixed a minor memory leak that could occur if importing a non MIDI based file and exporting to Vrhythm
//		Removed "PART VOCALS" as a usable vocal rhythm track
//		Added lyric format detection logic whose findings are given with verbose or debugging output.  If a lyric import is failing, you can use either of the logging features to see if you are specifying the correct import format for the given file.
//		Corrected logic that handles the conversion between MIDI pitch numbering (Pitch 24 is note C1) and UltraStar pitch numbering (Pitch 0 is note C1)
//		Rewrote pitch range validation to run only when the output format is MIDI, as only Rock Band MIDIs have a defined requirement of a vocal pitch range of [36,84].  Any vocal pitches outside of this range have their octave changed to correct the issue.
//		Added logic to allow pitchless, durationless MIDI vocals (Lyric events with no Note On and Off) to be imported, also allowing them to be exported to MIDI (without vocal pitch notes, if the nopitch parameter is defined), UltraStar (as freestyle) and Vrhythm (as freestyle)
//		Added the -nopitch parameter, which will cause imported pitchless lyrics (ie. VL, Script, LRC, Soft Karaoke, freestyle Pitched Lyric entries) to export to MIDI with no fake vocal notes, which also means that the affected lyrics will have no lyric durations
//		Improved the MIDI lyric error recovery logic's handling of corrected endings of lyric phrases
//		Corrected the program's logic regarding calculation of lyric line durations
//		Fixed logic errors with the combined use of nohyphens and noplus
//		Added support for exporting to Soft Karaoke format
//		Added more error handling and statistic tracking (lyric count) in MIDI parsing code
//		Added support for exporting to (unofficial) KAR format, which Rock Band's MIDI format was derived from
//		Removed some unused MIDI logic
//		Fixed a bug with the nohyphens logic, which would automatically remove hyphens that were inserted if "-nohyphens 2" was used to suppress the pre-existing hyphens in the imported lyrics
//		The -nohyphens parameter has been altered to allow it to be used without specifying a number after it, in which case "-nohyphens 3" is assumed, removing existing trailing hyphens and suppressing the automatic insertion of hyphens
//		Made various code optimizations
//		Fixed a bug in the KAR/MIDI import logic that could cause lyrics to lose their overdrive status
//		Used Splint, Yasca and RATS security analysis tools to add some minor hardening to the source code
//		Added more error checking
//		Added the -detect parameter, which will detect the lyric format of the specified file instead of performing a lyric import or export
//		Added logic to perform correction for malformed RB MIDIs that have empty lyric phrases (ie. Queen - "We Will Rock You")
//		Fixed a logic error that could cause the program to abort due to invalid running status events, even if they were valid
*/
/*	Changelog: 1.95 to 1.96
//		Corrected a bug that could produce unpredictable results when importing a relative timed UltraStar file that has a line break before the first lyric
//		Added the -nolyrics parameter, allowing vocal pitches to be imported from a MIDI or KAR track that has no lyrics.  Exported lyrics use asterisks where lyrics would be but retain note pitches and timings.
//		Added the ability to supply just the vrhythm track identifier instead of a pitched lyric file, allowing you to import a vrhythm file without lyrics and export back to vrhythm to create a pitched lyric file with the correct number of entries.
//		Fixed a missing pointer assignment in the pitched lyric line break logic, which may have eventually allowed a crash to occur
//		Increased error checking that occurs after lyric import
//		Added new logic to exclude the imported vocal track from export to avoid duplicate vocal tracks when converting to and from MIDI based formats (vrhythm import is excluded from this logic)
//		Added logic to correctly import MIDIs having the vocal track end in vocal phrases that have no lyrics defined (ie. tambourine sections)
//		Due to errors in several Rock Band Network MIDIs, logic was altered to disregard lyric events that begin with an open bracket ( [ )
*/
/*	Changelog: 1.94 to 1.95
//		Corrected a bug in the new source MIDI import logic that would import tracks from the input file instead of the source file
//		Corrected a bug in the new source MIDI import logic that would cause the program to abort instead of process the file
//		Corrected a false alarm that would claim that imported pitched lyrics had the wrong number of entries if the file ends in a blank line, even though all entries were properly loaded
//		Corrected a bug that would cause duplicate track names if the source MIDI contained a track named the same as the one created to export vocal rhythm to
//		Improved verbose output
//		If exporting to a MIDI based format without merging with tracks from an existing source file, the created MIDI explicitly defines a tempo of 120BPM instead of using it implicitly
//		Fixed the handling of the MIDI delay for MIDI based export format
//		Fixed a minor memory leak that would occur if an input MIDI track name was specified
//		Added the -outtrack parameter, allowing the destination track name for MIDI based exports to be specified
//		Added logic to allow the -bpm parameter to specify the tempo used during MIDI based export if no source MIDI is given
//		Based on Harmonix's documentation, I have altered program logic to recognize the expected range of pitches to be 36 to 84 (inclusive)
*/
/*	Changelog: 1.93 to 1.94
//		Finished removing previous KAR import logic
//		Improved efficiency of MIDI and Vrhythm export code
//		Added logic to disable appending a hyphen to a lyric if the following lyric is just a pitch shift (+ character), regardless of nohyphens setting
//		Added additional error checking to ensure all lyrics processed during MIDI/KAR import are handled completely
//		Fixed a logic error that could cause the program to abort itself at the end if the input or source MIDI file did not explicitly set a tempo
//		Made the source MIDI file optional for MIDI and Vrhythm export.  If not used, the exported file uses a default tempo of 120BPM
//		Added a source file offset for the optional source file given during MIDI based file export (ie. MIDI, Vrhythm)
//		Added whitespace logic allowing either leading OR trailing whitespace to separate whole words during UltraStar import
*/
/*	Changelog: 1.92 to 1.93
//		Added support for exporting to Vocal Rhythm format
//		Added support for writing flat note notation for Vrhythm import
//		Added handling for lyrics that end in a forward slash (ie. in Rock Band Beatles MIDIs) to cause a line break to occur
//		Corrected the grouping behavior of lyrics ending in "=", which are supposed to group with the next lyric in addition to representing a visible hyphen in the lyric
//		Added strong handling for incorrectly nested notes in KAR files
//		Improved error handling for remaining file I/O (file open/read/write/close/flush)
//		Combined new KAR import logic with RB MIDI import logic for simplification
//		Corrected byte ordering when writing files, should maintain ability to work on little endian platforms like Mac
//		Fixed grouping logic for pitched lyric import
//		Cleaned up some unused code
//		Consolidated memory allocation error checking
//		Fixed a bug that could prevent a successful MIDI export if not importing vocal rhythm
//		Improved memory management of import functions and corrected most/all memory leaks
//		Fixed various format->format specific logic errors
*/
/*	Changelog: 1.91 to 1.92
//		Added support for importing KAR files
//		Added support for unofficial MIDI meta event 0x21 (Set MIDI port), as some non completely MIDI-compliant files may use this
//		Added logic to account for running status MIDI events when vocal rhythm notes are ommitted during MIDI export
//		Corrected a MIDI parsing flaw that would cause Sysex events to be misread
//		Corrected a logic flaw that would export to VL format incorrectly if a lyric duration was less than 10ms
*/
/*	Changelog: 1.9 to 1.91
//		Consolidated error checking for MIDI writing code
//		Added logic to prevent creation of an empty MIDI track during a vocal rhythm to MIDI export if the track exported had vocal rhythm notes removed
//		Added more error checking for file I/O
//		Fixed a minor logic error that could cause the incorrect syntax handling to abort the program before displaying more appropriate information
//		Added logic to properly use zeros to pad timestamps to mm:ss.xx format during LRC export
//		Added support for simple LRC export
//		Significantly consolidated and improved tag loading code
//		Significantly consolidated file I/O error checking
//		Added logic to accept note names (ie. C-1 or G#8) in addition to pitch numbers in pitched lyric files (vocal rhythm import)
*/
/*	Changelog: 1.89 to 1.9
//		Added support for exporting to extended LRC format
//		During a vocal rhythm to MIDI conversion, the vocal rhythm notes are now removed from the exported MIDI.  All other notes retain their original timing
//		Added handling for empty line phrases during pitched lyric import, allowing the first line of lyrics to contain overdrive
//		Fixed a minor logic flaw caused by importing empty lyric lines in LRC format
//		Added error checking for almost all file I/O operations
//		Improved specificity and consistency for error messages, such as failed memory allocation, file I/O and MIDI file errors
//		Added leniency for UltraStar import, which now allows import for text files that are missing one or more important tags
//		Fixed a crash bug that could happen when using vocal rhythm import on MIDIs that had one or more track names missing
*/
/*	Changelog: 1.88 to 1.89
//		Fixed a bug that would cause the program to crash if importing a pitched lyric file that ends in a phrase marker (hyphen).  This is now allowed, but a warning is given
//		Moved some verbose output to the debug output option, and improved verbose output
//		Added logic to warn if the given output filename doesn't have the appropriate extension
//		Removed the requirement to use an undocumented parameter to export MIDI with correct pitches, as FoFiX 3.120 has fixed bugs that would have prevented custom vocal charts from working correctly
//		Fixed a previous over-zealous commenting-out that broke the ability to load the artist tag during UltraStar import
//		Added additional error output.  If a text input file (script, UltraStar, pitched lyric) has invalid number fields, the line containing the first error will be given
//		Fixed a logic flaw causing the source MIDI to be processed twice if converting from Vocal Rhythm to MIDI
//		Fixed overdrive and freestyle logic for pitched vocal import
//		Added logic to remove trailing whitespace from lyrics during pitched lyric import
//		As per Harmonix's documentation, I have added logic to interpret '^' as a freestyle indicator and '=' as a hyphen character during MIDI import.
//		Updated readme file
*/
/*	Changelog: 1.87 to 1.88
//		Significantly consolidated tag storing code
//		Fixed a conditional crash bug that could occur if a MIDI track had no defined name
//		Consolidated text file pre-parsing code
//		Added logic to exit with error if imported LRC file is empty
//		Added more error checking
//		Added a debug logging parameter, and offloaded some of the verbose output to it
//		Added logic to correct line durations if imported lyrics have timing overlap errors (ie. LRC)
*/
/*	Changelog: 1.86 to 1.87
//		Redesigned grouping and hyphen handling code, grouping should now work for all formats
//		Changed how lyric line durations are tracked to prevent incorrect values
//		MIDI export now supports writing overdrive phrases
//		Corrected program behavior when no lyrics were imported from a MIDI
//		Added ability to load lyrics from any specified MIDI track name
//		Fixed a bug with noplus that could cause the Lyrics to be corrupted and the program to abort
*/
/*	Changelog: 1.85 to 1.86
//		Improved verbose output
//		Added additional command line parameter validation
//		Added pitch range validation for MIDI import
//		Added logic to prevent double hyphens for partial words during UltraStar import
//		Added logic to append pound characters to freestyle/pitchless lyrics during MIDI export
//		Fixed memory leaks in Vocal Rhythm and MIDI import code
//		Fixed potential crash-inducing code in pitched lyric import code
//		Fixed grouping logic for Vocal Rhythm import
*/
/*	Changelog: 1.84 to 1.85
//		Added a new hybrid input method:  Vocal Rhythm
//		Corrected logic that caused some import files to open as binary instead of text, even though it didn't seem to be causing problems
//		Added additional logic to perform better handling of improperly-formatted MIDIs that would cause false detection overlapping lyrics ("Take it on the run")
//		Now allows an imported UltraStar file to contain an explicit GAP of 0, as some editors may set it so
//		Corrected a bug that would prematurely de-allocate a VL text entry, causing the program to abort when importing a VL file
*/
/*	Changelog: 1.83 to 1.84
//		Fixed a logic error in automatic duration extending for + lyrics that could cause an integer overflow
//		Fixed minor memory leaks in LRC, VL and MIDI import code
//		Improved verbose output for MIDI import
//		Fixed a logic error causing the input MIDI to be parsed twice if the input and output formats were both MIDI
//		Enabled quick MIDI import if exporting MIDI lyrics and not importing MIDI lyrics, as the instrument tracks wouldn't need to be parsed
//		Fixed a bug with UltraStar export that could cause a crash if the last lyric in a line ends with a hyphen
//		Corrected grouping logic for UltraStar import to prevent incorrect grouping detection of the first lyric in a line
//		Corrected grouping logic for UltraStar export to prevent the corruption of timestamps for grouped lyrics
//		Improved grouping logic for UltraStar export to use rounding to prevent unnecessary lyric grouping
//		Added logic to LRC import preventing hyphen appending to a line's last lyric piece if it doesn't end in whitespace
//		Cleaned up source code
*/
/*	Changelog: 1.82 to 1.83
//		Added capability to import LRC and extended LRC format
//		Corrected a very minor conditional memory leak caused if a tag was not recognized during UltraStar import
//		Added duplicate tag checking for all import formats and improved verbose output for tag loading
//		Added more error-checking and verbose output to the lyric handling code
//		Improved the verbose output of lyric handling functions
//		Added logic to UltraStar import so that partial words correctly have hyphens inserted unless suppressed by nohyphens
//		Added logic so that a lyric of "-" is not completely truncated by the nohyphens feature
//		Added pitch scaling logic when converting from UltraStar to MIDI that keeps relative pitch values wherever possible
//		Added logic so that the duration of a lyric is extended to reach a pitch shift (+ lyric) if one follows it
//		Added logic so that an imported UltraStar file's tempo is assumed if a tempo isn't manually supplied by command line
*/
/*	Changelog: 1.81 to 1.82
//		Fixed a bug during VL import that could improperly detect an end of lyric line
//		Fixed a bug where lyric pieces included one character more than they should during VL import
//		Improved the verbose output of VL and MIDI import
//		Improved UltraStar import to handle lines that are blank or begin with multiple whitespace characters
//		Improved error handling to correct lyric overlapping or durations of 0 between lines of lyrics
//		Fixed a bug where exported UltraStar pitches were modified if the import format was also UltraStar (lol)
*/
/*	Changelog: 1.8 to 1.81
//		Fixed a bug where the last line of an imported UltraStar file would not have all available error handling performed.
//		Added failover handling to automatically correct overlapping lyrics pieces (within a line) for lyric import
//		Added handling to automatically correct lyrics pieces (within a line) with a duration of 0 for any lyric import
//		Fixed a bug where UltraStar lyrics were exported as freestyle if the import format was also UltraStar (lol)
//		Added rounding logic to UltraStar import and export, for added accuracy
//		Added marklines parameter to enable exporting to script format with end of line information preserved
//		Added script import logic to recognize end of line information when available
//		Separated regular syntax output and help output from each other, offering a help parameter to see the latter
//		Fixed a bug that prevented the offset and startstamp parameters from working correctly for VL import
//		Added logic to convert ~ lyrics back to + for Ultrastar import
*/
/*	Changelog: 1.7 to 1.8
//		Added capability to import UltraStar format
//		Rewrote all lyric import/export functions to simplify program logic
//		Improved verbose output readability
//		Cleaned up source code and eliminated compiler warnings
//		Corrected all memory leaks, as verified by MemWatch
//		Rewrote nohyphens logic so that it works correctly.  Nohypens 2 will now allow automatically-inserted hyphens from the input file (currently, VL is the only format to utilize this functionality) to be kept.
//		Corrected handling of the offset during MIDI import (when automatically loaded from song.ini)
//		Fixed a logic error in that during VL export, the written offset should be 0 because all timestamps are absolute and not relative
//		Fixed a logic error that could cause a crash if the incorrect amount of parameters were supplied to the program
//		Fixed a logic error that could cause durations in VL export to be 10 ms shorter than they should be if there was no delay between two lyric pieces
//		Corrected the handling of the offset value during VL import
*/
/*	Changelog: 1.63 to 1.7
//		Added capability to export to VL format
//		Fixed a bug that caused incorrect note timing when the brute parameter is used
*/
/*	Changelog: 1.62 to 1.63
//		Added ability to load tag information from song.ini if it exists in the same directory as the input MIDI file (-offset will override any delay found in song.ini)
//		Replaced all single precision floating point variables with double precision floating point
//		Added a brute parameter that checks all tempos from 200-400bpm in increments of .01 to find the most accurate tempo for ustar export.
//		Fixed a bug with UltraStar export that would cause the program to abort when exporting all-freestyle songs
//		UltraStar export code now uses double precision floating point variables for additional accuracy
*/
/*	Changelog: 1.61 to 1.62
//		Fixed a type-casting omission that would cause accented characters to induce a logic error that detects overlapping lyrics (ie. "Finley - Tutto E' Possibile")
//		Added handling for Meta events embedded within a running status (ie. "The Hand That Feeds")
//		All abnormal program termination messages now use the term "aborting" to help search output when run in batch
//		Fixed a bug where a custom tempo is not written to UltraStar format correctly if it contained 0's
*/
/*	Changelog: 1.6 to 1.61
//		Added a startstamp parameter to allow the timestamp of the first lyric to be defined manually, keeping the relative timing for all other lyrics the same
//		For UltraStar export, the first word after a line change no longer has a leading space
//		Added support for recognizing overdrive lyrics in RB MIDIs and saving them as Golden Notes during UltraStar export
//		Added UltraStar export logic to write the tempo in the expected #,# format instead of #.#
//		Added UltraStar logic to prevent lyrics from having a duration of 0 quarter beats
//		Added UltraStar logic to export + lyric pieces as ~ and corrected lyric spacing
//		Added improved handling of trailing whitespace and multiple filtered characters on lyric pieces
//		Improved freestyle detection:  Pitched lyrics are detected as freestyle if the lyric has a # character or if there are no pitch changes throughout the entire set of lyrics.
//		Rock Band's lowest pitch value for note C (#36) is mapped to a neutral note C value for UltraStar (#0)
*/
/*	Changelog: 1.55 to 1.6
//		Added a quick parameter to skip processing for tracks besides PART VOCALS
//		Added capability to export to UltraStar format (with or without pitch)
//		Added a bpm parameter to allow the estimated tempo to be overridden used when exporting to UltraStar
//		Cleaned up the code a little
*/
/*	Changelog: 1.54 to 1.55
//		Added handling for emtpy Meta Event strings (ie. "Papa Roach- Time Is Running Out")
//		Added a definable filter feature to allow removal of pitch characters like #, ^, % and =
//		Cleaned up the code a little
//		Documented the verbose parameter, and increased its use throughout the program.
*/
/*	Changelog: 1.53 to 1.54
//		Cleaned up code to eliminate compiler warnings
//		Corrected a buffer overflow that didn't crash, but could cause unexpected behavior
//		Added value checking to prevent some implementations of atol() from crashing, such as the GCC implementation
//		Added code to release all allocated memory upon completion.  0 memory leaks as validated by MemWatch.
*/
/*	Changelog: 1.52 to 1.53
//		Added logic to handle incorrectly started lines of lyrics, such as in "Maps"
//		Added logic to handle incorrectly started pieces of lyrics, such as in "In Bloom"
//		Added support for interpreting text events as lyrics as long as the events don't begin with an open bracket
*/
/*	Changelog: 1.51 to 1.52
//		Corrected a bug that could cause track 0 to not be exported to MIDI
*/
/*	Changelog: 1.5 to 1.51
//		Compiled the program in Visual C++ 2008 to produce a Win32 executable program for added performance
//		Cleaned up code to prevent data type conversion warnings during compile
//		Fixed an incorrect pointer dereference that could potentially cause the program to be terminated by the OS
*/
/*	Changelog: 1.4 to 1.5
//		Added support for importing script.txt files
//		Added logic to treat Note On events with a velocity of 0 as Note Off events
//		Added logic to ignore incorrectly nested Note events in PART VOCALS, such as in "Welcome Home"
//		Added logic to handle incorrectly terminated lines of lyrics, such as in "Welcome Home"
//		Added logic to handle incorrectly defined Note events that precede Lyric events, such as in "Welcome Home"
//		Song tag information such as Artist, Song Title, Offset, etc. is now embedded in exported MIDI files
*/
/*	Changelog: 1.3 to 1.4
//		Added support for exporting to MIDI
//		Added offset, nohyphens, noplus and grouping command line parameters
//		Fixed logic error regarding the handling of + lyric events
//		Fixed various bugs
//		Rebuilt the command line parameter parsing to be flexible and specific regarding incorrect syntax
//		Added more specific command line parameter errors and more verbose syntax output
//		Added additional validation of lyric timestamps to prevent overlapping and enforce ascending chronological order
//		Added checking so that if no lyrics are found in the input file, the program indicates as such and does not open any files for writing
*/
//
//	TO DOCUMENT:
//
//	FOR FUTURE OPTIMIZATION:
//		See if AddLyricPiece()'s leading/trailing whitespace detection can be used in import functions that perform similar detection (ie. LRC)
//				->Potentially change this so that the groupswithnext parameter of AddLyricPiece() is changed to "grouplogic", one
//				of which is to base grouping on the presence of whitespace between lyrics instead of hyphens, etc.  This would
//				remove the need for special spacing logic for UltraStar and LRC import
//		Rewrite VL_PreLoad() to use regular exit_wrapper instead of conditionally returning.  The calling function can intercept it the same way failed MIDI validation is intercepted
//
//	TO TEST:
//		One of the KAR files I have (White Wedding) has a normal "Words" track and a "Melody" track with pitches but no lyrics
//			->	Attempt to export the Words track to pitched lyrics, export the Melody track with -nolyrics to UltraStar
//			->	Think of a way to merge these two together?
//			->		Add an import that takes two Script format files, either of the files will be specified for each of these
//					criteria: lyric text, timing, pitch.  As Script files, both will have timing, but how the lyric field will be
//					handled is specified by whether the file is designated as the lyric or the pitch file.  If it is the pitch file,
//					the lyric field is expected to be a note in numerical or name (such as C4) form.
//		Test this third party RB MIDI lyric validator: http://www.mimzic.com/authoring-tools/
//			->It doesn't recognize the line phrase markers I am using
//		Test ALL RB/GH Midis again, organize MIDIs into individual folders (ie. RB1, RB2, RBB, DLC) and release rebuild UltraStar files
//			Organize batch files and verify that all RB1,RB1DLC,RB2 and RB2DLC charts are scripted for
//		Test a chart with vocals on Performous
//
//	TO FIX:
//
//		?RemapPitches() crashes on some of the files?
//			->I asked NewCreature for some sample files, but until then I assume it is the calls to assert that I have fixed
//
//	BY REQUEST:
//
//	DONE:
//***
//	TO DO:
//	!	Consider a documented/undocumented switch to suppress lyric detection, for faster program execution for huge batches or horridly slow PCs
//	!	Consider writing wrapper functions for the functions I write to stdout with for errors/logging, so I can define a global variable for
//		which FILE stream to output to (ie. for passing debugging text back to EOF via text file)
//	!	Look for floating point comparison operators and consider multiplying by a number and casting to int before comparison
//	1.	Allow -in to be used without specifying the lyric format, but if the detection can't narrow down to just one
//		possible import, exit with error saying that the import would need to specify the import information
//	2.	Think about how to strengthen ELRC detection.  This won't be possible without having it read several/all lines in the input file to see
//		if any of them have more than one timestamp
//	3.	?Use the import lyric format detection to check if there is a conflict between the specified input format and the detected lyric format,
//		or whether the input lyrics cannot be detected
//	4.	Design a dialog window for EOF lyric import to control the command line parameters
//	5.	Design logic to handle Set Tempo events outside of track 0
//		->	Would have to find the correction location in the existing tempo list and get inserted
//
//		Consider having the Lyrics structure store double floating point timestamps.  The export functions will handle the rounding
//		accordingly.
//		->	Try using the "long double" data type to ensure greatest precision
//		->	Try to ensure that converting from and to the same format allows the lyrics to keep the same timestamps and durations
//		->	When this has been implemented, modify existing import/export formats to make use of this for rounding
//			->	Try processing lyrics in reverse order to make it simpler to check for validity of rounding up
//				This will simplify the rounding logic because I won't have to check whether the next lyric will be able to round up
//				as the rounding will have already taken effect (ie. Ustar rounding)
//		->	Ensure each import function uses long double point variables for timestamps and durations
//		->	Ensure each export function has rounding logic at the beginning to round each lyric piece up to the nearest appropriate intervals
//
//		For UltraStar import:  Add checking to require all tags to be placed before the lyric timings, this is consistent with UltraStar documentation
//
//		?Add support for importing components from multiple formats, ie. lyrics from one, timing from another, pitches from another
//		Add an option to discard pitch styles during pitched import
//
//		For MIDI import, add ability to load tags from the MIDI that weren't read from song.ini, then document this feature
//			Put the MIDI export code that embeds tags/etc. into a function that will be called for exporting vrhythm/KAR as well
//		For MIDI export, add ability to save tags/etc. in an INI file.  If song.ini already exists, try to save as "[songname].ini"
//			If that file already exists, log a message and write exported MIDI
//		Add a savepitch parameter that stores the pitch for each lyric piece to a text file.
//			One line stores one pitch.  This will allow pitch to be saved when exporting to
//			a format that doesn't keep pitch, such as VL, for editing purposes.
//		Add a loadpitch paramater that loads the pitches from the file saved by savepitch.  Pre-
//			parsing will need to be done to ensure that the number of lines imported matches the
//			number of pieces in the imported lyric file, otherwise the pitches can no longer be
//			matched.
//		Rewrite folder path handling code as a function that returns a newly allocated string out of two parameters:
//			A full path to the MIDI and a relative filename.  The function will strip the filename from the full path
//			and append the relative filename and must properly count the number of bytes necessary to store the string
//			This will be useful for generating file paths to pass to other programs, etc.
//		Implement rounding for VL export
//		Implement OGG merging using sox (command line "sox.exe -m infile1... outfile norm 0")
//			Sox is not built with mp3 support, so I can pipe wave output to Lame for mp3 export
//
//
//	For UltraStar export, automatically split lines that are >20-35 characters long (including leading spaces)?
//		->Do not split a ~/+ from the previous lyric piece
//		->Look at OLine font in USDX to see how many chars it takes to go offscreen
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "Lyric_storage.h"
#include "VL_parse.h"
#include "Midi_parse.h"
#include "Script_parse.h"
#include "UStar_parse.h"
#include "LRC_parse.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

//To enable memwatch, add memwatch-2.71\memwatch.c to the project, uncomment the USEMEMWATCH #define in Lyric_storage.h and pass these parameters to the compiler:
//-DMEMWATCH -DMEMWATCH_STDIO

//To enable very verbose warning output, pass these parameters to the compiler:
//-W -Wall -Wcast-align -Wcast-qual -Wchar-subscripts -Werror -Wextra -Wfloat-equal -Wformat -Wformat-security -Winit-self -Winvalid-pch -Wmissing-braces -Wpacked -Wparentheses -Wpointer-arith -Wreturn-type -Wshadow -Wsign-compare -Wswitch -Wswitch-default -Wunknown-pragmas -Wunused -Wwrite-strings -Wcomment -Wsequence-point -Wstrict-aliasing -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -Wno-unused-parameter -Wno-undef

//To use Splint to check for nit picky source code issues, use:
//..\FoFLyricConverter>	 d:\splint-3.1.1\bin\splint.exe -unrecog -nullpass -retvalint +charint -branchstate -mustfreeonly -predboolint -boolops -nullstate -realcompare -compmempass -mustfreefresh +longunsignedintegral +ignoresigns +ignorequals -kepttrans -temptrans -nullret -compdestroy -onlytrans -globstate -retvalother -nullassign +gnuextensions -immediatetrans *.c >splint.txt
//(Splint cannot parse Midi_parse.c, try parsing all source files individually)


static void Input_failed(int errnum,char *argument);
	//If the command line parameters are not correct, this function is called.  If a generic error (unrecognized
	//argument) is passed to errnum, argument will be a reference to argv[errnum]
static void DisplaySyntax(void);	//Displays program syntax
static void DisplayHelp(void);		//Lists verbose program help


//Provide testing code for the exit and assert wrapper functions if EOF_BUILD is defined
#ifndef EOF_BUILD
int main(int argc, char *argv[])
{
//Command line syntax:	FoFLyricConvert [-offset #] [-nohyphens [1|2|3]] [-noplus] [-marklines] [-nolyrics]
//						[-grouping {word | line}] [-verbose | debug] [-filter [...]] [-quick] [-startstamp #]
//						[-bpm #] [-brute] [-help] [-srcoffset #] [-intrack "TRACKNAME"] [-outtrack "TRACKNAME"]
//						[-notenames] [-relative] [-nopitch] [-detect infname]
//						{-in FORMAT infname (lyr | ID)} {-out FORMAT [srcmidi] outfname (lyrics)(vrhythmID)}
	FILE *inf=NULL,*outf=NULL,*srcmidi=NULL,*pitchedlyrics;	//Input and output file pointers (srcmidi is only used to open source midi on midi export, pitchedlyrics is only used during vrhythm export)
	char offset_defined=0;			//Boolean:  Offset value has been defined as a command line parameter
	char srcoffset_defined=0;		//Boolean:  Source offset value has been defined as a command line parameter
	char midi_based_import=0;		//Boolean:  The import format is MIDI based (KAR, MIDI or Vrhythm)
	char midi_based_export=0;		//Boolean:  The export format is MIDI based (KAR, MIDI or Vrhythm)
	int ctr;
	char c;								//Temporary variable
	size_t x,y;							//Temporary variables
	char *correct_extension=NULL;		//Used to validate the file extension
	char *temp=NULL;					//Used to validate the file extension
	char *vrhythmid=NULL;				//A command line parameter used during vrhythm export containing the instrument and difficulty ID (ie. D4 for Expert drums)
	struct Lyric_Line *lineptr=NULL;	//Used for applying the delay from a source MIDI's song.ini file
	struct Lyric_Piece *curpiece=NULL;	//Used for applying the delay from a source MIDI's song.ini file
//	int detectedtype;					//Used for return value of DetectedLyricFormat()
	struct Lyric_Format *detectionlist;	//List of formats returned by DetectedLyricFormat()
	char *detectfile=NULL;				//This is set to the filename to detect if the "-detect" parameter is specified, which will interrupt the regular program control

	if((argc < 2) || (argc > 40))	//# of parameters must be between 2 and 40 (including executable name)
		Input_failed(0xFF,NULL);

	InitLyrics();		//Initialize all variables in the Lyrics structure
	InitMIDI();			//Initialize all variables in the MIDI structure

	for(ctr=1;ctr<argc;ctr++)	//For each specified parameter (skipping executable name)
	{
//Handle -in parameter, which must be immediately followed by script, vl or midi and then by the input filename
//	For vocal rhythm import, the syntax is -in vrhythm srcrhythm input_lyrics
		if(strcasecmp(argv[ctr],"-in") == 0)
		{
			if(argc < ctr+2+1)	//If there are not two more parameters
				Input_failed(0xFF,NULL);

			if(Lyrics.in_format != 0)	//If the user already defined the input file parameters
				Input_failed(0xEF,NULL);

			if(strcasecmp(argv[ctr+1],"script") == 0)
				Lyrics.in_format=SCRIPT_FORMAT;
			else if(strcasecmp(argv[ctr+1],"vl") == 0)
				Lyrics.in_format=VL_FORMAT;
			else if(strcasecmp(argv[ctr+1],"midi") == 0)
			{
				Lyrics.in_format=MIDI_FORMAT;
				midi_based_import=1;
			}
			else if(strcasecmp(argv[ctr+1],"ustar") == 0)
				Lyrics.in_format=USTAR_FORMAT;
			else if(strcasecmp(argv[ctr+1],"lrc") == 0)
				Lyrics.in_format=LRC_FORMAT;
			else if(strcasecmp(argv[ctr+1],"vrhythm") == 0)
			{
				Lyrics.in_format=VRHYTHM_FORMAT;
				midi_based_import=1;
			}
			else if(strcasecmp(argv[ctr+1],"kar") == 0)
			{
				Lyrics.in_format=KAR_FORMAT;
				midi_based_import=1;
			}
			else if(strcasecmp(argv[ctr+1],"skar") == 0)
			{
				Lyrics.in_format=SKAR_FORMAT;
				midi_based_import=1;
			}
			else
				Input_failed(ctr+1,NULL);

			//Ensure next parameter doesn't start with a hyphen, because a filename is expected
			if((argv[ctr+2])[0] == '-')
				Input_failed(0xF0,NULL);

			Lyrics.infilename=argv[ctr+2];		//Store this filename

			if(Lyrics.in_format == VRHYTHM_FORMAT)
			{	//-in vrhythm infname {lyrics | ID}
				if(argc < ctr+3+1)	//Importing this format requires 3 parameters to -in
					Input_failed(0xFF,NULL);

				//Ensure that the next two parameters don't start with a hyphen, one or more filenames or an ID are expected
				if(((argv[ctr+2])[0] == '-') || ((argv[ctr+3])[0] == '-'))
					Input_failed(0xF0,NULL);

				Lyrics.srcrhythmname=argv[ctr+2];	//Store source rhythm midi file's name (it's after "-in vrhythm")
													//Used to import tags from song.ini if it exists

				Lyrics.inputtrack=AnalyzeVrhythmID(argv[ctr+3]);	//Test parameter 3 for being an ID, if so, load the parameters
				if(Lyrics.inputtrack)	//If it was a valid ID
					Lyrics.nolyrics=2;	//Implicitly enable the nolyrics behavior
				else					//If it was not a valid ID, store parameter 3 as a filename
					Lyrics.srclyrname=argv[ctr+3];		//Store the input pitched lyric file's name

				ctr++;		//Seek past one additional parameter because this format included one more than the others
			}

			ctr+=(unsigned short)2;		//seek past these parameters because we processed them
		}

//Handle -out parameter, which must be immediately followed by script,vl or midi, followed by the source midi (if midi is
//the ouput file type and the input type is not MIDI), followed by the output file name
		else if(strcasecmp(argv[ctr],"-out") == 0)
		{
			if(argc < ctr+2+1)	//If there are not at least two more parameters (format and output filename)
				Input_failed(0xFF,NULL);

			if(Lyrics.out_format != 0)	//If the user already defined the output file parameters
				Input_failed(0xEF,NULL);

			if(strcasecmp(argv[ctr+1],"script") == 0)
			{
				Lyrics.out_format=SCRIPT_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".txt");	//.txt is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"vl") == 0)
			{
				Lyrics.out_format=VL_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".vl");	//.vl is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"midi") == 0)	//-out midi (sourcemidi) outputfile
			{	//if output format is midi, the next parameter must be a source midi file, then output file
				//Ensure that the parameter after "midi" is read as a filename (not beginning with hyphen)
				if((argv[ctr+2])[0] == '-')
					Input_failed(0xF0,NULL);

				//Check if there is are one or two filenames given with the -out midi parameter
				if((argc > ctr+3) && ((argv[ctr+3])[0] != '-'))	//If there is a third parameter to -out that doesn't begin with a hyphen
				{	//The command line is -out midi filename1 filename2, filename1 is used as the source, filename2 is the output file
					Lyrics.srcmidiname=argv[ctr+2];	//Store source midi file's name
					Lyrics.outfilename=argv[ctr+3];	//Store output file's name
					ctr++;	//seek past one additional parameter compared to if the source midi was specified in -in
				}
				else
				{	//The command line is -out midi filename, filename is used as the output file
					Lyrics.outfilename=argv[ctr+2];	//Store output file's name
				}

				Lyrics.out_format=MIDI_FORMAT;
				correct_extension=DuplicateString(".mid");	//.mid is the correct extension for this file format
				midi_based_export=1;
			}
			else if(strcasecmp(argv[ctr+1],"ustar") == 0)
			{
				Lyrics.out_format=USTAR_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".txt");	//.txt is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"lrc") == 0)
			{	//Simple LRC format
				Lyrics.out_format=LRC_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".lrc");	//.lrc is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"elrc") == 0)
			{	//Extended LRC format
				Lyrics.out_format=ELRC_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".lrc");	//.lrc is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"vrhythm") == 0)	//-out vrhythm (sourcemidi) outputrythm pitchedlyrics rhythmID
			{	//Vocal Rhythm format
				if(argc < ctr+4+1)	//Importing this format requires at least 4 parameters to -out
					Input_failed(0xFF,NULL);

				//Ensure that the next two parameters don't start with a hyphen, because no new parameters are expected
				if(((argv[ctr+2])[0] == '-') || ((argv[ctr+3])[0] == '-'))
					Input_failed(0xF0,NULL);

				if((argc > ctr+5) && ((argv[ctr+5])[0] != '-'))	//If there is a fifth parameter to -out that doesn't begin with a hyphen
				{	//The command line is -out vrhythm sourcemidi outputrhythm pitchedlyrics rhythmID
					Lyrics.srcmidiname=argv[ctr+2];	//Store the source midi file's name
					Lyrics.outfilename=argv[ctr+3];	//Store output rhythm midi file name (it's after "-in vrhythm")
					Lyrics.dstlyrname=argv[ctr+4];	//Store the output pitched lyric file's name
					vrhythmid=argv[ctr+5];			//Store the vocal rhythm track/difficulty's ID
					ctr+=3;							//Seek past three additional parameters because this format included three more than normal
				}
				else
				{	//The command line is -out vrhythm outputrhythm pitchedlyrics rhythmID
					Lyrics.outfilename=argv[ctr+2];	//Store output rhythm midi file name (it's after "-in vrhythm")
					Lyrics.dstlyrname=argv[ctr+3];	//Store the output pitched lyric file's name
					vrhythmid=argv[ctr+4];			//Store the vocal rhythm track/difficulty's ID
					ctr+=2;							//Seek past two additional parameters because this format included two more than normal
				}

				Lyrics.out_format=VRHYTHM_FORMAT;
				correct_extension=DuplicateString(".mid");	//.mid is the correct extension for the output MIDI
				midi_based_export=1;

				//Validate instrument/difficulty identifier
				if(!isalpha(vrhythmid[0]))	//A letter is expected as the first character of the instrument/difficulty identifier
					Input_failed(0xE9,NULL);

				//Validate extension of output pitched lyric filename
				temp=strrchr(Lyrics.dstlyrname,'.');	//Find last instance of a period
				if(temp != NULL)
					if(strcasecmp(temp,".txt") != 0)	//Compare the filename starting with the last period with the correct extension
						temp=NULL;

				if(temp == NULL)	//If the output filename didn't end with a period, the correct extension and then a null terminator
					printf("\a! Warning: Output pitch file (\"%s\") does not have the correct file extension (.txt)\n",Lyrics.dstlyrname);
			}
			else if(strcasecmp(argv[ctr+1],"skar") == 0)	//-out skar (sourcemidi) output
			{	//Soft Karaoke format
				if((argv[ctr+2])[0] == '-')
					Input_failed(0xF0,NULL);

				//Check if there is are one or two filenames given with the -out midi parameter
				if((argc > ctr+3) && ((argv[ctr+3])[0] != '-'))	//If there is a third parameter to -out that doesn't begin with a hyphen
				{	//The command line is -out skar filename1 filename2, filename1 is used as the source, filename2 is the output file
					Lyrics.srcmidiname=argv[ctr+2];	//Store source midi file's name
					Lyrics.outfilename=argv[ctr+3];	//Store output file's name
					ctr++;	//seek past one additional parameter compared to if the source midi was specified in -in
				}
				else
				{	//The command line is -out midi filename, filename is used as the output file
					Lyrics.outfilename=argv[ctr+2];	//Store output file's name
				}

				Lyrics.out_format=SKAR_FORMAT;
				correct_extension=DuplicateString(".kar");	//.kar is the correct extension for this file format
				midi_based_export=1;
			}
			else if(strcasecmp(argv[ctr+1],"kar") == 0)	//-out kar (sourcemidi) output
			{	//Unofficial KAR format
				if((argv[ctr+2])[0] == '-')
					Input_failed(0xF0,NULL);

				//Check if there is are one or two filenames given with the -out midi parameter
				if((argc > ctr+3) && ((argv[ctr+3])[0] != '-'))	//If there is a third parameter to -out that doesn't begin with a hyphen
				{	//The command line is -out skar filename1 filename2, filename1 is used as the source, filename2 is the output file
					Lyrics.srcmidiname=argv[ctr+2];	//Store source midi file's name
					Lyrics.outfilename=argv[ctr+3];	//Store output file's name
					ctr++;	//seek past one additional parameter compared to if the source midi was specified in -in
				}
				else
				{	//The command line is -out midi filename, filename is used as the output file
					Lyrics.outfilename=argv[ctr+2];	//Store output file's name
				}

				Lyrics.out_format=KAR_FORMAT;
				correct_extension=DuplicateString(".kar");	//.kar is the correct extension for this file format
				midi_based_export=1;
			}
			else	//Any other specified output format is invalid
				Input_failed(ctr+1,NULL);

			//Ensure that the parameter after the format was a filename (doesn't start with hyphen)
			if((argv[ctr+2])[0] == '-')
				Input_failed(0xF0,NULL);

			ctr+=2;		//seek past these parameters because we processed them
		}

//Handle -offset parameter
		else if(strcasecmp(argv[ctr],"-offset") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			if(offset_defined != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(Lyrics.startstampspecified)	//If user defined a custom starting timestamp
				Input_failed(0xED,NULL);

			if(strcasecmp(argv[ctr+1],"0") != 0)
			{	//Only accept the specified offset if it is not zero
				assert_wrapper(argv[ctr+1] != NULL);	//atol() may crash the program if it is passed NULL

				Lyrics.realoffset=atol(argv[ctr+1]);
				if(Lyrics.realoffset == 0)	//if atol() couldn't convert string to integer
					Input_failed(0xFD,NULL);
			}
			else
				Lyrics.realoffset=0;	//User actually specified "-offset 0" on the command line

			Lyrics.offsetoverride=1;	//This offset will override anything specified in the input file
			ctr++;		//seek past next parameter because we processed it
			offset_defined=1;
		}

//Handle -srcoffset parameter
		else if(strcasecmp(argv[ctr],"-srcoffset") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			if(srcoffset_defined != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(strcasecmp(argv[ctr+1],"0") != 0)
			{	//Only accept the specified offset if it is not zero
				assert_wrapper(argv[ctr+1] != NULL);	//atol() may crash the program if it is passed NULL

				Lyrics.srcrealoffset=atol(argv[ctr+1]);
				if(Lyrics.srcrealoffset == 0)	//if atol() couldn't convert string to integer
					Input_failed(0xFD,NULL);
			}
			else
				Lyrics.srcrealoffset=0;	//User actually specified "-offset 0" on the command line

			Lyrics.srcoffsetoverride=1;	//This offset will override anything specified in the input file
			ctr++;		//seek past next parameter because we processed it
			srcoffset_defined=1;
		}

//Handle -nohyphens parameter
		else if(strcasecmp(argv[ctr],"-nohyphens") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
			{					//assume option 3 (suppress all hyphens)
//				Input_failed(0xFF,NULL);
				Lyrics.nohyphens=3;
			}
			else
			{
				if(Lyrics.nohyphens != 0)	//If this parameter was already defined
					Input_failed(0xEF,NULL);

				if(strcasecmp(argv[ctr+1],"0") != 0)
				{	//Only accept the specified nohyphens value if it is not zero
					Lyrics.nohyphens=(char)(atoi(argv[ctr+1]) & 0xFF);	//Enforce capping at an 8 bit value
					if(Lyrics.nohyphens == 0)	//if atoi() couldn't convert string to integer
						Input_failed(0xFD,NULL);
					if(Lyrics.nohyphens > 3)
						Input_failed(0xFC,NULL);;
				}
				else	//If the user specified "-nohyphens 0"
					Input_failed(ctr+1,NULL);

				ctr++;		//seek past next parameter because we processed it
			}
		}

//Handle -grouping parameter
		else if(strcasecmp(argv[ctr],"-grouping") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			if(Lyrics.grouping != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(strcasecmp(argv[ctr+1],"word") == 0)
				Lyrics.grouping=1;
			else if(strcasecmp(argv[ctr+1],"line") == 0)
				Lyrics.grouping=2;
			else
				Input_failed(ctr+1,NULL);

			ctr++;		//seek past next parameter because we processed it
		}

//Handle -noplus parameter
		else if(strcasecmp(argv[ctr],"-noplus") == 0)
		{
			if(Lyrics.noplus != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.noplus=1;
		}

//Handle -verbose parameter
		else if(strcasecmp(argv[ctr],"-verbose") == 0)
		{
			if(Lyrics.verbose != 0)
				Input_failed(0xE7,NULL);		//Can only define verbose/debug output parameter once
			Lyrics.verbose=1;
		}

//Handle -debug parameter
		else if(strcasecmp(argv[ctr],"-debug") == 0)
		{
			if(Lyrics.verbose != 0)
				Input_failed(0xE7,NULL);		//Can only define verbose/debug output parameter once
			Lyrics.verbose=2;
		}

//Handle -filter parameter
		else if(strcasecmp(argv[ctr],"-filter") == 0)
		{
			if(Lyrics.filter != NULL)
				Input_failed(0xEF,NULL);		//Can only define this parameter once

			if((ctr + 1 < argc) && ((argv[ctr+1])[0] != '-'))
			{	//If there's at least one more parameter and it doesn't begin with a hyphen
				Lyrics.filter=argv[ctr+1];		//Use it as the filter list
				//Validate custom filter list
				y=strlen(Lyrics.filter);
				assert_wrapper(y > 0);	//If a parameter has 0 characters, the OS' command line parser is glitching
				for(x=0;x<y;x++)
				{
					c=Lyrics.filter[x];
					if(isalnum((unsigned char)c) || isspace((unsigned char)c) || (c == '-') || (c == '+'))
						Input_failed(0xEE,NULL);
				}
				ctr++;		//seek past next parameter because we processed it
			}
			else	//Assume the default filter list
			{
				if(Lyrics.verbose)	puts("No filter string provided.  Default filter \"^=%#/\" is assumed");
				Lyrics.filter=DuplicateString("^=%#/");
				Lyrics.defaultfilter=1;	//Remember to deallocate this at end of program
			}
		}

//Handle -quick parameter
//	This parameter is only effectively used during MIDI import
		else if(strcasecmp(argv[ctr],"-quick") == 0)
		{
				if(Lyrics.quick != 0)
					Input_failed(0xEF,NULL);	//Can only define this parameter once

				Lyrics.quick=1;
		}

//Handle -bpm parameter
//	This parameter is only allowed for UltraStar export
		else if(strcasecmp(argv[ctr],"-bpm") == 0)
		{
			if(Lyrics.explicittempo > 1.0)	//If already set to a tempo
				Input_failed(0xEF,NULL);	//Can only define this parameter once

			if((ctr + 1 < argc) && ((argv[ctr+1])[0] != '-'))
			{	//If there's at least one more parameter and it doesn't begin with a hyphen
			}
			else
				Input_failed(0xFF,NULL);

			assert_wrapper(argv[ctr+1] != NULL);	//atol() may crash the program if it is passed NULL

			Lyrics.explicittempo=atof(argv[ctr+1]);
			if(Lyrics.explicittempo < 2.0)	//if atol() didn't set an acceptable tempo or couldn't convert string
				Input_failed(0xFD,NULL);
			if(atof(argv[ctr+1]) < 0)		//Supplied tempo cannot be negative
				Input_failed(ctr+1,NULL);

			ctr++;		//seek past next parameter because we processed it
		}

//Handle -startstamp parameter
		else if(strcasecmp(argv[ctr],"-startstamp") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			if(Lyrics.startstampspecified != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(offset_defined != 0)		//If user defined a custom starting timestamp offset
				Input_failed(0xED,NULL);

			if(strcasecmp(argv[ctr+1],"0") != 0)
			{	//Only accept the specified offset if it is not zero
				assert_wrapper(argv[ctr+1] != NULL);	//atol() may crash the program if it is passed NULL

				Lyrics.startstamp=(unsigned long)atol(argv[ctr+1]);
				if(Lyrics.startstamp == 0)	//if atol() couldn't convert string to integer
					Input_failed(0xFD,NULL);
				if(atol(argv[ctr+1]) < 0)	//Supplied starting timestamp cannot be negative
					Input_failed(ctr+1,NULL);
			}
			else
				Lyrics.startstamp=0;	//User actually specified "-startstamp 0" on the command line

			Lyrics.startstampspecified=1;	//This first timestamp will be overridden and the other timestamps will be offsetted
			Lyrics.offsetoverride=1;
			ctr++;		//seek past next parameter because we processed it
		}

//Handle -brute parameter
//	This parameter is only used for UltraStar export
		else if(strcasecmp(argv[ctr],"-brute") == 0)
		{
			if(Lyrics.brute != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.brute = 1;
		}

//Handle -marklines parameter
		else if(strcasecmp(argv[ctr],"-marklines") == 0)
		{
			if(Lyrics.marklines != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.marklines = 1;
		}

//Handle -help parameter
		else if(strcasecmp(argv[ctr],"-help") == 0)
			DisplayHelp();

//Handle -intrack parameter
		else if(strcasecmp(argv[ctr],"-intrack") == 0)
		{
			if(Lyrics.inputtrack != NULL)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			Lyrics.inputtrack=DuplicateString(argv[ctr+1]);
			ctr++;		//seek past next parameter because we processed it
		}

//Handle -outtrack parameter
		else if(strcasecmp(argv[ctr],"-outtrack") == 0)
		{
			if(Lyrics.outputtrack != NULL)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			Lyrics.outputtrack=DuplicateString(argv[ctr+1]);
			ctr++;		//seek past next parameter because we processed it
		}

//Handle -nolyrics parameter
		else if(strcasecmp(argv[ctr],"-nolyrics") == 0)
		{
			if(Lyrics.nolyrics == 1)	//If this parameter was already explicitly defined (not implicitly by importing vrhythm without lyrics)
				Input_failed(0xEF,NULL);

			Lyrics.nolyrics=1;
		}

//Handle -notenames parameter
		else if(strcasecmp(argv[ctr],"-notenames") == 0)
		{
			if(Lyrics.notenames)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.notenames=1;
		}

//Handle -relative parameter
		else if(strcasecmp(argv[ctr],"-relative") == 0)
		{
			if(Lyrics.relative)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.relative=1;
		}

//Handle -nopitch parameter
		else if(strcasecmp(argv[ctr],"-nopitch") == 0)
		{
			if(Lyrics.nopitch)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.nopitch=1;
		}
//Handle -detect parameter
		else if(strcasecmp(argv[ctr],"-detect") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);
			if(detectfile != NULL)	//If this is already defined
				Input_failed(0xEF,NULL);

			detectfile=argv[ctr+1];
			ctr++;		//seek past next parameter because we processed it
		}

//Anything else is not a supported command line parameter
		else
			Input_failed(ctr,argv[ctr]);	//Pass the argument, so that it can be pointed out to the user
	}//end for(ctr=1;ctr<argc;ctr++)

//Output command line
	if(Lyrics.verbose)
	{
		puts(PROGVERSION);	//Output program version
		printf("Invoked with the following command line:\n");
		for(ctr=0;ctr<argc;ctr++)
		{	//For each parameter (including the program name)
			assert_wrapper(argv[ctr] != NULL);	//If this condition is true, then the command line interpreter has failed,

			if(strchr(argv[ctr],' '))		//If the parameter contains whitespace
				putchar('"');				//Begin the parameter with a quotation mark
			printf("%s",argv[ctr]);			//Display the parameter
			if(strchr(argv[ctr],' '))		//If the parameter contains whitespace
				putchar('"');				//End the parameter with a quotation mark
			if(ctr+1<argc)	putchar(' ');	//Display a space if there's another parameter
		}
		putchar('\n');
		putchar('\n');
	}

	if(detectfile != NULL)	//If user opted to detect a file's lyric format instead of performing a conversion
	{
		detectionlist=DetectLyricFormat(detectfile);
		printf("\nDetected lyric format(s) of file \"%s\":\n",detectfile);
		EnumerateFormatDetectionList(detectionlist);
		return 0;	//Return successful program completion
	}

//Verify that the required parameters were defined
	if((Lyrics.in_format==0) || (Lyrics.out_format==0))
		Input_failed(0xFF,NULL);

//Verify that no conflicting parameters were defined
	if(Lyrics.quick && ((Lyrics.in_format != MIDI_FORMAT) && (Lyrics.in_format != KAR_FORMAT)))
		Input_failed(0xEC,NULL);	//The quick parameter is only valid for MIDI import
	if((Lyrics.explicittempo > 1.0) && ((Lyrics.out_format != USTAR_FORMAT) && !midi_based_export))
		Input_failed(0xEB,NULL);	//The BPM parameter is only valid for UltraStar and MIDI based exports
	if(Lyrics.brute && (Lyrics.out_format != USTAR_FORMAT))
		Input_failed(0xEA,NULL);	//The brute parameter is only valid for UltraStar export
	if(Lyrics.marklines && (Lyrics.out_format != SCRIPT_FORMAT))
		Input_failed(0xE0,NULL);	//The marklines parameter is only valid for Script export
	if((Lyrics.nolyrics == 2) && ((Lyrics.in_format != KAR_FORMAT) || (Lyrics.in_format != MIDI_FORMAT)))
		Input_failed(0xE8,NULL);	//The nolyrics parameter can only be specified explicitly for MIDI/KAR import
	if(Lyrics.quick && (!midi_based_import && !midi_based_export))
		Input_failed(0xE6,NULL);	//The quick parameter is only valid for MIDI, KAR or Vrhythm import/export
	if(srcoffset_defined && (Lyrics.srcmidiname == NULL))
		Input_failed(0xE5,NULL);	//The srcoffset parameter is only valid when a source MIDI is provided
	if(Lyrics.inputtrack && (!midi_based_import || Lyrics.in_format == SKAR_FORMAT))
		Input_failed(0xE4,NULL);	//The intrack parameter is only valid for MIDI or KAR import (not for SKAR import)
	if(Lyrics.outputtrack && !midi_based_export)
		Input_failed(0xE3,NULL);	//The outtrack parameter is only valid for MIDI or KAR export
	if(Lyrics.notenames && (Lyrics.out_format != VRHYTHM_FORMAT))
		Input_failed(0xE2,NULL);	//The notenames parameter is only valid for Vrhythm export
	if(Lyrics.relative && (Lyrics.out_format != USTAR_FORMAT))
		Input_failed(0xE1,NULL);	//The relative parameter is only valid for UltraStar export
	if((Lyrics.outputtrack != NULL) && (Lyrics.out_format == SKAR_FORMAT))
		Input_failed(0xD0,NULL);	//The outputtrack parameter may not be used for SKAR export, which requires pre-determined track names

//Display informational messages regarding parameters/formats
	if((Lyrics.grouping == 2) && Lyrics.marklines)	//User specified both marklines and line grouping
	{
		puts("Marklines is disabled when line grouping is specified");
		Lyrics.marklines=0;
	}
	if(Lyrics.grouping && ((Lyrics.out_format == MIDI_FORMAT) || (Lyrics.out_format == USTAR_FORMAT)))
		puts("Warning: Grouped lyrics will lose some timing and pitch information");

//If output file is MIDI based, validate the source midi file name if given
	if(midi_based_export)
	{	//verify that the source MIDI file was specified
		assert_wrapper(Lyrics.outfilename != NULL);	//It should not be possible for this to be NULL

		//If a source MIDI file was given, verify that it is not the same file as the output file
		if((Lyrics.srcmidiname != NULL) && (strcasecmp(Lyrics.srcmidiname,Lyrics.outfilename) == 0))
			Input_failed(0xFE,NULL); //special case:  Imported and exported MIDI file cannot be the same because one is read into the other
	}

//Check if the specified output filename has the correct file extension
//if not, present a warning to the user
	assert_wrapper(correct_extension != NULL);
	temp=strrchr(Lyrics.outfilename,'.');	//Find last instance of a period
	if(temp != NULL)
		if(strcasecmp(temp,correct_extension) != 0)	//Compare the filename starting with the last period with the correct extension
			temp=NULL;

	if(temp == NULL)	//If the output filename didn't end with a period, the correct extension and then a null terminator
		printf("\a! Warning: Output file does not have the correct file extension (%s)\n",correct_extension);

	free(correct_extension);	//Free this, as it is not referenced anymore

//	if(Lyrics.verbose)	printf("\nDetected lyric format is: %s (type #%d)\n\n",LYRICFORMATNAMES[detectedtype+1],detectedtype);
	detectionlist=DetectLyricFormat(Lyrics.infilename);
	if(Lyrics.verbose)
	{
		printf("\nDetected lyric format(s) of file \"%s\":\n",Lyrics.infilename);
		EnumerateFormatDetectionList(detectionlist);
	}

//Open the input file and import the specified format
	fflush_err(stdout);
	switch(Lyrics.in_format)
	{
		case SCRIPT_FORMAT:	//Load script.txt format file as input
			inf=fopen_err(Lyrics.infilename,"rt");	//Script is a text format
			Script_Load(inf);
		break;

		case VL_FORMAT:	//Load VL format file as input
			inf=fopen_err(Lyrics.infilename,"rb");	//VL is a binary format
			VL_Load(inf);
		break;

		case MIDI_FORMAT:	//Load MIDI format file as input
			if(Lyrics.inputtrack == NULL)	//If no input track name was specified via command line
				Lyrics.inputtrack=DuplicateString("PART VOCALS");	//Default to PART VOCALS

			inf=fopen_err(Lyrics.infilename,"rb");	//MIDI is a binary format
			Parse_Song_Ini(Lyrics.infilename,1,1);	//Load ALL tags from song.ini first, as the delay tag will affect timestamps
			MIDI_Load(inf,Lyric_handler,0);	//Call MIDI_Load, specifying the new KAR-compatible Lyric Event handler
		break;

		case USTAR_FORMAT:	//Load UltraStar format file as input
			inf=fopen_err(Lyrics.infilename,"rt");	//UltraStar is a text format
			UStar_Load(inf);
		break;

		case LRC_FORMAT:	//Load LRC format file as input
		case ELRC_FORMAT:
			inf=fopen_err(Lyrics.infilename,"rt");	//LRC is a text format
			LRC_Load(inf);
		break;

		case VRHYTHM_FORMAT:	//Load vocal rhythm (MIDI) and pitched lyrics
			inf=fopen_err(Lyrics.infilename,"rb");	//Vrhythm is a binary format
			VRhythm_Load(Lyrics.srclyrname,Lyrics.srcrhythmname,inf);
		break;

		case KAR_FORMAT:	//Load KAR MIDI file
		case SKAR_FORMAT:
			inf=fopen_err(Lyrics.infilename,"rb");	//KAR is a binary format
			if(!Lyrics.inputtrack || (Lyrics.in_format==SKAR_FORMAT) || !strcasecmp(Lyrics.inputtrack,"Words"))	//If user specified no input track, specified SKAR import or specified an input track of "Words"
			{	//Perform official "Soft Karaoke" KAR logic (load lyrics based on Text events in a track called "Words")
				if(Lyrics.inputtrack != NULL)
					free(Lyrics.inputtrack);
				Lyrics.inputtrack=DuplicateString("Words");
				if(Lyrics.verbose)	puts("Using Soft Karaoke import logic");
				Lyrics.in_format=SKAR_FORMAT;
				MIDI_Load(inf,SKAR_handler,0);	//Call MIDI_Load, specifying the Simple Karaoke Event handler
			}
			else
			{	//Perform KAR logic to load lyrics based off of Note On/Off events
				assert_wrapper(Lyrics.inputtrack != NULL);
				if(Lyrics.verbose)	puts("Using RB/KAR import logic");
				MIDI_Load(inf,Lyric_handler,0);	//Call MIDI_Load, specifying the new KAR-compatible Lyric Event handler
			}
//v2.0	Moved this function call to end of MIDI_Load()
//			EndLyricLine();	//KAR files do not demarcate the end of the last line of lyrics
		break;

		default:
			puts("Unexpected error in import switch\nAborting");
			exit_wrapper(2);
		break;
	}
	fflush_err(stdout);
	fclose_err(inf);	//Ensure this file is closed
	inf=NULL;

	assert_wrapper(Lyrics.line_on == 0);	//Import functions are expected to ensure this condition

//If there were no lyrics found in the input file, exit program without opening a file for writing
	if(Lyrics.piececount == 0)
	{
		puts("No lyrics detected in input file.  Exiting");
		ReleaseMemory(1);
		return 0;
	}

//If importing KAR or MIDI and the MIDI_Lyrics linked list isn't empty, exit program
	if(MIDI_Lyrics.head != NULL)
	{
		printf("Invalid input KAR or MIDI file:  At least one lyric's (\"%s\") note off event is missing.\nAborting\n",MIDI_Lyrics.head->lyric);
		exit_wrapper(3);
	}

	if(Lyrics.verbose)	printf("\n%lu lines of lyrics loaded.\n",Lyrics.linecount);
	PostProcessLyrics();	//Perform hyphen and grouping validation/handling

	if(Lyrics.pitch_tracking && (Lyrics.out_format == MIDI_FORMAT) && CheckPitches(NULL,NULL))
	{	//Only perform input pitch validation and remapping if the import lyrics had pitch and being exported to MIDI
		puts("\aWarning: Input vocal pitches are outside Harmonix's defined range of [36,84]\nCorrecting");
		RemapPitches();		//Verify vocal pitches are correct, remap them if necessary
	}

//Source MIDI logic
//Handle the timing for MIDI based exports based on the value of Lyrics.srcmidiname
//v2.0	Implement use of midi_based_export status
//	if((Lyrics.out_format == MIDI_FORMAT) || (Lyrics.out_format == VRHYTHM_FORMAT) || (Lyrics.out_format == KAR_FORMAT))
	if(midi_based_export)
	{	//If outputting to a MIDI based export format, use either a default tempo or the tempos from the source midi
		outf=fopen_err(Lyrics.outfilename,"wb");	//These are binary formats

		ReleaseMIDI();	//Disregard contents of input MIDI if applicable
		InitMIDI();

		if(Lyrics.Offset != NULL)	//Disregard any current offset value
		{
			Lyrics.realoffset=0;
			free(Lyrics.Offset);
			Lyrics.Offset=NULL;
		}

		if(Lyrics.out_format == VRHYTHM_FORMAT)
		{
			Lyrics.outputtrack=AnalyzeVrhythmID(vrhythmid);		//Determine the output MIDI track (instrument) name and note range (difficulty), so the output track can be omitted from the source MIDI copy
			if(Lyrics.outputtrack == NULL)	//Validate return value
			{
				printf("Error: Invalid output vocal rhythm identifier \"%s\"\nAborting\n",vrhythmid);
				exit_wrapper(4);
			}
		}
		if(Lyrics.out_format == MIDI_FORMAT)
			if(Lyrics.outputtrack == NULL)							//If no custom output track name was provided
				Lyrics.outputtrack=DuplicateString("PART VOCALS");	//Write track name as PART VOCALS by default

		if(Lyrics.srcmidiname != NULL)	//If a source MIDI was provided for export
		{
			srcmidi=fopen_err(Lyrics.srcmidiname,"rb");	//This is the source midi to read from

			if(Lyrics.verbose)	printf("Loading timing from specified source file \"%s\"\n",Lyrics.srcmidiname);
			Lyrics.quick=1;								//Should be fine to skip everything except loading basic track info
			MIDI_Load(srcmidi,NULL,0);					//Call MIDI_Load with no handler (just load MIDI info) Lyric structure is NOT re-init'd- it's already populated

			if(!Lyrics.srcoffsetoverride)				//If the offset for the source file is not being manually specified
				Parse_Song_Ini(Lyrics.srcmidiname,1,0);	//Load ONLY the offset tag from the song.ini file, it will be stored negative from the contents of song.ini, so add to timestamps instead of subtract
			else
				Lyrics.realoffset=Lyrics.srcrealoffset;	//Otherwise use the offset specified via command line

			if(Lyrics.realoffset != 0)
			{	//If there is an offset to apply to the source timing
				if(Lyrics.verbose)	printf("Applying additive offset of %ld from source MIDI file's song.ini\n",Lyrics.realoffset);
				for(lineptr=Lyrics.lines;lineptr!=NULL;lineptr=lineptr->next)	//For each line of lyrics
				{
					for(curpiece=lineptr->pieces;curpiece!=NULL;curpiece=curpiece->next)	//For each lyric in the line
					{
						if((long)curpiece->start < Lyrics.realoffset)
						{
							printf("Error: Offset in source MIDI is larger than lyric timestamp.\nAborting\n");
							exit_wrapper(5);
						}
						if(Lyrics.verbose>=2)	printf("\tLyric \"%s\"\tRealtime: %lu\t->\t%lu\n",curpiece->lyric,curpiece->start,curpiece->start+Lyrics.realoffset);
						curpiece->start+=Lyrics.realoffset;
					}
				}
			}

			Copy_Source_MIDI(srcmidi,outf);	//Copy all tracks besides destination vocal track to output file
			fclose_err(srcmidi);
			srcmidi=NULL;
		}
		else											//Assume default tempo
			Write_Default_Track_Zero(outf);				//Write up to the end of track 0 for a default MIDI and configure MIDI variables
	}


//Open the output file for text/binary writing and export to the specified format
	switch(Lyrics.out_format)
	{
		case SCRIPT_FORMAT:	//Export as script.txt format file
			outf=fopen_err(Lyrics.outfilename,"wt");	//Script.txt is a text format
			Export_Script(outf);
		break;

		case VL_FORMAT:	//Export as VL format file
			outf=fopen_err(Lyrics.outfilename,"wb");	//VL is a binary format
			ExportVL(outf);
		break;

		case MIDI_FORMAT:	//Export as MIDI format file.  Default export track is "PART VOCALS"
			Export_MIDI(outf);
		break;

		case USTAR_FORMAT:	//Export as UltraStar format file
			outf=fopen_err(Lyrics.outfilename,"wt");	//UltraStar is a text format
			Export_UStar(outf);
		break;

		case LRC_FORMAT:	//Export as simple LRC
		case ELRC_FORMAT:	//Export as extended LRC
			outf=fopen_err(Lyrics.outfilename,"wt");	//LRC is a text format
			Export_LRC(outf);
		break;

		case VRHYTHM_FORMAT:	//Export as Vocal Rhythm (MIDI and text file)
			pitchedlyrics=fopen_err(Lyrics.dstlyrname,"wt");	//Pitched lyrics is a text format
			Export_Vrhythm(outf,pitchedlyrics,vrhythmid);
			fflush_err(pitchedlyrics);	//Commit any pending pitched lyric writes to file
			fclose_err(pitchedlyrics);	//Close pitched lyric file
		break;

		case SKAR_FORMAT:	//Export as Soft Karaoke.  Default export track is "Words"
			Export_SKAR(outf);
		break;

		case KAR_FORMAT:	//Export as unofficial KAR.  Default export track is "Melody"
			if(Lyrics.outputtrack == NULL)
			{
				puts("\aNo ouput track name for KAR file was given.  A track named \"Melody\" will be used by default");
				Lyrics.outputtrack=DuplicateString("Melody");
			}
			Export_MIDI(outf);
		break;

		default:
			puts("Unexpected error in export switch\nAborting");
			exit_wrapper(4);
		break;
	}

//	if((Lyrics.out_format == MIDI_FORMAT) || (Lyrics.out_format == VRHYTHM_FORMAT) || (Lyrics.out_format == KAR_FORMAT))
	if(midi_based_export)
	{	//Update the MIDI header to reflect the number of MIDI tracks written to file for all applicable export formats
		fseek_err(outf,10,SEEK_SET);		//The number of tracks is 10 bytes in from the start of the file header
		fputc_err(MIDIstruct.trackswritten>>8,outf);
		fputc_err(MIDIstruct.trackswritten&0xFF,outf);
	}

//Commit any pending writes to file and close input and output files
	if(Lyrics.verbose)	puts("\nCleaning up");
	if(Lyrics.verbose>=2)	puts("\tClosing files");
	if(srcmidi != NULL)
	{
		fclose_err(srcmidi);
		srcmidi=NULL;
	}
	fflush_err(outf);
	fclose_err(outf);
	fflush_err(stdout);

	ReleaseMemory(1);
	return 0;	//return success
}
#else	//If EOF_BUILD is defined, provide some test code
int main(int argc, char *argv[])
{
	char string[]="TEST";
	int jumpcode;

	puts("Beginning EOF wrapper test");

//Test assert handling
	jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
	if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
	{
		puts("Assert handled sucessfully!");
	}
	else //jumpcode is zero, meaning that this was the original call to setjmp() and EOF can call my logic to import lyrics
	{
		//Test the assert wrapper by calling ReadMIDIHeader() with NULL
		ReadMIDIHeader(NULL,0);
	}

//Test exit handling
	jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
	if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
	{
		puts("Exit handled sucessfully!");
	}
	else //jumpcode is zero, meaning that this was the original call to setjmp() and EOF can call my logic to import lyrics
	{
		//Test the exit wrapper by calling AddLyricPiece() with a pitch greater than 127
		AddLyricPiece(string,0,1,255,0);
	}

	return 0;	//return success
}
#endif

void DisplaySyntax(void)
{
	puts("\nSyntax:\nFoFLyricConvert [-offset #][-nohyphens [1|2|3]][-noplus][-marklines][-nolyrics]");
	puts(" [-grouping {word|line}][-verbose|debug][-filter [...]][-quick][-startstamp #]");
	puts(" [-bpm #][-brute][-help][-srcoffset #][-intrack TRACKNAME][-outtrack TRACKNAME]");
	puts(" [-notenames][-relative][-nopitch][-detect infname]");
	puts(" {-in FORMAT infname (lyr|ID)} {-out FORMAT [srcmidi] outfname (lyr)(ID)}");
	puts("\nParameters in braces {} are required, those in brackets [] are optional");
	puts("Parameters in parentheses are required depending on the in/output FORMAT");
}

void Input_failed(int errnum,char *argument)
{
	puts("\n");	//Add leading newlines
	if(errnum == 0xFF)
		puts("Error: Incorrect number of parameters");
	else if(errnum == 0xFE)
		puts("Error: Cannot import and export the same file");
	else if(errnum == 0xFD)
		puts("Error: Invalid numerical value");
	else if(errnum == 0xFC)
		puts("Error: The optional nohyphens flag must be 1, 2 or 3");
//v2.0	Found that these error codes weren't being used anymore
//	else if(errnum == 0xFB)
//		puts("Error: Only one input MIDI can be specified");
//	else if(errnum == 0xFA)
//		puts("Error: Input file isn't MIDI- source and output MIDI files must be specified");
	else if(errnum == 0xF0)
		puts("Error: File name expected");
	else if(errnum == 0xEF)
		puts("Error: Each parameter can only be defined once");
	else if(errnum == 0xEE)
		puts("Error: Invalid character in filter string");
	else if(errnum == 0xED)
		puts("Error: The \"startstamp\" and \"offset\" parameters cannot be used simultaneously");
	else if(errnum == 0xEC)
		puts("Error: The \"quick\" parameter can only be used for MIDI import");
	else if(errnum == 0xEB)
		puts("Error: The \"bpm\" parameter can only be used for UltraStar or MIDI based export");
	else if(errnum == 0xEA)
		puts("Error: The \"brute\" parameter can only be used for UltraStar export");
	else if(errnum == 0xE0)
		puts("Error: The \"marklines\" parameter can only be used for Script export");
	else if(errnum == 0xE9)
		puts("Error: Vocal rhythm instrument+difficulty identifier is expected");
	else if(errnum == 0xE8)
		puts("Error: The \"nolyrics\" parameter can only be specified explicitly for MIDI/KAR import");
	else if(errnum == 0xE7)
		puts("Error: The \"verbose\" and \"debug\" parameters may not be used simultaneously");
	else if(errnum == 0xE6)
		puts("Error: The \"quick\" parameter is only valid for MIDI, KAR or Vrhythm import/export");
	else if(errnum == 0xE5)
		puts("Error: The \"srcoffset\" parameter is only valid when a source MIDI is provided");
	else if(errnum == 0xE4)
		puts("Error: The \"intrack\" parameter is only valid for MIDI or KAR import");
	else if(errnum == 0xE3)
		puts("Error: The \"outtrack\" parameter is only valid for MIDI or KAR export");
	else if(errnum == 0xE2)
		puts("Error: The \"notenames\" parameter is only valid for Vrhythm export");
	else if(errnum == 0xE1)
		puts("Error: The \"relative\" parameter is only valid for UltraStar export");
	else if(errnum == 0xD0)
		puts("Error: The \"outtrack\" parameter may not be used for Soft Karaoke export");
	else
	{
		printf("Parameter %d is incorrect\n",errnum);
		if(argument != NULL)	printf("(%s)\n",argument);
		puts("");
	}

	DisplaySyntax();
	puts("\nUse FoFLyricConvert -help for detailed help");

	exit_wrapper(1);
}

void DisplayHelp(void)
{
//Describe general syntax and required parameters
	DisplaySyntax();
	puts("-The use of detect will halt a conversion specified by the other parameters");
	puts("-In/output FORMAT must be script,vl,midi,ustar,lrc,elrc,vrhythm,kar or skar");
	puts("-infname is the name of the input file of the FORMAT preceding it");
	puts("-outfname is the name of the file to create in the FORMAT preceding it");
	puts("-If the input format is vrhythm, a source rhythm MIDI file (infname)");
	puts(" containing the vocal timings must be specified, followed by the input pitched");
	puts(" lyrics file or vrhythm track ID.  Both files are unique to this format, ie:");
	puts("\tFoFLyricConvert -in vrhythm timing.mid lyrics.txt -out ustar vox.txt");
	puts("-(srcmidi) may be used for MIDI based export formats for merging the lryics");
	puts(" with an existing file, keeping its timing intact.  For example:");
	puts("\tFoFLyricConvert -in vl myfile.vl -out midi notes.mid withlyrics.mid");
	puts("-If the output format is vrhythm, an output vocal rhythm file, pitched lyric");
	puts(" file and instrument/difficulty identifier must be specified, for example:");
	puts("\tFoFLyricConvert -in vl song.vl -out vrhythm rhythm.mid pitches.txt G4");
	printf("\nPress Enter for next screen");
	fflush_err(stdin);
	fgetc_err(stdin);	//wait for keypress

//Describe optional parameters
	puts("\n\n\tOPTIONAL PARAMETERS:");
	puts("-The offset parameter will subtract # milliseconds from the");
	puts(" beginning of all lyric timestamps.  Provide your song's delay value with this");
	puts("-The nohyphens flag will control how trailing hyphens are handled:");
	puts(" If 1 or 3, hyphens are not inserted.  If 2 or 3, hyphens in input are ignored");
	puts("-The noplus parameter will remove + lyric events from source");
	puts("-The marklines parameter retains line markings for script export");
	puts("-The nolyrics parameter loads lyrics from a MIDI or KAR input file, ignoring");
	puts(" the presence or absence of lyric text, just loading pitches and durations");
	puts("-The grouping parameter will control output lyric separation");
	puts("-Specify verbose or debug to log information to the screen");
	puts("-The filter parameter allows a defined set of characters to be");
	puts(" removed from end of lyric pieces.  If a character set isn't defined, ^=%#/ is");
	puts(" assumed.  The filter CAN'T contain space, alphanumerical, + or - characters");
	puts("-The quick parameter speeds up MIDI import");
	puts("-The startstamp parameter can be a substitute for offset by");
	puts(" overriding the starting timestamp, offsetting all following lyric timestamps");
	puts("-The bpm allows a custom tempo for exported ustar or MIDI based files");
	puts("-The brute parameter finds the best tempo for Ultrastar export");
	puts("-The srcoffset parameter allows the lyric timing for the source");
	puts(" file to be offset manually, such as to override what is read from song.ini");
	printf("\nPress Enter for next screen");
	fflush_err(stdin);
	fgetc_err(stdin);	//wait for keypress

//Describe notes
	puts("\n\n\tOPTIONAL PARAMETERS (cont'd):");
	puts("-The intrack and outtrack parameters allow the import or export MIDI track to");
	puts(" be manually defined (ie. \"PART HARM1\" for RB:Beatles MIDIs).");
	puts("-The notenames parameter causes note names to be written instead of note");
	puts(" numbers during Vrhythm export");
	puts("\n\tNOTES:");
	puts("* Input (including the source midi) and output files may not be the same");
	puts("* Filenames may not begin with a hyphen (ie. \"-notes.mid\" is not accepted)");
	puts("* Parameters containing whitespace (ie. filenames) must be enclosed in");
	puts("  quotation marks");

	exit_wrapper(1);
}
