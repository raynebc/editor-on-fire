#ifndef EOF_MAIN_H
#define EOF_MAIN_H

#include <allegro.h>
#include "alogg/include/alogg.h"
#include "modules/wfsel.h"
#include "window.h"
#include "note.h"
#include "notes.h"
#include "control.h"
#include "editor.h"
#include "pathing.h"
#include "music_pos.h"

#define EOF_VERSION_STRING "EOF v1.8RC14"
#define EOF_COPYRIGHT_STRING "(c)2008-2026 T^3 Software."

#define KEY_EITHER_ALT (key[KEY_ALT] || key[KEY_ALTGR])
#define KEY_EITHER_WIN (key[KEY_LWIN] || key[KEY_RWIN])
#ifdef ALLEGRO_MACOSX
		#define KEY_EITHER_CTRL (key[KEY_COMMAND])
#else
		#define KEY_EITHER_CTRL (key[KEY_LCONTROL] || key[KEY_RCONTROL])
#endif
#define KEY_EITHER_SHIFT (key[KEY_LSHIFT] || key[KEY_RSHIFT])

#ifdef ALLEGRO_MACOSX
	#define CTRL_NAME "Cmd"
#else
	#define CTRL_NAME "Ctrl"
#endif

#ifdef ALLEGRO_LEGACY
	#define EOF_MENU_HEIGHT 0
#else
	#define EOF_MENU_HEIGHT 20
#endif

#define EOF_SNAP_OFF           0
#define EOF_SNAP_QUARTER       1
#define EOF_SNAP_EIGHTH        2
#define EOF_SNAP_TWELFTH       3
#define EOF_SNAP_SIXTEENTH     4
#define EOF_SNAP_TWENTY_FOURTH 5
#define EOF_SNAP_THIRTY_SECOND 6
#define EOF_SNAP_FORTY_EIGHTH  7
#define EOF_SNAP_SIXTY_FORTH   8
#define EOF_SNAP_NINTY_SIXTH   9
#define EOF_SNAP_CUSTOM        10

#define EOF_HOPO_RF            0
#define EOF_HOPO_FOF           1
#define EOF_HOPO_OFF           2
#define EOF_HOPO_MANUAL        3
#define EOF_HOPO_GH3           4
#define EOF_NUM_HOPO_MODES     5

#define EOF_INPUT_CLASSIC      0
#define EOF_INPUT_PIANO_ROLL   1
#define EOF_INPUT_HOLD         2
#define EOF_INPUT_REX          3
#define EOF_INPUT_GUITAR_TAP   4
#define EOF_INPUT_GUITAR_STRUM 5
#define EOF_INPUT_FEEDBACK     6

#define EOF_NUM_COLOR_SETS     6
#define EOF_COLORS_DEFAULT     0
#define EOF_COLORS_RB          1
#define EOF_COLORS_GH          2
#define EOF_COLORS_RS          3
#define EOF_COLORS_BF          4
#define EOF_COLORS_IR          5

#define EOF_MAX_IMAGES 116

