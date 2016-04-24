#ifndef EOF_MENU_FILE_H
#define EOF_MENU_FILE_H

#include <stdio.h>
#include "../song.h"
#include "../control.h"
#include "../chart_import.h"

extern MENU eof_file_menu[];

extern DIALOG eof_settings_dialog[];
extern DIALOG eof_preferences_dialog[];
extern DIALOG eof_display_dialog[];
extern DIALOG eof_ogg_settings_dialog[];
extern DIALOG eof_controller_settings_dialog[];
extern DIALOG eof_file_new_dialog[];
extern DIALOG eof_file_new_windows_dialog[];

extern DIALOG eof_lyric_detections_dialog[];	//The dialog used to prompt the user to select a MIDI track to import from
extern struct Lyric_Format *lyricdetectionlist;	//Dialog functions cannot be passed local variables, requiring the use of this global variable
extern char gp_import_undo_made;				//Dialog functions cannot be passed local variables, requiring the use of this global variable

extern struct eof_guitar_pro_struct *eof_parsed_gp_file;	//Dialog windows cannot be passed local variables, requiring the use of this global variable
extern DIALOG eof_gp_import_dialog[];	//The dialog used to display the tracks imported from a Guitar Pro file, allowing one to overwrite the active pro guitar track
int eof_gp_import_track(DIALOG * d);
	//The function performed when the user selects and imports a track from the above dialog
	//If the gp_import_undo_made global variable is zero, an undo state is made and this variable is set to 1

void eof_prepare_file_menu(void);

int eof_menu_file_new_wizard(void);
	//Prompts for path to an audio file and then uses eof_new_chart() to continue the rest of the process of initializing a new chart
int eof_new_chart(char * filename);
	//Accepts an input audio file, prompts user for chart name/details/destination, encoding settings if applicable and initializes EOF variables
	//Returns nonzero on failure/cancellation

int eof_menu_file_load(void);
int eof_menu_file_save_logic(char silent);	//Performs "Save" logic, with the option to silence dialog prompts where possible
int eof_menu_file_save(void);				//Calls eof_menu_file_save_logic() with a silent value of 0
int eof_menu_file_quick_save(void);			//Calls eof_menu_file_save_logic() with a silent value of 1
int eof_menu_file_lyrics_import(void);
int eof_menu_file_feedback_import(void);	//Prompt for a .chart file and import it
int eof_menu_file_save_as(void);
int eof_menu_file_load_ogg(void);
int eof_menu_file_export_time_range(void);	//Exports a set time range of the project to a new project file
int eof_menu_file_midi_import(void);
int eof_menu_file_settings(void);
int eof_menu_file_preferences(void);
int eof_menu_file_display(void);
int eof_set_display_width(void);			//Overrides the current program window width with a user defined value
int eof_redraw_display(void);				//Rebuilds the program window with the current window size
int eof_menu_file_controllers(void);
int eof_menu_file_song_folder(void);
int eof_menu_file_link(char application);	//Prompts the user for an executable and song path, if application is 1, they are stored as a link to FoF, otherwise as a link to Phase Shift
int eof_menu_file_link_fof(void);			//Calls eof_menu_file_link() to link to FoF
int eof_menu_file_link_ps(void);			//Calls eof_menu_file_link() to link to Phase Shift
int eof_menu_file_exit(void);
int eof_menu_file_gh_import(void);	//Prompt for a Guitar Hero chart file and import it

char * eof_ogg_list(int index, int * size);
char * eof_guitar_list(int index, int * size);
char * eof_drum_list(int index, int * size);
char * eof_display_list(int index, int * size);

int eof_controller_settings_guitar(DIALOG * d);
int eof_controller_settings_drums(DIALOG * d);
int eof_guitar_controller_redefine(DIALOG * d);
int eof_drum_controller_redefine(DIALOG * d);

char *eof_lyric_detections_list_all(int index, int * size);
	//The dialog procedure that returns the strings to display in the list box

void eof_lyric_import_prompt(int *selectedformat, char **selectedtrack);
	//Requires storage for the selected format, track name to be passed back
	//The detection list created by DetectLyricFormat() is expected to be stored in the global lyricdetectionlist variable
	//This function displays the dialog for selecting one of of multiple lyric imports for a file
	// *selectedformat is set to 0 if a selection is not made

struct Lyric_Format *GetDetectionNumber(struct Lyric_Format *detectionlist, unsigned long number);
	//Returns detection #number from the detection linked list, or NULL on error

