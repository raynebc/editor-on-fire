/* HOPO note: if there is less than 170 mseconds between notes, they are created as HOPO */

#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif
#ifdef ALLEGRO_LEGACY
	#include <a5alleg.h>
#endif
#include <locale.h>	//For setlocale()
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include "alogg/include/alogg.h"
#include "agup/agup.h"
#include "modules/wfsel.h"
#include "modules/ocd3d.h"
#include "modules/gametime.h"
#include "modules/g-idle.h"
#include "dialog/main.h"
#include "menu/beat.h"
#include "menu/main.h"
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/track.h"
#include "menu/help.h"
#include "menu/note.h"	//For pitch macros
#include "main.h"
#include "utility.h"
#include "player.h"
#include "bf_import.h"
#include "config.h"
#include "midi.h"
#include "midi_import.h"
#include "chart_import.h"
#include "gh_import.h"
#include "window.h"
#include "note.h"
#include "notes.h"
#include "beat.h"
#include "event.h"
#include "editor.h"
#include "dialog.h"
#include "undo.h"
#include "mix.h"
#include "control.h"
#include "waveform.h"
#include "spectrogram.h"
#include "silence.h"
#include "tuning.h"
#include "ini_import.h"
#include "midi_data_import.h"	//For eof_track_overridden_by_stored_MIDI_track()
#include "rs.h"	//for eof_pro_guitar_track_find_effective_fret_hand_position()
#include "song.h"
#include "utility.h"	//For eof_ucode_table[] declaration
#include "pathing.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char        eof_note_type_name_fof[EOF_MAX_DIFFICULTIES][10] = {" Supaeasy", " Easy", " Medium", " Amazing", " BRE"};
char        eof_note_type_name_rb[EOF_MAX_DIFFICULTIES][10] = {" Easy", " Medium", " Hard", " Expert", " BRE"};
char        (*eof_note_type_name)[10] = eof_note_type_name_rb;	//By default, use Rock Band difficulty names
char        eof_vocal_tab_name[EOF_MAX_DIFFICULTIES][32] = {" Lyrics", " ", " ", " ", " "};
char        eof_dance_tab_name[EOF_MAX_DIFFICULTIES][32] = {" Beginner", " Easy", " Medium", " Hard", " Challenge"};
char        eof_track_diff_populated_status[256] = {0};
char        eof_track_diff_populated_tech_note_status[256] = {0};
char        eof_track_diff_highlighted_status[256] = {0};
char        eof_track_diff_highlighted_tech_note_status[256] = {0};
char      * eof_snap_name[11] = {"Off", "1/4", "1/8", "1/12", "1/16", "1/24", "1/32", "1/48", "1/64", "1/96", "Custom"};
char      * eof_input_name[EOF_INPUT_NAME_NUM + 1] = {"Classic", "Piano Roll", "Hold", "RexMundi", "Guitar Tap", "Guitar Strum", "Feedback"};

NCDFS_FILTER_LIST * eof_filter_music_files = NULL;
NCDFS_FILTER_LIST * eof_filter_ogg_files = NULL;
NCDFS_FILTER_LIST * eof_filter_midi_files = NULL;
NCDFS_FILTER_LIST * eof_filter_eof_files = NULL;
NCDFS_FILTER_LIST * eof_filter_exe_files = NULL;
NCDFS_FILTER_LIST * eof_filter_lyrics_files = NULL;
NCDFS_FILTER_LIST * eof_filter_dB_files = NULL;
NCDFS_FILTER_LIST * eof_filter_gh_files = NULL;
NCDFS_FILTER_LIST * eof_filter_ghl_files = NULL;
NCDFS_FILTER_LIST * eof_filter_gp_files = NULL;
NCDFS_FILTER_LIST * eof_filter_gp_lyric_text_files = NULL;
NCDFS_FILTER_LIST * eof_filter_rs_files = NULL;
NCDFS_FILTER_LIST * eof_filter_sonic_visualiser_files = NULL;
NCDFS_FILTER_LIST * eof_filter_bf_files = NULL;
NCDFS_FILTER_LIST * eof_filter_note_panel_files = NULL;
NCDFS_FILTER_LIST * eof_filter_array_txt_files = NULL;

PALETTE     eof_palette;
BITMAP *    eof_image[EOF_MAX_IMAGES] = {NULL};
BITMAP *    eof_stretch_bitmap = NULL;
BITMAP *    eof_hopo_stretch_bitmap = NULL;
BITMAP *    eof_background = NULL;
FONT *      eof_allegro_font = NULL;
FONT *      eof_font = NULL;
FONT *      eof_mono_font = NULL;
FONT *      eof_symbol_font = NULL;
int         eof_global_volume = 255;

EOF_WINDOW * eof_window_editor = NULL;
EOF_WINDOW * eof_window_editor2 = NULL;
EOF_WINDOW * eof_window_info = NULL;
EOF_WINDOW * eof_window_note_lower_left = NULL;
EOF_WINDOW * eof_window_note_upper_left = NULL;
EOF_WINDOW * eof_window_3d = NULL;
EOF_WINDOW * eof_window_notes = NULL;

/* configuration */
char        eof_fof_executable_path[1024] = {0};
char        eof_fof_executable_name[1024] = {0};
char        eof_fof_songs_path[1024] = {0};
char        eof_ps_executable_path[1024] = {0};
char        eof_ps_executable_name[1024] = {0};
char        eof_ps_songs_path[1024] = {0};
char        eof_rs_to_tab_executable_path[1024] = {0};
char        eof_last_frettist[256] = {0};
char        eof_temp_filename[1024] = {0};	//A string for temporarily storing file paths
char        eof_soft_cursor = 0;
char        eof_desktop = 1;
unsigned long eof_av_delay = 200;
int         eof_midi_tone_delay = 0;
int         eof_midi_synth_instrument_guitar = 28;	//Electric Guitar (clean) MIDI instrument
int         eof_midi_synth_instrument_guitar_muted = 29;	//An alternate MIDI instrument to play for palm muted guitar notes
int         eof_midi_synth_instrument_guitar_harm = 32;		//An alternate MIDI instrument to play for harmonic guitar notes
int         eof_midi_synth_instrument_bass = 34;	//Electric bass (fingered) MIDI instrument
int         eof_scroll_seek_percent = 5;	//The amount of one screen that ALT+scroll will seek
int         eof_buffer_size = 4096;
int         eof_audio_fine_tune = 0;
int         eof_inverted_notes = 0;
int         eof_lefty_mode = 0;
int         eof_note_auto_adjust = 1;
int         eof_use_ts = 0;	//By default, do not import/export TS events in MIDI/Feedback files
int			eof_hide_drum_tails = 0;
int         eof_hide_note_names = 0;
int         eof_disable_sound_processing = 0;
int         eof_disable_3d_rendering = 0;
int         eof_disable_2d_rendering = 0;
int         eof_disable_info_panel = 0;
int         eof_enable_notes_panel = 0;
int         eof_full_height_3d_preview = 0;
EOF_TEXT_PANEL *eof_notes_panel = NULL;			//The text panel instance defining the Notes panel
EOF_TEXT_PANEL *eof_info_panel = NULL;			//The text panel instance defining the Information panel
int         eof_paste_erase_overlap = 0;
int         eof_write_fof_files = 1;			//If nonzero, files are written during save that are used for Frets on Fire, Guitar Hero and Phase Shift authoring
int         eof_write_gh_files = 0;				//If nonzero, extra GH3 and GHWT MIDIs are written, as well as the GHWT drum animations file
int         eof_write_rb_files = 0;				//If nonzero, extra files are written during save that are used for authoring Rock Band customs
int         eof_write_music_midi = 0;			//If nonzero, extra MIDI files are written during save that contains normal MIDI pitches and can be used for other MIDI based games like Songs2See and Synthesia and also for Fretlight
int         eof_write_rs_files = 0;				//If nonzero, extra files are written during save that are used for authoring Rocksmith customs
int         eof_write_rs2_files = 0;			//If nonzero, extra files are written during save that are used for authoring Rocksmith 2014 customs
int         eof_abridged_rs2_export = 1;		//If nonzero, default XML attributes are omitted from RS2 export
int         eof_abridged_rs2_export_warning_suppressed = 0;	//Set to nonzero if the user has suppressed the warning that abridged RS2 files require a version of the toolkit >= 2.8.2.0
int         eof_rs2_export_extended_ascii_lyrics = 0;	//If nonzero, a larger set of text characters are allowed to export to RS2 lyrics
int         eof_disable_ini_difference_warnings = 0;	//If nonzero, warnings will be given during project load if there are tags in song.ini that don't match the contents of the EOF project
int         eof_write_bf_files = 0;				//If nonzero, an extra XML file is written during save that is used for authoring Bandfuse customs
int         eof_write_lrc_files = 0;			//If nonzero, lyrics in LRC, ELRC and QRC formats are each written during save
int         eof_force_pro_drum_midi_notation = 1;	//If nonzero, even drum tracks without cymbals will write with RB3 style tom markers for all yellow, blue and green drum notes (instead of no tom markers as would otherwise be done), and "pro_drums = true" is written to song.ini
int         eof_add_new_notes_to_selection = 0;	//If nonzero, newly added gems cause notes to be added to the selection instead of the selection being cleared first
int         eof_drum_modifiers_affect_all_difficulties = 1;	//If nonzero, a drum modifier (ie. open/pedal hi hat or rim shot apply to any notes at the same position in non active difficulties)
int         eof_fb_seek_controls = 0;			//If nonzero, the page up/dn keys have their seek directions reversed, and up/down seek forward/backward
int         eof_new_note_length_1ms = 0;		//If nonzero, newly created notes are initialized to a length of 1ms instead of the regular grid snap based logic
int         eof_new_note_forced_strum = 0;		//If nonzero, newly created notes are given forced HOPO off status
int         eof_use_fof_difficulty_naming = 0;	//If nonzero, the difficulties are labeled Supaeasy, Easy, Medium and Amazing instead of the Rock Band naming (Easy, Medium, Hard, Expert)
int         eof_gp_import_preference_1 = 0;		//If nonzero during GP import, beat texts with qualifying strings are imported as RS sections, section markers as RS phrases.
int         eof_gp_import_truncate_short_notes = 1;	//If nonzero during GP import, single notes shorter than one quarter note are set to the minimum length of 1ms
int         eof_gp_import_truncate_short_chords = 1;	//If nonzero during GP import, chords shorter than one quarter note are set to the minimum length of 1ms
int         eof_gp_import_replaces_track = 1;	//If nonzero during GP import, the imported track replaces the active track instead of just the active track difficulty
int         eof_gp_import_nat_harmonics_only = 0;	//If nonzero during GP import, only natural harmonics are imported as notes with harmonic status
int         eof_min_note_length = 0;			//Specifies the user-configured minimum length for legacy guitar/bass notes (for making Guitar Hero customs, is set to 0 if undefined)
int         eof_enforce_chord_density = 0;		//Specifies whether repeated chords that are separated by a rest automatically have crazy status applied to force RS export as low density chords
unsigned    eof_chord_density_threshold = 10000;	//Specifies the maximum distance between repeated chords that allows them to be marked as high density (repeat lines)
unsigned    eof_min_note_distance = 3;			//Specifies the user-configured minimum distance between notes (to avoid problems with timing conversion leading to precision loss that can cause notes to combine/drop)
unsigned    eof_min_note_distance_intervals = 0;	//If 0, the minimum distance between notes is observed to be in ms.  If 1, it's observed to be /# measure.  If 2, it's observed to be 1/# beat.
int         eof_render_bass_drum_in_lane = 0;	//If nonzero, the 3D rendering will draw bass drum gems in a lane instead of as a bar spanning all lanes
int         eof_click_changes_dialog_focus = 1;	//If nonzero, eof_verified_proc will not change dialog focus on mouse-over, it requires an explicit mouse click
int         eof_stop_playback_leave_focus = 1;	//If nonzero, EOF stops playback when it is not in the foreground
int         eof_inverted_chords_slash = 0;
int         eof_render_3d_rs_chords = 0;		//If nonzero, the 3D rendering will draw a rectangle to represent chords that will export to XML as repeats (Rocksmith), and 3D chord tails will not be rendered
int         eof_render_2d_rs_piano_roll = 0;	//If nonzero, the piano roll will display with some visual differences (thicker, colored measure markers, color-coded string lines)
int         eof_imports_recall_last_path = 0;	//If nonzero, various import dialogs will initialize the dialog to the path containing the last chosen import, instead of initializing to the project's folder
int         eof_rewind_at_end = 1;				//If nonzero, chart rewinds when the end of chart is reached during playback
int         eof_disable_rs_wav = 0;				//If nonzero, a WAV file for use in Rocksmith will not be maintained during save even if Rocksmith file export is enabled
int         eof_display_seek_pos_in_seconds = 0;	//If nonzero, the seek position in the piano roll and information panel is given in seconds instead of minutes:seconds
int         eof_note_tails_clickable = 0;		//If nonzero, when the mouse hovers over a note/lyric tail instead of just the note/lyric head, that note/lyric becomes the hover note
int         eof_auto_complete_fingering = 1;	//If nonzero, offer to apply specified chord fingering to matching notes in the track
int         eof_dont_auto_name_double_stops = 0;	//If nonzero, the chord detection logic will not name chords that have only two pitches (unique or otherwise)
int         eof_section_auto_adjust = 1;		//If nonzero, section and FHP positions are updated when all their contained notes are simultaneously moved
int         eof_technote_auto_adjust = 1;		//If nonzero, tech note positions are updated when all their applicable normal notes are simultaneously moved
int         eof_top_of_2d_pane_cycle_count_2 = 0;	//If nonzero, the SHIFT+F11 shortcut cycles between just hand positions and RS sections+phrases instead of also including chord names and tone changes
int         eof_rbn_export_slider_hopo = 0;		//If nonzero, notes in slider phrases will be exported to RBN MIDI as forced HOPO notes
int         eof_imports_drop_mid_beat_tempos = 0;	//If nonzero, any beats inserted due to mid beat tempo changes during Feedback and MIDI import are deleted after the import
int         eof_render_mid_beat_tempos_blue = 0;	//If nonzero, any beats that were inserted during Feedback/MIDI import due to mid beat tempo changes retain the EOF_BEAT_FLAG_MIDBEAT flag and have some special 2D rendering logic
int         eof_disable_ini_export = 0;			//If nonzero, song.ini is not written during save even if a MIDI file is exported
int         eof_gh_import_sustain_threshold_prompt = 0;	//If nonzero, GH import will prompt whether to apply a sustain threshold determined by half of the first beat's length (as per GH3 rules)
int         eof_db_import_suppress_5nc_conversion = 0;	//If nonzero, five note chords are not converted to open notes during Feedback import
int         eof_warn_missing_bass_fhps = 1;		//If nonzero, the Rocksmith export checks will complain about FHPs not being defined for bass arrangements
int         eof_4_fret_range = 1;				//Defines the lowest fret number at which the fret hand has a range of 4 frets, for fret hand position generation (the default value of 1 indicates this range for the entire fretboard)
int         eof_5_fret_range = 0;				//Defines the lowest fret number at which the fret hand has a range of 5 frets, for fret hand position generation (the default value of 0 indicates this range is undefined)
int         eof_6_fret_range = 0;				//Defines the lowest fret number at which the fret hand has a range of 6 frets, for fret hand position generation (the default value of 0 indicates this range is undefined)
int         eof_fingering_checks_include_mutes = 0;	//If nonzero, various functions that validate/complete chord fingerings will not dismiss strings with string mute status
int         eof_ghl_conversion_swaps_bw_gems = 0;	//If nonzero, toggling GHL mode on/off or copying between GHL and non GHL tracks will remap lanes 1-3 as the black GHL gems instead the white gems as per the default behavior
int         eof_3d_hopo_scale_size = 75;		//The percentage of full size to which the 3D HOPO gem images are scaled
int         eof_disable_backups = 0;			//If nonzero, .eof.bak files (created during undo and save operations) and .backup files (created during MIDI/GH/Feedback import) are not created
int         eof_enable_open_strums_by_default = 0;	//If nonzero, legacy tracks in new projects will be initialized to have open strums enabled automatically
int         eof_prefer_midi_friendly_grid_snapping = 1;	//If nonzero, the "Highlight non grid snapped notes" and "Repair grid snap" functions will only recognize grid snaps that quantize well to MIDI using the defined time division
int         eof_dont_auto_edit_new_lyrics = 0;	//If nonzero, the edit lyric dialog is not automatically launched when a new lyric is placed
double      eof_lyric_gap_multiplier = 0.0;		//If greater than zero, edited lyrics will have a minimum distance applied to them that is equal to this variable multiplied by the current grid snap
char        eof_lyric_gap_multiplier_string[20] = {0};	//The string representation of the above value, since it is to be stored in string format in the config file
int         eof_song_folder_prompt = 0;			//In the Mac version, tracks whether the user opted to define the song folder or not
int         eof_smooth_pos = 1;
int         eof_input_mode = EOF_INPUT_PIANO_ROLL;
int         eof_windowed = 1;
int         eof_anchor_all_beats = 1;
int         eof_disable_windows = 0;
int         eof_disable_vsync = 0;
int         eof_phase_cancellation = 0;		//If nonzero, the stereo mixing callback function will subtract the right channel amplitude from the left (ie. to get the effect of reducing center-panned vocals)
int         eof_center_isolation = 0;		//If nonzero, the stereo mixing callback function will add the right channel amplitude to the left (ie. to get the effect of reducing non center-panned vocals)
int         eof_playback_speed = 1000;
char        eof_playback_time_stretch = 1;
int         eof_ogg_setting = 1;
char      * eof_ogg_quality[7] = {"1.0", "2.0", "4.0", "5.0", "6.0", "8.0", "10.0"};
unsigned long eof_frame = 0;
int         eof_cpu_saver = 0;
char        eof_has_focus = 1;				//Indicates whether EOF is in foreground focus (if set to 2, EOF just switched back into focus)
char        eof_supports_mp3 = 0;
char        eof_supports_oggcat = 0;		//Set to nonzero if EOF determines oggCat is usable
int         eof_new_idle_system = 0;
char        eof_just_played = 0;
char        eof_mark_drums_as_cymbal = 0;		//Allows the user to specify whether Y/B/G drum notes will be placed with cymbal notation by default
char        eof_mark_drums_as_double_bass = 0;	//Allows the user to specify whether expert bass drum notes will be placed with expert+ notation by default
unsigned long eof_mark_drums_as_hi_hat = 0;		//Allows the user to specify whether Y drum notes in the PS drum track will be placed with one of the hi hat statuses by default (this variable holds the note flag of the desired status)
unsigned long eof_pro_guitar_fret_bitmask = 63;	//Defines which lanes are affected by these shortcuts:  CTRL+# to set fret numbers, CTRL+(plus/minus) increment/decrement fret values, CTRL+G to toggle ghost status
char		eof_legacy_view = 0;				//Specifies whether pro guitar notes will render as legacy notes
unsigned char eof_2d_render_top_option = 5;		//Specifies what item displays at the top of the 2D panel (defaults to note names)
char        eof_render_grid_lines = 0;			//Specifies whether grid snap positions will render in the editor window
char        eof_show_ch_sp_durations = 0;		//Specifies whether durations for defined star power deployments will render in the editor window

int         eof_undo_toggle = 0;
int         eof_redo_toggle = 0;
int         eof_window_title_dirty = 0;	//This is set to true when an undo state is made and is cleared when the window title is recreated with eof_fix_window_title()
int         eof_change_count = 0;	//Counts the number of undo states created since the last save operation
int         eof_zoom = 10;			//The width of one pixel in the editor window in ms (smaller = higher zoom)
int         eof_zoom_3d = 5;
int         eof_3d_min_depth = -100;
int         eof_3d_max_depth = 600;

char        eof_changes = 0;		//Tracks whether the chart has been modified since the last save operation
ALOGG_OGG * eof_music_track = NULL;
void      * eof_music_data = NULL;	//A memory buffer of the current chart audio's OGG file
int         eof_silence_loaded = 0;
int         eof_music_data_size = 0;
unsigned long eof_chart_length = 0;		//Stores the position of the last note/lyric/text event/bookmark or the end of the chart audio, whichever is longer
unsigned long eof_music_length = 0;
int         eof_music_pos;
int         eof_music_pos2 = -1;		//The position to display in the secondary piano roll (-1 means it will initialize to the current track when it is enabled)
int         eof_sync_piano_rolls = 1;	//If nonzero, the secondary piano roll will render with the current chart position instead of its own
unsigned long eof_music_actual_pos;
unsigned long eof_music_rewind_pos = 0;
int         eof_music_catalog_pos;
unsigned long eof_music_end_pos;
int         eof_music_paused = 1;
char        eof_music_catalog_playback = 0;
char        eof_play_selection = 0;
EOF_SONG    * eof_song = NULL;
EOF_NOTE    eof_pen_note;
EOF_LYRIC   eof_pen_lyric;
char        eof_temp_path[20] = {0};					//The relative path to the temp folder used by EOF
char        eof_temp_path_s[20] = {0};					//The relative path to the temp folder used by EOF, with a path separator appended
char        eof_filename[1024] = {0};					//The full path of the EOF file that is loaded
char        eof_song_path[1024] = {0};					//The path to active project's parent folder
char        eof_songs_path[1024] = {0};					//The path to the user's song folder
char        eof_last_ogg_path[1024] = {0};				//The path to the folder containing the last loaded OGG file
char        eof_last_eof_path[1024] = {0};				//The path to the folder containing the last loaded project
char        eof_last_midi_path[1024] = {0};				//The path to the folder containing the last imported MIDI
char        eof_last_db_path[1024] = {0};				//The path to the folder containing the last imported Feedback chart file
char        eof_last_gh_path[1024] = {0};				//The path to the folder containing the last imported Guitar Hero chart file
char        eof_last_ghl_path[1024] = {0};				//The path to the folder containing the last imported Guitar Hero Live chart file
char        eof_last_lyric_path[1024] = {0};			//The path to the folder containing the last imported lyric file
char        eof_last_gp_path[1024] = {0};				//The path to the folder containing the last imported Guitar Pro file
char        eof_last_rs_path[1024] = {0};				//The path to the folder containing the last imported Rocksmith XML file
char        eof_last_sonic_visualiser_path[1024] = {0};	//The path to the folder containing the last imported Sonic Visualiser file
char        eof_last_bf_path[1024] = {0};				//The path to the folder containing the last imported Bandfuse file
char        eof_last_browsed_notes_panel_path[1024] = {0};		//The full path of the last Notes panel text file that the user manually browsed to
char        eof_current_notes_panel_path[1024] = {0};	//The path to the current text file to display in the Notes panel (any non full path is expected to be relative to EOF's program folder)
char        eof_loaded_song_name[1024] = {0};			//The file name (minus the folder path) of the active project
char        eof_loaded_ogg_name[1024] = {0};			//The full path of the loaded OGG file
char        eof_window_title[4096] = {0};
int         eof_quit = 0;
unsigned long eof_main_loop_ctr = 0;
double      eof_main_loop_fps = 0.0;
unsigned    eof_note_type = EOF_NOTE_AMAZING;		//The active difficulty
unsigned    eof_note_type_i = EOF_NOTE_AMAZING;		//The active difficulty for instrumental tracks
unsigned    eof_note_type_v = EOF_NOTE_SUPAEASY;	//The active difficulty for the vocal track
unsigned    eof_selected_track = EOF_TRACK_GUITAR;
int         eof_display_second_piano_roll = 0;		//If nonzero, a second piano roll will render in place of the info panel and 3D preview
unsigned    eof_note_type2 = 256;	//The difficulty to display in the secondary piano roll (having a value greater than the highest supported difficulty level of 255 means it will initialize to the current difficulty when it is enabled)
unsigned    eof_selected_track2 = 0;	//The track to display in the secondary piano roll (0 means it will initialize to the current track when it is enabled)
int         eof_vocals_selected = 0;	//Is set to nonzero if the active track is a vocal track
unsigned    eof_vocals_tab = 0;
int         eof_vocals_offset = 60; // Start at "middle C"
int         eof_song_loaded = 0;	//The boolean condition that a chart and its audio are successfully loaded
int         eof_last_note = 0;
int         eof_last_midi_offset = 0;
PACKFILE *  eof_recovery = NULL;
unsigned long eof_seek_selection_start = 0, eof_seek_selection_end = 0;	//Used to track the keyboard driven note selection system in Feedback input mode.  If both variables are of equal value, no seek selection is in effect
int         eof_shift_released = 1;	//Tracks the press/release of the SHIFT keys for the Feedback input mode seek selection system
int         eof_shift_used = 0;	//Tracks whether the SHIFT key was used for a keyboard shortcut while SHIFT was held
int         eof_tab_released = 1;	//Tracks the press/release of the tab key to prevent the Allegro bug of its status getting stuck from repeatedly triggering keyboard functions
int         eof_emergency_stop = 0;	//Set to nonzero by eof_switch_out_callback() so that playback can be stopped OUTSIDE of the callback, in EOF's main loop so that a crash with time stretched playback can be avoided
int         ch_sp_path_worker = 0;	//Set to nonzero if EOF is launched with the -ch_sp_path_worker parameter to have it run as a worker process for star power pathing evaluation
int         ch_sp_path_worker_logging = 0;	//Set to nonzero if EOF is launched with the -ch_sp_path_worker_logging parameter, which will allow logging to be performed during solving
unsigned long ch_sp_path_worker_number = 0;	//Set to the number in the job filename, for creating worker-specific logging

char eof_default_ini_setting[EOF_MAX_INI_SETTINGS][EOF_INI_LENGTH] = {{0}};	//Stores the entries of the [default_ini_settings] section in the config file
unsigned short eof_default_ini_settings = 0;	//The number of such entries in memory

int         eof_display_catalog = 0;
int         eof_catalog_full_width = 0;
unsigned long eof_selected_catalog_entry = 0;

/* mouse control data */
int         eof_selected_control = -1;
int         eof_cselected_control = -1;
unsigned long eof_selected_beat = 0;
long        eof_selected_measure = 0;
int         eof_beat_in_measure = 0;
int         eof_beats_in_measure = 1;
int         eof_pegged_note = -1;
int         eof_hover_note = -1;
int         eof_seek_hover_note = -1;
int         eof_hover_note_2 = -1;
unsigned long eof_hover_beat = ULONG_MAX;
unsigned long eof_hover_beat_2 = ULONG_MAX;
int         eof_hover_piece = -1;
int         eof_hover_key = -1;
int         eof_hover_lyric = -1;
int         eof_last_tone = -1;
int         eof_mouse_x;				//Tracks the mouse's x coordinate when the main menu is opened, so it can be restored after the mouse moves
int         eof_mouse_y;				//Tracks the mouse's y coordinate when the main menu is opened, so it can be restored after the mouse moves
int         eof_mouse_z;
int         eof_mickey_z;
int         eof_mickeys_x;
int         eof_mickeys_y;
int         eof_lclick_released = 1;	//Tracks the handling of the left mouse button in general
int         eof_blclick_released = 1;	//Tracks the handling of the left mouse button when used on beat markers
int         eof_rclick_released = 1;	//Tracks the handling of the right mouse button in general
int         eof_mclick_released = 1;	//Tracks the handling of the middle mouse button in general
int         eof_click_x;
int         eof_click_y;
int         eof_peg_x;
int         eof_last_pen_pos = 0;
int         eof_cursor_visible = 1;
int         eof_pen_visible = 1;
int         eof_hover_type = -1;
int         eof_mouse_drug = 0;
int         eof_paste_at_mouse = 0;
int         eof_notes_moved = 0;

/* grid snap data */
char          eof_snap_mode = EOF_SNAP_OFF;
char          eof_last_snap_mode = EOF_SNAP_OFF;
int           eof_snap_interval = 1;
char          eof_custom_snap_measure = 0;	//Boolean: The user set a custom grid snap interval defined in measures instead of beats

char          eof_hopo_view = EOF_HOPO_RF;

/* controller info */
EOF_CONTROLLER eof_guitar;
EOF_CONTROLLER eof_drums;

/* colors */
int eof_color_black;
int eof_color_white;
int eof_color_gray;
int eof_color_gray2;
int eof_color_gray3;
int eof_color_light_gray;
int eof_color_red;
int eof_color_light_red;
int eof_color_green;
int eof_color_dark_green;
int eof_color_blue;
int eof_color_dark_blue;
int eof_color_light_blue;
int eof_color_lighter_blue;
int eof_color_turquoise;
int eof_color_yellow;
int eof_color_purple;
int eof_color_dark_purple;
int eof_color_orange;
int eof_color_silver;
int eof_color_dark_silver;
int eof_color_cyan;
int eof_color_dark_cyan;
int eof_info_color;

int eof_color_set = EOF_COLORS_DEFAULT;
eof_color eof_colors[6];	//Contain the color definitions for each lane
eof_color eof_color_green_struct, eof_color_red_struct, eof_color_yellow_struct, eof_color_blue_struct, eof_color_orange_struct, eof_color_purple_struct, eof_color_black_struct, eof_color_ghl_black_struct, eof_color_ghl_white_struct;
eof_color eof_lane_1_struct, eof_lane_2_struct, eof_lane_3_struct, eof_lane_4_struct, eof_lane_5_struct, eof_lane_6_struct;
	//Color data

EOF_SCREEN_LAYOUT eof_screen_layout;
BITMAP * eof_screen = NULL;
BITMAP * eof_screen2 = NULL;						//Used to render the screen when x2 zoom is used, so the menu can render on top of it at normal scaling
unsigned long eof_screen_width, eof_screen_height;	//Used to track the EOF window size, for when the 3D projection is altered
unsigned long eof_screen_width_default;				//Stores the default width based on the current window height
int eof_vanish_x = 0, eof_vanish_y = 0;				//Used to allow the user to control the vanishing point for the 3D preview
char eof_full_screen_3d = 0;						//If nonzero, directs the render logic to scale the 3D window to fit the entire program window
char eof_3d_fretboard_coordinates_cached = 0;		//Tracks the validity of the 3D window rendering's cache of the fretboard's 2D coordinates
char eof_screen_zoom = 0;							//Tracks whether EOF will perform a x2 zoom rendering

EOF_SELECTION_DATA eof_selection;

/* lyric preview data */
unsigned long eof_preview_line[2] = {0};
unsigned long eof_preview_line_lyric[2] = {0};
unsigned long eof_preview_line_end_lyric[2] = {0};

/* Windows-only data */
#ifdef ALLEGRO_WINDOWS
int eof_windows_argc = 0;
wchar_t ** eof_windows_internal_argv;
char ** eof_windows_argv;
#endif

/* fret/tuning editing arrays */
char eof_string_lane_1[5] = {0};	//Use a fifth byte to guarantee proper truncation
char eof_string_lane_2[5] = {0};
char eof_string_lane_3[5] = {0};
char eof_string_lane_4[5] = {0};
char eof_string_lane_5[5] = {0};
char eof_string_lane_6[5] = {0};
char *eof_fret_strings[6] = {eof_string_lane_1, eof_string_lane_2, eof_string_lane_3, eof_string_lane_4, eof_string_lane_5, eof_string_lane_6};
char eof_string_lane_1_number[] = "String 6:";
char eof_string_lane_2_number[] = "String 5:";
char eof_string_lane_3_number[] = "String 4:";
char eof_string_lane_4_number[] = "String 3:";
char eof_string_lane_5_number[] = "String 2:";
char eof_string_lane_6_number[] = "String 1:";
char *eof_fret_string_numbers[6] = {eof_string_lane_1_number, eof_string_lane_2_number, eof_string_lane_3_number, eof_string_lane_4_number, eof_string_lane_5_number ,eof_string_lane_6_number};

struct MIDIentry *MIDIqueue=NULL;		//Linked list of queued MIDI notes
struct MIDIentry *MIDIqueuetail=NULL;	//Points to the tail of the list
char eof_midi_initialized = 0;			//Specifies whether Allegro was able to set up a MIDI device

FILE *eof_log_fp = NULL;	//Is set to NULL if logging is disabled
char eof_log_level = 0;		//The logging level is set in the config file (1 = normal, 2 = verbose, 3 = exhaustive)
char eof_enable_logging = 1;	//Is set to 0 if logging is disabled
char eof_notes_panel_logged = 0;	//Is set to 1 after the notes panel processing was logged for one frame, if exhaustive logging is enabled, to reduce the repeated duplicate logging

int eof_custom_zoom_level = 0;	//Tracks any user-defined custom zoom level
int eof_display_flats = 0;		//Used to allow eof_get_tone_name() to return note names containing flats.  By default, display as sharps instead

/* storage for keyboard input, needed since we use key data in more than one function */
int eof_key_pressed = 0;
int eof_key_char = 0;
int eof_key_uchar = 0;
int eof_key_code = 0;
int eof_close_button_clicked = 0;
int eof_key_shifts = 0;

///DEBUG
int eof_last_key_char = 0;
int eof_last_key_code = 0;

/* waveform graph settings */
int eof_color_waveform_trough_raw;			//The raw hex formatted RRGGBB colors that need to be converted
int eof_color_waveform_peak_raw;
int eof_color_waveform_rms_raw;
int eof_color_waveform_trough;				//The colors build by makecol()
int eof_color_waveform_peak;
int eof_color_waveform_rms;
char eof_waveform_renderlocation = 0;		//Specifies where and how high the graph will render (0 = fretboard area, 1 = editor window)
char eof_waveform_renderleftchannel = 1;	//Specifies whether the left channel's graph should render
char eof_waveform_renderrightchannel = 0;	//Specifies whether the right channel's graph should render

/* highlight color settings */
int eof_color_highlight1_raw;	//The raw hex formatted RRGGBB colors that need to be converted
int eof_color_highlight2_raw;
int eof_color_highlight1;		//The color used for static highlighting
int eof_color_highlight2;		//The color used for dynamic highlighting