#define EOF_IMAGE_WAVE                        0	//This macro is currently unused
#define EOF_IMAGE_TAB0                        1
#define EOF_IMAGE_TAB1                        2
#define EOF_IMAGE_TAB2                        3
#define EOF_IMAGE_TAB3                        4
#define EOF_IMAGE_TAB4                        5
#define EOF_IMAGE_TABE                        6
#define EOF_IMAGE_NOTE_GREEN                  7
#define EOF_IMAGE_NOTE_RED                    8
#define EOF_IMAGE_NOTE_YELLOW                 9
#define EOF_IMAGE_NOTE_BLUE                  10
#define EOF_IMAGE_NOTE_PURPLE                11
#define EOF_IMAGE_NOTE_GREEN_HIT             12
#define EOF_IMAGE_NOTE_RED_HIT               13
#define EOF_IMAGE_NOTE_YELLOW_HIT            14
#define EOF_IMAGE_NOTE_BLUE_HIT              15
#define EOF_IMAGE_NOTE_PURPLE_HIT            16
#define EOF_IMAGE_CONTROLS_BASE              17
#define EOF_IMAGE_CONTROLS_0                 18
#define EOF_IMAGE_CONTROLS_1                 19
#define EOF_IMAGE_CONTROLS_2                 20
#define EOF_IMAGE_CONTROLS_3                 21
#define EOF_IMAGE_CONTROLS_4                 22
#define EOF_IMAGE_MENU                       23
#define EOF_IMAGE_MENU_FULL                  24
#define EOF_IMAGE_NOTE_HGREEN                25
#define EOF_IMAGE_NOTE_HRED                  26
#define EOF_IMAGE_NOTE_HYELLOW               27
#define EOF_IMAGE_NOTE_HBLUE                 28
#define EOF_IMAGE_NOTE_HPURPLE               29
#define EOF_IMAGE_NOTE_HGREEN_HIT            30
#define EOF_IMAGE_NOTE_HRED_HIT              31
#define EOF_IMAGE_NOTE_HYELLOW_HIT           32
#define EOF_IMAGE_NOTE_HBLUE_HIT             33
#define EOF_IMAGE_NOTE_HPURPLE_HIT           34
#define EOF_IMAGE_SCROLL_BAR                 35
#define EOF_IMAGE_SCROLL_HANDLE              36
#define EOF_IMAGE_CCONTROLS_BASE             37
#define EOF_IMAGE_CCONTROLS_0                38
#define EOF_IMAGE_CCONTROLS_1                39
#define EOF_IMAGE_CCONTROLS_2                40
#define EOF_IMAGE_NOTE_WHITE                 41
#define EOF_IMAGE_NOTE_WHITE_HIT             42
#define EOF_IMAGE_NOTE_HWHITE                43
#define EOF_IMAGE_NOTE_HWHITE_HIT            44
#define EOF_IMAGE_MENU_NO_NOTE               45
#define EOF_IMAGE_VTAB0                      46
#define EOF_IMAGE_LYRIC_SCRATCH              47
#define EOF_IMAGE_NOTE_YELLOW_CYMBAL         48
#define EOF_IMAGE_NOTE_YELLOW_CYMBAL_HIT     49
#define EOF_IMAGE_NOTE_BLUE_CYMBAL           50
#define EOF_IMAGE_NOTE_BLUE_CYMBAL_HIT       51
#define EOF_IMAGE_NOTE_PURPLE_CYMBAL         52
#define EOF_IMAGE_NOTE_PURPLE_CYMBAL_HIT     53
#define EOF_IMAGE_NOTE_WHITE_CYMBAL          54
#define EOF_IMAGE_NOTE_WHITE_CYMBAL_HIT      55
#define EOF_IMAGE_NOTE_ORANGE                56
#define EOF_IMAGE_NOTE_ORANGE_HIT            57
#define EOF_IMAGE_NOTE_HORANGE               58
#define EOF_IMAGE_NOTE_HORANGE_HIT           59
#define EOF_IMAGE_NOTE_GREEN_ARROW           60
#define EOF_IMAGE_NOTE_RED_ARROW             61
#define EOF_IMAGE_NOTE_YELLOW_ARROW          62
#define EOF_IMAGE_NOTE_BLUE_ARROW            63
#define EOF_IMAGE_NOTE_GREEN_ARROW_HIT       64
#define EOF_IMAGE_NOTE_RED_ARROW_HIT         65
#define EOF_IMAGE_NOTE_YELLOW_ARROW_HIT      66
#define EOF_IMAGE_NOTE_BLUE_ARROW_HIT        67
#define EOF_IMAGE_NOTE_GREEN_CYMBAL          68
#define EOF_IMAGE_NOTE_GREEN_CYMBAL_HIT      69
#define EOF_IMAGE_NOTE_ORANGE_CYMBAL         70
#define EOF_IMAGE_NOTE_ORANGE_CYMBAL_HIT     71
#define EOF_IMAGE_TAB_FG                     72
#define EOF_IMAGE_TAB_BG                     73
#define EOF_IMAGE_NOTE_BLACK                 74
#define EOF_IMAGE_NOTE_BLACK_HIT             75
#define EOF_IMAGE_NOTE_GHL_BLACK             76
#define EOF_IMAGE_NOTE_GHL_BLACK_HIT         77
#define EOF_IMAGE_NOTE_GHL_BLACK_HOPO        78
#define EOF_IMAGE_NOTE_GHL_BLACK_HOPO_HIT    79
#define EOF_IMAGE_NOTE_GHL_BLACK_SP          80
#define EOF_IMAGE_NOTE_GHL_BLACK_SP_HIT      81
#define EOF_IMAGE_NOTE_GHL_BLACK_SP_HOPO     82
#define EOF_IMAGE_NOTE_GHL_BLACK_SP_HOPO_HIT 83
#define EOF_IMAGE_NOTE_GHL_WHITE             84
#define EOF_IMAGE_NOTE_GHL_WHITE_HIT         85
#define EOF_IMAGE_NOTE_GHL_WHITE_HOPO        86
#define EOF_IMAGE_NOTE_GHL_WHITE_HOPO_HIT    87
#define EOF_IMAGE_NOTE_GHL_WHITE_SP          88
#define EOF_IMAGE_NOTE_GHL_WHITE_SP_HIT      89
#define EOF_IMAGE_NOTE_GHL_WHITE_SP_HOPO     90
#define EOF_IMAGE_NOTE_GHL_WHITE_SP_HOPO_HIT 91
#define EOF_IMAGE_NOTE_GHL_BARRE             92
#define EOF_IMAGE_NOTE_GHL_BARRE_HIT         93
#define EOF_IMAGE_NOTE_GHL_BARRE_HOPO        94
#define EOF_IMAGE_NOTE_GHL_BARRE_HOPO_HIT    95
#define EOF_IMAGE_NOTE_GHL_BARRE_SP          96
#define EOF_IMAGE_NOTE_GHL_BARRE_SP_HIT      97
#define EOF_IMAGE_NOTE_GHL_BARRE_SP_HOPO     98
#define EOF_IMAGE_NOTE_GHL_BARRE_SP_HOPO_HIT 99
#define EOF_IMAGE_NOTE_GREEN_SLIDER         100
#define EOF_IMAGE_NOTE_RED_SLIDER           101
#define EOF_IMAGE_NOTE_YELLOW_SLIDER        102
#define EOF_IMAGE_NOTE_BLUE_SLIDER          103
#define EOF_IMAGE_NOTE_PURPLE_SLIDER        104
#define EOF_IMAGE_NOTE_ORANGE_SLIDER        105
#define EOF_IMAGE_NOTE_WHITE_SLIDER         106
#define EOF_IMAGE_NOTE_BLACK_SLIDER         107
#define EOF_IMAGE_NOTE_GHL_WHITE_SLIDER     108
#define EOF_IMAGE_NOTE_GHL_BLACK_SLIDER     109
#define EOF_IMAGE_NOTE_GHL_BARRE_SLIDER     110
#define EOF_IMAGE_NOTE_GHL_WHITE_SP_SLIDER  111
#define EOF_IMAGE_NOTE_GHL_BLACK_SP_SLIDER  112
#define EOF_IMAGE_NOTE_GHL_BARRE_SP_SLIDER  113
#define EOF_IMAGE_NOTE_MINE 114
#define EOF_IMAGE_NOTE_MINE_HIT 115

#define EOF_DISPLAY_640             0
#define EOF_DISPLAY_800             1
#define EOF_DISPLAY_1024            2
#define EOF_DISPLAY_CUSTOM          3
#define EOF_SCREEN_PANEL_WIDTH (eof_screen_width_default / 2)

#define EOF_INPUT_NAME_NUM			7
	//The number of defined input methods

typedef struct
{

	int mode;					//If this is EOF_DISPLAY_CUSTOM, eof_screen_height is the screen height instead of a preset

	/* rendering offsets */
	int scrollbar_y;
	int string_space, string_space_unscaled;	//Defines the amount of vertical space between each lane in the piano roll when 5 lanes are in effect
										//string_space_unscaled is set by the display size, string_space is set by eof_scale_fretboard() to account for 6 lane modes
	int note_y[EOF_MAX_FRETS];	//Y coordinates for lanes that are calculated by eof_scale_fretboard()
	int lyric_y;				//A constant y coordinate offset (EOF_EDITOR_RENDER_OFFSET + 15 + lyric_y is the y position at which lyric text is drawn in the editor window)
	int vocal_y;				//A constant y coordinate offset (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y is the bottom of the bottom-most key in the mini piano)
	int vocal_tail_size;			//The height of a lyric note rectangle
	int vocal_view_size;			//A constant representing the number of piano keys to display in the left edge of the piano roll when the vocal track is active
	int lyric_view_key_width;		//How far apart each black line is rendered to separate the white piano keys in the vocal track 3D preview (1/29 the 3D preview window)
	int lyric_view_key_height;	//The height of the white keys are in the vocal track 3D preview (4 * lyric_view_key_width)
	int lyric_view_bkey_width;	//The width of the black keys in the vocal track 3D preview
	int lyric_view_bkey_height;	//The height of the black keys in the vocal track 3D preview (3 * lyric_view_bkey_width)
	int note_size;				//The radius of circular gems rendered in the editor window
	int hopo_note_size;			//The radius of forced HOPO circular gems rendered in the editor window
	int anti_hopo_note_size;		//The radius of forced HOPO off circular gems rendered in the editor window
	int note_dot_size;			//The radius of the circle rendered inside of circular gems in the editor window
	int hopo_note_dot_size;		//The radius of the circle rendered inside of HOPO circular gems in the editor window
	int anti_hopo_note_dot_size;	//The radius of the circle rendered inside of HOPO off circular gems in the editor window
	int note_marker_size;		//Half the height of the line segment rendered to mark the start position of each note/lyric in the editor window
	int note_tail_size;			//Half the height of a note/lyric tail rendered in the editor window
	int fretboard_h;			//A y coordinate offset (EOF_EDITOR_RENDER_OFFSET + fretboard_h is the bottom of the fretboard portion of the piano roll)
	int controls_x;				//The x coordinate to which the playback controls are rendered in the editor window (197 pixels from the right edge of the program window)

	/* editor mouse click areas */


} EOF_SCREEN_LAYOUT;

