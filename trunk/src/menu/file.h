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
extern struct Lyric_Format *lyricdetectionlist;	//Dialog windows cannot be passed local variables, requiring the use of this global variable

extern struct eof_guitar_pro_struct *eof_parsed_gp_file;	//Dialog windows cannot be passed local variables, requiring the use of this global variable
extern DIALOG eof_gp_import_dialog[];	//The dialog used to display the tracks imported from a Guitar Pro file, allowing one to overwrite the active pro guitar track
int eof_gp_import_track(DIALOG * d);	//The function performed when the user selects and imports a track from the above dialog

void eof_prepare_file_menu(void);

int eof_menu_file_new_wizard(void);
	//Prompts for path to an audio file and then uses eof_new_chart() to continue the rest of the process of initializing a new chart
int eof_new_chart(char * filename);
	//Accepts an input audio file, prompts user for chart name/details/destination, encoding settings if applicable and initializes EOF variables
	//Returns nonzero on failure/cancellation

int eof_menu_file_load(void);
int eof_menu_file_save(void);
int eof_menu_file_quick_save(void);
int eof_menu_file_lyrics_import(void);
int eof_menu_file_feedback_import(void);	//Prompt for a .chart file and import it
int eof_menu_file_save_as(void);
int eof_menu_file_load_ogg(void);
int eof_menu_file_midi_import(void);
int eof_menu_file_settings(void);
int eof_menu_file_preferences(void);
int eof_menu_file_display(void);
int eof_menu_file_controllers(void);
int eof_menu_file_song_folder(void);
int eof_menu_file_link(char application);	//Prompts the user for an executable and song path, if application is 1, they are stored as a link to FoF, otherwise as a link to Phase Shift
int eof_menu_file_link_fof(void);	//Calls eof_menu_file_link() to link to FoF
int eof_menu_file_link_ps(void);	//Calls eof_menu_file_link() to link to Phase Shift
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
	//*selectedformat is set to 0 if a selection is not made

struct Lyric_Format *GetDetectionNumber(struct Lyric_Format *detectionlist, unsigned long number);
	//Returns detection #number from the detection linked list

int eof_test_controller_conflict(EOF_CONTROLLER *ct,int start,int stop);
	//Tests for conflicts in the button[] array, from index #start to index #stop
	//eof_guitar.button[0] and button[1] are used for guitar strum, button[2] through button[6] are used for the fret buttons
	//eof_drums.button[0] is used for bass pedal, button[1] through button[4] are used for pads 1 through 4
	//If there is a button conflict, nonzero is returned

void EnumeratedBChartInfo(struct FeedbackChart *chart);
	//Call an allegro_message with summary information about the passed chart

int eof_mp3_to_ogg(char *file,char *directory);
	//Uses the specified file to create guitar.ogg in the specified directory
	//If the extension of the file is .mp3, it is converted to OGG format, otherwise the specified file is copied
	//Returns zero on successful conversion or nonzero if conversion was canceled/unsuccessful

void eof_restore_oggs_helper(void);
	//Function to discard changes made to OGGs during editing

int eof_save_helper(char *destfilename);
	//Performs logic that is common among "Save" and "Save as"
	//"Save as" operations should pass the destination file path through destfilename
	//"Save" operations should pass NULL for destfilename

void eof_apply_display_settings(int mode);
	//Used by eof_menu_file_display() and eof_create_image_sequence() to set EOF's display settings

int eof_menu_file_new_supplement(char *directory, char check);
	//Checks the specified directory for the presence of chart related files and prompts whether the user wants to overwrite existing files
	//If check equals 1, guitar.ogg is checked for
	//If check equals 2, original.mp3 is checked for
	//If check is neither 1 nor 2, guitar.ogg, original.mp3, notes.eof, song.ini and notes.mid are checked for

int eof_menu_prompt_save_changes(void);
	//Stops playback and if the chart is modified, prompt whether to save changes (Save/Discard/Cancel)
	//If Save is selected, the chart is saved, otherwise modified chart audio is restored if necessary
	//The user's answer to the prompt is returned (1=save, 2=discard, 3=cancel), otherwise 0 is returned if the chart isn't loaded/modified
int eof_menu_file_gp_import(void);
	//Loads each track from a Guitar Pro file and prompts user which to import into the active project
char * eof_gp_tracks_list(int index, int * size);
	//Dialog logic to display the imported pro guitar tracks present in the eof_parsed_gp_file global pointer

#endif