int eof_test_controller_conflict(EOF_CONTROLLER *ct, int start, int stop);
	//Tests for conflicts in the button[] array, from index #start to index #stop
	//eof_guitar.button[0] and button[1] are used for guitar strum, button[2] through button[6] are used for the fret buttons
	//eof_drums.button[0] is used for bass pedal, button[1] through button[4] are used for pads 1 through 4
	//If there is a button conflict or other error, nonzero is returned

void EnumeratedBChartInfo(struct FeedbackChart *chart);
	//Debug function to call an allegro_message with summary information about the passed chart

int eof_audio_to_ogg(char *file, char *directory);
	//Uses the specified file to create guitar.ogg in the specified directory
	//If the extension of the file is .mp3, it is converted to OGG format and a copy of the original mp3 file is stored at the specified directory,
	// if the extension of the file is .wav, it is converted to OGG format,
	// otherwise the specified file is simply copied to the specified directory
	//Returns zero on successful conversion or nonzero if conversion was canceled/unsuccessful

void eof_restore_oggs_helper(void);
	//Function to discard changes made to OGGs during editing

int eof_save_helper_checks(void);
	//Performs a series of checks on the active project and generates warnings, prompting the user to abort the save if a problem is found
	//Returns 1 if the user opts to abort the save, otherwise returns 0
int eof_save_helper(char *destfilename, char silent);
	//Performs logic that is common among "Save" and "Save as", including various validation checks
	//"Save as" operations should pass the destination file path through destfilename
	//  (but the calling function must not use eof_temp_filename[] to pass the target filename, as this function destroys that array's contents)
	//"Save" operations should pass NULL for destfilename
	//Returns zero on success, one on user cancellation, other values on error
	//If silent is nonzero, warnings and prompts are suppressed in order to perform a quick save operation

void eof_apply_display_settings(int mode);
	//Used by eof_menu_file_display() and eof_create_image_sequence() to set EOF's display settings

int eof_menu_file_new_supplement(char *directory, char *filename, char check);
	//Checks the specified directory for the presence of chart related files and prompts whether the user wants to overwrite existing files
	//If filename isn't NULL, checks for the presence of this relative file name instead of notes.eof when checking if the folder contains the project file already
	//If check & 1 is nonzero, guitar.ogg is checked for
	//If check & 2 is nonzero, original.mp3 is checked for
	//If check & 4 is nonzero, [filename].ogg, [filename].wav and [filename].eof are checked for
	//If any bits other than the lowest 3 bits of check are set, guitar.ogg, original.mp3, notes.eof, song.ini and notes.mid are checked for

int eof_menu_prompt_save_changes(void);
	//Stops playback and if the chart is modified, prompt whether to save changes (Save/Discard/Cancel)
	//If Save is selected, the chart is saved, otherwise modified chart audio is restored if necessary
	//The user's answer to the prompt is returned (1=save, 2=discard, 3=cancel), otherwise 0 is returned if the chart isn't loaded/modified

char * eof_gp_tracks_list(int index, int * size);
	//Dialog logic to display the imported pro guitar tracks present in the eof_parsed_gp_file global pointer
int eof_gp_import_common(const char *fn);
	//Imports the specified Guitar Pro file to the active track if the active track is a pro guitar/bass track
	//Returns nonzero on error
int eof_menu_file_gp_import(void);
	//Loads each track from a Guitar Pro file and prompts user which to import into the active project
int eof_command_line_gp_import(char *fn);
	//Creates a new project and imports the specified Guitar Pro track into the user-specified pro guitar/bass track
	//Returns zero on success

int eof_rs_import_common(char *fn);
	//Imports the specified Rocksmith XML file to the active track if the active track is a pro guitar/bass track
	//Returns nonzero on error
int eof_menu_file_rs_import(void);
	//Replaces the active pro guitar/bass track with the content of the selected arrangement in the user's selected Rocksmith XML file
	//If no project is open, a new project is created and the user is prompted to select a pro guitar/bass track to import to
	//Beat timings in the active project are replaced with the timings in the XML file
int eof_command_line_rs_import(char *fn);
	//Creates a new project and imports the specified Rocksmith arrangement into the user-specified pro guitar/bass track
	//Returns zero on success

int eof_menu_file_bf_import(void);
	//Replaces the active pro guitar/bass track with the content of the selected arrangement in the user's selected Bandfuse RIFF file
	//Beat timings in the active project are replaced with the timings in the XML file

int eof_toggle_display_zoom(void);
	//Alters EOF so that it renders a 480 height window and then stretches it to fit the program window, acting as a zoom feature

int eof_menu_file_sonic_visualiser_import(void);
	//Imports beat timings from a user specified Sonic Visualiser file

#endif