#define EOF_EDITOR_RENDER_OFFSET  56

typedef struct
{
	int color;					//RGB color for the normal note
	int hit;					//RGB color for when the note is highlighted (3D tail rendering)
	int border;					//Contrasting RGB color for when the note is highlighted (2D rendering)
	int lightcolor;				//RGB color for a lighter version of the note (for rendering trill/tremolo markers)
	unsigned int note3d;		//The index into eof_image[] storing the 3D note image for this color
	unsigned int notehit3d;		//The index into eof_image[] storing the hit 3D note image for this color
	unsigned int hoponote3d;	//The index into eof_image[] storing the hopo 3D note image for this color
	unsigned int hoponotehit3d;	//The index into eof_image[] storing the hopo hit 3D note image for this color
	unsigned int cymbal3d;		//The index into eof_image[] storing the 3D cymbal image for this color
	unsigned int cymbalhit3d;	//The index into eof_image[] storing the hit 3D cymbal image for this color
	unsigned int arrow3d;		//The index into eof_image[] storing the 3D arrow image for this color (dance track)
	unsigned int arrowhit3d;	//The index into eof_image[] storing the hit 3D arrow image for this color (dance track)
	unsigned int slider3d;		//The index into eof_image[] storing the 3D slider image for this color
	char *colorname;			//The display name of this color, ie "red"
}eof_color;

extern int eof_color_set;
extern char eof_note_toggle_menu_string_1[20];
extern char eof_note_toggle_menu_string_2[20];
extern char eof_note_toggle_menu_string_3[20];
extern char eof_note_toggle_menu_string_4[20];
extern char eof_note_toggle_menu_string_5[20];
extern char eof_note_toggle_menu_string_6[20];
extern eof_color eof_colors[6];
extern char eof_note_clear_menu_string_1[20];
extern char eof_note_clear_menu_string_2[20];
extern char eof_note_clear_menu_string_3[20];
extern char eof_note_clear_menu_string_4[20];
extern char eof_note_clear_menu_string_5[20];
extern char eof_note_clear_menu_string_6[20];
extern char *eof_note_clear_menu_strings[6];

extern NCDFS_FILTER_LIST * eof_filter_music_files;
extern NCDFS_FILTER_LIST * eof_filter_ogg_files;
extern NCDFS_FILTER_LIST * eof_filter_midi_files;
extern NCDFS_FILTER_LIST * eof_filter_dr_files;
extern NCDFS_FILTER_LIST * eof_filter_sm_files;
extern NCDFS_FILTER_LIST * eof_filter_eof_files;
extern NCDFS_FILTER_LIST * eof_filter_exe_files;
extern NCDFS_FILTER_LIST * eof_filter_lyrics_files;
extern NCDFS_FILTER_LIST * eof_filter_dB_files;
extern NCDFS_FILTER_LIST * eof_filter_gh_files;
extern NCDFS_FILTER_LIST * eof_filter_ghl_files;
extern NCDFS_FILTER_LIST * eof_filter_gp_files;
extern NCDFS_FILTER_LIST * eof_filter_gp_lyric_text_files;
extern NCDFS_FILTER_LIST * eof_filter_rs_files;
extern NCDFS_FILTER_LIST * eof_filter_sonic_visualiser_files;
extern NCDFS_FILTER_LIST * eof_filter_bf_files;
extern NCDFS_FILTER_LIST * eof_filter_note_panel_files;
extern NCDFS_FILTER_LIST * eof_filter_array_txt_files;
extern NCDFS_FILTER_LIST * eof_filter_beatable_files;
extern NCDFS_FILTER_LIST * eof_filter_ffmpeg_files;

extern int         eof_global_volume;

extern EOF_WINDOW * eof_window_editor;
extern EOF_WINDOW * eof_window_editor2;
extern EOF_WINDOW * eof_window_info;	//The info panel, will be set to either the lower left or upper left variants below depending on whether full screen 3D view is in effect
extern EOF_WINDOW * eof_window_note_lower_left;
extern EOF_WINDOW * eof_window_note_upper_left;
extern EOF_WINDOW * eof_window_3d;
extern EOF_WINDOW * eof_window_notes;