/* GP import drum note mappings */
unsigned char gp_drum_import_lane_1[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_2[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_2_rimshot[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_3[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_3_cymbal[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_3_hi_hat_pedal[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_3_hi_hat_open[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_4[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_4_cymbal[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_5[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_5_cymbal[EOF_GP_DRUM_MAPPING_COUNT] = {0};
unsigned char gp_drum_import_lane_6[EOF_GP_DRUM_MAPPING_COUNT] = {0};

char *ogg_profile_name = NULL;	//This pointer is used by eof_load_ogg to set the file name in the current OGG profile

void eof_show_mouse(BITMAP * bp)
{
	eof_log("eof_show_mouse() entered", 3);

	if(bp && eof_soft_cursor)
	{
		show_mouse(bp);
	}
}

double eof_get_porpos(unsigned long pos)
{
	return eof_get_porpos_sp(eof_song, pos);
}

double eof_get_porpos_sp(EOF_SONG *sp, unsigned long pos)
{
	double porpos = 0.0, blength, rpos;
	unsigned long beat;

	eof_log("eof_get_porpos() entered", 3);

	if(!sp || (sp->beats < 2))
		return 0.0;	//Invalid parameters

	beat = eof_get_beat(sp, pos);
	if(!eof_beat_num_valid(sp, beat))
	{	//If eof_get_beat() returned error
		beat = sp->beats - 1;	//Assume the note position is relative to the last beat marker
	}
	if(beat < sp->beats - 1)
	{
		blength = sp->beat[beat + 1]->fpos - sp->beat[beat]->fpos;
	}
	else
	{
		blength = sp->beat[sp->beats - 1]->fpos - sp->beat[sp->beats - 2]->fpos;
	}
	rpos = (double)pos - sp->beat[beat]->fpos;
	porpos = (rpos / blength) * 100.0;
	return porpos;
}

long eof_put_porpos(unsigned long beat, double porpos, double offset)
{
	return eof_put_porpos_sp(eof_song, beat, porpos, offset);
}

long eof_put_porpos_sp(EOF_SONG *sp, unsigned long beat, double porpos, double offset)
{
	double fporpos = porpos + offset;
	unsigned long cbeat = beat;

	eof_log("eof_put_porpos_sp() entered", 3);

	if(!sp)
		return -1;	//Invalid parameters

	if(fporpos <= -1.0)
	{	//If the target offset is behind the specified beat number
//		allegro_message("a - %f", fporpos);	//Debug
		while(fporpos < 0.0)
		{	//If the position within the beat to find is negative, scale up to a positive number
			fporpos += 100.0;
			if(cbeat == 0)
			{	//If this position couldn't be found
				return -1;	//Return error instead of decrementing below 0
			}
			cbeat--;
		}
		if(cbeat < sp->beats)
		{
			return ((sp->beat[cbeat]->fpos + (eof_get_beat_length(sp, cbeat) * fporpos) / 100.0) + 0.5);	//Round up to nearest millisecond
		}
		return -1;
	}
	else if(fporpos >= 100.0)
	{	//If the target offset is more than 100% of a beat ahead of the specified beat number
//		allegro_message("b - %f", fporpos);	//Debug
		while(fporpos >= 100.0)
		{
			fporpos -= 100.0;
			cbeat++;
		}
		if(cbeat < sp->beats)
		{
			return ((sp->beat[cbeat]->fpos + (eof_get_beat_length(sp, cbeat) * fporpos) / 100.0) + 0.5);	//Round up to nearest millisecond
		}
		return -1;
	}
//	allegro_message("c - %f", fporpos);	//Debug

	if(cbeat < sp->beats)
	{	//If cbeat is valid
		return ((sp->beat[cbeat]->fpos + (eof_get_beat_length(sp, cbeat) * fporpos) / 100.0) + 0.5);	//Round up to nearest millisecond
	}
	return -1;	//Error
}

void eof_reset_lyric_preview_lines(void)
{
	eof_log("eof_reset_lyric_preview_lines() entered", 3);

	eof_preview_line[0] = 0;
	eof_preview_line[1] = 0;
	eof_preview_line_lyric[0] = 0;
	eof_preview_line_lyric[1] = 0;
	eof_preview_line_end_lyric[0] = 0;
	eof_preview_line_end_lyric[1] = 0;
}

void eof_find_lyric_preview_lines(void)
{
//	eof_log("eof_find_lyric_preview_lines() entered");

	unsigned long i, j;
	int current_line = -1;
	int next_line = -1;
	unsigned long dist = 0;
	int beyond = 1;
	int adj_eof_music_pos = eof_music_pos - eof_av_delay;	//The current seek position of the chart, adjusted for AV delay

	for(i = 0; i < eof_song->vocal_track[0]->lines; i++)
	{
		if((adj_eof_music_pos >= eof_song->vocal_track[0]->line[i].start_pos) && (adj_eof_music_pos <= eof_song->vocal_track[0]->line[i].end_pos))
		{
			current_line = i;
			break;
		}
	}
	for(i = 0; i < eof_song->vocal_track[0]->lines; i++)
	{
		if(adj_eof_music_pos <= eof_song->vocal_track[0]->line[i].end_pos)
		{
			beyond = 0;
			break;
		}
	}
	if(beyond)
	{
		eof_preview_line[0] = -1;
		eof_preview_line_lyric[0] = 0;
		eof_preview_line_end_lyric[0] = 0;
		return;
	}
	if(current_line >= 0)
	{
		eof_preview_line[0] = current_line;
		for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
		{
			if((eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->vocal_track[0]->line[eof_preview_line[0]].start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos <= eof_song->vocal_track[0]->line[eof_preview_line[0]].end_pos))
			{
				eof_preview_line_lyric[0] = i;
				for(j = i; j < eof_song->vocal_track[0]->lyrics; j++)
				{
					if(eof_song->vocal_track[0]->lyric[j]->pos > eof_song->vocal_track[0]->line[eof_preview_line[0]].end_pos)
					{
						break;
					}
				}
				eof_preview_line_end_lyric[0] = j;
				break;
			}
		}
		for(i = 0; i < eof_song->vocal_track[0]->lines; i++)
		{
			if((eof_song->vocal_track[0]->line[current_line].start_pos < eof_song->vocal_track[0]->line[i].start_pos) && (!dist || (eof_song->vocal_track[0]->line[i].start_pos - eof_song->vocal_track[0]->line[current_line].start_pos < dist)))
			{
				next_line = i;
				dist = eof_song->vocal_track[0]->line[i].start_pos - eof_song->vocal_track[0]->line[current_line].start_pos;
			}
		}
		if(next_line >= 0)
		{
			eof_preview_line[1] = next_line;
			for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
			{
				if((eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->vocal_track[0]->line[eof_preview_line[1]].start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos <= eof_song->vocal_track[0]->line[eof_preview_line[1]].end_pos))
				{
					eof_preview_line_lyric[1] = i;
					for(j = i; j < eof_song->vocal_track[0]->lyrics; j++)
					{
						if(eof_song->vocal_track[0]->lyric[j]->pos > eof_song->vocal_track[0]->line[eof_preview_line[1]].end_pos)
						{
							break;
						}
					}
					eof_preview_line_end_lyric[1] = j;
					break;
				}
			}
		}
		else
		{
			eof_preview_line[1] = -1;
			eof_preview_line_lyric[1] = 0;
			eof_preview_line_end_lyric[1] = 0;
		}
	}
	else
	{
		eof_preview_line[0] = -1;
	}
}

void eof_emergency_stop_music(void)
{
	eof_log("eof_emergency_stop_music() entered", 2);

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_log("\tStopping playback", 1);
			eof_music_paused = 1;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
		}
		else if(eof_music_catalog_playback)
		{
			eof_log("\tStopping catalog playback", 1);
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
		}
	}
}

void eof_switch_out_callback(void)
{
	eof_log("eof_switch_out_callback() entered", 3);

	if(eof_stop_playback_leave_focus)
	{	//If the user preference is to stop playback when EOF leaves the foreground
		eof_emergency_stop = 1;	//Trigger EOF to call eof_emergency_stop_music()
	}
	eof_clear_input();

	#ifndef ALLEGRO_MACOSX
		eof_has_focus = 0;
	#endif
}

void eof_switch_in_callback(void)
{
	eof_log("eof_switch_in_callback() entered", 3);

	eof_clear_input();
	eof_read_keyboard_input(1);	//Update the keyboard input variables when EOF regains focus
	eof_has_focus = 2;			//Signal EOF has just regained focus
	gametime_reset();
}

void eof_close_button_callback(void)
{
	eof_log("eof_close_button_callback() entered", 2);

	eof_close_button_clicked = 1;
}

void eof_fix_catalog_selection(void)
{
	eof_log("eof_fix_catalog_selection() entered", 1);

	if(eof_song->catalog->entries > 0)
	{
		if(eof_selected_catalog_entry >= eof_song->catalog->entries)
		{
			eof_selected_catalog_entry = eof_song->catalog->entries - 1;
		}
	}
	else
	{
		eof_selected_catalog_entry = 0;
	}
}

int eof_set_display_mode_preset(int mode)
{
	switch(mode)
	{
		case EOF_DISPLAY_640:
			eof_set_display_mode(640, 480);
		break;

		case EOF_DISPLAY_800:
			eof_set_display_mode(800, 600);
		break;

		case EOF_DISPLAY_1024:
			eof_set_display_mode(1024, 768);
		break;

		case EOF_DISPLAY_CUSTOM:
			eof_set_display_mode(eof_screen_width, eof_screen_height);
		break;

		default:
		return 0;	//Invalid display mode
	}

	return 1;
}

int eof_set_display_mode_preset_custom_width(int mode, unsigned long width)
{
	switch(mode)
	{
		case EOF_DISPLAY_640:
			eof_set_display_mode(width, 480);
		break;

		case EOF_DISPLAY_800:
			eof_set_display_mode(width, 600);
		break;

		case EOF_DISPLAY_1024:
			eof_set_display_mode(width, 768);
		break;

		case EOF_DISPLAY_CUSTOM:
			eof_set_display_mode(width, eof_screen_height);
		break;

		default:
		return 0;	//Invalid display mode
	}

	return 1;
}

int eof_set_display_mode(unsigned long width, unsigned long height)
{
	int mode;
	unsigned long effectiveheight = height, effectivewidth = width;
	unsigned long editorwidth;

	eof_log("eof_set_display_mode() entered", 1);

	/* destroy windows first */
	if(eof_window_editor)
	{
		eof_window_destroy(eof_window_editor);
		eof_window_editor = NULL;
	}
	if(eof_window_editor2)
	{
		eof_window_destroy(eof_window_editor2);
		eof_window_editor2 = NULL;
	}
	if(eof_window_note_lower_left)
	{
		eof_window_destroy(eof_window_note_lower_left);
		eof_window_note_lower_left = NULL;
	}
	if(eof_window_note_upper_left)
	{
		eof_window_destroy(eof_window_note_upper_left);
		eof_window_note_upper_left = NULL;
	}
	if(eof_window_3d)
	{
		eof_window_destroy(eof_window_3d);
		eof_window_3d = NULL;
	}
	if(eof_window_notes)
	{
		eof_window_destroy(eof_window_notes);
		eof_window_notes = NULL;
	}
	if(eof_screen)
	{
		destroy_bitmap(eof_screen);
		eof_screen = NULL;
	}
	if(eof_screen2)
	{
		destroy_bitmap(eof_screen2);
		eof_screen2 = NULL;
	}

	if(eof_screen_zoom)
	{	//If x2 zoom is in effect
		effectiveheight = height * 2;
		effectivewidth = width * 2;
	}

	eof_3d_fretboard_coordinates_cached = 0;	//The 3D rendering logic will need to rebuild the fretboard's 2D coordinate projections
	switch(height)
	{
		case 480:
			mode = EOF_DISPLAY_640;
			eof_screen_width_default = 640;
			eof_screen_width = width;
			eof_screen_height = 480;
			eof_screen_layout.string_space_unscaled = 20;
			eof_screen_layout.vocal_y = 96;
			eof_screen_layout.vocal_tail_size = 4;
			eof_screen_layout.lyric_view_bkey_width = 3;
			eof_screen_layout.note_size = 6;
			eof_screen_layout.hopo_note_size = 4;
			eof_screen_layout.anti_hopo_note_size = 8;
			eof_screen_layout.note_dot_size = 2;
			eof_screen_layout.hopo_note_dot_size = 1;
			eof_screen_layout.anti_hopo_note_dot_size = 4;
			eof_screen_layout.note_marker_size = 10;
			eof_screen_layout.note_tail_size = 3;
		break;

		case 600:
			mode = EOF_DISPLAY_800;
			eof_screen_width_default = 800;
			eof_screen_width = width;
			eof_screen_height = 600;
			eof_screen_layout.string_space_unscaled = 30;
			eof_screen_layout.vocal_y = 128;
			eof_screen_layout.vocal_tail_size = 6;
			eof_screen_layout.lyric_view_bkey_width = 4;
			eof_screen_layout.note_size = 8;
			eof_screen_layout.hopo_note_size = 5;
			eof_screen_layout.anti_hopo_note_size = 11;
			eof_screen_layout.note_dot_size = 3;
			eof_screen_layout.hopo_note_dot_size = 2;
			eof_screen_layout.anti_hopo_note_dot_size = 5;
			eof_screen_layout.note_marker_size = 12;
			eof_screen_layout.note_tail_size = 4;
		break;

		case 768:
			mode = EOF_DISPLAY_1024;
			eof_screen_width_default = 1024;
			eof_screen_width = width;
			eof_screen_height = 768;
			eof_screen_layout.string_space_unscaled = 48;
			eof_screen_layout.vocal_y = 197;
			eof_screen_layout.vocal_tail_size = 11;
			eof_screen_layout.lyric_view_bkey_width = 5;
			eof_screen_layout.note_size = 10;
			eof_screen_layout.hopo_note_size = 7;
			eof_screen_layout.anti_hopo_note_size = 14;
			eof_screen_layout.note_dot_size = 4;
			eof_screen_layout.hopo_note_dot_size = 3;
			eof_screen_layout.anti_hopo_note_dot_size = 7;
			eof_screen_layout.note_marker_size = 15;
			eof_screen_layout.note_tail_size = 5;
		break;

		default:
			if(height < 480)
				return 0;	//Invalid display mode
			mode = EOF_DISPLAY_CUSTOM;

			//Find a suitable default width based on 4:3 aspect ratio
			if(height % 3 == 0)									//If the height is divisible by 3
				eof_screen_width_default = (height / 3) * 4;	//Keep a 4:3 aspect ratio for the default width
			else if(height >= 1536)
				eof_screen_width_default = 2048;
			else if(height >= 1440)
				eof_screen_width_default = 1920;
			else if(height >= 1392)
				eof_screen_width_default = 1856;
			else if(height >= 1200)
				eof_screen_width_default = 1600;
			else if(height >= 1080)
				eof_screen_width_default = 1440;
			else if(height >= 1050)
				eof_screen_width_default = 1400;
			else if(height >= 960)
				eof_screen_width_default = 1280;
			else if(height >= 720)
				eof_screen_width_default = 960;
			else
				eof_screen_width_default = 640;

			eof_screen_width = width;
			eof_screen_height = height;
			eof_screen_layout.string_space_unscaled = ((eof_screen_height / 2) - 80 - 60) / 5;	//Half the program window height, minus the empty space above and below the fretboard area, divided into 5 lanes
			eof_screen_layout.vocal_y = height / 5;
			eof_screen_layout.vocal_tail_size = height / 120;
			eof_screen_layout.lyric_view_bkey_width = height / 160;
			eof_screen_layout.note_size = height / 80;
			eof_screen_layout.hopo_note_size = height / 120;
			eof_screen_layout.anti_hopo_note_size = height / 60;
			eof_screen_layout.note_dot_size = eof_screen_layout.note_size / 3;
			eof_screen_layout.hopo_note_dot_size = eof_screen_layout.note_dot_size / 2;
			eof_screen_layout.anti_hopo_note_dot_size = eof_screen_layout.note_dot_size * 2;
			eof_screen_layout.note_marker_size = eof_screen_layout.string_space_unscaled / 3;
			eof_screen_layout.note_tail_size = height / 160;
		break;
	}
	if(eof_screen_width < eof_screen_width_default)
	{	//If the specified width is invalid
		eof_screen_width = eof_screen_width_default;
		effectivewidth = eof_screen_width_default;
	}

	if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, effectivewidth, effectiveheight, 0, 0))
	{	//If the specified window size could not be set, try again
		if(set_gfx_mode(GFX_AUTODETECT, effectivewidth, effectiveheight, 0, 0))
		{	//If it failed again
			if(eof_screen_zoom)
			{	//If x2 zoom display is enabled
				eof_screen_zoom = 0;	//Disable x2 zoom
				eof_set_display_mode(width, height);
				allegro_message("Warning:  Failed to enable x2 zoom.\nTry setting a smaller program window size first.");
				return 2;
			}
			if(width != eof_screen_width_default)
			{	//If the custom width failed to be applied, revert to the default width for this window height
				eof_set_display_mode(eof_screen_width_default, height);
				allegro_message("Warning:  Failed to set custom display width, reverted to default.\nTry setting a different width.");
				return 2;
			}
			if(set_gfx_mode(GFX_SAFE, width, eof_screen_height, 0, 0))
			{	//If the window size could not be set at all
				allegro_message("Can't set up screen!  Error: %s",allegro_error);
				return 0;
			}

			allegro_message("EOF is in safe mode!\nThings may not work as expected.");
		}
	}
	eof_screen = create_bitmap(eof_screen_width, eof_screen_height);
	if(!eof_screen)
	{
		return 0;
	}
	eof_screen2 = create_bitmap(eof_screen_width * 2, eof_screen_height * 2);
	if(!eof_screen2)
	{
		destroy_bitmap(eof_screen);
		return 0;
	}
	if(eof_full_height_3d_preview)
	{	//If the 3D preview is expanded to take the full program window height
		editorwidth = eof_screen_width - EOF_SCREEN_PANEL_WIDTH;	//It will use one panel's worth of the editor window's space
	}
	else
	{
		editorwidth = eof_screen_width;	//Otherwise the editor windows will be full window width
	}
	eof_window_editor = eof_window_create(0, 20, editorwidth, (eof_screen_height / 2) - 20, eof_screen);
	if(!eof_window_editor)
	{
		allegro_message("Unable to create editor window!");
		return 0;
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBuilt eof_window_editor:  x = %d, y = %d, w = %d, h = %d", eof_window_editor->x, eof_window_editor->y, eof_window_editor->w, eof_window_editor->h);
	eof_log(eof_log_string, 2);
	eof_window_editor2 = eof_window_create(0, (eof_screen_height / 2) + 1, editorwidth, eof_screen_height, eof_screen);
	if(!eof_window_editor2)
	{
		allegro_message("Unable to create second editor window!");
		return 0;
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBuilt eof_window_editor2:  x = %d, y = %d, w = %d, h = %d", eof_window_editor2->x, eof_window_editor2->y, eof_window_editor2->w, eof_window_editor2->h);
	eof_log(eof_log_string, 2);
	eof_window_note_lower_left = eof_window_create(0, eof_screen_height / 2, editorwidth, eof_screen_height / 2, eof_screen);	//Make the note window the same width as the editor window
	eof_window_note_upper_left = eof_window_create(0, 20, editorwidth, eof_screen_height / 2, eof_screen);
	if(!eof_window_note_lower_left || !eof_window_note_upper_left)
	{
		allegro_message("Unable to create information windows!");
		return 0;
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBuilt eof_window_note_lower_left:  x = %d, y = %d, w = %d, h = %d", eof_window_note_lower_left->x, eof_window_note_lower_left->y, eof_window_note_lower_left->w, eof_window_note_lower_left->h);
	eof_log(eof_log_string, 2);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBuilt eof_window_note_upper_left:  x = %d, y = %d, w = %d, h = %d", eof_window_note_upper_left->x, eof_window_note_upper_left->y, eof_window_note_upper_left->w, eof_window_note_upper_left->h);
	eof_log(eof_log_string, 2);
	if(eof_full_height_3d_preview)
	{	//If the 3D preview is expanded to take the full program window height
		eof_window_3d = eof_window_create(eof_screen_width - EOF_SCREEN_PANEL_WIDTH, 20, EOF_SCREEN_PANEL_WIDTH, eof_screen_height, eof_screen);
	}
	else
	{
		eof_window_3d = eof_window_create(eof_screen_width - EOF_SCREEN_PANEL_WIDTH, eof_screen_height / 2, EOF_SCREEN_PANEL_WIDTH, eof_screen_height / 2, eof_screen);
	}
	if(!eof_window_3d)
	{
		allegro_message("Unable to create 3D preview window!");
		return 0;
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBuilt eof_window_3d:  x = %d, y = %d, w = %d, h = %d", eof_window_3d->x, eof_window_3d->y, eof_window_3d->w, eof_window_3d->h);
	eof_log(eof_log_string, 2);
	eof_screen_layout.scrollbar_y = (eof_screen_height / 2) - 37;
	eof_scale_fretboard(5);	//Set the eof_screen_layout.note_y[] array based on a 5 lane track, for setting the fretboard height below
	eof_screen_layout.lyric_y = 20;
	eof_screen_layout.vocal_view_size = 13;

	if(mode == EOF_DISPLAY_CUSTOM)
	{	//Make some vocal editor adjustments for custom window sizes
		eof_screen_layout.vocal_tail_size = (eof_screen_layout.string_space_unscaled * 4 - eof_screen_layout.lyric_y) / (eof_screen_layout.vocal_view_size + 2);	//Divide the piano roll space among the number of keys that are to be displayed when the vocal track is active, with an extra key of blank space at the top and bottom of the piano
		eof_screen_layout.vocal_y = eof_screen_layout.note_y[4] - eof_screen_layout.vocal_tail_size;	//Position the bottom of the mini piano to one key height above the bottom of the piano roll
	}

	eof_screen_layout.lyric_view_key_width = eof_window_3d->screen->w / 29;
	eof_screen_layout.lyric_view_key_height = eof_screen_layout.lyric_view_key_width * 4;
	eof_screen_layout.lyric_view_bkey_height = eof_screen_layout.lyric_view_key_width * 3;
	eof_screen_layout.fretboard_h = eof_screen_layout.note_y[4] + 10 + (2 * eof_screen_layout.anti_hopo_note_size);	//Make enough of a gap below the lowest fret line so that a HOPO off note doesn't obscure tab notation
	eof_screen_layout.controls_x = eof_screen_width - 197;
	eof_screen_layout.mode = mode;
	eof_vanish_x = 160;
	eof_set_3d_projection();
	set_display_switch_mode(SWITCH_BACKGROUND);
	set_display_switch_callback(SWITCH_OUT, eof_switch_out_callback);
	set_display_switch_callback(SWITCH_IN, eof_switch_in_callback);
	set_close_button_callback(eof_close_button_callback);
	PrepVSyncIdle();
	return 1;
}

void eof_cat_track_difficulty_string(char *str)
{
	if(!str)
		return;	//Invalid parameter

	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_ALT_NAME)
	{	//If this track has an alternate name, append the track's alternate name
		(void) ustrcat(str, eof_song->track[eof_selected_track]->altname);
	}
	else
	{	//Otherwise append the track's native name
		(void) ustrcat(str, eof_song->track[eof_selected_track]->name);
	}
	if(!eof_vocals_selected)
	{	//If the vocal track isn't active, append other information such as the current difficulty
		char *ptr;

		(void) ustrcat(str, "  ");
		if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
		{	//If this track is not limited to 5 difficulties
			char diff_string[15] = {0};			//Used to generate the string for a numbered difficulty
			(void) snprintf(diff_string, sizeof(diff_string) - 1, " Diff: %u", eof_note_type);
			if(eof_track_diff_populated_status[eof_note_type])
			{	//If this difficulty is populated
				diff_string[0] = '*';
			}
			(void) ustrcat(str, diff_string);
		}
		else if(eof_selected_track == EOF_TRACK_DANCE)
		{	//If the dance track is active
			ptr = eof_dance_tab_name[eof_note_type];
			(void) ustrcat(str, ptr);					//Append the active dance difficulty name
		}
		else
		{
			ptr = eof_note_type_name[eof_note_type];
			(void) ustrcat(str, ptr);					//Append the active instrument difficulty name
		}
	}

	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_ALT_NAME)
	{	//If this track has an alternate name, append the track's native name
		(void) ustrcat(str, " (");
		(void) ustrcat(str, eof_song->track[eof_selected_track]->name);
		(void) ustrcat(str, ")");
	}
}

void eof_fix_window_title(void)
{
	eof_log("eof_fix_window_title() entered", 2);

	if(eof_changes)
	{
		(void) ustrcpy(eof_window_title, "*EOF - ");
	}
	else
	{
		(void) ustrcpy(eof_window_title, "EOF - ");
	}
	if(eof_song && eof_song_loaded)
	{
		eof_cat_track_difficulty_string(eof_window_title);	//Append the track difficulty's title to the window title

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If this is a pro guitar track being displayed
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
			if(tp->note == tp->technote)
			{	//If tech view is enabled
				(void) ustrcat(eof_window_title, "(Tech view)");
			}
			if(eof_legacy_view)
			{	//If legacy view is enabled
				(void) ustrcat(eof_window_title, "(Legacy view)");
			}
		}

		if(eof_track_is_ghl_mode(eof_song, eof_selected_track))
		{	//If GHL mode is enabled for the active track
			(void) ustrcat(eof_window_title, "(GHL)");
		}
		if(eof_song->tags->tempo_map_locked)
		{	//If the tempo map is locked
			(void) ustrcat(eof_window_title, "(Tempo map locked)");
		}
		if(eof_song->tags->click_drag_disabled)
		{	//If click and drag is disabled
			(void) ustrcat(eof_window_title, "(Click+drag disabled)");
		}
		if(eof_song->tags->double_bass_drum_disabled)
		{	//If expert+ bass drum is disabled
			(void) ustrcat(eof_window_title, "(Expert+ drums off)");
		}
		if(eof_silence_loaded)
		{	//If no chart audio is actually loaded
			(void) ustrcat(eof_window_title, "(No audio loaded)");
		}
		if(eof_song_has_stored_tempo_track(eof_song))
		{	//If the project has a stored tempo track
			(void) ustrcat(eof_window_title, "(Stored tempo map)");
		}
		if(eof_phase_cancellation)
		{	//If center panned vocals are being removed
			(void) ustrcat(eof_window_title, "(Phase cancellation)");
		}
		if(eof_center_isolation)
		{	//If center panned vocals are being removed
			(void) ustrcat(eof_window_title, "(Center isolation)");
		}
	}
	else
	{
		(void) ustrcat(eof_window_title, "No Song");
	}
	set_window_title(eof_window_title);
	eof_window_title_dirty = 0;
}

void eof_clear_input(void)
{
	int i;

	eof_log("eof_clear_input() entered", 3);

	clear_keybuf();
	for(i = 0; i < KEY_MAX; i++)
	{
		if((i != KEY_ALTGR) && (i != KEY_ALT) && (i != KEY_LCONTROL) && (i != KEY_RCONTROL) && (i != KEY_LSHIFT) && (i != KEY_RSHIFT))
		{
			key[i] = 0;
		}
	}
	eof_mouse_z = mouse_z;
	eof_use_key();	//Clear key input state of the new key press system
}

void eof_prepare_undo(int type)
{
	char fn[1024] = {0};	//Use an internal array for building the periodic backup path, since the calling function may need eof_temp_filename[] to remain intact

	eof_log("eof_prepare_undo() entered", 1);

	if(eof_undo_add(type))
	{
		eof_change_count++;
		eof_changes = 1;
	}
	eof_redo_toggle = 0;
	eof_undo_toggle = 1;
	eof_fix_window_title();	//Redraw the window title to reflect the chart is modified
	eof_window_title_dirty = 1;	//Indicate that the window title will need to be redrawn during the next normal render, to reflect whatever change is to follow this undo state
	eof_destroy_sp_solution(eof_ch_sp_solution);	//Destroy the SP solution structure so it's rebuilt
	eof_ch_sp_solution = NULL;
	if(!eof_disable_backups && (eof_change_count % 10 == 0))
	{	//If automatic backups are not disabled, backup the EOF project every 10 undo states
		(void) replace_extension(fn, eof_filename, "backup.eof.bak", 1024);
		eof_log("\tSaving periodic backup", 1);
		if(!eof_save_song(eof_song, fn))
		{
			allegro_message("Undo state error!");
		}
//		allegro_message("backup saved\n%s", eof_temp_filename);
	}
}

long eof_get_previous_note(long cnote, int function)
{
	long i;

	eof_log("eof_get_previous_note() entered", 3);

	for(i = cnote - 1; i >= 0; i--)
	{	//For each note before the specified note, in reverse order
		if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_get_note_type(eof_song, eof_selected_track, cnote))
		{	//If the note is in the same difficulty
			if(function && (eof_get_note_pos(eof_song, eof_selected_track, i) == eof_get_note_pos(eof_song, eof_selected_track, cnote)))
			{	//If the calling function also requires the previous note to be at a different timestamp, but this condition is not met
				continue;	//Skip this note
			}
			return i;
		}
	}
	return -1;
}

int eof_note_is_gh3_hopo(EOF_SONG *sp, unsigned long track, unsigned long note, double threshold)
{
	long pnote;
	unsigned long thispos, prevpos, pbeat;
	double distance;
	unsigned den = 4;

	if(!sp || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)))
		return 0;	//Invalid parameters

	if(eof_get_note_flags(sp, track, note) & EOF_NOTE_FLAG_F_HOPO)
		return 1;	//Forced HOPO

	if(eof_get_note_flags(sp, track, note) & EOF_NOTE_FLAG_NO_HOPO)
		return 0;	//Forced non HOPO

	if(eof_note_count_colors(sp, track, note) > 1)
		return 0;	//Chords are non HOPO by default

	pnote = eof_track_fixup_previous_note(sp, track, note);
	if(pnote < 0)
		return 0;	//A note can't be an auto HOPO if there is no previous note

	if(eof_get_note_note(sp, track, note) == eof_get_note_note(sp, track, pnote))
		return 0;	//Identical notes are non HOPO by default

	prevpos = eof_get_note_pos(sp, track, pnote);
	thispos = eof_get_note_pos(sp, track, note);
	if(thispos < prevpos)
		return 0;	//Notes out of order

	//Get the distance between the two notes, measured in quarter notes
	distance = eof_get_distance_in_beats(sp, prevpos, thispos);	//Get the distance measured in beats
	if(distance == 0.0)
	{	//If the distance between the two couldn't be determined
		return 0;
	}
	pbeat = eof_get_beat(sp, prevpos);
	if(pbeat >= sp->beats)
	{	//If the beat the previous note is in couldn't be identified
		return 0;
	}
	if(!eof_get_effective_ts(sp, NULL, &den, pbeat, 0))
	{	//If the time signature's beat unit in effect on the previous note's beat couldn't be determined
		return 0;
	}
	if(den != 4)
	{	//If the previous note isn't in #/4 meter
		distance = (distance * 4) / den;	//Scale the threshold to account for time signature so that it reflects quarter note length instead of beat length
	}

	if(distance < threshold)
	{	//If the distance between the the two notes is below the threshold
		return 1;	//Auto HOPO
	}

	return 0;	//Not a HOPO
}

int eof_note_is_hopo(unsigned long cnote)
{
	double delta;
	double hopo_delta = eof_song->tags->eighth_note_hopo ? 250.0 : 170.0;	//The proximity threshold to determine HOPO eligibility, when the tempo is 120BPM (within 1/3 beat, or 1/2 beat if 8th note HOPO option is enabled)
	unsigned long i, cnotepos, forcedhopo;
	long pnote;
	long beat = -1;
	double bpm;
	double scale;	//Scales the proximity threshold for the given beat's tempo

//	eof_log("eof_note_is_hopo() entered", 3);

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		return 0;	//Only guitar/bass tracks can have HOPO notes

	if(eof_hopo_view == EOF_HOPO_GH3)
		return eof_note_is_gh3_hopo(eof_song, eof_selected_track, cnote, (66.0 / 192.0));	//Use the typical GH3 threshold

	forcedhopo = (eof_get_note_flags(eof_song, eof_selected_track, cnote) & EOF_NOTE_FLAG_F_HOPO);
	if(eof_hopo_view == EOF_HOPO_MANUAL)
	{
		if(forcedhopo)
		{	//In manual HOPO preview mode, a note only shows as a HOPO if it is explicitly marked as one
			return 1;
		}
		return 0;
	}
	if(cnote == 0)
		return 0;	//Otherwise if the specified note isn't at least the second note in the track, it can't be a HOPO

	cnotepos = eof_get_note_pos(eof_song, eof_selected_track, cnote);
	for(i = 0; i < eof_song->beats - 1; i++)
	{	//Find the beat that cnote is in
		if((eof_song->beat[i]->pos <= cnotepos) && (eof_song->beat[i + 1]->pos > cnotepos))
		{
			beat = i;
			break;
		}
	}
	if(beat < 0)
	{
		return 0;
	}
	bpm = 60000000.0 / (double)eof_song->beat[beat]->ppqn;
	scale = 120.0 / bpm;

	pnote = eof_get_previous_note(cnote, 1);
	if(pnote < 0)
	{
		return 0;
	}
	if(eof_hopo_view == EOF_HOPO_RF)
	{
		delta = cnotepos - eof_get_note_pos(eof_song, eof_selected_track, pnote);
		if((delta <= (hopo_delta * scale)) && (eof_note_count_colors(eof_song, eof_selected_track, pnote) == 1) && (eof_note_count_colors(eof_song, eof_selected_track, cnote) == 1) && (eof_get_note_note(eof_song, eof_selected_track, pnote) != eof_get_note_note(eof_song, eof_selected_track, cnote)))
		{	//If this note is close enough to the previous note, both notes are not chords, and both notes are a different gem
			return 1;
		}
	}
	else if(eof_hopo_view == EOF_HOPO_FOF)
	{
		delta = cnotepos - (eof_get_note_pos(eof_song, eof_selected_track, pnote) + eof_get_note_length(eof_song, eof_selected_track, pnote));
		if((delta <= hopo_delta * scale) && !(eof_get_note_note(eof_song, eof_selected_track, pnote) & eof_get_note_note(eof_song, eof_selected_track, cnote)))
		{	//If this note is close enough to the previous note and both notes have no gems in common
			return 1;
		}
	}
//	allegro_message("bpm = %f\nscale = %f\ndelta = %f\npnote = %d(%d), cnote = %d(%d)", bpm, scale, delta, pnote, eof_song->legacy_track[tracknum]->note[pnote].pos, cnote, eof_song->legacy_track[tracknum]->note[cnote].pos);	//Debug

	return 0;
}

void eof_determine_phrase_status(EOF_SONG *sp, unsigned long track)
{
	unsigned long i, j, k, tracknum;
	char st[EOF_MAX_PHRASES] = {0};
	char so[EOF_MAX_PHRASES] = {0};
	char trills[EOF_MAX_PHRASES] = {0};
	char tremolos[EOF_MAX_PHRASES] = {0};
	char arpeggios[EOF_MAX_PHRASES] = {0};
	char sliders[EOF_MAX_PHRASES] = {0};
	unsigned long notepos, flags, tflags, numphrases, numnotes;
	unsigned char notetype;
	EOF_PHRASE_SECTION *sectionptr = NULL;
	char restore_tech_view = 0;

	eof_log("eof_determine_phrase_status() entered", 2);

	if(!sp || (track >= sp->tracks) || !track)
		return;	//Invalid parameters
	if(!eof_music_paused)
		return;	//Do not allow this to run during playback because it causes too much lag when switching to a track with a large number of notes

	restore_tech_view = eof_menu_track_get_tech_view_state(sp, track);
	eof_menu_track_set_tech_view_state(sp, track, 0);	//Disable tech view if applicable

	tracknum = sp->track[track]->tracknum;
	numnotes = eof_get_track_size(sp, track);
	for(k = 0; k < 2; k++)
	{	//Perform two passes, because the two drum tracks can share their phrasing
		if(k > 0)
		{	//If this is the second pass
			if(sp->tags->unshare_drum_phrasing)
			{	//If drum phrase sharing isn't enabled
				break;	//Don't perform a second pass
			}
			if(sp->track[track]->track_type == EOF_TRACK_DRUM_PS)
			{	//If the first pass processed the Phase Shift drum track
				track = EOF_TRACK_DRUM;	//The second pass will process the regular drum track
				numnotes = eof_get_track_size(sp, track);
			}
			else if(sp->track[track]->track_type == EOF_TRACK_DRUM)
			{	//And vice-versa
				track = EOF_TRACK_DRUM_PS;
				numnotes = eof_get_track_size(sp, track);
			}
			else
			{	//Otherwise if it's not a drum track
				break;	//Don't perform a second pass
			}
		}

		for(i = 0; i < numnotes; i++)
		{	//For each note in the active track
			/* clear the flags */
			notepos = eof_get_note_pos(sp, track, i);
			notetype = eof_get_note_type(sp, track, i);
			flags = eof_get_note_flags(sp, track, i);
			tflags = eof_get_note_tflags(sp, track, i);
			flags &= (~EOF_NOTE_FLAG_HOPO);
			flags &= (~EOF_NOTE_FLAG_SP);
			flags &= (~EOF_NOTE_FLAG_IS_TRILL);
			flags &= (~EOF_NOTE_FLAG_IS_TREMOLO);
			tflags &= (~EOF_NOTE_TFLAG_SOLO_NOTE);
			if(((sp->track[track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (sp->track[track]->track_format == EOF_LEGACY_TRACK_FORMAT)) || (track == EOF_TRACK_KEYS))
			{	//Only clear the is slider flag if this is a legacy guitar or keys track
				flags &= (~EOF_GUITAR_NOTE_FLAG_IS_SLIDER);
			}

			/* mark HOPO */
			if(eof_note_is_hopo(i))
			{
				flags |= EOF_NOTE_FLAG_HOPO;
				flags &= ~EOF_NOTE_FLAG_NO_HOPO;	//Ensure that if a note has forced HOPO and forced non HOPO, forced HOPO takes precedence
			}

			/* mark and check star power notes */
			numphrases = eof_get_num_star_power_paths(sp, track);
			for(j = 0; j < numphrases; j++)
			{	//For each star power path in the active track
				sectionptr = eof_get_star_power_path(sp, track, j);
				if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
				{	//If the note is in this star power section
					flags |= EOF_NOTE_FLAG_SP;
					st[j] = 1;
				}
			}

			/* check solos */
			numphrases = eof_get_num_solos(sp, track);
			for(j = 0; j < numphrases; j++)
			{	//For each solo section in the active track
				sectionptr = eof_get_solo(sp, track, j);
				if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
				{	//If the note is in this solo section
					tflags |= EOF_NOTE_TFLAG_SOLO_NOTE;
					so[j] = 1;
				}
			}

			/* mark and check trills */
			numphrases = eof_get_num_trills(sp, track);
			for(j = 0; j < numphrases; j++)
			{	//For each trill section in the active track
				sectionptr = eof_get_trill(sp, track, j);
				if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
				{	//If the note is in this trill section
					flags |= EOF_NOTE_FLAG_IS_TRILL;
					trills[j] = 1;
				}
			}

			/* mark and check tremolos */
			numphrases = eof_get_num_tremolos(sp, track);
			for(j = 0; j < numphrases; j++)
			{	//For each tremolo section in the active track
				sectionptr = eof_get_tremolo(sp, track, j);
				if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
				{	//If the note is in this tremolo section
					if(eof_song->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
					{	//If the track's difficulty limit has been removed
						if(sectionptr->difficulty == notetype)	//And the tremolo section applies to this note's track difficulty
							flags |= EOF_NOTE_FLAG_IS_TREMOLO;
					}
					else
					{
						if(sectionptr->difficulty == 0xFF)	//Otherwise if the tremolo section applies to all track difficulties
							flags |= EOF_NOTE_FLAG_IS_TREMOLO;
					}

					tremolos[j] = 1;
				}
			}

			/* check arpeggios */
			if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If this is a pro guitar track
				for(j = 0; j < sp->pro_guitar_track[tracknum]->arpeggios; j++)
				{	//For each arpeggio section in the active track
					sectionptr = &sp->pro_guitar_track[tracknum]->arpeggio[j];
					if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
					{	//If the note is in this arpeggio section
						if((sectionptr->difficulty == 0xFF) || (sectionptr->difficulty == notetype))
						{	//If the arpeggio section applies to all difficulties or if it applies to this note's track difficulty
							arpeggios[j] = 1;
						}
					}
				}
			}

			/* mark and check sliders */
			if(((sp->track[track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (sp->track[track]->track_format == EOF_LEGACY_TRACK_FORMAT)) || (track == EOF_TRACK_KEYS))
			{	//Only check the is slider flag if this is a legacy guitar or keys track
				numphrases = eof_get_num_sliders(sp, track);
				for(j = 0; j < numphrases; j++)
				{	//For each slider section in the active track
					sectionptr = eof_get_slider(sp, track, j);
					if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
					{	//If the note is in this slider section
						flags |= EOF_GUITAR_NOTE_FLAG_IS_SLIDER;
						sliders[j] = 1;
					}
				}
			}

			if(!k)
			{	//Only update note flags on the first pass
				eof_set_note_flags(sp, track, i, flags);	//Update the note's flags variable
				eof_set_note_tflags(sp, track, i, tflags);	//Update the note's temporary flags variable
			}
		}//For each note in the active track
	}//Perform two passes, because the two drum tracks can share their phrasing

	/* delete star power phrases with no notes */
	numphrases = eof_get_num_star_power_paths(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each star power section in the active track (in reverse order)
		if(!st[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_star_power_path(sp, track, j - 1);
		}
	}

	/* delete solos with no notes */
	numphrases = eof_get_num_solos(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each solo section in the active track (in reverse order)
		if(!so[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_solo(sp, track, j - 1);
		}
	}

	/* delete trills with no notes */
	numphrases = eof_get_num_trills(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each trill section in the active track (in reverse order)
		if(!trills[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_trill(sp, track, j - 1);
		}
	}

	/* delete tremolos with no notes */
	numphrases = eof_get_num_tremolos(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each tremolo section in the active track (in reverse order)
		if(!tremolos[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_tremolo(sp, track, j - 1);
		}
	}

	/* delete arpeggios with no notes */
	if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If this is a pro guitar track
		for(j = sp->pro_guitar_track[tracknum]->arpeggios; j > 0; j--)
		{	//For each arpeggio section in the active track (in reverse order)
			if(!arpeggios[j - 1])
			{	//If the section's note count taken above was 0
				eof_pro_guitar_track_delete_arpeggio(sp->pro_guitar_track[tracknum], j - 1);
			}
		}
	}

	/* delete sliders with no notes */
	numphrases = eof_get_num_sliders(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each slider section in the active track (in reverse order)
		if(!sliders[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_slider(sp, track, j - 1);
		}
	}

	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
}

int eof_figure_difficulty(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int nt[5] = {0};
	int dt[5] = {-1, -1, -1, -1};
	int count = 0;

	eof_log("eof_figure_difficulty() entered", 1);

	if(eof_selected_track > EOF_TRACK_VOCALS)
		return -1;	//FoFiX does not support any custom, keys or pro tracks

	/* check for lyrics if PART VOCALS is selected */
	if(eof_selected_track == EOF_TRACK_VOCALS)
	{
		/* see if there are any pitched lyrics */
		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{
			if(eof_song->vocal_track[tracknum]->lyric[i]->note != 0)
			{
				break;
			}
		}

		/* if we have pitched lyrics and line definitions, allow test */
		if((i < eof_song->vocal_track[tracknum]->lyrics) && (eof_song->vocal_track[tracknum]->lines > 0))
		{
			return 0;
		}
		return -1;
	}

	/* see which difficulties are populated with notes */
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		unsigned type = eof_get_note_type(eof_song, eof_selected_track, i);
		if(type != 0xFF)
		{	//If the note's difficulty level was identified
			nt[type] = 1;
		}
	}

	/* no notes in this difficulty so don't allow test */
	if(!nt[eof_note_type])
	{
		return -1;
	}
	for(i = 0; i < 5; i++)
	{
		if(nt[i])
		{
			dt[i] = count;
			count++;
		}
	}
	return dt[eof_note_type];
}

/* total is used to determine the total number of notes including unselected notes */
unsigned long eof_count_selected_notes(unsigned long *total)
{
//	eof_log("eof_count_selected_notes() entered");

	unsigned long count = 0, i;
	long last = -1;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note/lyric in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
		{	//If the note is in the active difficulty
			if(eof_selection.track == eof_selected_track)
			{	//If the note selection pertains to the active track
				if(eof_selection.multi[i])
				{	//If the note is selected
					last = i;
					count++;
				}
			}
			if(total)
			{	//If the calling function passed a non NULL pointer
				(*total)++;	//Increment the value it points to
			}
		}
	}

	/* if no notes remain selected, current note should be unset */
	if(count == 0)
	{
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_selection.current_pos = 0;
	}

	/* if only one note is selected, make sure current note is set to that note */
	else if(count == 1)
	{
		eof_selection.current = last;
	}
	return count;
}

unsigned long eof_get_selected_note_range(unsigned long *sel_start, unsigned long *sel_end, char function)
{
	unsigned long ctr, start, end, pos, startpos, endpos = 0, count = 0;
	long length, after;
	char first = 1;

	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type))
			continue;	//If the note is not selected, skip it

		pos = eof_get_note_pos(eof_song, eof_selected_track, ctr);
		length = eof_get_note_length(eof_song, eof_selected_track, ctr);
		count++;	//Track the number of notes within the selection that are explicitly selected
		if(first)
		{	//The first selected note is always the earliest in the selection, since notes are sorted
			start = end = ctr;
			startpos = pos;
			endpos = pos + length;
			first = 0;
		}
		else
		{
			if(ctr > end)
			{	//Track the index of the last note that is selected
				end = ctr;
			}
			if(pos + length > endpos)
			{	//Track the latest end position among all selected notes, due to crazy notes overlapping, end and endpos may reflect different notes
				endpos = pos + length;
			}
		}
	}
	if(first)
	{	//If no selected notes were found
		return 0;
	}
	after = eof_track_fixup_next_note(eof_song, eof_selected_track, end);
	if(after > 0)
	{	//If there is a note that follows the last selected note
		if(endpos == eof_get_note_pos(eof_song, eof_selected_track, after))
		{	//If that note begins at the same time the last selected note ends (can occur if linknext status is applied to the earlier note)
			if(endpos - startpos > 1)
			{	//If the selection range can be shortened
				endpos--;	//Do so by 1ms so that phrase selection functions can still target the affected notes individually
			}
		}
	}
	if(!function)
	{	//Return the selection information as index numbers
		if(sel_start)
			*sel_start = start;
		if(sel_end)
			*sel_end = end;
	}
	else
	{	//Return the selection information as timestamps
		if(sel_start)
			*sel_start = startpos;
		if(sel_end)
			*sel_end = endpos;
	}
	return count;
}

int eof_figure_part(void)
{
	int part[EOF_TRACKS_MAX] = {-1};	//Ensure that any tracks FoF don't support reflect -1 being returned

	eof_log("eof_figure_part() entered", 1);

	if(eof_get_track_size(eof_song,eof_selected_track) == 0)
		return -1;

	part[EOF_TRACK_GUITAR] = 0;
	part[EOF_TRACK_RHYTHM] = 1;
	part[EOF_TRACK_BASS] = 2;
	part[EOF_TRACK_GUITAR_COOP] = 3;
	part[EOF_TRACK_DRUM] = 4;
	part[EOF_TRACK_VOCALS] = 5;
	part[EOF_TRACK_KEYS] = 6;
	return part[eof_selected_track];
}

int eof_load_ogg_quick(char * filename)
{
	int loaded = 0;

	eof_log("eof_load_ogg_quick() entered", 1);

	if(!filename)
	{
		return 0;
	}
	eof_destroy_ogg();
	eof_music_data = (void *)eof_buffer_file(filename, 0);
	eof_music_data_size = file_size_ex(filename);

	if(eof_music_data)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLoading OGG file \"%s\"", filename);
		eof_log(eof_log_string, 1);
		eof_music_track = alogg_create_ogg_from_buffer(eof_music_data, eof_music_data_size);
		if(eof_music_track)
		{
			loaded = 1;
		}
	}
	if(loaded)
	{
		eof_music_length = alogg_get_length_msecs_ogg_ul(eof_music_track);
		eof_truncate_chart(eof_song);	//Remove excess beat markers and update the eof_chart_length variable
	}
	else
	{
		if(eof_music_data)
		{
			free(eof_music_data);
		}
	}

	(void) ustrcpy(eof_loaded_ogg_name,filename);	//Store the loaded OGG filename
	eof_loaded_ogg_name[1023] = '\0';	//Guarantee NULL termination

	return loaded;
}

int eof_load_ogg(char * filename, char function)
{
	char * returnedfn = NULL;
	char * ptr = filename;	//Used to refer to the OGG file that was processed from memory buffer
	char output[1024] = {0};
	char dest_name[256] = {0};
	int loaded = 0;
	char load_silence = 0;
	char * emptystring = "";

	eof_log("eof_load_ogg() entered", 1);

	if(!filename)
		return 0;	//Invalid parameters

	eof_destroy_ogg();
	eof_music_data = (void *)eof_buffer_file(filename, 0);
	eof_music_data_size = file_size_ex(filename);
	strncpy(dest_name, get_filename(filename), sizeof(dest_name) - 1);	//By default, the specified file's name will be stored to the OGG profile
	strncpy(output, filename, sizeof(output) - 1);	//By default, the specified file will be the one loaded
	if(!eof_music_data)
	{	//If the referenced file couldn't be buffered to memory, have the user browse for another file
		(void) replace_filename(output, filename, "", 1024);	//Get the path of the target file's parent directory (the project folder)
		returnedfn = ncd_file_select(0, output, "Select Music File", eof_filter_music_files);
		eof_clear_input();
		if(returnedfn)
		{	//User selected an OGG, WAV or MP3 file, write a suitably named OGG into the chart's destination folder
			ptr = returnedfn;
			(void) replace_filename(output, filename, "", 1024);	//Store the path of the file's parent folder

			if(!eof_audio_to_ogg(returnedfn, output, dest_name, (function == 2 ? 1 : 0)))
			{	//If the file copy or conversion to create guitar.ogg (or a suitably named OGG file if function == 2) succeeded
				(void) replace_filename(output, filename, dest_name, 1024);
				eof_music_data = (void *)eof_buffer_file(output, 0);
				eof_music_data_size = file_size_ex(output);
			}
		}
		else if(function)
		{	//If the user canceled loading audio, and the calling function allows defaulting to second_of_silence.ogg
			load_silence = 1;
			ptr = emptystring;
			get_executable_name(output, 1024);	//Get EOF's executable path
			#ifdef ALLEGRO_MACOSX
				(void) strncat(output, "/Contents/Resources/eof/", sizeof(output) - strlen(output) - 1);
			#endif
			(void) replace_filename(output, output, "second_of_silence.ogg", 1024);
			eof_music_data = (void *)eof_buffer_file(output, 0);
			eof_music_data_size = file_size_ex(output);
		}
	}
	if(eof_music_data)
	{	//If the OGG file was able to buffer to memory
		if(load_silence)
		{	//If EOF failed over to silent audio
			eof_silence_loaded = 1;	//Track this condition
		}
		else
		{
			eof_silence_loaded = 0;

			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tApplying name \"%s\" to OGG profile", dest_name);
			eof_log(eof_log_string, 1);
			if(ogg_profile_name)
			{	//This pointer should refer to the file name string in the default OGG profile
				(void) ustrcpy(ogg_profile_name, dest_name);		//Update the OGG profile with the appropriate file name
				ogg_profile_name = NULL;	//Reset this pointer
			}
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLoading OGG file \"%s\"", output);
		eof_log(eof_log_string, 1);
		eof_music_track = alogg_create_ogg_from_buffer(eof_music_data, eof_music_data_size);
		if(eof_music_track)
		{
			loaded = 1;
			eof_music_length = alogg_get_length_msecs_ogg_ul(eof_music_track);
			eof_truncate_chart(eof_song);	//Remove excess beat markers and update the eof_chart_length variable
			(void) ustrcpy(eof_loaded_ogg_name, output);	//Store the loaded OGG filename
			eof_loaded_ogg_name[1023] = '\0';	//Guarantee NULL termination
		}
	}
	if(!loaded)
	{
		allegro_message("Unable to load OGG file.\n%s\nMake sure your file is a valid OGG file.", ptr);
		if(eof_music_data)
		{
			free(eof_music_data);
		}
		eof_music_data = NULL;
	}

	eof_fix_window_title();
	eof_log("\teof_load_ogg() completed", 2);

	return loaded;
}

int eof_destroy_ogg(void)
{
	int failed = 1;

	eof_log("eof_destroy_ogg() entered", 1);

	if(eof_music_track)
	{
		alogg_destroy_ogg(eof_music_track);
		free(eof_music_data);
		eof_music_track = NULL;
		eof_music_data = NULL;
		failed = 0;
	}

	return !failed;
}

int eof_save_ogg(char * fn)
{
	PACKFILE * fp;

	eof_log("eof_save_ogg() entered", 1);

	if(fn && eof_music_data)
	{
		fp = pack_fopen(fn, "w");
		if(!fp)
		{
			return 0;
		}
		(void) pack_fwrite(eof_music_data, eof_music_data_size, fp);
		(void) pack_fclose(fp);
		return 1;
	}
	return 0;
}

void eof_fix_waveform_graph(void)
{
	eof_log("eof_fix_waveform_graph() entered", 1);

	if(eof_music_paused && eof_waveform)
	{
		eof_destroy_waveform(eof_waveform);
		eof_waveform = eof_create_waveform(eof_loaded_ogg_name,1);	//Generate 1ms waveform data from the current audio file
		if(eof_waveform)
		{
			eof_waveform_menu[0].flags = D_SELECTED;	//Check the Show item in the Song>Waveform graph menu
		}
		else
		{
			eof_display_waveform = 0;
			eof_waveform_menu[0].flags = 0;	//Clear the Show item in the Song>Waveform graph menu
		}
	}
}

void eof_fix_spectrogram(void)
{
	eof_log("eof_fix_spectrogram() entered", 1);

	if(eof_music_paused && eof_spectrogram)
	{
		eof_destroy_spectrogram(eof_spectrogram);
		eof_spectrogram = eof_create_spectrogram(eof_loaded_ogg_name);	//Generate 1ms spectrogram data from the current audio file
		if(eof_spectrogram)
		{
			eof_spectrogram_menu[0].flags = D_SELECTED;	//Check the Show item in the Song>Waveform graph menu
		}
		else
		{
			eof_display_spectrogram = 0;
			eof_spectrogram_menu[0].flags = 0;	//Clear the Show item in the Song>Waveform graph menu
		}
	}
}

void eof_read_keyboard_input(char function)
{
	int ctr;
	#define SHIFT_NUMBER_ARRAY_SIZE 10
	char shift_number_array[SHIFT_NUMBER_ARRAY_SIZE] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
	#define NUMPAD_KEY_ARRAY_SIZE 13
	unsigned char numpad_key_array[NUMPAD_KEY_ARRAY_SIZE] = {KEY_PLUS_PAD, KEY_MINUS_PAD, KEY_ENTER_PAD, KEY_0_PAD, KEY_1_PAD, KEY_2_PAD, KEY_3_PAD, KEY_4_PAD, KEY_5_PAD, KEY_6_PAD, KEY_7_PAD, KEY_8_PAD, KEY_9_PAD};
		//When these keys are pressed, the ASCII value must be ignored because they conflict with other keys on the keyboard

	if(keypressed())
	{	//If a keypress was detected
		eof_key_pressed = 1;
		eof_key_uchar = ureadkey(&eof_key_code);
		if(eof_key_uchar < 255)
		{	//Standard US-English keys
			eof_key_char = tolower(eof_key_uchar);
		}
		else
		{	//Other, ie. non-ASCII
			eof_key_char = eof_key_code + 96;
		}
//		eof_key_code = key_read >> 8;
		eof_key_shifts = key_shifts;
		if(function)
		{	//If the calling function wants to discard the ASCII value for number pad number keys
			for(ctr = 0; ctr < NUMPAD_KEY_ARRAY_SIZE; ctr++)
			{	//For each of the number pad keys used in keyboard controls
				if(eof_key_code == numpad_key_array[ctr])
				{	//If the key pressed is one of those number pad keys
					eof_key_char = 0;	//Destroy the ASCII code
					break;
				}
			}
		}
		else
		{	//Otherwise ensure that the numberpad enter key can be picked up as if it was the regular enter key (ie. in dialog menus)
			if(eof_key_code == KEY_ENTER_PAD)
			{
				eof_key_code = KEY_ENTER;
				eof_key_char = '\r';
			}
		}
		if(KEY_EITHER_CTRL)
		{
			if((eof_key_char >= 1) && (eof_key_char <= 26))
			{	//readkey() converts the ASCII return value CTRL+letter presses to # where A is 1, Z is 26, etc.
				eof_key_char = 'a' + eof_key_char - 1;	//Convert back to ASCII numbering
			}
			if((eof_key_code >= KEY_0) && (eof_key_code <= KEY_9))
			{	//readkey() cannot read an ASCII value for CTRL+#, the scan code has to be interpreted
				eof_key_char = '0' + (eof_key_code - KEY_0);	//Convert to ASCII numbering
			}
		}
		if(KEY_EITHER_SHIFT)
		{	//readkey() returns the ASCII character for SHIFT+# keypresses, while the actual number is still desired
			for(ctr = 0; ctr < SHIFT_NUMBER_ARRAY_SIZE; ctr++)
			{	//For each of the ten number keys
				if(eof_key_char == shift_number_array[ctr])
				{	//If this is the number key that was pressed
					eof_key_char = '0' + ctr;	//Convert back to the ASCII character for the number
					break;
				}
			}
		}
		#ifdef ALLEGRO_MACOSX
			if(function && (eof_key_code == KEY_BACKSPACE))
			{	//If the calling function was NOT a dialog function, and the keyboard input was backspace
				eof_key_code = KEY_DEL;	//Remap it to delete (since Mac keyboards often combine backspace and delete) so it can be used for applicable delete key shortcuts
			}
		#endif
		eof_last_key_char = eof_key_char;
		eof_last_key_code = eof_key_code;
	}
	else
	{
		eof_key_pressed = 0;
		eof_key_uchar = 0;
		eof_key_char = 0;
		eof_key_code = 0;
		eof_key_shifts = key_shifts;
		return;
	}
}

/* Any time we use a key press, we should call this if we don't want that key
 * press to be used again in another part of the code. This should replace the
 * old method of doing key[KEY_*] = 0. */
void eof_use_key(void)
{
	eof_key_pressed = 0;
	eof_key_char = 0;
	eof_key_code = 0;
}

/* read keys that are universally usable */
void eof_read_global_keys(void)
{
//	eof_log("eof_read_global_keys() entered");

	/* exit program (Esc or close button) */
	if((eof_key_code == KEY_ESC) || eof_close_button_clicked)
	{	//Use the key code for Escape instead of the ASCII char, since CTLR+[ triggers the same ASCII character value of 27
		(void) eof_redraw_display();
		eof_menu_file_exit();
		eof_use_key(); //If user cancelled quitting, make sure these keys are cleared
	}

	if(!eof_music_paused)
	{
		return;
	}

	/* activate the menu when ALT is pressed */
	if(KEY_EITHER_ALT)
	{
		eof_log("ALT keypress detected, activating main menu", 1);
		clear_keybuf();
		eof_cursor_visible = 0;
		eof_emergency_stop_music();
		eof_render();
		eof_mouse_x = mouse_x;	//Store the mouse's coordinates
		eof_mouse_y = mouse_y;
		mouse_x = mouse_y = 0;	//Move the mouse out of the way of the menu
		eof_show_mouse(screen);
		mouse_callback = eof_hidden_mouse_callback;	//Install a mouse callback that will restore the mouse's original position if it moves
		(void) eof_popup_dialog(eof_main_dialog, 0);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
	}

	/* switch between window and full screen mode */
/*	if(KEY_EITHER_ALT && key[KEY_ENTER])
	{
		if(eof_windowed == 1)
		{
			eof_windowed = 0;
			if(set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0))
			{
				allegro_message("Can't set up screen!  Error: %s",allegro_error);
				exit(1);
			}
			set_palette(eof_palette);
			ncdfs_use_allegro = 1;
		}
		else
		{
			eof_windowed = 1;
			if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0))
			{
				allegro_message("Can't set up screen!  Error: %s",allegro_error);
				exit(1);
			}
			set_palette(eof_palette);
			ncdfs_use_allegro = 0;
		}
	}
*/

	/* stuff you can only do when a chart is loaded */
	if(eof_song_loaded && eof_song)
	{
		double tempochange;

		/* undo (CTRL+Z) */
		if(KEY_EITHER_CTRL && (eof_key_char == 'z') && (eof_undo_count > 0))
		{
			eof_menu_edit_undo();
			eof_reset_lyric_preview_lines();	//Rebuild the preview lines
		}

		/* redo (CTRL+R) */
		if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && (eof_key_char == 'r') && eof_redo_toggle)
		{
			eof_menu_edit_redo();
			eof_reset_lyric_preview_lines();	//Rebuild the preview lines
		}

		/* decrease tempo by 1BPM (-) */
		/* decrease tempo by .1BPM (SHIFT+-) */
		/* decrease tempo by .01BPM (SHIFT+CTRL+-) */
		if(eof_key_code == KEY_MINUS)	//Use the scan code because CTRL+- cannot be reliably detected via ASCII value
		{
			unsigned long ctr;
			int updated = 0;

			if(KEY_EITHER_SHIFT)
			{	//If SHIFT is held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(KEY_EITHER_CTRL)
				{	//If both SHIFT and CTRL are held
					tempochange = -0.01;
				}
				else
				{	//If only SHIFT is held
					tempochange = -0.1;
				}
				eof_menu_beat_adjust_bpm(tempochange);
				eof_use_key();
				updated = 1;
			}
			else
			{	//If SHIFT is not held
				if(!KEY_EITHER_CTRL)
				{	//If the CTRL key is not held (CTRL+- is reserved for decrement pro guitar fret value
					tempochange = -1.0;
					eof_menu_beat_adjust_bpm(tempochange);

					eof_use_key();
					updated = 1;
				}
			}
			if(updated && eof_song->tags->highlight_unsnapped_notes)
			{	//If one of the valid shortcuts for this command (ie. not CTRL+-) was intercepted, and unsnapped notes are to be highlighted
				for(ctr = 1; ctr < eof_song->tracks; ctr++)
				{	//For each track
					eof_track_remove_highlighting(eof_song, ctr, 1);	//Remove existing temporary highlighting from the track
					eof_song_highlight_non_grid_snapped_notes(eof_song, ctr);	//Re-create the non grid snapped highlighting as appropriate
				}
			}
		}

		/* increase tempo by 1BPM (+) */
		/* increase tempo by .1BPM (SHIFT+(plus)) */
		/* increase tempo by .01BPM (SHIFT+CTRL+(plus)) */
		if(eof_key_code == KEY_EQUALS)	//Use the scan code because CTRL+= cannot be reliably detected via ASCII value
		{
			unsigned long ctr;
			int updated = 0;

			if(KEY_EITHER_SHIFT)
			{	//If SHIFT is held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(KEY_EITHER_CTRL)
				{	//If both SHIFT and CTRL are held
					tempochange = 0.01;
				}
				else
				{	//If only the SHIFT key is held
					tempochange = 0.1;
				}
				eof_menu_beat_adjust_bpm(tempochange);
				eof_use_key();
				updated = 1;
			}
			else
			{	//The SHIFT key is not held
				if(!KEY_EITHER_CTRL)
				{	//If the CTRL key is not held (CTRL++ is reserved for increment pro guitar fret value)
					tempochange = 1.0;
					eof_menu_beat_adjust_bpm(tempochange);
					eof_use_key();
					updated = 1;
				}
			}
			if(updated && eof_song->tags->highlight_unsnapped_notes)
			{	//If one of the valid shortcuts for this command (ie. not CTRL++) was intercepted, and unsnapped notes are to be highlighted
				for(ctr = 1; ctr < eof_song->tracks; ctr++)
				{	//For each track
					eof_track_remove_highlighting(eof_song, ctr, 1);	//Remove existing temporary highlighting from the track
					eof_song_highlight_non_grid_snapped_notes(eof_song, ctr);	//Re-create the non grid snapped highlighting as appropriate
				}
			}
		}
	}//If a song is loaded

	/* save (F2 or CTRL+S) */
	if(((eof_key_code == KEY_F2) && !KEY_EITHER_CTRL && !KEY_EITHER_SHIFT) || ((eof_key_char == 's') && KEY_EITHER_CTRL && !KEY_EITHER_SHIFT))
	{
		eof_menu_file_save();
		eof_use_key();
	}

	/* find next catalog match (F3) */
	if(eof_key_code == KEY_F3)
	{
		if(!KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
		{	//Neither CTRL nor SHIFT are held
			eof_menu_catalog_find_next();
			eof_use_key();
		}
	}

	/* find previous catalog match (SHIFT+F3) */
	if((eof_key_code == KEY_F3) && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
	{
		eof_shift_used = 1;	//Track that the SHIFT key was used
		eof_menu_catalog_find_prev();
		eof_use_key();
	}

	/* load chart (CTRL+O) */
	if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && (eof_key_char == 'o'))
	{	//File>Load
		clear_keybuf();
		eof_menu_file_load();
		eof_use_key();
	}

	/* new chart (CTRL+N) */
	if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && (eof_key_char == 'n'))
	{
		clear_keybuf();
		eof_menu_file_new_wizard();
		eof_use_key();
	}

	if(!KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
	{	//If neither CTRL nor SHIFT are held
	/* show help */
		if(eof_key_code == KEY_F1)
		{
			clear_keybuf();
			(void) eof_menu_help_keys();
			eof_use_key();
		}

	/* toggle tech view (F4) */
		else if(eof_key_code == KEY_F4)
		{
			(void) eof_menu_track_toggle_tech_view();
			key[KEY_F4] = 0;
		}

	/* show waveform graph (F5) */
		else if(eof_key_code == KEY_F5)
		{
			if(!KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
			{	//If neither CTRL nor SHIFT are held
				clear_keybuf();
				(void) eof_menu_song_waveform();
				eof_use_key();
			}
		}

	/* midi import (F6) */
		else if(eof_key_code == KEY_F6)
		{	//Launch MIDI import
			clear_keybuf();
			(void) eof_menu_file_midi_import();
			eof_use_key();
		}

	/* feedback import (F7) */
		else if(eof_key_code == KEY_F7)
		{	//Launch Feedback chart import
			clear_keybuf();
			(void) eof_menu_file_feedback_import();
			eof_use_key();
		}

	/* lyric import (F8) */
		else if(eof_key_code == KEY_F8)
		{
			clear_keybuf();
			(void) eof_menu_file_lyrics_import();
			eof_use_key();
		}

	/* song properties (F9) */
		else if(eof_key_code == KEY_F9)
		{
			clear_keybuf();
			(void) eof_menu_song_properties();
			eof_use_key();
		}

	/* settings (F10) */
		else if(eof_key_code == KEY_F10)
		{
			clear_keybuf();
			(void) eof_menu_file_settings();
			eof_use_key();
		}

	/* preferences (F11) */
		else if(eof_key_code == KEY_F11)
		{
			clear_keybuf();
			(void) eof_menu_file_preferences();
			eof_use_key();
		}

	/* import Guitar Pro (F12) */
		else if(eof_key_code == KEY_F12)
		{
			clear_keybuf();
			(void) eof_menu_file_gp_import();
			eof_use_key();
		}
	}//If neither CTRL nor SHIFT are held
}

void eof_lyric_logic(void)
{
//	eof_log("eof_lyric_logic() entered");

	int note[7] = {0, 2, 4, 5, 7, 9, 11};
	int bnote[7] = {1, 3, 0, 6, 8, 10, 0};
	unsigned long i, k;
	unsigned long tracknum;
	int eof_scaled_mouse_x = mouse_x, eof_scaled_mouse_y = mouse_y;	//Rescaled mouse coordinates that account for the x2 zoom display feature

	if(eof_song == NULL)	//Do not allow lyric processing to occur if no song is loaded
		return;

	if(!eof_vocals_selected)
		return;

	eof_hover_key = -1;
	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_scaled_mouse_x = mouse_x / 2;
		eof_scaled_mouse_y = mouse_y / 2;
	}

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_music_paused)
	{
		if((eof_scaled_mouse_x >= eof_window_3d->x) && (eof_scaled_mouse_x < eof_window_3d->x + eof_window_3d->w) && (eof_scaled_mouse_y >= eof_window_3d->y + eof_window_3d->h - eof_screen_layout.lyric_view_key_height))
		{
			/* check for black key */
			if(eof_scaled_mouse_y < eof_window_3d->y + eof_window_3d->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height)
			{
				for(i = 0; i < 28; i++)
				{
					k = i % 7;
					if((k == 0) || (k == 1) || (k == 3) || (k == 4) || (k == 5))
					{
						if((eof_scaled_mouse_x - eof_window_3d->x >= i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width) && (eof_scaled_mouse_x - eof_window_3d->x <= (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1))
						{
							eof_hover_key = EOF_LYRIC_PITCH_MIN + (i / 7) * 12 + bnote[k];
							break;
						}
					}
				}
			}

			/* otherwise, white key */
			if(eof_hover_key < 0)
			{
				for(i = 0; i < 29; i++)
				{
					k = i % 7;
					if((eof_scaled_mouse_x - eof_window_3d->x >= i * eof_screen_layout.lyric_view_key_width) && (eof_scaled_mouse_x - eof_window_3d->x < i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width - 1))
					{
						eof_hover_key = EOF_LYRIC_PITCH_MIN + (i / 7) * 12 + note[k];
						break;
					}
				}
			}
			if(eof_hover_key >= 0)
			{
				if(!eof_full_screen_3d && (mouse_b & 1))
				{
					if(KEY_EITHER_CTRL && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics))
					{
						if(eof_count_selected_notes(NULL) == 1)
						{
							eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);
							eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note = eof_hover_key;
						}
					}
					if(eof_hover_key != eof_last_tone)
					{
						eof_mix_play_note(eof_hover_key);
						eof_last_tone = eof_hover_key;
					}
				}
				else
				{
					eof_last_tone = -1;
				}
				if(!eof_full_screen_3d && ((mouse_b & 2) || (eof_key_code == KEY_INSERT)))
				{	//Right click or Insert key changes the mini piano visible area
					eof_vocals_offset = eof_hover_key - eof_screen_layout.vocal_view_size / 2;
					if(KEY_EITHER_CTRL)
					{
						eof_vocals_offset = (eof_hover_key / 12) * 12;
					}
					if(eof_vocals_offset < EOF_LYRIC_PITCH_MIN)
					{
						eof_vocals_offset = EOF_LYRIC_PITCH_MIN;
					}
					else if(eof_vocals_offset > EOF_LYRIC_PITCH_MAX - eof_screen_layout.vocal_view_size + 1)
					{
						eof_vocals_offset = EOF_LYRIC_PITCH_MAX - eof_screen_layout.vocal_view_size + 1;
					}
				}
			}
		}
		else
		{
			if(eof_selection.current < eof_song->vocal_track[tracknum]->lyrics)
			{
				eof_hover_key = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note;
			}
		}
	}

	if(eof_music_catalog_playback || !eof_music_paused)
	{	//If the chart or fret catalog is playing
		if(eof_hover_note >= 0)
		{	//If there is a hover note, define the hover key
			eof_hover_key = eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note;
		}
	}
}

void eof_note_logic(void)
{
	int eof_scaled_mouse_x = mouse_x, eof_scaled_mouse_y = mouse_y;	//Rescaled mouse coordinates that account for the x2 zoom display feature
//	eof_log("eof_note_logic() entered");

	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_scaled_mouse_x = mouse_x / 2;
		eof_scaled_mouse_y = mouse_y / 2;
	}

	if(eof_display_catalog && (eof_scaled_mouse_x >= 0) && (eof_scaled_mouse_x < 90) && (eof_scaled_mouse_y > 40 + eof_window_info->y) && (eof_scaled_mouse_y < 40 + 18 + eof_window_info->y))
	{
		eof_cselected_control = eof_scaled_mouse_x / 30;
		if(!eof_full_screen_3d && (mouse_b & 1))
		{
			eof_blclick_released = 0;
		}
		if(!(mouse_b & 1))
		{
			if(!eof_blclick_released)
			{
				eof_blclick_released = 1;
				if(eof_cselected_control == 0)
				{
					(void) eof_menu_catalog_previous();
				}
				else if(eof_cselected_control == 1)
				{
					eof_catalog_play();
				}
				else if(eof_cselected_control == 2)
				{
					(void) eof_menu_catalog_next();
				}
			}
		}
	}
	else
	{
		eof_cselected_control = -1;
	}
	if((eof_scaled_mouse_y >= eof_window_info->y) && (eof_scaled_mouse_y < eof_window_info->y + 12) && (eof_scaled_mouse_x >= 0) && (eof_scaled_mouse_x < (eof_display_catalog ? text_length(font, "Fret Catalog") : text_length(font, "Information Panel"))))
	{
		if(!eof_full_screen_3d && (mouse_b & 1))
		{
			eof_blclick_released = 0;
		}
		if(!(mouse_b & 1))
		{
			if(!eof_blclick_released)
			{
				eof_blclick_released = 1;
				(void) eof_menu_catalog_show();
			}
		}
		eof_info_color = eof_color_green;
	}
	else
	{
		eof_info_color = eof_color_white;
	}
}

void eof_logic(void)
{
//	eof_log("eof_logic() entered");

	eof_read_keyboard_input(1);	//Drop ASCII values for number pad key presses
	eof_read_global_keys();

	if(eof_emergency_stop)
	{	//If the switch out callback function was triggered, stop playback immediately
		eof_emergency_stop_music();
		eof_emergency_stop = 0;
	}

	/* see if we need to activate the menu */
	if((mouse_y < eof_image[EOF_IMAGE_MENU]->h) && (mouse_b & 1))
	{
		eof_cursor_visible = 0;
		eof_emergency_stop_music();
		eof_render();
		clear_keybuf();
		(void) eof_popup_dialog(eof_main_dialog, 0);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
	}

	eof_last_note = EOF_MAX_NOTES;

	get_mouse_mickeys(&eof_mickeys_x, &eof_mickeys_y);
	if(eof_song_loaded)
	{
		eof_read_editor_keys();
	}
	if(eof_music_catalog_playback)
	{	//If the fret catalog is playing
		eof_music_catalog_pos += 10;
		if(eof_music_catalog_pos - eof_av_delay > eof_song->catalog->entry[eof_selected_catalog_entry].end_pos)
		{
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_pos);
		}
	}
	else if(!eof_music_paused)
	{	//If the chart is playing
		if(eof_smooth_pos)
		{
			if(eof_playback_speed != eof_mix_speed)
			{
				if(eof_mix_speed == 1000)		//Force full speed playback
					eof_music_pos += 10;
				else if(eof_mix_speed == 500)	//Force half speed playback
					eof_music_pos += 5;
				else if(eof_mix_speed == 250)	//Force quarter speed playback
				{
					if(eof_frame % 2 == 0)
					{	//Round up every even frame
						eof_music_pos += 3;
					}
					else
					{	//Round down every odd frame
						eof_music_pos += 2;
					}
				}
				else							//Something unexpected is going on
					eof_playback_speed = eof_mix_speed;
			}
			else
			{
				switch(eof_playback_speed)
				{
					case 1000:
					{
						eof_music_pos += 10;
						break;
					}
					case 750:
					{
						if(eof_frame % 2 == 0)
						{	//Round up every even frame
							eof_music_pos += 8;
						}
						else
						{	//Round down every odd frame
							eof_music_pos += 7;
						}
						break;
					}
					case 500:
					{
						eof_music_pos += 5;
						break;
					}
					case 250:
					{
						if(eof_frame % 2 == 0)
						{	//Round up every even frame
							eof_music_pos += 3;
						}
						else
						{	//Round down every odd frame
							eof_music_pos += 2;
						}
						break;
					}
					default:	//For custom playback rate
					{
						if(eof_frame % 2 == 0)	//If eof_frame is even
						{
							eof_music_pos += (eof_playback_speed / 100.0 + 0.5);	//Round up
						}
						else
						{
							eof_music_pos += eof_playback_speed / 100;	//Round down
						}
						break;
					}
				}
			}
		}
		else
		{
			eof_music_pos = eof_music_actual_pos;
		}
		if(eof_play_selection && (eof_music_pos - eof_av_delay > eof_music_end_pos))
		{
			eof_music_paused = 1;
			eof_music_pos = eof_music_rewind_pos;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_pos);
			if(key[KEY_S])
			{	//If S is still being held down, replay the note selection
				eof_music_play(0);
			}
		}
	}//If the chart is playing
	if(eof_song_loaded)
	{
		if(eof_vocals_selected)
		{
			eof_vocal_editor_logic();
		}
		else
		{
			eof_editor_logic();
		}
	}
	eof_note_logic();
	if(eof_vocals_selected)
	{
		eof_lyric_logic();
	}
	if(eof_song_loaded)
	{
		eof_find_lyric_preview_lines();
	}
	eof_frame++;
}

