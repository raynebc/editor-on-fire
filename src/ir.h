#ifndef EOF_IR_H
#define EOF_IR_H

extern int eof_ir_export_allow_fhp_finger_placements;	//Tracks whether the user opts to use finger placements derived from FHPs during export

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
	// with .jpg, .jpeg, .png or .tiff extensions.
	//If any of these filenames exist, the first one found is stored in match
	// and nonzero is returned.  If no matches are found, 0 is returned
	//match_arraysize is the number of bytes the match array can store, to avoid
	// buffer overflow
int eof_check_for_immerrock_album_art(char *folderpath, char *album_art_filename, unsigned long filenamesize, char force_recheck);
	//Checks for the presence of any JPG, PNG or TIF file with a base filename of "Cover", "Album", "Label" or "Image" in the given folder path
	//If invalid parameters are given, zero is returned
	//If no file match is found, zero is returned and the album_art_filename string is emptied
	//If a match is found, the matching file name is written to the album_art_filename string (bounds limited to filenamesize) and nonzero is returned
	//Unless force_recheck is nonzero, the lookup results are cached and not replaced unless at least 10 seconds have passed since the last recheck
int eof_detect_immerrock_arrangements(unsigned long *lead, unsigned long *rhythm, unsigned long *bass);
	//Examines the arrangement type of the active project's pro guitar tracks as would export to IMMERROCK format
	//The first populated lead, rhythm and bass arrangement's track numbers are stored via reference
	//Returns nonzero on success
unsigned long eof_count_immerrock_sections(void);
	//Counts the number of text events that would export as section events
	//Returns 0 if there are no such events, or upon error
unsigned long eof_count_immerrock_chords_missing_fingering(unsigned long *total);
	//Counts the number of chords in the active pro guitar track that have incomplete fingering and returns the count
	//If the active track has dynamic difficulty, the flattened version of the highest difficulty level is checked, otherwise the active static difficulty is checked
	//Only notes with at least two non ghosted/muted gems are examined, and muted strings are not required to have fingering defined
	//If total is not NULL, the number of applicable chords that were checked are returned through it
	//Returns zero on error
int eof_lookup_immerrock_effective_section_at_pos(EOF_SONG *sp, unsigned long pos, char *section_name, unsigned long section_name_size);
	//Examines the text events and stores the name of the section event immediately at/before thse specified position (if any) into section_name[]
	//Returns 1 if a matching section is found.  If none is found, section_name[] is emptied and 0 is returned.  0 is returned upon error

unsigned char eof_pro_guitar_track_find_effective_hand_mode_change(EOF_PRO_GUITAR_TRACK *tp, unsigned long position, EOF_PHRASE_SECTION **ptr, unsigned long *index);
	//Returns the hand mode in effect at the specified position (0 = chord mode, 1 = string mode)
	//If ptr is not NULL, the pointer to the effective hand mode change is returned through it
	//If index is not NULL, the hand mode change's index number in the array is returned through it
int eof_pro_guitar_track_set_hand_mode_change_at_timestamp(unsigned long timestamp, unsigned long mode);
	//Sets the specified hand mode at the specified timestamp, editing any existing hand mode change at that timestamp before creating one
void eof_pro_guitar_track_delete_hand_mode_change(EOF_PRO_GUITAR_TRACK *tp, unsigned long index);
	//Deletes the specified hand mode change

unsigned long eof_ir_get_rs_section_instance_number(EOF_SONG *sp, unsigned long gglead, unsigned long ggrhythm, unsigned long ggbass, unsigned long event);
	//Similar to eof_get_rs_section_instance_number(), but since IMMERROCK has one list of sections shared among all 3 exported arrangements,
	// all three specified track numbers are in scope for the numbering
	//If a Rocksmith section's associated track is 0, then it is not track specific
	//Upon error, or if the given event is not a Rocksmith section in the specified track, 0 is returned

int eof_export_immerrock_diff(EOF_SONG *sp, unsigned long gglead, unsigned long ggrhythm, unsigned long ggbass, unsigned char diff, char *destpath, char option);
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