extern char        eof_last_frettist[256];
extern int         eof_zoom;
extern int         eof_zoom_3d;
extern int         eof_3d_min_depth;
extern int         eof_3d_max_depth;
extern char        eof_screen_zoom;
extern char        eof_changes;
extern ALOGG_OGG * eof_music_track;
extern void      * eof_music_data;
extern int         eof_silence_loaded;	//Tracks whether "second_of_silence.ogg" was loaded from EOF's program folder instead of user-specified chart audio
extern int         eof_music_data_size;
extern unsigned long eof_chart_length;
extern unsigned long eof_music_length;
extern int         eof_logic_rate;
extern EOF_MUSIC_POS eof_music_pos;
extern int         eof_music_pos2;
extern int         eof_sync_piano_rolls;
extern unsigned long eof_music_actual_pos;
extern unsigned long eof_music_rewind_pos;
#define EOF_SONG_CATALOG_PLAYBACK_PADDING 100
extern int         eof_music_catalog_pos;
extern unsigned long eof_music_end_pos;
extern int         eof_music_paused;
extern char        eof_music_catalog_playback;
extern char        eof_play_selection;
extern EOF_SONG    * eof_song;
extern EOF_NOTE    eof_pen_note;
extern EOF_LYRIC   eof_pen_lyric;
extern char        eof_temp_path[20];
extern char        eof_temp_path_s[20];
extern char        eof_filename[1024];
extern char        eof_song_path[1024];
extern char        eof_songs_path[1024];
extern char        eof_window_title[4096];
extern char        eof_last_ogg_path[1024];
extern char        eof_last_eof_path[1024];
extern char        eof_last_midi_path[1024];
extern char        eof_last_dr_path[1024];
extern char        eof_last_sm_path[1024];
extern char        eof_last_db_path[1024];
extern char        eof_last_gh_path[1024];
extern char        eof_last_ghl_path[1024];
extern char        eof_last_lyric_path[1024];
extern char        eof_last_gp_path[1024];
extern char        eof_last_rs_path[1024];
extern char        eof_last_sonic_visualiser_path[1024];
extern char        eof_last_bf_path[1024];
extern char        eof_last_browsed_notes_panel_path[1024];
extern char        eof_current_notes_panel_path[1024];
extern char        eof_loaded_song_name[1024];
extern char        eof_loaded_ogg_name[1024];
extern int         eof_quit;
extern unsigned long eof_main_loop_ctr;
extern double      eof_main_loop_fps;
extern EOF_SCREEN_LAYOUT eof_screen_layout;
extern BITMAP *    eof_screen;
extern unsigned long eof_screen_width, eof_screen_height;
extern unsigned long eof_screen_width_default;
extern int         eof_vanish_x, eof_vanish_y;
extern char        eof_full_screen_3d;
extern char        eof_3d_fretboard_coordinates_cached;
extern unsigned long eof_av_delay;
extern int         eof_midi_tone_delay;
extern int         eof_midi_synth_instrument_guitar;
extern int         eof_midi_synth_instrument_guitar_muted;
extern int         eof_midi_synth_instrument_guitar_harm;
extern int         eof_midi_synth_instrument_bass;
extern int         eof_midi_pan;
extern int         eof_chart_pan;
extern int         eof_scroll_seek_percent;
extern int         eof_buffer_size;
extern int         eof_audio_fine_tune;
extern int         eof_inverted_notes;
extern int         eof_lefty_mode;
extern int         eof_note_auto_adjust;
extern int         eof_use_ts;	//Determines whether Time Signature events will be handled for MIDI/Feedback import/export
extern int         eof_hide_drum_tails;
extern int         eof_hide_note_names;
extern int         eof_disable_sound_processing;
extern int         eof_disable_3d_rendering;
extern int         eof_disable_2d_rendering;
extern int         eof_disable_info_panel;
extern int         eof_enable_notes_panel;
extern int         eof_full_height_3d_preview;
extern EOF_TEXT_PANEL *eof_notes_panel;
extern EOF_TEXT_PANEL *eof_info_panel;
extern int         eof_paste_erase_overlap;
extern int         eof_write_fof_files;
extern int         eof_write_gh_files;
extern int         eof_write_rb_files;
extern int         eof_write_music_midi;
extern int         eof_write_rs_files;
extern int         eof_write_rs2_files;
extern int         eof_write_immerrock_files;
extern int         eof_abridged_rs2_export;
extern int         eof_rs2_export_extended_ascii_lyrics;
extern int         eof_disable_ini_difference_warnings;
extern int         eof_write_bf_files;
extern int         eof_write_lrc_files;
extern int         eof_force_pro_drum_midi_notation;
extern int         eof_add_new_notes_to_selection;
extern int         eof_drum_modifiers_affect_all_difficulties;
extern int         eof_fb_seek_controls;
extern int         eof_new_note_length_1ms;
extern int         eof_new_note_forced_strum;
extern int         eof_use_fof_difficulty_naming;
extern int         eof_gp_import_text;
extern int         eof_gp_import_text_techniques;
extern unsigned eof_gp_import_slide_in_beat_interval;
extern unsigned eof_gp_import_slide_in_fret_count;
extern unsigned eof_gp_import_slide_out_fret_count;
extern int         eof_gp_import_preference_1;
extern int         eof_gp_import_truncate_short_notes;
extern int         eof_gp_import_truncate_short_chords;
extern int         eof_gp_import_replaces_track;
extern int         eof_gp_import_nat_harmonics_only;
extern int         eof_min_note_length;
extern int         eof_enforce_chord_density;
extern unsigned    eof_chord_density_threshold;
extern unsigned    eof_min_note_distance;
extern unsigned    eof_min_note_distance_intervals;
extern int         eof_render_bass_drum_in_lane;
extern int         eof_inverted_chords_slash;
extern int         eof_click_changes_dialog_focus;
extern int         eof_stop_playback_leave_focus;
extern int         eof_render_3d_rs_chords;
extern int         eof_render_2d_rs_piano_roll;
extern int         eof_dont_restrict_tone_change_timing;
extern int         eof_imports_recall_last_path;
extern int         eof_rewind_at_end;
extern int         eof_disable_rs_wav;
extern int         eof_display_seek_pos_in_seconds;
extern int         eof_note_tails_clickable;
extern int         eof_lyric_tails_clickable;
extern int         eof_auto_complete_fingering;
extern int         eof_dont_auto_name_double_stops;
extern int         eof_section_auto_adjust;
extern int         eof_technote_auto_adjust;
extern int         eof_top_of_2d_pane_cycle_count_2;
extern int         eof_rbn_export_slider_hopo;
extern int         eof_imports_drop_mid_beat_tempos;
extern int         eof_render_mid_beat_tempos_blue;
extern int         eof_disable_ini_export;
extern int         eof_gh_import_sustain_threshold_prompt;
extern int         eof_rs_import_all_handshapes;
extern int         eof_rs2_export_version_8;
extern int         eof_midi_export_enhanced_open_marker;
extern int         eof_gp_import_remove_accent_from_staccato;
extern int         eof_gp_import_track_specific_events;
extern int         eof_db_import_suppress_5nc_conversion;
extern int         eof_warn_missing_bass_fhps;
extern int         eof_4_fret_range;
extern int         eof_5_fret_range;
extern int         eof_6_fret_range;
extern int         eof_fingering_checks_include_mutes;
extern int         eof_ghl_conversion_swaps_bw_gems;
extern int         eof_3d_hopo_scale_size;
extern int         eof_disable_backups;
extern int         eof_enable_open_strums_by_default;
extern int         eof_prefer_midi_friendly_grid_snapping;
extern int         eof_dont_auto_edit_new_lyrics;
extern int         eof_dont_redraw_on_exit_prompt;
extern int         eof_offer_fhp_derived_finger_placements;
extern double      eof_lyric_gap_multiplier;
extern char        eof_lyric_gap_multiplier_string[20];
extern int         eof_song_folder_prompt;
extern int         eof_smooth_pos;
extern int         eof_windowed;
extern int         eof_anchor_all_beats;
extern int         eof_disable_windows;
extern int         eof_disable_vsync;
extern int         eof_phase_cancellation;
extern int         eof_center_isolation;
extern int         eof_playback_speed;
extern char        eof_playback_time_stretch;
extern int         eof_ogg_setting;
extern char      * eof_ogg_quality[7];
extern int         eof_cpu_saver;
extern unsigned    eof_note_type;
extern unsigned    eof_note_type_i;
extern unsigned    eof_note_type_v;
extern unsigned    eof_selected_track;
extern int         eof_display_second_piano_roll;
extern unsigned    eof_note_type2;
extern unsigned    eof_selected_track2;
extern int         eof_vocals_selected;
extern int         eof_vocals_offset;
extern unsigned    eof_vocals_tab;
extern int         eof_song_loaded;
extern char        eof_fof_executable_path[1024];
extern char        eof_fof_executable_name[1024];
extern char        eof_fof_songs_path[1024];
extern char        eof_ps_executable_path[1024];
extern char        eof_ps_executable_name[1024];
extern char        eof_ps_songs_path[1024];
extern char        eof_rs_to_tab_executable_path[1024];
extern char        eof_ffmpeg_executable_path[1024];
extern char        eof_temp_filename[1024];
extern char        eof_soft_cursor;
extern char        eof_desktop;
extern int         eof_input_mode;
extern int         eof_undo_toggle;
extern int         eof_redo_toggle;
extern int         eof_window_title_dirty;
extern int         eof_change_count;
extern int         eof_last_note;
extern int         eof_last_midi_offset;
extern int         eof_notes_moved;
extern EOF_NOTE *  eof_entering_note_note;
extern EOF_LYRIC *  eof_entering_note_lyric;
extern int         eof_entering_note;
extern int         eof_snote;
extern PACKFILE *  eof_recovery;
extern unsigned long eof_seek_selection_start, eof_seek_selection_end;
extern int         eof_shift_released;
extern int         eof_shift_used;
extern int         eof_tab_released;
extern int         eof_emergency_stop;
extern int         ch_sp_path_worker;
extern int         ch_sp_path_worker_logging;
extern unsigned long ch_sp_path_worker_number;
extern char        eof_default_ini_setting[EOF_MAX_INI_SETTINGS][EOF_INI_LENGTH];
extern unsigned short eof_default_ini_settings;

