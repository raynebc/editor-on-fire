#ifndef EOF_MENU_FILE_H
#define EOF_MENU_FILE_H

#include <stdio.h>
#include "../song.h"
#include "../control.h"
#include "../chart_import.h"

extern MENU eof_file_menu[];

extern DIALOG eof_settings_dialog[];
extern DIALOG eof_preferences_dialog[];
extern DIALOG eof_pg_preferences_dialog[];
extern DIALOG eof_display_dialog[];
extern DIALOG eof_custom_display_size_dialog[];
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
int eof_gp_import_guitar_track(int importvoice);
	//Used if the selected track is being imported as a guitar pro track (even if the GP track is a drum track)
	//importvoice specifies which voices are imported (1 = Lead, 2 = Bass, 3 = both)
int eof_gp_import_drum_track(int importvoice, int function);
	//Used if the selected track is being imported as a drum track
	//importvoice specifies which voices are imported (1 = Lead, 2 = Bass, 3 = both)
	//function specifies the destination track (1 = normal drums, 2 = Phase Shift drums, 3 = both)

void eof_prepare_file_menu(void);

int eof_menu_file_new_wizard(void);
	//Prompts for path to an audio file and then uses eof_new_chart() to continue the rest of the process of initializing a new chart
int eof_new_chart(char * filename);
	//Accepts an input audio file, prompts user for chart name/details/destination, encoding settings if applicable and initializes EOF variables
	//Returns nonzero on failure/cancellation

int eof_menu_file_load(void);
int eof_menu_file_save_logic(char silent);	//Performs "Save" logic, with the option to silence dialog prompts where possible
int eof_menu_file_save(void);				//Calls eof_menu_file_save_logic() with a silent value of 0
int eof_menu_file_quick_save(void);		//Calls eof_menu_file_save_logic() with a silent value of 1
int eof_menu_file_lyrics_import(void);
int eof_menu_file_feedback_import(void);	//Prompt for a .chart file and import it
int eof_menu_file_save_as(void);
int eof_menu_file_load_ogg(void);
int eof_menu_file_export_chart_range(void);		//Exports a set time range of the project to a new project file
int eof_menu_file_export_audio_range(void);	//Similar to eof_menu_file_export_chart_range(), but just exports the audio for the selected range to a user specified OGG file
int eof_menu_file_export_guitar_pro(void);		//Calls the third party RocksmithToTab program to create a GP5 file of the project's guitar/bass arrangements
int eof_menu_file_export_song_preview(void);
	//Allows the user to define a portion of the chart audio to export to preview.wav and preview.ogg in the project folder
int eof_menu_file_export_immerrock_track_diff(void);	//Exports the active pro guitar track difficulty to IMMERROCK format using eof_export_immerrock_diff()
int eof_menu_file_midi_import(void);
int eof_menu_file_drums_rock_import(void);
int eof_menu_file_settings(void);
int eof_menu_file_default_ini_settings(void);
int eof_menu_file_preferences(void);
int eof_menu_file_import_export_preferences(void);
int eof_menu_file_gp_preferences(void);
int eof_menu_file_display(void);
int eof_set_display_width(void);			//Overrides the current program window width with a user defined value
int eof_set_3d_hopo_scale_size(void);		//Sets the scale size for non-GHL 3D HOPO gems
int eof_set_3d_camera_angle(void);			//Sets the 3D camera angle
int eof_menu_file_controllers(void);
int eof_menu_file_song_folder(void);		//Prompts the user to define a default path for new projects.  Returns 0 if user cancels the dialog
int eof_menu_file_link(unsigned char application);
	//Prompts the user for an executable and song path
	//If application is 0, they are stored as a link to FoF
	//If application is 1, they are stored as a link to Phase Shift
	//Otherwise only an executable path is asked for, and it is stored as a link to RocksmithToTab
int eof_menu_file_link_fof(void);			//Calls eof_menu_file_link() to link to FoF
int eof_menu_file_link_ps(void);			//Calls eof_menu_file_link() to link to Phase Shift
int eof_menu_file_link_rs_to_tab(void);		//Calls eof_menu_file_link() to link to RocksmithToTab
int eof_menu_file_exit(void);
int eof_menu_file_gh_import(void);	//Prompt for a Guitar Hero chart file and import it
int eof_menu_file_ghl_import(void);	//Prompt for a Guitar Hero Live chart file and import it into the active track