static char eof_tone_name_buffer[16] = {0};
char * eof_get_tone_name(int tone)
{
//	eof_log("eof_get_tone_name() entered");

	char * note_name[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	char * note_name_flats[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
	char ** array_to_use=note_name;	//Default to using the regular note name array

	if(eof_display_flats)				//If user enabled the feature to display flat notes
		array_to_use=note_name_flats;	//Use the other array

	(void) snprintf(eof_tone_name_buffer, sizeof(eof_tone_name_buffer) - 1, "%s%d", array_to_use[tone % 12], tone / 12 - 1);
	return eof_tone_name_buffer;
}

void eof_render_extended_ascii_fonts(void)
{
	unsigned ch;
	char buffer[50];
	int x = 2, ctr;

	eof_allocate_ucode_table();
	for(ch = 128, ctr = 0; ch < 256; ch++, ctr++)
	{
		if(ctr == 19)
		{
			x += 45;
			ctr = 0;
		}
		sprintf(buffer, "%u:", ch);
		buffer[4] = (unsigned char)ch;
		buffer[5] = '\0';
		eof_convert_from_extended_ascii(buffer, 200);

		textprintf_ex(eof_window_info->screen, font, x, 12*ctr,  eof_color_white, eof_color_black, "%s", buffer);
	//	textprintf_ex(eof_window_info->screen, eof_allegro_font, x, 12*ctr,  eof_color_white, eof_color_black, "%s", buffer);
	//	textprintf_ex(eof_window_info->screen, eof_mono_font, x, 12*ctr,  eof_color_white, eof_color_black, "%s", buffer);
	}
	eof_free_ucode_table();

	return;
}

void eof_render_info_window(void)
{
	int opacity = 1;			//Unless full screen 3D preview is in effect, the Info panel will be opaque

	if(eof_disable_info_panel)	//If the user disabled the info panel's rendering
		return;					//Return immediately without rendering anything

	if(eof_display_second_piano_roll)	//If a second piano roll is being rendered instead of the 3D preview
		return;							//Return immediately

	if(eof_full_screen_3d)
	{	//If full screen 3D view is in effect
		eof_window_info = eof_window_note_upper_left;	//Render info panel at the top left of EOF's program window
		opacity = 0;
	}
	else
	{
		eof_window_info = eof_window_note_lower_left;	//Render info panel at the lower left of EOF's program window
		if(!eof_background)
		{	//If a background image was NOT loaded
			clear_to_color(eof_window_info->screen, eof_color_gray);	//Clear the info panel portion of the screen
		}
	}

	if(eof_display_catalog && eof_song->catalog->entries)
	{	//If show catalog is selected and there's at least one entry
		eof_render_fret_catalog_window();	//Render the fret catalog
		return;
	}

	if(!eof_info_panel)
	{	//If the info panel isn't properly initialized
		eof_disable_info_panel = 1;		//Properly disable the panel
		return;
	}

	eof_info_panel->window = eof_window_info;
	eof_render_text_panel(eof_info_panel, opacity);
}

void eof_render_fret_catalog_window(void)
{
	unsigned long i;
	int pos;
	int lpos, npos;
	unsigned long tracknum;
	int xcoord;
	unsigned long numlanes;				//The number of fretboard lanes that will be rendered
	char temp[1024] = {0};
	unsigned long notepos;

	if(eof_disable_info_panel)	//If the user disabled the info panel's rendering
		return;					//Return immediately without rendering anything

	if(eof_display_second_piano_roll)	//If a second piano roll is being rendered instead of the info panel
		return;							//Return immediately

	if(eof_catalog_full_width)
	{	//If the fret catalog is allowed to render the full width of the program window
		set_clip_rect(eof_window_info->screen, 0, 0, eof_window_info->screen->w, eof_window_info->screen->h);
	}
	else
	{	//Otherwise enforce clipping around the fret catalog if it isn't meant to render into other panels' space
		unsigned long width = EOF_SCREEN_PANEL_WIDTH;	//By default, give it one panel width

		if(!eof_enable_notes_panel)
		{	//If the notes panel is not being displayed
			if(eof_disable_3d_rendering)
			{	//If the 3D preview is also disabled
				width = eof_window_info->screen->w;	//Give the fret catalog the entire program window width
			}
			else
			{
				width = eof_screen->w - EOF_SCREEN_PANEL_WIDTH;	//Give it the entire program window width, minus one panel for the 3D preview
			}
		}
		set_clip_rect(eof_window_info->screen, 0, 0, width, eof_window_info->screen->h);
	}

	numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	if(eof_display_catalog && eof_song->catalog->entries)
	{//If show catalog is selected and there's at least one entry
		eof_set_2D_lane_positions(eof_selected_track);	//Update the ychart[] array
		if(eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track] == NULL)
			return;	//If this is NULL for some reason (broken track array or corrupt catalog entry, abort rendering it

		tracknum = eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->tracknum;	//Information about the active fret catalog entry is going to be displayed
		textprintf_ex(eof_window_info->screen, font, 2, 0, eof_info_color, -1, "Fret Catalog");
		textprintf_ex(eof_window_info->screen, font, 2, 12, eof_color_white, -1, "-------------------");
		if(eof_song->catalog->entry[eof_selected_catalog_entry].name[0] != '\0')
		{	//If the active fret catalog has a defined name
			textprintf_ex(eof_window_info->screen, font, 2, 24,  eof_color_white, -1, "Entry: %lu of %lu: %s", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries, eof_song->catalog->entry[eof_selected_catalog_entry].name);
		}
		else
		{
			textprintf_ex(eof_window_info->screen, font, 2, 24,  eof_color_white, -1, "Entry: %lu of %lu", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries);
		}
		if((eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		{	//If the catalog entry is a pro guitar note and the active track is a legacy track
			(void) snprintf(temp, sizeof(temp) - 1, "Would paste from \"%s\" as:",eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->name);
			textout_ex(eof_window_info->screen, font, temp, 2, 53, eof_color_white, -1);
		}

		if(eof_cselected_control < 0)
		{
			draw_sprite(eof_window_info->screen, eof_image[EOF_IMAGE_CCONTROLS_BASE], 0, 40);
		}
		else
		{
			draw_sprite(eof_window_info->screen, eof_image[EOF_IMAGE_CCONTROLS_0 + eof_cselected_control], 0, 40);
		}

		// render catalog entry
		if(eof_song->catalog->entries > 0)
		{
			char emptystring[2] = "", *difficulty_name = emptystring;	//The empty string will be 2 characters, so that the first can (used to define populated status of each track) be skipped
			char diff_string[12] = {0};				//Used to generate the string for a numbered difficulty (prefix with an extra space, since the string rendering skips the populated status character used for normal tracks
			int zoom = eof_av_delay / eof_zoom;	//AV delay compensated for zoom level

			pos = eof_music_catalog_pos / eof_zoom;
			if(pos < 140)
			{
				lpos = 20;
			}
			else
			{
				lpos = 20 - (pos - 140);
			}
			// draw fretboard area
			rectfill(eof_window_info->screen, 0, EOF_EDITOR_RENDER_OFFSET + 25, eof_window_info->w - 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_black);
			// draw fretboard area
			for(i = 0; i < numlanes; i++)
			{
				if(!i || (i + 1 >= numlanes))
				{	//Ensure the top and bottom lines extend to the left of the piano roll
					hline(eof_window_info->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[i], lpos + (eof_chart_length) / eof_zoom, eof_color_white);
				}
				else if(eof_song->catalog->entry[eof_selected_catalog_entry].track != EOF_TRACK_VOCALS)
				{	//Otherwise, if not drawing the vocal editor, draw the other fret lines from the first beat marker to the end of the chart
					hline(eof_window_info->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[i], lpos + (eof_chart_length) / eof_zoom, eof_color_white);
				}
			}
			vline(eof_window_info->screen, lpos + (eof_chart_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_white);

			// render information about the entry
			if(eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->track_format != EOF_VOCAL_TRACK_FORMAT)
			{	//If the catalog entry is not from a vocal track, determine the name of the active difficulty
				if(eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
				{	//If this track is not limited to 5 difficulties
					snprintf(diff_string, sizeof(diff_string) - 1, " Diff: %u", eof_song->catalog->entry[eof_selected_catalog_entry].type);
					difficulty_name = diff_string;
				}
				else if(eof_song->catalog->entry[eof_selected_catalog_entry].track == EOF_TRACK_DANCE)
				{	//The dance track has different difficulty names
					difficulty_name = eof_dance_tab_name[(int)eof_song->catalog->entry[eof_selected_catalog_entry].type];
				}
				else
				{	//All other tracks use the same difficulty names
					difficulty_name = eof_note_type_name[(int)eof_song->catalog->entry[eof_selected_catalog_entry].type];
				}
			}
			textprintf_ex(eof_window_info->screen, font, 2, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 10, eof_color_white, -1, "%s , %s , %lums - %lums", eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->name, difficulty_name + 1, eof_song->catalog->entry[eof_selected_catalog_entry].start_pos, eof_song->catalog->entry[eof_selected_catalog_entry].end_pos);
			// draw beat lines
			if(pos < 140)
			{
				npos = 20;
			}
			else
			{
				npos = 20 - ((pos - 140));
			}
			for(i = 0; i < eof_song->beats; i++)
			{
				xcoord = npos + eof_song->beat[i]->pos / eof_zoom;
				if(xcoord >= eof_window_info->screen->w)
				{	//If this beat line would render off the edge of the screen
					break;	//Stop rendering them
				}
				if(xcoord >= 0)
				{	//If this beat line would render visibly, render it
					vline(eof_window_info->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10, eof_color_white);
				}
			}
			if(eof_song->catalog->entry[eof_selected_catalog_entry].track == EOF_TRACK_VOCALS)
			{	//If drawing a vocal catalog entry
				// clear lyric text area
				rectfill(eof_window_info->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_info->w - 1, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, eof_color_black);
				hline(eof_window_info->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, lpos + (eof_chart_length) / eof_zoom, eof_color_white);

				for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
				{	//For each lyric
					if(eof_song->vocal_track[tracknum]->lyric[i]->pos > eof_song->catalog->entry[eof_selected_catalog_entry].end_pos)
					{	//If this lyric is after the end of the catalog entry
						break;	//Stop processing lyrics
					}
					if(eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos)
					{	//If this lyric is in the catalog entry, render it
						if(eof_lyric_draw(eof_song->vocal_track[tracknum]->lyric[i], i == eof_hover_note_2 ? 2 : 0, eof_window_info) > 0)
							break;	//Break if the function indicated that the lyric was rendered beyond the clip window
					}
				}
			}//If drawing a vocal catalog entry
			else
			{	//If drawing a non vocal catalog entry
				for(i = 0; i < eof_get_track_size(eof_song, eof_song->catalog->entry[eof_selected_catalog_entry].track); i++)
				{	//For each note in the entry's track
					notepos = eof_get_note_pos(eof_song, eof_song->catalog->entry[eof_selected_catalog_entry].track, i);	//Get the note's position
					if(notepos > eof_song->catalog->entry[eof_selected_catalog_entry].end_pos)
					{	//If this note is after the end of the catalog entry
						break;	//Stop processing notes
					}
					if((eof_song->catalog->entry[eof_selected_catalog_entry].type == eof_get_note_type(eof_song, eof_song->catalog->entry[eof_selected_catalog_entry].track, i)) &&
					  (notepos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos))
					{	//If this note is the same difficulty as that from where the catalog entry was taken, and is in the catalog entry
						(void) eof_note_draw(eof_song->catalog->entry[eof_selected_catalog_entry].track, i, i == eof_hover_note_2 ? 2 : 0, eof_window_info);
					}
				}
			}//If drawing a non vocal catalog entry
			// draw the current position
			if(pos > zoom)
			{
				if(pos < 140)
				{
					vline(eof_window_info->screen, 20 + pos - zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
				}
				else
				{
					vline(eof_window_info->screen, 20 + 140 - zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
				}
			}
		}//if(eof_song->catalog->entries > 0)
	}//If show catalog is selected
}

void eof_display_info_panel(void)
{
	eof_disable_info_panel = 1 - eof_disable_info_panel;	//Toggle the info panel on/off

	if(eof_info_panel)
	{	//If an Info panel instance already exists
		eof_destroy_text_panel(eof_info_panel);
		eof_info_panel = NULL;
	}

	if(!eof_disable_info_panel)
	{	//If the Info panel was just enabled
		if(eof_validate_temp_folder())
		{	//Ensure the correct working directory and presence of the temporary folder
			eof_log("\tCould not validate working directory and temp folder", 1);
			eof_log_cwd();
			eof_disable_info_panel = 1;

			return;
		}
		eof_info_panel = eof_create_text_panel("info.panel.txt", 1);	//Reload the Info panel, recover the panel file from eof.dat if necessary
		if(!eof_info_panel)
		{
			allegro_message("Could not open Information panel");
			eof_disable_info_panel = 1;

			return;
		}
	}

	return;
}

#define MAX_LYRIC_PREVIEW_LENGTH 512
void eof_render_lyric_preview(BITMAP * bp)
{
//	eof_log("eof_render_lyric_preview() entered");

	unsigned long currentlength = 0;	//Used to track the length of the preview line being built
	unsigned long lyriclength = 0;		//The length of the lyric being added
	char *tempstring = NULL;			//The code to render in green needs special handling to suppress the / character
	EOF_PHRASE_SECTION *linenum = NULL;	//Used to find the background color to render the lyric lines in (green for overdrive, otherwise transparent)
	int bgcol1 = -1, bgcol2 = -1;

	char lline[2][MAX_LYRIC_PREVIEW_LENGTH+1] = {{0}};
	unsigned long i, x;
	int offset = -1;
	int space;	//Track the spacing for lyric preview, taking pitch shift and grouping logic into account

	if(!bp)
	{
		return;
	}

//If the active track is being overridden by a stored MIDI track
	if(eof_track_overridden_by_stored_MIDI_track(eof_song, eof_selected_track))
	{
		textout_centre_ex(bp, font, "A stored MIDI track overrides this track", bp->w / 2, 20, eof_color_red, bgcol1);
		textout_centre_ex(bp, font, "View Song>Manage Stored MIDI tracks", bp->w / 2, 36, eof_color_red, bgcol2);
		return;
	}

//Build both lyric preview lines
	for(x = 0; x < 2; x++)
	{	//For each of the two lyric preview lines to build
		space = 0;			//The first lyric will have no space inserted in front of it
		currentlength = 0;	//Reset preview line length counter

		linenum = eof_find_lyric_line(eof_preview_line_lyric[x]);	//Find the line structure representing this lyric preview
		if(linenum && (linenum->flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE))	//If this line is overdrive
		{
			if(x == 0)	//This is the first preview line
				bgcol1 = makecol(64, 128, 64);	//Render the line's text with an overdrive green background
			else		//This is the second preview line
				bgcol2 = makecol(64, 128, 64);	//Render the line's text with an overdrive green background
		}

		for(i = eof_preview_line_lyric[x]; i < eof_preview_line_end_lyric[x]; i++)
		{	//For each lyric in the preview line
			lyriclength = ustrlen(eof_song->vocal_track[0]->lyric[i]->text);	//This value will be used multiple times

		//Perform grouping logic
			if((eof_song->vocal_track[0]->lyric[i]->text[0] != '+'))
			{	//If the lyric is not a pitch shift
				if(space)
				{	//If space is nonzero, append a space if possible
					if(currentlength+1 <= MAX_LYRIC_PREVIEW_LENGTH)
					{	//If appending a space would NOT cause an overflow
						(void) ustrcat(lline[x], " ");
						currentlength++;	//Track the length of this preview line
					}
					else
						break;				//Stop building this line's preview
				}
			}

			if(ustrchr(eof_song->vocal_track[0]->lyric[i]->text, '-') || ustrchr(eof_song->vocal_track[0]->lyric[i]->text, '='))
			{	//If the lyric contains a hyphen or an equal sign
				space = 0;	//The next syllable will group with this one
			}
			else
			{
				space = 1;	//The next syllable will not group with this one, and will be separated by space
			}

			if(!x && !eof_music_paused)	//Only perform this logic for the first lyric preview line
			{
				if(i == eof_hover_lyric)
				{
					offset = text_length(font, lline[x]);
				}
			}

		//Perform bounds checking before appending string
			if(currentlength+lyriclength > MAX_LYRIC_PREVIEW_LENGTH)	//If appending this lyric would cause an overflow
				break;													//Stop building this line's preview

		//Append string
			(void) ustrcat(lline[x], eof_song->vocal_track[0]->lyric[i]->text);
			currentlength += lyriclength;									//Track the length of this preview line

		//Truncate a '/' character off the end of the lyric line if it exists (TB:RB notation for a forced line break)
			if(ugetat(lline[x], ustrlen(lline[x]) - 1) == '/')	//If the string ends in a forward slash
				usetat(lline[x], ustrlen(lline[x]) - 1, '\0');
		}//For each lyric in the preview line
	}//For each of the two lyric preview lines to build

	textout_centre_ex(bp, font, lline[0], bp->w / 2, 20, eof_color_white, bgcol1);
	textout_centre_ex(bp, font, lline[1], bp->w / 2, 36, eof_color_white, bgcol2);
	if((offset >= 0) && (eof_hover_lyric >= 0))
	{
		if(eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text[strlen(eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text)-1] == '/')
		{	//If the at-playback position lyric ends in a forward slash, make a copy with the slash removed and display it instead
			tempstring = malloc(ustrlen(eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text) + 1);
			if(tempstring == NULL)	//If there wasn't enough memory to copy this string...
				return;
			(void) ustrcpy(tempstring,eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text);
			tempstring[ustrlen(tempstring)-1] = '\0';
			textout_ex(bp, font, tempstring, bp->w / 2 - text_length(font, lline[0]) / 2 + offset, 20, eof_color_green, -1);
			free(tempstring);
		}
		else	//Otherwise, display the regular string instead
			textout_ex(bp, font, eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text, bp->w / 2 - text_length(font, lline[0]) / 2 + offset, 20, eof_color_green, -1);
	}
}

void eof_render_lyric_window(void)
{
//	eof_log("eof_render_lyric_window() entered");

	int i;
	int k, n;
	int kcol;
	int kcol2;
	int note[7] = {0, 2, 4, 5, 7, 9, 11};
	int bnote[7] = {1, 3, 0, 6, 8, 10, 0};

	if(eof_display_catalog && eof_catalog_full_width)
	{	//If the fret catalog is visible and it's configured to display at full width
		return;	//Don't draw the lyric window
	}

	if(eof_display_second_piano_roll)	//If a second piano roll is being rendered instead of the 3D preview
		return;							//Return immediately

	clear_to_color(eof_window_3d->screen, eof_color_gray);

	/* render the 29 white keys */
	for(i = 0; i < 29; i++)
	{
		n = (i / 7) * 12 + note[i % 7] + EOF_LYRIC_PITCH_MIN;
		if(n == eof_hover_key)
		{
			if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				kcol = eof_color_green;
				kcol2 = makecol(0, 192, 0);
			}
			else
			{
				kcol = makecol(0, 160, 0);
				kcol2 = makecol(0, 96, 0);
			}
			textout_centre_ex(eof_window_3d->screen, font, eof_get_tone_name(eof_hover_key), i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2, eof_window_3d->h - eof_screen_layout.lyric_view_key_height - text_height(font), eof_color_white, eof_color_black);
		}
		else
		{
			if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				kcol = eof_color_white;
				kcol2 = eof_color_silver;
			}
			else
			{
				kcol = eof_color_dark_silver;
				kcol2 = makecol(96, 96, 96);
			}
		}
		rectfill(eof_window_3d->screen, i * eof_screen_layout.lyric_view_key_width, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width - 1, eof_window_3d->screen->h, kcol);
		vline(eof_window_3d->screen, i * eof_screen_layout.lyric_view_key_width, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, eof_window_3d->screen->h, kcol2);
	}

	/* render black keys over white keys */
	for(i = 0; i < 28; i++)
	{
		n = (i / 7) * 12 + bnote[i % 7] + EOF_LYRIC_PITCH_MIN;
		k = n % 12;
		if((k != 1) && (k != 3) && (k != 6) && (k != 8) && (k != 10))
			continue;	//If this isn't a black key, skip this rendering logic

		if(n == eof_hover_key)
		{
			if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				kcol = eof_color_green;
				kcol2 = makecol(0, 192, 0);
			}
			else
			{
				kcol = makecol(0, 160, 0);
				kcol2 = makecol(0, 96, 0);
			}
			textout_centre_ex(eof_window_3d->screen, font, eof_get_tone_name(eof_hover_key), i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width + eof_screen_layout.lyric_view_bkey_width / 2, eof_window_3d->h - eof_screen_layout.lyric_view_key_height - text_height(font), eof_color_white, eof_color_black);
		}
		else
		{
			if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				kcol = makecol(48, 48, 48);
				kcol2 = makecol(24, 24, 24);
			}
			else
			{
				kcol = makecol(16, 16, 16);
				kcol2 = eof_color_black;
			}
		}
		rectfill(eof_window_3d->screen, i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height, kcol);
		vline(eof_window_3d->screen, (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height, kcol2);
	}
	eof_render_lyric_preview(eof_window_3d->screen);

	//Draw a border around the edge of the lyric window
	rect(eof_window_3d->screen, 0, 0, eof_window_3d->w - 1, eof_window_3d->h - 1, eof_color_dark_silver);
	rect(eof_window_3d->screen, 1, 1, eof_window_3d->w - 2, eof_window_3d->h - 2, eof_color_black);
	hline(eof_window_3d->screen, 1, eof_window_3d->h - 2, eof_window_3d->w - 2, eof_color_white);
	vline(eof_window_3d->screen, eof_window_3d->w - 2, 1, eof_window_3d->h - 2, eof_color_white);
}

void eof_render_3d_window(void)
{
//	eof_log("eof_render_3d_window() entered");

	static int fretboardpoint[8];		//Used to cache the 3D->2D coordinate projections for the 3D fretboard
	int point[8];
	unsigned long i;
	unsigned long numsolos = 0;					//Used to abstract the solo sections
	unsigned long numnotes;				//Used to abstract the notes
	unsigned long numlanes;				//The number of fretboard lanes that will be rendered
	unsigned long tracknum;
	unsigned long firstlane = 0, lastlane;	//Used to track the first and last lanes that get track specific rendering (ie. drums don't render markers for lane 1, bass doesn't render markers for lane 6)
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the regular notes are rendered instead
	unsigned long temptrack = 0;
	int temphover = 0;
	long sz, sez;
	long spz, spez;
	long halflanewidth;
	long obx, oby, oex, oey;
	long bz;
	float y_projection;
	static float seek_y_projection, seek_x1_projection, seek_x2_projection;
	char ts_text[16] = {0}, tempo_text[16] = {0};
	long tr;
	int offset_y_3d = 0;	//If full height 3D preview is in effect, y coordinates will be shifted down by one panel height, otherwise will shift by 0

	//Used to draw trill and tremolo sections:
	unsigned long j, ctr, usedlanes, bitmask, numsections;
	EOF_PHRASE_SECTION *sectionptr = NULL;	//Used to abstract sections

	if(!eof_full_screen_3d)
	{	//If full screen 3D view isn't in effect, 3D rendering can be disabled
		if(eof_disable_3d_rendering)	//If the user wanted to disable the rendering of the 3D window to improve performance
			return;						//Return immediately

		if(eof_display_second_piano_roll && !eof_full_height_3d_preview)	//If a second piano roll is being rendered and full height 3D preview isn't enabled
			return;															//Return immediately
	}

	if(eof_display_catalog && eof_catalog_full_width)
	{	//If the fret catalog is visible and it's configured to display at full width
		return;	//Don't draw the 3D window
	}

	if(eof_song->track[eof_selected_track]->track_format == EOF_VOCAL_TRACK_FORMAT)
	{	//If this is a vocal track
		eof_render_lyric_window();
		return;
	}
	else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If the track being rendered is a pro guitar track
		tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
		if(tp->note == tp->technote)
		{	//If tech view is in effect for the active track
			restore_tech_view = 1;
			eof_menu_pro_guitar_track_disable_tech_view(tp);
			temphover = eof_hover_note;				//Temporarily clear the hover note, since any hover note in effect at this time applies to the tech notes and not the normal notes
			eof_hover_note = -1;
			temptrack = eof_selection.track;		//Likewise temporarily clear the selection track number
			eof_selection.track = 0;
		}
	}

	if(!eof_background)
	{	//If a background image was NOT loaded
		clear_to_color(eof_window_3d->screen, eof_color_gray);	//Clear the 3D preview portion of the screen
	}
	numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	lastlane = numlanes - 1;	//This variable begins lane numbering at 0 instead of 1
	eof_set_3D_lane_positions(eof_selected_track);	//Update the xchart[] array
	if(eof_track_is_ghl_mode(eof_song, eof_selected_track))
	{	//Special case:  Guitar Hero Live style tracks display with 3 lanes
		numlanes = 3;
		lastlane = 2;
	}
	else if(eof_track_is_legacy_guitar(eof_song, eof_selected_track))
	{	//Special case:  Legacy guitar tracks can use a sixth lane but their 3D representation still only draws 5 lanes
		numlanes = 5;
		lastlane = 4;	//Don't render trill/tremolo markers for the 6th lane (render for lanes 0 through 4)
	}
	if((eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && !eof_render_bass_drum_in_lane)
	{	//If a drum track is active and the user hasn't enabled the preference to render the bass drum in its own lane
		firstlane = 1;		//Don't render drum roll/special drum roll markers for the first lane, 0 (unless user enabled the preference to render bass drum in its own lane)
	}

	if(eof_full_height_3d_preview)
	{
		offset_y_3d = eof_screen_height / 2;	//Y coordinates will be lowered by one panel height so that the bottom of the 3D fret board is at the bottom of the program window
	}

	if(!eof_3d_fretboard_coordinates_cached)
	{	//If the appropriate 3D->2D coordinate projections for the 3D fretboard aren't cached yet
		fretboardpoint[0] = ocd3d_project_x(20, eof_3d_max_depth);
		fretboardpoint[1] = ocd3d_project_y(200 + offset_y_3d, eof_3d_max_depth);
		fretboardpoint[2] = ocd3d_project_x(300, eof_3d_max_depth);
		fretboardpoint[3] = fretboardpoint[1];
		fretboardpoint[4] = ocd3d_project_x(300, eof_3d_min_depth);
		fretboardpoint[5] = ocd3d_project_y(200 + offset_y_3d, eof_3d_min_depth);
		fretboardpoint[6] = ocd3d_project_x(20, eof_3d_min_depth);
		fretboardpoint[7] = fretboardpoint[5];
		seek_y_projection = ocd3d_project_y(200 + offset_y_3d, 0);
		seek_x1_projection = ocd3d_project_x(48, 0);
		seek_x2_projection = ocd3d_project_x(48 + 4 * 56, 0);
		eof_3d_fretboard_coordinates_cached = 1;
	}
	polygon(eof_window_3d->screen, 4, fretboardpoint, eof_color_black);

	/* render solo sections */
	numsolos = eof_get_num_solos(eof_song, eof_selected_track);
	for(i = 0; i < numsolos; i++)
	{
		sectionptr = eof_get_solo(eof_song, eof_selected_track, i);	//Obtain the information for this legacy/pro guitar solo
		if(sectionptr == NULL)
			continue;	//If the solo section couldn't be found, skip it

		sz = (long)(sectionptr->start_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
		sez = (long)(sectionptr->end_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
		if((eof_3d_min_depth <= sez) && (eof_3d_max_depth >= sz))
		{
			spz = sz < eof_3d_min_depth ? eof_3d_min_depth : sz;
			spez = sez > eof_3d_max_depth ? eof_3d_max_depth : sez;
			point[0] = ocd3d_project_x(20, spez);
			point[1] = ocd3d_project_y(200 + offset_y_3d, spez);
			point[2] = ocd3d_project_x(300, spez);
			point[3] = point[1];
			point[4] = ocd3d_project_x(300, spz);
			point[5] = ocd3d_project_y(200 + offset_y_3d, spz);
			point[6] = ocd3d_project_x(20, spz);
			point[7] = point[5];
			polygon(eof_window_3d->screen, 4, point, eof_color_dark_blue);
		}
	}

	/* render arpeggio sections */
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{
		for(i = 0; i < eof_song->pro_guitar_track[tracknum]->arpeggios; i++)
		{	//For each arpeggio section in the track
			int arpeggiocolor;

			sectionptr = &eof_song->pro_guitar_track[tracknum]->arpeggio[i];
			if(sectionptr->difficulty != eof_note_type)
				continue;	//If this arpeggio isn't in the active difficulty, skip it

			sz = (long)(sectionptr->start_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
			sez = (long)(sectionptr->end_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
			if((eof_3d_min_depth > sez) || (eof_3d_max_depth < sz))
				continue;	//If the arpeggio section would not render visibly, skip it

			//Otherwise fill the topmost lane with the appropriate color
			arpeggiocolor = eof_color_turquoise;	//Normal arpeggio phrases will render in turquoise
			if(sectionptr->flags & EOF_RS_ARP_HANDSHAPE)
			{	//If this arpeggio is configured to export as a normal handshape
				arpeggiocolor = eof_color_lighter_blue;
			}
			spz = sz < eof_3d_min_depth ? eof_3d_min_depth : sz;
			spez = sez > eof_3d_max_depth ? eof_3d_max_depth : sez;
			point[0] = ocd3d_project_x(20, spez);
			point[1] = ocd3d_project_y(200 + offset_y_3d, spez);
			point[2] = ocd3d_project_x(300, spez);
			point[3] = point[1];
			point[4] = ocd3d_project_x(300, spz);
			point[5] = ocd3d_project_y(200 + offset_y_3d, spz);
			point[6] = ocd3d_project_x(20, spz);
			point[7] = point[5];
			polygon(eof_window_3d->screen, 4, point, arpeggiocolor);	//Fill with a turquoise or light blue color
		}
	}

	/* render seek selection */
	halflanewidth = (56.0 * (4.0 / (numlanes-1))) / 2;
	if(eof_seek_selection_start != eof_seek_selection_end)
	{	//If there is a seek selection
		sz = (long)(eof_seek_selection_start + eof_av_delay - eof_music_pos) / eof_zoom_3d;
		sez = (long)(eof_seek_selection_end + eof_av_delay - eof_music_pos) / eof_zoom_3d;
		if((eof_3d_min_depth <= sez) && (eof_3d_max_depth >= sz))
		{
			spz = sz < eof_3d_min_depth ? eof_3d_min_depth : sz;
			spez = sez > eof_3d_max_depth ? eof_3d_max_depth : sez;
			point[0] = ocd3d_project_x(xchart[0], spez);
			point[1] = ocd3d_project_y(200 + offset_y_3d, spez);
			point[2] = ocd3d_project_x(xchart[lastlane], spez);
			point[3] = point[1];
			point[4] = ocd3d_project_x(xchart[lastlane], spz);
			point[5] = ocd3d_project_y(200 + offset_y_3d, spz);
			point[6] = ocd3d_project_x(xchart[0], spz);
			point[7] = point[5];
			polygon(eof_window_3d->screen, 4, point, eof_color_red);
		}
	}

	/* render trill and tremolo sections */
	if(eof_get_num_trills(eof_song, eof_selected_track) || eof_get_num_tremolos(eof_song, eof_selected_track))
	{	//If this track has any trill or tremolo sections
		long xoffset = 0;	//This will be used to offset the trill/tremolo lane fill as necessary to center the fill over that lane's gem
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			xoffset = halflanewidth;	//Drum gems render half a lane width further right (in between fret lines instead of centered over the lines)
		}

		//Build the lane X coordinate array
		for(j = 0; j < 2; j++)
		{	//For each of the two phrase types (trills and tremolos)
			if(j == 0)
			{	//On the first pass, render trill sections
				numsections = eof_get_num_trills(eof_song, eof_selected_track);
			}
			else
			{	//On the second pass, render tremolo sections
				numsections = eof_get_num_tremolos(eof_song, eof_selected_track);
			}
			for(i = 0; i < numsections; i++)
			{	//For each trill or tremolo section in the track
				if(j == 0)
				{	//On the first pass, render trill sections
					sectionptr = eof_get_trill(eof_song, eof_selected_track, i);
				}
				else
				{	//On the second pass, render tremolo sections
					sectionptr = eof_get_tremolo(eof_song, eof_selected_track, i);
				}
				if(sectionptr == NULL)
					continue;	//If the section could not be found for some reason, skip it

				if(j)
				{	//If tremolo sections are being rendered
					if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
					{	//If the track's difficulty limit has been removed
						if(sectionptr->difficulty != eof_note_type)	//And the tremolo section does not apply to the active track difficulty
							continue;	//Skip rendering it
					}
					else
					{
						if(sectionptr->difficulty != 0xFF)	//Otherwise if the tremolo section does not apply to all track difficulties
							continue;	//Skip rendering it
					}
				}
				sz = (long)(sectionptr->start_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
				sez = (long)(sectionptr->end_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
				spz = sz < eof_3d_min_depth ? eof_3d_min_depth : sz;
				spez = sez > eof_3d_max_depth ? eof_3d_max_depth : sez;
				if((eof_3d_min_depth > sez) || (eof_3d_max_depth < sz))
					continue;	//If the section would not render to the visible portion of the screen, skip it

				usedlanes = eof_get_used_lanes(eof_selected_track, sectionptr->start_pos, sectionptr->end_pos, eof_note_type);	//Determine which lane(s) use this phrase
				for(ctr = firstlane, bitmask = (1 << firstlane); ctr <= lastlane; ctr++, bitmask <<= 1)
				{	//For each of the usable lanes (that are allowed to have lane specific marker rendering)
					if(usedlanes & bitmask)
					{	//If this lane is used in the phrase and the lane is active
						point[0] = ocd3d_project_x(xchart[ctr] - halflanewidth - xoffset, spez);	//Offset drum lanes by drawing them one lane further left than other tracks
						point[1] = ocd3d_project_y(200 + offset_y_3d, spez);
						point[2] = ocd3d_project_x(xchart[ctr] + halflanewidth - xoffset, spez);
						point[3] = point[1];
						point[4] = ocd3d_project_x(xchart[ctr] + halflanewidth - xoffset, spz);
						point[5] = ocd3d_project_y(200 + offset_y_3d, spz);
						point[6] = ocd3d_project_x(xchart[ctr] - halflanewidth - xoffset, spz);
						point[7] = point[5];
						polygon(eof_window_3d->screen, 4, point, eof_colors[ctr].lightcolor);
					}
				}
			}//For each trill or tremolo section in the track
		}//For each of the two phrase types (trills and tremolos)
	}//If this track has any trill or tremolo sections

	/* draw the beat markers */
	for(i = 0; i < eof_song->beats; i++)
	{	//For each beat
		bz = (long)(eof_song->beat[i]->pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
		if((bz >= eof_3d_min_depth) && (bz <= eof_3d_max_depth))
		{	//If the beat is visible
			y_projection = ocd3d_project_y(200 + offset_y_3d, bz);
			hline(eof_window_3d->screen, ocd3d_project_x(48, bz), y_projection, ocd3d_project_x(48 + 4 * 56, bz), (eof_song->beat[i]->has_ts && (eof_song->beat[i]->beat_within_measure == 0)) ? eof_color_white : eof_color_dark_silver);
			if(eof_song->beat[i]->contains_tempo_change || eof_song->beat[i]->contains_ts_change)
			{	//If this beat contains either a tempo or TS change
				if(eof_song->beat[i]->contains_tempo_change)
				{	//If there is a tempo change
					eof_get_tempo_text(i, tempo_text);
				}
				else
				{
					tempo_text[0] = '\0';	//Otherwise empty out this string
				}
				eof_get_ts_text(i, ts_text);
				textprintf_ex(eof_window_3d->screen, eof_font, ocd3d_project_x(48 + 4 * 56 + 4, bz), y_projection - 12, eof_color_white, -1, "%s%s", tempo_text, ts_text);	//Render the tempo and time signature to the right of the beat marker
			}
		}
		else if(bz > eof_3d_max_depth)
		{	//If this beat wasn't visible
			break;	//None of the remaining ones will be either, so stop rendering them
		}
	}//For each beat

	//Draw fret lanes
	for(i = 0; i < numlanes; i++)
	{
		obx = ocd3d_project_x(xchart[i], eof_3d_min_depth);
		oex = ocd3d_project_x(xchart[i], eof_3d_max_depth);
		oby = fretboardpoint[5];	//This is the front edge of the 3D piano roll
		oey = fretboardpoint[1];	//This is the back edge of the 3D piano roll

		line(eof_window_3d->screen, obx, oby, oex, oey, eof_color_white);
	}

	/* draw the position marker */
	line(eof_window_3d->screen, seek_x1_projection, seek_y_projection, seek_x2_projection, seek_y_projection, eof_color_green);

//	int first_note = -1;	//Used for debugging
//	int last_note = 0;		//Used for debugging
	/* draw the note tails and notes */
	numnotes = eof_get_track_size(eof_song, eof_selected_track);	//Get the number of notes in this legacy/pro guitar track
	for(i = numnotes; i > 0; i--)
	{	//Render 3D notes from last to first so that the earlier notes are in front
		int p;

		if(eof_note_type != eof_get_note_type(eof_song, eof_selected_track, i - 1))
			continue;	//If this note isn't in the active difficulty, skip it

		p = ((eof_selection.track == eof_selected_track) && eof_selection.multi[i-1] && eof_music_paused) ? 1 : (i-1) == eof_hover_note ? 2 : 0;	//Cache this result to use it twice
		tr = eof_note_tail_draw_3d(eof_selected_track, i-1, p);
		(void) eof_note_draw_3d(eof_selected_track, i-1, p);

		if(tr < 0)	//if eof_note_tail_draw_3d skipped rendering the tail because it renders before the visible area
			break;	//Stop rendering 3d notes

/*	Used for debugging
		if(tr == 0)
		{
			if(first_note < 0)
			{
				first_note = i;
			}
		}
		else if(tr < 0)
		{
			if(first_note >= 0)
			{
				last_note = i;
				break;
			}
		}
*/
	}
//	allegro_message("first = %d\nlast = %d", first_note, last_note);	//Debug

	eof_render_lyric_preview(eof_window_3d->screen);

	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If a pro guitar/bass track is active
		unsigned long popupmessage;
		if(eof_find_effective_rs_popup_message(eof_music_pos - eof_av_delay, &popupmessage))
		{	//If there is a popup message in effect at the current position
			textout_centre_ex(eof_window_3d->screen, font, eof_song->pro_guitar_track[tracknum]->popupmessage[popupmessage].name, eof_window_3d->screen->w / 2, 4, eof_color_white, eof_color_black);
		}
	}

	//Draw a border around the edge of the 3D window
	if(!eof_full_screen_3d)
	{	//Skip drawing a border if the 3D preview is going to take up the full program window already
		rect(eof_window_3d->screen, 0, 0, eof_window_3d->w - 1, eof_window_3d->h - 1, eof_color_dark_silver);
		rect(eof_window_3d->screen, 1, 1, eof_window_3d->w - 2, eof_window_3d->h - 2, eof_color_black);
		hline(eof_window_3d->screen, 1, eof_window_3d->h - 2, eof_window_3d->w - 2, eof_color_white);
		vline(eof_window_3d->screen, eof_window_3d->w - 2, 1, eof_window_3d->h - 2, eof_color_white);
	}

	if(tp && restore_tech_view)
	{	//If tech view needs to be re-enabled
		eof_menu_pro_guitar_track_enable_tech_view(tp);
		eof_hover_note = temphover;				//Restore the hover note
		eof_selection.track = temptrack;		//Restore the selection track
	}
}

void eof_render_notes_window(void)
{
	if(!eof_enable_notes_panel || !eof_window_notes)	//If the notes panel isn't enabled, the note window isn't created or the notes panel text file isn't loaded
		return;		//Return immediately
	if(eof_display_second_piano_roll)	//If a second piano roll is being rendered instead of the panels that display on the bottom half of the program window
		return;							//Return immediately
	if(eof_display_catalog && eof_catalog_full_width)	//If the fret catalog is visible and it's configured to display at full width
		return;		//Return immediately
	if(!eof_notes_panel)
	{	//If the notes panel isn't properly loaded
		eof_enable_notes_panel = 0;		//Properly disable the panel's display option
		return;
	}

	eof_render_text_panel(eof_notes_panel, 1);
}

void eof_render(void)
{
//	eof_log("eof_render() entered.", 3);

	/* don't draw if window is out of focus */
	if(!eof_has_focus)
	{	//If EOF is not in the foreground
		eof_log("\tEOF is in background, ending render.", 3);
		if(eof_music_paused && !eof_music_catalog_playback)
		{	//If neither the chart nor the catalog are playing (depending on user preference, playback is allowed when EOF is not in the foreground)
			return;
		}
	}
	if(eof_song_loaded)
	{	//If a project is loaded
//		eof_log("\tProject is loaded.", 3);
		if(eof_window_title_dirty)
		{	//If the window title needs to be redrawn
			eof_fix_window_title();
		}
		if(eof_background)
		{	//If a background image was loaded
			if((eof_screen->h > eof_background->h + 5) || (eof_screen->w > eof_background->w + 5))
			{	//If the program window is more than 5 pixels taller/wider than the background image
				clear_to_color(eof_screen, eof_color_gray);	//Clear the screen to ensure that remnants of the previous frame don't get left
			}
			blit(eof_background, eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);	//Display it
		}
		else
		{	//Otherwise just draw a blank screen
			clear_to_color(eof_screen, eof_color_light_gray);
		}
		if(!eof_full_screen_3d && !eof_screen_zoom)
		{	//Only blit the menu bar now if neither full screen 3D view nor x2 zoom is in effect, otherwise it will be blitted later
			if((eof_count_selected_notes(NULL) > 0) || ((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_seek_hover_note >= 0)))
			{	//If notes are selected, or the seek position is at a note position when Feedback input mode is in use
				blit(eof_image[EOF_IMAGE_MENU_FULL], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
			}
			else
			{
				blit(eof_image[EOF_IMAGE_MENU_NO_NOTE], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
			}
		}
		if(!eof_beat_stats_cached)
		{	//If the cached beat statistics are not current
			eof_log("\tRebuilding beat stats.", 3);
			eof_process_beat_statistics(eof_song, eof_selected_track);	//Rebuild them (from the perspective of the specified track)
		}
		eof_ch_sp_solution_wanted = 0;	//The rendering of the info and notes panels will change this to 1 any CH SP scoring information is needed for expansion macros
		if(!eof_full_screen_3d)
		{	//In full screen 3D view, don't render the info window yet, it will just be overwritten by the 3D window
//			eof_log("\tRendering Information panel.", 3);
			eof_render_info_window();	//Otherwise render the info window first, so if the user didn't opt to display its full width, it won't draw over the 3D window
		}
//		eof_log("\tRendering 3D preview.", 3);
		eof_render_3d_window();
		if(!eof_full_screen_3d)
		{	//In full screen 3D view, don't render these windows
			eof_render_editor_window(eof_window_editor);	//Render the primary piano roll
			eof_render_editor_window_2();	//Render the secondary piano roll if applicable
			eof_render_notes_window();		//Render the notes panel if applicable
		}
		eof_ch_sp_solution_rebuild();	//Build SP CH solution data if necessary
	}
	else
	{	//If no project is loaded
		eof_log("\tProject is not loaded.", 3);
		if(eof_background)
		{	//If a background image was loaded
			blit(eof_background, eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);	//Display it
		}
		else
		{	//Otherwise just draw a blank screen
			clear_to_color(eof_screen, eof_color_gray);
		}
		if(eof_screen_zoom)
		{	//If x2 zoom is enabled, stretch blit the menu
			stretch_blit(eof_image[EOF_IMAGE_MENU], eof_screen, 0, 0, eof_image[EOF_IMAGE_MENU]->w, eof_image[EOF_IMAGE_MENU]->h, 0, 0, eof_image[EOF_IMAGE_MENU]->w / 2, eof_image[EOF_IMAGE_MENU]->h / 2);
		}
		else
		{	//Otherwise render it normally
			blit(eof_image[EOF_IMAGE_MENU], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
		}
	}

//	eof_log("\tSub-windows rendered", 3);

	if(eof_cursor_visible && eof_soft_cursor)
	{	//If rendering the software mouse cursor, do so at the actual screen coordinates
		eof_log("\tRendering software cursor.", 3);
		draw_sprite(eof_screen, mouse_sprite, mouse_x - 1, mouse_y - 1);
	}

	if(eof_full_screen_3d && eof_song_loaded)
	{	//If the user enabled full screen 3D view, scale it to fill the program window
		BITMAP *temp_3d;	//A temporary copy of the 3D preview, so eof_screen doesn't have to blit on top of itself

		eof_log("\tRendering full screen 3D preview.", 3);

		temp_3d = create_bitmap(eof_window_3d->screen->w, eof_window_3d->screen->h);
		if(!temp_3d)
			return;	//Failed to create temporary bitmap

		blit(eof_window_3d->screen, temp_3d, 0, 0, 0, 0, temp_3d->w, temp_3d->h);	//Make a copy of the 3D preview

		if(!eof_full_height_3d_preview)
		{	//If a normal size 3D preview is to be scaled to take up the entire program window
			stretch_blit(temp_3d, eof_screen, 0, 0, EOF_SCREEN_PANEL_WIDTH, eof_screen_height / 2, 0, 0, eof_screen_width_default, eof_screen_height);
		}
		else
		{	//If the 3D preview is already the full height of the program window, draw it along the right edge of the screen to allow space for the info panel
			int xpos = EOF_SCREEN_PANEL_WIDTH / 6;

			if(eof_background)
			{	//If a background image was loaded
				blit(eof_background, eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);	//Display it
			}
			else
			{	//Otherwise just draw a blank screen
				clear_to_color(eof_screen, eof_color_gray);
			}
			stretch_blit(temp_3d, eof_screen, 0, 0, EOF_SCREEN_PANEL_WIDTH, eof_screen_height, xpos, 20, EOF_SCREEN_PANEL_WIDTH * 2, eof_screen_height);
		}
		destroy_bitmap(temp_3d);	//Destroy the copy of the 3D preview
		rectfill(eof_screen, EOF_SCREEN_PANEL_WIDTH * 2 + 1, 0, eof_screen->w - 1, eof_screen->h - 1, eof_color_gray);	//Erase the portion to the right of the scaled 3D preview (2 panel widths), in case the window width was increased, otherwise the normal sized 3D preview will be visible
		eof_window_info->y = 0;	//Re-position the info window to the top left corner of EOF's program window
		eof_render_info_window();
		if(!eof_screen_zoom)
		{	//If x2 zoom is not enabled, render the menu now
			if((eof_count_selected_notes(NULL) > 0) || ((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_seek_hover_note >= 0)))
			{	//If notes are selected, or the seek position is at a note position when Feedback input mode is in use
				blit(eof_image[EOF_IMAGE_MENU_FULL], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
			}
			else
			{
				blit(eof_image[EOF_IMAGE_MENU_NO_NOTE], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
			}
		}
		eof_window_info->y = eof_screen_height / 2;	//Re-position the info window to the bottom left corner of EOF's program window
	}

	if(!eof_disable_vsync)
	{	//Wait for vsync unless this was disabled
		eof_log("\tWaiting for vsync.", 3);
		IdleUntilVSync();
		vsync();
		DoneVSync();
	}
	if(eof_screen_zoom)
	{	//If x2 zoom is enabled, stretch blit the menu to eof_screen, blit that to eof_screen2, blit the menu to eof_screen2 and then stretch blit that to screen
		//Blitting straight to screen causes flickery menus
		//Drawing the menu half size and then stretching it to full size makes it unreadable, but that may be better than not rendering them at all
		//The highest quality (and most memory wasteful) solution would require another large bitmap to render the x2 program window and then the x1 menus on top, which would then be blit to screen
		eof_log("\tPerforming x2 blit.", 3);
		stretch_blit(eof_screen, eof_screen2, 0, 0, eof_screen_width, eof_screen_height, 0, 0, SCREEN_W, SCREEN_H);	//Stretch blit the screen to another bitmap
		if((eof_count_selected_notes(NULL) > 0) || ((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_seek_hover_note >= 0)))
		{	//If notes are selected, or the seek position is at a note position when Feedback input mode is in use
			blit(eof_image[EOF_IMAGE_MENU_FULL], eof_screen2, 0, 0, 0, 0, eof_screen->w, eof_screen->h);			//Normal blit the menu to that latter bitmap
		}
		else
		{
			blit(eof_image[EOF_IMAGE_MENU_NO_NOTE], eof_screen2, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
		}
		blit(eof_screen2, screen, 0, 0, 0, 0, eof_screen2->w, eof_screen2->h);	//Blit that latter bitmap to screen
	}
	else
	{
//		eof_log("\tPerforming normal blit.", 3);
		blit(eof_screen, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);	//Render the screen normally
	}
	#ifdef ALLEGRO_LEGACY
		allegro_render_screen();
	#endif

//	eof_log("eof_render() completed.", 3);
}

static int work_around_fsel_bug = 0;
/* The AGUP d_agup_edit_proc isn't exactly compatible to Allegro's stock
 * d_edit_proc, which has no border at all. Therefore we resort to this little
 * hack.
 */
int d_hackish_edit_proc (int msg, DIALOG *d, int c)
{
	if(!d)	//If this pointer is NULL for any reason
		return d_agup_edit_proc (msg, d, c);

	if ((msg == MSG_START) && !work_around_fsel_bug)
	{
		/* Adjust position/dimension so it is the same as AGUP's. */
		d->y -= 3;
		d->h += 6;
		/* The Allegro GUI has a bug where it repeatedly sends MSG_START to a
		 * custom GUI procedure. We need to work around that.
		 */
		work_around_fsel_bug = 1;
	}
	if ((msg == MSG_END) && work_around_fsel_bug)
	{
		d->y += 3;
		d->h -= 6;
		work_around_fsel_bug = 0;
	}
	return d_agup_edit_proc (msg, d, c);
}

BITMAP *eof_load_pcx(char *filename)
{
	char path[1024];

	if(!filename)
		return NULL;	//Invalid parameter

	//Check for the specified file in the resources folder
	(void) snprintf(path, sizeof(path) - 1, "resources");
	put_backslash(path);	//Append a file separator
	(void) strncat(path, filename, sizeof(path) - 1);	//Append the file name

	if(exists(path))
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLoading custom resource \"%s\"", path);
		eof_log(eof_log_string, 1);
	}
	else
	{	//If the file doesn't exist within /resources, change the path to within eof.dat
		(void) snprintf(path, sizeof(path) - 1, "eof.dat#%s", filename);
	}

	return load_pcx(path, NULL);
}

FONT *eof_load_bitmap_font(char *filename)
{
	char path[1024];

	if(!filename)
		return NULL;	//Invalid parameter

	//Check for the specified file in the resources folder
	(void) snprintf(path, sizeof(path) - 1, "resources");
	put_backslash(path);	//Append a file separator
	(void) strncat(path, filename, sizeof(path) - 1);	//Append the file name

	if(exists(path))
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLoading custom font \"%s\"", path);
		eof_log(eof_log_string, 1);
	}
	else
	{	//If the file doesn't exist within /resources, change the path to within eof.dat
		(void) snprintf(path, sizeof(path) - 1, "eof.dat#%s", filename);
	}

	return load_bitmap_font(path, NULL, NULL);
}

int eof_load_data(void)
{
	int i;

	eof_log("eof_load_data() entered", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 0;
	}
	if(!exists("eof.dat"))
	{
		allegro_message("DAT file missing.  If using a hotfix, make sure you extract the appropriate EOF release candidate first and then extract the hotfix on top of it (replacing with files in the hotfix).");
		return 0;
	}

	//Load the palette
	eof_image[EOF_IMAGE_TAB0] = load_pcx("eof.dat#tab0.pcx", eof_palette);	//Load an image from eof.dat and store the 256 color palette
	if(eof_image[EOF_IMAGE_TAB0])
		destroy_bitmap(eof_image[EOF_IMAGE_TAB0]);

	//Load images
	eof_image[EOF_IMAGE_TAB0] = eof_load_pcx("tab0.pcx");
	eof_image[EOF_IMAGE_TAB1] = eof_load_pcx("tab1.pcx");
	eof_image[EOF_IMAGE_TAB2] = eof_load_pcx("tab2.pcx");
	eof_image[EOF_IMAGE_TAB3] = eof_load_pcx("tab3.pcx");
	eof_image[EOF_IMAGE_TAB4] = eof_load_pcx("tab4.pcx");
	eof_image[EOF_IMAGE_TABE] = eof_load_pcx("tabempty.pcx");
	eof_image[EOF_IMAGE_VTAB0] = eof_load_pcx("vtab0.pcx");
	eof_image[EOF_IMAGE_SCROLL_BAR] = eof_load_pcx("scrollbar.pcx");
	eof_image[EOF_IMAGE_SCROLL_HANDLE] = eof_load_pcx("scrollhandle.pcx");
	eof_image[EOF_IMAGE_NOTE_GREEN] = eof_load_pcx("note_green.pcx");
	eof_image[EOF_IMAGE_NOTE_RED] = eof_load_pcx("note_red.pcx");
	eof_image[EOF_IMAGE_NOTE_YELLOW] = eof_load_pcx("note_yellow.pcx");
	eof_image[EOF_IMAGE_NOTE_BLUE] = eof_load_pcx("note_blue.pcx");
	eof_image[EOF_IMAGE_NOTE_PURPLE] = eof_load_pcx("note_purple.pcx");
	eof_image[EOF_IMAGE_NOTE_WHITE] = eof_load_pcx("note_white.pcx");
	eof_image[EOF_IMAGE_NOTE_GREEN_HIT] = eof_load_pcx("note_green_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_RED_HIT] = eof_load_pcx("note_red_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_YELLOW_HIT] = eof_load_pcx("note_yellow_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_BLUE_HIT] = eof_load_pcx("note_blue_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_PURPLE_HIT] = eof_load_pcx("note_purple_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_WHITE_HIT] = eof_load_pcx("note_white_hit.pcx");
	eof_image[EOF_IMAGE_CONTROLS_BASE] = eof_load_pcx("controls0.pcx");
	eof_image[EOF_IMAGE_CONTROLS_0] = eof_load_pcx("controls1.pcx");
	eof_image[EOF_IMAGE_CONTROLS_1] = eof_load_pcx("controls2.pcx");
	eof_image[EOF_IMAGE_CONTROLS_2] = eof_load_pcx("controls3.pcx");
	eof_image[EOF_IMAGE_CONTROLS_3] = eof_load_pcx("controls4.pcx");
	eof_image[EOF_IMAGE_CONTROLS_4] = eof_load_pcx("controls5.pcx");
	eof_image[EOF_IMAGE_MENU] = eof_load_pcx("menu.pcx");
	eof_image[EOF_IMAGE_MENU_FULL] = eof_load_pcx("menufull.pcx");
	eof_image[EOF_IMAGE_CCONTROLS_BASE] = eof_load_pcx("ccontrols0.pcx");
	eof_image[EOF_IMAGE_CCONTROLS_0] = eof_load_pcx("ccontrols1.pcx");
	eof_image[EOF_IMAGE_CCONTROLS_1] = eof_load_pcx("ccontrols2.pcx");
	eof_image[EOF_IMAGE_CCONTROLS_2] = eof_load_pcx("ccontrols3.pcx");
	eof_image[EOF_IMAGE_MENU_NO_NOTE] = eof_load_pcx("menunonote.pcx");
	eof_image[EOF_IMAGE_NOTE_YELLOW_CYMBAL] = eof_load_pcx("note_yellow_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_YELLOW_CYMBAL_HIT] = eof_load_pcx("note_yellow_hit_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_BLUE_CYMBAL] = eof_load_pcx("note_blue_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_BLUE_CYMBAL_HIT] = eof_load_pcx("note_blue_hit_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_PURPLE_CYMBAL] = eof_load_pcx("note_purple_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_PURPLE_CYMBAL_HIT] = eof_load_pcx("note_purple_hit_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_WHITE_CYMBAL] = eof_load_pcx("note_white_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_WHITE_CYMBAL_HIT] = eof_load_pcx("note_white_hit_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_ORANGE] = eof_load_pcx("note_orange.pcx");
	eof_image[EOF_IMAGE_NOTE_ORANGE_HIT] = eof_load_pcx("note_orange_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GREEN_ARROW] = eof_load_pcx("note_green_arrow.pcx");
	eof_image[EOF_IMAGE_NOTE_RED_ARROW] = eof_load_pcx("note_red_arrow.pcx");
	eof_image[EOF_IMAGE_NOTE_YELLOW_ARROW] = eof_load_pcx("note_yellow_arrow.pcx");
	eof_image[EOF_IMAGE_NOTE_BLUE_ARROW] = eof_load_pcx("note_blue_arrow.pcx");
	eof_image[EOF_IMAGE_NOTE_GREEN_ARROW_HIT] = eof_load_pcx("note_green_hit_arrow.pcx");
	eof_image[EOF_IMAGE_NOTE_RED_ARROW_HIT] = eof_load_pcx("note_red_hit_arrow.pcx");
	eof_image[EOF_IMAGE_NOTE_YELLOW_ARROW_HIT] = eof_load_pcx("note_yellow_hit_arrow.pcx");
	eof_image[EOF_IMAGE_NOTE_BLUE_ARROW_HIT] = eof_load_pcx("note_blue_hit_arrow.pcx");
	eof_image[EOF_IMAGE_NOTE_GREEN_CYMBAL] = eof_load_pcx("note_green_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_GREEN_CYMBAL_HIT] = eof_load_pcx("note_green_hit_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_ORANGE_CYMBAL] = eof_load_pcx("note_orange_cymbal.pcx");
	eof_image[EOF_IMAGE_NOTE_ORANGE_CYMBAL_HIT] = eof_load_pcx("note_orange_hit_cymbal.pcx");
	eof_image[EOF_IMAGE_TAB_FG] = eof_load_pcx("tabfg.pcx");
	eof_image[EOF_IMAGE_TAB_BG] = eof_load_pcx("tabbg.pcx");
	eof_image[EOF_IMAGE_NOTE_BLACK] = eof_load_pcx("note_black.pcx");
	eof_image[EOF_IMAGE_NOTE_BLACK_HIT] = eof_load_pcx("note_black_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK] = eof_load_pcx("note_ghl_black.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_HIT] = eof_load_pcx("note_ghl_black_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_HOPO] = eof_load_pcx("note_ghl_black_hopo.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_HOPO_HIT] = eof_load_pcx("note_ghl_black_hopo_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_SP] = eof_load_pcx("note_ghl_black_sp.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_SP_HIT] = eof_load_pcx("note_ghl_black_sp_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_SP_HOPO] = eof_load_pcx("note_ghl_black_sp_hopo.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_SP_HOPO_HIT] = eof_load_pcx("note_ghl_black_sp_hopo_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE] = eof_load_pcx("note_ghl_white.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_HIT] = eof_load_pcx("note_ghl_white_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_HOPO] = eof_load_pcx("note_ghl_white_hopo.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_HOPO_HIT] = eof_load_pcx("note_ghl_white_hopo_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_SP] = eof_load_pcx("note_ghl_white_sp.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_SP_HIT] = eof_load_pcx("note_ghl_white_sp_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_SP_HOPO] = eof_load_pcx("note_ghl_white_sp_hopo.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_SP_HOPO_HIT] = eof_load_pcx("note_ghl_white_sp_hopo_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE] = eof_load_pcx("note_ghl_barre.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_HIT] = eof_load_pcx("note_ghl_barre_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_HOPO] = eof_load_pcx("note_ghl_barre_hopo.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_HOPO_HIT] = eof_load_pcx("note_ghl_barre_hopo_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_SP] = eof_load_pcx("note_ghl_barre_sp.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_SP_HIT] = eof_load_pcx("note_ghl_barre_sp_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_SP_HOPO] = eof_load_pcx("note_ghl_barre_sp_hopo.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_SP_HOPO_HIT] = eof_load_pcx("note_ghl_barre_sp_hopo_hit.pcx");
	eof_image[EOF_IMAGE_NOTE_GREEN_SLIDER] = eof_load_pcx("note_green_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_RED_SLIDER] = eof_load_pcx("note_red_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_YELLOW_SLIDER] = eof_load_pcx("note_yellow_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_BLUE_SLIDER] = eof_load_pcx("note_blue_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_PURPLE_SLIDER] = eof_load_pcx("note_purple_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_ORANGE_SLIDER] = eof_load_pcx("note_orange_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_WHITE_SLIDER] = eof_load_pcx("note_white_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_BLACK_SLIDER] = eof_load_pcx("note_black_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_SLIDER] = eof_load_pcx("note_ghl_white_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_SLIDER] = eof_load_pcx("note_ghl_black_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_SLIDER] = eof_load_pcx("note_ghl_barre_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_WHITE_SP_SLIDER] = eof_load_pcx("note_ghl_white_sp_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BLACK_SP_SLIDER] = eof_load_pcx("note_ghl_black_sp_slider.pcx");
	eof_image[EOF_IMAGE_NOTE_GHL_BARRE_SP_SLIDER] = eof_load_pcx("note_ghl_barre_sp_slider.pcx");
	eof_background = eof_load_pcx("background.pcx");

	//Load and scale the non-GHL HOPO images, using the scale size in user preferences
	if(!eof_load_and_scale_hopo_images((double)eof_3d_hopo_scale_size / 100.0))
	{
		allegro_message("Error loading and scaling 3D HOPO images!");
		return 0;
	}

	//Load and process fonts
	eof_font = eof_load_bitmap_font("font_times_new_roman.pcx");
	if(!eof_font)
	{
		allegro_message("Could not load font!");
		return 0;
	}
	eof_mono_font = eof_load_bitmap_font("font_courier_new.pcx");
	if(!eof_mono_font)
	{
		allegro_message("Could not load mono font!");
		return 0;
	}
	eof_symbol_font = eof_load_bitmap_font("font_symbols.pcx");
	if(!eof_symbol_font)
	{
		allegro_message("Could not load symbol font!");
		return 0;
	}
	eof_add_extended_ascii_glyphs();	//Copy extended ASCII glyphs 128 - 159 to their Unicode equivalent code points

	eof_image[EOF_IMAGE_LYRIC_SCRATCH] = create_bitmap(320, text_height(eof_font) - 1);
	eof_allegro_font = font;	//Back up the pointer to Allegro's built-in font
	font = eof_font;
	set_palette(eof_palette);
	set_mouse_sprite(NULL);

	for(i = 1; i < EOF_MAX_IMAGES; i++)
	{
		if(!eof_image[i])
		{
			allegro_message("Could not load all images!\nFailed on %d", i);
			return 0;
		}
	}

	eof_color_black = makecol(0, 0, 0);
	eof_color_white = makecol(255, 255, 255);
	eof_color_gray = makecol(64, 64, 64);
	eof_color_gray2 = makecol(147, 147, 147);
	eof_color_gray3 = makecol(163, 163, 163);
	eof_color_light_gray = makecol(224, 224, 224);
	eof_color_red = makecol(255, 0, 0);
	eof_color_light_red = makecol(255, 150, 150);
	eof_color_green = makecol(0, 255, 0);
	eof_color_dark_green = makecol(0, 128, 0);
	eof_color_blue = makecol(0, 0, 255);
	eof_color_dark_blue = makecol(0, 0, 96);
	eof_color_light_blue = makecol(96, 96, 255);
	eof_color_lighter_blue = makecol(160, 160, 255);
	eof_color_turquoise = makecol(51,166,153);	//(use (68,221,204) for light turquoise)
	eof_color_yellow = makecol(255, 255, 0);
	eof_color_purple = makecol(255, 0, 255);
	eof_color_dark_purple = makecol(128, 0, 128);
	eof_color_orange = makecol(255, 127, 0);
	eof_color_silver = makecol(192, 192, 192);
	eof_color_dark_silver = makecol(160, 160, 160);
	eof_color_cyan = makecol(0, 255, 255);
	eof_color_dark_cyan = makecol(0, 160, 160);

	//Recreate the colors read from the config file to ensure they are in the correct color system
	//The original RRGGBB hex values need to be retained to write to config file upon exit
	eof_color_waveform_trough = eof_remake_color(eof_color_waveform_trough_raw);
	eof_color_waveform_peak = eof_remake_color(eof_color_waveform_peak_raw);
	eof_color_waveform_rms = eof_remake_color(eof_color_waveform_rms_raw);
	eof_color_highlight1 = eof_remake_color(eof_color_highlight1_raw);
	eof_color_highlight2 = eof_remake_color(eof_color_highlight2_raw);

	gui_fg_color = agup_fg_color;
	gui_bg_color = agup_bg_color;
	gui_mg_color = agup_mg_color;
	agup_init(awin95_theme);

	return 1;
}

BITMAP *eof_scale_image(BITMAP *source, double value)
{
	BITMAP *new_bitmap;

	if(!source)
		return source;	//Invalid parameter

	new_bitmap = create_bitmap(source->w * value, source->h * value);
	if(!new_bitmap)
		return source;	//Failed to create bitmap

	stretch_blit(source, new_bitmap, 0, 0, source->w, source->h, 0, 0, new_bitmap->w, new_bitmap->h);
	destroy_bitmap(source);

	return new_bitmap;
}

void eof_destroy_data(void)
{
	int i;

	eof_log("eof_destroy_data() entered", 1);

	agup_shutdown();
	for(i = 0; i < EOF_MAX_IMAGES; i++)
	{
		if(eof_image[i])
		{
			destroy_bitmap(eof_image[i]);
			eof_image[i] = NULL;
		}
	}
	if(eof_stretch_bitmap)
	{
		destroy_bitmap(eof_stretch_bitmap);
		eof_stretch_bitmap = NULL;
	}
	if(eof_hopo_stretch_bitmap)
	{
		destroy_bitmap(eof_hopo_stretch_bitmap);
		eof_hopo_stretch_bitmap = NULL;
	}
	if(eof_font)
	{	//If the main font was loaded
		destroy_font(eof_font);
		eof_font = NULL;
	}
	if(eof_mono_font)
	{	//If the monospace font was loaded
		destroy_font(eof_mono_font);
		eof_mono_font = NULL;
	}
	if(eof_symbol_font)
	{	//If the symbol font was loaded
		destroy_font(eof_symbol_font);
		eof_symbol_font = NULL;
	}
	if(eof_background)
	{
		destroy_bitmap(eof_background);
		eof_background = NULL;
	}
}

int eof_initialize(int argc, char * argv[])
{
	int i, eof_zoom_backup;
	char temp_filename[1024] = {0}, eof_recover_on_path[50], eof_recover_path[50], recovered = 0;
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format
	char *logging_level[] = {"NULL", "normal", "verbose", "exhaustive"};
	unsigned retry;

	eof_log("eof_initialize() entered", 1);

	if(!argv)
	{
		return 0;
	}

	allegro_init();

	if((argc >= 3) && !ustricmp(argv[1], "-ch_sp_path_worker"))
	{	//If this EOF instance was launched as a worker process to perform star power pathing (must be second parameter)
		ch_sp_path_worker = 1;
	}
	if((argc >= 4) && !ustricmp(argv[3], "-ch_sp_path_worker_logging"))
	{	//If this EOF instance was is being called with logging manually enabled (must be fourth parameter)
		char * fnptr = get_filename(argv[2]);	//Get a pointer to the filename of the job file path
		(void) replace_extension(temp_filename, fnptr, "", sizeof(temp_filename));	//Strip away the extension to get the worker number
		ch_sp_path_worker_number = atol(temp_filename);	//Convert it to a number
		ch_sp_path_worker_logging = 1;
	}

	set_window_title("EOF - No Song");
	if(!ch_sp_path_worker)
	{	//Don't bother setting up the sound system if EOF is acting as a worker process
		if(install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL))
		{	//If Allegro failed to initialize the sound AND midi
//			allegro_message("Can't set up MIDI!  Error: %s\nAttempting to init audio only",allegro_error);
			if(install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL))
			{
				allegro_message("Can't set up sound!  Error: %s",allegro_error);
				return 0;
			}
			eof_midi_initialized = 0;	//Couldn't set up MIDI
		}
		else
		{
			install_timer();	//Needed to use midi_out()
			eof_midi_initialized = 1;
		}
	}

	InitIdleSystem();

	retry = 5;
	while(install_keyboard())
	{	//Try to install the keyboard up to 5 times
		retry--;
		if(!retry)
		{
			allegro_message("Can't set up keyboard!  Error: %s",allegro_error);
			return 0;
		}
		Idle(1);	//Brief wait before retry
	}

	retry = 5;
	while(install_mouse() < 0)
	{	//Try to install the mouse up to 5 times
		retry--;
		if(!retry)
		{
			allegro_message("Can't set up mouse!  Error: %s",allegro_error);
			return 0;
		}
		Idle(1);	//Brief wait before retry
	}
	install_joystick(JOY_TYPE_AUTODETECT);
	alogg_detect_endianess(); // make sure OGG player works for PPC

	//Ensure the current working directory is EOF's program folder
	if(!file_exists("eof.dat", 0, NULL))
	{	//If eof.dat doesn't exist in the current working directory (ie. opening a file with EOF over command line)
		get_executable_name(temp_filename, 1024);
		(void) replace_filename(temp_filename, temp_filename, "", 1024);
		if(eof_chdir(temp_filename))
		{
			allegro_message("Could not load program data!\n%s\nMove EOF to a file path without any special (ie. accented) characters if applicable.", temp_filename);
			return 0;
		}
	}

	//Build the path to the temp subfolder
	#ifdef ALLEGRO_WINDOWS
        snprintf(eof_temp_path, sizeof(eof_temp_path) - 1, "temp");
    #else
        snprintf(eof_temp_path, sizeof(eof_temp_path) - 1, "/tmp/eof");
    #endif
	strncpy(eof_temp_path_s, eof_temp_path, sizeof(eof_temp_path_s) - 1);
	put_backslash(eof_temp_path_s);

	//Set the locale back to the default "C" locale because on Linux builds of Allegro, the locale is set to the local system locale when the keyboard system is initialized above
	(void) setlocale(LC_ALL, "C");

	//Start the logging system (unless the user disabled it via preferences)
	if(!ch_sp_path_worker || ch_sp_path_worker_logging)
	{	//Don't start logging if this is a worker process (unless the separate parameter was provided to enable logging)
		eof_start_logging();
		seconds = time(NULL);
		caltime = localtime(&seconds);
		(void) strftime(eof_log_string, sizeof(eof_log_string) - 1, "Logging started during program initialization at %c", caltime);
		eof_log(eof_log_string, 0);
		eof_log(EOF_VERSION_STRING, 0);
	}

	if(!ch_sp_path_worker)
		show_mouse(NULL);
	eof_load_config("eof.cfg");
	if((eof_log_level >= 0) && (eof_log_level <= 3))
	{	//If the logging level is valid
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Logging level set to %s", logging_level[(unsigned)eof_log_level]);
		eof_log(eof_log_string, 0);
	}
	if(eof_disable_backups)
	{
		eof_log("File backups are disabled", 1);
	}
	eof_load_chord_shape_definitions("chordshapes.xml");

	/* reset songs path */
	/* remove slash from folder name so we can check if it exists */
	if((eof_songs_path[uoffset(eof_songs_path, ustrlen(eof_songs_path) - 1)] == '\\') || (eof_songs_path[uoffset(eof_songs_path, ustrlen(eof_songs_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_songs_path[uoffset(eof_songs_path, ustrlen(eof_songs_path) - 1)] = '\0';	//Remove it
	}
	if((eof_songs_path[0] == '\0') || !file_exists(eof_songs_path, FA_RDONLY | FA_HIDDEN | FA_DIREC, NULL))
	{	//If the user-specified song folder couldn't be loaded from the config file above, or if the folder does not exist
		get_executable_name(eof_songs_path, 1024);	//Set it to EOF's program file folder
		(void) replace_filename(eof_songs_path, eof_songs_path, "", 1024);
	}
	put_backslash(eof_songs_path);	//Append a file separator if necessary

	eof_zoom_backup = eof_zoom;	//Save this because it will be over-written in eof_set_display_mode()
	if(eof_zoom_backup <= 0)
	{	//Validate eof_zoom_backup, to ensure a valid zoom level was loaded from the config file
		eof_zoom_backup = 10;
	}
	if(eof_zoom > EOF_NUM_ZOOM_LEVELS)
	{	//If the zoom level is higher than the highest preset, track this as the custom zoom level
		eof_custom_zoom_level = eof_zoom;
	}
	if(eof_desktop)
	{
		set_color_depth(desktop_color_depth() != 0 ? desktop_color_depth() : 8);
	}
	if(!eof_set_display_mode_preset_custom_width(eof_screen_layout.mode, eof_screen_width))
	{
		allegro_message("Unable to set display mode, reverting to default width!");
		if(!eof_set_display_mode_preset(eof_screen_layout.mode))
		{
			allegro_message("Unable to set display mode, reverting to default resolution!");
			eof_screen_layout.mode = 0;
			if(!eof_set_display_mode_preset(eof_screen_layout.mode))
			{
				allegro_message("Unable to set display mode!");
				return 0;
			}
		}
	}
	eof_window_info = eof_window_note_lower_left;	//By default, the info panel is at the lower left corner
	eof_menu_edit_zoom_level(eof_zoom_backup);	//Apply the zoom level loaded from the config file

	if(!eof_load_data())
	{
		return 0;
	}

	eof_init_colors();
	gui_shadow_box_proc = d_agup_shadow_box_proc;
	gui_button_proc = d_agup_button_proc;
	gui_ctext_proc = d_agup_ctext_proc;
	gui_text_list_proc = d_agup_text_list_proc;
	gui_edit_proc = d_hackish_edit_proc;

	/* divert to the Clone Hero SP pathing behavior if applicable */
	if(ch_sp_path_worker)
	{
		eof_log("Changing to SP path worker mode", 1);
		eof_ch_sp_path_worker(argv[2]);
		eof_quit = 1;	//Signal the main function to exit
		return 1;
	}

	/* create file filters */
	eof_filter_eof_files = ncdfs_filter_list_create();
	if(!eof_filter_eof_files)
	{
		allegro_message("Could not create file list filter (*.eof)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_eof_files, "eof", "EOF Files (*.eof)", 1);

	eof_filter_midi_files = ncdfs_filter_list_create();
	if(!eof_filter_midi_files)
	{
		allegro_message("Could not create file list filter (*.mid)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_midi_files, "mid;rba", "MIDI Files (*.mid, *.rba)", 1);

	eof_filter_dB_files = ncdfs_filter_list_create();
	if(!eof_filter_dB_files)
	{
		allegro_message("Could not create file list filter (*.chart)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_dB_files, "chart", "dB Chart Files (*.chart)", 1);

	eof_filter_gh_files = ncdfs_filter_list_create();
	if(!eof_filter_gh_files)
	{
		allegro_message("Could not create file list filter (*.*)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_gh_files, "*", "GH Chart Files (*.*)", 1);

	eof_filter_ghl_files = ncdfs_filter_list_create();
	if(!eof_filter_ghl_files)
	{
		allegro_message("Could not create file list filter (*.xmk)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_ghl_files, "xmk", "GHL Chart Files (*.xmk)", 1);

	eof_filter_gp_files = ncdfs_filter_list_create();
	if(!eof_filter_gp_files)
	{
		allegro_message("Could not create file list filter (*.gp5;gp4;gp3;xml)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_gp_files, "gp5;gp4;gp3;xml", "Guitar Pro (*.gp?), Go PlayAlong (*.xml)", 1);

	eof_filter_gp_lyric_text_files = ncdfs_filter_list_create();
	if(!eof_filter_gp_lyric_text_files)
	{
		allegro_message("Could not create file list filter (*.txt)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_gp_lyric_text_files, "txt", "Guitar Pro lyric text files (*.txt)", 1);

	eof_filter_rs_files = ncdfs_filter_list_create();
	if(!eof_filter_rs_files)
	{
		allegro_message("Could not create file list filter (*.xml)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_rs_files, "xml", "Rocksmith chart files (*.xml)", 1);

	eof_filter_sonic_visualiser_files = ncdfs_filter_list_create();
	if(!eof_filter_sonic_visualiser_files)
	{
		allegro_message("Could not create file list filter (*.svl)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_sonic_visualiser_files, "svl", "Sonic Visualiser files (*.svl)", 1);

	eof_filter_bf_files = ncdfs_filter_list_create();
	if(!eof_filter_bf_files)
	{
		allegro_message("Could not create file list filter (*.rif)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_bf_files, "rif", "Bandfuse chart files (*.rif)", 1);

	eof_filter_note_panel_files = ncdfs_filter_list_create();
	if(!eof_filter_note_panel_files)
	{
		allegro_message("Could not create file list filter (*.panel.txt)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_note_panel_files, "panel.txt", "Notes panel text (*.panel.txt)", 1);

	eof_filter_array_txt_files = ncdfs_filter_list_create();
	if(!eof_filter_array_txt_files)
	{
		allegro_message("Could not create file list filter (*.txt)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_array_txt_files, "txt", "Queen Bee array text (*.txt)", 1);

	/* check availability of MP3 conversion tools */
	if(!eof_supports_mp3)
	{
	#ifdef ALLEGRO_WINDOWS
		if((eof_system("lame -S check.wav >foo") == 0) && (eof_system("oggenc2 -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			(void) delete_file("check.mp3");
			(void) delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			(void) delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#elif defined(ALLEGRO_MACOSX)
		if((eof_system("lame -S check.wav >foo") == 0) && (eof_system("oggenc -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			(void) delete_file("check.mp3");
			(void) delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			(void) delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#else
		if((eof_system("lame -S check.wav >foo") == 0) && (eof_system("oggenc -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			(void) delete_file("check.wav.mp3");
			(void) delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			(void) delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#endif
	}

	/* check availability of silence generating tools */
	if(!eof_supports_oggcat)
	{
		SAMPLE *silentaudio = create_silence_sample(1);	//Create 1ms worth of silence
		if(silentaudio != NULL)
		{	//If the silence was successfully created
			(void) delete_file("silence.wav");	//Delete these temp files if they exist
			(void) delete_file("silence.ogg");
			(void) delete_file("silence2.ogg");
			(void) delete_file("silence3.ogg");
			if(save_wav("silence.wav", silentaudio) && exists("silence.wav"))
			{	//If the wave file was successfully created
				#ifdef ALLEGRO_WINDOWS
				if((eof_system("oggenc2 -o silence.ogg -b 128 silence.wav") == 0) &&
				#else
				if((eof_system("oggenc -o silence.ogg -b 128 silence.wav") == 0) &&
				#endif
					exists("silence.ogg"))
				{	//If the OGG file was succesfully created
					if(eof_copy_file("silence.ogg","silence2.ogg") && exists("silence2.ogg"))
					{	//If the OGG file was successfully duplicated
						if(!eof_system("oggCat silence3.ogg silence.ogg silence2.ogg") && exists("silence3.ogg"))
						{	//If oggCat successfully concatenated the two files
							eof_supports_oggcat = 1;
							(void) delete_file("silence3.ogg");
						}
						(void) delete_file("silence2.ogg");
					}
					(void) delete_file("silence.ogg");
				}
				(void) delete_file("silence.wav");
			}
		}
	}

	/* make music filter */
	eof_filter_music_files = ncdfs_filter_list_create();
	if(!eof_filter_music_files)
	{
		allegro_message("Could not create file list filter (*.mp3, *.ogg)!");
		return 0;
	}
	if(eof_supports_mp3)
	{
		ncdfs_filter_list_add(eof_filter_music_files, "ogg;mp3;wav", "Music Files (*.ogg, *.mp3, *.wav)", 1);
	}
	else
	{
		ncdfs_filter_list_add(eof_filter_music_files, "ogg;wav", "Music Files (*.ogg, *.wav)", 1);
	}
	eof_filter_ogg_files = ncdfs_filter_list_create();
	if(!eof_filter_ogg_files)
	{
		allegro_message("Could not create file list filter (*.ogg)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_ogg_files, "ogg", "Music Files (*.ogg)", 1);

	eof_filter_lyrics_files = ncdfs_filter_list_create();
	if(!eof_filter_lyrics_files)
	{
		allegro_message("Could not create file list filter (*.txt, *.mid, *.rmi, *.rba, *.lrc, *.vl, *.kar, *.mp3, *.srt, *.xml)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_lyrics_files, "txt;mid;rmi;rba;lrc;vl;kar;mp3;srt;xml;c9c", "Lyrics (txt,mid,rmi,rba,lrc,vl,kar,mp3,srt,xml,c9c)", 1);

	eof_filter_exe_files = ncdfs_filter_list_create();
	#ifdef ALLEGRO_WINDOWS
		if(!eof_filter_exe_files)
		{
			allegro_message("Could not create file list filter (*.exe)!");
			return 0;
		}
		ncdfs_filter_list_add(eof_filter_exe_files, "exe", "Executable Files (*.exe)", 1);
	#else
		if(!eof_filter_exe_files)
		{
			allegro_message("Could not create file list filter (*.*)!");
			return 0;
		}
		ncdfs_filter_list_add(eof_filter_exe_files, "*", "Executable Files (*.*)", 1);
	#endif

	eof_undo_reset(); // restart undo system
	memset(&eof_selection, 0, sizeof(EOF_SELECTION_DATA));
	eof_selection.current = EOF_MAX_NOTES - 1;

	eof_log("\tInitializing audio", 1);
	eof_mix_init();
	if(!eof_soft_cursor)
	{
		if(show_os_cursor(2) < 0)
		{
			eof_soft_cursor = 1;
		}
	}
	gametime_init(100); // 100hz timer

	MIDIqueue=MIDIqueuetail=NULL;	//Initialize the MIDI queue as empty
	set_volume_per_voice(0);		//By default, Allegro halves the volume of each voice so that it won't clip if played fully panned to either the left or right channels.  EOF doesn't use panning, so force full volume.
	set_volume(eof_global_volume, eof_global_volume);

	/* check for a previous crash condition of EOF */
	eof_log("\tChecking for crash recovery files", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 0;
	}

	(void) snprintf(eof_recover_on_path, sizeof(eof_recover_on_path) - 1, "%seof.recover.on", eof_temp_path_s);
	if(exists(eof_recover_on_path))
	{	//If the recovery status file is present
		(void) delete_file(eof_recover_on_path);	//Try to delete the file
		if(!exists(eof_recover_on_path))
		{	//If the file no longer exists, it is not open by another EOF instance
			(void) snprintf(eof_recover_path, sizeof(eof_recover_path) - 1, "%seof.recover", eof_temp_path_s);
			if(exists(eof_recover_path))
			{	//If the recovery file exists
				char *buffer = eof_buffer_file(eof_recover_path, 1);	//Read its contents into a NULL terminated string buffer
				char *ptr = NULL;
				if(buffer)
				{	//If the file could buffer
					(void) ustrtok(buffer, "\r\n");	//Split each line into NULL separated strings
					ptr = ustrtok(NULL, "\r\n[]");	//Get the second line (the project file path)
					if(exists(buffer) && ptr && exists(ptr))
					{	//If the recovery file contained the names of an undo file and a project file that each exist
						eof_clear_input();
						eof_use_key();
						if(alert(NULL, "Recover crashed project from last undo state?", NULL, "&Yes", "&No", 'y', 'n') == 1)
						{	//If user opts to recover from a crashed EOF instance
							eof_log("\t\tLoading last undo state", 1);
							eof_song = eof_load_song(buffer);
							if(!eof_song)
							{
								allegro_message("Unable to load last undo state. File could be corrupt!");
								return 0;
							}
							(void) ustrcpy(eof_filename, ptr);		//Set the full project path
							(void) replace_filename(eof_song_path, eof_filename, "", 1024);		//Set the song folder path
							(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);	//Set the last loaded song path
							(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));	//Set the project filename
							(void) append_filename(temp_filename, eof_song_path, eof_song->tags->ogg[0].filename, 1024);	//Construct the full OGG path
							if(!eof_load_ogg(temp_filename, 1))	//If user does not provide audio, fail over to using silent audio
							{
								allegro_message("Failed to load OGG!");
								return 0;
							}
							eof_song_loaded = 1;
							eof_chart_length = alogg_get_length_msecs_ogg_ul(eof_music_track);
							recovered = 1;	//Remember that a file was recovered so an undo state can be made after the call to eof_init_after_load()
						}

						(void) delete_file(eof_recover_path);
					}
					free(buffer);
				}
			}//If the recovery file exists
			eof_recovery = pack_fopen(eof_recover_on_path, "w");	//Open the recovery active file for writing
		}//If the file no longer exists, it is not open by another EOF instance
	}//If the recovery status file is present
	else
	{
		eof_recovery = pack_fopen(eof_recover_on_path, "w");	//Open the recovery active file for writing
	}

	/* see if we are opening a file on launch */
	for(i = 1; i < argc; i++)
	{	//For each command line argument
		if(!ustricmp(argv[i], "-newidle"))
		{
			eof_new_idle_system = 1;
		}
		else if(!eof_song_loaded)
		{	//If the argument is not one of EOF's native command line parameters and no file is loaded yet
			if(!ustricmp(get_extension(argv[i]), "eof"))
			{
				int warn = 1;	//By default, warn about differences in the INI file

				if(eof_disable_ini_difference_warnings)
					warn = 0;	//Unless the user enabled the preference to disable the warnings

				/* load the specified project */
				eof_song = eof_load_song(argv[i]);
				if(!eof_song)
				{
					allegro_message("Unable to load project. File could be corrupt!");
					return 0;
				}

				/* check song.ini and prompt user to load any external edits */
				(void) replace_filename(temp_filename, eof_song_path, "song.ini", 1024);
				(void) eof_import_ini(eof_song, temp_filename, warn);	//Read song.ini and prompt to replace values of existing settings in the project if they are different (unless user preference suppresses the prompts)

				/* attempt to load the OGG profile OGG */
				(void) append_filename(temp_filename, eof_song_path, eof_song->tags->ogg[0].filename, 1024);
				if(!eof_load_ogg(temp_filename, 1))	//If user does not provide audio, fail over to using silent audio
				{
					allegro_message("Failed to load OGG!");
					return 0;
				}
				eof_song_loaded = 1;
				eof_chart_length = alogg_get_length_msecs_ogg_ul(eof_music_track);
			}
			else if(!ustricmp(get_extension(argv[i]), "mid"))
			{
				eof_song = eof_import_midi(argv[i]);
				if(!eof_song)
				{
					allegro_message("Could not import MIDI!");
					return 0;
				}
				eof_init_after_load(0);
				if(!eof_repair_midi_import_grid_snap())
				{
					eof_log("\tGrid snap correction failed.", 1);
				}
				eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
				eof_song_enforce_mid_beat_tempo_change_removal();	//Remove mid beat tempo changes if applicable
				(void) eof_detect_difficulties(eof_song, eof_selected_track);
				eof_determine_phrase_status(eof_song, eof_selected_track);	//Update HOPO statuses
				eof_detect_mid_measure_ts_changes();
///				eof_skip_mid_beats_in_measure_numbering = 1;	//Enable mid beat tempo changes to be ignored in the measure numbering now that any applicable warnings were given
				eof_beat_stats_cached = 0;
			}
			else if(!ustricmp(get_extension(argv[i]), "rba"))
			{
				(void) replace_filename(temp_filename, argv[i], "eof_rba_import.tmp", 1024);

				if(eof_extract_rba_midi(argv[i], temp_filename) == 0)
				{	//If this was an RBA file and the MIDI was extracted successfully
					eof_song = eof_import_midi(temp_filename);
					(void) delete_file(temp_filename);	//Delete temporary file
				}
				if(!eof_song)
				{
					allegro_message("Could not import RBA!");
					return 0;
				}

				eof_init_after_load(0);
				if(!eof_repair_midi_import_grid_snap())
				{
					eof_log("\tGrid snap correction failed.", 1);
				}
				eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
				eof_song_enforce_mid_beat_tempo_change_removal();	//Remove mid beat tempo changes if applicable
				(void) eof_detect_difficulties(eof_song, eof_selected_track);
				eof_determine_phrase_status(eof_song, eof_selected_track);	//Update HOPO statuses
				eof_detect_mid_measure_ts_changes();
///				eof_skip_mid_beats_in_measure_numbering = 1;	//Enable mid beat tempo changes to be ignored in the measure numbering now that any applicable warnings were given
				eof_beat_stats_cached = 0;
			}
			else if(!ustricmp(get_extension(argv[i]), "chart"))
			{	//Import a Feedback chart via command line
				eof_song = eof_import_chart(argv[i]);
				if(!eof_song)
				{
					allegro_message("Could not import Feedback chart!");
					return 0;
				}
				eof_song_enforce_mid_beat_tempo_change_removal();	//Remove mid beat tempo changes if applicable
				eof_song_loaded = 1;
			}
			else if(strcasestr_spec(argv[i], ".pak."))
			{	//Import a Guitar Hero file via command line
				eof_song = eof_import_gh(argv[i]);
				if(!eof_song)
				{
					allegro_message("Could not import Guitar Hero .pak!");
					return 0;
				}
				eof_song_loaded = 1;
				(void) replace_filename(eof_last_gh_path, eof_filename, "", 1024);
			}
			else if(!ustricmp(get_extension(argv[i]), "rif"))
			{	//Import a Bandfuse chart via command line
				eof_song = eof_load_bf(argv[i]);
				if(!eof_song)
				{
					allegro_message("Could not import Bandfuse chart!");
					return 0;
				}
				eof_song_loaded = 1;
				(void) replace_filename(eof_last_bf_path, eof_filename, "", 1024);
			}
			else if(!ustricmp(get_extension(argv[i]), "xml"))
			{	//Import a Rocksmith arrangement or Go PlayAlong XML file via command line
				int retval ;

				retval = eof_identify_xml(argv[i]);
				if(retval == 1)
				{	//Import Rocksmith file
					if(!eof_command_line_rs_import(argv[i]))
					{	//If a new project was created and the RS file was imported successfully
						eof_log("\tImport complete", 1);
					}
					else
					{
						allegro_message("Could not import Rocksmith XML file!");
						return 0;
					}
				}
				else if(retval == 2)
				{	//Go PlayAlong file
					if(!eof_command_line_gp_import(argv[i]))
					{	//If a new project was created and the Guitar Pro file was imported successfully
						eof_log("\tImport complete", 1);
					}
					else
					{
						allegro_message("Could not import Go PlayAlong XML file!");
						return 0;
					}
				}
			}
			else if(!ustricmp(get_extension(argv[i]), "gp3") || !ustricmp(get_extension(argv[i]), "gp4") || !ustricmp(get_extension(argv[i]), "gp5"))
			{	//Import a Guitar Pro file via command line
				if(!eof_command_line_gp_import(argv[i]))
				{	//If a new project was created and the Guitar Pro file was imported successfully
					eof_log("\tImport complete", 1);
				}
				else
				{
					allegro_message("Could not import Guitar Pro file!");
					return 0;
				}
			}
			else if(!ustricmp(get_extension(argv[i]), "ogg") || !ustricmp(get_extension(argv[i]), "mp3") || !ustricmp(get_extension(argv[i]), "wav"))
			{	//Launch new chart wizard via command line
				int old_eof_disable_info_panel = eof_disable_info_panel;	//Remember this because the call to eof_new_chart() before the info panel is formally loaded will ultimately cause the call to eof_render() to forcibly disable that panel

				eof_new_chart(argv[i]);
				eof_disable_info_panel = old_eof_disable_info_panel;	//Restore the info panel status that was defined in the configuration file
			}
		}//If the argument is not one of EOF's native command line parameters and no file is loaded yet
	}//For each command line argument

	if(eof_song_loaded)
	{	//The command line load succeeded (or a project was recovered), perform some common initialization
		eof_init_after_load(0);	//Initialize variables
		if(!eof_repair_midi_import_grid_snap())
		{
			eof_log("\tGrid snap correction failed.", 1);
		}
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		show_mouse(NULL);
		if(recovered)
		{	//If a project was recovered
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
		}
	}

	//Warn about toolkit support for abridged RS2 files
	if(eof_abridged_rs2_export && !eof_abridged_rs2_export_warning_suppressed)
	{	//If abridged RS2 export is enabled and this warning hasn't been permanently dismissed
		if(alert3("Warning:  The abridged RS2 file export preference is enabled.", "These XML files are only supported in Rocksmith Custom Song Toolkit 2.8.2.0 or newer.", "Ensure you have a new enough toolkit or disable this option in File>Preferences>Import/Export", "&OK", "Don't warn again", "&OK", 'O', 0, 'O') == 2)
		{	//If the user opts to permanently suppress this warning
			eof_abridged_rs2_export_warning_suppressed = 1;
		}
	}

	//Prompt user to define the song folder path if the Mac build is in use
	#ifdef ALLEGRO_MACOSX
		if(!eof_song_folder_prompt)
		{	//If the user hasn't been prompted to set a song path yet
			char exepath[1024] = {0};

			get_executable_name(exepath, 1024);	//Set it to EOF's program file folder
			(void) replace_filename(exepath, exepath, "", 1024);
			put_backslash(exepath);	//Append a file separator if necessary

			if(!ustrcmp(eof_songs_path, exepath))
			{	//If the song path is currently set to the default of the executable file's parent folder
				int retval = alert3("The default location for new projects is with EOF's executable and that can ultimately", "be within the application bundle.  This can make it hard to find in OS X.", "Would you like to browse for a different song folder location now?", "Yes", "No", "Later", 0, 0, 0);

				if(retval == 1)
				{	//If the user opts to define the song folder
					if(eof_menu_file_song_folder())
					{	//If the user did not cancel the dialog and chose a folder
						eof_song_folder_prompt = 1;
					}
				}
				else if(retval == 2)
				{	//If the user declined to define the song folder
					eof_song_folder_prompt = 2;
				}
			}
			else
			{	//Track that a song path is already defined
				eof_song_folder_prompt = 3;
			}
		}
	#endif

	//Load FFTW wisdom from disk
	(void) fftw_import_wisdom_from_filename("FFTW.wisdom");

	//Initialize the Information panel if it is enabled
	if(!eof_disable_info_panel)
	{
		eof_disable_info_panel = 1;	//Toggle this because the function call below will toggle it back to off
		eof_display_info_panel();
	}

	//Initialize the notes panel if it was enabled via config file
	if(eof_enable_notes_panel)
	{
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
		(void) eof_display_notes_panel();
	}

	return 1;
}

void eof_exit(void)
{
	unsigned long i;
	char fn[1024] = {0}, eof_recover_on_path[50], eof_autoadjust_path[50];
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format

	eof_log("eof_exit() entered", 1);

	StopIdleSystem();

	//Delete the undo/redo related files
	(void) eof_validate_temp_folder();	//Attempt to set the current working directory if it isn't EOF's executable/resource folder

	if(!ch_sp_path_worker)
	{	//None of these are applicable if this EOF instance was a worker process
		eof_save_config("eof.cfg");
		if(!exists("eof.cfg"))
		{
			FILE *fp;
			fp = fopen("eof.cfg", "wt");	//Attempt to manually create the file
			if(!fp)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open output config file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
				eof_log(eof_log_string, 1);
			}
			else
			{
				eof_log("Allegro's config routines could not create eof.cfg, but the standard fopen() function was able to.  Please report whether subsequent EOF sessions also fail to save the configuration.", 1);
				fclose(fp);
			}
			allegro_message("Error:  Could not write config file eof.cfg.  Check logging for details.");
		}
		(void) snprintf(fn, sizeof(fn) - 1, "%seof%03u.redo", eof_temp_path_s, eof_log_id);	//Get the name of this EOF instance's redo file
		(void) delete_file(fn);	//And delete it if it exists
		(void) snprintf(fn, sizeof(fn) - 1, "%seof%03u.redo.ogg", eof_temp_path_s, eof_log_id);	//Get the name of this EOF instance's redo OGG
		(void) delete_file(fn);	//And delete it if it exists
		if(eof_undo_states_initialized > 0)
		{
			for(i = 0; i < EOF_MAX_UNDO; i++)
			{	//For each undo slot
				if(eof_undo_filename[i])
				{
					(void) delete_file(eof_undo_filename[i]);	//Delete the undo file
					(void) snprintf(fn, sizeof(fn) - 1, "%s.ogg", eof_undo_filename[i]);	//Get the filename of any associated undo OGG
					(void) delete_file(fn);	//And delete it if it exists
				}
			}
		}
		(void) snprintf(eof_autoadjust_path, sizeof(eof_autoadjust_path) - 1, "%seof.autoadjust", eof_temp_path_s);
		(void) delete_file(eof_autoadjust_path);
		eof_destroy_undo();
	}

	//Free the file filters (they will not have been set to a non NULL value if EOF launched via command line as a worker process
	if(eof_filter_music_files)
		free(eof_filter_music_files);
	if(eof_filter_ogg_files)
		free(eof_filter_ogg_files);
	if(eof_filter_midi_files)
		free(eof_filter_midi_files);
	if(eof_filter_eof_files)
		free(eof_filter_eof_files);
	if(eof_filter_exe_files)
		free(eof_filter_exe_files);
	if(eof_filter_lyrics_files)
		free(eof_filter_lyrics_files);
	if(eof_filter_dB_files)
		free(eof_filter_dB_files);
	if(eof_filter_gh_files)
		free(eof_filter_gh_files);
	if(eof_filter_ghl_files)
		free(eof_filter_ghl_files);
	if(eof_filter_gp_files)
		free(eof_filter_gp_files);
	if(eof_filter_gp_lyric_text_files)
		free(eof_filter_gp_lyric_text_files);
	if(eof_filter_rs_files)
		free(eof_filter_rs_files);
	if(eof_filter_sonic_visualiser_files)
		free(eof_filter_sonic_visualiser_files);
	if(eof_filter_bf_files)
		free(eof_filter_bf_files);
	if(eof_filter_note_panel_files)
		free(eof_filter_note_panel_files);
	if(eof_filter_array_txt_files)
		free(eof_filter_array_txt_files);

	//Free command line storage variables (for Windows build)
	#ifdef ALLEGRO_WINDOWS
	{
		int x;
		for(x = 0; x < eof_windows_argc; x++)
		{	//For each stored command line parameter
			free(eof_windows_argv[x]);
			eof_windows_argv[x] = NULL;
		}
		free(eof_windows_argv);
		eof_windows_argv = NULL;
	}
	#endif

	agup_shutdown();
	eof_mix_exit();		//Frees memory used by audio cues
	eof_destroy_data();	//Frees graphics and fonts from memory
	eof_destroy_ogg();	//Frees chart audio
	eof_destroy_song(eof_song);	//Frees memory used by any currently loaded chart
	eof_destroy_waveform(eof_waveform);	//Frees memory used by any currently loaded waveform data
	eof_waveform = NULL;
	eof_destroy_spectrogram(eof_spectrogram);	//Frees memory used by any currently loaded spectrogram data
	eof_spectrogram = NULL;
	eof_window_destroy(eof_window_editor);
	eof_window_destroy(eof_window_editor2);
	eof_window_destroy(eof_window_note_lower_left);
	eof_window_destroy(eof_window_note_upper_left);
	eof_window_destroy(eof_window_3d);
	eof_window_destroy(eof_window_notes);
	if(eof_notes_panel)
	{	//If the notes panel instance was created
		eof_destroy_text_panel(eof_notes_panel);
		eof_notes_panel = NULL;
	}
	if(eof_info_panel)
	{	//If the info panel instance was created
		eof_destroy_text_panel(eof_info_panel);
		eof_info_panel = NULL;
	}

	eof_destroy_shape_definitions();

	eof_destroy_sp_solution(eof_ch_sp_solution);	//Destroy the cached Clone Hero SP solution structure if applicable

	//Stop the logging system
	seconds = time(NULL);
	caltime = localtime(&seconds);
	(void) strftime(eof_log_string, sizeof(eof_log_string) - 1, "Logging stopped during program completion at %c", caltime);
	eof_log(eof_log_string, 1);
	eof_stop_logging();

	//Close the autorecover file if it is open, and delete the auto-recovery status file
	if(eof_recovery)
	{	//If this EOF instance is maintaining auto-recovery files
		(void) pack_fclose(eof_recovery);
		eof_recovery = NULL;
		(void) snprintf(eof_recover_on_path, sizeof(eof_recover_on_path) - 1, "%seof.recover.on", eof_temp_path_s);
		(void) delete_file(eof_recover_on_path);
	}

	//Save FFTW wisdom to disk
	(void) fftw_export_wisdom_to_filename("FFTW.wisdom");
}

void eof_all_midi_notes_off(void)
{
	eof_log("eof_all_midi_notes_off() entered", 3);

	if(midi_driver)
	{
		unsigned char ALL_NOTES_OFF[3] = {0xB1,123,0};	//Data sequence for a Control Change, controller 123, value 0 (All notes off)

		midi_driver->raw_midi(ALL_NOTES_OFF[0]);
		midi_driver->raw_midi(ALL_NOTES_OFF[1]);
		midi_driver->raw_midi(ALL_NOTES_OFF[2]);
	}
}

void eof_stop_midi(void)
{
	if(eof_midi_initialized)
	{
		eof_log("eof_stop_midi() entered", 3);

		eof_all_midi_notes_off();
	}
}

void eof_init_after_load(char initaftersavestate)
{
	unsigned long tracknum, tracknum2;

	eof_log("\tInitializing after load", 1);
	eof_log("eof_init_after_load() entered", 1);

	eof_music_paused = 1;
	if((eof_selected_track == 0) || (eof_selected_track >= eof_song->tracks))
	{	//Validate eof_selected_track, to ensure a valid track was loaded from the config file
		eof_selected_track = EOF_TRACK_GUITAR;
	}
	(void) eof_menu_track_selected_track_number(eof_selected_track, 1);
	if(eof_zoom <= 0)
	{	//Validate eof_zoom, to ensure a valid zoom level was loaded from the config file
		eof_zoom = 10;
	}
	if(eof_note_type >= eof_song->track[eof_selected_track]->numdiffs)
	{	//Validate eof_note_type, to ensure a valid active difficulty was loaded from the config file
		eof_note_type = EOF_NOTE_AMAZING;
	}
	eof_menu_edit_zoom_level(eof_zoom);
	if(!eof_song->fpbeattimes)
	{	//If floating point beat times weren't loaded from the project file
		eof_calculate_beats(eof_song);	//Rebuild the beat timings
	}
	eof_truncate_chart(eof_song);	//Add or remove beat markers as necessary and update the eof_chart_length variable
	if(eof_selected_beat >= eof_song->beats)
	{	//If the selected beat is no longer valid
		eof_selected_beat = 0;	//Select the first beat
	}
	if(!initaftersavestate)
	{	//If this wasn't cleanup after an undo/redo state, reset more variables
		eof_music_pos = eof_av_delay;
		eof_changes = 0;
		eof_undo_last_type = 0;
		eof_change_count = 0;
		eof_selected_catalog_entry = 0;
		eof_display_waveform = 0;
		eof_display_spectrogram = 0;
		eof_display_catalog = 0;		//Hide the fret catalog by default
		eof_select_beat(0);
		eof_undo_reset();
		if(eof_song->beats > 0)
			eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);	//Seek to the first beat marker
		(void) eof_menu_edit_deselect_all();	//Deselect all notes
		(void) eof_detect_tempo_map_corruption(eof_song, 0);
	}
	tracknum = eof_song->track[EOF_TRACK_DRUM]->tracknum;
	if((eof_song->track[EOF_TRACK_DRUM]->flags & EOF_TRACK_FLAG_SIX_LANES) || (eof_song->legacy_track[tracknum]->numlanes == 6))
	{	//If the normal drum track uses 6 lanes, update the PS drum track's settings to match
		tracknum2 = eof_song->track[EOF_TRACK_DRUM_PS]->tracknum;
		eof_song->track[EOF_TRACK_DRUM_PS]->flags |= eof_song->track[EOF_TRACK_DRUM]->flags;
		eof_song->legacy_track[tracknum2]->numlanes = eof_song->legacy_track[tracknum]->numlanes;
	}
	if(((eof_song->track[EOF_TRACK_DRUM]->flags & 0xF0000000) >> 24) != 0xF0)
	{	//If the PS deal drums difficulty is defined in the high nibble of the normal drum track's flags (deprecated)
		eof_song->track[EOF_TRACK_DRUM_PS]->difficulty = (eof_song->track[EOF_TRACK_DRUM]->flags & 0xF0000000) >> 28;	//Use it to set the PS drum track's difficulty
		eof_song->track[EOF_TRACK_DRUM]->flags &= ~0xF0000000;	//Clear the high nibble of the normal drum track's flags
		eof_song->track[EOF_TRACK_DRUM]->flags |= 0xF0000000;	//Set the high nibble of the normal drum track's flags of 0xF
	}
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_reset_lyric_preview_lines();
	eof_prepare_menus();
	eof_sort_notes(eof_song);
	eof_fixup_notes(eof_song);
	eof_fix_window_title();
	eof_cleanup_beat_flags(eof_song);	//Make corrections to beat statuses if necessary
	eof_sort_events(eof_song);
	eof_delete_blank_events(eof_song);
	eof_beat_stats_cached = 0;		//Mark the cached beat stats as not current
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track

	eof_log("\tInitialization after load complete", 1);
}

void eof_scale_fretboard(unsigned long numlanes)
{
	unsigned long ctr;
	double lanewidth;

	eof_log("eof_scale_fretboard() entered", 2);

	eof_screen_layout.string_space = eof_screen_layout.string_space_unscaled;

	if(!numlanes)	//If 0 was passed, find the number of lanes in the active track
	{
		numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
		if(eof_track_is_legacy_guitar(eof_song, eof_selected_track) && !eof_open_strum_enabled(eof_selected_track))
		{	//Special case:  Legacy guitar tracks can use a sixth lane but hide that lane if it is not enabled
			numlanes = 5;
		}
	}

	lanewidth = (double)eof_screen_layout.string_space * (4.0 / (numlanes-1));	//This is the correct lane width for either 5 or 6 lanes
	if(numlanes > 5)
	{	//If the active track has more than 5 lanes, scale the spacing between the fretboard lanes
		eof_screen_layout.string_space = (double)eof_screen_layout.string_space * 5.0 / (double)numlanes;
	}

	for(ctr = 0; ctr < EOF_MAX_FRETS; ctr++)
	{	//For each fretboard lane after the first is eof_screen_layout.string_space higher than the previous lane
		eof_screen_layout.note_y[ctr] = 20.0 + ((double)ctr * lanewidth) + 0.5;	//Round up to nearest pixel
	}
}

long xchart[EOF_MAX_FRETS] = {0};

void eof_set_3D_lane_positions(unsigned long track)
{
//	eof_log("eof_set_3D_lane_positions() entered");

	static unsigned long numlanes = 0;		//This remembers the number of lanes handled by the previous call
	unsigned long newnumlanes;				//This is the number of lanes in the specified track
	unsigned long numlaneswidth = 5 - 1;	//By default, the lane width will be based on a 5 lane track
	unsigned long ctr;
	double lanewidth = 0.0;

	if(!eof_song)	//If a song isn't loaded, ie. the user changed the lefty mode option with no song loaded
		return;		//Return immediately

	newnumlanes = eof_count_track_lanes(eof_song, track);	//This is the number of lanes in the specified track
	if(eof_track_is_ghl_mode(eof_song, track))
	{	//Special case:  Guitar Hero Live style tracks display with 3 lanes
		newnumlanes = 3;
		numlaneswidth = 2;
	}
	else if(eof_track_is_legacy_guitar(eof_song, track))
	{	//Special case:  Legacy guitar tracks can use a sixth lane but their 3D representation still only draws 5 lanes
		newnumlanes = 5;
	}
	else if(track && (eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && !eof_render_bass_drum_in_lane)
	{	//Special case:  The drum track renders with only 4 lanes (unless the user enabled the preference to render the bass drum in a dedicated lane)
		if(eof_five_lane_drums_enabled())
		{	//The exception is if the fifth drum lane has been enabled
			newnumlanes = 6;
			numlaneswidth = 5;	//Draw six lines on the 3D fretboard like with pro guitar
		}
		else
		{
			newnumlanes = 4;
		}
	}
	else
	{
		numlaneswidth = newnumlanes - 1;	//Otherwise, base the lane width on the number of lanes used in the track
	}

	if(track && (newnumlanes == numlanes))	//If the xchart[] array doesn't need to be updated
		return;			//Return immediately

	numlanes = newnumlanes;		//Permanently store this as the number of lanes the xchart[] array represents
	lanewidth = 56.0 * (4.0 / numlaneswidth);
	if(eof_lefty_mode)
	{
		for(ctr = 0; ctr < numlanes; ctr++)
		{	//Store the fretboard lane positions in reverse order, with respect to the number of lanes in use
			xchart[ctr] = 48 + (lanewidth * (numlanes - 1 - ctr));
		}
	}
	else
	{
		for(ctr = 0; ctr < numlanes; ctr++)
		{	//Store the fretboard lane positions in normal order
			xchart[ctr] = 48 + (lanewidth * ctr);
		}
	}
}

long ychart[EOF_MAX_FRETS] = {0};

void eof_set_2D_lane_positions(unsigned long track)
{
//	eof_log("eof_set_2D_lane_positions() entered");

	static unsigned long numlanes = 0;		//This remembers the number of lanes handled by the previous call
	unsigned long newnumlanes, newnumlanes2;
	unsigned long ctr, ctr2;

	if(!eof_song)	//If a song isn't loaded, ie. the user changed the inverted notes option with no song loaded
		return;		//Return immediately

	newnumlanes = eof_count_track_lanes(eof_song, eof_selected_track);	//Count the number of lanes in the active track
	newnumlanes2 = eof_count_track_lanes(eof_song, track);	//Count the number of lanes in that note's track
	if(eof_track_is_legacy_guitar(eof_song, track) && !eof_open_strum_enabled(track))
	{	//Special case:  Legacy guitar tracks can use a sixth lane but hide that lane if it is not enabled
		newnumlanes = 5;
	}
	if(newnumlanes > newnumlanes2)
	{	//Special case (ie. viewing an open bass guitar catalog entry when any other legacy track is active)
		newnumlanes = newnumlanes2;	//Use the number of lanes in the active track
	}

	if(track && (newnumlanes == numlanes))	//If the ychart[] array doesn't need to be updated
		return;			//Return immediately

	numlanes = newnumlanes;		//Permanently store this as the number of lanes the xchart[] array represents
	if(!track)
	{	//If track is 0, force a rebuild of the current track
		track = eof_selected_track;
	}
	if(eof_inverted_notes || (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//If the user selected the inverted notes option OR a pro guitar track is active (force inverted notes display)
		for(ctr = 0, ctr2 = 0; ctr < EOF_MAX_FRETS; ctr++)
		{	//Store the fretboard lane positions in reverse order, with respect to the number of lanes in use
			if(EOF_MAX_FRETS - ctr <= numlanes)
			{	//If this lane is used in the chart, store it in ychart[] in reverse order
				ychart[ctr2++] = eof_screen_layout.note_y[EOF_MAX_FRETS - 1 - ctr];
			}
		}
	}
	else
	{
		for(ctr = 0; ctr < EOF_MAX_FRETS; ctr++)
		{	//Store the fretboard lane positions in normal order
			ychart[ctr] = eof_screen_layout.note_y[ctr];
		}
	}
}

unsigned int eof_log_id = 0;
void eof_start_logging(void)
{
	char log_filename[1024] = {0};

	if((eof_log_fp == NULL) && eof_enable_logging)
	{	//If logging isn't alredy running, and logging isn't disabled
		srand(time(NULL));	//Seed the random number generator with the current time
		// coverity[dont_call]
		eof_log_id = ((unsigned int) rand()) % 1000;	//Create a 3 digit random number to represent this EOF instance
		get_executable_name(log_filename, 1024);	//Get the path of the EOF binary that is running
		if(ch_sp_path_worker_logging)
		{	//If this is a worker process, use the process number in the log file name
			char tempstr[10];

			(void) snprintf(tempstr, sizeof(tempstr) - 1, "%lu.log", ch_sp_path_worker_number);
			(void) replace_filename(log_filename, log_filename, tempstr, 1024);
		}
		else
		{	//Normal EOF process
			(void) replace_filename(log_filename, log_filename, "eof_log.txt", 1024);
		}
		eof_log_fp = fopen(log_filename, "w");

		if(eof_log_fp == NULL)
		{
			#ifdef ALLEGRO_MACOSX
				allegro_message("Error opening log file for writing.  Move EOF to a file path without any special (ie. accented) characters if applicable, and also move outside of the Downloads folder.");
			#else
				allegro_message("Error opening log file for writing.  Move EOF to a file path without any special (ie. accented) characters if applicable.");
			#endif
		}
		#ifdef ALLEGRO_WINDOWS
		{
			char *systemdrive, programfilespath[30] = "";	//Used to obtain the x86 and x64 Program Files folders via environment variables
			systemdrive = getenv("SystemDrive");
			if(systemdrive)
			{	//If the SystemDrive environment variable was read, construct the expected path to the program files folder
				// (the environment variable can't be relied on for this because the 32 and 64 bit variables will return only
				//  the 32 bit path when the running executable is 32 bit)
				(void) snprintf(programfilespath, sizeof(programfilespath) - 1, "%s\\Program Files", systemdrive);
			}
			if(strcasestr_spec(log_filename, "VirtualStore") || (systemdrive && strcasestr_spec(log_filename, programfilespath)))
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tDetected program files path:  %s", programfilespath);
				eof_log(eof_log_string, 1);
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tOutput log path:  %s", log_filename);
				eof_log(eof_log_string, 1);
				allegro_message("Warning:  Running EOF from within a \"Program files\" folder in Windows Vista or newer can cause problems.  Moving the EOF program folder elsewhere is recommended.\"");
			}
		}
		#endif
	}
}

void eof_stop_logging(void)
{
	if(eof_log_fp)
	{
		(void) fclose(eof_log_fp);
		eof_log_fp = NULL;
	}
}

#define EOF_LOG_STRING_SIZE 2048
char eof_log_string[EOF_LOG_STRING_SIZE] = {0};
void eof_log(const char *text, int level)
{
	if(text && eof_log_fp && (eof_log_level >= level))
	{	//If the log file is open and the current logging level is high enough
		if(fprintf(eof_log_fp, "%03u: %s\n", eof_log_id, text) > 0)	//Prefix the log text with this EOF instance's logging ID
		{	//If the log line was successfully written
			(void) fflush(eof_log_fp);	//Explicitly commit the write to disk
		}
	}
}

void eof_log_casual(const char *text, int level, int prefix, int newline)
{
	static char buffer[EOF_LOG_STRING_SIZE * 2] = {0};
	static char buffer2[EOF_LOG_STRING_SIZE];
	static unsigned long buffered_chars = 0;
	size_t length;
	int retval;

	if(eof_log_fp)
	{	//If the log file is open
		if(!text)
		{	//If the input string is NULL, flush buffer to disk
			(void) fputs(buffer, eof_log_fp);	//Write the buffer to file
			(void) fflush(eof_log_fp);			//Explicitly commit the write to disk
			buffer[0] = '\0';	//Empty the buffers
			buffer2[0] = '\0';
			buffered_chars = 0;
		}
		else if(eof_log_fp && (eof_log_level >= level))
		{	//Otherwise if the input string is not NULL and the logging level is high enough, log to buffer
			buffer2[0] = '\0';
			if(prefix)
			{	//If the log ID is to be prefixed to the output string
				if(newline)
				{	//If a newline character is also to be appended
					retval = snprintf(buffer2, sizeof(buffer2) - 1, "%03u: %s\n", eof_log_id, text);
				}
				else
				{	//Just the log ID
					retval = snprintf(buffer2, sizeof(buffer2) - 1, "%03u: %s", eof_log_id, text);
				}
			}
			else
			{	//No log ID
				if(newline)
				{	//Just the newline character suffix
					retval = snprintf(buffer2, sizeof(buffer2) - 1, "%s\n", text);
				}
				else
				{	//Just the provided text
					retval = snprintf(buffer2, sizeof(buffer2) - 1, "%s", text);
				}
			}
			if(retval > 0)
			{	//If the string was written to the temporary buffer
				length = strlen(buffer2);
				if(buffered_chars + length >= sizeof(buffer))
				{	//If the buffer isn't large enough to hold this string
					(void) fputs(buffer, eof_log_fp);	//Flush the buffer to file
					buffer[0] = '\0';	//Empty the buffer
					buffered_chars = 0;
				}
				strncat(buffer, buffer2, sizeof(buffer) - 1);
				buffered_chars += length;	//Track the number of characters in the buffer
			}
		}
	}
}

void eof_log_notes(EOF_SONG *sp, unsigned long track)
{
	unsigned long ctr;

	if((sp == NULL) || (track >= sp->tracks))
		return;

	eof_log("Dumping note timings:", 1);
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNote #%lu: Start = %lums\tLength = %ldms", ctr, eof_get_note_pos(sp, track, ctr), eof_get_note_length(sp, track, ctr));
		eof_log(eof_log_string, 1);
	}
}

void eof_log_cwd(void)
{
	char cwd[1024];
	if(!getcwd(cwd, 1024))
	{	//Couldn't obtain current working directory
		eof_log("\tCould not detect working directory", 1);
		return;
	}
	put_backslash(cwd);	//Append a file separator if necessary
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tDetected working directory:  %s", cwd);
	eof_log(eof_log_string, 1);
}

#ifdef ALLEGRO_WINDOWS
	int eof_initialize_windows(void)
	{
		int i, retval;
		wchar_t * cl = GetCommandLineW();

		eof_windows_internal_argv = CommandLineToArgvW(cl, &eof_windows_argc);
		eof_windows_argv = malloc(eof_windows_argc * sizeof(char *));
		for(i = 0; i < eof_windows_argc; i++)
		{
			eof_windows_argv[i] = malloc(1024 * sizeof(char));
			if(eof_windows_argv[i] == NULL)
			{	//If the allocation failed
				while(i > 0)
				{	//Free all the previously allocated argument strings
					free(eof_windows_argv[i - 1]);
					i--;
				}
				free(eof_windows_argv);
				return 0;	//Return failure
			}
			memset(eof_windows_argv[i], 0, 1024);
			(void) uconvert((char *)eof_windows_internal_argv[i], U_UNICODE, eof_windows_argv[i], U_UTF8, 4096);
		}
		retval = eof_initialize(eof_windows_argc, eof_windows_argv);
		if(!retval)
		{	//EOF failed to initialize
			for(i = 0; i < eof_windows_argc; i++)
			{
				free(eof_windows_argv[i]);
			}
			free(eof_windows_argv);
		}
		return retval;
	}
#endif

/* use to prevent 100% CPU usage */
static void eof_idle_logic(void)
{
	if(eof_new_idle_system)
	{	//If the newer idle system was enabled via command line
		/* rest to save CPU */
		if(eof_has_focus)
		{
			#ifndef ALLEGRO_WINDOWS
				Idle(eof_cpu_saver * 5);
			#else
				if(eof_disable_vsync)
				{
					Idle(eof_cpu_saver * 5);
				}
			#endif
		}

		/* make program "sleep" until it is back in focus */
		else
		{
			Idle(500);
			gametime_reset();
		}
	}
	else
	{	//If the normal idle system is in effect
		/* rest to save CPU */
		if(eof_has_focus)
		{
			rest(eof_cpu_saver * 5);
		}

		/* make program "sleep" until it is back in focus */
		else
		{
			rest(500);
			gametime_reset();
		}
	}
}

int main(int argc, char * argv[])
{
	#ifdef ALLEGRO_LEGACY
		ALLEGRO_TIMER * timer = NULL;
		ALLEGRO_EVENT_QUEUE * queue = NULL;
		ALLEGRO_EVENT event;
		int tick_count = 0;
		int tick_limit = 5;
	#endif
	int updated = 0, init_failed = 0;
	clock_t time1 = 0, time2 = 0;

	#ifdef ALLEGRO_WINDOWS
		if(!eof_initialize_windows())
		{
			eof_quit = init_failed = 1;
		}
	#else
		if(!eof_initialize(argc, argv))
		{
			eof_quit = init_failed = 1;
		}
	#endif

	#ifdef ALLEGRO_LEGACY
		timer = al_create_timer(1.0 / 100.0);
		if(!timer)
		{
			eof_log("Failed to create Allegro 5 timer", 1);
			return -1;
		}
		queue = al_create_event_queue();
		if(!queue)
		{
			eof_log("Failed to create Allegro 5 event queue", 1);
			return -1;
		}
		al_register_event_source(queue, al_get_timer_event_source(timer));
		al_start_timer(timer);
	#endif

	eof_log("\tEntering main program loop", 1);

	time1 = clock();
	while(!eof_quit)
	{	//While EOF isn't meant to quit
		/* frame skip mode */
		#ifdef ALLEGRO_LEGACY
			tick_count = 0;
			while(!al_is_event_queue_empty(queue))
			{
				al_get_next_event(queue, &event);
				tick_count++;
				if(tick_count < tick_limit)
				{
					eof_logic();
				}
				updated = 0;
			}
		#else
			while(gametime_get_frames() - gametime_tick > 0)
			{
				eof_logic();
				updated = 0;
				++gametime_tick;
			}
		#endif

		/* update and draw the screen */
		if(!updated)
		{
			eof_render();
			updated = 1;

			#define EOF_FPS_SAMPLE_COUNT 10
			eof_main_loop_ctr++;	//Count the number of calls to eof_render in this loop
			if(eof_main_loop_ctr >= EOF_FPS_SAMPLE_COUNT)
			{	//Every 5 renders, detect the number of rendered frames per second
				time2 = clock();
				if(time2 > time1)
				{	//Verify these values are different in order to avoid a division by zero
					eof_main_loop_fps = (double)CLOCKS_PER_SEC / ((double)time2 - time1) * EOF_FPS_SAMPLE_COUNT;
					time1 = time2;
					eof_main_loop_ctr = 0;
				}
			}
		}

		/* update the music */
		if(!eof_music_paused)
		{	//Chart is not paused
			int ret = alogg_poll_ogg(eof_music_track);
			eof_music_actual_pos = alogg_get_pos_msecs_ogg_ul(eof_music_track);
			eof_play_queued_midi_tones();	//Played cued MIDI tones for pro guitar/bass notes
			if((ret == ALOGG_POLL_PLAYJUSTFINISHED) && !eof_rewind_at_end)
			{	//If the end of the chart has been reached during playback, and the user did not want the chart to automatically rewind
				(void) eof_menu_song_seek_end();	//Re-seek to end of the audio
				eof_music_paused = 1;
			}
			else if((ret == ALOGG_POLL_PLAYJUSTFINISHED) || (ret == ALOGG_POLL_NOTPLAYING) || (ret == ALOGG_POLL_FRAMECORRUPT) || (ret == ALOGG_POLL_INTERNALERROR) || (eof_music_actual_pos > alogg_get_length_msecs_ogg_ul(eof_music_track)))
			{	//Otherwise if ALOGG reported a completed/error condition or if the reported position is greater than the length of the audio
				eof_music_pos = eof_music_actual_pos + eof_av_delay;
				eof_music_paused = 1;
			}
			else
			{
				if(eof_smooth_pos)
				{
					if((eof_music_actual_pos > eof_music_pos) || eof_music_paused)
					{
						eof_music_pos = eof_music_actual_pos;
					}
				}
			}
		}

		/* update the music for playing a portion from catalog */
		else if(eof_music_catalog_playback)
		{	//Catalog is playing
			int ret = alogg_poll_ogg(eof_music_track);
			if(ret == ALOGG_POLL_PLAYJUSTFINISHED)
			{
				eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
				eof_music_catalog_playback = 0;
			}
			else if((ret == ALOGG_POLL_NOTPLAYING) || (ret == ALOGG_POLL_FRAMECORRUPT) || (ret == ALOGG_POLL_INTERNALERROR))
			{
				eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
				eof_music_catalog_playback = 0;
			}
			else
			{
				unsigned long ap = alogg_get_pos_msecs_ogg_ul(eof_music_track);
				if((ap > eof_music_catalog_pos) || !eof_music_catalog_playback)
				{
					eof_music_catalog_pos = ap;
				}
			}
		}
		else
		{	//Chart is paused
			#ifndef ALLEGRO_LEGACY
				eof_idle_logic();
			#endif
		}//Chart is paused
	}//While EOF isn't meant to quit
	if(!init_failed)
	{	//If EOF was previously able to initialize
		#ifdef ALLEGRO_LEGACY
			al_destroy_event_queue(queue);
			al_destroy_timer(timer);
		#endif
		eof_exit();
	}
	return 0;
}

int eof_beat_is_mandatory_anchor(EOF_SONG *sp, unsigned long beat)
{
	unsigned den = 0, den2 = 0;

	if(!sp || (beat >= sp->beats) || (sp->beats < 2))
		return 0;

	if(beat < 1)
		return 1;	//The first beat is always an anchor

	if(sp->beat[beat - 1]->ppqn != sp->beat[beat]->ppqn)
		return 1;	//This beat has a different tempo than the previous one

	if(eof_get_effective_ts(sp, NULL, &den, beat - 1, 0))
	{	//If the time signature in effect at the previous beat was able to be found
		if(eof_get_ts(sp, NULL, &den2, beat) == 1)
		{	//If there is a time signature change at this beat
			if(den != den2)
			{	//If the denominator changed
				return 1;	//This beat has a different beat unit than the previous one
			}
		}
	}

	return 0;	//This beat isn't required to be an anchor
}

void eof_cleanup_beat_flags(EOF_SONG *sp)
{
	unsigned long ctr;

	eof_log("eof_cleanup_beat_flags() entered", 1);

	if(sp == NULL)
		return;

	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat
		sp->beat[ctr]->flags &= ~(EOF_BEAT_FLAG_EVENTS);	//Clear the events flag
	}

	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event
		if(!(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(sp->text_event[ctr]->pos >= sp->beats)
			{	//If the text event is assigned to an invalid beat number
				sp->text_event[ctr]->pos = sp->beats - 1;	//Re-assign to the last beat marker
			}
			sp->beat[sp->text_event[ctr]->pos]->flags |= EOF_BEAT_FLAG_EVENTS;	//Ensure the text event status flag is set
		}
	}

	if(sp->beats < 1)
		return;

	sp->beat[0]->flags |= EOF_BEAT_FLAG_ANCHOR;	//The first beat marker is required to be an anchor
	for(ctr = 1; ctr < sp->beats; ctr++)
	{	//For each beat after the first
		if(eof_beat_is_mandatory_anchor(sp, ctr))
		{	//If this beat must be an anchor due to a tempo or TS change
			sp->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Ensure the anchor status flag is set
		}
	}
}

char eof_color_green_name[] = "&Green";
char eof_color_red_name[] = "&Red";
char eof_color_yellow_name[] = "&Yellow";
char eof_color_blue_name[] = "&Blue";
char eof_color_orange_name[] = "&Orange";
char eof_color_purple_name[] = "&Purple";
char eof_color_black_name[] = "B&Lack";
char eof_color_white_name[] = "&White";

char eof_lane_1_name[] = "Lane &1";
char eof_lane_2_name[] = "Lane &2";
char eof_lane_3_name[] = "Lane &3";
char eof_lane_4_name[] = "Lane &4";
char eof_lane_5_name[] = "Lane &5";
char eof_lane_6_name[] = "Lane &6";

char eof_note_toggle_menu_string_1[20] = "";	//These strings are used in the eof_note_toggle_menu[] array
char eof_note_toggle_menu_string_2[20] = "";
char eof_note_toggle_menu_string_3[20] = "";
char eof_note_toggle_menu_string_4[20] = "";
char eof_note_toggle_menu_string_5[20] = "";
char eof_note_toggle_menu_string_6[20] = "";
char *eof_note_toggle_menu_strings[] = {eof_note_toggle_menu_string_1, eof_note_toggle_menu_string_2, eof_note_toggle_menu_string_3, eof_note_toggle_menu_string_4, eof_note_toggle_menu_string_5, eof_note_toggle_menu_string_6};

char eof_note_clear_menu_string_1[20] = "";	//These strings are used in the eof_note_clear_menu[] array
char eof_note_clear_menu_string_2[20] = "";
char eof_note_clear_menu_string_3[20] = "";
char eof_note_clear_menu_string_4[20] = "";
char eof_note_clear_menu_string_5[20] = "";
char eof_note_clear_menu_string_6[20] = "";
char *eof_note_clear_menu_strings[] = {eof_note_clear_menu_string_1, eof_note_clear_menu_string_2, eof_note_clear_menu_string_3, eof_note_clear_menu_string_4, eof_note_clear_menu_string_5, eof_note_clear_menu_string_6};

void eof_init_colors(void)
{
	eof_log("eof_init_colors() entered", 1);

	//Init green
	eof_color_green_struct.color = eof_color_green;
	eof_color_green_struct.hit = makecol(192, 255, 192);
	eof_color_green_struct.lightcolor = makecol(170,255,170);
	eof_color_green_struct.border = eof_color_red;
	eof_color_green_struct.note3d = EOF_IMAGE_NOTE_GREEN;
	eof_color_green_struct.notehit3d = EOF_IMAGE_NOTE_GREEN_HIT;
	eof_color_green_struct.hoponote3d = EOF_IMAGE_NOTE_HGREEN;
	eof_color_green_struct.hoponotehit3d = EOF_IMAGE_NOTE_HGREEN_HIT;
	eof_color_green_struct.cymbal3d = EOF_IMAGE_NOTE_GREEN_CYMBAL;
	eof_color_green_struct.cymbalhit3d = EOF_IMAGE_NOTE_GREEN_CYMBAL_HIT;
	eof_color_green_struct.arrow3d = EOF_IMAGE_NOTE_GREEN_ARROW;
	eof_color_green_struct.arrowhit3d = EOF_IMAGE_NOTE_GREEN_ARROW_HIT;
	eof_color_green_struct.slider3d = EOF_IMAGE_NOTE_GREEN_SLIDER;
	eof_color_green_struct.colorname = eof_color_green_name;
	//Init red
	eof_color_red_struct.color = eof_color_red;
	eof_color_red_struct.hit = makecol(255, 192, 192);
	eof_color_red_struct.lightcolor = makecol(255,156,156);
	eof_color_red_struct.border = eof_color_white;
	eof_color_red_struct.note3d = EOF_IMAGE_NOTE_RED;
	eof_color_red_struct.notehit3d = EOF_IMAGE_NOTE_RED_HIT;
	eof_color_red_struct.hoponote3d = EOF_IMAGE_NOTE_HRED;
	eof_color_red_struct.hoponotehit3d = EOF_IMAGE_NOTE_HRED_HIT;
	eof_color_red_struct.cymbal3d = EOF_IMAGE_NOTE_RED;
	eof_color_red_struct.cymbalhit3d = EOF_IMAGE_NOTE_RED_HIT;
	eof_color_red_struct.arrow3d = EOF_IMAGE_NOTE_RED_ARROW;
	eof_color_red_struct.arrowhit3d = EOF_IMAGE_NOTE_RED_ARROW_HIT;
	eof_color_red_struct.slider3d = EOF_IMAGE_NOTE_RED_SLIDER;
	eof_color_red_struct.colorname = eof_color_red_name;
	//Init yellow
	eof_color_yellow_struct.color = eof_color_yellow;
	eof_color_yellow_struct.hit = makecol(255, 255, 192);
	eof_color_yellow_struct.lightcolor = makecol(255,255,224);
	eof_color_yellow_struct.border = eof_color_red;
	eof_color_yellow_struct.note3d = EOF_IMAGE_NOTE_YELLOW;
	eof_color_yellow_struct.notehit3d = EOF_IMAGE_NOTE_YELLOW_HIT;
	eof_color_yellow_struct.hoponote3d = EOF_IMAGE_NOTE_HYELLOW;
	eof_color_yellow_struct.hoponotehit3d = EOF_IMAGE_NOTE_HYELLOW_HIT;
	eof_color_yellow_struct.cymbal3d = EOF_IMAGE_NOTE_YELLOW_CYMBAL;
	eof_color_yellow_struct.cymbalhit3d = EOF_IMAGE_NOTE_YELLOW_CYMBAL_HIT;
	eof_color_yellow_struct.arrow3d = EOF_IMAGE_NOTE_YELLOW_ARROW;
	eof_color_yellow_struct.arrowhit3d = EOF_IMAGE_NOTE_YELLOW_ARROW_HIT;
	eof_color_yellow_struct.slider3d = EOF_IMAGE_NOTE_YELLOW_SLIDER;
	eof_color_yellow_struct.colorname = eof_color_yellow_name;
	//Init blue
	eof_color_blue_struct.color = eof_color_blue;
	eof_color_blue_struct.hit = makecol(192, 192, 255);
	eof_color_blue_struct.lightcolor = makecol(156,156,255);
	eof_color_blue_struct.border = eof_color_white;
	eof_color_blue_struct.note3d = EOF_IMAGE_NOTE_BLUE;
	eof_color_blue_struct.notehit3d = EOF_IMAGE_NOTE_BLUE_HIT;
	eof_color_blue_struct.hoponote3d = EOF_IMAGE_NOTE_HBLUE;
	eof_color_blue_struct.hoponotehit3d = EOF_IMAGE_NOTE_HBLUE_HIT;
	eof_color_blue_struct.cymbal3d = EOF_IMAGE_NOTE_BLUE_CYMBAL;
	eof_color_blue_struct.cymbalhit3d = EOF_IMAGE_NOTE_BLUE_CYMBAL_HIT;
	eof_color_blue_struct.arrow3d = EOF_IMAGE_NOTE_BLUE_ARROW;
	eof_color_blue_struct.arrowhit3d = EOF_IMAGE_NOTE_BLUE_ARROW_HIT;
	eof_color_blue_struct.slider3d = EOF_IMAGE_NOTE_BLUE_SLIDER;
	eof_color_blue_struct.colorname = eof_color_blue_name;
	//Init orange
	eof_color_orange_struct.color = eof_color_orange;
	eof_color_orange_struct.hit = makecol(255, 192, 0);
	eof_color_orange_struct.lightcolor = makecol(255,170,128);
	eof_color_orange_struct.border = eof_color_white;
	eof_color_orange_struct.note3d = EOF_IMAGE_NOTE_ORANGE;
	eof_color_orange_struct.notehit3d = EOF_IMAGE_NOTE_ORANGE_HIT;
	eof_color_orange_struct.hoponote3d = EOF_IMAGE_NOTE_HORANGE;
	eof_color_orange_struct.hoponotehit3d = EOF_IMAGE_NOTE_HORANGE_HIT;
	eof_color_orange_struct.cymbal3d = EOF_IMAGE_NOTE_ORANGE_CYMBAL;
	eof_color_orange_struct.cymbalhit3d = EOF_IMAGE_NOTE_ORANGE_CYMBAL_HIT;
	eof_color_orange_struct.arrow3d = EOF_IMAGE_NOTE_ORANGE;
	eof_color_orange_struct.arrowhit3d = EOF_IMAGE_NOTE_ORANGE_HIT;
	eof_color_orange_struct.slider3d = EOF_IMAGE_NOTE_ORANGE_SLIDER;
	eof_color_orange_struct.colorname = eof_color_orange_name;
	//Init purple
	eof_color_purple_struct.color = eof_color_purple;
	eof_color_purple_struct.hit = makecol(255, 192, 255);
	eof_color_purple_struct.lightcolor = makecol(255,156,255);
	eof_color_purple_struct.border = eof_color_white;
	eof_color_purple_struct.note3d = EOF_IMAGE_NOTE_PURPLE;
	eof_color_purple_struct.notehit3d = EOF_IMAGE_NOTE_PURPLE_HIT;
	eof_color_purple_struct.hoponote3d = EOF_IMAGE_NOTE_HPURPLE;
	eof_color_purple_struct.hoponotehit3d = EOF_IMAGE_NOTE_HPURPLE_HIT;
	eof_color_purple_struct.cymbal3d = EOF_IMAGE_NOTE_PURPLE_CYMBAL;
	eof_color_purple_struct.cymbalhit3d = EOF_IMAGE_NOTE_PURPLE_CYMBAL_HIT;
	eof_color_purple_struct.arrow3d = EOF_IMAGE_NOTE_PURPLE;
	eof_color_purple_struct.arrowhit3d = EOF_IMAGE_NOTE_PURPLE_HIT;
	eof_color_purple_struct.slider3d = EOF_IMAGE_NOTE_PURPLE_SLIDER;
	eof_color_purple_struct.colorname = eof_color_purple_name;
	//Init black
	eof_color_black_struct.color = makecol(51, 51, 51);
	eof_color_black_struct.hit = makecol(67, 67, 67);
	eof_color_black_struct.lightcolor = eof_color_silver;
	eof_color_black_struct.border = eof_color_green;
	eof_color_black_struct.note3d = EOF_IMAGE_NOTE_BLACK;
	eof_color_black_struct.notehit3d = EOF_IMAGE_NOTE_BLACK_HIT;
	eof_color_black_struct.hoponote3d = EOF_IMAGE_NOTE_BLACK;
	eof_color_black_struct.hoponotehit3d = EOF_IMAGE_NOTE_BLACK_HIT;
	eof_color_black_struct.cymbal3d = EOF_IMAGE_NOTE_BLACK;
	eof_color_black_struct.cymbalhit3d = EOF_IMAGE_NOTE_BLACK_HIT;
	eof_color_black_struct.arrow3d = EOF_IMAGE_NOTE_BLACK;
	eof_color_black_struct.arrowhit3d = EOF_IMAGE_NOTE_BLACK_HIT;
	eof_color_black_struct.slider3d = EOF_IMAGE_NOTE_BLACK_SLIDER;
	eof_color_black_struct.colorname = eof_color_black_name;
	//Init ghl black
	eof_color_ghl_black_struct.color = makecol(51, 51, 51);
	eof_color_ghl_black_struct.hit = makecol(67, 67, 67);
	eof_color_ghl_black_struct.lightcolor = eof_color_silver;
	eof_color_ghl_black_struct.border = eof_color_white;
	eof_color_ghl_black_struct.note3d = EOF_IMAGE_NOTE_GHL_BLACK;
	eof_color_ghl_black_struct.notehit3d = EOF_IMAGE_NOTE_GHL_BLACK_HIT;
	eof_color_ghl_black_struct.hoponote3d = EOF_IMAGE_NOTE_GHL_BLACK_HOPO;
	eof_color_ghl_black_struct.hoponotehit3d = EOF_IMAGE_NOTE_GHL_BLACK_HOPO_HIT;
	eof_color_ghl_black_struct.cymbal3d = EOF_IMAGE_NOTE_GHL_BLACK;
	eof_color_ghl_black_struct.cymbalhit3d = EOF_IMAGE_NOTE_GHL_BLACK_HIT;
	eof_color_ghl_black_struct.arrow3d = EOF_IMAGE_NOTE_GHL_BLACK;
	eof_color_ghl_black_struct.arrowhit3d = EOF_IMAGE_NOTE_GHL_BLACK_HIT;
	eof_color_ghl_black_struct.slider3d = EOF_IMAGE_NOTE_GHL_BLACK_SLIDER;
	eof_color_ghl_black_struct.colorname = eof_color_black_name;
	//Init ghl white
	eof_color_ghl_white_struct.color = eof_color_white;
	eof_color_ghl_white_struct.hit = eof_color_light_gray;
	eof_color_ghl_white_struct.lightcolor = eof_color_silver;
	eof_color_ghl_white_struct.border = eof_color_blue;
	eof_color_ghl_white_struct.note3d = EOF_IMAGE_NOTE_GHL_WHITE;
	eof_color_ghl_white_struct.notehit3d = EOF_IMAGE_NOTE_GHL_WHITE_HIT;
	eof_color_ghl_white_struct.hoponote3d = EOF_IMAGE_NOTE_GHL_WHITE_HOPO;
	eof_color_ghl_white_struct.hoponotehit3d = EOF_IMAGE_NOTE_GHL_WHITE_HOPO_HIT;
	eof_color_ghl_white_struct.cymbal3d = EOF_IMAGE_NOTE_GHL_WHITE;
	eof_color_ghl_white_struct.cymbalhit3d = EOF_IMAGE_NOTE_GHL_WHITE_HIT;
	eof_color_ghl_white_struct.arrow3d = EOF_IMAGE_NOTE_GHL_WHITE;
	eof_color_ghl_white_struct.arrowhit3d = EOF_IMAGE_NOTE_GHL_WHITE_HIT;
	eof_color_ghl_white_struct.slider3d = EOF_IMAGE_NOTE_GHL_WHITE_SLIDER;
	eof_color_ghl_white_struct.colorname = eof_color_white_name;
	//Init lane 1 (will be used to represent open fingering in Bandfuse color mode)
	eof_lane_1_struct = eof_color_purple_struct;
	eof_lane_1_struct.colorname = eof_lane_1_name;
	//Init lane 2 (will be used to represent index fingering in Bandfuse color mode)
	eof_lane_2_struct = eof_color_green_struct;
	eof_lane_2_struct.colorname = eof_lane_2_name;
	//Init lane 3 (will be used to represent middle fingering in Bandfuse color mode)
	eof_lane_3_struct = eof_color_red_struct;
	eof_lane_3_struct.colorname = eof_lane_3_name;
	//Init lane 4 (will be used to represent ring fingering in Bandfuse color mode)
	eof_lane_4_struct = eof_color_yellow_struct;
	eof_lane_4_struct.colorname = eof_lane_4_name;
	//Init lane 5 (will be used to represent pinky fingering in Bandfuse color mode)
	eof_lane_5_struct = eof_color_blue_struct;
	eof_lane_5_struct.colorname = eof_lane_5_name;
	//Init lane 6 (will be used to represent thumb fingering in Bandfuse color mode)
	eof_lane_6_struct = eof_color_orange_struct;
	eof_lane_6_struct.colorname = eof_lane_6_name;
}

void eof_set_color_set(void)
{
	int x;

	eof_log("eof_set_color_set() entered", 2);

	if(!eof_song)
		return;

	if(eof_track_is_ghl_mode(eof_song, eof_selected_track))
	{	//Guitar Hero Live only uses two gem colors
		eof_colors[0] = eof_color_ghl_black_struct;		//Lane 1 is B1
		eof_colors[1] = eof_color_ghl_black_struct;
		eof_colors[2] = eof_color_ghl_black_struct;
		eof_colors[3] = eof_color_ghl_white_struct;		//Lane 4 is W1
		eof_colors[4] = eof_color_ghl_white_struct;
		eof_colors[5] = eof_color_ghl_white_struct;
	}
	else if(eof_color_set == EOF_COLORS_DEFAULT)
	{	//If user is using the original EOF color set
		eof_colors[0] = eof_color_green_struct;
		eof_colors[1] = eof_color_red_struct;
		eof_colors[2] = eof_color_yellow_struct;
		eof_colors[3] = eof_color_blue_struct;
		eof_colors[4] = eof_color_purple_struct;
		eof_colors[5] = eof_color_orange_struct;
	}
	else if((eof_color_set == EOF_COLORS_RB) || (eof_color_set == EOF_COLORS_RS) || (eof_color_set == EOF_COLORS_BF))
	{	//If user is using the Rock Band, Rocksmith or Bandfuse color sets
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			eof_colors[0] = eof_color_orange_struct;
			eof_colors[1] = eof_color_red_struct;
			eof_colors[2] = eof_color_yellow_struct;
			eof_colors[3] = eof_color_blue_struct;
			eof_colors[4] = eof_color_green_struct;
			eof_colors[5] = eof_color_purple_struct;
		}
		else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar/bass track is active
			if(eof_color_set == EOF_COLORS_RB)
			{	//If the user is using the Rock Band color set
				eof_colors[0] = eof_color_red_struct;
				eof_colors[1] = eof_color_green_struct;
				eof_colors[2] = eof_color_orange_struct;
				eof_colors[3] = eof_color_blue_struct;
				eof_colors[4] = eof_color_yellow_struct;
				eof_colors[5] = eof_color_purple_struct;
			}
			else if(eof_color_set == EOF_COLORS_BF)
			{	//If the user is using the Bandfuse color set
				eof_colors[0] = eof_lane_1_struct;
				eof_colors[1] = eof_lane_2_struct;
				eof_colors[2] = eof_lane_3_struct;
				eof_colors[3] = eof_lane_4_struct;
				eof_colors[4] = eof_lane_5_struct;
				eof_colors[5] = eof_lane_6_struct;
			}
			else
			{	//The user is using the Rocksmith color set
				eof_colors[0] = eof_color_red_struct;
				eof_colors[1] = eof_color_yellow_struct;
				eof_colors[2] = eof_color_blue_struct;
				eof_colors[3] = eof_color_orange_struct;
				eof_colors[4] = eof_color_green_struct;
				eof_colors[5] = eof_color_purple_struct;
			}
		}
		else
		{	//All other tracks use the generic Rock Band color set
			eof_colors[0] = eof_color_green_struct;
			eof_colors[1] = eof_color_red_struct;
			eof_colors[2] = eof_color_yellow_struct;
			eof_colors[3] = eof_color_blue_struct;
			eof_colors[4] = eof_color_orange_struct;
			eof_colors[5] = eof_color_purple_struct;
		}
	}
	else if(eof_color_set == EOF_COLORS_GH)
	{	//If user is using the Guitar Hero color set
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			eof_colors[0] = eof_color_purple_struct;
			eof_colors[1] = eof_color_red_struct;
			eof_colors[2] = eof_color_yellow_struct;
			eof_colors[3] = eof_color_blue_struct;
			eof_colors[4] = eof_color_orange_struct;
			eof_colors[5] = eof_color_green_struct;
		}
		else
		{	//All other tracks use the generic Guitar Hero color set
			eof_colors[0] = eof_color_green_struct;
			eof_colors[1] = eof_color_red_struct;
			eof_colors[2] = eof_color_yellow_struct;
			eof_colors[3] = eof_color_blue_struct;
			eof_colors[4] = eof_color_orange_struct;
			eof_colors[5] = eof_color_purple_struct;
		}
	}
	//Update the strings for the Note>Toggle and Note>Clear menus
	for(x = 0; x < 6; x++)
	{
		 snprintf(eof_note_toggle_menu_strings[x], sizeof(eof_note_toggle_menu_string_1) - 1, "%s\tShift+%d", eof_colors[x].colorname, x+1);
		 strncpy(eof_note_clear_menu_strings[x], eof_colors[x].colorname, sizeof(eof_note_clear_menu_string_1) - 1);
	}

	//Update the strings for the Edit>Clap Notes menu
	for(x = 0; x < 6; x++)
	{
		eof_edit_claps_menu[x + 1].text = eof_colors[x].colorname;
	}
}

void eof_set_3d_projection(void)
{
	float scale_x, scale_y;

	if(eof_full_height_3d_preview)
	{	//If the 3D preview is twice as tall as usual, use different logic to define the y scale value to ensure the seek position is on-screen
		if(eof_screen_height == 480)
		{	//640 x 480
			scale_y = (float)eof_screen_height / 480.0;
		}
		else if(eof_screen_height == 600)
		{	//800 x 600
			scale_y = (float)eof_screen_height / 600.0;
		}
		else
		{	//1024 x 768
			scale_y = (float)eof_screen_height / 768.0;
		}
	}
	else
	{
		scale_y = (float)eof_screen_height / 480.0;
	}
	scale_x = (float)eof_screen_width_default / 640.0;
	ocd3d_set_projection(scale_x, scale_y, (float)eof_vanish_x, (float)eof_vanish_y, 320.0, 320.0);
}

void eof_seek_and_render_position(unsigned long track, unsigned char diff, unsigned long pos)
{
	(void) eof_menu_track_selected_track_number(track, 1);	//Change the active track
	eof_note_type = diff;					//Change the active difficulty
	eof_set_seek_position(pos + eof_av_delay);		//Change the seek position
	eof_find_lyric_preview_lines();
	eof_render();						//Render the screen
}

void eof_hidden_mouse_callback(int flags)
{
	if(flags & MOUSE_FLAG_MOVE)
	{	//If the mouse moved
		mouse_x = eof_mouse_x;	//Restore the original mouse coordinates
		mouse_y = eof_mouse_y;
		eof_show_mouse(screen);	//Display the mouse
		mouse_callback = NULL;	//Uninstall the callback
	}
}

DIALOG eof_import_to_track_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h) (fg) (bg) (key) (flags)     (d1) (d2)  (dp)                   (dp2) (dp3) */
	{ d_agup_window_proc, 0,   48, 480, 190,   2,   23,  0,    0,        0,   0,  "Import track",           NULL, NULL },
	{ d_agup_text_proc,   16,  75,  48,  15,   2,   23,  0,    0,        0,   0,  eof_etext,                NULL, NULL },
	{ d_agup_radio_proc,  16, 105, 136,  15,   2,   23,  0,    0,        1,   0,  "PART REAL_BASS",         NULL, NULL },
	{ d_agup_radio_proc,  16, 120, 152,  15,   2,   23,  0,   D_SELECTED,1,   0,  "PART REAL_GUITAR",       NULL, NULL },
	{ d_agup_radio_proc,  16, 135, 156,  15,   2,   23,  0,    0,        1,   0,  "PART REAL_BASS_22",      NULL, NULL },
	{ d_agup_radio_proc,  16, 150, 172 , 15,   2,   23,  0,    0,        1,   0,  "PART REAL_GUITAR_22",    NULL, NULL },
	{ d_agup_radio_proc,  16, 165, 202 , 15,   2,   23,  0,    0,        1,   0,  "PART REAL_GUITAR_BONUS", NULL, NULL },
	{ d_agup_button_proc, 100,195,  68,  28,   2,   23, '\r', D_EXIT,    0,   0,  "OK",                     NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

EOF_SONG *eof_create_new_project_select_pro_guitar(void)
{
	unsigned long user_selection = EOF_TRACK_PRO_GUITAR;	//By default, a track imports to the 17 fret pro guitar track

	eof_log("eof_create_new_project_select_pro_guitar() entered", 1);

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return NULL;
	}

	eof_color_dialog(eof_import_to_track_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_import_to_track_dialog);
	(void) eof_popup_dialog(eof_import_to_track_dialog, 0);

	if(eof_import_to_track_dialog[2].flags == D_SELECTED)
	{
		user_selection = EOF_TRACK_PRO_BASS;
	}
	else if(eof_import_to_track_dialog[4].flags == D_SELECTED)
	{
		user_selection = EOF_TRACK_PRO_BASS_22;
	}
	else if(eof_import_to_track_dialog[5].flags == D_SELECTED)
	{
		user_selection = EOF_TRACK_PRO_GUITAR_22;
	}
	else if(eof_import_to_track_dialog[6].flags == D_SELECTED)
	{
		user_selection = EOF_TRACK_PRO_GUITAR_B;
	}

	if(eof_song)
	{
		eof_destroy_song(eof_song);
		eof_song = NULL;
		eof_song_loaded = 0;
		eof_changes = 0;
		eof_undo_last_type = 0;
		eof_change_count = 0;
	}
	(void) eof_destroy_ogg();

	eof_song = eof_create_song_populated();
	if(eof_song)
	{
		eof_song_loaded = 1;
		(void) eof_menu_track_selected_track_number(user_selection, 1);	//Change to the user selected track
	}

	if(!eof_song_append_beats(eof_song, 1))
	{	//If there was an error adding a beat
		eof_destroy_song(eof_song);
		return NULL;
	}
	if(!eof_song_append_beats(eof_song, 1))
	{	//If there was an error adding a second beat
		eof_destroy_song(eof_song);
		return NULL;
	}

	return eof_song;
}

int eof_identify_xml(char *fn)
{
	char *buffer;
	PACKFILE *inf = NULL;
	size_t maxlinelength;
	int done = 0;
	int retval = 0;

	if(!fn)
		return 0;	//Return error

	inf = pack_fopen(fn, "rt");	//Open file in text mode
	if(!inf)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input xml file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;	//Return error
	}

	//Allocate memory buffers large enough to hold any line in this file
	maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(fn, 0);
	if(!maxlinelength)
	{
		eof_log("\tError finding the largest line in the file.  Aborting", 1);
		(void) pack_fclose(inf);
		return 0;	//Return error
	}
	buffer = (char *)malloc(maxlinelength);
	if(!buffer)
	{
		(void) pack_fclose(inf);
		return 0;	//Return error
	}

	//Read first line of text, capping it to prevent buffer overflow
	if(!pack_fgets(buffer, (int)maxlinelength, inf))
	{	//I/O error
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "XML inspection failed.  Unable to read from file:  \"%s\"", strerror(errno));
		eof_log(eof_log_string, 1);
		done = 1;
	}

	//Parse the contents of the file
	while(!done && !pack_feof(inf))
	{	//Until there was an error reading from the file or end of file is reached
		if(strcasestr_spec(buffer, "<ebeats"))
		{	//If this is the opening ebeats tag (Rocksmith XML file)
			retval = 1;
			done = 1;
		}
		else if(strcasestr_spec(buffer, "<scoreUrl>"))
		{	//If this is a scoreUrl tag (Go PlayAlong XML file)
			retval = 2;
			done = 1;
		}

		(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
	}//Until there was an error reading from the file or end of file is reached

	free(buffer);
	(void) pack_fclose(inf);

	return retval;
}

static int exe_is_bundle(const char * fn)
{
	if(ustrstr(fn, ".app"))
	{
		return 1;
	}
	return 0;
}

int eof_validate_temp_folder(void)
{
	char correct_wd[1024], cwd[1024];

	//Determine the CWD and what it is supposed to be
	if(!getcwd(cwd, 1024))
	{	//Couldn't obtain current working directory
		return 1;
	}
	put_backslash(cwd);	//Append a file separator if necessary
	get_executable_name(correct_wd, 1024);
	#ifdef ALLEGRO_MACOSX
		if(exe_is_bundle(correct_wd))
		{
			(void) strncat(correct_wd, "/Contents/Resources/eof/", sizeof(correct_wd) - strlen(correct_wd) - 1);
		}
		else
		{
			(void) replace_filename(correct_wd, correct_wd, "", 1024);
		}
	#else
		(void) replace_filename(correct_wd, correct_wd, "", 1024);
	#endif

	//Change CWD if necessary
	if(ustricmp(correct_wd, cwd))
	{	//There's a discrepancy
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tDetected working directory:  %s", cwd);
		eof_log(eof_log_string, 1);
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tExpected working directory:  %s", correct_wd);
		eof_log(eof_log_string, 1);
		eof_log("Correcting current working directory", 1);
		if(eof_chdir(correct_wd))
		{
			eof_log("Could not change working directory", 1);
			return 2;
		}
	}

	//Ensure the temporary folder exists
	if(eof_folder_exists(eof_temp_path))
		return 0;	//If this folder already exists

	if(!getcwd(cwd, 1024))
	{	//Couldn't obtain current working directory
		return 1;
	}
	put_backslash(cwd);	//Append a file separator if necessary

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCould not detect temp folder at:  %s", cwd);
	eof_log(eof_log_string, 1);

	if(eof_mkdir(eof_temp_path))
	{	//If the folder could not be created
		eof_log("\t\tCould not create temp folder", 1);
		allegro_message("Could not create temp folder (%s)", eof_temp_path_s);
		return 3;
	}

	eof_log("\t\tTemp folder created", 1);
	return 0;
}

void eof_add_extended_ascii_glyphs(void)
{
	int ctr;
	FONT *glyph, *combined;

	if(!eof_font || !font)
		return;	//Invalid parameters

	eof_allocate_ucode_table();	//Ensure the extended ASCII->Unicode translation table is built
	if(!eof_ucode_table)
		return;	//Error

	for(ctr = 128; ctr < 160; ctr++)
	{	//For each of the extended ASCII characters that don't map to Unicode contiguously with the others
		if(eof_ucode_table[ctr] == 0x20)	//If this is an extended ASCII character with no Unicode equivalent
			continue;						//Skip it

		glyph = extract_font_range(eof_font, ctr, ctr);	//Extract this glyph into its own font
		if(glyph)
		{	//If the font was created successfully
			if(!transpose_font(glyph, eof_ucode_table[ctr] - ctr))
			{	//If the glyph was able to be remapped to the appropriate Unicode value
				combined = merge_fonts(eof_font, glyph);	//Merge the transposed glyph with the main EOF font
				if(combined)
				{	//If that succeeded
					destroy_font(eof_font);
					eof_font = combined;	//The new combined font will replace the existing EOF font
				}
			}
			destroy_font(glyph);	//Free the temporary font
		}
	}
	eof_free_ucode_table();
}

int eof_load_and_scale_hopo_images(double value)
{
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 0;
	}
	if(!exists("eof.dat"))
	{
		allegro_message("DAT file missing.  If using a hotfix, make sure you extract the appropriate EOF release candidate first and then extract the hotfix on top of it (replacing with files in the hotfix).");
		return 0;
	}

	//Destroy and reload each relevant image
	if(eof_image[EOF_IMAGE_NOTE_HGREEN])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HGREEN]);
	eof_image[EOF_IMAGE_NOTE_HGREEN] = eof_load_pcx("note_green_hopo.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HRED])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HRED]);
	eof_image[EOF_IMAGE_NOTE_HRED] = eof_load_pcx("note_red_hopo.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HYELLOW])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HYELLOW]);
	eof_image[EOF_IMAGE_NOTE_HYELLOW] = eof_load_pcx("note_yellow_hopo.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HBLUE])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HBLUE]);
	eof_image[EOF_IMAGE_NOTE_HBLUE] = eof_load_pcx("note_blue_hopo.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HPURPLE])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HPURPLE]);
	eof_image[EOF_IMAGE_NOTE_HPURPLE] = eof_load_pcx("note_purple_hopo.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HWHITE])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HWHITE]);
	eof_image[EOF_IMAGE_NOTE_HWHITE] = eof_load_pcx("note_white_hopo.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HGREEN_HIT])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HGREEN_HIT]);
	eof_image[EOF_IMAGE_NOTE_HGREEN_HIT] = eof_load_pcx("note_green_hopo_hit.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HRED_HIT])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HRED_HIT]);
	eof_image[EOF_IMAGE_NOTE_HRED_HIT] = eof_load_pcx("note_red_hopo_hit.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HYELLOW_HIT])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HYELLOW_HIT]);
	eof_image[EOF_IMAGE_NOTE_HYELLOW_HIT] = eof_load_pcx("note_yellow_hopo_hit.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HBLUE_HIT])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HBLUE_HIT]);
	eof_image[EOF_IMAGE_NOTE_HBLUE_HIT] = eof_load_pcx("note_blue_hopo_hit.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HPURPLE_HIT])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HPURPLE_HIT]);
	eof_image[EOF_IMAGE_NOTE_HPURPLE_HIT] = eof_load_pcx("note_purple_hopo_hit.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HWHITE_HIT])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HWHITE_HIT]);
	eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] = eof_load_pcx("note_white_hopo_hit.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HORANGE])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HORANGE]);
	eof_image[EOF_IMAGE_NOTE_HORANGE] = eof_load_pcx("note_orange_hopo.pcx");

	if(eof_image[EOF_IMAGE_NOTE_HORANGE_HIT])
		destroy_bitmap(eof_image[EOF_IMAGE_NOTE_HORANGE_HIT]);
	eof_image[EOF_IMAGE_NOTE_HORANGE_HIT] = eof_load_pcx("note_orange_hopo_hit.pcx");

	//Scale each image
	eof_image[EOF_IMAGE_NOTE_HGREEN] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HGREEN], value);
	eof_image[EOF_IMAGE_NOTE_HRED] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HRED], value);
	eof_image[EOF_IMAGE_NOTE_HYELLOW] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HYELLOW], value);
	eof_image[EOF_IMAGE_NOTE_HBLUE] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HBLUE], value);
	eof_image[EOF_IMAGE_NOTE_HPURPLE] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HPURPLE], value);
	eof_image[EOF_IMAGE_NOTE_HWHITE] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HWHITE], value);
	eof_image[EOF_IMAGE_NOTE_HGREEN_HIT] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HGREEN_HIT], value);
	eof_image[EOF_IMAGE_NOTE_HRED_HIT] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HRED_HIT], value);
	eof_image[EOF_IMAGE_NOTE_HYELLOW_HIT] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HYELLOW_HIT], value);
	eof_image[EOF_IMAGE_NOTE_HBLUE_HIT] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HBLUE_HIT], value);
	eof_image[EOF_IMAGE_NOTE_HPURPLE_HIT] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HPURPLE_HIT], value);
	eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HWHITE_HIT], value);
	eof_image[EOF_IMAGE_NOTE_HORANGE] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HORANGE], value);
	eof_image[EOF_IMAGE_NOTE_HORANGE_HIT] = eof_scale_image(eof_image[EOF_IMAGE_NOTE_HORANGE_HIT], value);

	return 1;
}

unsigned int eof_get_display_panel_count(void)
{
	unsigned int count = 0;

	if(!eof_disable_info_panel)
		count++;	//If the info panel isn't disabled, count it
	if(!eof_disable_3d_rendering)
		count++;	//If the 3D preview isn't disabled, count it
	if(eof_enable_notes_panel)
		count++;	//If the notes panel is enabled, count it

	return count;
}

int eof_increase_display_width_to_panel_count(int prompt)
{
	unsigned long needed_width = eof_get_display_panel_count() * EOF_SCREEN_PANEL_WIDTH;
	int retval;

	if(eof_screen_width < needed_width)
	{	//If the current screen width isn't sufficient to display all enabled panels
		eof_clear_input();
		if(prompt)
		{	//If the calling function specified to prompt the user
			retval = alert(NULL, "Resize the program window to fit all panels?", NULL, "&Yes", "&No", 'y', 'n');

			if(retval != 1)
			{	//If the user declined to resize
				eof_log("User declined window resize.  Disabling Notes panel.", 2);
				eof_enable_notes_panel = 0;	//Disable the notes panel
				return 1;
			}
		}

		eof_log("Resizing window to fit Notes panel.", 2);
		if(eof_set_display_mode(needed_width, eof_screen_height) != 1)
		{	//If the program window couldn't be resized to the appropriate width, disable the non-default panels (info, 3d preview)
			eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
			eof_log("Couldn't resize window.  Disabling Notes panel.", 2);
			eof_enable_notes_panel = 0;		//Disable the notes panel
			return 0;
		}
	}

	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	return 1;
}

END_OF_MAIN()