extern int         eof_display_catalog;
extern int         eof_catalog_full_width;
extern unsigned long eof_selected_catalog_entry;

/* mouse control data */
extern int         eof_selected_control;
extern int         eof_cselected_control;
extern unsigned long eof_selected_beat;
extern long        eof_selected_measure;
extern int         eof_beat_in_measure;
extern int         eof_beats_in_measure;
extern int         eof_pegged_note;
extern int         eof_hover_note;		//The hover note that is tracked using the mouse position
extern int         eof_seek_hover_note;	//The hover note that is tracked using the seek position (for Feedback input method)
extern int         eof_hover_note_2;	//The hover note that is tracked for fret catalog playback
extern unsigned long eof_hover_beat;	//The beat that is hovered over, set to ULONG_MAX if no beat is hovered over
extern unsigned long eof_hover_beat_2;	//The last beat in which the current playback position was determined to be
extern int         eof_hover_piece;		//The lane number that the mouse is over
extern int         eof_hover_key;
extern int         eof_hover_lyric;
extern int         eof_last_tone;
extern int         eof_mouse_x;
extern int         eof_mouse_y;
extern int         eof_mouse_z;
extern int         eof_mickey_z;
extern int         eof_mickeys_x;
extern int         eof_mickeys_y;
extern int         eof_lclick_released;
extern int         eof_blclick_released;
#define EOF_CLICK_AND_DRAG_THRESHOLD 150
extern clock_t   eof_lclick_time;
extern int         eof_rclick_released;
extern int         eof_mclick_released;
extern int         eof_click_x;
extern int         eof_click_y;
extern int         eof_peg_x;
extern int         eof_last_pen_pos;
extern int         eof_cursor_visible;
extern int         eof_pen_visible;
extern int         eof_hover_type;
extern int         eof_mouse_drug;
extern int         eof_paste_at_mouse;

extern int eof_mouse_area;
extern int eof_mini_keyboard_boundary_x1, eof_mini_keyboard_boundary_x2, eof_mini_keyboard_boundary_y1, eof_mini_keyboard_boundary_y2;
extern int eof_fretboard_boundary_x1, eof_fretboard_boundary_x2, eof_fretboard_boundary_y1, eof_fretboard_boundary_y2;
extern int eof_beat_marker_boundary_x1, eof_beat_marker_boundary_x2, eof_beat_marker_boundary_y1, eof_beat_marker_boundary_y2;
extern int eof_difficulty_tab_boundary_x1, eof_difficulty_tab_boundary_x2, eof_difficulty_tab_boundary_y1, eof_difficulty_tab_boundary_y2;
extern int eof_mouse_boundary_x1, eof_mouse_boundary_x2, eof_mouse_boundary_y1, eof_mouse_boundary_y2;
extern char eof_mouse_bound;

extern char eof_snap_mode;
extern char eof_last_snap_mode;
extern int eof_snap_interval;
extern char eof_custom_snap_measure;
extern char eof_notes_panel_wants_grid_snap_data;

extern char          eof_hopo_view;

extern EOF_CONTROLLER eof_guitar;
extern EOF_CONTROLLER eof_drums;
extern int eof_color_black;
extern int eof_color_white;
extern int eof_color_gray;
extern int eof_color_gray2;
extern int eof_color_gray3;
extern int eof_color_light_gray;
extern int eof_color_red;
extern int eof_color_light_red;
extern int eof_color_green;
extern int eof_color_dark_green;
extern int eof_color_blue;
extern int eof_color_dark_blue;
extern int eof_color_light_blue;
extern int eof_color_lighter_blue;
extern int eof_color_even_lighter_blue;
extern int eof_color_turquoise;
extern int eof_color_yellow;
extern int eof_color_purple;
extern int eof_color_dark_purple;
extern int eof_color_orange;
extern int eof_color_silver;
extern int eof_color_dark_silver;
extern int eof_color_cyan;
extern int eof_color_dark_cyan;
extern int eof_info_color;

extern int eof_color_waveform_trough_raw;
extern int eof_color_waveform_peak_raw;
extern int eof_color_waveform_rms_raw;
extern int eof_color_waveform_trough;
extern int eof_color_waveform_peak;
extern int eof_color_waveform_rms;
extern char eof_waveform_renderlocation;
extern char eof_waveform_renderleftchannel;
extern char eof_waveform_renderrightchannel;
extern char eof_waveform_renderscale_enabled;
extern unsigned eof_waveform_renderscale;

extern int eof_notes_panel_error_bg_color, eof_notes_panel_error_fg_color;
extern int eof_notes_panel_warning_bg_color, eof_notes_panel_warning_fg_color;
extern int eof_notes_panel_success_bg_color, eof_notes_panel_success_fg_color;
extern int eof_notes_panel_alert_bg_color, eof_notes_panel_alert_fg_color;
extern int eof_notes_panel_info_bg_color, eof_notes_panel_info_fg_color;

extern int eof_color_highlight1_raw;
extern int eof_color_highlight2_raw;
extern int eof_color_highlight1;
extern int eof_color_highlight2;

extern eof_color eof_color_green_struct;
extern eof_color eof_color_red_struct;
extern eof_color eof_color_light_red_struct;
extern eof_color eof_color_yellow_struct;
extern eof_color eof_color_blue_struct;
extern eof_color eof_color_orange_struct;
extern eof_color eof_color_purple_struct;
extern eof_color eof_color_black_struct;
extern eof_color eof_color_ghl_black_struct;
extern eof_color eof_color_ghl_white_struct;

extern char eof_string_lane_1[5];
extern char eof_string_lane_2[5];
extern char eof_string_lane_3[5];
extern char eof_string_lane_4[5];
extern char eof_string_lane_5[5];
extern char eof_string_lane_6[5];
extern char *eof_fret_strings[6];
extern char eof_string_lane_1_number[];
extern char eof_string_lane_2_number[];
extern char eof_string_lane_3_number[];
extern char eof_string_lane_4_number[];
extern char eof_string_lane_5_number[];
extern char eof_string_lane_6_number[];
extern char *eof_fret_string_numbers[6];