char * eof_ogg_list(int index, int * size);
int eof_ogg_list_bitrate(int index);			//Returns the integer value bitrate (in kbps) that corresponds to the index from the OGG quality selection dialog, or 320 on error
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

int eof_test_controller_conflict(EOF_CONTROLLER *controller, int start, int stop);
	//Tests for conflicts in the button[] array, from index #start to index #stop
	//eof_guitar.button[0] and button[1] are used for guitar strum, button[2] through button[7] are used for the fret buttons
	//eof_drums.button[0] is used for bass pedal, button[1] through button[4] are used for pads 1 through 4
	//If there is a button conflict or other error, nonzero is returned

void EnumeratedBChartInfo(struct FeedbackChart *chart);
	//Debug function to call an allegro_message with summary information about the passed chart

int eof_audio_to_ogg(char *file, char *directory, char *dest_name, char function);
	//Uses the specified file to create an OGG file with a suitable name in the specified directory
	//If the extension of the file is .mp3, it is converted to OGG format and a copy of the original mp3 file is stored at the specified directory,
	// if the extension of the file is .wav, it is converted to OGG format,
	// otherwise the specified file is simply copied to the specified directory
	//If function is 0, the resulting file is named guitar.ogg, and "guitar.ogg" is written to *dest_name to confirm this to the calling function
	//If function is nonzero, the pre-extension part of the file name is allowed to be kept (instead of being forcibly renamed to guitar.ogg) if it has one of the following names:
	// song.mp3, song.wav, drums.mp3, drums.wav, rhythm.mp3, rhythm.wav, vocals.mp3 or vocals.wav
	// the selected destination OGG file name is written to *dest_name so the calling function is aware of the chosen file name
	//dest_name is expected to be an array at least 15 bytes in size (all of the supported OGG names are shorter than this)
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

void eof_rebuild_notes_window(void);
	//Destroys and rebuilds the window for the notes panel at the appropriate coordinates depending on whether the Info panel is being rendered
	//If eof_notes_panel is initialized, its window pointer is set to the newly allocated Window, otherwise it must be manually set later
int eof_display_notes_panel(void);					//Frees and reloads the notes.panel.txt file from memory appropriately, enables or disables the display of the notes panel
int eof_menu_file_notes_panel_notes(void);			//Reloads the Notes panel with the built-in notes.panel.txt file
int eof_menu_file_notes_panel_note_controls(void);	//Reloads the Notes panel with the built-in note_controls.panel.txt file
int eof_menu_file_notes_panel_information(void);	//Reloads the Notes panel with the built-in info.panel.txt file
int eof_menu_file_notes_panel_note_counts(void);	//Reloads the Notes panel with the built-in note_counts.panel.txt file
int eof_menu_file_notes_panel_immerrock(void);		//Reloads the Notes panel with the built-in immerrock.panel.txt file
int eof_menu_file_notes_panel_clone_hero(void);		//Reloads the Notes panel with the built-in clone_hero.panel.txt file
int eof_menu_file_notes_panel_user(void);
	//Reloads the Notes panel with the last text file the user manually browsed to
	//If no such browse has been done, or if that file no longer exists, eof_menu_file_notes_panel_browse() is called instead
int eof_menu_file_notes_panel_browse(void);
	//Prompts the user to browse for a panel.txt file to display in the Notes panel
	//Stores the selected path into eof_last_browsed_notes_panel_path() and reloads the Notes panel appropriately

int eof_3d_preview_toggle_full_height(void);
	//Changes whether the 3D preview will take one panel's or two panels' worth of height

int eof_menu_file_array_txt_import(void);
	//Prompts the user to browse for an array.txt file exported by Queen Bee, to import Guitar Hero chart data
int eof_menu_file_multiple_array_txt_import(void);
	//Prompts the user to browse to an array.txt file exported by Queen Bee, then imports each .txt file in that file's folder with eof_import_array_txt()

int eof_menu_file_gh3_section_import(void);
	//Prompts the user to browser for a Guitar Hero chart file, to import its section events into the active project

#endif
