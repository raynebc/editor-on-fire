#ifndef EOF_IR_H
#define EOF_IR_H

int qsort_helper_immerrock(const void * e1, const void * e2);
	//A sort algorithm used when quick sorting the eof_midi_event[] array for use with IMMERROCK, which has some unique requirements
	//such as properly sorting pairs or note on and off events that are all at the same timestamp, using the index variable to define sort order
	//If index is nonzero for both events being compared, the lower number sorts earlier
void eof_add_midi_pitch_bend_event(unsigned long pos, unsigned value, int channel, unsigned long index);
	//Calls eof_add_midi_event_indexed() to add a pitch bend event with the defined bend value
	//Index is incremented for each added MIDI event to ensure desired sort order
	//A bend value of 8192 is considered neutal, lower values are a pitch bend down, higher values are a pitch bend up
	//Each 640 increment is considered a quarter step in pitch
void eof_add_midi_pitch_bend_event_qsteps(unsigned long pos, unsigned quarter_steps, int channel, unsigned long index);
	//Calls eof_add_midi_pitch_bend_event() to add a a pitch bend event
	//Index is incremented for each added MIDI event to ensure desired sort order
	//Each quarter step adds a pitch bend amount of 640 to the neutral bend value of 8192
int eof_export_immerrock_midi(EOF_SONG *sp, unsigned long track, unsigned char diff, char *fn);
	//Exports the specified pro guitar track difficulty to a MIDI file suited for use in the game IMMERROCK
int eof_search_image_files(char *folderpath, char *filenamebase, char *match, unsigned long match_arraysize);
	//Accepts a path to a folder (which must end in a folder separator),
	// uses replace_filename() to append the file name base, then uses
	// replace_extension() to append and test for the existence of the file
	// with .jpg, .png or .tiff extensions.
	//If any of these filenames exist, the first one found is stored in match
	// and nonzero is returned.  If no matches are found, 0 is returned
	//match_arraysize is the number of bytes the match array can store, to avoid
	// buffer overflow
int eof_check_for_immerrock_album_art(char *folderpath, char *album_art_filename, unsigned long filenamesize, char force_recheck);
	//Checks for the presence of any JPG, PNG or TIF file with a base filename of "cover", "album", "label" or "image" in the given folder path
	//If invalid parameters are given, zero is returned
	//If no file match is found, zero is returned and the album_art_filename string is emptied
	//If a match is found, the matching file name is written to the album_art_filename string (bounds limited to filenamesize) and nonzero is returned
	//Unless force_recheck is nonzero, the lookup results are cached and not replaced unless at least 10 seconds have passed since the last recheck
int eof_export_immerrock_diff(EOF_SONG * sp, unsigned long gglead, unsigned long ggrhythm, unsigned long ggbass, unsigned char diff, char *destpath, char option);
	//Exports IMMERROCK files for the specified pro guitar tracks, for the specified difficulty in a folder with multiple files
	//gglead, ggrhythm and ggbass are the track numbers to export as these arrangements, or 0 if that arrangement is not specified for export
	//destpath will be the folder level at which the folder of IMMERROCK files will be written
	//If option is 1 (manual export of single arrangement), the created subfolder will include the name of the exported track
	//If the specified tracks are not pro guitar tracks, or have no notes in the specified difficulty, no files or folders are created
void eof_export_immerrock(char silent);
	//Chooses up to one lead, one rhythm and one bass arrangement for export based on the defined arrangement type of the active project's tracks
	//Uses eof_export_immerrock_diff() to export content for these arrangements so that all easy difficulty level content is in one folder, medium in another folder, etc.
	//If silent is nonzero, warnings and prompts are suppressed in order to perform a quick save operation

#endif