extern PALETTE     eof_palette;
extern BITMAP *    eof_image[EOF_MAX_IMAGES];
extern BITMAP *    eof_stretch_bitmap;			//Used to cache and reuse the bitmap created to render a double-tall 3D gem when full height 3D preview is in effect
extern BITMAP *    eof_hopo_stretch_bitmap;		//Used to cache and reuse the bitmap created to render a double-tall 3D HOPO gem when full height 3D preview is in effect
extern BITMAP *    eof_background;
extern FONT *      eof_allegro_font;
extern FONT *      eof_font;
extern FONT *      eof_mono_font;
extern FONT *      eof_symbol_font;	//A font where the 0, 1, 2, 3 and 4 glyphs have been replaced with guitar tab notation characters
extern char        eof_note_type_name_fof[EOF_MAX_DIFFICULTIES][10];
extern char        eof_note_type_name_rb[EOF_MAX_DIFFICULTIES][10];
extern char        (*eof_note_type_name)[10];	//Will point to either eof_note_type_name_fof or eof_note_type_name_rb based on user preference
extern char        eof_vocal_tab_name[EOF_MAX_DIFFICULTIES][10];
extern char        eof_dance_tab_name[EOF_MAX_DIFFICULTIES][15];
extern char        eof_beatable_tab_name[EOF_MAX_DIFFICULTIES][15];
extern char        eof_track_diff_populated_status[256];	//For each of the 255 possible difficulties, the element is set to nonzero if populated
extern char        eof_track_diff_populated_tech_note_status[256];	//Same as eof_track_diff_populated_status[], but for tech notes
extern char        eof_track_diff_highlighted_status[256];	//For each of the 255 possible difficulties, the element is set to nonzero if any of its notes are highlighted
extern char        eof_track_diff_highlighted_tech_note_status[256];	//Same as eof_track_diff_highlighted_status[], but for tech notes
extern char      * eof_snap_name[11];
extern char      * eof_input_name[EOF_INPUT_NAME_NUM + 1];

extern char        eof_supports_mp3;
extern char        eof_supports_oggcat;
extern char        eof_just_played;
extern char        eof_mark_drums_as_cymbal;
extern char        eof_mark_drums_as_double_bass;
extern unsigned long eof_mark_drums_as_hi_hat;
extern unsigned long eof_pro_guitar_fret_bitmask;
extern char        eof_legacy_view;
extern char        eof_fingering_view;
extern char        eof_flat_dd_view;
extern unsigned char eof_2d_render_top_option;
extern char        eof_render_grid_lines;
extern char        eof_show_ch_sp_durations;

extern char eof_midi_initialized;			//Specifies whether Allegro was able to set up a MIDI device
extern EOF_SELECTION_DATA eof_selection;

extern FILE *eof_log_fp;
extern char eof_log_level;
extern char eof_enable_logging;

extern int eof_custom_zoom_level;
extern int eof_display_flats;

extern long xchart[EOF_MAX_FRETS];	//Stores coordinate values used for 3D rendering, updated by eof_set_3D_lane_positions()
extern long ychart[EOF_MAX_FRETS];	//Stores coordinate values used for 3D rendering, updated by eof_set_2D_lane_positions()

extern char eof_has_focus;
extern int eof_key_pressed;
extern int eof_key_char;
extern int eof_key_uchar;
extern int eof_key_code;
extern int eof_close_button_clicked;
extern int eof_key_shifts;
extern int eof_last_key_char;
extern int eof_last_key_code;

#define EOF_GP_DRUM_MAPPING_COUNT 50
extern unsigned char gp_drum_import_lane_1[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_2[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_2_rimshot[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_3[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_3_cymbal[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_3_hi_hat_pedal[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_3_hi_hat_open[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_4[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_4_cymbal[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_5[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_5_cymbal[EOF_GP_DRUM_MAPPING_COUNT];
extern unsigned char gp_drum_import_lane_6[EOF_GP_DRUM_MAPPING_COUNT];

extern unsigned char drums_rock_remap_lane_1;
extern unsigned char drums_rock_remap_lane_2;
extern unsigned char drums_rock_remap_lane_3;
extern unsigned char drums_rock_remap_lane_3_cymbal;
extern unsigned char drums_rock_remap_lane_4;
extern unsigned char drums_rock_remap_lane_4_cymbal;
extern unsigned char drums_rock_remap_lane_5;
extern unsigned char drums_rock_remap_lane_5_cymbal;
extern unsigned char drums_rock_remap_lane_6;

extern int eof_color_grid_lines;
extern unsigned eof_grid_line_opacity;
extern unsigned eof_grid_line_solid, eof_grid_line_gap;

extern DIALOG eof_import_to_track_dialog[];

extern char *ogg_profile_name;

void eof_show_mouse(BITMAP * bp);	//Shows the software mouse if it is being used
double eof_get_porpos_sp(EOF_SONG *sp, unsigned long pos);	//Returns the timestamp's position within a beat (percentage)
double eof_get_porpos(unsigned long pos);	//Calls eof_get_porpos_sp() against eof_song
double eof_get_porpos_sp_abs(EOF_SONG *sp, unsigned long pos);
	//Returns the number of beats (as a percentage) that the specified timestamp is relative to the project's first beat, as a way to track a timing measured in beats
	//To be used with eof_put_porpos_sp()
	//Returns 0.0 on error
long eof_put_porpos_sp(EOF_SONG *sp, unsigned long beat, double porpos, double offset);
	//Returns the timestamp of the specified position (defined as a percentage) within a specified beat, or -1 on error
	//porpos can be a positive or negative value larger in magnitude than 100.0 to indicate the position is more than one beat length from the specified beat
long eof_put_porpos(unsigned long beat, double porpos, double offset);	//Calls eof_put_porpos_sp() against eof_song
void eof_reset_lyric_preview_lines(void);	//Resets the preview line variables to 0
void eof_find_lyric_preview_lines(void);	//Sets the first and second preview line variables
void eof_emergency_stop_music(void);	//Stops audio playback
void eof_fix_catalog_selection(void);	//Ensures that a valid catalog entry is active, if any
unsigned long eof_count_selected_and_unselected_notes(unsigned long *total);
	//Returns the number of notes selected in the active track difficulty, sets values in the eof_selection structure
	//If total is not NULL, its value is incremented once for each note in the active difficulty, regardless of whether it's selected (to count the number of notes in the active difficulty)
	//Reset *total to 0 before calling if the intention is to get a count of all notes in the active track difficulty
unsigned long eof_count_selected_notes_a(unsigned long track, unsigned char diff);
	//Returns the number of notes in the specified track difficulty that are selected, or 0 upon error
unsigned long eof_get_selected_note_range(unsigned long *sel_start, unsigned long *sel_end, char function);
	//Returns the number of notes in the active track difficulty that are explicitly selected, allowing an easy way to check if some number of them are selected
	//If sel_start and sel_end are not NULL, information about the first and last selected note are returned through them
	//If function is zero, the information returned via the pointers is the index number of the notes, otherwise it is their timestamps
	//The notes are expected to be sorted.  Zero is returned on error.
void eof_fix_waveform_graph(void);	//Rebuild the waveform graph data if it exists
void eof_fix_spectrogram(void);	//Rebuild the spectrogram data if it exists
void eof_clear_input(void);	//Clears the keypress states in allegro's key[] array (except for CTRL, ALT and SHIFT) and uses eof_use_key() to clear EOF's key press handling system
void eof_prepare_undo(int type);	//Adds an undo state, creating a backup of the project every 10 undo states.
int eof_figure_difficulty(void);	//Returns -1 if the active track difficulty has no notes or pitched lyrics.  If the vocal track is active and has at least one pitched lyric, 0 is returned, otherwise the number of notes is returned
int eof_figure_part(void);	//Returns the active track number in terms of FoF's command line play functionality, or -1 on error
int d_hackish_edit_proc (int msg, DIALOG *d, int c);
int eof_set_display_mode_preset(int mode);
	//Sets one of the pre-set window sizes by calling eof_set_display_mode()
	//If mode is EOF_DISPLAY_CUSTOM, the resolution is set to eof_screen_width x eof_screen_height
int eof_set_display_mode_preset_custom_width(int mode, unsigned long width);
	//Sets one of pre-set window heights and uses the specified width
	//If mode is EOF_DISPLAY_CUSTOM, the height is set to eof_screen_height instead of a preset height
int eof_set_display_mode(unsigned long width, unsigned long height);
	//Sets the program window size, rebuilds sub-windows and sets related variables
	//Returns 0 on error, 1 on success or 2 if a display alteration (enabling x2 zoom or setting a custom window width) fails
void eof_set_3d_projection(void);	//Sets the 3d projection by calling ocd3d_set_projection() with the screen dimensions and vanishing coordinate
void eof_determine_phrase_status(EOF_SONG *sp, unsigned long track);
	//Re-applies the HOPO, SP, trill and tremolo status of each note in the specified track, as well as deleting empty SP, Solo, trill, tremolo and arpeggio phrases
void eof_cat_track_difficulty_string(char *str);	//Concatenates the current track name and difficulty name/number to the specified string
void eof_fix_window_title(void);
int eof_load_ogg_quick(char * filename);
	//Loads the specified OGG file with no error recovery
	//Returns zero on error
int eof_load_ogg(char * filename, char function);
	//Loads the specified OGG file, or if it does not exist, have the user browse for an audio file.  Upon success, eof_loaded_ogg_name is updated and nonzero is returned.
	//If the first attempt to load the OGG file fails, and FFMPEG is linked, it is used to re-encode the audio to OGG format and the loading of that file is attempted
	//If function is 1, and the user cancels loading audio, "second_of_silence.ogg" is loaded instead, and nonzero is returned.
	//If function is 2, the browsed file name is allowed to keep its original filename (instead of being forcibly renamed to guitar.ogg) if it has one of the following names:
	//  song.ogg, drums.ogg, rhythm.ogg, vocals.ogg, bass.ogg, keys.ogg, crowd.ogg
	//  This would generally be used after a file import
	//If function is 2 and ogg_profile_name is not NULL, the file name for the loaded OGG file is written to the pointer and ogg_profile_name is reset to NULL
	//  This is to ensure that the OGG profile for the new project created from a file import has the user-selected OGG file name
	//  Resetting it to NULL avoids writing to invalid memory after importing and undoing causes the function to reload the OGG file
int eof_load_complete_song(char * filename);
int eof_destroy_ogg(void);	//Frees chart audio
int eof_save_ogg(char * fn);	//Writes the memory buffered chart audio OGG (eof_music_data) to the specified file
void eof_render(void);
void eof_render_lyric_window(void);
void eof_render_3d_window(void);
	//Renders the 3D preview
	//Calls eof_render_lyric_window() instead if a vocal track is to be rendered
void eof_render_extended_ascii_fonts(void);	//A test function that prints prints each Unicode converted extended ASCII character to test that the eof_font has glyphs mapped appropriately

void eof_render_info_window(void);
void eof_render_fret_catalog_window(void);	//Renders the fret catalog in the Information panel window
void eof_display_info_panel(void);
	//Toggles the Information panel on/off, either initializing or destroying the eof_info_panel instance appropriately
	//If info.panel.txt does not exist in the program folder when the panel is being enabled, that file is copied out of eof.dat
	//If the file can't be restored from eof.dat or cannot be opened, the panel is disabled

void eof_render_notes_window(void);
int eof_load_data(void);	//Loads graphics and fonts from eof.dat
BITMAP *eof_load_pcx(char *filename);
	//Loads the specified PCX file from /resources if it exists, otherwise loads it from eof.dat
FONT *eof_load_bitmap_font(char *filename);
	//Loads the specified PCX bitmap from /resources if it exists, otherwise loads it from eof.dat
BITMAP *eof_scale_image(BITMAP *source, double value);
	//Builds a new bitmap containing the input bitmap scaled by the specified value (ie. 0.5 to decrease to half size)
	//Destroys the input bitmap and returns the new bitmap on success, or simply returns the input bitmap on error
void eof_destroy_data(void);	//Frees graphics and fonts from memory
char * eof_get_tone_name(int tone);	//Returns the name of the given note number (ie. C# or Db) based on the value of eof_display_flats
void eof_all_midi_notes_off(void);	//Sends a channel mode message to turn off all active notes, as per MIDI specification
void eof_stop_midi(void);	//To be called whenever playback stops, turning off all playing MIDI notes and destroying the MIDI queue
void eof_exit(void);		//Formally exits the program, releasing all data structures allocated

void eof_init_after_load(char initaftersavestate);
	//Initializes variables and cleans up notes, should be used after loading or creating a chart
	//Functions that import into a track (lyrics, GP, etc) should not use this function as it will do things like erase all undo information
	//If initaftersavestate is zero (as should be used when loading/importing a chart/MIDI/etc), more variables are forcibly initialized
int eof_beat_is_mandatory_anchor(EOF_SONG *sp, unsigned long beat);	//Returns nonzero if the specified beat is valid and represents a TS or tempo change that requires the beat to be an anchor by eof_cleanup_beat_flags()
void eof_cleanup_beat_flags(EOF_SONG *sp);	//Repairs the text event and anchor statuses for beats in the specified chart

void eof_scale_fretboard(unsigned long numlanes);
	//Prepares 2D screen layout offsets, etc. based on EOF's window display size and a given number of lanes (depending on the passed numlanes parameter)
	//If numlanes is 0, the number of lanes of the active track is used, otherwise the parameter itself is used
void eof_set_3D_lane_positions(unsigned long track);
	//Sets the 3D coordinate values in the global xchart[] array based on the needs of the specified track
	//If the array doesn't need to be changed, the function returns without recalculating the array's values
	//If a track value of 0 is passed, xchart[] is forcibly updated, such as to be used after changing window size, lefty mode, etc.
void eof_set_2D_lane_positions(unsigned long track);
	//Sets the 2D coordinate values in the global ychart[] array based on the needs of the specified track
	//If the array doesn't need to be changed, the function returns without recalculating the array's values
	//If a track value of 0 is passed, ychart[] is forcibly updated, such as to be used after changing window size, inverted note mode, etc.

void eof_start_logging(void);	//Opens the log file for writing if it isn't already open
void eof_stop_logging(void);	//Closes the log file if it is open
void eof_log(const char *text, int level);
	//If the log is open, writes the string to the log file, followed by a newline character, and flushes the I/O stream
	//Level indicates the minimum level of logging that must be in effect to log the message (ie. 1 = on, 2 = verbose)
	//Verbose logging should be disabled during chart creation/deletion due to the large amount of note creations/deletions
void eof_log_casual(const char *text, int level, int prefix, int newline);
	//Similar to eof_log(), except that it has an internal memory buffer four times the size of EOF_LOG_STRING_SIZE
	//only flushes to disk when the buffer has insufficient space for another EOF_LOG_STRING_SIZE number of bytes
	//Calling with a NULL value for text signals the function to flush to disk manually
	//If prefix is nonzero, the log ID number is not written in front of the provided text
	//If newline is nonzero, a newline character is appended to the provided text
void eof_log_notes(EOF_SONG *sp, unsigned long track);
	//Debug function that logs the position and length of each note in the specified track
#define EOF_LOG_STRING_SIZE 2048
extern char eof_log_string[EOF_LOG_STRING_SIZE];	//A string reserved for use with eof_log()
extern unsigned int eof_log_id;
	//This will be set to a random value when logging is started, so if multiple instances of EOF are writing to the same
	//log file (Windows does not prevent this), each instance's log entries can be identified separately from each other.
void eof_log_cwd(void);		//Logs the current working directory

void eof_init_colors(void);		//Initializes the color structures, to be called after eof_load_data()
void eof_set_color_set(void);	//Updates the eof_colors[] array with color data based on the current track and the user color set preference
void eof_switch_out_callback(void);	//Performs some logic that needs to occur when EOF loses foreground focus
void eof_switch_in_callback(void);	//Performs some logic that needs to occur when EOF gains foreground focus
void eof_close_button_callback(void);	//Sets eof_close_button_clicked to track that the close button was clicked
long eof_get_previous_note(long cnote, int function);
	//Returns the note that exists immediately before the specified note in the active track difficulty, or -1 if no such note exists
	//If function is nonzero, the added criterion of being at a different timestamp is also required, to allow for the existence of disjointed chords
int eof_note_is_gh3_hopo(EOF_SONG *sp, unsigned long track, unsigned long note, double threshold);
	//Returns nonzero if the specified note is a HOPO by GH3 rules:
	//	Is explicitly marked as a HOPO, OR:
	//	Is not explicitly marked as a non HOPO, and isn't a chord, and isn't the same as the previous note, and is less than (threshold) amount of quarter notes from a previous note
	//The specified track's format is not taken into account, the calling function must use that criterion if needed
int eof_note_is_hopo(unsigned long cnote);	//Returns nonzero if the specified note in the active track should be rendered as a HOPO, based on the current HOPO preview setting
void eof_read_global_keys(void);	//Reads and acts on various keyboard combinations, only if the chart is paused
void eof_lyric_logic(void);	//Performs various vocal editor logic
void eof_note_logic(void);	//Performs various note catalog logic
void eof_logic(void);		//Performs various editor logic
void eof_render_lyric_preview(BITMAP * bp);	//Renders lyric preview lines (found by eof_find_lyric_preview_lines()) to the specified bitmap
int eof_initialize(int argc, char * argv[]);	//Initializes various values, checks for crash recovery, parses command line parameters, etc.
#ifdef ALLEGRO_WINDOWS
	int eof_initialize_windows(void);	//Performs additional initialization for the Windows platform
#endif

void eof_seek_and_render_position(unsigned long track, unsigned char diff, unsigned long pos);
	//Seeks to the specified position and renders the screen, taking the AV delay into account

void eof_read_keyboard_input(char function);
	//Updates the keypress state variables
	//If function is nonzero, the numberpad number key strokes have the ASCII value discarded so that the main editor logic can detect the keys without conflicting with the other number keys.
	//  And the backspace key is remapped to delete in OS X to suit Mac keyboards often not having a dedicated delete key
	//Function needs to be zero when this function is used for dialog input or else the number pad keys cannot be used to type into input fields

void eof_use_key(void);
	//Erases the keypress state variables

void eof_hidden_mouse_callback(int flags);	//A mouse callback that uninstalls itself and restores the mouse position to (eof_mouse_x, eof_mouse_y) if it has moved

EOF_SONG *eof_create_new_project_select_pro_guitar(void);
	//Closes a project if one is open
	//Prompts the user for a pro guitar/bass track to activate
	//eof_etext[] is displayed to the user in the dialog
	//Creates a new EOF_SONG structure with two beats and sets the active track to the user-selected one
	//This is used to prepare for a Guitar Pro or Rocksmith import when no chart is loaded
	//Returns the created project, or NULL on error

int eof_identify_xml(char *fn);
	//Parses the XML file and determines what file type it is
	//Returns 0 on error or if the file type couldn't be determined
	//Returns 1 if the file is determined to be Rocksmith format
	//Returns 2 if the file is determined to be Go PlayAlong format

int eof_lookup_program_folder(char *path, unsigned long pathsize);
	//Gets the path to the running executable's folder, storing it into the specific char array of the specified size
	//Takes Windows vs. Mac logic into account
	//Returns 0 on error

int eof_validate_temp_folder(void);
	//Ensures that the current working directory is EOF's program folder and verifies that the temporary folder exists
	//Returns nonzero on error

void eof_add_extended_ascii_glyphs(void);
	//Merges the glyphs for extended ASCII characters 128 through 159 into the UTF-8 code points of eof_font so that these characters will
	//display properly when an extended ASCII string is converted to UTF-8

int eof_load_and_scale_hopo_images(double value);
	//Loads the images for the (non-GHL) HOPO gems scaled to the given value (ie. 0.50 for half size) from eof.dat into eof_image[]
	//Returns zero on error

unsigned int eof_get_display_panel_count(void);
	//Returns the number of panels that are enabled for display (info panel, 3D preview, notes panel)
int eof_increase_display_width_to_panel_count(int prompt);
	//Increases the program window size to the minimum needed to fit the active number of panels, if necessary
	//If prompt is nonzero, the user is asked whether to perform the window resize
	//If the user declines that prompt, or if the window resize fails, the non-default panels are disabled
	//Returns zero on error

void eof_draw_dashed_pixel(BITMAP *bmp, int x, int y, int d);
	//A specified number of consecutive calls to this function will render the pixel, and a specified number of consecutive calls will skip the render, then the pattern resets
	//This allows for drawing a dashed line when used as the pixel render function with do_line()
	//To configure the drawing parameters, call this function with bmp having a value of NULL, x being the number of consecutive pixels to render and y being the consecutive number of pixels to skip
void eof_draw_dashed_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color, unsigned drawpixels, unsigned skippixels);
	//Uses eof_draw_dashed_pixel() and do_line() to render the specified line as a dashed line, where each rendered segment is drawpixels long and each non-rendered segment is skippixels long

#endif
