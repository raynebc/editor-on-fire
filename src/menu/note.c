#include <allegro.h>
#include <ctype.h>
#include <string.h>
#include "../agup/agup.h"
#include "../undo.h"
#include "../dialog.h"
#include "../dialog/proc.h"
#include "../player.h"
#include "../utility.h"
#include "../main.h"
#include "../midi.h"
#include "../mix.h"
#include "../note.h"
#include "../pathing.h"
#include "../rs.h"	//For eof_is_string_muted()
#include "../tuning.h"	//For eof_rs_check_chord_name()
#include "edit.h"
#include "note.h"
#include "song.h"	//For eof_menu_track_selected_track_number()
#include "track.h"	//For tech view functions
#include "main.h"
#include "context.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char eof_solo_menu_mark_text[32] = "&Mark";
char eof_star_power_menu_mark_text[32] = "&Mark\t" CTRL_NAME "+W";
char eof_lyric_line_menu_mark_text[32] = "&Mark";
char eof_arpeggio_menu_mark_text[32] = "&Mark";
char eof_handshape_menu_mark_text[32] = "&Mark";
char eof_trill_menu_mark_text[32] = "&Mark";
char eof_tremolo_menu_mark_text[32] = "&Mark";
char eof_slider_menu_mark_text[32] = "&Mark";
char eof_trill_menu_text[32] = "&TrIll";
char eof_tremolo_menu_text[32] = "Tre&Molo";
int eof_menu_note_edit_pro_guitar_note_prompt_suppress = 0;

int eof_suppress_pitched_transpose_warning = 0;		//Set to nonzero if user opts to suppress the warning that selected notes could not pitch transpose

MENU eof_menu_solo_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_menu_copy_solos_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_menu_copy_solos_track_2, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_menu_copy_solos_track_3, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_menu_copy_solos_track_4, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_menu_copy_solos_track_5, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_menu_copy_solos_track_6, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_menu_copy_solos_track_7, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_copy_solos_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_copy_solos_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_menu_copy_solos_track_10, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_copy_solos_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_copy_solos_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_menu_copy_solos_track_13, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_copy_solos_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_solo_menu[] =
{
	{eof_solo_menu_mark_text, eof_menu_solo_mark, NULL, 0, NULL},
	{"&Remove", eof_menu_solo_unmark, NULL, 0, NULL},
	{"&Erase All", eof_menu_solo_erase_all, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_solo_edit_timing, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_solo_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_sp_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_menu_copy_sp_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_menu_copy_sp_track_2, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_menu_copy_sp_track_3, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_menu_copy_sp_track_4, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_menu_copy_sp_track_5, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_menu_copy_sp_track_6, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_menu_copy_sp_track_7, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_copy_sp_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_copy_sp_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_menu_copy_sp_track_10, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_copy_sp_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_copy_sp_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_menu_copy_sp_track_13, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_copy_sp_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_star_power_menu[] =
{
	{eof_star_power_menu_mark_text, eof_menu_star_power_mark, NULL, 0, NULL},
	{"&Remove", eof_menu_star_power_unmark, NULL, 0, NULL},
	{"&Erase All", eof_menu_star_power_erase_all, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_sp_edit_timing, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_sp_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_delete[] =
{
	{"Delete\tDel", eof_menu_note_delete, NULL, 0, NULL},
	{"Delete w/ FHP\tShift+Del", eof_menu_note_delete_with_fhp, NULL, 0, NULL},
	{"Delete w/ lower diffs\t" CTRL_NAME "+Shift+Del", eof_menu_note_delete_with_lower_difficulties, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_lyric_line_menu[] =
{
	{eof_lyric_line_menu_mark_text, eof_menu_lyric_line_mark, NULL, 0, NULL},
	{"&Remove", eof_menu_lyric_line_unmark, NULL, 0, NULL},
	{"&Erase All", eof_menu_lyric_line_erase_all, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_note_lyric_line_edit_timing, NULL, 0, NULL},
	{"Split &After selected", eof_menu_note_split_lyric_line_after_selected, NULL, 0, NULL},
	{"Re&Pair timing", eof_menu_note_lyric_line_repair_timing, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"Toggle &Overdrive", eof_menu_lyric_line_toggle_overdrive, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_arpeggio_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_menu_copy_arpeggio_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_menu_copy_arpeggio_track_2, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_menu_copy_arpeggio_track_3, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_menu_copy_arpeggio_track_4, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_menu_copy_arpeggio_track_5, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_menu_copy_arpeggio_track_6, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_menu_copy_arpeggio_track_7, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_copy_arpeggio_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_copy_arpeggio_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_menu_copy_arpeggio_track_10, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_copy_arpeggio_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_copy_arpeggio_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_menu_copy_arpeggio_track_13, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_copy_arpeggio_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_arpeggio_menu[] =
{
	{eof_arpeggio_menu_mark_text, eof_menu_arpeggio_mark, NULL, 0, NULL},
	{"&Remove", eof_menu_arpeggio_unmark, NULL, 0, NULL},
	{"&Erase All", eof_menu_arpeggio_erase_all, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_arpeggio_edit_timing, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_arpeggio_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_handshape_menu[] =
{
	{eof_handshape_menu_mark_text, eof_menu_handshape_mark, NULL, 0, NULL},
	{"&Remove", eof_menu_handshape_unmark, NULL, 0, NULL},
	{"&Erase All", eof_menu_handshape_erase_all, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_handshape_edit_timing, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_pro_guitar_slide_menu[] =
{
	{"Toggle slide up\t" CTRL_NAME "+Up", eof_menu_note_toggle_slide_up, NULL, 0, NULL},
	{"Toggle slide &Down\t" CTRL_NAME "+Down", eof_menu_note_toggle_slide_down, NULL, 0, NULL},
	{"Remove pitched &Slide", eof_menu_note_remove_pitched_slide, NULL, 0, NULL},
	{"&Reverse slide", eof_menu_note_reverse_slide, NULL, 0, NULL},
	{"Set &End fret\t" CTRL_NAME "+Shift+L", eof_pro_guitar_note_slide_end_fret_save, NULL, 0, NULL},
	{"Convert to &Unpitched", eof_menu_note_convert_slide_to_unpitched, NULL, 0, NULL},
	{"Convert to &Pitched", eof_menu_note_convert_slide_to_pitched, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_pro_guitar_strum_menu[] =
{
	{"Toggle strum &Up\tShift+U", eof_pro_guitar_toggle_strum_up, NULL, 0, NULL},
	{"Toggle strum &Mid\tShift+M", eof_pro_guitar_toggle_strum_mid, NULL, 0, NULL},
	{"Toggle strum &Down\tShift+D", eof_pro_guitar_toggle_strum_down, NULL, 0, NULL},
	{"&Remove strum direction", eof_menu_note_remove_strum_direction, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_trill_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_menu_copy_trill_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_menu_copy_trill_track_2, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_menu_copy_trill_track_3, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_menu_copy_trill_track_4, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_menu_copy_trill_track_5, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_menu_copy_trill_track_6, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_menu_copy_trill_track_7, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_copy_trill_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_copy_trill_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_menu_copy_trill_track_10, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_copy_trill_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_copy_trill_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_menu_copy_trill_track_13, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_copy_trill_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_trill_menu[] =
{
	{eof_trill_menu_mark_text, eof_menu_trill_mark, NULL, 0, NULL},
	{"&Remove", eof_menu_trill_unmark, NULL, 0, NULL},
	{"&Erase All", eof_menu_trill_erase_all, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_trill_edit_timing, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_trill_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_tremolo_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_menu_copy_tremolo_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_menu_copy_tremolo_track_2, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_menu_copy_tremolo_track_3, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_menu_copy_tremolo_track_4, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_menu_copy_tremolo_track_5, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_menu_copy_tremolo_track_6, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_menu_copy_tremolo_track_7, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_copy_tremolo_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_copy_tremolo_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_menu_copy_tremolo_track_10, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_copy_tremolo_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_copy_tremolo_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_menu_copy_tremolo_track_13, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_copy_tremolo_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_tremolo_menu[] =
{
	{eof_tremolo_menu_mark_text, eof_menu_tremolo_mark, NULL, 0, NULL},
	{"&Remove", eof_menu_tremolo_unmark, NULL, 0, NULL},
	{"&Erase All", eof_menu_tremolo_erase_all, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_tremolo_edit_timing, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_tremolo_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_slider_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_menu_copy_sliders_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_menu_copy_sliders_track_2, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_menu_copy_sliders_track_3, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_menu_copy_sliders_track_4, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_menu_copy_sliders_track_5, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_menu_copy_sliders_track_6, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_menu_copy_sliders_track_7, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_copy_sliders_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_copy_sliders_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_menu_copy_sliders_track_10, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_copy_sliders_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_copy_sliders_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_menu_copy_sliders_track_13, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_copy_sliders_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_slider_menu[] =
{
	{eof_slider_menu_mark_text, eof_menu_slider_mark, NULL, 0, NULL},
	{"&Remove", eof_menu_slider_unmark, NULL, 0, NULL},
	{"&Erase All", eof_menu_slider_erase_all, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_slider_edit_timing, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_slider_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_legacy_hopo_menu[] =
{
	{"&Auto", eof_menu_hopo_auto, NULL, 0, NULL},
	{"&Force On", eof_menu_hopo_force_on, NULL, 0, NULL},
	{"Force &Off", eof_menu_hopo_force_off, NULL, 0, NULL},
	{"&Cycle On/Off/Auto\tH", eof_menu_hopo_cycle, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_pro_guitar_hopo_menu[] =
{
	{"Toggle hammer on\tH", eof_menu_pro_guitar_toggle_hammer_on, NULL, 0, NULL},
	{"Remove &Hammer on", eof_menu_pro_guitar_remove_hammer_on, NULL, 0, NULL},
	{"Toggle pull off\tP", eof_menu_pro_guitar_toggle_pull_off, NULL, 0, NULL},
	{"Remove &Pull off", eof_menu_pro_guitar_remove_pull_off, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_toggle_menu[] =
{
	{eof_note_toggle_menu_string_1, eof_menu_note_toggle_green, NULL, 0, NULL},
	{eof_note_toggle_menu_string_2, eof_menu_note_toggle_red, NULL, 0, NULL},
	{eof_note_toggle_menu_string_3, eof_menu_note_toggle_yellow, NULL, 0, NULL},
	{eof_note_toggle_menu_string_4, eof_menu_note_toggle_blue, NULL, 0, NULL},
	{eof_note_toggle_menu_string_5, eof_menu_note_toggle_purple, NULL, 0, NULL},
	{eof_note_toggle_menu_string_6, eof_menu_note_toggle_orange, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_clear_menu[] =
{
	{eof_note_clear_menu_string_1, eof_menu_note_clear_green, NULL, 0, NULL},
	{eof_note_clear_menu_string_2, eof_menu_note_clear_red, NULL, 0, NULL},
	{eof_note_clear_menu_string_3, eof_menu_note_clear_yellow, NULL, 0, NULL},
	{eof_note_clear_menu_string_4, eof_menu_note_clear_blue, NULL, 0, NULL},
	{eof_note_clear_menu_string_5, eof_menu_note_clear_purple, NULL, 0, NULL},
	{eof_note_clear_menu_string_6, eof_menu_note_clear_orange, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_toggle_accent_menu[] =
{
	{eof_note_clear_menu_string_1, eof_menu_note_toggle_accent_green, NULL, 0, NULL},
	{eof_note_clear_menu_string_2, eof_menu_note_toggle_accent_red, NULL, 0, NULL},
	{eof_note_clear_menu_string_3, eof_menu_note_toggle_accent_yellow, NULL, 0, NULL},
	{eof_note_clear_menu_string_4, eof_menu_note_toggle_accent_blue, NULL, 0, NULL},
	{eof_note_clear_menu_string_5, eof_menu_note_toggle_accent_purple, NULL, 0, NULL},
	{eof_note_clear_menu_string_6, eof_menu_note_toggle_accent_orange, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_clear_accent_menu[] =
{
	{eof_note_clear_menu_string_1, eof_menu_note_clear_accent_green, NULL, 0, NULL},
	{eof_note_clear_menu_string_2, eof_menu_note_clear_accent_red, NULL, 0, NULL},
	{eof_note_clear_menu_string_3, eof_menu_note_clear_accent_yellow, NULL, 0, NULL},
	{eof_note_clear_menu_string_4, eof_menu_note_clear_accent_blue, NULL, 0, NULL},
	{eof_note_clear_menu_string_5, eof_menu_note_clear_accent_purple, NULL, 0, NULL},
	{eof_note_clear_menu_string_6, eof_menu_note_clear_accent_orange, NULL, 0, NULL},
	{"&All", eof_menu_note_clear_accent_all, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_toggle_ghost_menu[] =
{
	{eof_note_clear_menu_string_1, eof_menu_note_toggle_ghost_green, NULL, 0, NULL},
	{eof_note_clear_menu_string_2, eof_menu_note_toggle_ghost_red, NULL, 0, NULL},
	{eof_note_clear_menu_string_3, eof_menu_note_toggle_ghost_yellow, NULL, 0, NULL},
	{eof_note_clear_menu_string_4, eof_menu_note_toggle_ghost_blue, NULL, 0, NULL},
	{eof_note_clear_menu_string_5, eof_menu_note_toggle_ghost_purple, NULL, 0, NULL},
	{eof_note_clear_menu_string_6, eof_menu_note_toggle_ghost_orange, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_clear_ghost_menu[] =
{
	{eof_note_clear_menu_string_1, eof_menu_note_clear_ghost_green, NULL, 0, NULL},
	{eof_note_clear_menu_string_2, eof_menu_note_clear_ghost_red, NULL, 0, NULL},
	{eof_note_clear_menu_string_3, eof_menu_note_clear_ghost_yellow, NULL, 0, NULL},
	{eof_note_clear_menu_string_4, eof_menu_note_clear_ghost_blue, NULL, 0, NULL},
	{eof_note_clear_menu_string_5, eof_menu_note_clear_ghost_purple, NULL, 0, NULL},
	{eof_note_clear_menu_string_6, eof_menu_note_clear_ghost_orange, NULL, 0, NULL},
	{"&All", eof_menu_note_clear_ghost_all, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_freestyle_menu[] =
{
	{"&On", eof_menu_set_freestyle_on, NULL, 0, NULL},
	{"O&ff", eof_menu_set_freestyle_off, NULL, 0, NULL},
	{"&Toggle\tF", eof_menu_toggle_freestyle, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_drum_hi_hat_menu[] =
{
	{"&Open hi hat",eof_menu_note_default_open_hi_hat, NULL, 0, NULL},
	{"&Pedal hi hat",eof_menu_note_default_pedal_hi_hat, NULL, 0, NULL},
	{"&Sizzle hi hat",eof_menu_note_default_sizzle_hi_hat, NULL, 0, NULL},
	{"&Non hi hat",eof_menu_note_default_no_hi_hat, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_drum_accent_menu[] =
{
	{"&Toggle", NULL, eof_note_toggle_accent_menu, 0, NULL},
	{"&Clear", NULL, eof_note_clear_accent_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_drum_ghost_menu[] =
{
	{"&Toggle", NULL, eof_note_toggle_ghost_menu, 0, NULL},
	{"&Clear", NULL, eof_note_clear_ghost_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_drum_disjointed_menu[] =
{
	{"&Toggle", eof_menu_note_toggle_disjointed, NULL, 0, NULL},
	{"&Remove", eof_menu_note_remove_disjointed, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_drum_flam_menu[] =
{
	{"&Toggle\tSHIFT+F", eof_menu_note_toggle_flam, NULL, 0, NULL},
	{"&Remove", eof_menu_note_remove_flam, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_drum_menu[] =
{
	{"&Toggle cymbal\t" CTRL_NAME "+ALT+C", eof_menu_note_toggle_rb3_cymbal_all, NULL, 0, NULL},
	{"Toggle Yellow cymbal\t" CTRL_NAME "+Y", eof_menu_note_toggle_rb3_cymbal_yellow, NULL, 0, NULL},
	{"Toggle Blue cymbal\t" CTRL_NAME "+B", eof_menu_note_toggle_rb3_cymbal_blue, NULL, 0, NULL},
	{"Toggle Green cymbal\t" CTRL_NAME "+G", eof_menu_note_toggle_rb3_cymbal_green, NULL, 0, NULL},
	{"Mark as &Non cymbal", eof_menu_note_remove_cymbal, NULL, 0, NULL},
	{"Mark new notes as &Cymbals", eof_menu_note_default_cymbal, NULL, 0, NULL},
	{"Toggle expert+ bass drum\t" CTRL_NAME "+E", eof_menu_note_toggle_double_bass, NULL, 0, NULL},
	{"Remove expert+ bass drum", eof_menu_note_remove_double_bass, NULL, 0, NULL},
	{"Mark new notes as &Expert+", eof_menu_note_default_double_bass, NULL, 0, NULL},
	{"Toggle Y note as &Open hi hat\tShift+O", eof_menu_note_toggle_hi_hat_open, NULL, 0, NULL},
	{"Toggle Y note as &Pedal hi hat\tShift+P",eof_menu_note_toggle_hi_hat_pedal, NULL, 0, NULL},
	{"Toggle Y note as &Sizzle hi hat\tShift+S", eof_menu_note_toggle_hi_hat_sizzle, NULL, 0, NULL},
	{"Remove &Hi hat status", eof_menu_note_remove_hi_hat_status, NULL, 0, NULL},
	{"&Mark new Y notes as", NULL, eof_note_drum_hi_hat_menu, 0, NULL},
	{"Toggle R note as rim shot\t" CTRL_NAME "+Shift+R",eof_menu_note_toggle_rimshot, NULL, 0, NULL},
	{"Remove &Rim shot status", eof_menu_note_remove_rimshot, NULL, 0, NULL},
	{"Toggle &Y cymbal+tom\t" CTRL_NAME "+ALT+Y", eof_menu_note_toggle_rb3_cymbal_combo_yellow, NULL, 0, NULL},
	{"Toggle &B cymbal+tom\t" CTRL_NAME "+ALT+B", eof_menu_note_toggle_rb3_cymbal_combo_blue, NULL, 0, NULL},
	{"Toggle G cymbal+tom\t" CTRL_NAME "+ALT+G", eof_menu_note_toggle_rb3_cymbal_combo_green, NULL, 0, NULL},
	{"&Accent", NULL, eof_note_drum_accent_menu, 0, NULL},
	{"&Disjointed", NULL, eof_note_drum_disjointed_menu, 0, NULL},
	{"&Ghost", NULL, eof_note_drum_ghost_menu, 0, NULL},
	{"&Flam", NULL, eof_note_drum_flam_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_filtered_note_drum_menu[EOF_SCRATCH_MENU_SIZE];	//This will be built by eof_create_filtered_menu() to contain the contents of eof_note_drum_menu[] minus the hidden items

MENU eof_note_proguitar_menu[] =
{
	{"Edit pro guitar &Note\tN", eof_menu_note_edit_pro_guitar_note, NULL, 0, NULL},
	{"&Arpeggio", NULL, eof_arpeggio_menu, 0, NULL},
	{"s&Lide", NULL, eof_pro_guitar_slide_menu, 0, NULL},
	{"&Strum", NULL, eof_pro_guitar_strum_menu, 0, NULL},
	{"&Clear legacy bitmask", eof_menu_note_clear_legacy_values, NULL, 0, NULL},
	{"Toggle tapping\t" CTRL_NAME "+T", eof_menu_note_toggle_tapping, NULL, 0, NULL},
	{"Remove &Tapping", eof_menu_note_remove_tapping, NULL, 0, NULL},
	{"Toggle bend\t" CTRL_NAME "+B", eof_menu_note_toggle_bend, NULL, 0, NULL},
	{"Toggle pre-bend\t" CTRL_NAME "+SHIFT+N", eof_menu_note_toggle_prebend, NULL, 0, NULL},
	{"Remove &Bend", eof_menu_note_remove_bend, NULL, 0, NULL},
	{"Set bend strength\tShift+B", eof_pro_guitar_note_bend_strength_save, NULL, 0, NULL},
	{"Toggle palm muting\t" CTRL_NAME "+M", eof_menu_note_toggle_palm_muting, NULL, 0, NULL},
	{"Remove palm &Muting", eof_menu_note_remove_palm_muting, NULL, 0, NULL},
	{"Toggle harmonic\t" CTRL_NAME "+H", eof_menu_note_toggle_harmonic, NULL, 0, NULL},
	{"Remove &Harmonic", eof_menu_note_remove_harmonic, NULL, 0, NULL},
	{"Toggle vibrato\tShift+V", eof_menu_note_toggle_vibrato, NULL, 0, NULL},
	{"Remove &Vibrato", eof_menu_note_remove_vibrato, NULL, 0, NULL},
	{"Toggle ghost\t"  CTRL_NAME "+G", eof_menu_note_toggle_ghost, NULL, 0, NULL},
	{"Remove &Ghost", eof_menu_note_remove_ghost, NULL, 0, NULL},
	{"Toggle string mute\tShift+X", eof_menu_pro_guitar_toggle_string_mute, NULL, 0, NULL},
	{"Pitched tranpose &Up\tShift+Up", eof_menu_note_pitched_transpose_up, NULL, 0, NULL},
	{"Pitched tranpose &Down\tShift+Down", eof_menu_note_pitched_transpose_down, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_rocksmith_toggle_menu[] =
{
	{"&Pop\t" CTRL_NAME "+Shift+P", eof_menu_note_toggle_pop, NULL, 0, NULL},
	{"&Slap\t" CTRL_NAME "+Shift+S", eof_menu_note_toggle_slap, NULL, 0, NULL},
	{"&Accent\t" CTRL_NAME "+Shift+A", eof_menu_note_toggle_accent, NULL, 0, NULL},
	{"Pinch &Harmonic\tShift+H", eof_menu_note_toggle_pinch_harmonic, NULL, 0, NULL},
	{"Force sus&Tain", eof_menu_note_toggle_rs_sustain, NULL, 0, NULL},
	{"&Ignore\t" CTRL_NAME "+Shift+I", eof_menu_note_toggle_rs_ignore, NULL, 0, NULL},
	{"&Linknext\tShift+N", eof_menu_note_toggle_linknext, NULL, 0, NULL},
	{"&Fingerless", eof_menu_note_toggle_rs_fingerless, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_rocksmith_remove_menu[] =
{
	{"&Pop", eof_menu_note_remove_pop, NULL, 0, NULL},
	{"&Slap", eof_menu_note_remove_slap, NULL, 0, NULL},
	{"&Accent", eof_menu_note_remove_accent, NULL, 0, NULL},
	{"Pinch &Harmonic", eof_menu_note_remove_pinch_harmonic, NULL, 0, NULL},
	{"Force sus&Tain", eof_menu_note_remove_rs_sustain, NULL, 0, NULL},
	{"&Ignore", eof_menu_note_remove_rs_ignore, NULL, 0, NULL},
	{"&Linknext", eof_menu_note_remove_linknext, NULL, 0, NULL},
	{"&Fingerless", eof_menu_note_remove_rs_fingerless, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_rocksmith_menu[] =
{
	{"&Handshape", NULL, eof_handshape_menu, 0, NULL},
	{"&Toggle", NULL, eof_note_rocksmith_toggle_menu, 0, NULL},
	{"&Remove", NULL, eof_note_rocksmith_remove_menu, 0, NULL},
	{"Edit frets/fingering\tF", eof_menu_note_edit_pro_guitar_note_frets_fingers_menu, NULL, 0, NULL},
	{"Edit fingering\t" CTRL_NAME "+F", eof_menu_note_edit_pro_guitar_note_fingers, NULL, 0, NULL},
	{"Clear fingering", eof_menu_pro_guitar_remove_fingering, NULL, 0, NULL},
	{"&Lookup fingering", eof_menu_note_lookup_fingering, NULL, 0, NULL},
	{"Apply &Derived fingering", eof_menu_pro_guitar_apply_derived_fingering, NULL, 0, NULL},
	{"Define unpitched slide\t" CTRL_NAME "+U", eof_pro_guitar_note_define_unpitched_slide, NULL, 0, NULL},
	{"Remove &Unpitched slide", eof_menu_note_remove_unpitched_slide, NULL, 0, NULL},
	{"Remove all &Slide", eof_menu_note_remove_all_slide, NULL, 0, NULL},
	{"Mute->Single note P.M.", eof_rocksmith_convert_mute_to_palm_mute_single_note, NULL, 0, NULL},
	{"&Move t.n. to prev note", eof_menu_note_move_tech_note_to_previous_note_pos, NULL, 0, NULL},
	{"&Generate FHPs", eof_generate_efficient_hand_positions_for_selected_notes, NULL, 0, NULL},
	{"Remove FHPs", eof_delete_hand_positions_for_selected_notes, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_beatable_menu[] =
{
	{"Toggle &Left snap\t" CTRL_NAME "+Shift+L", eof_menu_beatable_toggle_beatable_lsnap, NULL, 0, NULL},
	{"Toggle &Right snap\t" CTRL_NAME "+Shift+R", eof_menu_beatable_toggle_beatable_rsnap, NULL, 0, NULL},
	{"&Remove slide\t" CTRL_NAME "+~", eof_menu_set_beatable_slide_lane_none, NULL, 0, NULL},
	{"Slide to L2\t" CTRL_NAME "+1", eof_menu_set_beatable_slide_lane_1, NULL, 0, NULL},
	{"Slide to L1\t" CTRL_NAME "+2", eof_menu_set_beatable_slide_lane_2, NULL, 0, NULL},
	{"Slide to R1\t" CTRL_NAME "+3", eof_menu_set_beatable_slide_lane_3, NULL, 0, NULL},
	{"Slide to R2\t" CTRL_NAME "+4", eof_menu_set_beatable_slide_lane_4, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_lyrics_menu[] =
{
	{"&Edit Lyric\tL", eof_edit_lyric_dialog, NULL, 0, NULL},
	{"Split Lyric\tShift+S", eof_menu_split_lyric, NULL, 0, NULL},
	{"&Lyric Lines", NULL, eof_lyric_line_menu, 0, NULL},
	{"&Freestyle", NULL, eof_note_freestyle_menu, 0, NULL},
	{"&Remove pitch", eof_menu_lyric_remove_pitch, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_reflect_menu[] =
{
	{"&Vertical", eof_menu_note_reflect_vertical, NULL, 0, NULL},
	{"&Horizontal", eof_menu_note_reflect_horizontal, NULL, 0, NULL},
	{"&Both", eof_menu_note_reflect_both, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_name_menu[] =
{
	{"&Edit", eof_menu_note_edit_name, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_crazy_menu[] =
{
	{"&Toggle\tT", eof_menu_note_toggle_crazy, NULL, 0, NULL},
	{"&Remove", eof_menu_note_remove_crazy, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_transpose_menu[] =
{
	{"&Up\tUp", eof_menu_note_transpose_up, NULL, 0, NULL},
	{"&Down\tDown", eof_menu_note_transpose_down, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_highlight_menu[] =
{
	{"&On", eof_menu_note_highlight_on, NULL, 0, NULL},
	{"O&Ff", eof_menu_note_highlight_off, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_move_grid_snap_menu[] =
{
	{"&Backward\t" CTRL_NAME "+[", eof_menu_note_move_back_grid_snap, NULL, 0, NULL},
	{"&Forward\t" CTRL_NAME "+]", eof_menu_note_move_forward_grid_snap, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_grid_snap_menu[] =
{
	{"&Resnap to this grid\tShift+R", eof_menu_note_resnap, NULL, 0, NULL},
	{"Resnap &Auto\tALT+R", eof_menu_note_resnap_auto, NULL, 0, NULL},
	{"&Move grid snap", NULL, eof_note_move_grid_snap_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_move_by_millisecond_menu[] =
{
	{"&Backward\t" CTRL_NAME "+[", eof_menu_note_move_back_millisecond, NULL, 0, NULL},
	{"&Forward\t" CTRL_NAME "+]", eof_menu_note_move_forward_millisecond, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_clone_hero_sp_deploy_menu[] =
{
	{"&Toggle", eof_menu_note_toggle_ch_sp_deploy, NULL, 0, NULL},
	{"&Remove", eof_menu_note_remove_ch_sp_deploy, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_clone_hero_menu[] =
{
	{"&Disjointed", NULL, eof_note_drum_disjointed_menu, 0, NULL},
	{"S&P deploy", NULL, eof_note_clone_hero_sp_deploy_menu, 0, NULL},
	{"Convert GHL &Open\t" CTRL_NAME "+G", eof_menu_note_convert_to_ghl_open, NULL, 0, NULL},
	{"&Swap GHL B/W gems", eof_menu_note_swap_ghl_black_white_gems, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_simplify_menu[] =
{
	{"C&Hords", eof_menu_note_simplify_chords, NULL, 0, NULL},
	{"Chords (&Low)", eof_menu_note_simplify_chords_low, NULL, 0, NULL},
	{"&Cymbals", eof_menu_note_simplify_cymbals, NULL, 0, NULL},
	{"&Toms", eof_menu_note_simplify_toms, NULL, 0, NULL},
	{"&Bass drum", eof_menu_note_simplify_bass_drum, NULL, 0, NULL},
	{"&Expert+ bass drum", eof_menu_note_simplify_double_bass_drum, NULL, 0, NULL},
	{"&String mutes", eof_menu_note_simplify_string_mute, NULL, 0, NULL},
	{"&Ghost", eof_menu_note_simplify_ghost, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_move_menu[] =
{
	{"&Start pos to seek pos\tALT+Left arrow", eof_menu_note_move_note_start, NULL, 0, NULL},
	{"&End pos to seek pos\tALT+Right arrow", eof_menu_note_move_note_end, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_filtered_note_simplify_menu[EOF_SCRATCH_MENU_SIZE];	//This will be built by eof_create_filtered_menu() to contain the contents of eof_note_simplify_menu[] minus the hidden items

MENU eof_note_menu[] =
{
	{"Toggle", NULL, eof_note_toggle_menu, 0, NULL},
	{"Cle&Ar", NULL, eof_note_clear_menu, 0, NULL},
	{"Transpose", NULL, eof_note_transpose_menu, 0, NULL},
	{"Move", NULL, eof_note_move_menu, 0, NULL},
	{"&Highlight", NULL, eof_note_highlight_menu, 0, NULL},
	{"Gr&Id snap", NULL, eof_note_grid_snap_menu, 0, NULL},
	{"&Solos", NULL, eof_solo_menu, 0, NULL},
	{"Star &Power", NULL, eof_star_power_menu, 0, NULL},
	{"Delete", NULL, eof_menu_delete, 0, NULL},
	{"&Name", NULL, eof_note_name_menu, 0, NULL},
	{"Cra&Zy", NULL, eof_note_crazy_menu, 0, NULL},
	{"H&OPO", NULL, eof_legacy_hopo_menu, 0, NULL},
	{eof_trill_menu_text, NULL, eof_trill_menu, 0, NULL},
	{eof_tremolo_menu_text, NULL, eof_tremolo_menu, 0, NULL},
	{"Slid&Er", NULL, eof_slider_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Drum", NULL, eof_note_drum_menu, 0, NULL},
	{"Pro &Guitar", NULL, eof_note_proguitar_menu, 0, NULL},
	{"&Rocksmith", NULL, eof_note_rocksmith_menu, 0, NULL},
	{"&BEATABLE", NULL, eof_note_beatable_menu, 0, NULL},
	{"&Lyrics", NULL, eof_note_lyrics_menu, 0, NULL},
	{"Re&Flect", NULL, eof_note_reflect_menu, 0, NULL},
	{"&Clone Hero", NULL, eof_note_clone_hero_menu, 0, NULL},
	{"Simplif&Y", NULL, eof_note_simplify_menu, 0, NULL},
	{"Remove statuses", eof_menu_remove_statuses, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_filtered_note_menu[EOF_SCRATCH_MENU_SIZE];	//This will be built by eof_create_filtered_menu() to contain the contents of eof_note_menu[] minus the hidden items

DIALOG eof_lyric_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)          (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  314, 106, 2,   23,  0,    0,      0,   0,   "Edit Lyric", NULL, NULL },
	{ d_agup_text_proc,   12,  84,  64,  8,   2,   23,  0,    0,      0,   0,   "Text:",      NULL, NULL },
	{ eof_edit_proc,   48,  80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_etext,    NULL, NULL },
	{ d_agup_button_proc, 67,  112, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",         NULL, NULL },
	{ d_agup_button_proc, 163, 112, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",     NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_split_lyric_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  314, 106, 2,   23,  0,    0,      0,   0,   "Split Lyric", NULL, NULL },
	{ d_agup_text_proc,   12,  84,  64,  8,   2,   23,  0,    0,      0,   0,   "Text:",       NULL, NULL },
	{ eof_edit_proc,   48,  80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_etext,     NULL, NULL },
	{ d_agup_button_proc, 67,  112, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",          NULL, NULL },
	{ d_agup_button_proc, 163, 112, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",          NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_note_name_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1)             (d2) (dp)              (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  314, 106, 2,   23,  0,    0,      0,               0,   "Edit note name", NULL, NULL },
	{ d_agup_text_proc,   12,  84,  64,  8,   2,   23,  0,    0,      0,               0,   "Text:",          NULL, NULL },
	{ eof_edit_proc,   48,  80,  254, 20,  2,   23,  0,    0,      EOF_NAME_LENGTH, 0,   eof_etext,        NULL, NULL },
	{ d_agup_button_proc, 67,  112, 84,  28,  2,   23,  '\r', D_EXIT, 0,               0,   "OK",             NULL, NULL },
	{ d_agup_button_proc, 163, 112, 78,  28,  2,   23,  0,    D_EXIT, 0,               0,   "Cancel",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_drum_roll_count_dialog[] =
{
	/* (proc)                         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1)  (d2) (dp)              (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  314, 106, 2,   23,  0,    0,          0,    0,   "Edit drum roll count", NULL, NULL },
	{ d_agup_text_proc,       12,  84,  96,  8,   2,   23,  0,    0,            0,    0,   "Number:",          NULL, NULL },
	{ eof_verified_edit_proc, 80,  80,  222, 20,  2,   23,  0,    0,          3,   0,   eof_etext,        "0123456789", NULL },
	{ d_agup_button_proc,   67,  112, 84,  28,  2,   23,  '\r', D_EXIT, 0,    0,   "OK",             NULL, NULL },
	{ d_agup_button_proc,  163, 112, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_prepare_note_menu(void)
{
	int vselected;
	unsigned long insp = 0, insolo = 0, inll = 0, inarpeggio = 0, intrill = 0, intremolo = 0, inslider = 0, inhandshape = 0;
	unsigned long spstart = 0, ssstart = 0, llstart = 0;
	unsigned long spend = 0, ssend = 0, llend = 0;
	unsigned long spp = 0, ssp = 0, llp = 0;
	unsigned long i, j;
	unsigned long tracknum;
	unsigned long sel_start = eof_chart_length, sel_end = 0;
	EOF_PHRASE_SECTION *sectionptr = NULL;
	unsigned long track_behavior = 0;

	if(eof_selected_track >= EOF_TRACKS_MAX)
		return;	//Invalid track selected

	if(eof_song && eof_song_loaded)
	{
		int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

		track_behavior = eof_song->track[eof_selected_track]->track_behavior;
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		if(eof_get_selected_note_range(&sel_start, &sel_end, 1))	//Find the start and end position of the collection of selected notes in the active difficulty
		{	//If one or more notes/lyrics are selected
			if(eof_vocals_selected)
			{	//PART VOCALS SELECTED
				for(j = 0; j < eof_song->vocal_track[tracknum]->lines; j++)
				{
					if((sel_end >= eof_song->vocal_track[tracknum]->line[j].start_pos) && (sel_start <= eof_song->vocal_track[tracknum]->line[j].end_pos))
					{
						inll = 1;
						llstart = sel_start;
						llend = sel_end;
						llp = j;
					}
				}
			}
			else
			{	//PART VOCALS NOT SELECTED
				for(j = 0; j < eof_get_num_star_power_paths(eof_song, eof_selected_track); j++)
				{	//For each star power path in the active track
					sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, j);
					if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
					{
						insp = 1;
						spstart = sel_start;
						spend = sel_end;
						spp = j;
					}
				}
				for(j = 0; j < eof_get_num_solos(eof_song, eof_selected_track); j++)
				{	//For each solo section in the active track
					sectionptr = eof_get_solo(eof_song, eof_selected_track, j);
					if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
					{
						insolo = 1;
						ssstart = sel_start;
						ssend = sel_end;
						ssp = j;
					}
				}
				if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
				{	//If a pro guitar track is active
					for(j = 0; j < eof_song->pro_guitar_track[tracknum]->arpeggios; j++)
					{	//For each arpeggio phrase in the active track
						if((sel_end >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (sel_start <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos) && (eof_song->pro_guitar_track[tracknum]->arpeggio[j].difficulty == eof_note_type))
						{	//If the selection overlaps any arpeggio/handshape phrases
							if(!(eof_song->pro_guitar_track[tracknum]->arpeggio[j].flags & EOF_RS_ARP_HANDSHAPE))
							{	//If this is an arpeggio phrase instead of a handshape phrase
								inarpeggio = 1;
							}
							else
							{
								inhandshape = 1;
							}
						}
					}
				}
				if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_KEYS_TRACK_BEHAVIOR))
				{	//If a legacy/pro guitar/bass/keys or drum track is active
					for(j = 0; j < eof_get_num_trills(eof_song, eof_selected_track); j++)
					{	//For each trill phrase in the active track
						sectionptr = eof_get_trill(eof_song, eof_selected_track, j);
						if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
						{
							intrill = 1;
						}
					}
					for(j = 0; j < eof_get_num_tremolos(eof_song, eof_selected_track); j++)
					{	//For each tremolo phrase in the active track
						sectionptr = eof_get_tremolo(eof_song, eof_selected_track, j);
						if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
						{	//If the track's difficulty limit has been removed
							if(sectionptr->difficulty != eof_note_type)	//And the tremolo section does not apply to the active track difficulty
								continue;
						}
						else
						{
							if(sectionptr->difficulty != 0xFF)	//Otherwise if the tremolo section does not apply to all track difficulties
								continue;
						}
						if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
						{
							intremolo = 1;
						}
					}
					for(j = 0; j < eof_get_num_sliders(eof_song, eof_selected_track); j++)
					{	//For each slider phrase in the active track
						sectionptr = eof_get_slider(eof_song, eof_selected_track, j);
						if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
						{
							inslider = 1;
						}
					}
				}//If a legacy/pro guitar/bass/keys or drum track is active
			}//PART VOCALS NOT SELECTED
		}//If one or more notes/lyrics are selected
		eof_note_reflect_menu[0].flags = 0;		//Note>Reflect>Vertical
		eof_note_reflect_menu[1].flags = 0;		//Note>Reflect>Horizontal
		eof_note_reflect_menu[2].flags = 0;		//Note>Reflect>Both
		vselected = eof_count_selected_and_unselected_notes(NULL);
		if(vselected)
		{	//ONE OR MORE NOTES/LYRICS SELECTED
			/* star power mark */
			sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, spp);
			if((sectionptr != NULL) && insp && (spstart == sectionptr->start_pos) && (spend == sectionptr->end_pos))
			{	//If the selected notes already match an existing star power phrase exactly
				eof_star_power_menu[0].flags = D_DISABLED;	//Note>Star Power>Mark/Remark
			}
			else
			{
				eof_star_power_menu[0].flags = 0;
			}

			/* solo mark */
			sectionptr = eof_get_solo(eof_song, eof_selected_track, ssp);
			if((sectionptr != NULL) && insolo && (ssstart == sectionptr->start_pos) && (ssend == sectionptr->end_pos))
			{	//If the selected notes already match an existing solo phrase exactly
				eof_solo_menu[0].flags = D_DISABLED;		//Note>Solos>Mark/Remark
			}
			else
			{
				eof_solo_menu[0].flags = 0;
			}

			/* lyric line mark */
			if(inll && (llstart == eof_song->vocal_track[0]->line[llp].start_pos) && (llend == eof_song->vocal_track[0]->line[llp].end_pos))
			{	//If the selected notes already match an existing lyric line exactly
				eof_lyric_line_menu[0].flags = D_DISABLED;	//Note>Lyrics>Lyric Lines>Mark/Remark
			}
			else
			{
				eof_lyric_line_menu[0].flags = 0;
			}

			eof_note_menu[6].flags = 0;	//Note>Solos> submenu
			eof_note_menu[7].flags = 0; //Note>Star Power> submenu
			if(((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT)) || (eof_selected_track == EOF_TRACK_KEYS))
			{	//If a legacy guitar or keys note is selected
				eof_note_menu[14].flags = 0;		//Note>Slider> submenu
			}
			else
			{	//Otherwise disable and hide this item
				eof_note_menu[14].flags = D_DISABLED | D_HIDDEN;
			}

			if(vselected == 1)
			{	//If only one note/lyric is selected
				eof_note_reflect_menu[1].flags = D_DISABLED;	//Note>Reflect>Horizontal
				eof_note_reflect_menu[2].flags = D_DISABLED;	//Note>Reflect>Both
			}
		}//ONE OR MORE NOTES/LYRICS SELECTED
		else
		{	//NO NOTES/LYRICS SELECTED
			eof_star_power_menu[0].flags = D_DISABLED;	//Note>Star Power>Mark/Remark
			eof_solo_menu[0].flags = D_DISABLED; 			//Note>Solos>Mark/Remark
			eof_lyric_line_menu[0].flags = D_DISABLED;		//Note>Lyrics>Lyric Lines>Mark/Remark
			eof_note_menu[6].flags = D_DISABLED; 		//Note>Solos> submenu
			eof_note_menu[7].flags = D_DISABLED; 		//Note>Star Power> submenu
			eof_note_menu[14].flags = D_DISABLED;		//Note>Slider> submenu
		}

		/* star power mark/remark */
		/* edit timing */
		if(insp)
		{
			eof_star_power_menu[1].flags = 0;	//Mark/re-mark
			eof_star_power_menu[3].flags = 0;	//Edit timing
			(void) ustrcpy(eof_star_power_menu_mark_text, "Re-&Mark\t" CTRL_NAME "+W");
		}
		else
		{
			eof_star_power_menu[1].flags = D_DISABLED;
			eof_star_power_menu[3].flags = D_DISABLED;
			(void) ustrcpy(eof_star_power_menu_mark_text, "&Mark\t" CTRL_NAME "+W");
		}

		/* star power copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_sp_copy_menu[i].flags = 0;

			if(!eof_get_num_star_power_paths(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has no star power phrases or is the active track
				eof_menu_sp_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* solo mark/remark */
		/* edit timing */
		if(insolo)
		{
			eof_solo_menu[1].flags = 0;
			eof_solo_menu[3].flags = 0;	//Edit timing
			(void) ustrcpy(eof_solo_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_solo_menu[1].flags = D_DISABLED;
			eof_solo_menu[3].flags = D_DISABLED;
			(void) ustrcpy(eof_solo_menu_mark_text, "&Mark");
		}

		/* solo copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_solo_copy_menu[i].flags = 0;

			if(!eof_get_num_solos(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has no solos or is the active track
				eof_menu_solo_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* lyric line */
		if(inll)
		{
			eof_lyric_line_menu[1].flags = 0;	//Note>Lyrics>Lyric Lines>Remove
			eof_lyric_line_menu[3].flags = 0;	//Note>Lyrics>Lyric Lines>Edit timing
			eof_lyric_line_menu[4].flags = 0;	//Note>Lyrics>Lyric Lines>Split after selected
			eof_lyric_line_menu[7].flags = 0; 	//Note>Lyrics>Lyric Lines>Toggle Overdrive
			(void) ustrcpy(eof_lyric_line_menu_mark_text, "Re-&Mark\t" CTRL_NAME "+M/X");
		}
		else
		{
			eof_lyric_line_menu[1].flags = D_DISABLED;
			eof_lyric_line_menu[3].flags = D_DISABLED;
			eof_lyric_line_menu[4].flags = D_DISABLED;
			eof_lyric_line_menu[7].flags = D_DISABLED;
			(void) ustrcpy(eof_lyric_line_menu_mark_text, "&Mark\t" CTRL_NAME "+M/X");
		}

		/* arpeggio mark/remark */
		/* edit timing */
		if(inarpeggio)
		{
			eof_arpeggio_menu[1].flags = 0;				//Note>Pro Guitar>Arpeggio>Remove
			eof_arpeggio_menu[3].flags = 0;				//Note>Pro Guitar>Arpeggio>Edit timing
			(void) ustrcpy(eof_arpeggio_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_arpeggio_menu[1].flags = D_DISABLED;
			eof_arpeggio_menu[3].flags = D_DISABLED;
			(void) ustrcpy(eof_arpeggio_menu_mark_text, "&Mark");
		}

		/* handshape mark/remark */
		/* edit timing */
		if(inhandshape)
		{
			eof_handshape_menu[1].flags = 0;			//Note>Pro Guitar>Handshape>Remove
			eof_handshape_menu[3].flags = 0;			//Note>Pro Guitar>Handshape>Edit timing
			(void) ustrcpy(eof_handshape_menu_mark_text, "Re-&Mark\t" CTRL_NAME "+Shift+H");
		}
		else
		{
			eof_handshape_menu[1].flags = D_DISABLED;
			eof_handshape_menu[3].flags = D_DISABLED;
			(void) ustrcpy(eof_handshape_menu_mark_text, "&Mark\t" CTRL_NAME "+Shift+H");
		}

		/* arpeggio copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_arpeggio_copy_menu[i].flags = 0;

			if((i + 1 < EOF_TRACKS_MAX) && (!eof_get_num_arpeggios(eof_song, i + 1) || (i + 1 == eof_selected_track) || (!eof_track_is_pro_guitar_track(eof_song, i + 1))))
			{	//If the track has no arpeggios, is the active track or is not a pro guitar/bass track
				eof_menu_arpeggio_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* star power erase all */
		if(eof_get_num_star_power_paths(eof_song, eof_selected_track) > 0)
		{	//If there are one or more star power paths in the active track
			eof_star_power_menu[2].flags = 0;	//Note>Star Power>Erase All
		}
		else
		{
			eof_star_power_menu[2].flags = D_DISABLED;
		}

		/* solo erase all */
		if(eof_get_num_solos(eof_song, eof_selected_track) > 0)
		{	//If there are one or more solo paths in the active track
			eof_solo_menu[2].flags = 0;	//Note>Solos>Erase All
		}
		else
		{
			eof_solo_menu[2].flags = D_DISABLED;
		}

		/* trill erase all */
		if(eof_get_num_trills(eof_song, eof_selected_track) > 0)
		{	//If there are one or more trill phrases in the active track
			eof_trill_menu[2].flags = 0;	//Note>Trill>Erase All
		}
		else
		{
			eof_trill_menu[2].flags = D_DISABLED;
		}

		/* tremolo erase all */
		if(eof_get_num_tremolos(eof_song, eof_selected_track) > 0)
		{	//If there are one or more tremolo phrases in the active track
			eof_tremolo_menu[2].flags = 0;	//Note>Tremolo>Erase All
		}
		else
		{
			eof_tremolo_menu[2].flags = D_DISABLED;
		}

		/* slider erase all */
		if(eof_get_num_sliders(eof_song, eof_selected_track) > 0)
		{	//If there are one or more slider phrases in the active track
			eof_slider_menu[2].flags = 0;	//Note>Slider>Erase All
		}
		else
		{
			eof_slider_menu[2].flags = D_DISABLED;
		}

		/* slider copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_slider_copy_menu[i].flags = 0;

			if(!eof_get_num_sliders(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has no star power phrases or is the active track
				eof_menu_slider_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* resnap */
		if(eof_snap_mode == EOF_SNAP_OFF)
		{
			eof_note_menu[5].text = "&Move by millisecond";
			eof_note_menu[5].child = eof_note_move_by_millisecond_menu;
		}
		else
		{
			eof_note_menu[5].text = "Gr&Id snap";
			eof_note_menu[5].child = eof_note_grid_snap_menu;
		}

		if(eof_vocals_selected)
		{	//PART VOCALS SELECTED, disable and hide many items
			eof_note_menu[0].flags = D_DISABLED | D_HIDDEN;	//Note>Toggle
			eof_note_menu[1].flags = D_DISABLED | D_HIDDEN;	//Note>Clear
			eof_note_menu[6].flags = D_DISABLED | D_HIDDEN;	//Note>Solos
			eof_note_menu[7].flags = D_DISABLED | D_HIDDEN;	//Note>Star power
			eof_note_menu[9].flags = D_DISABLED | D_HIDDEN;	//Note>Edit Name
			eof_note_menu[10].flags = D_DISABLED | D_HIDDEN;	//Note>Toggle Crazy
			eof_note_menu[11].flags = D_DISABLED | D_HIDDEN;	//Note>HOPO
			eof_note_menu[12].flags = D_DISABLED | D_HIDDEN;	//Note>Trill> submenu
			eof_note_menu[13].flags = D_DISABLED | D_HIDDEN;	//Note>Tremolo> submenu
			eof_note_menu[16].flags = D_DISABLED | D_HIDDEN;	//Note>Drum> submenu
			eof_note_menu[17].flags = D_DISABLED | D_HIDDEN;	//Note>Pro Guitar> submenu
			eof_note_menu[18].flags = D_DISABLED | D_HIDDEN;	//Note>Rocksmith> submenu
			eof_note_menu[21].flags = D_DISABLED | D_HIDDEN;	//Note>Reflect> submenu
			eof_note_menu[23].flags = D_DISABLED | D_HIDDEN;	//Note>Simplify> submenu

			eof_note_menu[20].flags = 0;	//Note>Lyrics> submenu
			if((eof_selection.current < eof_song->vocal_track[tracknum]->lyrics) && (vselected == 1))
			{	//Only enable edit and split lyric if only one lyric is selected
				eof_note_lyrics_menu[0].flags = 0;	//Note>Lyrics>Edit Lyric
				eof_note_lyrics_menu[1].flags = 0;	//Note>Lyrics>Split Lyric
			}
			else
			{
				eof_note_lyrics_menu[0].flags = D_DISABLED;
				eof_note_lyrics_menu[1].flags = D_DISABLED;
			}

			/* lyric lines */
			if((eof_song->vocal_track[tracknum]->lines > 0) || vselected)
			{
				eof_note_lyrics_menu[2].flags = 0;			//Note>Lyrics>Lyric Lines
			}
			else
			{
				eof_note_lyrics_menu[2].flags = D_DISABLED;
			}

			if(vselected)
			{
				eof_note_lyrics_menu[3].flags = 0;			//Note>Lyrics>Freestyle> submenu
			}
			else
			{
				eof_note_lyrics_menu[3].flags = D_DISABLED;
			}

			/* lyric lines erase all */
			if(eof_song->vocal_track[0]->lines > 0)
			{
				eof_lyric_line_menu[2].flags = 0;		//Note>Lyrics>Lyric Lines>Erase All
			}
			else
			{
				eof_lyric_line_menu[2].flags = D_DISABLED;
			}
		}//PART VOCALS SELECTED
		else
		{	//PART VOCALS NOT SELECTED
			eof_note_menu[0].flags = 0; 		//Note>Toggle
			eof_note_menu[1].flags = 0; 		//Note>Clear
			eof_note_menu[9].flags = 0;		//Note>Edit Name
			eof_note_menu[12].flags = 0;		//Note>Trill> submenu
			eof_note_menu[13].flags = 0;		//Note>Tremolo> submenu
			eof_note_menu[21].flags = 0;		//Note>Reflect> submenu
			eof_note_menu[23].flags = 0;		//Note>Simplify> submenu

			eof_note_menu[20].flags = D_DISABLED | D_HIDDEN;	//Note>Lyrics> submenu

			/* toggle crazy */
			if((track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (track_behavior == EOF_DRUM_TRACK_BEHAVIOR))
			{	//When a guitar or drum track is active
				eof_note_menu[10].flags = 0;				//Note>Toggle Crazy
			}
			else
			{	//Otherwise disable and hide this
				eof_note_menu[10].flags = D_DISABLED | D_HIDDEN;
			}

			if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
			{	//When a drum track is not active
				eof_note_menu[16].flags = D_DISABLED | D_HIDDEN;	//Note>Drum> submenu
				eof_note_simplify_menu[2].flags = D_DISABLED | D_HIDDEN;	//Note>Simplify>Cymbals
				eof_note_simplify_menu[3].flags = D_DISABLED | D_HIDDEN;	//Note>Simplify>Toms
				eof_note_simplify_menu[4].flags = D_DISABLED | D_HIDDEN;	//Note>Simplify>Bass drum
				eof_note_simplify_menu[5].flags = D_DISABLED | D_HIDDEN;	//Note>Simplify>Expert+ bass
			}
			else
			{
				eof_note_menu[16].flags = 0;
				eof_note_simplify_menu[2].flags = 0;
				eof_note_simplify_menu[3].flags = 0;
				eof_note_simplify_menu[4].flags = 0;
				eof_note_simplify_menu[5].flags = 0;

				if(eof_selected_track == EOF_TRACK_DRUM_PS)
				{	//If the PS drum track is active
					eof_note_drum_menu[9].flags = 0;		//Enable toggle Y note as open hi hat
					eof_note_drum_menu[10].flags = 0;	//Enable toggle Y note as pedal hi hat
					eof_note_drum_menu[11].flags = 0;	//Enable toggle Y note as sizzle hi hat
					eof_note_drum_menu[13].flags = 0;	//Enable mark new Y notes as submenu
					eof_note_drum_menu[14].flags = 0;	//Enable toggle R note as rim shot
					eof_note_drum_menu[15].flags = 0;	//Enable remove rim shot status
					eof_note_drum_menu[16].flags = 0;	//Enable toggle Y cymbal+tom
					eof_note_drum_menu[17].flags = 0;	//Enable toggle B cymbal+tom
					eof_note_drum_menu[18].flags = 0;	//Enable toggle G cymbal+tom
				}
				else
				{	//Otherwise disable and hide these items
					eof_note_drum_menu[9].flags = D_DISABLED | D_HIDDEN;
					eof_note_drum_menu[10].flags = D_DISABLED | D_HIDDEN;
					eof_note_drum_menu[11].flags = D_DISABLED | D_HIDDEN;
					eof_note_drum_menu[13].flags = D_DISABLED | D_HIDDEN;
					eof_note_drum_menu[14].flags = D_DISABLED | D_HIDDEN;
					eof_note_drum_menu[15].flags = D_DISABLED | D_HIDDEN;
					eof_note_drum_menu[16].flags = D_DISABLED | D_HIDDEN;
					eof_note_drum_menu[17].flags = D_DISABLED | D_HIDDEN;
					eof_note_drum_menu[18].flags = D_DISABLED | D_HIDDEN;
				}
			}

			/* toggle Expert+ bass drum */
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (eof_note_type == EOF_NOTE_AMAZING))
			{	//If the Amazing difficulty of a drum track is active
				eof_note_drum_menu[6].flags = 0;			//Note>Drum>Toggle Expert+ Bass Drum
			}
			else
			{
				eof_note_drum_menu[6].flags = D_DISABLED;
			}

			/* HOPO */
			if(track_behavior == EOF_GUITAR_TRACK_BEHAVIOR)
			{	//When a guitar track is active
				eof_note_menu[11].flags = 0;	//Enable Note>HOPO> submenu
				eof_note_menu[11].child = eof_legacy_hopo_menu;	//Set it to the legacy guitar HOPO menu
			}
			else if(track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR)
			{
				eof_note_menu[11].flags = 0;	//Enable Note>HOPO> submenu
				eof_note_menu[11].child = eof_pro_guitar_hopo_menu;	//Set it to the pro guitar HOPO menu
			}
			else
			{	//Otherwise disable and hide this menu
				eof_note_menu[11].flags = D_DISABLED | D_HIDDEN;
			}

			/* Pro Guitar mode notation> */
			if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
			{	//If the active track is a pro guitar track
				EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];

				eof_menu_delete[1].flags = 0;			//Note>Delete>Delete w/ FHP
				eof_note_menu[17].flags = 0;			//Note>Pro Guitar> submenu
				eof_note_menu[18].flags = 0;			//Note>Rocksmith> submenu
				eof_note_simplify_menu[6].flags = 0;	//Note>Simplify>String mutes
				eof_note_simplify_menu[7].flags = 0;	//Note>Simplify>Ghost
				if(tp->note == tp->technote)
				{	//If tech view is in effect
					eof_note_rocksmith_menu[6].flags = D_DISABLED;	//Note>Rocksmith>Lookup fingering
					eof_note_rocksmith_menu[12].flags = 0;			//Note>Rocksmith>Move tech note to prev note
					eof_note_rocksmith_menu[13].flags = D_DISABLED;	//Note>Rocksmith>Generate FHPs
					eof_note_proguitar_menu[8].flags = 0;				//Note>Pro Guitar>Toggle pre-bend
				}
				else
				{
					eof_note_rocksmith_menu[6].flags = 0;				//Note>Rocksmith>Lookup fingering
					eof_note_rocksmith_menu[12].flags = D_DISABLED;
					eof_note_proguitar_menu[8].flags = D_DISABLED;		//Note>Pro Guitar>Toggle pre-bend
					if(vselected > 1)
					{	//If multiple notes are selected
						eof_note_rocksmith_menu[13].flags = 0;		//Note>Rocksmith>Generate FHPs
					}
					else
					{	//Only one note is selected
						eof_note_rocksmith_menu[13].flags = D_DISABLED;
					}
				}

				/* Arpeggio>Erase all */
				/* Handshape>Erase all */
				eof_arpeggio_menu[2].flags = D_DISABLED;	//Unless an arpeggio phrase is found, disable Note>Pro Guitar>Arpeggio>Erase All
				eof_handshape_menu[2].flags = D_DISABLED;	//Unless a handshape phrase is found, disable Note>Rocksmith>Handshape>Erase All
				for(i = 0; i < tp->arpeggios; i++)
				{	//For each arpeggio/handshape phrase
					if(!(tp->arpeggio[i].flags & EOF_RS_ARP_HANDSHAPE))
					{	//If this is an arpeggio phrase instead of a handshape phrase
						eof_arpeggio_menu[2].flags = 0;		//Enable Note>Pro Guitar>Arpeggio>Erase All
					}
					else
					{	//This is a handshape phrase
						eof_handshape_menu[2].flags = 0;		//Enable Note>Rocksmith>Handshape>Erase All
					}
				}

				eof_note_proguitar_menu[20].flags = eof_pitched_transpose_possible(-1, 1) ? 0 : D_DISABLED;	//Note>Pro guitar>Pitched transpose up
				eof_note_proguitar_menu[21].flags = eof_pitched_transpose_possible(1, 1) ? 0 : D_DISABLED;	//Note>Pro guitar>Pitched transpose down
			}
			else
			{	//A pro guitar track is not active
				eof_menu_delete[1].flags = D_DISABLED;
				eof_note_menu[17].flags = D_DISABLED | D_HIDDEN;
				eof_note_menu[18].flags = D_DISABLED | D_HIDDEN;
				eof_note_simplify_menu[6].flags = D_DISABLED | D_HIDDEN;
				eof_note_simplify_menu[7].flags = D_DISABLED | D_HIDDEN;
			}

			/* Trill mark/remark*/
			/* edit timing */
			if(intrill)
			{
				eof_trill_menu[1].flags = 0;	//Note>Trill>Remove
				eof_trill_menu[3].flags = 0;	//Note>Trill>Edit timing
				(void) ustrcpy(eof_trill_menu_mark_text, "Re-&Mark");
			}
			else
			{
				eof_trill_menu[1].flags = D_DISABLED;
				eof_trill_menu[3].flags = D_DISABLED;
				(void) ustrcpy(eof_trill_menu_mark_text, "&Mark");
			}

			/* Trill copy from */
			for(i = 0; i < EOF_TRACKS_MAX; i++)
			{	//For each track supported by EOF
				eof_menu_trill_copy_menu[i].flags = 0;

				if(!eof_get_num_trills(eof_song, i + 1) || (i + 1 == eof_selected_track))
				{	//If the track has no trill phrases or is the active track
					eof_menu_trill_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
				}
			}

			/* Tremolo mark/remark */
			/* edit timing */
			if(intremolo)
			{
				eof_tremolo_menu[1].flags = 0;	//Note>Tremolo>Remove
				eof_tremolo_menu[3].flags = 0;	//Note>Tremolo>Edit timing
				(void) ustrcpy(eof_tremolo_menu_mark_text, "Re-&Mark\t" CTRL_NAME "+Shift+O");
			}
			else
			{
				eof_tremolo_menu[1].flags = D_DISABLED;
				eof_tremolo_menu[3].flags = D_DISABLED;
				(void) ustrcpy(eof_tremolo_menu_mark_text, "&Mark\t" CTRL_NAME "+Shift+O");
			}

			/* Tremolo copy from */
			for(i = 0; i < EOF_TRACKS_MAX; i++)
			{	//For each track supported by EOF
				eof_menu_tremolo_copy_menu[i].flags = 0;

				if(!eof_get_num_tremolos(eof_song, i + 1) || (i + 1 == eof_selected_track))
				{	//If the track has no tremolo phrases or is the active track
					eof_menu_tremolo_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
				}
			}

			/* Rename Trill and Tremolo menus as necessary for the drum track */
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_KEYS_TRACK_BEHAVIOR))
			{	//If a legacy/pro guitar/bass/keys track is active, set the guitar terminology for trill and tremolo sections
				(void) ustrcpy(eof_trill_menu_text, "&Trill");
				(void) ustrcpy(eof_tremolo_menu_text, "Tre&molo");
			}
			else if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If a legacy drum track is active, set the drum terminology for trill and tremolo sections
				(void) ustrcpy(eof_trill_menu_text, "Special Drum R&Oll");
				(void) ustrcpy(eof_tremolo_menu_text, "Drum Rol&L");
			}
			else
			{	//Disable and hide these submenus unless a track that can use them is active
				eof_note_menu[12].flags = D_DISABLED | D_HIDDEN;	//Note>Trill> submenu
				eof_note_menu[13].flags = D_DISABLED | D_HIDDEN;	//Note>Tremolo> submenu
			}
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_KEYS_TRACK_BEHAVIOR))
			{	//If this is a keys track, ensure the tremolo menu still gets disabled and hidden
				eof_note_menu[13].flags = D_DISABLED | D_HIDDEN;	//Note>Tremolo> submenu
			}

			/* Slider mark/remark */
			/* edit timing */
			if(inslider)
			{
				eof_slider_menu[1].flags = 0;	//Note>Slider>Remove
				eof_slider_menu[3].flags = 0;	//Note>Slider>Edit timing
				(void) ustrcpy(eof_slider_menu_mark_text, "Re-&Mark\tShift+S");
			}
			else
			{
				eof_slider_menu[1].flags = D_DISABLED;
				eof_slider_menu[3].flags = D_DISABLED;
				(void) ustrcpy(eof_slider_menu_mark_text, "&Mark\tShift+S");
			}

			/* Toggle>Purple */
			if(eof_count_track_lanes(eof_song, eof_selected_track) > 4)
			{	//If the active track has a sixth usable lane
				eof_note_toggle_menu[4].flags = 0;	//Note>Toggle>Purple
				eof_note_clear_menu[4].flags = 0;	//Note>Clear>Purple
			}
			else
			{
				eof_note_toggle_menu[4].flags = D_DISABLED;
				eof_note_clear_menu[4].flags = D_DISABLED;
			}

			/* Toggle>Orange */
			if(eof_count_track_lanes(eof_song, eof_selected_track) > 5)
			{	//If the active track has a sixth usable lane
				eof_note_toggle_menu[5].flags = 0;	//Note>Toggle>Orange
				eof_note_clear_menu[5].flags = 0;	//Note>Clear>Orange
			}
			else
			{
				eof_note_toggle_menu[5].flags = D_DISABLED;
				eof_note_clear_menu[5].flags = D_DISABLED;
			}
		}//PART VOCALS NOT SELECTED

		if(eof_track_is_beatable_mode(eof_song, eof_selected_track))
			eof_note_menu[19].flags = 0;		//Note>BEATABLE> submenu
		else
			eof_note_menu[19].flags = D_DISABLED | D_HIDDEN;

		/* transpose up */
		if(eof_transpose_possible(-1))
		{
			eof_note_transpose_menu[0].flags = 0;			//Note>Transpose>Up
		}
		else
		{
			eof_note_transpose_menu[0].flags = D_DISABLED;
		}

		/* transpose down */
		if(eof_transpose_possible(1))
		{
			eof_note_transpose_menu[1].flags = 0;			//Note>Transpose>Down
		}
		else
		{
			eof_note_transpose_menu[1].flags = D_DISABLED;
		}
		if(note_selection_updated)
		{	//If the note selection was originally empty and was dynamically updated
			(void) eof_menu_edit_deselect_all();	//Clear the note selection
		}

		/* Note>Clone Hero */
		if(eof_track_is_legacy_guitar(eof_song, eof_selected_track) || (eof_selected_track == EOF_TRACK_KEYS))
		{	//If a legacy guitar or the keys track is active
			eof_note_menu[22].flags = 0;	//Note>Clone Hero>

			if(eof_track_is_ghl_mode(eof_song, eof_selected_track))
			{	//If GHL mode is enabled
				eof_note_clone_hero_menu[2].flags = 0;	//Note>Clone Hero>Convert GHL Open
				eof_note_clone_hero_menu[3].flags = 0;	//Note>Clone Hero>Swap GHL B/W gems
			}
			else
			{
				eof_note_clone_hero_menu[2].flags = D_DISABLED;
				eof_note_clone_hero_menu[3].flags = D_DISABLED;
			}
		}
		else
		{	//Otherwise disable and hide this menu
			eof_note_menu[22].flags = D_DISABLED | D_HIDDEN;	//Note>Clone Hero>
		}

		if(eof_create_filtered_menu(eof_note_menu, eof_filtered_note_menu, EOF_SCRATCH_MENU_SIZE))
		{	//If the Note menu was recreated to filter out hidden items
			eof_main_menu[4].child = eof_filtered_note_menu;	//Use this in the main menu
			eof_right_click_menu_note[0].child = eof_filtered_note_menu;	//And the context menu
		}
		else
		{	//Otherwise use the unabridged Note menu
			eof_main_menu[4].child = eof_note_menu;
			eof_right_click_menu_note[0].child = eof_note_menu;
		}
		if(eof_create_filtered_menu(eof_note_drum_menu, eof_filtered_note_drum_menu, EOF_SCRATCH_MENU_SIZE))
		{	//If the Note>Drum menu was recreated to filter out hidden items
			eof_note_menu[16].child = eof_filtered_note_drum_menu;	//Use this in the Note menu
		}
		else
		{	//Otherwise use the unabridged Note>Drum menu
			eof_note_menu[16].child = eof_note_drum_menu;
		}
		if(eof_create_filtered_menu(eof_note_simplify_menu, eof_filtered_note_simplify_menu, EOF_SCRATCH_MENU_SIZE))
		{	//If the Note>Simplify menu was recreated to filter out hidden items
			eof_note_menu[23].child = eof_filtered_note_simplify_menu;	//Use this in the Note menu
		}
		else
		{	//Otherwise use the unabridged Note>Simplify menu
			eof_note_menu[23].child = eof_note_simplify_menu;
		}
	}//if(eof_song && eof_song_loaded)
}

void eof_menu_note_transpose_tech_notes(int dir)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long normalnote = 0;	//The normal note that a tech note is found to apply to
	unsigned long i;

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (!eof_track_is_pro_guitar_track(eof_song, eof_selected_track)) || eof_legacy_view || eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
		return;	//Don't run unless a pro guitar track is active, legacy view is not in effect and tech view is not in effect

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	if(tp->note != tp->technote)
	{	//If tech view is not in effect, check whether any of the tech notes apply to normal notes about to be transposed
		unsigned long higheststringmask = 1 << (tp->numstrings - 1);
		unsigned long max = (1 << tp->numstrings) - 1;	//This represents the combination of all usable lanes, based on the current track options (including open bass strumming)

		for(i = 0; i < tp->technotes; i++)
		{	//For each tech note in the active track
			if(tp->technote[i]->type == eof_note_type)
			{	//If the tech note is in the active difficulty
				if(eof_pro_guitar_tech_note_overlaps_a_note(tp, i, 0xFF, &normalnote))
				{	//If this tech note overlaps any normal note on any string
					if(normalnote < EOF_MAX_NOTES)
					{	//Bounds check
						if(eof_selection.multi[normalnote])
						{	//If the normal note this tech note overlaps is selected
							if((dir > 0) && !(tp->technote[i]->note & 1))
							{	//If transposing down, and this tech note has no gems on lane 1, it can transpose down one lane
								tp->technote[i]->note = (tp->technote[i]->note >> 1) & 63;
							}
							else if((dir < 0) && !(tp->technote[i]->note & higheststringmask))
							{	//If transposing up, and this tech note has no gems on the highest valid lane, it can transpose up one lane
								tp->technote[i]->note = (tp->technote[i]->note << 1) & max;
							}
						}
					}
				}
			}
		}
	}
}

int eof_menu_note_transpose_up(void)
{
	unsigned long i, j;
	unsigned long max = 31;	//This represents the combination of all usable lanes, based on the current track options (including open bass strumming)
	unsigned long flags, note, tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_transpose_possible(-1))
	{	//If selected notes can transpose up one
		if(eof_vocals_selected)
		{
			eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each lyric in the active track
				note = eof_get_note_note(eof_song, eof_selected_track, i);
				if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (note != EOF_LYRIC_PERCUSSION))
				{
					note++;
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
			}
		}
		else
		{	//Vocal track is not active
			if(eof_lane_six_enabled(eof_selected_track))
			{	//If lane 6 is enabled for use in the active track
				max = 63;
			}
			if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
			{	//Special case:  Legacy view is in effect, display pro guitar notes as 5 lane notes
				max = 31;
			}
			tracknum = eof_song->track[eof_selected_track]->tracknum;
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);

			//Transpose tech notes if applicable (ie. tech view is not already in effect), must be transposed first because after the normal note set is transposed, the applicable tech notes won't be identifiable
			eof_menu_note_transpose_tech_notes(-1);

			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the active track
				if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
					continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

				if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
				{	//If legacy view is in effect, get the note's legacy bitmask
					note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
				}
				else
				{	//Otherwise get the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
				}
				note = (note << 1) & max;
				if(!eof_track_is_ghl_mode(eof_song, eof_selected_track) && eof_open_strum_enabled(eof_selected_track) && (note & 32))
				{	//If GHL mode is not enabled, open strum is enabled, and this transpose operation resulted in a gem in lane 6 (open strum)
					flags = eof_get_note_flags(eof_song, eof_selected_track, i);
					eof_set_note_note(eof_song, eof_selected_track, i, 32);		//Clear all lanes except lane 6
					note = 32;													//Ensure remainder of this function's logic sees this change
					flags &= ~(EOF_NOTE_FLAG_CRAZY);	//Clear the crazy flag, which is invalid for open strum notes
					flags &= ~(EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO flags, which are invalid for open strum notes
					flags &= ~(EOF_NOTE_FLAG_NO_HOPO);
					eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				}
				///Update fret values first, because eof_set_note_note() will clear fret values for unused strings
				if(!eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
				{	//If a pro guitar note will tranpose, move the fret values accordingly (only if legacy view isn't in effect)
					for(j = 7; j > 0; j--)
					{	//For the upper 7 frets
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[j] = eof_song->pro_guitar_track[tracknum]->note[i]->frets[j-1];		//Cycle fret values up from lower lane
						eof_song->pro_guitar_track[tracknum]->note[i]->finger[j] = eof_song->pro_guitar_track[tracknum]->note[i]->finger[j-1];	//Do the same for fingering
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[0] = 0xFF;	//Re-initialize lane 1 to the default of (muted)
					eof_song->pro_guitar_track[tracknum]->note[i]->finger[0] = 0;		//Re-initialize lane 1 fingering
				}
				if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
				{	//If legacy view is in effect, set the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise set the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
				if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
				{	//If a pro guitar track is active, update the note's ghost bitmask
					note = eof_get_note_ghost(eof_song, eof_selected_track, i);
					note = (note << 1) & max;
					eof_set_note_ghost(eof_song, eof_selected_track, i, note);
				}
			}//For each note in the active track
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_transpose_down(void)
{
	unsigned long i, j;
	unsigned long note, tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_transpose_possible(1))
	{	//If selected notes can transpose down one
		if(eof_vocals_selected)
		{
			eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each lyric in the active track
				note = eof_get_note_note(eof_song, eof_selected_track, i);
				if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (note != EOF_LYRIC_PERCUSSION))
				{
					note--;
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
			}
		}
		else
		{
			tracknum = eof_song->track[eof_selected_track]->tracknum;
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);

			//Transpose tech notes if applicable (ie. tech view is not already in effect), must be transposed first because after the normal note set is transposed, the applicable tech notes won't be identifiable
			eof_menu_note_transpose_tech_notes(1);

			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the active track
				if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
					continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

				if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
				{	//If legacy view is in effect, get the note's legacy bitmask
					note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
				}
				else
				{	//Otherwise get the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
				}
				note = (note >> 1) & 63;
				///Update fret values first, because eof_set_note_note() will clear fret values for unused strings
				if(!eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
				{	//If a pro guitar note will tranpose, move the fret values accordingly (only if legacy view isn't in effect)
					for(j = 0; j < 7; j++)
					{	//For the 7 supported lower frets
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[j] = eof_song->pro_guitar_track[tracknum]->note[i]->frets[j+1];		//Cycle fret values down from upper lane
						eof_song->pro_guitar_track[tracknum]->note[i]->finger[j] = eof_song->pro_guitar_track[tracknum]->note[i]->finger[j+1];	//Do the same for fingering
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[7] = 0xFF;	//Re-initialize lane 7 to the default of (muted)
					eof_song->pro_guitar_track[tracknum]->note[i]->finger[7] = 0;		//Re-initialize lane 7 fingering
				}
				if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
				{	//If legacy view is in effect, set the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise set the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
				if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
				{	//If a pro guitar track is active, update the note's ghost bitmask
					note = eof_get_note_ghost(eof_song, eof_selected_track, i);
					note = (note >> 1) & 63;
					eof_set_note_ghost(eof_song, eof_selected_track, i, note);
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_pitched_transpose(int dir, char option)
{
	unsigned long max;	//This represents the combination of all usable lanes, based on the number of strings in the track
	unsigned long notectr, stringctr, targetstring, bitmask;
	EOF_PRO_GUITAR_TRACK *tp;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	unsigned char pitchmask, pitches[6] = {0}, stringpitch, note, ghost;
	char warned = 0;

	if(dir > 1)
		dir = 1;	//Bounds check the dir parameter
	else if(dir < -1)
		dir = -1;
	else if(!dir)
		return 0;	//A direction of 0 effectively means do nothing

	if(!eof_pitched_transpose_possible(dir, option))
	{
		if(option)
			allegro_message("No selected notes can pitch transpose %s", (dir < 0) ? "up" : "down");

		return 0;	//Don't run unless a pro guitar track is active, legacy view is not in effect, tech view is not in effect and the specified criterion of all notes or any notes being able to transpose is met
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];	//Simplify
	max = (1 << tp->numstrings) - 1;

	//Transpose tech notes if applicable, must be transposed first because after the normal note set is transposed, the applicable tech notes won't be identifiable
	eof_menu_note_transpose_tech_notes(dir);

	for(notectr = 0; notectr < eof_get_track_size(eof_song, eof_selected_track); notectr++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[notectr] || (eof_get_note_type(eof_song, eof_selected_track, notectr) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_note_can_pitch_transpose(eof_song, eof_selected_track, notectr, dir))
		{	//If this note can transpose
			char newfrets[8] = {0};

			note = eof_get_note_note(eof_song, eof_selected_track, notectr);
			ghost = eof_get_note_ghost(eof_song, eof_selected_track, notectr);
			pitchmask = eof_get_midi_pitches(eof_song, eof_selected_track, notectr, pitches);

			///Transpose fret values first, because eof_set_note_note() will clear fret values for unused strings
			for(stringctr = 0, bitmask = 1; stringctr < tp->numstrings; stringctr++, bitmask <<= 1)
			{	//For each string in this track
				if(!stringctr && (dir > 0))
					continue;	//If transposing down and this is the lowest string, skip it
				targetstring = stringctr - dir;		//Determine the string the pitch is tranposing to
				if(targetstring >= tp->numstrings)
					continue;	//If transposing up and this is the highest string, skip it

				if(pitchmask & bitmask)
				{	//If this string has a pitch to transpose
					stringpitch =  tp->tuning[targetstring] + eof_lookup_default_string_tuning_absolute(tp, eof_selected_track, targetstring) + tp->capo;	//Determine the pitch of the string the note pitch will transpose to
					if(stringpitch > pitches[stringctr])
						return 0;	//Logic error
					newfrets[targetstring] = pitches[stringctr] - stringpitch;
					if(newfrets[targetstring] > tp->numfrets)
						return 0;	//Logic error
				}
				else
				{	//There is no pitch (ie. this string isn't used, or is a ghosted or string muted gem)
					newfrets[targetstring] = tp->pgnote[notectr]->frets[stringctr];	//Retain the string's existing fret value and transpose it up/down one string
				}
			}
			memcpy(tp->pgnote[notectr]->frets, newfrets, sizeof(newfrets));

			//Transpose the note bitmask
			if(dir < 0)
				note = (note << 1) & max;	//Transpose up
			else
				note = (note >> 1) & max;	//Transpose down
			eof_set_note_note(eof_song, eof_selected_track, notectr, note);

			//Transpose the ghost bitmask
			if(dir < 0)
				ghost = (ghost << 1) & max;	//Transpose up
			else
				ghost = (ghost >> 1) & max;	//Transpose down
			eof_set_note_ghost(eof_song, eof_selected_track, notectr, ghost);
		}//If this note can transpose
		else
		{
			if(!eof_suppress_pitched_transpose_warning && !warned)
			{
				if(alert("At least one selected note could not pitch transpose", "", "They will be highlighted.", "OK", "Don't warn me", 0, 0) == 2)
				{	//If user opts opts to suppress this warning
					eof_suppress_pitched_transpose_warning = 1;
				}
				warned = 1;
			}
			tp->pgnote[notectr]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Apply highlight status
		}
	}//For each note in the active track

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_pitched_transpose_up(void)
{
	return eof_menu_note_pitched_transpose(-1, 1);
}

int eof_menu_note_pitched_transpose_down(void)
{
	return eof_menu_note_pitched_transpose(1, 1);
}

int eof_menu_note_transpose_up_octave(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(!eof_vocals_selected)			//If PART VOCALS is not active
		return 1;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if(eof_transpose_possible(-12))
	{	//If selected lyrics can move up one octave
		eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{	//For each lyric in the track
			if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (eof_song->vocal_track[tracknum]->lyric[i]->note != EOF_LYRIC_PERCUSSION))
			{	//If the lyric is selected and is not a percussion note
				eof_song->vocal_track[tracknum]->lyric[i]->note += 12;
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_transpose_down_octave(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(!eof_vocals_selected)		//If PART VOCALS is not active
		return 1;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if(eof_transpose_possible(12))
	{	//If selected lyrics can move down one octave
		eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{	//For each lyric in the track
			if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (eof_song->vocal_track[tracknum]->lyric[i]->note != EOF_LYRIC_PERCUSSION))
			{	//If the lyric is selected and is not a percussion note
				eof_song->vocal_track[tracknum]->lyric[i]->note -= 12;
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_resnap_logic(int any)
{
	unsigned long i, x, notepos, newnotepos = 0, tailpos;
	unsigned long oldnotes;
	char undo_made = 0;
	int note_selection_updated, user_warned = 0, cancel = 0;

	if(eof_snap_mode == EOF_SNAP_OFF)
	{
		return 1;
	}

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	oldnotes = eof_get_track_size(eof_song, eof_selected_track);
	undo_made = 1;
	eof_auto_adjust_sections(eof_song, eof_selected_track, 0, 0, any, &undo_made);		//Move sections to nearest appropriate grid snap
	(void) eof_auto_adjust_tech_notes(eof_song, eof_selected_track, 0, 0, any, &undo_made);	//Move tech notes to nearest appropriate grid snap
	for(i = 0; i < oldnotes; i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		/* snap the note itself */
		notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
		if(!any)
		{	//If the calling function wants to reposition the notes to the nearest snap position of the current grid snap setting
			eof_snap_logic(&eof_tail_snap, notepos);
			newnotepos = eof_tail_snap.pos;
		}
		else
		{
			(void) eof_is_any_beat_interval_position(notepos, NULL, NULL, NULL, &newnotepos, eof_prefer_midi_friendly_grid_snapping);	//Store the closest beat interval position of any size into newnotepos, taking the "prefer MIDI friendly grid snaps" preference into account
			if(newnotepos == ULONG_MAX)
			{	//If the nearest beat interval position was NOT determined
				newnotepos = notepos;	//Have the note retain its current timestamp
			}
		}
		if(!user_warned)
		{	//If the user hasn't been warned about resnapped notes overlapping and combining
			for(x = 0; x < oldnotes; x++)
			{	//For each note in the active track
				char note1ghosted = 0, note2ghosted = 0;

				if((eof_get_note_type(eof_song, eof_selected_track, x) != eof_note_type) || (newnotepos != eof_get_note_pos(eof_song, eof_selected_track, x)) || (x == i))
					continue;	//If this note is not in the active track difficulty or isn't at the same position as where the resnapped note will be moved or the note is being compared to itself, skip it

				if(eof_get_note_note(eof_song, eof_selected_track, i) == eof_get_note_ghost(eof_song, eof_selected_track, i))
				{	//If the note from the outer for loop is fully ghosted
					note1ghosted = 1;
				}
				if(eof_get_note_note(eof_song, eof_selected_track, x) == eof_get_note_ghost(eof_song, eof_selected_track, x))
				{	//If the note from the inner for loop is fully ghosted
					note2ghosted = 1;
				}

				if(!note1ghosted && !note2ghosted)
				{	//As long as neither note being merged is fully ghosted, warn the user
					eof_seek_and_render_position(eof_selected_track, eof_note_type, notepos);
					if(alert("Warning: One or more notes/lyrics will snap to the same position", "and will be automatically combined.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
					{	//If user opts opts to cancel the operation
						cancel = 1;
					}
					user_warned = 1;
					break;
				}
			}
		}
		if(cancel)
		{	//If the user canceled
			break;
		}
		eof_set_note_pos(eof_song, eof_selected_track, i, newnotepos);

		/* snap the tail */
		tailpos = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
		if(!any)
		{	//If the calling function wants to reposition the notes to the nearest snap position of the current grid snap setting
			eof_snap_logic(&eof_tail_snap, tailpos);
			newnotepos = eof_tail_snap.pos;
		}
		else
		{
			(void) eof_is_any_beat_interval_position(tailpos, NULL, NULL, NULL, &newnotepos, eof_prefer_midi_friendly_grid_snapping);	//Store the closest beat interval position of any size into newnotepos, taking the "prefer MIDI friendly grid snaps" preference into account
			if(newnotepos == ULONG_MAX)
			{	//If the nearest beat interval position was NOT determined
				newnotepos = tailpos;	//Have the tail retain its current ending timestamp
			}
		}
		eof_snap_logic(&eof_tail_snap, eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i));
		eof_snap_length_logic(&eof_tail_snap);
		if(eof_get_note_length(eof_song, eof_selected_track, i) > 1)
		{
			eof_snap_logic(&eof_tail_snap, eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i));
			eof_note_set_tail_pos(eof_song, eof_selected_track, i, newnotepos);
		}
	}
	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_resnap(void)
{
	return eof_menu_note_resnap_logic(0);
}

int eof_menu_note_resnap_auto(void)
{
	return eof_menu_note_resnap_logic(1);
}

int eof_menu_note_delete_logic(int delete_fhps)
{
	unsigned long i, d = 0;

	(void) eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	//Count the number of selected notes in the active track difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If this note is in the active track, is selected and is in the active difficulty
			d++;
		}
	}
	if(!d)
		return 1;	//If there isn't at least one selected note in the active track difficulty, return immediately

	eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note (in reverse order)
		if(eof_selection.multi[i - 1] && (eof_get_note_type(eof_song, eof_selected_track, i - 1) == eof_note_type))
		{	//If the note is in the active track difficulty and is selected
			eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);	//Delete any tech notes applying to this note
			if(delete_fhps)
			{	//If the calling function specified to delete any FHPs at the same timestamps of deleted notes
				eof_track_delete_fret_hand_position_at_pos(eof_selected_track, eof_note_type, eof_get_note_pos(eof_song, eof_selected_track, i - 1));
			}
			eof_track_delete_note(eof_song, eof_selected_track, i - 1);
			eof_selection.multi[i - 1] = 0;
		}
	}
	(void) eof_menu_edit_deselect_all();	//Clear selection data
	eof_track_fixup_notes(eof_song, eof_selected_track, 0);
	eof_reset_lyric_preview_lines();
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_determine_phrase_status(eof_song, eof_selected_track);

	return 1;
}

int eof_menu_note_delete(void)
{
	return eof_menu_note_delete_logic(0);
}

int eof_menu_note_delete_with_fhp(void)
{
	return eof_menu_note_delete_logic(1);
}

int eof_menu_note_delete_with_lower_difficulties(void)
{
	unsigned long ctr;
	char undo_made = 0;

	(void) eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	eof_track_sort_notes(eof_song, eof_selected_track);	//Ensure the notes are sorted ascending by timestamp and then by difficulty
	for(ctr = eof_get_track_size(eof_song, eof_selected_track); ctr > 0; ctr--)
	{	//For each note in the track, in reverse order
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr - 1] && (eof_get_note_type(eof_song, eof_selected_track, ctr - 1) == eof_note_type))
		{	//If the note is selected
			if(!undo_made)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
				undo_made = 1;
			}
			eof_track_delete_note_with_difficulties(eof_song, eof_selected_track, ctr - 1, -1);	//Delete this note as well as all notes at the same position in lower difficulties
		}
	}

	(void) eof_menu_edit_deselect_all();	//Clear selection data
	eof_track_fixup_notes(eof_song, eof_selected_track, 0);
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_determine_phrase_status(eof_song, eof_selected_track);

	return 1;
}

int eof_menu_note_toggle_green(void)
{
	unsigned long i;
	unsigned long flags, note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask ^= 1;
			if(!eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
		}
		else
		{	//Otherwise alter the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
			note ^= 1;	//Toggle off lane 1
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If green drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i - 1);
				flags &= (~EOF_DRUM_NOTE_FLAG_DBASS);		//Clear the Expert+ status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i - 1, flags);
			}
			else if(eof_selected_track == EOF_TRACK_BASS)
			{	//When a lane 1 bass note is added, open bass must be forced clear, because they use conflicting MIDI notation
				note &= ~(32);	//Clear the bit for lane 6 (open bass)
			}
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
			if(!note)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_red(void)
{
	unsigned long i;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask ^= 2;
			if(!eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
		}
		else
		{	//Otherwise alter the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
			note ^= 2;	//Toggle off lane 2
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
			if(!note)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_yellow(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask ^= 4;
			if(!eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
		}
		else
		{	//Otherwise alter the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
			note ^= 4;	//Toggle off lane 3
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
			if(!note)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If yellow drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i - 1);
				flags &= (~EOF_DRUM_NOTE_FLAG_Y_CYMBAL);	//Clear the Pro yellow cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i - 1, flags);
				if(eof_mark_drums_as_cymbal && (note & 4))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum], i - 1, EOF_DRUM_NOTE_FLAG_Y_CYMBAL, 1, 0);	//Set the yellow cymbal flag on all drum notes at this position
				}
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_blue(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask ^= 8;
			if(!eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
		}
		else
		{	//Otherwise alter the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
			note ^= 8;	//Toggle off lane 4
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
			if(!note)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If blue drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i - 1);
				flags &= (~EOF_DRUM_NOTE_FLAG_B_CYMBAL);	//Clear the Pro blue cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i - 1, flags);
				if(eof_mark_drums_as_cymbal && (note & 8))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum], i - 1, EOF_DRUM_NOTE_FLAG_B_CYMBAL, 1, 0);	//Set the blue cymbal flag on all drum notes at this position
				}
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_purple(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask ^= 16;
			if(!eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
		}
		else
		{	//Otherwise alter the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
			note ^= 16;	//Toggle off lane 5
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
			if(!note)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If green drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i - 1);
				flags &= (~EOF_DRUM_NOTE_FLAG_G_CYMBAL);	//Clear the Pro green cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i - 1, flags);
				if(eof_mark_drums_as_cymbal && (note & 16))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum], i - 1, EOF_DRUM_NOTE_FLAG_G_CYMBAL, 1, 0);	//Set the green cymbal flag on all drum notes at this position
				}
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_orange(void)
{
	unsigned long i;
	unsigned long flags, note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_track_lanes(eof_song, eof_selected_track) < 6)
	{
		return 1;	//Don't do anything if there is less than 6 lanes available
	}
	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask ^= 32;
			if(!eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask)
			{	//If all gems in the note have been toggled off
				eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
			}
		}
		else
		{
			if(eof_selected_track == EOF_TRACK_BASS)
			{	//When an open bass note is added, all other lanes must be forced clear, because they use conflicting MIDI notation
				flags = eof_get_note_flags(eof_song, eof_selected_track, i - 1);
				eof_set_note_note(eof_song, eof_selected_track, i - 1, 32);	//Clear all lanes except lane 6
				flags &= ~(EOF_NOTE_FLAG_CRAZY);		//Clear the crazy flag, which is invalid for open strum notes
				flags &= ~(EOF_NOTE_FLAG_F_HOPO);		//Clear the HOPO flags, which are invalid for open strum notes
				flags &= ~(EOF_NOTE_FLAG_NO_HOPO);
				eof_set_note_flags(eof_song, eof_selected_track, i - 1, flags);
			}
			else
			{	//Otherwise alter the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
				note ^= 32;	//Toggle off lane 6
				if(!note)
				{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
					eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
				}
				eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
				if(!note)
				{	//If all gems in the note have been toggled off
					eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
				}
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_green(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, check the note's legacy bitmask
			note = eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask;
		}
		else
		{	//Otherwise check the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
		}
		if(!(note & 1))
			continue;	//If this note doesn't have a green gem, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		note &= ~1;	//Clear the 1st lane gem
		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask = note;
		}
		else
		{	//Otherwise alter the note's normal bitmask
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
		}
		if(!note)
		{	//If all gems in the note have been toggled off
			eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_red(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, check the note's legacy bitmask
			note = eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask;
		}
		else
		{	//Otherwise check the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
		}
		if(!(note & 2))
			continue;	//If this note doesn't have a red gem, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		note &= ~2;	//Clear the 2nd lane gem
		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask = note;
		}
		else
		{	//Otherwise alter the note's normal bitmask
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
		}
		if(!note)
		{	//If all gems in the note have been toggled off
			eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_yellow(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, check the note's legacy bitmask
			note = eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask;
		}
		else
		{	//Otherwise check the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
		}
		if(!(note & 4))
			continue;	//If this note doesn't have a yellow gem, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		note &= ~4;	//Clear the 3rd lane gem
		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask = note;
		}
		else
		{	//Otherwise alter the note's normal bitmask
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
		}
		if(!note)
		{	//If all gems in the note have been toggled off
			eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_blue(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, check the note's legacy bitmask
			note = eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask;
		}
		else
		{	//Otherwise check the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
		}
		if(!(note & 8))
			continue;	//If this note doesn't have a blue gem, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		note &= ~8;	//Clear the 4th lane gem
		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask = note;
		}
		else
		{	//Otherwise alter the note's normal bitmask
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
		}
		if(!note)
		{	//If all gems in the note have been toggled off
			eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_purple(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}
	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, check the note's legacy bitmask
			note = eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask;
		}
		else
		{	//Otherwise check the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
		}
		if(!(note & 16))
			continue;	//If this note doesn't have a purple gem, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		note &= ~16;	//Clear the 5th lane gem
		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask = note;
		}
		else
		{	//Otherwise alter the note's normal bitmask
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
		}
		if(!note)
		{	//If all gems in the note have been toggled off
			eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_orange(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(eof_count_track_lanes(eof_song, eof_selected_track) < 6)
	{
		return 1;	//Don't do anything if there is less than 6 lanes available
	}
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track, in reverse order
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i - 1] || (eof_get_note_type(eof_song, eof_selected_track, i - 1) != eof_note_type))
			continue;	//If the note isn't selected or isn't in the active track difficulty, skip it

		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, check the note's legacy bitmask
			note = eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask;
		}
		else
		{	//Otherwise check the note's normal bitmask
			note = eof_get_note_note(eof_song, eof_selected_track, i - 1);
		}
		if(!(note & 32))
			continue;	//If this note does not have an orange gem, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		note &= ~32;	//Clear the 6th lane gem
		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//If legacy view is in effect, alter the note's legacy bitmask
			eof_song->pro_guitar_track[tracknum]->note[i - 1]->legacymask = note;
		}
		else
		{	//Otherwise alter the note's normal bitmask
			if(!note)
			{	//If the note's last gem is being toggled off, delete any overlapping tech notes first
				eof_track_delete_overlapping_tech_notes(eof_song, eof_selected_track, i - 1);
			}
			eof_set_note_note(eof_song, eof_selected_track, i - 1, note);
		}
		if(!note)
		{	//If all gems in the note have been toggled off
			eof_menu_note_cycle_selection_back(i - 1);	//Update the note selection
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_accent_lane(unsigned int lanenum)
{
	unsigned long i;
	unsigned char mask, undo_made = 0, accent;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if((eof_count_track_lanes(eof_song, eof_selected_track) < lanenum) || !lanenum)
	{
		return 1;	//Don't do anything if the specified lane number is higher than the number the active track contains or if it is otherwise invalid
	}
	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			mask = 1 << (lanenum - 1);
			if(eof_get_note_note(eof_song, eof_selected_track, i) & mask)
			{	//If the note has a gem on the specified lane
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				accent = eof_get_note_accent(eof_song, eof_selected_track, i);
				accent ^= mask;
				eof_set_note_accent(eof_song, eof_selected_track, i, accent);
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_accent_green(void)
{
	return eof_menu_note_toggle_accent_lane(1);
}

int eof_menu_note_toggle_accent_red(void)
{
	return eof_menu_note_toggle_accent_lane(2);
}

int eof_menu_note_toggle_accent_yellow(void)
{
	return eof_menu_note_toggle_accent_lane(3);
}

int eof_menu_note_toggle_accent_blue(void)
{
	return eof_menu_note_toggle_accent_lane(4);
}

int eof_menu_note_toggle_accent_purple(void)
{
	return eof_menu_note_toggle_accent_lane(5);
}

int eof_menu_note_toggle_accent_orange(void)
{
	return eof_menu_note_toggle_accent_lane(6);
}

int eof_menu_note_clear_accent_lane(unsigned int lanenum)
{
	unsigned long i;
	unsigned char accent, mask, undo_made = 0;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if((eof_count_track_lanes(eof_song, eof_selected_track) < lanenum) || !lanenum)
	{
		return 1;	//Don't do anything if the specified lane number is higher than the number the active track contains or if it is otherwise invalid
	}
	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			accent = eof_get_note_accent(eof_song, eof_selected_track, i);
			mask = 1 << (lanenum - 1);
			if((eof_get_note_note(eof_song, eof_selected_track, i) & mask) && (accent & mask))
			{	//If the note has a gem on the specified lane and it is accented
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				accent &= ~mask;
				eof_set_note_accent(eof_song, eof_selected_track, i, accent);
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_accent_green(void)
{
	return eof_menu_note_clear_accent_lane(1);
}

int eof_menu_note_clear_accent_red(void)
{
	return eof_menu_note_clear_accent_lane(2);
}

int eof_menu_note_clear_accent_yellow(void)
{
	return eof_menu_note_clear_accent_lane(3);
}

int eof_menu_note_clear_accent_blue(void)
{
	return eof_menu_note_clear_accent_lane(4);
}

int eof_menu_note_clear_accent_purple(void)
{
	return eof_menu_note_clear_accent_lane(5);
}

int eof_menu_note_clear_accent_orange(void)
{
	return eof_menu_note_clear_accent_lane(6);
}

int eof_menu_note_clear_accent_all(void)
{
	unsigned long i;
	unsigned char undo_made = 0;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			if(eof_get_note_accent(eof_song, eof_selected_track, i))
			{	//If this note has accent status
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_set_note_accent(eof_song, eof_selected_track, i, 0);	//Clear the accent mask
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_ghost_lane(unsigned int lanenum)
{
	unsigned long i;
	unsigned char mask, undo_made = 0, ghost;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if((eof_count_track_lanes(eof_song, eof_selected_track) < lanenum) || !lanenum)
	{
		return 1;	//Don't do anything if the specified lane number is higher than the number the active track contains or if it is otherwise invalid
	}
	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			mask = 1 << (lanenum - 1);
			if(eof_get_note_note(eof_song, eof_selected_track, i) & mask)
			{	//If the note has a gem on the specified lane
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				ghost = eof_get_note_ghost(eof_song, eof_selected_track, i);
				ghost ^= mask;
				eof_set_note_ghost(eof_song, eof_selected_track, i, ghost);
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_ghost_green(void)
{
	return eof_menu_note_toggle_ghost_lane(1);
}

int eof_menu_note_toggle_ghost_red(void)
{
	return eof_menu_note_toggle_ghost_lane(2);
}

int eof_menu_note_toggle_ghost_yellow(void)
{
	return eof_menu_note_toggle_ghost_lane(3);
}

int eof_menu_note_toggle_ghost_blue(void)
{
	return eof_menu_note_toggle_ghost_lane(4);
}

int eof_menu_note_toggle_ghost_purple(void)
{
	return eof_menu_note_toggle_ghost_lane(5);
}

int eof_menu_note_toggle_ghost_orange(void)
{
	return eof_menu_note_toggle_ghost_lane(6);
}

int eof_menu_note_clear_ghost_lane(unsigned int lanenum)
{
	unsigned long i;
	unsigned char ghost, mask, undo_made = 0;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if((eof_count_track_lanes(eof_song, eof_selected_track) < lanenum) || !lanenum)
	{
		return 1;	//Don't do anything if the specified lane number is higher than the number the active track contains or if it is otherwise invalid
	}
	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			ghost = eof_get_note_ghost(eof_song, eof_selected_track, i);
			mask = 1 << (lanenum - 1);
			if((eof_get_note_note(eof_song, eof_selected_track, i) & mask) && (ghost & mask))
			{	//If the note has a gem on the specified lane and it is ghosted
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				ghost &= ~mask;
				eof_set_note_ghost(eof_song, eof_selected_track, i, ghost);
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_ghost_green(void)
{
	return eof_menu_note_clear_ghost_lane(1);
}

int eof_menu_note_clear_ghost_red(void)
{
	return eof_menu_note_clear_ghost_lane(2);
}

int eof_menu_note_clear_ghost_yellow(void)
{
	return eof_menu_note_clear_ghost_lane(3);
}

int eof_menu_note_clear_ghost_blue(void)
{
	return eof_menu_note_clear_ghost_lane(4);
}

int eof_menu_note_clear_ghost_purple(void)
{
	return eof_menu_note_clear_ghost_lane(5);
}

int eof_menu_note_clear_ghost_orange(void)
{
	return eof_menu_note_clear_ghost_lane(6);
}

int eof_menu_note_clear_ghost_all(void)
{
	unsigned long i;
	unsigned char undo_made = 0;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			if(eof_get_note_ghost(eof_song, eof_selected_track, i))
			{	//If this note has ghost status
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_set_note_ghost(eof_song, eof_selected_track, i, 0);	//Clear the ghost bitmask
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_crazy(void)
{
	unsigned long i, crazycount1 = 0, crazycount2 = 0;
	int u = 0;	//Is set to nonzero when an undo state has been made
	unsigned long track_behavior = eof_song->track[eof_selected_track]->track_behavior;
	unsigned long flags;
	int note_selection_updated;

	if((track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_DANCE_TRACK_BEHAVIOR) && (track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a guitar, dance or drum track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If the note isn't selected or in the active track difficulty, skip it

		if(!u)
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		if(flags & EOF_NOTE_FLAG_CRAZY)
		{	//If this note has crazy status
			crazycount1++;	//Track the number of notes that this function modified
		}
		flags ^= EOF_NOTE_FLAG_CRAZY;
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	//There are many circumstances where crazy status may be enforced for a note that would effectively undo this operation
	//Run fixup function and compare the number of selected notes that still have crazy status with those that did at the beginning of the function
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_flags(eof_song, eof_selected_track, i) & EOF_NOTE_FLAG_CRAZY)
			{	//If this note has crazy status
				crazycount2++;	//Track the number of selected notes that still have crazy status
			}
		}
	}
	if(u)
	{	//If an undo state was made
		if(crazycount1 == crazycount2)
		{	//If the same number of selected notes have crazy status as did before this function modified anything
			(void) eof_remove_undo();	//Remove the undo state that was created
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_crazy(void)
{
	unsigned long i, crazycount1 = 0, crazycount2 = 0;
	long u = 0;
	unsigned long flags;
	int note_selection_updated;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_NOTE_FLAG_CRAZY)
			{	//If this note has crazy status
				crazycount1++;	//Track the number of notes that this function modified
				if(!u)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_NOTE_FLAG_CRAZY;	//Remove crazy status
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	//There are many circumstances where crazy status may be enforced for a note that would effectively undo this operation
	//Run fixup function and compare the number of selected notes that still have crazy status with those that did at the beginning of the function
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_flags(eof_song, eof_selected_track, i) & EOF_NOTE_FLAG_CRAZY)
			{	//If this note has crazy status
				crazycount2++;	//Track the number of selected notes that still have crazy status
			}
		}
	}
	if(u)
	{	//If an undo state was made
		if(crazycount1 == crazycount2)
		{	//If the same number of selected notes have crazy status as did before this function modified anything
			(void) eof_remove_undo();	//Remove the undo state that was created
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_double_bass(void)
{
	unsigned long i;
	long u = 0;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == EOF_NOTE_AMAZING) && (eof_get_note_note(eof_song, eof_selected_track, i) & 1))
		{	//If this note is in the currently active track, is selected, is in the Expert difficulty and has a green gem
			if(!u)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_xor_note_flags(eof_song, eof_selected_track, i, EOF_DRUM_NOTE_FLAG_DBASS);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_double_bass(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == EOF_NOTE_AMAZING) && (eof_get_note_note(eof_song, eof_selected_track, i) & 1))
		{	//If this note is in the currently active track, is selected, is in the Expert difficulty and has a green gem
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_DRUM_NOTE_FLAG_DBASS)
			{	//If this note has double bass status
				if(!u)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_DRUM_NOTE_FLAG_DBASS;	//Remove double bass status
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_green_logic(int function, char *undo_made)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;		//Tracks whether this function call made any changes, to call fixup logic accordingly
	int note_selection_updated;
	EOF_LEGACY_TRACK *tp;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active
	if(function && (eof_selected_track != EOF_TRACK_DRUM_PS))
		return 1;	//Do not allow this function to apply tom/cymbal combo status except in the Phase Shift drum track
	if(!undo_made)
		return 1;	//Invalid parameter

	tp = eof_song->legacy_track[tracknum];
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 16))
			continue;	//If this drum note contains no purple gem (represents a green drum in Rock Band), skip it

		if(!function)
		{	//Toggle green cymbal status
			if(!*undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			eof_set_flags_at_legacy_note_pos(tp,i,EOF_DRUM_NOTE_FLAG_G_CYMBAL,2,0);	//Toggle the green cymbal flag on all drum notes at this position in all difficulties
			u = 1;
		}
		else
		{	//Toggle green tom/cymbal combo status
			if(tp->note[i]->flags & EOF_DRUM_NOTE_FLAG_G_CYMBAL)
			{	//If this note is already a cymbal
				if(!*undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				tp->note[i]->flags ^= EOF_DRUM_NOTE_FLAG_G_COMBO;						//Toggle the green tom/cymbal combo flag on this note only
				u = 1;
			}
		}
	}
	if(u)
	{	//If changes were made in this function call
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Remove tom+cymbal combo status from notes that are no longer cymbals
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_green(void)
{
	char undo_made = 0;
	return eof_menu_note_toggle_rb3_cymbal_green_logic(0, &undo_made);	//Toggle normal cymbal status
}

int eof_menu_note_toggle_rb3_cymbal_combo_green(void)
{
	char undo_made = 0;
	return eof_menu_note_toggle_rb3_cymbal_green_logic(1, &undo_made);	//Toggle tom/cymbal combo status
}

int eof_menu_note_toggle_rb3_cymbal_yellow_logic(int function, char *undo_made)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;		//Tracks whether this function call made any changes, to call fixup logic accordingly
	int note_selection_updated;
	EOF_LEGACY_TRACK *tp;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active
	if(function && (eof_selected_track != EOF_TRACK_DRUM_PS))
		return 1;	//Do not allow this function to apply tom/cymbal combo status except in the Phase Shift drum track
	if(!undo_made)
		return 1;	//Invalid parameter

	tp = eof_song->legacy_track[tracknum];
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 4))
			continue;	//If this drum note has no yellow gem, skip it

		if(!function)
		{	//Toggle yellow cymbal status
			if(!*undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			eof_set_flags_at_legacy_note_pos(tp,i,EOF_DRUM_NOTE_FLAG_Y_CYMBAL,2,0);	//Toggle the yellow cymbal flag on all drum notes at this position
			u = 1;
		}
		else
		{	//Toggle yellow tom/cymbal combo status
			if(tp->note[i]->flags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL)
			{	//If this note is already a cymbal
				if(!*undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				tp->note[i]->flags ^= EOF_DRUM_NOTE_FLAG_Y_COMBO;					//Toggle the yellow tom/cymbal combo flag on this note only
				u = 1;
			}
		}
	}
	if(u)
	{	//If changes were made in this function call
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Remove tom+cymbal combo status from notes that are no longer cymbals
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_yellow(void)
{
	char undo_made = 0;
	return eof_menu_note_toggle_rb3_cymbal_yellow_logic(0, &undo_made);	//Toggle normal cymbal status
}

int eof_menu_note_toggle_rb3_cymbal_combo_yellow(void)
{
	char undo_made = 0;
	return eof_menu_note_toggle_rb3_cymbal_yellow_logic(1, &undo_made);	//Toggle tom/cymbal combo status
}

int eof_menu_note_toggle_rb3_cymbal_blue_logic(int function, char *undo_made)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;
	int note_selection_updated;
	EOF_LEGACY_TRACK *tp;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active
	if(function && (eof_selected_track != EOF_TRACK_DRUM_PS))
		return 1;	//Do not allow this function to apply tom/cymbal combo status except in the Phase Shift drum track
	if(!undo_made)
		return 1;	//Invalid parameter

	tp = eof_song->legacy_track[tracknum];
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 8))
			continue;	//If this drum note has no blue gem, skip it

		if(!function)
		{	//Toggle blue cymbal status
			if(!*undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			eof_set_flags_at_legacy_note_pos(tp,i,EOF_DRUM_NOTE_FLAG_B_CYMBAL,2,0);	//Toggle the blue cymbal flag on all drum notes at this position
			u = 1;
		}
		else
		{	//Toggle blue tom/cymbal combo status
			if(tp->note[i]->flags & EOF_DRUM_NOTE_FLAG_B_CYMBAL)
			{	//If this note is already a cymbal
				if(!*undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				tp->note[i]->flags ^= EOF_DRUM_NOTE_FLAG_B_COMBO;					//Toggle the blue tom/cymbal combo flag on this note only
				u = 1;
			}
		}
	}
	if(u)
	{	//If changes were made in this function call
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Remove tom+cymbal combo status from notes that are no longer cymbals
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_blue(void)
{
	char undo_made = 0;
	return eof_menu_note_toggle_rb3_cymbal_blue_logic(0, &undo_made);	//Toggle normal cymbal status
}

int eof_menu_note_toggle_rb3_cymbal_combo_blue(void)
{
	char undo_made = 0;
	return eof_menu_note_toggle_rb3_cymbal_blue_logic(1, &undo_made);	//Toggle tom/cymbal combo status
}

int eof_menu_note_toggle_rb3_cymbal_all(void)
{
	char undo_made = 0;
	(void) eof_menu_note_toggle_rb3_cymbal_green_logic(0, &undo_made);	//Toggle normal green cymbal status
	(void) eof_menu_note_toggle_rb3_cymbal_yellow_logic(0, &undo_made);	//Toggle normal yellow cymbal status
	return eof_menu_note_toggle_rb3_cymbal_blue_logic(0, &undo_made);	//Toggle normal blue cymbal status
}

int eof_menu_note_remove_cymbal(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int undo_made = 0;
	unsigned long flags, note;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		note = eof_get_note_note(eof_song, eof_selected_track, i);
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		if(	!((note & 4) && (flags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL)) &&
			!((note & 8) && (flags & EOF_DRUM_NOTE_FLAG_B_CYMBAL)) &&
			!((note & 16) && (flags & EOF_DRUM_NOTE_FLAG_G_CYMBAL)))
			continue;	//If this note doesn't have cymbal notation, skip it

		if(!undo_made)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_CYMBAL,0,0);	//Clear the yellow cymbal flag on all drum notes at this position
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_B_CYMBAL,0,0);	//Clear the blue cymbal flag on all drum notes at this position
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_G_CYMBAL,0,0);	//Clear the green cymbal flag on all drum notes at this position
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN,0,0);	//Clear the open hi hat cymbal flag on all drum notes at this position
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL,0,0);	//Clear the pedal hi hat cymbal flag on all drum notes at this position
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_SIZZLE,0,0);		//Clear the sizzle hi hat cymbal flag on all drum notes at this position
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_COMBO,0,0);		//Clear the yellow tom/cymbal combo flag on all drum notes at this position
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_B_COMBO,0,0);		//Clear the blue tom/cymbal combo flag on all drum notes at this position
		eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_G_COMBO,0,0);		//Clear the green tom/cymbal combo flag on all drum notes at this position
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_default_cymbal(void)
{
	if(eof_mark_drums_as_cymbal)
	{
		eof_mark_drums_as_cymbal = 0;
		eof_note_drum_menu[5].flags = 0;
	}
	else
	{
		eof_mark_drums_as_cymbal = 1;
		eof_note_drum_menu[5].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_note_default_double_bass(void)
{
	if(eof_mark_drums_as_double_bass)
	{
		eof_mark_drums_as_double_bass = 0;
		eof_note_drum_menu[8].flags = 0;
	}
	else
	{
		eof_mark_drums_as_double_bass = 1;
		eof_note_drum_menu[8].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_note_default_open_hi_hat(void)
{
	eof_note_drum_hi_hat_menu[0].flags = D_SELECTED;
	eof_note_drum_hi_hat_menu[1].flags = 0;
	eof_note_drum_hi_hat_menu[2].flags = 0;
	eof_note_drum_hi_hat_menu[3].flags = 0;
	eof_mark_drums_as_hi_hat = EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;
	return 1;
}

int eof_menu_note_default_pedal_hi_hat(void)
{
	eof_note_drum_hi_hat_menu[0].flags = 0;
	eof_note_drum_hi_hat_menu[1].flags = D_SELECTED;
	eof_note_drum_hi_hat_menu[2].flags = 0;
	eof_note_drum_hi_hat_menu[3].flags = 0;
	eof_mark_drums_as_hi_hat = EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;
	return 1;
}

int eof_menu_note_default_sizzle_hi_hat(void)
{
	eof_note_drum_hi_hat_menu[0].flags = 0;
	eof_note_drum_hi_hat_menu[1].flags = 0;
	eof_note_drum_hi_hat_menu[2].flags = D_SELECTED;
	eof_note_drum_hi_hat_menu[3].flags = 0;
	eof_mark_drums_as_hi_hat = EOF_DRUM_NOTE_FLAG_Y_SIZZLE;
	return 1;
}

int eof_menu_note_default_no_hi_hat(void)
{
	eof_note_drum_hi_hat_menu[0].flags = 0;
	eof_note_drum_hi_hat_menu[1].flags = 0;
	eof_note_drum_hi_hat_menu[2].flags = 0;
	eof_note_drum_hi_hat_menu[3].flags = D_SELECTED;
	eof_mark_drums_as_hi_hat = 0;
	return 1;
}

/* split a lyric into multiple pieces (look for ' ' characters) */
static void eof_split_lyric(unsigned long lyric)
{
	unsigned long i, l, c = 0, lastc;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long piece = 1, pieces = 1;
	char * token = NULL;
	size_t stringlen;

	if(!eof_vocals_selected)
		return;
	if(lyric >= eof_song->vocal_track[tracknum]->lyrics)
		return;	//Invalid parameter
	stringlen = strlen(eof_song->vocal_track[tracknum]->lyric[lyric]->text);

	/* see how many pieces there are */
	for(i = 0; i < (unsigned long) stringlen; i++)
	{
		lastc = c;
		c = eof_song->vocal_track[tracknum]->lyric[lyric]->text[i];
		if((c == ' ') && (lastc != ' '))
		{
			if((i + 1 < (unsigned long)strlen(eof_song->vocal_track[tracknum]->lyric[lyric]->text)) && (eof_song->vocal_track[tracknum]->lyric[lyric]->text[i+1] != ' '))
			{	//Only count this as a piece if there is a non space character after this space
				pieces++;
			}
		}
	}

	/* shorten the original note */
	l = eof_song->vocal_track[tracknum]->lyric[lyric]->length;
	if(l < pieces * 20)
	{	//Ensure that the original lyric is long enough to encompass a gap of 20ms between each split (pieces - 1) * 20 plus a minimum duration of 1ms each (pieces)
		return;	//Cancel the operation if it's not long enough
	}
	eof_song->vocal_track[tracknum]->lyric[lyric]->length = l / pieces - 20;
	if(eof_song->vocal_track[tracknum]->lyric[lyric]->length < 1)
	{	//Enforce a minimum lyric duration of 1
		eof_song->vocal_track[tracknum]->lyric[lyric]->length = 1;
	}

	/* split at spaces */
	(void) strtok(eof_song->vocal_track[tracknum]->lyric[lyric]->text, " ");
	do
	{
		token = strtok(NULL, " ");
		if(token)
		{
			(void) eof_track_add_create_note(eof_song, eof_selected_track, eof_song->vocal_track[tracknum]->lyric[lyric]->note, eof_song->vocal_track[tracknum]->lyric[lyric]->pos + ((double)l / pieces) * piece, (double)l / pieces - 20.0, 0, token);
			piece++;
		}
	} while(token != NULL);
	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
}

int eof_menu_split_lyric(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(!eof_vocals_selected)
		return 1;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if(eof_count_selected_and_unselected_notes(NULL) != 1)
	{
		return 1;
	}

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_split_lyric_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_split_lyric_dialog);
	(void) ustrcpy(eof_etext, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text);
	if(eof_popup_dialog(eof_split_lyric_dialog, 2) == 3)
	{
		if(strchr(eof_etext, ' ') && eof_check_string(eof_etext))
		{	//If the string contains at least one space character and one non space character
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			(void) ustrcpy(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext);
			eof_split_lyric(eof_selection.current);
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return D_O_K;
}

int eof_menu_solo_mark(void)
{
	return eof_menu_section_mark(EOF_SOLO_SECTION);
}

int eof_menu_solo_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *soloptr = NULL;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			for(j = 0; j < eof_get_num_solos(eof_song, eof_selected_track); j++)
			{	//For each solo section in the track
				soloptr = eof_get_solo(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= soloptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= soloptr->end_pos))
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_track_delete_solo(eof_song, eof_selected_track, j);
					break;
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;
}

int eof_menu_solo_erase_all(void)
{
	eof_clear_input();
	if(eof_get_num_solos(eof_song, eof_selected_track) && (alert(NULL, "Erase all solos from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1))
	{	//If the active track has solos and the user opts to delete them
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_solos(eof_song, eof_selected_track, 0);
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	return 1;
}

int eof_menu_solo_edit_timing(void)
{
	EOF_PHRASE_SECTION *phraseptr;
	int note_selection_updated;
	unsigned long start, end;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account)

	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
	{	//If a note is selected
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_SOLO_SECTION, eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));
		if(phraseptr)
		{	//If the selected note is within a solo phrase
			if(note_selection_updated)
			{	//If notes were selected based on start/end points, use these for the section timings
				start = eof_song->tags->start_point;
				end = eof_song->tags->end_point;
			}
			else
			{
				start = phraseptr->start_pos;
				end = phraseptr->end_pos;
			}
			snprintf(eof_etext3, sizeof(eof_etext3) - 1, "Edit solo");	//Set the title of the dialog
			return eof_phrase_edit_timing(&phraseptr->start_pos, &phraseptr->end_pos, start, end);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_star_power_mark(void)
{
	return eof_menu_section_mark(EOF_SP_SECTION);
}

int eof_menu_star_power_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *starpowerptr = NULL;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			for(j = 0; j < eof_get_num_star_power_paths(eof_song, eof_selected_track); j++)
			{	//For each star power path in the track
				starpowerptr = eof_get_star_power_path(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= starpowerptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= starpowerptr->end_pos))
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_track_delete_star_power_path(eof_song, eof_selected_track, j);
					break;
				}
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_star_power_erase_all(void)
{
	eof_clear_input();
	if(eof_get_num_star_power_paths(eof_song, eof_selected_track) && (alert(NULL, "Erase all star power from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1))
	{	//If the active track has star power sections and the user opts to delete them
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_star_power_paths(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;
}

int eof_menu_sp_edit_timing(void)
{
	EOF_PHRASE_SECTION *phraseptr;
	int note_selection_updated;
	unsigned long start, end;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account)

	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
	{	//If a note is selected
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_SP_SECTION, eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));
		if(phraseptr)
		{	//If the selected note is within a star power phrase
			if(note_selection_updated)
			{	//If notes were selected based on start/end points, use these for the section timings
				start = eof_song->tags->start_point;
				end = eof_song->tags->end_point;
			}
			else
			{
				start = phraseptr->start_pos;
				end = phraseptr->end_pos;
			}
			snprintf(eof_etext3, sizeof(eof_etext3) - 1, "Edit star power");	//Set the title of the dialog
			return eof_phrase_edit_timing(&phraseptr->start_pos, &phraseptr->end_pos, start, end);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_lyric_line_mark(void)
{
	int retval = eof_menu_section_mark(EOF_LYRIC_PHRASE_SECTION);

	eof_reset_lyric_preview_lines();

	///DEBUG
	eof_check_and_log_lyric_line_errors(eof_song, 0);

	return retval;
}

int eof_menu_lyric_line_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(!eof_vocals_selected)
		return 1;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{
			for(j = 0; j < eof_song->vocal_track[tracknum]->lines; j++)
			{
				if((eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_song->vocal_track[tracknum]->line[j].start_pos) && (eof_song->vocal_track[tracknum]->lyric[i]->pos <= eof_song->vocal_track[tracknum]->line[j].end_pos))
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_vocal_track_delete_line(eof_song->vocal_track[tracknum], j);
					break;
				}
			}
		}
	}
	eof_reset_lyric_preview_lines();
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_lyric_line_erase_all(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_vocals_selected)
		return 1;

	eof_clear_input();
	if(alert(NULL, "Erase all lyric lines?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->vocal_track[tracknum]->lines = 0;
		eof_reset_lyric_preview_lines();
	}
	return 1;
}

int eof_menu_note_lyric_line_edit_timing(void)
{
	EOF_PHRASE_SECTION *phraseptr;
	int note_selection_updated;
	unsigned long start, end;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account)

	if((eof_selection.track == EOF_TRACK_VOCALS) && (eof_selection.current < eof_get_track_size(eof_song, eof_selected_track)))
	{	//If a lyric is selected
		phraseptr = eof_get_section_instance_at_pos(eof_song, EOF_TRACK_VOCALS, EOF_LYRIC_PHRASE_SECTION, eof_get_note_pos(eof_song, EOF_TRACK_VOCALS, eof_selection.current));
		if(phraseptr)
		{	//If the selected lyric is within a lyric line
			if(note_selection_updated)
			{	//If notes were selected based on start/end points, use these for the section timings
				start = eof_song->tags->start_point;
				end = eof_song->tags->end_point;
			}
			else
			{
				start = phraseptr->start_pos;
				end = phraseptr->end_pos;
			}
			snprintf(eof_etext3, sizeof(eof_etext3) - 1, "Edit lyric line");	//Set the title of the dialog
			return eof_phrase_edit_timing(&phraseptr->start_pos, &phraseptr->end_pos, start, end);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_lyric_line_toggle_overdrive(void)
{
	char used[1024] = {0};
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(!eof_vocals_selected)
		return 1;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{
			for(j = 0; j < eof_song->vocal_track[tracknum]->lines; j++)
			{
				if((eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_song->vocal_track[tracknum]->line[j].start_pos) && (eof_song->vocal_track[tracknum]->lyric[i]->pos <= eof_song->vocal_track[tracknum]->line[j].end_pos && !used[j]))
				{
					eof_song->vocal_track[tracknum]->line[j].flags ^= EOF_LYRIC_LINE_FLAG_OVERDRIVE;
					used[j] = 1;
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_hopo_auto(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;
	int note_selection_updated;

	if((eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when a drum track or the vocal track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		oldflags = flags;					//Save another copy of the original flags
		flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO on flag
		flags &= (~EOF_NOTE_FLAG_NO_HOPO);	//Clear the HOPO off flag
		if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		{	//If this is a pro guitar note, ensure that various statuses are cleared (they would be invalid if this note was a HO/PO)
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
		}
		if(!undo_made && (flags != oldflags))
		{	//If an undo state hasn't been made yet
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
			undo_made = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_hopo_cycle(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	unsigned long track_behavior = eof_song->track[eof_selected_track]->track_behavior;
	int note_selection_updated;

	if((track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a guitar track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if((eof_count_selected_and_unselected_notes(NULL) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
				continue;	//If the note isn't selected, skip it

			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_NOTE_FLAG_F_HOPO)
			{	//If the note was a forced on HOPO, make it a forced off HOPO
				flags &= ~EOF_NOTE_FLAG_F_HOPO;	//Turn off forced on hopo
				flags |= EOF_NOTE_FLAG_NO_HOPO;	//Turn on forced off hopo
				if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
				{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
				}
			}
			else if(flags & EOF_NOTE_FLAG_NO_HOPO)
			{	//If the note was a forced off HOPO, make it an auto HOPO
				flags &= ~EOF_NOTE_FLAG_F_HOPO;		//Clear the forced on hopo flag
				flags &= ~EOF_NOTE_FLAG_NO_HOPO;	//Clear the forced off hopo flag
				if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
				{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
				}
			}
			else
			{	//If the note was an auto HOPO, make it a forced on HOPO
				flags |= EOF_NOTE_FLAG_F_HOPO;	//Turn on forced on hopo
				if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
				{	//If this is a pro guitar note
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Set the hammer on flag (default HOPO type)
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
				}
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_hopo_force_on(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;
	int note_selection_updated;

	if((eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when a drum track or the vocal track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32))
			continue;	//If the note is an open bass strum note, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		oldflags = flags;					//Save another copy of the original flags
		flags |= EOF_NOTE_FLAG_F_HOPO;		//Set the HOPO on flag
		flags &= (~EOF_NOTE_FLAG_NO_HOPO);	//Clear the HOPO off flag
		if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		{	//If this is a pro guitar note
			flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Set the hammer on flag (default HOPO type)
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
		}
		if(!undo_made && (flags != oldflags))
		{	//If an undo state hasn't been made yet
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
			undo_made = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_hopo_force_off(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;
	int note_selection_updated;

	if((eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when a drum track or the vocal track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		oldflags = flags;					//Save another copy of the original flags
		flags |= EOF_NOTE_FLAG_NO_HOPO;		//Set the HOPO off flag
		flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO on flag
		if(eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		{	//If this is a pro guitar note, ensure that Hammer on, Pull off, Tap and bend statuses are cleared
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
		}
		if(!undo_made && (flags != oldflags))
		{	//If an undo state hasn't been made yet
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
			undo_made = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_flag(unsigned char function, unsigned char track_format, unsigned long bitmask)
{
	unsigned long i, flags;
	int note_selection_updated, undo_made = 0;

	if((track_format != EOF_ANY_TRACK_FORMAT) && (eof_song->track[eof_selected_track]->track_format != track_format))
		return 1;	//Do not allow this function to run unless the active track isn't of an allowed type for this function

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If this note isn't selected, skip it

		if(!undo_made)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		if(!function)
		{	//Alter the note's normal flags
			eof_xor_note_flags(eof_song, eof_selected_track, i, bitmask);
		}
		else
		{	//Alter the note's extended flags
			flags = eof_get_note_eflags(eof_song, eof_selected_track, i);
			flags ^= bitmask;	//Toggle the specified bits
			eof_set_note_eflags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_clear_flag(unsigned char function, unsigned char track_format, unsigned long bitmask)
{
	unsigned long i, flags;
	int note_selection_updated, undo_made = 0;

	if((track_format != EOF_ANY_TRACK_FORMAT) && (eof_song->track[eof_selected_track]->track_format != track_format))
		return 1;	//Do not allow this function to run unless the active track isn't of an allowed type for this function

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If this note isn't selected, skip it

		if(!function)
		{	//Alter the note's normal flags
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		}
		else
		{	//Alter the note's extended flags
			flags = eof_get_note_eflags(eof_song, eof_selected_track, i);
		}

		if(!(flags & bitmask))
			continue;	//If this note doesn't have the flag set, skip it

		//Otherwise it needs to be cleared
		if(!undo_made)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		flags &= ~bitmask;	//Clear the specified bits
		if(!function)
		{	//Alter the note's normal flags
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
		else
		{	//Alter the note's extended flags
			eof_set_note_eflags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_transpose_possible(int dir)
{
	unsigned long i, note, tracknum;
	unsigned long max = 16;	//This represents the highest note bitmask value that will be allowed to transpose up, based on the current track options (including open bass strumming)
	int retval = 1;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	/* no notes, no transpose */
	if(eof_vocals_selected)
	{
		if(eof_get_track_size(eof_song, eof_selected_track) == 0)
		{
			retval = 0;
		}
		else if(eof_count_selected_and_unselected_notes(NULL) == 0)
		{
			retval = 0;
		}
		else
		{
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//Test if the lyric can transpose the given amount in the given direction
				if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
				{
					if((eof_get_note_note(eof_song, eof_selected_track, i) == 0) || (eof_get_note_note(eof_song, eof_selected_track, i) == EOF_LYRIC_PERCUSSION))
					{	//Cannot transpose a pitchless lyric or a vocal percussion note
						retval = 0;
						break;
					}
					else if(eof_get_note_note(eof_song, eof_selected_track, i) - dir < EOF_LYRIC_PITCH_MIN)
					{
						retval = 0;
						break;
					}
					else if(eof_get_note_note(eof_song, eof_selected_track, i) - dir > EOF_LYRIC_PITCH_MAX)
					{
						retval = 0;
						break;
					}
				}
			}
		}
	}
	else
	{	//Lyrics are not selected
		if(eof_lane_six_enabled(eof_selected_track))
		{	//If lane 6 is enabled for use in the active track
			max = 32;
		}
		if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		{	//Special case:  Legacy view is in effect, revert to lane 4 being the highest that can transpose up
			max = 16;
		}
		if(eof_count_track_lanes(eof_song, eof_selected_track) == 4)
		{	//If the active track has 4 lanes (ie. PART DANCE), make lane 3 the highest that can transpose up
			max = 8;
		}
		if(eof_get_track_size(eof_song, eof_selected_track) == 0)
		{
			retval = 0;
		}
		else if(eof_count_selected_and_unselected_notes(NULL) == 0)
		{
			retval = 0;
		}
		else
		{
			tracknum = eof_song->track[eof_selected_track]->tracknum;
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the active track
				if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
					continue;	//If the note isn't selected or in the active track difficulty, skip it

				if(eof_legacy_view && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
				{	//If legacy view is in effect, get the note's legacy bitmask
					note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
				}
				else
				{	//Otherwise get the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
				}
				if(note == 0)
				{	//Special case: In legacy view, a note's legacy bitmask is undefined
					retval = 0;	//Disallow tranposing for this selection of notes
					break;
				}
				else if((note & 1) && (dir > 0))
				{
					retval = 0;
					break;
				}
				else if((note & max) && (dir < 0))
				{
					retval = 0;
					break;
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return retval;
}

int eof_note_can_pitch_transpose(EOF_SONG *sp, unsigned long track, unsigned long notenum, int dir)
{
	unsigned char note, pitchmask, pitches[6] = {0}, stringpitch;
	unsigned long stringctr, bitmask, targetstring;
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long max;	//This represents the highest note bitmask value that the track can allow, the point at which the note cannot transpose up, based on the number of strings used in this track

	if(dir > 1)
		dir = 1;	//Bounds check the dir parameter
	else if(dir < -1)
		dir = -1;
	else if(!dir)
		return 0;	//A direction of 0 effectively means do nothing

	if(!sp || !track || (track >= sp->tracks) || (!eof_track_is_pro_guitar_track(sp, track)) || eof_legacy_view || eof_menu_track_get_tech_view_state(sp, track))
		return 0;	//Only pro guitar tracks when legacy and tech views are disabled can use pitched transpose

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];	//Simplify
	if(tp->numstrings < 4)
		return 0;	//A string count less than 4 would be unexpected
	max = 1 << (tp->numstrings - 1);
	note = eof_get_note_note(sp, track, notenum);

	//Check whether the note bitmask can transpose
	if((note & 1) && (dir > 0))
		return 0;	//Can't transpose down if this note has a gem on the lowest lane
	else if((note & max) && (dir < 0))
		return 0;	//Can't transpose up if this note has a gem on the highest lane

	//Check whether each pitch played in the note can transpose
	pitchmask = eof_get_midi_pitches(sp, track, notenum, pitches);
	for(stringctr = 0, bitmask = 1; stringctr < tp->numstrings; stringctr++, bitmask <<= 1)
	{	//For each string in this track
		if(pitchmask & bitmask)
		{	//If this string has a pitch to transpose
			if(!stringctr && (dir > 0))
				continue;	//Redundant bounds check to satisfy Coverity
			targetstring = stringctr - dir;		//Determine the string the pitch is tranposing to
			stringpitch =  tp->tuning[targetstring] + eof_lookup_default_string_tuning_absolute(tp, track, targetstring) + tp->capo;	//Determine the pitch of the string the note pitch will transpose to

			if(pitches[stringctr] < stringpitch)
				return 0;	//The note's pitch is too low to be played on this string

			if(pitches[stringctr] > stringpitch + tp->numfrets)
				return 0;	//The note's pitch is too high to be played on this string
		}
	}

	return 1;
}

int eof_pitched_transpose_possible(int dir, char option)
{
	unsigned long notectr;
	int retval = 0;	//The return value cannot become true unless at least one note is able to transpose
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	EOF_PRO_GUITAR_TRACK *tp;
	char success;	//Tracks the transpose-ability of the individual note being examined

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (!eof_track_is_pro_guitar_track(eof_song, eof_selected_track)) || eof_legacy_view || eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
		return 0;	//Don't run unless a pro guitar track is active, legacy view is not in effect and tech view is not in effect

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];	//Simplify
	if(tp->numstrings < 4)
		return 0;	//A string count less than 4 would be unexpected

	if(dir > 1)
		dir = 1;	//Bounds check the dir parameter
	else if(dir < -1)
		dir = -1;
	else if(!dir)
		return 0;	//A direction of 0 effectively means do nothing

	/* no selected notes, no transpose */
	if(eof_get_track_size(eof_song, eof_selected_track) == 0)
	{
		retval = 0;
	}
	else if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		retval = 0;
	}
	else
	{
		for(notectr = 0; notectr < eof_get_track_size(eof_song, eof_selected_track); notectr++)
		{	//For each note in the active track
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[notectr] || (eof_get_note_type(eof_song, eof_selected_track, notectr) != eof_note_type))
				continue;	//If the note isn't selected or in the active track difficulty, skip it

			success = eof_note_can_pitch_transpose(eof_song, eof_selected_track, notectr, dir);	//Determine whether this note can be transposed

			if(!option && !success)
			{	//If the option parameter indicates all selected notes must be able to transpose, and this note cannot
				retval = 0;	//This function will return false
				break;
			}
			if(option && success)
			{	//If the option parameter indicates any of the selected notes must be able to transpose, and this note can
				retval = 1;	//This function will return true
				break;
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return retval;
}

int eof_new_lyric_dialog(void)
{
	EOF_LYRIC * new_lyric = NULL;
	int ret=0;

	if(!eof_vocals_selected)
		return D_O_K;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_lyric_dialog);
	(void) ustrcpy(eof_etext, "");

	if(eof_pen_lyric.note != EOF_LYRIC_PERCUSSION)		//If not entering a percussion note
	{
		if(!eof_dont_auto_edit_new_lyrics)
		{	//Unless the user has opted to not have this dialog launch automatically when a lyric is placed
			ret = eof_popup_dialog(eof_lyric_dialog, 2);	//prompt for lyric text
			if(!eof_check_string(eof_etext) && !eof_pen_lyric.note)	//If the placed lyric is both pitchless AND textless
				return D_O_K;	//Return without adding
		}
	}

	if((ret == 3) || (eof_pen_lyric.note == EOF_LYRIC_PERCUSSION) || eof_dont_auto_edit_new_lyrics)
	{	//If the user clicked OK on the edit lyric prompt, a vocal percussion note is being added or the user suppressed the prompt to add lyric text
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_lyric.note, eof_pen_lyric.pos, eof_pen_lyric.length, 0, NULL);
		(void) ustrcpy(new_lyric->text, eof_etext);
		eof_selection.track = EOF_TRACK_VOCALS;
		eof_selection.current_pos = new_lyric->pos;
		eof_selection.range_pos_1 = eof_selection.current_pos;
		eof_selection.range_pos_2 = eof_selection.current_pos;
		memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
		eof_track_sort_notes(eof_song, eof_selected_track);
		eof_track_fixup_notes(eof_song, eof_selected_track, 0);
		eof_enforce_lyric_gap_multiplier(eof_song, eof_selected_track, eof_selection.current);		//Enforce the variable note gap on the lyric before the new lyric, if appropriate
		eof_enforce_lyric_gap_multiplier(eof_song, eof_selected_track, eof_selection.current + 1);	//Enforce the variable note gap on the new lyric (if placd too close before another lyric), if appropriate
		(void) eof_detect_difficulties(eof_song, eof_selected_track);
		eof_reset_lyric_preview_lines();

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAdded lyric:  %s", (eof_pen_lyric.note == EOF_LYRIC_PERCUSSION) ? "(percussion)" : eof_etext);
		eof_log(eof_log_string, 1);
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_edit_lyric_dialog(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(!eof_vocals_selected)
		return 1;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if(eof_count_selected_and_unselected_notes(NULL) != 1)
		return 1;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_lyric_dialog);
	(void) ustrcpy(eof_etext, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text);
	if(eof_popup_dialog(eof_lyric_dialog, 2) == 3)	//User hit OK on "Edit Lyric" dialog instead of canceling
	{
		if(eof_is_freestyle(eof_etext))		//If the text entered had one or more freestyle characters
			eof_set_freestyle(eof_etext,1);	//Perform any necessary corrections

		if(ustrcmp(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext))	//If the updated string (eof_etext) is different
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			if(!eof_check_string(eof_etext))
			{	//If the updated string is empty or just whitespace
				eof_track_delete_note(eof_song, eof_selected_track, eof_selection.current);
			}
			else
			{
				(void) ustrcpy(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext);
				eof_fix_lyric(eof_song->vocal_track[tracknum],eof_selection.current);	//Correct the freestyle character if necessary
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return D_O_K;
}

int eof_menu_set_freestyle(char status)
{
	unsigned long i=0,ctr=0;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(!eof_vocals_selected)
		return 1;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
//Determine if any lyrics will actually be affected by this action
	if(eof_vocals_selected && (eof_selection.track == EOF_TRACK_VOCALS))
	{	//If lyrics are selected
		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{	//For each lyric...
			if(eof_selection.multi[i])
			{	//...that is selected, count the number of lyrics that would be altered
				if(eof_lyric_is_freestyle(eof_song->vocal_track[tracknum],i) && (status == 0))
					ctr++;	//Increment if a lyric would change from freestyle to non freestyle
				else if(!eof_lyric_is_freestyle(eof_song->vocal_track[tracknum],i) && (status != 0))
					ctr++;	//Increment if a lyric would change from non freestyle to freestyle
			}
		}

//If so, make an undo state and perform the action on the lyrics
		if(ctr)
		{	//If at least one lyric is going to be modified
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state

			for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
			{	//For each lyric...
				if(eof_selection.multi[i])
				{	//...that is selected, apply the specified freestyle status
					eof_set_freestyle(eof_song->vocal_track[tracknum]->lyric[i]->text,status);
				}
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_set_freestyle_on(void)
{
	return eof_menu_set_freestyle(1);
}

int eof_menu_set_freestyle_off(void)
{
	return eof_menu_set_freestyle(0);
}

int eof_menu_toggle_freestyle(void)
{
	unsigned long i=0;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated;

	if(!eof_vocals_selected)
		return 1;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{	//For each lyric...
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//...that is selected, toggle its freestyle status
			eof_toggle_freestyle(eof_song->vocal_track[tracknum],i);
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_lyric_remove_pitch(void)
{
	unsigned long i;
	long u = 0;
	int note_selection_updated;

	if(!eof_vocals_selected)
		return 1;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each lyric in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this lyric is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i))
			{	//If the lyric has a pitch defined
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_note_note(eof_song, eof_selected_track, i, 0);	//Set an undefined pitch
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

char eof_note_edit_name[EOF_NAME_LENGTH+1] = {0};

DIALOG eof_pro_guitar_note_dialog[] =
{
	/*	(proc)             (x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          (dp2)          (dp3) */
	{ eof_window_proc,    0,   48,  230, 492,2,   23,  0,    0,      0,         0,   "Edit pro guitar note",NULL, NULL },
	{ d_agup_text_proc,      16,  80,  64,  8,  2,   23,  0,    0,      0,         0,   "Name:",      NULL,          NULL },
	{ eof_focus_controlled_edit_proc, 74,  76,  134, 20, 2,   23,  0,    0, EOF_NAME_LENGTH,0,eof_note_edit_name,       NULL, NULL },

	//Note:  In guitar theory, string 1 refers to high e
	{ d_agup_text_proc,      16,  128, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_6_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  124, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_6,  "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  152, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_5_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  148, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_5,  "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  176, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_4_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  172, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_4,  "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  200, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_3_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  196, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_3,  "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  224, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_2_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  220, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_2,  "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  248, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_1_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  244, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_1,  "0123456789Xx",NULL },

	{ d_agup_text_proc,      16,  108, 64,  8,  2,   23,  0,    0,      0,         0,   "Fret #",     NULL,          NULL },
	{ d_agup_text_proc,      176, 127, 64,  8,  2,   23,  0,    0,      0,         0,   "Legacy",     NULL,          NULL },
	{ d_agup_check_proc,		158, 151, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 5",     NULL,          NULL },
	{ d_agup_check_proc,		158, 175, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 4",     NULL,          NULL },
	{ d_agup_check_proc,		158, 199, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 3",     NULL,          NULL },
	{ d_agup_check_proc,		158, 223, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 2",     NULL,          NULL },
	{ d_agup_check_proc,		158, 247, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 1",     NULL,          NULL },

	{ d_agup_text_proc,      102, 108, 64,  8,  2,   23,  0,    0,      0,         0,   "Ghost",      NULL,          NULL },
	{ d_agup_check_proc,		104, 127, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		104, 151, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		104, 175, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		104, 199, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		104, 223, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		104, 247, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },

	{ d_agup_text_proc,      140, 108, 64,  8,  2,   23,  0,    0,      0,         0,   "Mute",       NULL,          NULL },
	{ d_agup_check_proc,		140, 127, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		140, 151, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		140, 175, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		140, 199, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		140, 223, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,		140, 247, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },

	{ d_agup_text_proc,      10,  292, 64,  8,  2,   23,  0,    0,      0,         0,   "Slide:",     NULL,          NULL },
	{ d_agup_check_proc,		85,  312, 138, 16, 2,   23,  0,    0,      0,         0,   "Reverse slide (RB3)",NULL,  NULL },
	{ d_agup_text_proc,      10,  332, 64,  8,  2,   23,  0,    0,      0,         0,   "Mute:",      NULL,          NULL },
	{ d_agup_text_proc,      10,  352, 64,  8,  2,   23,  0,    0,      0,         0,   "Strum:",     NULL,          NULL },
	{ d_agup_radio_proc,		6,   272, 38,  16, 2,   23,  0,    0,      1,         0,   "HO",         NULL,          NULL },
	{ d_agup_radio_proc,		58,  272, 38,  16, 2,   23,  0,    0,      1,         0,   "PO",         NULL,          NULL },
	{ d_agup_radio_proc,		102, 272, 45,  16, 2,   23,  0,    0,      1,         0,   "Tap",        NULL,          NULL },
	{ d_agup_radio_proc,		154, 272, 50,  16, 2,   23,  0,    0,      1,         0,   "None",       NULL,          NULL },
	{ d_agup_radio_proc,		58,  292, 38,  16, 2,   23,  0,    0,      2,         0,   "Up",         NULL,          NULL },
	{ d_agup_radio_proc,		102, 292, 54,  16, 2,   23,  0,    0,      2,         0,   "Down",       NULL,          NULL },
	{ d_agup_radio_proc,		154, 292, 64,  16, 2,   23,  0,    0,      2,         0,   "Neither",    NULL,          NULL },
	{ d_agup_radio_proc,		46,  332, 58,  16, 2,   23,  0,    0,      3,         0,   "String",     NULL,          NULL },
	{ d_agup_radio_proc,		102, 332, 52,  16, 2,   23,  0,    0,      3,         0,   "Palm",       NULL,          NULL },
	{ d_agup_radio_proc,		154, 332, 64,  16, 2,   23,  0,    0,      3,         0,   "Neither",    NULL,          NULL },
	{ d_agup_radio_proc,		49,  352, 38,  16, 2,   23,  0,    0,      4,         0,   "Up",         NULL,          NULL },
	{ d_agup_radio_proc,		85,  352, 46,  16, 2,   23,  0,    0,      4,         0,   "Mid",        NULL,          NULL },
	{ d_agup_radio_proc,		128, 352, 54,  16, 2,   23,  0,    0,      4,         0,   "Down",       NULL,          NULL },
	{ d_agup_radio_proc,		180, 352, 46,  16, 2,   23,  0,    0,      4,         0,   "Any",        NULL,          NULL },

	{ d_agup_button_proc,    10,  500, 20,  28, 2,   23,  0,    D_EXIT, 0,         0,   "<-",         NULL,          NULL },
	{ d_agup_button_proc,    35,  500, 50,  28, 2,   23,  '\r', D_EXIT, 0,         0,   "OK",         NULL,          NULL },
	{ d_agup_button_proc,    90,  500, 50,  28, 2,   23,  0,    D_EXIT, 0,         0,   "Apply",      NULL,          NULL },
	{ d_agup_button_proc,    145, 500, 50,  28, 2,   23,  0,    D_EXIT, 0,         0,   "Cancel",     NULL,          NULL },
	{ d_agup_button_proc,    200, 500, 20,  28, 2,   23,  0,    D_EXIT, 0,         0,   "->",         NULL,          NULL },

	{ d_agup_text_proc,      10,  372, 64,  8,  2,   23,  0,    0,      0,         0,   "Bass:",      NULL,          NULL },
	{ d_agup_radio_proc,		46,  372, 58,  16, 2,   23,  0,    0,      5,         0,   "Pop",        NULL,          NULL },
	{ d_agup_radio_proc,		102, 372, 52,  16, 2,   23,  0,    0,      5,         0,   "Slap",       NULL,          NULL },
	{ d_agup_radio_proc,		154, 372, 64,  16, 2,   23,  0,    0,      5,         0,   "Neither",    NULL,          NULL },

	{ d_agup_check_proc,		10,  392, 64,  16, 2,   23,  0,    0,      0,         0,   "Accent",     NULL,          NULL },
	{ d_agup_check_proc,		87,  392, 64,  16, 2,   23,  0,    0,      0,         0,   "P.Harm",     NULL,          NULL },
	{ d_agup_check_proc,		154, 392, 64,  16, 2,   23,  0,    0,      0,         0,   "Vibrato",    NULL,          NULL },
	{ d_agup_check_proc,		10,  412, 76,  16, 2,   23,  0,    0,      0,         0,   "Harmonic",   NULL,          NULL },
	{ d_agup_check_proc,		87,  412, 50,  16, 2,   23,  0,    0,      0,         0,   "Bend",       NULL,          NULL },
	{ d_agup_check_proc,		154, 412, 72,  16, 2,   23,  0,    0,      0,         0,   "Linknext",   NULL,          NULL },
	{ d_agup_check_proc,		10,  432, 60,  16, 2,   23,  0,    0,      0,         0,   "Ignore",     NULL,          NULL },
	{ d_agup_check_proc,		87,  432, 65,  16, 2,   23,  0,    0,      0,         0,   "Sustain",    NULL,          NULL },
	{ d_agup_check_proc,		154, 432, 50,  16, 2,   23,  0,    0,      0,         0,   "Stop",       NULL,          NULL },
	{ d_agup_check_proc,		10,  452, 78,  16, 2,   23,  0,    0,      0,         0,   "Ghost HS",   NULL,          NULL },
	{ d_agup_check_proc,		87,  452, 68,  16, 2,   23,  0,    0,      0,         0,   "Hi Dens",    NULL,          NULL },
	{ d_agup_check_proc,		154, 452, 48,  16, 2,   23,  0,    0,      0,         0,   "Split",      NULL,          NULL },
	{ d_agup_check_proc,		10,  312, 78,  16, 2,   23,  0,    0,      0,         0,   "Chordify",   NULL,          NULL },
	{ d_agup_check_proc,		10,  472, 80,  16, 2,   23,  0,    0,      0,         0,   "Fingerless", NULL,          NULL },
	{ d_agup_check_proc,		87,  472, 78,  16, 2,   23,  0,    0,      0,         0,   "Pre-bend",   NULL,          NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_note_edit_pro_guitar_note(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long ctr, ctr2, stringcount, i;
	char undo_made = 0;	//Set to nonzero when an undo state is created
	long fretvalue, highfretvalue;
	char allmuted;					//Used to track whether all used strings are string muted
	unsigned long bitmask = 0;		//Used to build the updated pro guitar note bitmask
	unsigned char legacymask;		//Used to build the updated legacy note bitmask
	unsigned char ghostmask;		//Used to build the updated ghost bitmask
	unsigned long flags;			//Used to build the updated flag bitmask
	unsigned long eflags;			//Used to build the updated extended flag bitmask
	char *newname = NULL, *tempptr;
	char autoprompt[100] = {0};
	char previously_refused;
	char declined_list[EOF_MAX_NOTES] = {0};	//This lists all notes whose names/legacy bitmasks the user declined to have applied to the edited note
	char autobitmask[10] = {0};
	unsigned long index = 0;
	char pro_guitar_string[30] = {0};
	long previous_note = 0, next_note = 0;
	int retval;
	int note_selection_updated;
	static char dont_ask = 0;	//Is set to nonzero if the user opts to suppress the prompt regarding modifying multiple selected notes
	EOF_PRO_GUITAR_TRACK *tp;
	EOF_PRO_GUITAR_NOTE *np;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless the pro guitar track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if(eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run if a valid note isn't selected

	tp = eof_song->pro_guitar_track[tracknum];	//Simplify
	np = tp->note[eof_selection.current];	//Simplify
	highfretvalue = tp->numfrets;
	if(!eof_music_paused)
	{
		eof_music_play(0);
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pro_guitar_note_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_pro_guitar_note_dialog);

	do
	{	//Prepare the dialog
		retval = 0;
	//Update the note name text box to match the last selected note
		memcpy(eof_note_edit_name, np->name, sizeof(eof_note_edit_name));

	//Find the next/previous notes if applicable
		previous_note = eof_track_fixup_previous_note(eof_song, eof_selected_track, eof_selection.current);
		if(previous_note >= 0)
		{	//If there is a previous note
			eof_pro_guitar_note_dialog[54].flags = D_EXIT;		//Make the previous note button clickable
		}
		else
		{
			eof_pro_guitar_note_dialog[54].flags = D_HIDDEN;	//Otherwise hide it
		}
		next_note = eof_track_fixup_next_note(eof_song, eof_selected_track, eof_selection.current);
		if(next_note >= 0)
		{	//If there is a next note
			eof_pro_guitar_note_dialog[58].flags = D_EXIT;		//Make the next note button clickable
		}
		else
		{
			eof_pro_guitar_note_dialog[58].flags = D_HIDDEN;	//Otherwise hide it
		}

	//Update the fret text boxes (listed from top to bottom as string 1 through string 6)
		stringcount = eof_count_track_lanes(eof_song, eof_selected_track);
		if(eof_legacy_view)
		{	//Special case:  If legacy view is enabled, correct stringcount
			stringcount = tp->numstrings;
		}
		ghostmask = np->ghost;
		for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask<<=1)
		{	//For each of the 6 supported strings
			if(ctr < stringcount)
			{	//If this track uses this string, copy the fret value to the appropriate string
				eof_pro_guitar_note_dialog[13 - (2 * ctr)].flags = 0;	//Ensure this text boxes' label is enabled
				eof_fret_string_numbers[ctr][7] = '0' + (stringcount - ctr);	//Correct the string number for this label
				eof_pro_guitar_note_dialog[14 - (2 * ctr)].flags = 0;	//Ensure this text box is enabled
				eof_pro_guitar_note_dialog[28 - ctr].flags = (ghostmask & bitmask) ? D_SELECTED : 0;	//Ensure the ghost check box is enabled and cleared/checked appropriately
				eof_pro_guitar_note_dialog[35 - ctr].flags = 0;		//Ensure the mute check box is enabled and cleared (it will be checked below if applicable)
				if(np->note & bitmask)
				{	//If this string is already defined as being in use, copy its fret value to the string
					if(np->frets[ctr] & 0x80)
					{	//If this string is muted (most significant bit is set)
						eof_pro_guitar_note_dialog[35 - ctr].flags = D_SELECTED;	//Check the string's muted checkbox
					}
					if(np->frets[ctr] == 0xFF)
					{	//If this string is muted with no fret value specified
						(void) snprintf(eof_fret_strings[ctr], sizeof(eof_fret_strings[ctr]) - 1, "X");
					}
					else
					{
						(void) snprintf(eof_fret_strings[ctr], sizeof(eof_fret_strings[ctr]) - 1, "%d", np->frets[ctr] & 0x7F);	//Mask out the MSB to obtain the fret value
					}
				}
				else
				{	//Otherwise empty the string
					eof_fret_strings[ctr][0] = '\0';
				}
			}
			else
			{	//Otherwise disable the text box for this fret and empty the string
				eof_pro_guitar_note_dialog[13 - (2 * ctr)].flags = D_HIDDEN;	//Ensure this text boxes' label is hidden
				eof_pro_guitar_note_dialog[14 - (2 * ctr)].flags = D_HIDDEN;	//Ensure this text box is hidden
				eof_pro_guitar_note_dialog[28 - ctr].flags = D_HIDDEN;			//Ensure this ghost check box is hidden
				eof_pro_guitar_note_dialog[35 - ctr].flags = D_HIDDEN;			//Ensure this mute check box is hidden
				eof_fret_strings[ctr][0] = '\0';
			}
		}

	//Update the legacy bitmask checkboxes
		legacymask = np->legacymask;
		eof_pro_guitar_note_dialog[17].flags = (legacymask & 16) ? D_SELECTED : 0;
		eof_pro_guitar_note_dialog[18].flags = (legacymask & 8) ? D_SELECTED : 0;
		eof_pro_guitar_note_dialog[19].flags = (legacymask & 4) ? D_SELECTED : 0;
		eof_pro_guitar_note_dialog[20].flags = (legacymask & 2) ? D_SELECTED : 0;
		eof_pro_guitar_note_dialog[21].flags = (legacymask & 1) ? D_SELECTED : 0;

	//Clear the reverse slide checkbox
		eof_pro_guitar_note_dialog[37].flags = 0;

	//Update the note flag radio buttons
		for(ctr = 0; ctr < 14; ctr++)
		{	//Clear 14 of the status radio buttons
			eof_pro_guitar_note_dialog[40 + ctr].flags = 0;
		}
		flags = np->flags;
		eflags = np->eflags;
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
		{	//Select "HO"
			eof_pro_guitar_note_dialog[40].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PO)
		{	//Select "PO"
			eof_pro_guitar_note_dialog[41].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
		{	//Select "Tap"
			eof_pro_guitar_note_dialog[42].flags = D_SELECTED;
		}
		else
		{	//Select "None"
			eof_pro_guitar_note_dialog[43].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
		{	//Select Slide "Up"
			eof_pro_guitar_note_dialog[44].flags = D_SELECTED;
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)
				eof_pro_guitar_note_dialog[37].flags = D_SELECTED;	//Reverse slide
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
		{	//Select Slide "Down"
			eof_pro_guitar_note_dialog[45].flags = D_SELECTED;
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)
				eof_pro_guitar_note_dialog[37].flags = D_SELECTED;
		}
		else
		{	//Select Slide "Neither"
			eof_pro_guitar_note_dialog[46].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)
		{	//Select Mute "String"
			eof_pro_guitar_note_dialog[47].flags = D_SELECTED;
		}
		else if(flags &EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
		{	//Select Mute "Palm"
			eof_pro_guitar_note_dialog[48].flags = D_SELECTED;
		}
		else
		{	//Select Mute "Neither"
			eof_pro_guitar_note_dialog[49].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
		{	//Select Strum "Up"
			eof_pro_guitar_note_dialog[50].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM)
		{	//Select Strum "Mid"
			eof_pro_guitar_note_dialog[51].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM)
		{	//Select Strum "Down"
			eof_pro_guitar_note_dialog[52].flags = D_SELECTED;
		}
		else
		{	//Select Strum "Any"
			eof_pro_guitar_note_dialog[53].flags = D_SELECTED;
		}
		eof_pro_guitar_note_dialog[60].flags = eof_pro_guitar_note_dialog[61].flags = eof_pro_guitar_note_dialog[62].flags = 0;	//Deselect these radio buttons
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_POP)
		{	//Select "Pop"
			eof_pro_guitar_note_dialog[60].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP)
		{	//Select "Slap"
			eof_pro_guitar_note_dialog[61].flags = D_SELECTED;
		}
		else
		{	//Select "Neither"
			eof_pro_guitar_note_dialog[62].flags = D_SELECTED;
		}
		eof_pro_guitar_note_dialog[63].flags = (flags & EOF_PRO_GUITAR_NOTE_FLAG_ACCENT) ? D_SELECTED : 0;		//Update "Accent" checkbox value
		eof_pro_guitar_note_dialog[64].flags = (flags & EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC) ? D_SELECTED : 0;	//Update "P.Harm" checkbox value
		eof_pro_guitar_note_dialog[65].flags = (flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO) ? D_SELECTED : 0;		//Update "Vibrato" checkbox value
		eof_pro_guitar_note_dialog[66].flags = (flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC) ? D_SELECTED : 0;	//Update "Harmonic" checkbox value
		eof_pro_guitar_note_dialog[67].flags = (flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) ? D_SELECTED : 0;			//Update "Bend" checkbox value
		eof_pro_guitar_note_dialog[68].flags = (flags & EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT) ? D_SELECTED : 0;		//Update "Linknext" checkbox value
		eof_pro_guitar_note_dialog[69].flags = (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_IGNORE) ? D_SELECTED : 0;		//Update "Ignore" checkbox value
		eof_pro_guitar_note_dialog[70].flags = (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN) ? D_SELECTED : 0;	//Update "Sustain" checkbox value
		eof_pro_guitar_note_dialog[73].flags = (flags & EOF_PRO_GUITAR_NOTE_FLAG_HD) ? D_SELECTED : 0;			//Update "Hi Dens" checkbox value
		eof_pro_guitar_note_dialog[74].flags = (flags & EOF_PRO_GUITAR_NOTE_FLAG_SPLIT) ? D_SELECTED : 0;			//Update "Split" checkbox value
		eof_pro_guitar_note_dialog[75].flags = (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_CHORDIFY) ? D_SELECTED : 0;	//Update "Chordify" checkbox value
		eof_pro_guitar_note_dialog[76].flags = (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS) ? D_SELECTED : 0;	//Update "Fingerless" checkbox value

		//The remaining statuses depend on whether tech view is in effect
		if(!eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
		{	//If tech view isn't in effect for the current track
			eof_pro_guitar_note_dialog[71].flags = D_DISABLED;	//"Stop" status
			eof_pro_guitar_note_dialog[77].flags = D_DISABLED;	//"Pre-bend" status
			eof_pro_guitar_note_dialog[72].flags = (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_GHOST_HS) ? D_SELECTED : 0;	//Update "Ghost HS" checkbox value
		}
		else
		{	//Tech view is in effect
			eof_pro_guitar_note_dialog[37].flags = D_DISABLED;	//Reverse slide
			eof_pro_guitar_note_dialog[71].flags = (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_STOP) ? D_SELECTED : 0;	//Update "Stop" checkbox value
			eof_pro_guitar_note_dialog[72].flags = D_DISABLED;	//"Ghost HS" status
			eof_pro_guitar_note_dialog[75].flags = D_DISABLED;	//"Chordify" status
			eof_pro_guitar_note_dialog[77].flags = (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND) ? D_SELECTED : 0;	//Update "Pre-bend" checkbox value
		}

		bitmask = 0;
		retval = eof_popup_dialog(eof_pro_guitar_note_dialog, 2);	//Run the dialog, set initial focus to the name field
		if((retval == 55) || (retval == 56))
		{	//If user clicked OK or Apply
			//Validate and store the input
			if(eof_note_edit_name[0] != '\0')
			{	//If the user specified a name
				//Ensure it does not include brackets
				for(i = 0; i < (unsigned long)ustrlen(eof_note_edit_name); i++)
				{	//For each character in the name field
					if((eof_note_edit_name[i] == '[') || (eof_note_edit_name[i] == ']'))
					{	//If the character is a bracket
						allegro_message("You cannot use a bracket in the note's name");
						eof_show_mouse(NULL);
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						return 1;
					}
				}
			}

			if(eof_count_selected_and_unselected_notes(NULL) > 1)
			{	//If multiple notes are selected, warn the user
				int retval2;

				eof_clear_input();
				if(!dont_ask)
				{	//If the user didn't suppress this prompt
					retval2 = alert3(NULL, "Warning:  This information will be applied to all selected notes.", NULL, "&OK", "&Cancel", "OK, don't ask again", 'o', 'c', 0);
					if(retval2 == 2)
					{	//If user opts to cancel the operation
						if(note_selection_updated)
						{	//If the note selection was originally empty and was dynamically updated
							(void) eof_menu_edit_deselect_all();	//Clear the note selection
						}
						eof_show_mouse(NULL);
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						return 1;
					}
					if(retval2 == 3)
					{	//If this user is suppressing this prompt
						dont_ask = 1;
					}
				}
			}

			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the track
				if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
					continue;	//If the note isn't selected or in the active track difficulty, skip it

				//Save the updated note name
				if(ustrcmp(eof_note_edit_name, tp->note[i]->name))
				{	//If the name was changed
					if(!undo_made)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					memcpy(tp->note[i]->name, eof_note_edit_name, sizeof(eof_note_edit_name));
					(void) eof_rs_check_chord_name(eof_song, eof_selected_track, eof_selection.current, 0);	//Check if the user included lowercase "maj" in the name
				}

				for(ctr = 0, allmuted = 1; ctr < 6; ctr++)
				{	//For each of the 6 supported strings
					if((eof_fret_strings[ctr][0] != '\0') || (eof_pro_guitar_note_dialog[35 - ctr].flags == D_SELECTED))
					{	//If this string isn't empty, or the string's mute checkbox is set, apply the fret value
						fretvalue = 0;
						bitmask |= (1 << ctr);	//Set the appropriate bit for this lane
						for(ctr2 = 0;eof_fret_strings[ctr][ctr2] != '\0';ctr2++)
						{	//For each character in the fret string
							if(toupper(eof_fret_strings[ctr][ctr2]) == 'X')
							{	//If the character is an X
								fretvalue = 0xFF;	//Consider this string muted
								break;
							}
						}
						if(!fretvalue)
						{	//If the string didn't contain an X, convert it to an integer value
							fretvalue = atol(eof_fret_strings[ctr]);
							if((fretvalue < 0) || (fretvalue > 255))
							{	//If the conversion to number failed or specifies a fret EOF cannot store as an 8 bit number
								fretvalue = 0xFF;
							}
							else if(fretvalue > highfretvalue)
							{
								highfretvalue = fretvalue;	//Track the highest user-defined fret value
							}
						}
						if(eof_pro_guitar_note_dialog[35 - ctr].flags == D_SELECTED)
						{	//If this string's muted checkbox is checked
							if(eof_fret_strings[ctr][0] == '\0')
							{	//If the fret string is empty
								fretvalue = 0xFF;	//Mark it as muted with no fret specified
							}
							else
							{	//Otherwise just ensure the MSB bit (muted status) is checked
								fretvalue |= 0x80;
							}
						}
						if(eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
						{	//If tech view is in effect, ignore the user specified fret value except for the MSB (mute status)
							fretvalue &= 0x80;
						}
						if(fretvalue != tp->note[i]->frets[ctr])
						{	//If this fret value (or the string's muting) changed
							if(!undo_made)
							{	//If an undo state hasn't been made yet
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								undo_made = 1;
							}
							tp->note[i]->frets[ctr] = fretvalue;
							if((fretvalue & 0x7F) != (tp->note[i]->frets[ctr] & 0x7F))
							{	//If this fret value (ignoring the MSB indicating string muting) changed
								memset(tp->note[i]->finger, 0, 8);	//Initialize all fingers to undefined
							}
						}
						if((fretvalue != 0xFF) && ((fretvalue & 0x80) == 0))
						{	//Track whether all of the used strings in this note/chord are string muted
							allmuted = 0;
						}
					}
					else
					{	//Clear the fret value and return the fret back to its default of 0 (open)
						bitmask &= ~(1 << ctr);	//Clear the appropriate bit for this lane
						if(tp->note[i]->frets[ctr] != 0)
						{	//If this fret value changed
							if(!undo_made)
							{	//If an undo state hasn't been made yet
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								undo_made = 1;
							}
							tp->note[i]->frets[ctr] = 0;
						}
					}
				}//For each of the 6 supported strings
				if(bitmask == 0)
				{	//If edits results in selected notes having no played strings
					(void) eof_menu_note_delete();		//All selected notes must be deleted
					eof_show_mouse(NULL);
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					return 1;					//Return from function
				}
//Save the updated note bitmask
				if(tp->note[i]->note != bitmask)
				{	//If the note bitmask changed
					if(!undo_made)
					{	//If an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					tp->note[i]->note = bitmask;
				}

//Save the updated legacy note bitmask
				legacymask = 0;
				if(eof_pro_guitar_note_dialog[17].flags == D_SELECTED)
					legacymask |= 16;
				if(eof_pro_guitar_note_dialog[18].flags == D_SELECTED)
					legacymask |= 8;
				if(eof_pro_guitar_note_dialog[19].flags == D_SELECTED)
					legacymask |= 4;
				if(eof_pro_guitar_note_dialog[20].flags == D_SELECTED)
					legacymask |= 2;
				if(eof_pro_guitar_note_dialog[21].flags == D_SELECTED)
					legacymask |= 1;
				if(legacymask != tp->note[i]->legacymask)
				{	//If the legacy bitmask changed
					if(!undo_made)
					{	//If an undo state hasn't been made yet
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					tp->note[i]->legacymask = legacymask;
				}

//Save the updated ghost bitmask
				ghostmask = 0;
				if(eof_pro_guitar_note_dialog[23].flags == D_SELECTED)
					ghostmask |= 32;
				if(eof_pro_guitar_note_dialog[24].flags == D_SELECTED)
					ghostmask |= 16;
				if(eof_pro_guitar_note_dialog[25].flags == D_SELECTED)
					ghostmask |= 8;
				if(eof_pro_guitar_note_dialog[26].flags == D_SELECTED)
					ghostmask |= 4;
				if(eof_pro_guitar_note_dialog[27].flags == D_SELECTED)
					ghostmask |= 2;
				if(eof_pro_guitar_note_dialog[28].flags == D_SELECTED)
						ghostmask |= 1;
				ghostmask &= bitmask;	//Clear all lanes that are specified by the note bitmask as being used
				if(ghostmask != tp->note[i]->ghost)
				{	//If the ghost mask changed
					if(!undo_made)
					{	//If an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					tp->note[i]->ghost = ghostmask;
				}

//Save the updated note flag bitmask
				flags = flags & (EOF_NOTE_FLAG_HOPO | EOF_NOTE_FLAG_SP);	//Clear the flags variable, except retain the note's existing SP and HOPO flag statuses
				eflags = 0;													//Clear the extended flags variable
				if(eof_pro_guitar_note_dialog[40].flags == D_SELECTED)
				{	//HO is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Set the hammer on flag
					flags |= EOF_NOTE_FLAG_F_HOPO;					//Set the legacy HOPO flag
				}
				else if(eof_pro_guitar_note_dialog[41].flags == D_SELECTED)
				{	//PO is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Set the pull off flag
					flags |= EOF_NOTE_FLAG_F_HOPO;					//Set the legacy HOPO flag
				}
				else if(eof_pro_guitar_note_dialog[42].flags == D_SELECTED)
				{	//Tap is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Set the tap flag
					flags |= EOF_NOTE_FLAG_F_HOPO;					//Set the legacy HOPO flag
				}
				if(eof_pro_guitar_note_dialog[44].flags == D_SELECTED)
				{	//Slide Up is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
					if(eof_pro_guitar_note_dialog[37].flags == D_SELECTED)	//Reverse slide is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE;
				}
				else if(eof_pro_guitar_note_dialog[45].flags == D_SELECTED)
				{	//Slide Down is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
					if(eof_pro_guitar_note_dialog[37].flags == D_SELECTED)	//Reverse slide is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE;
				}
				if(eof_pro_guitar_note_dialog[47].flags == D_SELECTED)
				{	//Mute String is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
				}
				else if(eof_pro_guitar_note_dialog[48].flags == D_SELECTED)
				{	//Mute Palm is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
				}
				if(eof_pro_guitar_note_dialog[50].flags == D_SELECTED)
				{	//Strum Up is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
				}
				else if(eof_pro_guitar_note_dialog[51].flags == D_SELECTED)
				{	//Strum Mid is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM;
				}
				else if(eof_pro_guitar_note_dialog[52].flags == D_SELECTED)
				{	//Strum Down is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;
				}
				if(eof_pro_guitar_note_dialog[60].flags == D_SELECTED)
				{	//Pop is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_POP;
				}
				else if(eof_pro_guitar_note_dialog[61].flags == D_SELECTED)
				{	//Slap is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLAP;
				}
				if(eof_pro_guitar_note_dialog[63].flags == D_SELECTED)
				{	//Accent is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_ACCENT;
				}
				if(eof_pro_guitar_note_dialog[64].flags == D_SELECTED)
				{	//Pinch Harmonic is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC;
				}
				if(eof_pro_guitar_note_dialog[65].flags == D_SELECTED)
				{	//Vibrato is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
				}
				if(eof_pro_guitar_note_dialog[66].flags == D_SELECTED)
				{	//Harmonic is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
				}
				if(eof_pro_guitar_note_dialog[67].flags == D_SELECTED)
				{	//Bend is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
				}
				if(eof_pro_guitar_note_dialog[68].flags == D_SELECTED)
				{	//Linknext is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;
				}
				if(eof_pro_guitar_note_dialog[69].flags == D_SELECTED)
				{	//Ignore is selected
					eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_IGNORE;
				}
				if(eof_pro_guitar_note_dialog[70].flags == D_SELECTED)
				{	//Sustain is selected
					eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN;
				}
				if(eof_pro_guitar_note_dialog[71].flags == D_SELECTED)
				{	//Stop is selected
					eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_STOP;
				}
				if(eof_pro_guitar_note_dialog[72].flags == D_SELECTED)
				{	//Ghost HS is selected
					eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_GHOST_HS;
				}
				if(eof_pro_guitar_note_dialog[73].flags == D_SELECTED)
				{	//High Density is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_HD;
				}
				if(eof_pro_guitar_note_dialog[74].flags == D_SELECTED)
				{	//Split is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_SPLIT;
				}
				if(eof_pro_guitar_note_dialog[75].flags == D_SELECTED)
				{	//Chordify is selected
					eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_CHORDIFY;
				}
				if(eof_pro_guitar_note_dialog[76].flags == D_SELECTED)
				{	//Fingerless is selected
					eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS;
				}
				if(eof_pro_guitar_note_dialog[77].flags == D_SELECTED)
				{	//Pre-bend is selected, apply the normal bend status in addition to the pre-bend status
					eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND;
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
				}

				if((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
				{	//If this is a slide or bend note, retain the note's original RS notation flag so any existing bend strengths or slide positions are kept
					flags |= (tp->note[i]->flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION);
				}

				if(allmuted && !(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
				{	//If all strings are muted and the user didn't specify a palm mute
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;		//Set the string mute flag
				}
				flags |= (tp->note[i]->flags & EOF_NOTE_FLAG_CRAZY);	//Retain the note's original crazy status
				flags |= (tp->note[i]->flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE);	//Retain the note's original unpitched slide status
				if((flags != tp->note[i]->flags) || (eflags != tp->note[i]->eflags))
				{	//If the flags changed
					if(!undo_made)
					{	//If an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					tp->note[i]->flags = flags;
					tp->note[i]->eflags = eflags;
				}
			}//For each note in the track

	//Prompt whether matching notes need to have their name updated
			if(!eof_menu_note_edit_pro_guitar_note_prompt_suppress && (eof_note_edit_name[0] != '\0'))
			{	//If the user didn't suppress this prompt and has entered a note name
				int retval2;

				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the active track
					if((eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) != 0) || !ustrcmp(eof_note_edit_name, eof_get_note_name(eof_song, eof_selected_track, ctr)))
						continue;	//If this note doesn't match the one that was edited, or if the name is already the same, skip it

					eof_clear_input();
					retval2 = alert3(NULL, "Update other matching notes in this track to have the same name?", NULL, "&Yes", "&No", "No, stop asking", 'y', 'n', 0);
					if(retval2 == 1)
					{	//If the user opts to use the updated note name on matching notes in this track
						for(; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
						{	//For each note in the active track, starting from the one that just matched the comparison
							if((eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) == 0) && (eof_selection.current != ctr))
							{	//If this note matches the note that was edited, and we're not comparing the note to itself, copy the edited note's name to this note
								if(!undo_made)
								{	//If an undo state hasn't been made yet
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									undo_made = 1;
								}
								(void) ustrcpy(eof_get_note_name(eof_song, eof_selected_track, ctr), eof_note_edit_name);
							}
						}
					}
					else if(retval2 ==3)
					{	//Suppress this prompt
						eof_menu_note_edit_pro_guitar_note_prompt_suppress = 1;
					}
					break;	//Break from loop
				}//For each note in the active track
			}//If the user entered a name

	//Or prompt whether the selected notes' name should be updated from the other existing notes
			else
			{	//The user did not enter a name
				memset(declined_list, 0, sizeof(declined_list));	//Clear the declined list
				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the active track
					if((ctr == eof_selection.current) || (eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) != 0))
						continue;	//If this note is the one that was just edited, or if it doesn't match the latter, skip it
					newname = eof_get_note_name(eof_song, eof_selected_track, ctr);
					if(!newname || (newname[0] == '\0'))
						continue;	//If this note doesn't have a name, skip it

					previously_refused = 0;
					for(ctr2 = 0; ctr2 < ctr; ctr2++)
					{	//For each previous note that was checked
						if(declined_list[ctr2] && !ustrcmp(newname, eof_get_note_name(eof_song, eof_selected_track, ctr2)))
						{	//If this note name matches one the user previously rejected to assign to the edited note
							declined_list[ctr] = 1;	//Automatically decline this instance of the same name
							previously_refused = 1;
							break;
						}
					}
					if(previously_refused)
						continue;	//If this name is one the user already refused, skip it

					if(eof_get_pro_guitar_note_fret_string(tp, eof_selection.current, pro_guitar_string, 0))
					{	//If the note's frets can be represented in string format, specify it in the prompt
						(void) snprintf(autoprompt, sizeof(autoprompt) - 1, "Set the name of selected notes (%s) to \"%s\"?",pro_guitar_string, newname);
					}
					else
					{	//Otherwise use a generic prompt
						(void) snprintf(autoprompt, sizeof(autoprompt) - 1, "Set selected notes' name to \"%s\"?",newname);
					}
					eof_clear_input();
					if(alert(NULL, autoprompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If the user opts to assign this note's name to the selected notes
						for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, eof_selected_track); ctr2++)
						{	//For each note in the track
							if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr2] && (eof_get_note_type(eof_song, eof_selected_track, ctr2) == eof_note_type))
							{	//If the note is in the active instrument difficulty and is selected
								tempptr = eof_get_note_name(eof_song, eof_selected_track, ctr2);	//Get the name of the note
								if(tempptr && ustricmp(tempptr, newname))
								{	//If the note's name doesn't match the one the user selected from the prompt
									if(!undo_made)
									{
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										undo_made = 1;
									}
									(void) ustrcpy(tempptr, newname);	//Update the note's name to the user selection
								}
							}
						}
						break;	//Break from "For each note in the active track" loop
					}
					else
					{	//Otherwise mark this note's name as refused
						declined_list[ctr] = 1;	//Mark this note's name as having been declined
					}
				}//For each note in the active track
			}//The user did not enter a name

	//Prompt whether matching notes need to have their legacy bitmask updated
			if(legacymask)
			{	//If the user entered a legacy bitmask
				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the active track
					if((ctr == eof_selection.current) || (eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type) || (eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) != 0))
						continue;	//If this note is the one that was just edited, or if it isn't in the active track difficulty, or it doesn't match the note that was just edited, skip it
					if(legacymask == tp->note[ctr]->legacymask)
						continue;	//If the two notes have the same legacy bitmask, skip it

					eof_clear_input();
					if(alert(NULL, "Update other matching notes in this track difficulty to have the same legacy bitmask?", NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If the user opts to use the updated note legacy bitmask on matching notes in this track difficulty
						for(; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
						{	//For each note in the active track, starting from the one that just matched the comparison
							if((ctr != eof_selection.current) && (eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type) && (eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) == 0))
							{	//If this note isn't the one that was just edited, but it matches it and is in the active track difficulty, copy the edited note's legacy bitmask to this note
								if(!undo_made)
								{	//If an undo state hasn't been made yet
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									undo_made = 1;
								}
								tp->note[ctr]->legacymask = legacymask;
							}
						}
					}
					break;	//Break from loop
				}//For each note in the active track
			}//If the user entered a legacy bitmask

	//Or prompt whether the selected notes' legacy bitmask should be updated from the other existing notes
			else
			{	//The user did not enter a legacy bitmask
				memset(declined_list, 0, sizeof(declined_list));	//Clear the declined list
				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the active track
					if((ctr == eof_selection.current) || (eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) != 0))
						continue;	//If this note is the one that was just edited, or it doesn't match the latter, skip it

					legacymask = tp->note[ctr]->legacymask;
					if(!legacymask)
						continue;	//If this note doesn't have a legacy bitmask, skip it

					previously_refused = 0;
					for(ctr2 = 0; ctr2 < ctr; ctr2++)
					{	//For each previous note that was checked
						if(declined_list[ctr2] && (legacymask == tp->note[ctr2]->legacymask))
						{	//If this note's legacy mask matches one the user previously rejected to assign to the edited note
							declined_list[ctr] = 1;	//Automatically decline this instance of the same legacy bitmask
							previously_refused = 1;
							break;
						}
					}
					if(previously_refused)
						continue;	//If this legacy bitmask was one the user already refused, skip it

					for(ctr2 = 0, bitmask = 1, index = 0; ctr2 < 5; ctr2++, bitmask<<=1)
					{	//For each of the legacy bitmasks 5 usable bits
						if(legacymask & bitmask)
						{	//If this bit is set, append the fret number to the autobitmask string
							autobitmask[index++] = '1' + ctr2;
						}
						autobitmask[index] = '\0';	//Ensure the string is terminated
					}
					if(eof_get_pro_guitar_note_fret_string(tp, eof_selection.current, pro_guitar_string, 0))
					{	//If the note's frets can be represented in string format, specify it in the prompt
						(void) snprintf(autoprompt, sizeof(autoprompt) - 1, "Set the legacy bitmask of selected notes (%s) to \"%s\"?",pro_guitar_string, autobitmask);
					}
					else
					{	//Otherwise use a generic prompt
						(void) snprintf(autoprompt, sizeof(autoprompt) - 1, "Set selected notes' legacy bitmask to \"%s\"?",autobitmask);
					}
					eof_clear_input();
					if(alert(NULL, autoprompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If the user opts to assign this note's legacy bitmask to the edited note
						for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, eof_selected_track); ctr2++)
						{	//For each note in the track
							if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr2] && (eof_get_note_type(eof_song, eof_selected_track, ctr2) == eof_note_type))
							{	//If the note is in the active instrument difficulty and is selected
								if(legacymask != tp->note[ctr2]->legacymask)
								{	//If the note's legacy mask doesn't match the one the user selected from the prompt
									if(!undo_made)
									{
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										undo_made = 1;
									}
									tp->note[ctr2]->legacymask = legacymask;	//Update the note's legacy mask to the user selection
								}
							}
						}
						break;	//Break from "For each note in the active track" loop
					}
					else
					{	//Otherwise mark this note's name as refused
						declined_list[ctr] = 1;	//Mark this note's name as having been declined
					}
				}//For each note in the active track
			}//The user did not enter a legacy bitmask
			if(retval == 56)
			{	//If the user clicked Apply, re-render the screen to reflect any changes made
				eof_render();
			}
		}//If user clicked OK or Apply
		else if(retval == 54)
		{	//If user clicked <- (previous note)
			memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
			eof_selection.current = previous_note;	//Set the previous note as the currently selected note
			eof_selection.multi[previous_note] = 1;	//Ensure the note selection includes the previous note
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, previous_note) + eof_av_delay);	//Seek to previous note
			np = tp->note[eof_selection.current];	//Update note pointer
			eof_render();	//Redraw the screen
		}
		else if(retval == 58)
		{	//If user clicked -> (next note)
			memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
			eof_selection.current = next_note;	//Set the next note as the currently selected note
			eof_selection.multi[next_note] = 1;	//Ensure the note selection includes the next note
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, next_note) + eof_av_delay);	//Seek to next note
			np = tp->note[eof_selection.current];	//Update note pointer
			eof_render();	//Redraw the screen
		}
	}while((retval == 54) || (retval == 56) || (retval == 58));	//Re-run this dialog if the user clicked previous, apply or next

	if(undo_made)
	{	//If any notes were altered
		if(highfretvalue > tp->numfrets)
		{	//If the user input includes a fret value that exceeds the track's current maximum
			(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "Increase the fret limit to %ld to compensate?", highfretvalue);
			if(alert("One or more fret values specified exceed the track's fret limit", eof_etext, NULL, "&Yes", "&No", 'y', 'n') == 1)
			{	//If user opts to increase the fret limit
				tp->numfrets = highfretvalue;
			}
		}
		eof_pro_guitar_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run the fixup logic for this track, since the alteration of the linkNext status can change the length of applicable notes
	}

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

char eof_finger_string_lane_1[4] = {0};	//Use a fourth byte to guarantee proper truncation
char eof_finger_string_lane_2[4] = {0};
char eof_finger_string_lane_3[4] = {0};
char eof_finger_string_lane_4[4] = {0};
char eof_finger_string_lane_5[4] = {0};
char eof_finger_string_lane_6[4] = {0};
char *eof_finger_strings[6] = {eof_finger_string_lane_1, eof_finger_string_lane_2, eof_finger_string_lane_3, eof_finger_string_lane_4, eof_finger_string_lane_5, eof_finger_string_lane_6};

char eof_pro_guitar_note_frets_dialog_function_0_string[] = "Edit note frets / fingering";
char eof_pro_guitar_note_frets_dialog_function_1_string[] = "Edit note fingering";
DIALOG eof_pro_guitar_note_frets_dialog[] =
{
	/*	(proc)				(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          (dp2)          (dp3) */
	{ eof_window_proc,    0,   48,  214, 278,2,   23,  0,    0,      0,         0,   eof_pro_guitar_note_frets_dialog_function_0_string,NULL, NULL },

	//Note:  In guitar theory, string 1 refers to high e
	{ d_agup_text_proc,      60,  80,  64,  8,  2,   23,  0,    0,      0,         0,   "Fret #",     NULL,          NULL },
	{ d_agup_text_proc,      16,  108, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_6_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  104, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_6, "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  132, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_5_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  128, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_5, "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  156, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_4_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  152, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_4, "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  180, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_3_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  176, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_3, "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  204, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_2_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  200, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_2, "0123456789Xx",NULL },
	{ d_agup_text_proc,      16,  228, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_1_number, NULL, NULL },
	{ eof_verified_edit_proc,74,  224, 28,  20, 2,   23,  0,    0,      3,         0,   eof_string_lane_1, "0123456789Xx",NULL },

	{ d_agup_text_proc,      110, 80,  64,  8,  2,   23,  0,    0,      0,         0,   "Finger #",     NULL,          NULL },
	{ eof_verified_edit_proc,130, 104, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_6, "01234XxTt",NULL },
	{ eof_verified_edit_proc,130, 128, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_5, "01234XxTt",NULL },
	{ eof_verified_edit_proc,130, 152, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_4, "01234XxTt",NULL },
	{ eof_verified_edit_proc,130, 176, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_3, "01234XxTt",NULL },
	{ eof_verified_edit_proc,130, 200, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_2, "01234XxTt",NULL },
	{ eof_verified_edit_proc,130, 224, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_1, "01234XxTt",NULL },

	{ d_agup_text_proc,       170, 80,  64,  8,  2,   23,  0,    0,      0,         0,   "Mute",       NULL,          NULL },
	{ d_agup_check_proc,    170, 109, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,    170, 133, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,    170, 157, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,    170, 181, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,    170, 205, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{ d_agup_check_proc,    170, 229, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },

	{ d_agup_button_proc,    10,   256, 20,  28, 2,   23,  0,    D_EXIT, 0,       0,   "<-",         NULL,          NULL },
	{ d_agup_button_proc,    35,   270, 35,  28, 2,   23,  '\r',  D_EXIT, 0,       0,   "OK",         NULL,          NULL },
	{ d_agup_button_proc,    75,   270,  50,  28, 2,   23,  0,   D_EXIT, 0,       0,   "Apply",         NULL,          NULL },
	{ d_agup_button_proc,    130,  270, 55,  28, 2,   23,  0,   D_EXIT, 0,       0,   "Cancel",     NULL,          NULL },
	{ d_agup_button_proc,    188,  256, 20,  28, 2,   23,  0,   D_EXIT, 0,       0,   "->",         NULL,          NULL },
	{ d_agup_button_proc,    10,   286, 20,  28, 2,   23,  0,    D_EXIT, 0,       0,   "<<",         NULL,          NULL },
	{ d_agup_button_proc,    188,  286, 20,  28, 2,   23,  0,   D_EXIT, 0,       0,   ">>",         NULL,          NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_note_edit_pro_guitar_note_frets_fingers_menu(void)
{
	char undo_made = 0;
	if(eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
		return 1;	//If tech view is in effect, don't run the dialog

	return eof_menu_note_edit_pro_guitar_note_frets_fingers(0, &undo_made);
}

long edit_pg_dialog_previous_note = 0, edit_pg_dialog_next_note = 0, edit_pg_dialog_previous_undefined_note = 0, edit_pg_dialog_next_undefined_note = 0;

int eof_menu_note_edit_pro_guitar_note_frets_fingers_prepare_dialog(char dialog, char function)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long ctr, stringcount;
	unsigned long bitmask = 0;		//Used to build the updated pro guitar note bitmask
	EOF_PRO_GUITAR_TRACK *tp;
	EOF_PRO_GUITAR_NOTE *np;
	int first_fret_field = 0;	//Remember the dialog item representing the first populated fret field
	int first_finger_field = 0;	//Remember the dialog item representing the first populated finger field
	int start_focus = 0;		//Used to return to the calling function the dialog field that should have focus by default, ie. the first finger number field missing a value or otherwise the first populated fret field

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 0;	//Do not allow this function to run unless the pro guitar track is active
	if(eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track))
		return 0;	//Do not allow this function to run if a valid note is not selected

	//Find the next/previous notes if applicable
	edit_pg_dialog_previous_note = eof_track_fixup_previous_note(eof_song, eof_selected_track, eof_selection.current);
	if(edit_pg_dialog_previous_note >= 0)
	{	//If there is a previous note
		eof_pro_guitar_note_frets_dialog[28].flags = D_EXIT;		//Make the previous note button clickable
	}
	else
	{
		eof_pro_guitar_note_frets_dialog[28].flags = D_HIDDEN;	//Otherwise hide it
	}
	edit_pg_dialog_next_note = eof_track_fixup_next_note(eof_song, eof_selected_track, eof_selection.current);
	if(edit_pg_dialog_next_note >= 0)
	{	//If there is a next note
		eof_pro_guitar_note_frets_dialog[32].flags = D_EXIT;		//Make the next note button clickable
	}
	else
	{
		eof_pro_guitar_note_frets_dialog[32].flags = D_HIDDEN;	//Otherwise hide it
	}

	//Find the next/previous notes requiring finger definitions if applicable
	tp = eof_song->pro_guitar_track[tracknum];
	edit_pg_dialog_previous_undefined_note = eof_previous_pro_guitar_note_needing_finger_definitions(tp, eof_selection.current);
	if(edit_pg_dialog_previous_undefined_note >= 0)
	{	//If there is a previous note lacking finger definitions
		eof_pro_guitar_note_frets_dialog[33].flags = D_EXIT;		//Make the << button clickable
	}
	else
	{
		eof_pro_guitar_note_frets_dialog[33].flags = D_HIDDEN;	//Otherwise hide it
	}
	edit_pg_dialog_next_undefined_note = eof_next_pro_guitar_note_needing_finger_definitions(tp, eof_selection.current);
	if(edit_pg_dialog_next_undefined_note >= 0)
	{	//If there is a next note lacking finger definitions
		eof_pro_guitar_note_frets_dialog[34].flags = D_EXIT;		//Make the >> button clickable
	}
	else
	{
		eof_pro_guitar_note_frets_dialog[34].flags = D_HIDDEN;	//Otherwise hide it
	}

	//Update the fret text boxes (listed from top to bottom as string 1 through string 6)
	np = tp->note[eof_selection.current];	//Simplify
	stringcount = eof_count_track_lanes(eof_song, eof_selected_track);
	if(eof_legacy_view)
	{	//Special case:  If legacy view is enabled, correct stringcount
		stringcount = tp->numstrings;
	}
	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask<<=1)
	{	//For each of the 6 supported strings
		if(ctr < stringcount)
		{	//If this track uses this string, copy the fret value to the appropriate string
			eof_pro_guitar_note_frets_dialog[12 - (2 * ctr)].flags = 0;		//Ensure this text boxes' label is enabled
			eof_fret_string_numbers[ctr][7] = '0' + (stringcount - ctr);		//Correct the string number for this label
			eof_pro_guitar_note_frets_dialog[13 - (2 * ctr)].flags = 0;		//Ensure this fret # input box is enabled
			if(tp->note[eof_selection.current]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
			{	//If the note has fingerless status
				eof_pro_guitar_note_frets_dialog[20 - ctr].flags = D_HIDDEN;	//Hide this finger # input box
			}
			else
			{	//Otherwise ensure this finger # input box is enabled
				eof_pro_guitar_note_frets_dialog[20 - ctr].flags = 0;			//Ensure this finger # input box is enabled
			}
			eof_pro_guitar_note_frets_dialog[27 - ctr].flags = 0;			//Ensure this mute check box is enabled
			if(np->note & bitmask)
			{	//If this string is already defined as being in use, copy its fret value to the string
				if(!first_fret_field)
					first_fret_field = 13 - (2 * ctr);	//If first_fret_field hasn't been set yet, set it to the first populated fret input field

				if(np->frets[ctr] == 0xFF)
				{	//If this string is muted with no fret value specified
					(void) snprintf(eof_fret_strings[ctr], sizeof(eof_fret_strings[ctr]) - 1, "X");
					eof_finger_strings[ctr][0] = '\0';	//Empty the fingering string
					eof_pro_guitar_note_frets_dialog[27 - ctr].flags = D_SELECTED;	//Check the mute box for this string
				}
				else
				{	//If this string has a fret value specified
					(void) snprintf(eof_fret_strings[ctr], sizeof(eof_fret_strings[ctr]) - 1, "%d", np->frets[ctr] & 0x7F);	//Mask out the MSB to obtain the fret value

					if(np->frets[ctr] & 0x80)
					{	//If the fret number's MSB is set, the string is muted
						eof_pro_guitar_note_frets_dialog[27 - ctr].flags = D_SELECTED;	//Check the mute box for this string
					}
					if(np->finger[ctr] != 0)
					{	//If the finger used to fret this string is defined
						if(!first_finger_field)
							first_finger_field = 20 - ctr;	//If first_finger_field hasn't been set yet, set it to the first populated finger input field

						(void) snprintf(eof_finger_strings[ctr], sizeof(eof_finger_strings[ctr]) - 1, "%u", np->finger[ctr]);	//Create the finger string
						if(eof_finger_strings[ctr][0] == '5')
						{	//If this is the value for the thumb
							eof_finger_strings[ctr][0] = '0';	//Convert to 0, which specifies the thumb in Rocksmith numbering
						}
					}
					else
					{
						if(!start_focus)
							start_focus = 20 - ctr;	//If this is the first undefined finger number, track the corresponding dialog field

						eof_finger_strings[ctr][0] = '\0';	//Otherwise empty the string
					}
				}
			}
			else
			{	//Otherwise empty the fret and finger strings
				eof_fret_strings[ctr][0] = '\0';
				eof_finger_strings[ctr][0] = '\0';
				if(function)
				{	//If the calling function wanted finger input boxes for unused strings disabled
					eof_pro_guitar_note_frets_dialog[20 - ctr].flags = D_DISABLED;		//Ensure this finger # input box is disabled
				}
				else if(tp->note[eof_selection.current]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
				{	//If the note has fingerless status
					eof_pro_guitar_note_frets_dialog[20 - ctr].flags = D_HIDDEN;		//Ensure this finger # input box is hidden
				}
				else
				{
					eof_pro_guitar_note_frets_dialog[20 - ctr].flags = 0;				//Ensure this finger # input box is enabled
				}
			}
			if(function)
			{	//If the calling function wanted fret input boxes disabled
				eof_pro_guitar_note_frets_dialog[13 - (2 * ctr)].flags = D_DISABLED;	//Ensure this text box is disabled
			}
			else
			{
				eof_pro_guitar_note_frets_dialog[13 - (2 * ctr)].flags = 0;		//Ensure this text box is enabled
			}
		}
		else
		{	//Otherwise disable the inputs for this string
			eof_pro_guitar_note_frets_dialog[12 - (2 * ctr)].flags = D_HIDDEN;	//Ensure this text boxes' label is hidden
			eof_pro_guitar_note_frets_dialog[13 - (2 * ctr)].flags = D_HIDDEN;	//Ensure this text box is hidden
			eof_pro_guitar_note_frets_dialog[20 - ctr].flags = D_HIDDEN;		//Ensure this finger # input box is hidden
			eof_pro_guitar_note_frets_dialog[27 - ctr].flags = D_HIDDEN;		//Ensure this mute check box is hidden
			eof_fret_strings[ctr][0] = '\0';
			eof_finger_strings[ctr][0] = '\0';
		}
	}//For each of the 6 supported strings

	if(dialog == 1)
	{	//In the "Edit note fingering" use of this function
		for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask<<=1)
		{	//For each of the 6 supported strings
			if(ctr < stringcount)
			{	//If this track uses this string
				eof_pro_guitar_note_frets_dialog[13 - (2 * ctr)].flags = D_DISABLED;	//Disable edits for the fret number input field
				eof_pro_guitar_note_frets_dialog[27 - ctr].flags = D_HIDDEN;		//Hide the mute check box
			}
		}
		eof_pro_guitar_note_frets_dialog[21].flags = D_HIDDEN;	//Hide the mute column header
		eof_pro_guitar_note_frets_dialog[0].dp = eof_pro_guitar_note_frets_dialog_function_1_string;	//Correct the dialog's title bar
	}
	else
	{	//In the "Edit note frets/fingering" use of this function
		eof_pro_guitar_note_frets_dialog[21].flags = 0;	//Show the mute column header
		eof_pro_guitar_note_frets_dialog[0].dp = eof_pro_guitar_note_frets_dialog_function_0_string;	//Correct the dialog's title bar
	}

	if(!start_focus)
	{	//If no missing finger definition was found
		if(!dialog)
			start_focus = first_fret_field;	//Set the starting focus to the first populated fret number field if the "Edit note frets/fingering" dialog is being prepared
		else
			start_focus = first_finger_field;	//Otherwise set it to the first populated finger number field
	}
	if(!start_focus)
	{	//If no suitable initial focus was found, do not return 0 upon success
		if(!dialog)
			start_focus = 13;	//In the case of the "Edit note frets/fingering" dialog, init to the last fret input field
		else
			start_focus = 20;	//Otherwise init to the last finger input field
	}

	return start_focus;
}

int eof_menu_note_edit_pro_guitar_note_frets_fingers(char function, char *undo_made)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long ctr, ctr2, trackctr, i;
	long fretvalue, fingervalue, highfretvalue;
	unsigned long bitmask = 0;		//Used to build the updated pro guitar note bitmask
	char allmuted;					//Used to track whether all used strings are string muted
	unsigned long flags;			//Used to build the updated flag bitmask
	int retval, retval2, note_selection_updated;
	static char dont_ask = 0;	//Is set to nonzero if the user opts to suppress the prompt regarding modifying multiple selected notes
	EOF_PRO_GUITAR_TRACK *tp, *tp2;
	EOF_PRO_GUITAR_NOTE *np;
	char retry, fingeringdefined, offerupdatefingering;
	char rerun;	//Tracks whether the dialog will re-initialize and run again, such as if any buttons other than OK or cancel were used
	int init_focus;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 0;	//Do not allow this function to run unless the pro guitar track is active
	if(!undo_made)
		return 0;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tp = eof_song->pro_guitar_track[tracknum];
	highfretvalue = tp->numfrets;
	if(!eof_music_paused)
	{
		eof_music_play(0);
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pro_guitar_note_frets_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_pro_guitar_note_frets_dialog);

	do
	{
		if(eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track))
			return 0;	//Do not allow this function to run if a valid note isn't selected

		init_focus = eof_menu_note_edit_pro_guitar_note_frets_fingers_prepare_dialog(0, function);	//Prepare the dialog

		//Run and process the dialog
		rerun = 0;
		retval = 0;
		bitmask = 0;
		retry = fingeringdefined = offerupdatefingering = 0;
		retval = eof_popup_dialog(eof_pro_guitar_note_frets_dialog, init_focus);
		if((retval == 29) || (retval == 30))
		{	//If user clicked OK or Apply
			//Validate the finger strings
			for(i = 0; i < 6; i++)
			{	//For each of the supported strings
				if(eof_finger_strings[i][0] != '\0')
				{	//If this string has a finger value defined
					fingeringdefined = 1;	//The finger fields will need to be validated
					break;
				}
			}
			if(fingeringdefined)
			{	//If any fingering fields are defined, they have to be valid
				for(i = 0; i < 6; i++)
				{	//For each of the supported strings
					if(eof_pro_guitar_note_frets_dialog[27 - i].flags == D_SELECTED)
						continue;	//If the string's mute box is checked, skip it

					if(eof_fret_strings[i][0] != '\0')
					{	//If this string has a fret value
						if(atol(eof_fret_strings[i]) != 0)
						{	//If the value doesn't indicate that it is played open or muted
							if(eof_finger_strings[i][0] == '\0')
							{	//If no finger value is given
								allegro_message("If any fingering is specified, it must be done for all fretted strings.\nCorrect or erase the finger numbers before clicking OK.");
								retry = 1;	//Flag that the user must enter valid finger information
								break;
							}
						}
						else
						{	//The string is played open or muted
							if(((eof_finger_strings[i][0] != '\0') && (toupper(eof_finger_strings[i][0]) != 'X')) && (eof_pro_guitar_note_frets_dialog[27 - i].flags != D_SELECTED))
							{	//If the inputs don't indicate muting
								allegro_message("If any fingering is specified, it must be done for all fretted strings.\nCorrect or erase the finger numbers before clicking OK.");
								retry = 1;
								break;
							}
						}
					}
					else
					{	//There is no fret value defined
						if(eof_finger_strings[i][0] != '\0')
						{	//If there is a fingering defined
							allegro_message("If any fingering is specified, it must be done only for the fretted strings.\nCorrect or erase the finger numbers before clicking OK.");
							retry = 1;
							break;
						}
					}
				}//For each of the supported strings
			}//If any fingering fields are defined, they have to be valid

			//Apply changes
			if(!retry)
			{	//If the finger entries weren't invalid
				if(eof_count_selected_and_unselected_notes(NULL) > 1)
				{	//If multiple notes are selected, warn the user
					eof_clear_input();
					if(!dont_ask)
					{	//If the user didn't suppress this prompt
						retval2 = alert3(NULL, "Warning:  This information will be applied to all selected notes.", NULL, "&OK", "&Cancel", "OK, don't ask again", 'y', 'n', 0);
						if(retval2 == 2)
						{	//If user opts to cancel the operation
							retval = 31;	//Track this as a user cancellation
							break;		//Break from inner while loop
						}
						if(retval2 == 3)
						{	//If this user is suppressing this prompt
							dont_ask = 1;
						}
					}
				}
				for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
				{	//For each note in the track
					if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
						continue;	//If the note isn't selected or in the active track difficulty, skip it
					if(tp->note[i]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
						continue;	//If the note is designated as having no fingering, skip it

					for(ctr = 0, allmuted = 1; ctr < 6; ctr++)
					{	//For each of the 6 supported strings
						if((eof_fret_strings[ctr][0] != '\0') || (eof_pro_guitar_note_frets_dialog[27 - ctr].flags == D_SELECTED))
						{	//If this string isn't empty or is marked as muted
							fretvalue = fingervalue = 0;
							if(eof_finger_strings[ctr][0] != '\0')
							{	//If there is a finger value specified for this string
								if(toupper(eof_finger_strings[ctr][0]) == 'X')
								{	//If the character is an X, it defines that the string is muted
									fretvalue |= 0x80;	//Set the fret value's MSB, leave the finger value at 0 (undefined)
								}
								else
								{	//Otherwise it defines the fingering for the string
									if((eof_finger_strings[ctr][0] == 't') || (eof_finger_strings[ctr][0] == 'T'))
									{	//If the user used the letter T
										eof_finger_strings[ctr][0] = '0';	//Convert to the numeric Rocksmith notation for the thumb
									}
									fingervalue = eof_finger_strings[ctr][0] - '0';	//Convert the ASCII numerical character to decimal
									if(fingervalue == 0)
									{	//User defined use of the thumb
										fingervalue = 5;	//Translate into EOF's notation for the thumb
									}
								}
							}
							if(eof_pro_guitar_note_frets_dialog[27 - ctr].flags == D_SELECTED)
							{	//If the mute box is checked
								fretvalue |= 0x80;	//Set the fret value's MSB
								if(eof_fret_strings[ctr][0] == '\0')
								{	//Unless the fret value is undefined
									fretvalue = 0xFF;	//In which case this is a muted note with no specified fret value
								}
							}
							bitmask |= (1 << ctr);	//Set the appropriate bit for this lane
							for(ctr2 = 0;eof_fret_strings[ctr][ctr2] != '\0';ctr2++)
							{	//For each character in the fret string
								if(toupper(eof_fret_strings[ctr][ctr2]) == 'X')
								{	//If the character is an X
									fretvalue = 0xFF;	//Consider this string muted
									break;
								}
							}
							if(fretvalue != 0xFF)
							{	//If the fret string didn't contain an X, convert it to an integer value
								fretvalue += atol(eof_fret_strings[ctr]);	//Add the converted number so that string muting can be retained
								if((fretvalue < 0) || (fretvalue > 255))
								{	//If the conversion to number failed or specifies a fret EOF cannot store as an 8 bit number
									fretvalue = 0xFF;
								}
								else if((fretvalue & 0x7F) > highfretvalue)
								{	//If the fret value (masking out the mute bit) is higher than the fret value being tracked
									highfretvalue = fretvalue & 0x7F;	//Track the new highest fret value
								}
							}
							if(fretvalue == 0xFF)
							{	//If this string is muted
								fingervalue = 0;	//It doesn't have a fingering
							}
							if(fretvalue != tp->note[i]->frets[ctr])
							{	//If this fret value changed
								if(!*undo_made)
								{	//If an undo state hasn't been made yet
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									*undo_made = 1;
								}
								tp->note[i]->frets[ctr] = fretvalue;
							}
							if(fingervalue != tp->note[i]->finger[ctr])
							{	//If the finger value changed
								if(!*undo_made)
								{	//If an undo state hasn't been made yet
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									*undo_made = 1;
								}
								tp->note[i]->finger[ctr] = fingervalue;
							}
							if((fretvalue != 0xFF) && ((fretvalue & 0x80) == 0))
							{	//Track whether all of the used strings in this note/chord are muted
								allmuted = 0;
							}
						}
						else
						{	//Clear the fret value and return the fret back to its default of 0 (open)
							bitmask &= ~(1 << ctr);	//Clear the appropriate bit for this lane
							if(tp->note[i]->frets[ctr] != 0)
							{	//If this fret value changed
								if(!*undo_made)
								{	//If an undo state hasn't been made yet
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									*undo_made = 1;
								}
								tp->note[i]->frets[ctr] = 0;
								tp->note[i]->finger[ctr] = 0;
							}
						}
					}//For each of the 6 supported strings
					if(bitmask == 0)
					{	//If edits results in selected notes having no played strings
						(void) eof_menu_note_delete();		//All selected notes must be deleted
						eof_show_mouse(NULL);
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						return 1;					//Return OK selected
					}

		//Save the updated note bitmask
					if(tp->note[i]->note != bitmask)
					{	//If the note bitmask changed
						if(!*undo_made)
						{	//If an undo state hasn't been made yet
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							*undo_made = 1;
						}
						tp->note[i]->note = bitmask;
					}

		//Update the flags to reflect string muting
					flags = tp->note[i]->flags;
					if(!allmuted)
					{	//If any used strings in this note/chord weren't string muted
						flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE);	//Clear the string mute flag
					}
					else if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
					{	//If all strings are muted and the user didn't specify a palm mute
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;		//Set the string mute flag
					}
					tp->note[i]->flags = flags;
				}//For each note in the track

				//Offer to update the fingering for all notes in the track matching the selected note (which all selected notes now match because they were altered if they didn't)
				if(eof_note_count_colors(eof_song, eof_selected_track, eof_selection.current) > 1)
				{	//If the note that was edited is a chord
					if(fingeringdefined && eof_auto_complete_fingering && !(tp->note[eof_selection.current]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS))
					{	//If the fingering is defined, and the user didn't disable this auto-completion feature, and the selected note isn't designated as having no fingering
						for(trackctr = 1; trackctr < eof_song->tracks; trackctr++)
						{	//For each track in the project
							if(!eof_track_is_pro_guitar_track(eof_song, trackctr))	//If this isn't a pro guitar track
								continue;	//Skip it

							tp2 = eof_song->pro_guitar_track[eof_song->track[trackctr]->tracknum];
							for(ctr = 0; ctr < tp2->notes; ctr++)
							{	//For each note in the track
								if((trackctr == eof_selected_track) && (ctr == eof_selection.current))	//If this is the selected note that was just edited
									continue;	//Skip it
								if(eof_note_compare(eof_song, eof_selected_track, eof_selection.current, trackctr, ctr, 0))	//If this note doesn't match the one that was just edited
									continue;	//Skip it
								if(eof_pro_guitar_note_fingering_valid(tp2, ctr, 1) == 1)	//If this note's fingering is already fully defined
									continue;	//Skip it
								if(tp2->note[ctr]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
									continue;	//If this note is designated as having no fingering, skip it

								offerupdatefingering = 1;	//Note that the user should be prompted whether to update the fingering array of all matching notes
								break;
							}
						}
						eof_clear_input();
						if(function || (offerupdatefingering && (alert(NULL, "Update all instances of this note to use this fingering?", NULL, "&Yes", "&No", 'y', 'n') == 1)))
						{	//If the user opts to update the fingering array of all matching notes (or if the calling function wanted all instances to be updated automatically)
							for(trackctr = 1; trackctr < eof_song->tracks; trackctr++)
							{	//For each track in the project
								if(!eof_track_is_pro_guitar_track(eof_song, trackctr))	//If this isn't a pro guitar track
									continue;	//Skip it

								tp2 = eof_song->pro_guitar_track[eof_song->track[trackctr]->tracknum];
								for(ctr = 0; ctr < tp2->notes; ctr++)
								{	//For each note in the track
									if((trackctr == eof_selected_track) && (ctr == eof_selection.current))	//If this is the selected note that was just edited
										continue;	//Skip it
									if(eof_note_compare(eof_song, eof_selected_track, eof_selection.current, trackctr, ctr, 0))	//If this note doesn't match the one that was just edited
										continue;	//Skip it
									if(eof_pro_guitar_note_fingering_valid(tp2, ctr, 1) == 1)	//If this note's fingering is already fully defined
										continue;	//Skip it
									if(tp2->note[ctr]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
										continue;	//If this note is designated as having no fingering, skip it

									if(!*undo_made)
									{	//If an undo state hasn't been made yet
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										*undo_made = 1;
									}
									np = tp->note[eof_selection.current];	//Simplify
									for(ctr2 = 0; ctr2 < 6; ctr2++)
									{	//For each of the 6 usable strings
										tp2->note[ctr]->finger[ctr2] = (tp2->note[ctr]->finger[ctr2] & 0x80) + (np->finger[ctr2] & 0x7F);	//Overwrite the finger number, but keep the original mute status intact
									}
								}
							}
						}
					}//If the fingering is defined...
				}//If the note that was edited is a chord

				//Check for a conflict with the track's fret limit
				if(highfretvalue > tp->numfrets)
				{	//If the user input includes a fret value that exceeds the track's current maximum
					(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "Increase the fret limit to %ld to compensate?", highfretvalue);
					if(alert("One or more fret values specified exceed the track's fret limit", eof_etext, NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If user opts to increase the fret limit
						tp->numfrets = highfretvalue;
					}
				}
				eof_pro_guitar_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run the fixup logic for this track in order to enforce the fret limit
			}//If the finger entries weren't invalid
		}//If user clicked OK or Apply
		else if(retval == 28)
		{	//If user clicked <- (previous note)
			rerun = 1;
			eof_selection.current = edit_pg_dialog_previous_note;	//Set the previous note as the currently selected note
		}
		else if(retval == 32)
		{	//If user clicked -> (next note)
			rerun = 1;
			eof_selection.current = edit_pg_dialog_next_note;	//Set the next note as the currently selected note
		}
		else if(retval == 33)
		{	//If user clicked << (next note without finger definitions)
			rerun = 1;
			eof_selection.current = edit_pg_dialog_previous_undefined_note;		//Set the previous note with undefined fingerings as the currently selected note
		}
		else if(retval == 34)
		{	//If user clicked >> (next note without finger definitions)
			rerun = 1;
			eof_selection.current = edit_pg_dialog_next_undefined_note;	//Set the next note with undefined fingerings as the currently selected note
		}
		if(rerun)
		{	//If the dialog is being run again, update the selection and rendered seek position
			memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
			eof_selection.multi[eof_selection.current] = 1;	//Ensure the note selection includes the newly-selected note
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current) + eof_av_delay);	//Seek to that note
			eof_render();	//Redraw the screen
			np = tp->note[eof_selection.current];	//Update note pointer
		}
		if(retval == 30)
		{	//If the user clicked Apply, re-render the screen to reflect any changes made, but don't recreate the selection array as per above logic
			rerun = 1;
			eof_render();
		}
	}while(rerun);	//Re-run this dialog if the user clicked previous, apply, next, << or >>

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	if(retval == 31)
		return 0;	//Return user cancelation (button click)
	if(retval == -1)
		return 0;	//Return user cancelation (Escape key)

	return 1;	//Return OK selected
}

int eof_menu_note_edit_pro_guitar_note_fingers(void)
{
	char undo_made = 0;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long ctr, i;
	long fingervalue;
	unsigned long bitmask = 0;		//Used to build the updated pro guitar note bitmask
	int retval, retval2, note_selection_updated;
	static char dont_ask = 0;	//Is set to nonzero if the user opts to suppress the prompt regarding modifying multiple selected notes
	EOF_PRO_GUITAR_TRACK *tp;
	char rerun;	//Tracks whether the dialog will re-initialize and run again, such as if any buttons other than OK or cancel were used
	int init_focus;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 0;	//Do not allow this function to run unless the pro guitar track is active
	if(eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
		return 0;	//If tech view is in effect, don't run the dialog

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if(eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track))
		return 0;	//Do not allow this function to run if a valid note isn't selected

	tp = eof_song->pro_guitar_track[tracknum];
	if(!eof_music_paused)
	{
		eof_music_play(0);
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pro_guitar_note_frets_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_pro_guitar_note_frets_dialog);

	do
	{
		init_focus = eof_menu_note_edit_pro_guitar_note_frets_fingers_prepare_dialog(1, 0);	//Prepare the dialog

		//Run and process the dialog
		rerun = 0;
		bitmask = 0;
		retval = eof_popup_dialog(eof_pro_guitar_note_frets_dialog, init_focus);
		if((retval == 29) || (retval == 30))
		{	//If user clicked OK or Apply
			//Apply changes
			if(eof_count_selected_and_unselected_notes(NULL) > 1)
			{	//If multiple notes are selected, warn the user
				eof_clear_input();
				if(!dont_ask)
				{	//If the user didn't suppress this prompt
					retval2 = alert3(NULL, "Warning:  This information will be applied to the existing gems of all selected notes.", NULL, "&OK", "&Cancel", "OK, don't ask again", 'y', 'n', 0);
					if(retval2 == 2)
					{	//If user opts to cancel the operation
						retval = 17;	//Track this as a user cancellation
						break;		//Break from inner while loop
					}
					if(retval2 == 3)
					{	//If this user is suppressing this prompt
						dont_ask = 1;
					}
				}
			}

			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the track
				if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
					continue;	//If the note isn't selected or in the active track difficulty, skip it
				if(tp->note[i]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
					continue;	//If the note is designated as having no fingering, skip it

				for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					fingervalue = 0;
					if(eof_finger_strings[ctr][0] != '\0')
					{	//If there is a finger value specified for this string
						if((eof_finger_strings[ctr][0] == 't') || (eof_finger_strings[ctr][0] == 'T'))
						{	//If the user used the letter T
							eof_finger_strings[ctr][0] = '0';	//Convert to the numeric Rocksmith notation for the thumb
						}
						fingervalue = eof_finger_strings[ctr][0] - '0';	//Convert the ASCII numerical character to decimal
						if(fingervalue == 0)
						{	//User defined use of the thumb
							fingervalue = 5;	//Translate into EOF's notation for the thumb
						}

						if((tp->note[i]->note & bitmask) && (fingervalue != tp->note[i]->finger[ctr]))
						{	//If the note has a gem on this string and its finger value is to be changed
							if(!undo_made)
							{	//If an undo state hasn't been made yet
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								undo_made = 1;
							}
							tp->note[i]->finger[ctr] = fingervalue;
						}
					}
				}//For each of the 6 supported strings
			}//For each note in the track
		}//If user clicked OK or Apply
		else if(retval == 28)
		{	//If user clicked <- (previous note)
			rerun = 1;
			eof_selection.current = edit_pg_dialog_previous_note;	//Set the previous note as the currently selected note
		}
		else if(retval == 32)
		{	//If user clicked -> (next note)
			rerun = 1;
			eof_selection.current = edit_pg_dialog_next_note;	//Set the next note as the currently selected note
		}
		else if(retval == 33)
		{	//If user clicked << (next note without finger definitions)
			rerun = 1;
			eof_selection.current = edit_pg_dialog_previous_undefined_note;		//Set the previous note with undefined fingerings as the currently selected note
		}
		else if(retval == 34)
		{	//If user clicked >> (next note without finger definitions)
			rerun = 1;
			eof_selection.current = edit_pg_dialog_next_undefined_note;	//Set the next note with undefined fingerings as the currently selected note
		}
		if(rerun)
		{	//If the dialog is being run again, update the selection and rendered seek position
			memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
			eof_selection.multi[eof_selection.current] = 1;	//Ensure the note selection includes the newly-selected note
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current) + eof_av_delay);	//Seek to that note
			eof_render();	//Redraw the screen
		}
		if(retval == 30)
		{	//If the user clicked Apply, re-render the screen to reflect any changes made, but don't recreate the selection array as per above logic
			rerun = 1;
			eof_render();
		}
	}while(rerun);	//Re-run this dialog if the user clicked previous, apply, next, << or >>

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	if(retval == 17)
		return 0;	//Return user cancelation

	return 1;	//Return OK selected
}

long eof_previous_pro_guitar_note_needing_finger_definitions(EOF_PRO_GUITAR_TRACK * tp, unsigned long note)
{
	unsigned long i;

	if(!tp || (note >= tp->notes))
		return -1;	//Invalid parameters

	for(i = note; i > 0; i--)
	{	//Starting from the specified note and going in reverse order
		if(tp->note[i - 1]->type == tp->note[note]->type)
		{	//If the previous note is in the same difficulty
			if(eof_pro_guitar_note_fingering_valid(tp, i - 1, 0) != 1)
				return i - 1;	//If this note doesn't have fingering fully defined, return it
		}
	}

	return -1;
}

long eof_next_pro_guitar_note_needing_finger_definitions(EOF_PRO_GUITAR_TRACK * tp, unsigned long note)
{
	unsigned long i;

	if(!tp || (note >= tp->notes))
		return -1;	//Invalid parameters

	for(i = note + 1; i < tp->notes; i++)
	{	//Starting from the specified note
		if(tp->note[i]->type == tp->note[note]->type)
		{	//If the next note is in the same difficulty
			if(eof_pro_guitar_note_fingering_valid(tp, i, 0) != 1)
				return i;	//If this note doesn't have fingering fully defined, return it
		}
	}

	return -1;
}

int eof_correct_chord_fingerings(void)
{
	char undo_made = 0;
	return eof_correct_chord_fingerings_option(0, &undo_made);	//Correct fingerings, don't report to user if no corrections were needed
}

int eof_correct_chord_fingerings_menu(void)
{
	char undo_made = 0;
	return eof_correct_chord_fingerings_option(1, &undo_made);	//Correct fingerings, report to user if no corrections were needed
}

int eof_correct_chord_fingerings_option(char report, char *undo_made)
{
	unsigned long ctr, ctr2, tracknum, shapenum = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	char cancelled, user_prompted = 0, restore_tech_view;
	int auto_complete = 0;
	int result;

	if(!eof_song || !undo_made)
		return 0;	//Error

	if(report)
	{	//If the user invoked this function from the Song menu
		user_prompted = 1;	//Suppress asking to make changes since the user already gave intention to make any needed corrections
	}
	eof_song_fix_fingerings(eof_song, undo_made);		//Erase partial note fingerings, replicate valid finger definitions to matching notes without finger definitions
	memset(&eof_selection, 0, sizeof(EOF_SELECTION_DATA));	//Clear the note selection
	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track (skipping the global track, 0)
		if(!eof_track_is_pro_guitar_track(eof_song, ctr))
			continue;	//If this isn't a pro guitar track, skip it

		restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, ctr);
		eof_menu_track_set_tech_view_state(eof_song, ctr, 0); //Disable tech view if applicable
		tracknum = eof_song->track[ctr]->tracknum;
		tp = eof_song->pro_guitar_track[tracknum];
		for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
		{	//For each note in this track
			if(tp->note[ctr2]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
				continue;	//If this note is designated as having no fingering, skip it
			if(eof_note_count_colors(eof_song, ctr, ctr2) <= 1)
				continue;	//If this note isn't a chord, skip it
			if(!eof_fingering_checks_include_mutes && eof_is_string_muted(eof_song, ctr, ctr2))
				continue;	//If the user hasn't opted to validate fingerings for muted strings, and this chord is completed string muted, skip it
			if(eof_pro_guitar_note_fingering_valid(tp, ctr2, eof_fingering_checks_include_mutes) == 1)
				continue;	//If the fingering for this chord is defined and valid (including for muted strings if user has enabled that preference), skip it

			if(!user_prompted)
			{	//If the user hasn't been prompted whether to update the fingering (or if the prompt hasn't been suppressed)
				eof_clear_input();
				if(alert("One or more chords don't have correct finger information", "Update them now?", NULL, "&Yes", "&No", 'y', 'n') != 1)
				{	//If the user does not opt to update the fingering
					eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view); //Re-enable tech view if applicable
					return 0;
				}
				user_prompted = 1;
			}
			result = 0;
			if(auto_complete != 2)
			{	//As long as the user didn't decline to use the chord shape definitions
				result = eof_lookup_chord_shape(tp->note[ctr2], &shapenum, eof_fingering_checks_include_mutes);	//Look for a match in the chord shape definitions, taking muted strings into account if user opted to do so
			}
			if(!auto_complete && result)
			{	//If the user hasn't been prompted whether to use the chord definitions yet, and the chord's fingering CAN be automatically applied
				auto_complete = alert3(NULL, "Automatically apply fingerings for known chord shapes?", NULL, "&Yes", "&No", "&Highlight", 'y', 'n', 'h');
			}

			if((auto_complete == 1) && result)
			{	//If this chord's fingering is to be applied automatically
				if(!*undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				eof_apply_chord_shape_definition(tp->note[ctr2], shapenum, eof_fingering_checks_include_mutes);	//Apply the matching chord shape definition's fingering, taking muted strings into account if user opted to do so
			}
			else if((auto_complete == 3) && result)
			{	//If the user wanted to highlight chords with fingerings that could be automatically applied
				tp->note[ctr2]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Apply highlight status
			}
			else
			{	//If this chord's fingering is to be applied manually
				eof_selection.current = ctr2;	//Select this note
				eof_selection.track = ctr;		//Select this track
				eof_selection.multi[ctr2] = 1;	//Select this note in the selection array
				eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);	//Show the offending note
				cancelled = !eof_menu_note_edit_pro_guitar_note_frets_fingers(1, undo_made);	//Open the edit fret/finger dialog where only the necessary finger fields can be altered
				eof_selection.multi[ctr2] = 0;	//Unselect this note
				eof_selection.current = EOF_MAX_NOTES - 1;
				if(cancelled)
				{	//If the user canceled updating the chord fingering
					eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view); //Re-enable tech view if applicable
					return 0;
				}
			}//If this chord's fingering is to be applied manually
		}//For each note in this track
		eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view); //Re-enable tech view if applicable
	}//For each track (skipping the global track, 0)
	if(report && !(*undo_made) && (auto_complete != 3))
	{	//If no alterations were necessary (excluding the highlight only option) and the calling function wanted this reported
		allegro_message("All fingerings are already defined");
	}
	return 1;
}

int eof_menu_note_lookup_fingering(void)
{
	unsigned long shapenum = 0, notenum;
	int note_selection_updated, result;
	EOF_PRO_GUITAR_TRACK *tp;
	char undo_made = 0;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active
	if(eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
		return 0;	//Do not allow this function to run when tech view is in effect for the active track

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	for(notenum = 0; notenum < eof_get_track_size(eof_song, eof_selected_track); notenum++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[notenum])
			continue;	//If this note isn't selected, skip it
		if(!eof_note_is_chord(eof_song, eof_selected_track, notenum))
			continue;	//If this note is not a chord, skip it

		note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
		if(eof_note_needs_fingering_definition(eof_song, eof_selected_track, notenum))
		{	//If the note does not have valid finger definition
			result = eof_lookup_chord_shape(tp->note[notenum], &shapenum, eof_fingering_checks_include_mutes);	//Look for a match in the chord shape definitions, taking muted strings into account if user opted to do so
			if(result)
			{	//If a suitable chord definition was found
				if(!undo_made)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_apply_chord_shape_definition(tp->note[notenum], shapenum, eof_fingering_checks_include_mutes);	//Apply the matching chord shape definition's fingering, taking muted strings into account if user opted to do so
			}
		}
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_tapping(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//IF this note isn't selected, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		flags ^= EOF_PRO_GUITAR_NOTE_FLAG_TAP;	//Toggle the tap flag
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
		{	//If tapping was just enabled
			flags |= EOF_NOTE_FLAG_F_HOPO;				//Set the legacy HOPO on flag (no strum required for this note)
			flags &= ~EOF_NOTE_FLAG_NO_HOPO;			//Clear the HOPO off flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;		//Clear the hammer on flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;		//Clear the pull off flag
		}
		else
		{	//If tapping was just disabled
			flags &= ~EOF_NOTE_FLAG_F_HOPO;		//Clear the legacy HOPO on flag
		}
		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_tapping(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
			{	//If this note has tap status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;	//Clear the tap flag
				flags &= ~EOF_NOTE_FLAG_F_HOPO;			//Clear the legacy HOPO on flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_bend_logic(int function)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, eflags, tracknum;
	char bends_created = 0;		//Will be set to nonzero if any selected un-bent notes become bend notes
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active
	if(function && !eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to toggle pre-bend status when tech view is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		eflags = eof_get_note_eflags(eof_song, eof_selected_track, i);

		if(!function)
		{	//Toggle bend status
			if(eflags & EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND)
			{	//If the note is already a pre-bend
				eflags ^= EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND;	//Toggle that status off
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;			//Ensure these flags are still set
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
			}
			else
			{
				flags ^= EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Toggle the bend flag
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
				{	//If bend status was just enabled
					bends_created = 1;
				}
			}
		}
		else
		{	//Toggle pre-bend status
			if((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && !(eflags & EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND))
			{	//If the note is already a bend but is not a pre-bend
				eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND;	//Set the pre-bend status
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;			//Ensure these flags are still set
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
			}
			else
			{
				eflags ^= EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND;	//Toggle the pre-bend flag
				if(eflags & EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND)
				{	//If pre-bend status was just enabled on a non-bent note
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
					bends_created = 1;
				}
				else
				{	//Pre-bend status was removed from a pre-bend note
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Also remove bend status
				}
			}
		}

		if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND))
		{	//If the note is no longer a bend note
			if(!(((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)) && eof_song->pro_guitar_track[tracknum]->note[i]->slideend))
			{	//If it is also NOT a slide note with a defined end of slide position
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag because no applicable RS status remains
			}
		}
		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		eof_set_note_eflags(eof_song, eof_selected_track, i, eflags);
		if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND))
		{	//If this is not a bend anymore
			eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength = 0;	//Reset the strength of the bend
		}
	}
	if((eof_write_rs_files || eof_write_rs2_files || eof_write_bf_files) && bends_created)
	{	//If the user wants to save Rocksmith or Bandfuse capable files, prompt to set the strength of bend notes
		(void) eof_pro_guitar_note_bend_strength_no_save();	//Don't make another undo state
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_bend(void)
{
	return eof_menu_note_toggle_bend_logic(0);
}

int eof_menu_note_toggle_prebend(void)
{
	return eof_menu_note_toggle_bend_logic(1);
}

int eof_menu_note_remove_bend(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, eflags, tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			eflags = eof_get_note_eflags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
			{	//If this note has bend status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Clear the bend flag
				eflags &= ~EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND;	//Clear the pre-bend flag
				if(!(((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)) && eof_song->pro_guitar_track[tracknum]->note[i]->slideend))
				{	//If it is also NOT a slide note with a defined end of slide position
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag because no applicable RS status remains
				}
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				eof_set_note_eflags(eof_song, eof_selected_track, i, eflags);
				eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength = 0;	//Reset the strength of the bend
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_harmonic(void)
{
	return eof_menu_note_toggle_flag(0, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC);
}

int eof_menu_note_remove_harmonic(void)
{
	return eof_menu_note_clear_flag(0, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC);
}

int eof_menu_note_toggle_slide_up(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	char slides_present = 0;	//Will be set to nonzero if any selected notes become slide notes
	char slide_change;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		slide_change = 0;
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;			//Toggle the slide up flag
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the slide down flag
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
		{	//If the note now slides up
			slides_present = 1;	//Track that at least one selected note is an up slide
			slide_change = 1;	//Track that this note became an upward slide
		}
		else
		{	//If the note doesn't slide, clear the slide end fret status
			if(!((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength))
			{	//If this is NOT also a bend note with a defined bend strength
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag
			}
		}
		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || slide_change)
		{	//If this is not a slide note anymore, or it it just became an upward slide (ie. if it was a downward slide it may a now invalid end position)
			eof_song->pro_guitar_track[tracknum]->note[i]->slideend = 0;	//Reset the ending fret number of the slide
		}
	}
	if((eof_write_rs_files || eof_write_rs2_files || eof_write_bf_files) && slides_present)
	{	//If the user wants to save Rocksmith or Bandfuse capable files, prompt to set the ending fret for the slide notes
		(void) eof_pro_guitar_note_slide_end_fret_no_save();	//Don't make another undo state
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_slide_down(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	char slides_present = 0;	//Will be set to nonzero if any selected notes become slide notes
	char slide_change;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		slide_change = 0;
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;		//Toggle the slide down flag
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the slide down flag
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
		{	//If the note now slides down
			slides_present = 1;	//Track that at least one selected note is a down slide
			slide_change = 1;	//Track that this note became a downward slide
		}
		else
		{	//If the note doesn't slide, clear the slide end fret status
			if(!((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength))
			{	//If this is NOT also a bend note with a defined bend strength
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag
			}
		}
		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || slide_change)
		{	//If this is not a slide note anymore, or it it just became a downward slide (ie. if it was an upward slide it may a now invalid end position)
			eof_song->pro_guitar_track[tracknum]->note[i]->slideend = 0;	//Reset the ending fret number of the slide
		}
	}
	if((eof_write_rs_files || eof_write_rs2_files || eof_write_bf_files) && slides_present)
	{	//If the user wants to save Rocksmith or Bandfuse capable files, prompt to set the ending fret for the slide notes
		(void) eof_pro_guitar_note_slide_end_fret_no_save();	//Don't make another undo state
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_pitched_slide(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags, tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		oldflags = flags;							//Save an extra copy of the original flags
		flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the slide up flag
		flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the slide down flag
		if(!((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength))
		{	//If this is NOT also a bend note with a defined bend strength
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;		//Clear this flag
		}
		if(!u && (oldflags != flags))
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		eof_song->pro_guitar_track[tracknum]->note[i]->slideend = 0;	//Reset the ending fret number of the slide
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_all_slide(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags, tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		oldflags = flags;							//Save an extra copy of the original flags
		flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);			//Clear the slide up flag
		flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);		//Clear the slide down flag
		flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE);	//Clear the unpitched slide flag
		if(!((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength))
		{	//If this is NOT also a bend note with a defined bend strength
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;		//Clear this flag
		}
		if(!u && (oldflags != flags))
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		eof_song->pro_guitar_track[tracknum]->note[i]->slideend = 0;	//Reset the ending fret number of the slide
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_reverse_slide(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		oldflags = flags;
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
		{	//If the note currently slides up
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;		//Clear the slide up flag
			flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;		//Set the slide down flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE;	//Ensure the reverse slide flag is clear
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
		{	//If the note currently slides down
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;		//Clear the slide down flag
			flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;		//Set the slide up flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE;	//Ensure the reverse slide flag is clear
		}
		if(!u && (oldflags != flags))
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_convert_slide_to_unpitched(void)
{
	unsigned long ctr;
	long u = 0;
	unsigned long flags, skipped = 0;
	int note_selection_updated;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr])
			continue;	//If this note isn't selected, skip it
		flags = tp->note[ctr]->flags;
		if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && !(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
			continue;	//If this note does not have a pitched slide, skip it
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
			continue;	//If this note also has an unpitched slide, skip it

		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION)
		{	//If the end of slide position was defined
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			tp->note[ctr]->unpitchend = tp->note[ctr]->slideend;	//Transfer the end of slide position to the end of unpitched slide position
			tp->note[ctr]->slideend = 0;
			if(!((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && tp->note[ctr]->bendstrength))
			{	//If this is NOT also a bend note with a defined bend strength
				flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION);	//Clear the RS notation flag to signify that neither an end of (pitched) slide position nor a bend strength are defined
			}
		}
		else
		{
			unsigned char lowestfret = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Determine the lowest used fret value for the note
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
			{	//If the note was an upward slide
				if(lowestfret + 1 > tp->numfrets)
				{	//If a valid end of upward slide position cannot be defined for this note
					skipped++;
					tp->note[ctr]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Highlight the offending note
					continue;	//Skip it
				}
				tp->note[ctr]->unpitchend = lowestfret + 1;	//Define an upward slide of one fret
			}
			else
			{	//The note was a downward slide
				if(lowestfret < 2)
				{	//If a valid end of downward slide position cannot be defined for this note
					skipped++;
					tp->note[ctr]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Highlight the offending note
					continue;	//Skip it
				}
				tp->note[ctr]->unpitchend = lowestfret - 1;	//Define a downward slide of one fret
			}
		}
		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the slide up flag
		flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the slide down flag
		flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;	//Set the unpitched slide flag
		tp->note[ctr]->flags = flags;
	}
	if(skipped)
	{	//If at least one slide note was skipped
		allegro_message("%lu slide note%s skipped during conversion since a usable end of slide position could not be defined.\nThe offending note%s been highlighted.", skipped, ((skipped == 1) ? " was" : "s were"), ((skipped == 1) ? " has" : "s have"));
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_convert_slide_to_pitched(void)
{
	unsigned long ctr;
	long u = 0;
	unsigned long flags;
	int note_selection_updated;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the active track
		unsigned char lowestfret;

		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr])
			continue;	//If this note isn't selected, skip it
		flags = tp->note[ctr]->flags;
		if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE) || !tp->note[ctr]->unpitchend)
			continue;	//If this note doesn't have an  unpitched slide or its unpitched slide end position is not defined, skip it
		if((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
			continue;	//If this note has a pitched slide, skip it

		lowestfret = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Determine the lowest used fret value for the note
		if(tp->note[ctr]->unpitchend > lowestfret)
		{	//If this slide is upward
			flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
		}
		else if(tp->note[ctr]->unpitchend < lowestfret)
		{	//If this slide is downward
			flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
		}
		else
		{	//Invalid slide
			continue;	//Skip it
		}
		flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;			//Set this flag to indicate that the end of slide position is defined
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE);		//Clear the unpitched slide flag

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		tp->note[ctr]->slideend = tp->note[ctr]->unpitchend;			//Transfer the unpitched end of slide position to the pitched end of slide position
		tp->note[ctr]->unpitchend = 0;
		tp->note[ctr]->flags = flags;
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_palm_muting(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;		//Toggle the palm mute flag
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE);	//Clear the string mute flag
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_palm_muting(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;								//Save an extra copy of the original flags
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE);	//Clear the palm mute flag
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_arpeggio_mark_logic(int handshape)
{
	unsigned long section_type = handshape ? EOF_HANDSHAPE_SECTION : EOF_ARPEGGIO_SECTION;

	return eof_menu_section_mark(section_type);
}

int eof_menu_arpeggio_mark(void)
{
	return eof_menu_arpeggio_mark_logic(0);
}

int eof_menu_handshape_mark(void)
{
	return eof_menu_arpeggio_mark_logic(1);
}

int eof_menu_arpeggio_unmark_logic(int handshape)
{
	unsigned long i, j, ghostnote;
	unsigned long tracknum;
	int note_selection_updated;
	EOF_PRO_GUITAR_TRACK *tp;
	char undo_made = 0;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Don't allow this function to run unless a pro guitar track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If this note isn't selected, skip it

		for(j = 0; j < tp->arpeggios; j++)
		{	//For each arpeggio section in the track
			if((eof_get_note_pos(eof_song, eof_selected_track, i) < tp->arpeggio[j].start_pos) || (eof_get_note_pos(eof_song, eof_selected_track, i) > tp->arpeggio[j].end_pos) || (tp->arpeggio[j].difficulty != eof_note_type))
				continue;	//If the note is not encompassed within this arpeggio section, or the arpeggio section doesn't exists in the active difficulty, skip it
			if(handshape && !(tp->arpeggio[j].flags & EOF_RS_ARP_HANDSHAPE))
				continue;	//If only handshape phrases were to be removed, but this is an arpeggio phrase, skip this phrase
			if(!handshape && (tp->arpeggio[j].flags & EOF_RS_ARP_HANDSHAPE))
				continue;	//If only arpeggio phrases were to be removed, but this is a handshape phrase, skip this phrase

			if(!undo_made)
			{	//If an undo hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				undo_made = 1;
			}

			//Delete the ghost gems that were placed at the start of the arpeggio/hanshape
			EOF_NOTE_SEARCH_INFO si = {
				.track = eof_selected_track,
				.snap_pos = -1,
				.pos = tp->arpeggio[j].start_pos,
				.x_tolerance = 0,
				.nlen = 0
			};
			ghostnote = eof_find_note_at_pos(&si, NULL);
			if(ghostnote < eof_get_track_size(eof_song, eof_selected_track))
			{	//If a note was found at the start of this arpeggio/handshape
				unsigned long note = eof_get_note_note(eof_song, eof_selected_track, ghostnote);
				note &= ~eof_get_note_ghost(eof_song, eof_selected_track, ghostnote);	//Clear all gems that are ghosted
				eof_set_note_note(eof_song, eof_selected_track, ghostnote, note);		//Update the note at the start of the arpeggio/handshape
			}

			eof_pro_guitar_track_delete_arpeggio(tp, j);	//Delete the arpeggio section
			break;
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Delete any notes that no longer have any gems (ie. they were arpeggio/handshape ghost chords with no non-ghost gems)
	return 1;
}

int eof_menu_arpeggio_unmark(void)
{
	return eof_menu_arpeggio_unmark_logic(0);
}

int eof_menu_handshape_unmark(void)
{
	return eof_menu_arpeggio_unmark_logic(1);
}

void eof_pro_guitar_track_delete_arpeggio(EOF_PRO_GUITAR_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(!tp || (index >= tp->arpeggios))
		return;	//Invalid parameters

	tp->arpeggio[index].name[0] = '\0';	//Empty the name string
	for(i = index; i < tp->arpeggios - 1; i++)
	{
		memcpy(&tp->arpeggio[i], &tp->arpeggio[i + 1], sizeof(EOF_PHRASE_SECTION));
	}
	tp->arpeggios--;
}

int eof_menu_arpeggio_erase_all(void)
{
	unsigned long ctr, tracknum;
	EOF_PRO_GUITAR_TRACK * tp;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	eof_clear_input();
	if(alert(NULL, "Erase all arpeggios from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		tp = eof_song->pro_guitar_track[tracknum];
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = tp->arpeggios; ctr > 0; ctr--)
		{	//For each arpeggio section in this track, in reverse order
			if(!(tp->arpeggio[ctr - 1].flags & EOF_RS_ARP_HANDSHAPE))
			{	//If this is an arpeggio phrase and not a handshape phrase
				eof_pro_guitar_track_delete_arpeggio(tp, ctr - 1);	//Remove it
			}
		}
		eof_song->pro_guitar_track[tracknum]->arpeggios = 0;
	}
	return 1;
}

int eof_menu_handshape_erase_all(void)
{
	unsigned long ctr, tracknum;
	EOF_PRO_GUITAR_TRACK * tp;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	eof_clear_input();
	if(alert(NULL, "Erase all handshape phrases from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		tp = eof_song->pro_guitar_track[tracknum];
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = tp->arpeggios; ctr > 0; ctr--)
		{	//For each arpeggio section in this track, in reverse order
			if(tp->arpeggio[ctr - 1].flags & EOF_RS_ARP_HANDSHAPE)
			{	//If this is a handshape phrase instead of an arpeggio phrase
				eof_pro_guitar_track_delete_arpeggio(tp, ctr - 1);	//Remove it
			}
		}
		eof_song->pro_guitar_track[tracknum]->arpeggios = 0;
	}
	return 1;
}

int eof_menu_trill_mark(void)
{
	int retval = eof_menu_section_mark(EOF_TRILL_SECTION);

	if(eof_track_is_drums_rock_mode(eof_song, eof_selected_track))
	{	//If the active track has Drums Rock mode enabled
		(void) eof_menu_note_edit_name();	//Prompt the user for the drum roll count as well
	}

	return retval;
}

int eof_menu_tremolo_mark(void)
{
	int retval = eof_menu_section_mark(EOF_TREMOLO_SECTION);

	if(eof_track_is_drums_rock_mode(eof_song, eof_selected_track))
	{	//If the active track has Drums Rock mode enabled
		(void) eof_menu_note_edit_name();	//Prompt the user for the drum roll count as well
	}

	return retval;
}

int eof_menu_slider_mark(void)
{
	return eof_menu_section_mark(EOF_SLIDER_SECTION);
}

int eof_menu_trill_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated;
	char undo_made = 0;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_get_num_trills(eof_song, eof_selected_track); j++)
			{	//For each trill section in the track
				sectionptr = eof_get_trill(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= sectionptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= sectionptr->end_pos))
				{	//If the note is encompassed within this trill section
					if(!undo_made)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eof_track_delete_trill(eof_song, eof_selected_track, j);	//Delete the trill section
					break;
				}
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_tremolo_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated;
	char undo_made = 0;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If this note isn't selected, skip it

		for(j = 0; j < eof_get_num_tremolos(eof_song, eof_selected_track); j++)
		{	//For each tremolo section in the track
			sectionptr = eof_get_tremolo(eof_song, eof_selected_track, j);
			if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
			{	//If the track's difficulty limit has been removed
				if(sectionptr->difficulty != eof_note_type)	//And the tremolo section does not apply to the active track difficulty
					continue;
			}
			else
			{
				if(sectionptr->difficulty != 0xFF)	//Otherwise if the tremolo section does not apply to all track difficulties
					continue;
			}
			if((eof_get_note_pos(eof_song, eof_selected_track, i) >= sectionptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= sectionptr->end_pos))
			{	//If the note is encompassed within this tremolo section
				if(!undo_made)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_track_delete_tremolo(eof_song, eof_selected_track, j);	//Delete the tremolo section
				break;
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_slider_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated;
	char undo_made = 0;

	if(!eof_track_is_legacy_guitar(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a legacy guitar track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_get_num_sliders(eof_song, eof_selected_track); j++)
			{	//For each slider section in the track
				sectionptr = eof_get_slider(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= sectionptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= sectionptr->end_pos))
				{	//If the note is encompassed within this slider section
					if(!undo_made)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eof_track_delete_slider(eof_song, eof_selected_track, j);	//Delete the slider section
					break;
				}
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_trill_erase_all(void)
{
	char drum_track_prompt[] = "Erase all special drum rolls from this track?";
	char other_track_prompt[] = "Erase all trills from this track?";
	char *prompt = other_track_prompt;	//By default, assume a non drum track

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		prompt = drum_track_prompt;	//If this is a drum track, refer to the sections as "special drum rolls" instead of "trills"

	eof_clear_input();
	if(alert(NULL, prompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_trills(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;
}

int eof_menu_tremolo_erase_all(void)
{
	char drum_track_prompt[] = "Erase all drum rolls from this track?";
	char other_track_prompt[] = "Erase all tremolos from this track?";
	char *prompt = other_track_prompt;	//By default, assume a non drum track

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		prompt = drum_track_prompt;	//If this is a drum track, refer to the sections as "drum rolls" instead of "tremolos"

	eof_clear_input();
	if(alert(NULL, prompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_tremolos(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;
}

int eof_menu_slider_erase_all(void)
{
	if(!eof_track_is_legacy_guitar(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a legacy guitar track is active

	eof_clear_input();
	if(alert(NULL, "Erase all slider sections from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_sliders(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;
}

int eof_menu_trill_edit_timing(void)
{
	char *phrasename1 = "trill";
	char *phrasename2 = "special drum roll";
	char *effectivephrasename = phrasename1;
	EOF_PHRASE_SECTION *phraseptr;
	int note_selection_updated;
	unsigned long start, end;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account)

	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
	{	//If a note is selected
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If the active track is a drum track
			effectivephrasename = phrasename2;
		}
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_TRILL_SECTION, eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));
		if(phraseptr)
		{	//If the seek position is within a trill phrase
			if(note_selection_updated)
			{	//If notes were selected based on start/end points, use these for the section timings
				start = eof_song->tags->start_point;
				end = eof_song->tags->end_point;
			}
			else
			{
				start = phraseptr->start_pos;
				end = phraseptr->end_pos;
			}
			snprintf(eof_etext3, sizeof(eof_etext3) - 1, "Edit %s", effectivephrasename);	//Set the title of the dialog
			eof_phrase_edit_timing(&phraseptr->start_pos, &phraseptr->end_pos, start, end);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_tremolo_edit_timing(void)
{
	char *phrasename1 = "tremolo";
	char *phrasename2 = "drum roll";
	char *effectivephrasename = phrasename1;
	EOF_PHRASE_SECTION *phraseptr;
	int note_selection_updated;
	unsigned long start, end;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account)

	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
	{	//If a note is selected
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If the active track is a drum track
			effectivephrasename = phrasename2;
		}
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_TREMOLO_SECTION, eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));
		if(phraseptr)
		{	//If the seek position is within a tremolo phrase
			if(note_selection_updated)
			{	//If notes were selected based on start/end points, use these for the section timings
				start = eof_song->tags->start_point;
				end = eof_song->tags->end_point;
			}
			else
			{
				start = phraseptr->start_pos;
				end = phraseptr->end_pos;
			}
			snprintf(eof_etext3, sizeof(eof_etext3) - 1, "Edit %s", effectivephrasename);	//Set the title of the dialog
			eof_phrase_edit_timing(&phraseptr->start_pos, &phraseptr->end_pos, start, end);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_slider_edit_timing(void)
{
	EOF_PHRASE_SECTION *phraseptr;
	int note_selection_updated;
	unsigned long start, end;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account)

	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
	{	//If a note is selected
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_SLIDER_SECTION, eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));
		if(phraseptr)
		{	//If the seek position is within a slider phrase
			if(note_selection_updated)
			{	//If notes were selected based on start/end points, use these for the section timings
				start = eof_song->tags->start_point;
				end = eof_song->tags->end_point;
			}
			else
			{
				start = phraseptr->start_pos;
				end = phraseptr->end_pos;
			}
			snprintf(eof_etext3, sizeof(eof_etext3) - 1, "Edit slider");	//Set the title of the dialog
			eof_phrase_edit_timing(&phraseptr->start_pos, &phraseptr->end_pos, start, end);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_arpeggio_edit_timing(void)
{
	EOF_PHRASE_SECTION *phraseptr;
	int note_selection_updated;
	unsigned long start, end;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account)

	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
	{	//If a note is selected
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));
		if(phraseptr && !(phraseptr->flags & EOF_RS_ARP_HANDSHAPE))
		{	//If the seek position is within an arpeggio phrase and is NOT marked as a handshape
			if(note_selection_updated)
			{	//If notes were selected based on start/end points, use these for the section timings
				start = eof_song->tags->start_point;
				end = eof_song->tags->end_point;
			}
			else
			{
				start = phraseptr->start_pos;
				end = phraseptr->end_pos;
			}
			snprintf(eof_etext3, sizeof(eof_etext3) - 1, "Edit arpeggio");	//Set the title of the dialog
			eof_phrase_edit_timing(&phraseptr->start_pos, &phraseptr->end_pos, start, end);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_handshape_edit_timing(void)
{
	EOF_PHRASE_SECTION *phraseptr;
	int note_selection_updated;
	unsigned long start, end;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account)

	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
	{	//If a note is selected
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));
		if(phraseptr && (phraseptr->flags & EOF_RS_ARP_HANDSHAPE))
		{	//If the seek position is within an arpeggio phrase and is marked as a handshape
			if(note_selection_updated)
			{	//If notes were selected based on start/end points, use these for the section timings
				start = eof_song->tags->start_point;
				end = eof_song->tags->end_point;
			}
			else
			{
				start = phraseptr->start_pos;
				end = phraseptr->end_pos;
			}
			snprintf(eof_etext3, sizeof(eof_etext3) - 1, "Edit handshape");	//Set the title of the dialog
			eof_phrase_edit_timing(&phraseptr->start_pos, &phraseptr->end_pos, start, end);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_note_clear_legacy_values(void)
{
	unsigned long i;
	long u = 0;
	unsigned long tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(!u && eof_song->pro_guitar_track[tracknum]->note[i]->legacymask)
			{	//Make a back up before clearing the first legacy bitmask
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = 0;
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_edit_name(void)
{
	unsigned long i;
	char *notename = NULL, undo_made = 0, auto_apply = 0;
	DIALOG *dialog_to_use = eof_note_name_dialog;
	int ret;

	if(!eof_music_catalog_playback)
	{
		int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

		if(eof_track_is_drums_rock_mode(eof_song, eof_selected_track))
		{	//If Drums Rock mode is enabled for the active track
			dialog_to_use = eof_drum_roll_count_dialog;		//Use a different variation of the dialog
		}

		eof_cursor_visible = 0;
		eof_render();
		eof_color_dialog(dialog_to_use, gui_fg_color, gui_bg_color);
		eof_conditionally_center_dialog(dialog_to_use);

		notename = eof_get_note_name(eof_song, eof_selected_track, eof_selection.current);	//Get the last selected note's name
		if(notename == NULL)
		{	//If there was an error getting the name
			eof_etext[0] = '\0';	//Empty the edit field
		}
		else
		{	//Otherwise copy the note name into the edit field
			(void) ustrcpy(eof_etext, notename);
		}

		if(eof_popup_dialog(dialog_to_use, 2) == 3)	//User hit OK
		{
			if(eof_track_is_drums_rock_mode(eof_song, eof_selected_track))
			{	//If Drums Rock mode is enabled for the active track
				unsigned long number = 0;
				ret = eof_read_macro_number(eof_etext, &number);		//Convert the specified string into a number
				if(!ret || (number > 100))
				{	//If the number wasn't parsed, or was out of bounds
					allegro_message("Drum roll count must be between 0 and 100");
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					eof_show_mouse(screen);
					return D_O_K;
				}
			}
			else if((eof_etext[0] == '\0') && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
			{	//If the user kept the name field empty and this is a pro guitar track, offer to apply auto-detected names
				eof_clear_input();
				if(alert(NULL, "Apply automatically-detected chord names?", NULL, "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user opts to apply auto-detected names
					auto_apply = 1;
				}
			}
			else
			{
				for(i = 0; i < (unsigned long)ustrlen(eof_etext); i++)
				{	//For each character in the name field
					if((eof_etext[i] == '[') || (eof_etext[i] == ']'))
					{	//If the character is a bracket
						allegro_message("You cannot use a bracket in the note's name");
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						eof_show_mouse(screen);
						return D_O_K;
					}
				}
			}
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the track
				if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
					continue;	//If this note isn't selected, skip it

				notename = eof_get_note_name(eof_song, eof_selected_track, i);	//Get the note's name
				if(!notename)
					continue;	//If the note's name couldn't be accessed, skip it

				if(auto_apply)
				{	//If applying automatically detected names, get the name of this note
					eof_etext[0] = '\0';	//Empty the string, so that it won't assign a name unless it is detected
					(void) eof_build_note_name(eof_song, eof_selected_track, i, eof_etext);	//Detect the name of this chord
				}
				if(ustrcmp(notename, eof_etext))
				{	//If the updated string (eof_etext) is different from the note's existing name
					if(!undo_made)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eof_set_note_name(eof_song, eof_selected_track, i, eof_etext);	//Update the note's name
					(void) eof_rs_check_chord_name(eof_song, eof_selected_track, eof_selection.current, 0);	//Check if the user included lowercase "maj" in the name
				}
			}
		}
		if(note_selection_updated)
		{	//If the note selection was originally empty and was dynamically updated
			(void) eof_menu_edit_deselect_all();	//Clear the note selection
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_pro_guitar_toggle_strum_up(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		if(!undo_made)
		{	//If an undo state hasn't been made yet
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
			undo_made = 1;
		}
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
		{	//If the note was already set to strum up, set it to strum either
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM);		//Clear the strum up flag
		}
		else
		{	//Otherwise, set it to strum up
			flags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;		//Set the strum up flag
		}
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM);	//Clear the strum down flag
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM);		//Clear the strum mid flag
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_pro_guitar_toggle_strum_down(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected skip it

		if(!undo_made)
		{	//If an undo state hasn't been made yet
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
			undo_made = 1;
		}
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM)
		{	//If the note was already set to strum down, set it to strum either
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM);	//Clear the strum down flag
		}
		else
		{	//Otherwise, set it to strum down
			flags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;		//Set the strum down flag
		}
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM);		//Clear the strum up flag
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM);		//Clear the strum mid flag
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_pro_guitar_toggle_strum_mid(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it

		if(!undo_made)
		{	//If an undo state hasn't been made yet
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
			undo_made = 1;
		}
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM)
		{	//If the note was already set to strum mid, set it to strum either
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM);		//Clear the strum mid flag
		}
		else
		{	//Otherwise, set it to strum mid
			flags |= EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM;		//Set the strum mid flag
		}
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM);	//Clear the strum down flag
		flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM);		//Clear the strum up flag
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_strum_direction(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;							//Save an extra copy of the original flags
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM);	//Clear the strum down flag
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM);		//Clear the strum up flag
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM);		//Clear the strum mid flag
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_copy_solos_track_1(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 1, eof_selected_track);
}

int eof_menu_copy_solos_track_2(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 2, eof_selected_track);
}

int eof_menu_copy_solos_track_3(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 3, eof_selected_track);
}

int eof_menu_copy_solos_track_4(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 4, eof_selected_track);
}

int eof_menu_copy_solos_track_5(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 5, eof_selected_track);
}

int eof_menu_copy_solos_track_6(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 6, eof_selected_track);
}

int eof_menu_copy_solos_track_7(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 7, eof_selected_track);
}

int eof_menu_copy_solos_track_8(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_copy_solos_track_9(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_copy_solos_track_10(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 10, eof_selected_track);
}

int eof_menu_copy_solos_track_11(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_copy_solos_track_12(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_copy_solos_track_13(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 13, eof_selected_track);
}

int eof_menu_copy_solos_track_14(void)
{
	return eof_menu_copy_solos_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_copy_solos_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_solos(sp, sourcetrack))
		return 0;	//Source track has no solo phrases
	if(eof_get_num_solos(sp, desttrack))
	{	//If there are already solos in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing solo phrases in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	while(eof_get_num_solos(sp, desttrack))
	{	//While there are solos in the destination track
		eof_track_delete_solo(sp, desttrack, 0);	//Delete the first one
	}

	for(ctr = 0; ctr < eof_get_num_solos(sp, sourcetrack); ctr++)
	{	//For each solo phrase in the source track
		ptr = eof_get_solo(sp, sourcetrack, ctr);
		if(ptr)
		{	//If this phrase could be found
			(void) eof_track_add_solo(sp, desttrack, ptr->start_pos, ptr->end_pos);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(sp, eof_selected_track);
	return 1;	//Return completion
}

int eof_menu_copy_sp_track_1(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 1, eof_selected_track);
}

int eof_menu_copy_sp_track_2(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 2, eof_selected_track);
}

int eof_menu_copy_sp_track_3(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 3, eof_selected_track);
}

int eof_menu_copy_sp_track_4(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 4, eof_selected_track);
}

int eof_menu_copy_sp_track_5(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 5, eof_selected_track);
}

int eof_menu_copy_sp_track_6(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 6, eof_selected_track);
}

int eof_menu_copy_sp_track_7(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 7, eof_selected_track);
}

int eof_menu_copy_sp_track_8(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_copy_sp_track_9(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_copy_sp_track_10(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 10, eof_selected_track);
}

int eof_menu_copy_sp_track_11(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_copy_sp_track_12(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_copy_sp_track_13(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 13, eof_selected_track);
}

int eof_menu_copy_sp_track_14(void)
{
	return eof_menu_copy_sp_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_copy_sp_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_star_power_paths(sp, sourcetrack))
		return 0;	//Source track has no star power phrases
	if(eof_get_num_star_power_paths(sp, desttrack))
	{	//If there are already star power phrases in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing star power phrases in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	while(eof_get_num_star_power_paths(sp, desttrack))
	{	//While there are star  power phrases in the destination track
		eof_track_delete_star_power_path(sp, desttrack, 0);	//Delete the first one
	}

	for(ctr = 0; ctr < eof_get_num_star_power_paths(sp, sourcetrack); ctr++)
	{	//For each star power phrase in the source track
		ptr = eof_get_star_power_path(sp, sourcetrack, ctr);
		if(ptr)
		{	//If this phrase could be found
			(void) eof_track_add_star_power_path(sp, desttrack, ptr->start_pos, ptr->end_pos);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(sp, eof_selected_track);
	return 1;	//Return completion
}

int eof_menu_copy_arpeggio_track_1(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 1, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_2(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 2, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_3(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 3, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_4(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 4, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_5(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 5, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_6(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 6, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_7(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 7, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_8(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_9(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_10(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 10, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_11(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_12(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_13(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 13, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_14(void)
{
	return eof_menu_copy_arpeggio_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_copy_arpeggio_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_arpeggios(sp, sourcetrack))
		return 0;	//Source track has no arpeggio phrases
	if(eof_get_num_arpeggios(sp, desttrack))
	{	//If there are already arpeggio phrases in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing arpeggio phrases in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	while(eof_get_num_arpeggios(sp, desttrack))
	{	//While there are arpeggio phrases in the destination track
		eof_track_delete_arpeggio(sp, desttrack, 0);	//Delete the first one
	}

	for(ctr = 0; ctr < eof_get_num_arpeggios(sp, sourcetrack); ctr++)
	{	//For each arpeggio phrase in the source track
		ptr = eof_get_arpeggio(sp, sourcetrack, ctr);
		if(ptr && (!(ptr->flags & EOF_RS_ARP_HANDSHAPE)))
		{	//If this phrase could be found and it is an arpeggio and not a handshape phrase
			(void) eof_track_add_section(sp, desttrack, EOF_ARPEGGIO_SECTION, ptr->difficulty, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(sp, eof_selected_track);
	return 1;	//Return completion
}

int eof_menu_copy_trill_track_1(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 1, eof_selected_track);
}

int eof_menu_copy_trill_track_2(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 2, eof_selected_track);
}

int eof_menu_copy_trill_track_3(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 3, eof_selected_track);
}

int eof_menu_copy_trill_track_4(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 4, eof_selected_track);
}

int eof_menu_copy_trill_track_5(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 5, eof_selected_track);
}

int eof_menu_copy_trill_track_6(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 6, eof_selected_track);
}

int eof_menu_copy_trill_track_7(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 7, eof_selected_track);
}

int eof_menu_copy_trill_track_8(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_copy_trill_track_9(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_copy_trill_track_10(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 10, eof_selected_track);
}

int eof_menu_copy_trill_track_11(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_copy_trill_track_12(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_copy_trill_track_13(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 13, eof_selected_track);
}

int eof_menu_copy_trill_track_14(void)
{
	return eof_menu_copy_trill_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_copy_trill_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_trills(sp, sourcetrack))
		return 0;	//Source track has no trill phrases
	if(eof_get_num_trills(sp, desttrack))
	{	//If there are already trill phrases in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing trill phrases in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	while(eof_get_num_trills(sp, desttrack))
	{	//While there are trill phrases in the destination track
		eof_track_delete_trill(sp, desttrack, 0);	//Delete the first one
	}

	for(ctr = 0; ctr < eof_get_num_trills(sp, sourcetrack); ctr++)
	{	//For each trill phrase in the source track
		ptr = eof_get_trill(sp, sourcetrack, ctr);
		if(ptr)
		{	//If this phrase could be found
			(void) eof_track_add_trill(sp, desttrack, ptr->start_pos, ptr->end_pos);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(sp, eof_selected_track);
	return 1;	//Return completion
}

int eof_menu_copy_tremolo_track_1(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 1, eof_selected_track);
}

int eof_menu_copy_tremolo_track_2(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 2, eof_selected_track);
}

int eof_menu_copy_tremolo_track_3(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 3, eof_selected_track);
}

int eof_menu_copy_tremolo_track_4(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 4, eof_selected_track);
}

int eof_menu_copy_tremolo_track_5(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 5, eof_selected_track);
}

int eof_menu_copy_tremolo_track_6(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 6, eof_selected_track);
}

int eof_menu_copy_tremolo_track_7(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 7, eof_selected_track);
}

int eof_menu_copy_tremolo_track_8(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_copy_tremolo_track_9(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_copy_tremolo_track_10(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 10, eof_selected_track);
}

int eof_menu_copy_tremolo_track_11(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_copy_tremolo_track_12(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_copy_tremolo_track_13(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 13, eof_selected_track);
}

int eof_menu_copy_tremolo_track_14(void)
{
	return eof_menu_copy_tremolo_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_copy_tremolo_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_tremolos(sp, sourcetrack))
		return 0;	//Source track has no tremolo phrases
	if(eof_get_num_tremolos(sp, desttrack))
	{	//If there are already tremolo phrases in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing tremolo phrases in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	while(eof_get_num_tremolos(sp, desttrack))
	{	//While there are tremolo phrases in the destination track
		eof_track_delete_tremolo(sp, desttrack, 0);	//Delete the first one
	}

	for(ctr = 0; ctr < eof_get_num_tremolos(sp, sourcetrack); ctr++)
	{	//For each tremolo phrase in the source track
		ptr = eof_get_tremolo(sp, sourcetrack, ctr);
		if(ptr)
		{	//If this phrase could be found
			(void) eof_track_add_tremolo(sp, desttrack, ptr->start_pos, ptr->end_pos, ptr->difficulty);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(sp, eof_selected_track);
	return 1;	//Return completion
}

int eof_menu_copy_sliders_track_1(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 1, eof_selected_track);
}

int eof_menu_copy_sliders_track_2(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 2, eof_selected_track);
}

int eof_menu_copy_sliders_track_3(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 3, eof_selected_track);
}

int eof_menu_copy_sliders_track_4(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 4, eof_selected_track);
}

int eof_menu_copy_sliders_track_5(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 5, eof_selected_track);
}

int eof_menu_copy_sliders_track_6(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 6, eof_selected_track);
}

int eof_menu_copy_sliders_track_7(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 7, eof_selected_track);
}

int eof_menu_copy_sliders_track_8(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_copy_sliders_track_9(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_copy_sliders_track_10(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 10, eof_selected_track);
}

int eof_menu_copy_sliders_track_11(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_copy_sliders_track_12(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_copy_sliders_track_13(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 13, eof_selected_track);
}

int eof_menu_copy_sliders_track_14(void)
{
	return eof_menu_copy_sliders_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_copy_sliders_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_sliders(sp, sourcetrack))
		return 0;	//Source track has no slider phrases
	if(eof_get_num_sliders(sp, desttrack))
	{	//If there are already sliders in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing slider phrases in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	while(eof_get_num_sliders(sp, desttrack))
	{	//While there are sliders in the destination track
		eof_track_delete_slider(sp, desttrack, 0);	//Delete the first one
	}

	for(ctr = 0; ctr < eof_get_num_sliders(sp, sourcetrack); ctr++)
	{	//For each slider phrase in the source track
		ptr = eof_get_slider(sp, sourcetrack, ctr);
		if(ptr)
		{	//If this phrase could be found
			(void) eof_track_add_slider(sp, desttrack, ptr->start_pos, ptr->end_pos);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(sp, eof_selected_track);
	return 1;	//Return completion
}

int eof_menu_note_toggle_hi_hat_open(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated;

	if(eof_selected_track != EOF_TRACK_DRUM_PS)
		return 1;	//Do not allow this function to run when the PS drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 6))
			continue;	//If the drum note doesn't contain a yellow gem (or red gem, to allow for notation during disco flips), skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
		{	//If this is a yellow gem
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_CYMBAL,1,0);	//Automatically mark as a cymbal
		}
		if(eof_drum_modifiers_affect_all_difficulties)
		{	//If the user wants to apply this change to notes at this position among all difficulties
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL,0,0);	//Clear the pedal controlled hi hat status
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_SIZZLE,0,0);		//Clear the sizzle hi hat status
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN,2,0);	//Toggle the open hi hat status
		}
		else
		{	//Otherwise just apply it to this difficulty's notes
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Clear the pedal controlled hi hat status
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;			//Clear the sizzle hi hat status
			flags ^= EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;		//Toggle the open hi hat status
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_hi_hat_pedal(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated;

	if(eof_selected_track != EOF_TRACK_DRUM_PS)
		return 1;	//Do not allow this function to run when the PS drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 6))
			continue;	//If the drum note doesn't contain a yellow gem (or red gem, to allow for notation during disco flips), skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
		{	//If this is a yellow gem
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_CYMBAL,1,0);	//Automatically mark as a cymbal
		}
		if(eof_drum_modifiers_affect_all_difficulties)
		{	//If the user wants to apply this change to notes at this position among all difficulties
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN,0,0);	//Clear the open hi hat status
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_SIZZLE,0,0);		//Clear the sizzle hi hat status
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL,2,0);	//Toggle the pedal controlled hi hat status
		}
		else
		{	//Otherwise just apply it to this difficulty's notes
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;	//Clear the open hi hat status
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;		//Clear the sizzle hi hat status
			flags ^= EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Toggle the pedal controlled hi hat status
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_hi_hat_sizzle(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated;

	if(eof_selected_track != EOF_TRACK_DRUM_PS)
		return 1;	//Do not allow this function to run when the PS drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 6))
			continue;	//If the drum note doesn't contain a yellow gem (or red gem, to allow for notation during disco flips), skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
		{	//If this is a yellow gem
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_CYMBAL,1,0);	//Automatically mark as a cymbal
		}
		if(eof_drum_modifiers_affect_all_difficulties)
		{	//If the user wants to apply this change to notes at this position among all difficulties
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL,0,0);	//Clear the pedal controlled hi hat status
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN,0,0);	//Clear the open hi hat status
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_SIZZLE,2,0);		//Toggle the sizzle hi hat status
		}
		else
		{	//Otherwise just apply it to this difficulty's notes
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;		//Clear the open hi hat status
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Clear the pedal controlled hi hat status
			flags ^= EOF_DRUM_NOTE_FLAG_Y_SIZZLE;			//Toggle the sizzle hi hat status
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_hi_hat_status(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 6))
			continue;	//If the drum note doesn't contain a yellow gem (or red gem, to allow for notation during disco flips), skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		if(eof_drum_modifiers_affect_all_difficulties)
		{	//If the user wants to apply this change to notes at this position among all difficulties
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL,0,0);	//Clear the pedal controlled hi hat status
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_SIZZLE,0,0);		//Clear the sizzle hi hat status
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN,0,0);	//Clear the open hi hat status
		}
		else
		{	//Otherwise just apply it to this difficulty's notes
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Clear the pedal controlled hi hat status
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;			//Clear the sizzle hi hat status
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;	//Clear the open hi hat status
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_rimshot(void)
{
	unsigned long i, tracknum;
	long u = 0;
	int note_selection_updated;

	if(eof_selected_track != EOF_TRACK_DRUM_PS)
		return 1;	//Do not allow this function to run when the PS drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 2))
			continue;	//If this drum note doesn't contain a red gem, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		if(eof_drum_modifiers_affect_all_difficulties)
		{	//If the user wants to apply this change to notes at this position among all difficulties
			tracknum = eof_song->track[eof_selected_track]->tracknum;
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_R_RIMSHOT,2,0);	//Toggle the rimshot status
		}
		else
		{
			eof_xor_note_flags(eof_song, eof_selected_track, i, EOF_DRUM_NOTE_FLAG_R_RIMSHOT);	//Apply the flag changes
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_rimshot(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if(!(eof_get_note_note(eof_song, eof_selected_track, i) & 2) || !(eof_get_note_flags(eof_song, eof_selected_track, i) & EOF_DRUM_NOTE_FLAG_R_RIMSHOT))
			continue;	//If the drum note doesn't contain a red gem or doesn't contain rimshot status, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		if(eof_drum_modifiers_affect_all_difficulties)
		{	//If the user wants to apply this change to notes at this position among all difficulties
			eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_R_RIMSHOT,0,0);		//Clear the rimshot status
		}
		else
		{	//Otherwise just apply it to this difficulty's notes
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags &= ~EOF_DRUM_NOTE_FLAG_R_RIMSHOT;			//Clear the rimshot status
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_flam(void)
{
	unsigned long i, flags;
	long u = 0;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run if a drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if(eof_get_note_type(eof_song, eof_selected_track, i) != EOF_NOTE_AMAZING)
			continue;	//If this drum note isn't in the expert difficulty, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		flags ^= EOF_DRUM_NOTE_FLAG_FLAM;	//Toggle the flam status
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_flam(void)
{
	unsigned long i, flags;
	long u = 0;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it
		if(!(eof_get_note_flags(eof_song, eof_selected_track, i) & EOF_DRUM_NOTE_FLAG_FLAM))
			continue;	//If the drum note doesn't contain a note with flam status, skip it

		if(!u)
		{	//Make a back up before changing the first note
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			u = 1;
		}
		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		flags &= ~EOF_DRUM_NOTE_FLAG_FLAM;			//Clear the flam status
		eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_pro_guitar_toggle_hammer_on(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if((eof_count_selected_and_unselected_notes(NULL) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
				continue;	//If the note isn't selected, skip it

			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
			{	//If the note was already a hammer on, remove the status
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;	//Clear the hammer on flag
				flags &= ~EOF_NOTE_FLAG_F_HOPO;			//Clear the legacy HOPO flag
			}
			else
			{	//Otherwise set the hammer on flag
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;	//Set the hammer on flag
				flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO flag
			}
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_pro_guitar_remove_hammer_on(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
			{	//If this note has hammer on status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
				flags &= ~EOF_NOTE_FLAG_F_HOPO;					//Clear the legacy HOPO on flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_pro_guitar_toggle_pull_off(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if((eof_count_selected_and_unselected_notes(NULL) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
				continue;	//If the note isn't selected, skip it

			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PO)
			{	//If the note was already a pull off, remove the status
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;	//Clear the pull off flag
				flags &= ~EOF_NOTE_FLAG_F_HOPO;			//Clear the legacy HOPO flag
			}
			else
			{	//Otherwise set the pull off flag
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;	//Set the pull off flag
				flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO flag
			}
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_pro_guitar_remove_pull_off(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PO)
			{	//If this note has pull off status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
				flags &= ~(EOF_NOTE_FLAG_F_HOPO);			//Clear the legacy HOPO on flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_ghost(void)
{
	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;
	int note_selection_updated;

 	eof_log("eof_menu_note_toggle_ghost() entered", 1);

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar/bass track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->notes; ctr++)
	{	//For each note in the active pro guitar track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr] && (eof_song->pro_guitar_track[tracknum]->note[ctr]->type == eof_note_type))
		{	//If the note is selected and is in the active difficulty
			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask<<=1)
			{	//For each of the 6 usable strings
				if((eof_song->pro_guitar_track[tracknum]->note[ctr]->note & bitmask) && (eof_pro_guitar_fret_bitmask & bitmask))
				{	//If this string is in use, and this string is enabled for fret shortcut manipulation
					if(!undo_made)
					{	//Make an undo state before making the first change
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eof_song->pro_guitar_track[tracknum]->note[ctr]->ghost ^= bitmask;	//Toggle this string's ghost flag
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_ghost(void)
{
	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;
	int note_selection_updated;

 	eof_log("eof_menu_note_remove_ghost() entered", 1);

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar/bass track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->notes; ctr++)
	{	//For each note in the active pro guitar track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr] && (eof_song->pro_guitar_track[tracknum]->note[ctr]->type == eof_note_type))
		{	//If the note is selected and is in the active difficulty
			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask<<=1)
			{	//For each of the 6 usable strings
				if((eof_song->pro_guitar_track[tracknum]->note[ctr]->note & bitmask) && (eof_pro_guitar_fret_bitmask & bitmask) && (eof_song->pro_guitar_track[tracknum]->note[ctr]->ghost & bitmask))
				{	//If this string is in use, and this string is enabled for fret shortcut manipulation and the string is marked as ghosted
					if(!undo_made)
					{	//Make an undo state before making the first change
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eof_song->pro_guitar_track[tracknum]->note[ctr]->ghost &= ~bitmask;	//Clear this string's ghost flag
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_vibrato(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	int note_selection_updated;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song || (!eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (tp->note[i]->type == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
			{	//If this note has vibrato status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;	//Clear the vibrato flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_vibrato(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->pro_guitar_track[tracknum]->note[i]->type == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;	//Toggle the vibrato flag
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_pop(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->pro_guitar_track[tracknum]->note[i]->type == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_POP)
			{	//If this note has pop status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_POP;	//Clear the pop flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_pop(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->pro_guitar_track[tracknum]->note[i]->type == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_POP;	//Toggle the pop flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_POP)
			{	//If pop status was just enabled
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLAP;				//Clear the slap flag
			}
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_slap(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->pro_guitar_track[tracknum]->note[i]->type == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP)
			{	//If this note has slap status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLAP;	//Clear the slap flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_slap(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->pro_guitar_track[tracknum]->note[i]->type == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLAP;	//Toggle the slap flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP)
			{	//If slap status was just enabled
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_POP;				//Clear the pop flag
			}
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_remove_accent(void)
{
	return eof_menu_note_clear_flag(0, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_FLAG_ACCENT);
}

int eof_menu_note_toggle_accent(void)
{
	return eof_menu_note_toggle_flag(0, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_FLAG_ACCENT);
}

int eof_menu_note_remove_pinch_harmonic(void)
{
	return eof_menu_note_clear_flag(0, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC);
}

int eof_menu_note_toggle_pinch_harmonic(void)
{
	return eof_menu_note_toggle_flag(0, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC);
}

int eof_menu_note_remove_rs_sustain(void)
{
	return eof_menu_note_clear_flag(1, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN);
}

int eof_menu_note_toggle_rs_sustain(void)
{
	return eof_menu_note_toggle_flag(1, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN);
}

int eof_menu_note_remove_rs_ignore(void)
{
	return eof_menu_note_clear_flag(1, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_EFLAG_IGNORE);
}

int eof_menu_note_toggle_rs_ignore(void)
{
	return eof_menu_note_toggle_flag(1, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_EFLAG_IGNORE);
}

int eof_menu_note_remove_linknext(void)
{
	int retval = eof_menu_note_clear_flag(0, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT);
	eof_pro_guitar_track_fixup_notes(eof_song, eof_selected_track, 1);
	return retval;
}

int eof_menu_note_toggle_linknext(void)
{
	int retval = eof_menu_note_toggle_flag(0, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT);
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
	return retval;
}

int eof_menu_note_remove_rs_fingerless(void)
{
	return eof_menu_note_clear_flag(1, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS);
}

int eof_menu_note_toggle_rs_fingerless(void)
{
	return eof_menu_note_toggle_flag(1, EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS);
}

int eof_menu_note_highlight_on(void)
{
	(void) eof_menu_note_clear_flag(0, EOF_ANY_TRACK_FORMAT, EOF_NOTE_FLAG_HIGHLIGHT);	//Clear the highlight flag
	(void) eof_menu_note_toggle_flag(0, EOF_ANY_TRACK_FORMAT, EOF_NOTE_FLAG_HIGHLIGHT);	//Toggle the highlight flag, which combined with the above clear, causes selected notes to have this flag set
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update highlighting variables
	return 1;
}

int eof_menu_note_highlight_off(void)
{
	(void) eof_menu_note_clear_flag(0, EOF_ANY_TRACK_FORMAT, EOF_NOTE_FLAG_HIGHLIGHT);	//Clear the highlight flag
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update highlighting variables
	return 1;
}

int eof_menu_note_remove_ch_sp_deploy(void)
{
	unsigned long i;
	char undo_made = 0;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_format != EOF_LEGACY_TRACK_FORMAT)
		return 1;	//Only allow this to run for legacy tracks

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If this note isn't selected, skip it

		eof_set_note_ch_sp_deploy_status(eof_song, eof_selected_track, i, 0, &undo_made);	//Make an undo state if applicable and remove the status
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_toggle_ch_sp_deploy(void)
{
	unsigned long i;
	char undo_made = 0;
	int note_selection_updated;

	if(eof_song->track[eof_selected_track]->track_format != EOF_LEGACY_TRACK_FORMAT)
		return 1;	//Only allow this to run for legacy tracks

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If this note isn't selected, skip it

		eof_set_note_ch_sp_deploy_status(eof_song, eof_selected_track, i, -1, &undo_made);	//Make an undo state if applicable and toggle the status
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_update_implied_note_selection(void)
{
	unsigned long i, count = 0, notepos;

	if(!eof_song)
		return 0;	//Do not run if no chart is loaded

	if(eof_get_selected_note_range(NULL, NULL, 0))
		return 0;	//If any notes in the active track difficulty are explicitly selected, don't alter the selection

	if((eof_song->tags->start_point != ULONG_MAX) && (eof_song->tags->end_point != ULONG_MAX) && (eof_song->tags->start_point != eof_song->tags->end_point))
	{	//If the start and end points are suitably defined
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
			{	//If this note is in the active difficulty
				notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
				if(notepos > eof_song->tags->end_point)
					break;	//If this and all remaining notes are after the end point, stop checking notes
				if(notepos >= eof_song->tags->start_point)
				{	//If this note starts within the start/end point range
					if(!count)
					{
						(void) eof_menu_edit_deselect_all();		//Clear the note selection
						eof_selection.track = eof_selected_track;	//Update the track that the note selection applies to
						eof_selection.current = i;					//Consider the first automatically selected note to be the explicitly selected one for functions that need this
						eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
					}
					eof_selection.multi[i] = 1;	//Add this note to the selection
					count++;	//Track how many notes have been selected
				}
			}
		}
		if(count)
			return 1;	//If any notes were added to the selection, return true
	}

	if((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_seek_hover_note >= 0) && (eof_seek_hover_note < eof_get_track_size(eof_song, eof_selected_track)))
	{	//If Feedback input method is in effect and there is a valid hover note
		(void) eof_menu_edit_deselect_all();		//Clear the note selection
		eof_selection.multi[eof_seek_hover_note] = 1;
		eof_selection.current = eof_seek_hover_note;
		eof_selection.track = eof_selected_track;
		return 1;	//Return true
	}

	return 0;	//The note selection was not altered
}

DIALOG eof_pro_guitar_note_slide_end_fret_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Edit slide end fret",  NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "End slide at fret #",  NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  50,  20,  0,   0,   0,    0,      2,   0,   eof_etext,              "0123456789", NULL },
	{ d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                   NULL, NULL },
	{ d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",               NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                   NULL, NULL }
};

int eof_pro_guitar_note_slide_end_fret(char undo)
{
	unsigned long newend, i, flags;
	int note_selection_updated;
	unsigned long tracknum;
	EOF_PRO_GUITAR_NOTE *np;
	char undo_made = 0, error = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_selection.current >= eof_song->pro_guitar_track[tracknum]->notes)
		return 1;	//Invalid selected note number
	np = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current];

	eof_render();
	eof_color_dialog(eof_pro_guitar_note_slide_end_fret_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_pro_guitar_note_slide_end_fret_dialog);
	if(np->slideend == 0)
	{	//If the selected note has no ending fret defined
		eof_etext[0] = '\0';	//Empty this string
	}
	else
	{	//Otherwise write the ending fret into the string
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%u", np->slideend);
	}

	if(eof_popup_dialog(eof_pro_guitar_note_slide_end_fret_dialog, 2) == 3)
	{	//User clicked OK
		if(eof_etext[0] == '\0')
		{	//If the user did not define the ending fret number
			newend = 0;
		}
		else
		{
			newend = atol(eof_etext);
			if(newend > eof_song->pro_guitar_track[tracknum]->numfrets)
			{	//If this fret value is higher than the track supports
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				eof_show_mouse(NULL);
				allegro_message("Error:  The fret number specified is higher than the max fret for this track");
				return 1;	//Return error
			}
		}

		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track) && !error; i++)
		{	//For each note in the active track, unless an error was encountered above
			unsigned char lowestfret;

			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
				continue;	//If the note isn't selected, skip it
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && !(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
				continue;	//If this note isn't a slide, skip it

			lowestfret = eof_pro_guitar_note_lowest_fret(eof_song->pro_guitar_track[tracknum], i);	//Determine the lowest used fret value
			if(lowestfret && newend)
			{	//If a fret value was used, and an ending fret was defined, validate the slide ending fret
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
				{	//If this note is an upward slide, the ending fret must be higher than the note's lowest fret
					if(newend <= lowestfret)
					{
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						eof_show_mouse(NULL);
						allegro_message("Error:  The fret number specified must be higher than the lowest fret on upward slide notes");
						break;	//Stop processing selected notes
					}
				}
				else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
				{	//If this note is a downward slide, the ending fret must be lower than the note's lowest fret
					if(newend >= lowestfret)
					{
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						eof_show_mouse(NULL);
						allegro_message("Error:  The fret number specified must be lower than the lowest fret on downward slide notes");
						break;	//Stop processing selected notes
					}
				}
			}
			if(!eof_song->pro_guitar_track[tracknum]->note[i]->slideend || (newend != eof_song->pro_guitar_track[tracknum]->note[i]->slideend))
			{	//If the slide end position wasn't already defined, or the specified end position is different than what the note already had
				if(undo && !undo_made)
				{	//Make a back up before changing the first note (but only if the calling function specified to create an undo state)
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_song->pro_guitar_track[tracknum]->note[i]->slideend = newend;
				if(newend)
				{	//If the ending fret is nonzero, it is now defined
					eof_song->pro_guitar_track[tracknum]->note[i]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Set this flag to indicate that the slide's ending fret is defined
				}
				else
				{	//Otherwise it is now undefined
					if(!((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength))
					{	//If this is NOT also a bend note with a defined bend strength
						eof_song->pro_guitar_track[tracknum]->note[i]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag to indicate that the slide's ending fret is undefined and the note also has no bend strength
					}
				}
			}
			else if(newend)
			{	//Or as long as a valid end of slide position was otherwise provided
				allegro_message("Error:  The fret number specified must be different from the selected note's lowest used fret");
			}
		}//For each note in the active track
	}//User clicked OK

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_pro_guitar_note_slide_end_fret_save(void)
{
	return eof_pro_guitar_note_slide_end_fret(1);	//Call the function and allow it to save before making changes
}

int eof_pro_guitar_note_slide_end_fret_no_save(void)
{
	return eof_pro_guitar_note_slide_end_fret(0);	//Call the function and do NOT allow it to save before making changes
}

DIALOG eof_pro_guitar_note_define_unpitched_slide_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                        (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Unpitched slide end fret", NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "End slide at fret #",      NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  50,  20,  0,   0,   0,    0,      2,   0,   eof_etext,                  "0123456789", NULL },
	{ d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                       NULL, NULL },
	{ d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                   NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                       NULL, NULL }
};

int eof_pro_guitar_note_define_unpitched_slide(void)
{
	unsigned long newend, i, flags;
	int note_selection_updated;
	unsigned long tracknum;
	EOF_PRO_GUITAR_NOTE *np;
	char undo_made = 0, error = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_selection.current >= eof_song->pro_guitar_track[tracknum]->notes)
		return 1;	//Invalid selected note number
	np = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current];

	eof_render();
	eof_color_dialog(eof_pro_guitar_note_define_unpitched_slide_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_pro_guitar_note_define_unpitched_slide_dialog);
	if(np->unpitchend == 0)
	{	//If the selected note has no unpitched slide ending fret defined
		eof_etext[0] = '\0';	//Empty this string
	}
	else
	{	//Otherwise write the ending fret into the string
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%u", np->unpitchend);
	}

	if(eof_popup_dialog(eof_pro_guitar_note_define_unpitched_slide_dialog, 2) == 3)
	{	//User clicked OK
		if(eof_etext[0] == '\0')
		{	//If the user did not define the ending fret number
			newend = 0;
		}
		else
		{
			newend = atol(eof_etext);
			if(newend > eof_song->pro_guitar_track[tracknum]->numfrets)
			{	//If this fret value is higher than the track supports
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				eof_show_mouse(NULL);
				allegro_message("Error:  The fret number specified is higher than the max fret for this track");
				error = 1;
			}
		}

		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track) && !error; i++)
		{	//For each note in the active track, unless an error was encountered above
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
				continue;	//If the note isn't selected, skip it

			flags = eof_song->pro_guitar_track[tracknum]->note[i]->flags;
			if(newend)
			{	//If a slide end value was given
				unsigned char lowestfret = eof_pro_guitar_note_lowest_fret(eof_song->pro_guitar_track[tracknum], i);	//Determine the lowest used fret value

				if(lowestfret)
				{	//If the note has a fretted string, and an ending fret was defined, validate the unpitched slide ending fret
					if(newend == lowestfret)
					{
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						eof_show_mouse(NULL);
						allegro_message("Error:  The fret number specified must be higher or lower than the note's lowest used fret");
						break;	//Stop processing selected notes
					}
				}
				if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE) || (newend != eof_song->pro_guitar_track[tracknum]->note[i]->unpitchend))
				{	//If the note wasn't already an unpitched slide, or the specified slide ending is different than what the note already has
					if(!undo_made)
					{	//Make an undo state before changing the first note
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->unpitchend = newend;
					eof_song->pro_guitar_track[tracknum]->note[i]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;	//Enable the status flag
				}
			}
			else
			{	//No slide end value was given, remove the status
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
				{	//If this note has unpitched slide status
					if(!undo_made)
					{	//Make an undo state before changing the first note
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->unpitchend = 0;
					eof_song->pro_guitar_track[tracknum]->note[i]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;	//Clear the status flag
				}
			}
		}//For each note in the active track
	}//User clicked OK

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_note_remove_unpitched_slide(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;							//Save an extra copy of the original flags
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE);	//Clear the unpitched slide flag
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->note[i]->unpitchend = 0;	//Reset the ending fret number of the slide
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

DIALOG eof_pro_guitar_note_bend_strength_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                          (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Edit bend strength", NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  20,  20,  0,   0,   0,    0,      1,   0,   eof_etext,     "0123456789",NULL },
	{d_agup_radio_proc,		 40,  40,  82,  16,  2,   23,  0,    0,      1,   0,   "half steps",         NULL, NULL },
	{d_agup_radio_proc,		 40,  60,  100, 16,  2,   23,  0,    0,      1,   0,   "quarter steps",      NULL, NULL },
	{ d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                 NULL, NULL },
	{ d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",             NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                 NULL, NULL }
};

int eof_pro_guitar_note_bend_strength(char undo)
{
	unsigned long newstrength, i, flags;
	int note_selection_updated;
	unsigned long tracknum;
	EOF_PRO_GUITAR_TRACK *tp;
	EOF_PRO_GUITAR_NOTE *np;
	char undo_made = 0;
	static unsigned long lastselected = 2;	//Track the last selected option (half steps or quarter steps)

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_selection.current >= eof_song->pro_guitar_track[tracknum]->notes)
		return 1;	//Invalid selected note number
	tp = eof_song->pro_guitar_track[tracknum];
	np = tp->note[eof_selection.current];

	eof_render();
	eof_color_dialog(eof_pro_guitar_note_bend_strength_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_pro_guitar_note_bend_strength_dialog);
	eof_pro_guitar_note_bend_strength_dialog[2].flags = 0;	//Clear the half steps radio button
	eof_pro_guitar_note_bend_strength_dialog[3].flags = 0;	//Clear the quarter steps radio button
	if((np->bendstrength & 0x7F) == 0)
	{	//If the selected note has no bend strength defined, or otherwise a value of 0
		if(tp->note != tp->technote)
		{	//Normal notes are not allowed to have a bend strength of 0
			eof_etext[0] = '\0';	//Otherwise it's not valid, empty this string
		}
		else
		{	//If tech view is in effect, bend status is allowed to have a strength of 0 (defines the release of a bend)
			if(!(np->flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
			{	//If the note's flags indicate the bend strength is not defined
				eof_etext[0] = '\0';	//Empty this string
			}
		}
		eof_pro_guitar_note_bend_strength_dialog[lastselected].flags = D_SELECTED;
	}
	else
	{	//Otherwise write the ending fret into the string
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", (np->bendstrength & 0x7F));	//Mask out the MSB, which specifies the bend strength's unit of measurement
		if(np->bendstrength & 0x80)
		{	//If the note's current bend strength specifies the value is in quarter steps
			eof_pro_guitar_note_bend_strength_dialog[3].flags = D_SELECTED;	//Select the quarter steps radio button
		}
		else
		{	//The bend strength specifies the value in half steps (the default)
			eof_pro_guitar_note_bend_strength_dialog[2].flags = D_SELECTED;	//Select the half steps radio button
		}
	}

	if(eof_popup_dialog(eof_pro_guitar_note_bend_strength_dialog, 1) == 4)
	{	//User clicked OK
		if(eof_etext[0] == '\0')
		{	//If the user did not define the bend strength
			newstrength = 0;
		}
		else
		{
			newstrength = atol(eof_etext);
			if(newstrength && (eof_pro_guitar_note_bend_strength_dialog[3].flags == D_SELECTED))
			{	//If the user gave a nonzero bend strength and selected the quarter steps radio button
				newstrength |= 0x80;	//Set the MSB to track this option
			}
		}

		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
				continue;	//If the note isn't selected, skip it

			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if((newstrength == eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength) &&
			((tp->note != tp->technote) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION)))
				continue;	//If the bend strength isn't different than what it already had, and this isn't a tech note or is a tech note that didn't have a bend strength defined (necessary check to allow a value of 0 to set the RS notation flag), skip it

			if(undo && !undo_made)
			{	//Make a back up before changing the first note (but only if the calling function specified to create an undo state)
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				undo_made = 1;
			}
			eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength = newstrength;
			if(newstrength || (tp->note == tp->technote))
			{	//If the bend strength is nonzero, or it is zero and tech view is in effect, the bend strength is now defined
				eof_song->pro_guitar_track[tracknum]->note[i]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Set this flag to indicate that the bend's strength is defined
				eof_song->pro_guitar_track[tracknum]->note[i]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;			//If the note wasn't a bend note before, it is now
			}
			else
			{	//Otherwise it is now undefined
				if(!(((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)) && eof_song->pro_guitar_track[tracknum]->note[i]->slideend))
				{	//If this is NOT also a slide note with a defined end of slide position
					eof_song->pro_guitar_track[tracknum]->note[i]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag to indicate that the bend's strength is undefined and the note also has no end of slide position
				}
			}
		}//For each note in the active track
	}//User clicked OK

	if(eof_pro_guitar_note_bend_strength_dialog[2].flags == D_SELECTED)
	{	//If the half steps radio button is selected
		lastselected = 2;
	}
	else
	{	//The quarter steps radio button is selected
		lastselected = 3;
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_pro_guitar_note_bend_strength_save(void)
{
	return eof_pro_guitar_note_bend_strength(1);
}

int eof_pro_guitar_note_bend_strength_no_save(void)
{
	return eof_pro_guitar_note_bend_strength(0);
}

int eof_menu_pro_guitar_remove_fingering(void)
{
	unsigned long i;
	long u = 0;
	unsigned long tracknum;
	char empty_array[8] = {0};
	EOF_PRO_GUITAR_TRACK *tp;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	for(i = 0; i < tp->notes; i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(memcmp(tp->note[i]->finger, empty_array, 8))
			{	//If the finger array isn't completely undefined
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				memset(tp->note[i]->finger, 0, 8);	//Initialize all fingers to undefined
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_pro_guitar_apply_derived_fingering(void)
{
	unsigned long notenum, stringnum;
	long u = 0;
	unsigned long tracknum;
	EOF_PRO_GUITAR_TRACK *tp;
	int note_selection_updated;
	unsigned char fingernum = 0;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	for(notenum = 0; notenum < tp->notes; notenum++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[notenum])
		{	//If this note is in the currently active track and is selected
			for(stringnum = 0; stringnum < 6; stringnum++)
			{	//For each of the 6 usable strings
				int retval = eof_pro_guitar_note_derive_string_fingering(eof_song, eof_selected_track, notenum, stringnum, &fingernum);	//Determine what the fingering for this note is, deriving it from handshape/arpeggio/FHP if possible

				if(retval > 1)
				{	//If the fingering for this gem is not defined but can be derived
					if(!u)
					{	//Make a back up before changing the first note
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						u = 1;
					}
					tp->note[notenum]->finger[stringnum] = fingernum;	//Apply the derived fingering
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_note_menu_read_gp_lyric_texts(void)
{
	unsigned long ctr, tracknum, linestart, lastlyricend = 0;
	char * returnedfn = NULL;
	PACKFILE *fp;
	EOF_VOCAL_TRACK *tp;
	int character;
	char buffer[101];
	unsigned long index;
	char undo_made = 0, last_text_entry_was_linebreak = 0, firstlinephrasecreated = 0, last_text_entry_was_hyphen = 0;

	if(eof_song == NULL)	//Do not import lyric text if no chart is open
		return 0;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_filename, "Import GP format lyric text file", eof_filter_gp_lyric_text_files);
	eof_clear_input();
	if(!returnedfn)
		return 0;	//User didn't select a file

	fp = pack_fopen(returnedfn, "rt");
	if(!fp)
	{
		allegro_message("Couldn't open file:  \"%s\"", strerror(errno));
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input .eof file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;
	}

	tracknum = eof_song->track[EOF_TRACK_VOCALS]->tracknum;
	tp = eof_song->vocal_track[tracknum];
	if(tp->lyrics == 0)
		return 0;	//No vocal notes defined
	linestart = tp->lyric[0]->pos;	//Track the position of the first lyric (for adding line phrases)
	for(ctr = 0; ctr < tp->lyrics; ctr++)
	{	//For each lyric in the vocal track
		index = 0;	//Point index to start of lyric text buffer

		lastlyricend = tp->lyric[ctr]->pos + tp->lyric[ctr]->length;	//Track the end position of this lyric (for adding line phrases)
		//Read next lyric text entry
		while(1)
		{
			character = pack_getc(fp);	//Read next character
			if(character == EOF)
			{	//End of file was reached
				break;
			}

			if(character == '\r')	//If this is the first character in a newline sequence (carriage return)
			{
				if(pack_getc(fp) != '\n')
				{	//If it was followed by the rest of the sequence (newline)
					allegro_message("Error:  Malformed text file?");
					(void) pack_fclose(fp);
					return 0;
				}
				if(!last_text_entry_was_linebreak)
				{	//If the last text was not a linebreak, this one counts as a new lyric text entry
					if(index == 0)
					{	//If this was the first character read for this lyric, it is interpreted as a pitch shift for the previous lyric text
						buffer[index++] = '+';
					}

					if(!firstlinephrasecreated)
					{	//If this if the first lyric line being added by this function
						tp->lines = 0;	//Erase any existing lyric lines in the track
						firstlinephrasecreated = 1;
					}
					(void) eof_vocal_track_add_line(tp, linestart, lastlyricend, 0xFF);	//Add a line phrase
					if(ctr + 1 < tp->lyrics)
					{	//If there's at least one more lyric
						linestart = tp->lyric[ctr + 1]->pos;	//Track its start position
					}
					lastlyricend = 0;	//Reset this variable to track that the line has ended

					last_text_entry_was_linebreak = 1;
					break;	//A newline ends the current lyric text
				}
			}
			else
			{	//This lyric text entry is not a line break
				last_text_entry_was_linebreak = 0;	//Reset this status
			}

			if(character == ' ')
			{	//This character ends the current lyric text
				if(!last_text_entry_was_hyphen)
				{	//Ignore the space isn't immediately after a hyphen
					break;	//It ends the current lyric text
				}
			}

			if(character == '-')
			{	//If this is a hyphen
				if(index == 0)
				{	//If this was the first character read for this lyric, it is interpreted as a pitch shift for the previous lyric text
					buffer[index++] = '+';
				}
				else if(index < 100)
				{	//Otherwise as long as it will fit in the buffer, it is included in the text read for this lyric so far and ends the current lyric text
					buffer[index++] = '-';
				}

				last_text_entry_was_hyphen = 1;
				break;
			}
			else
			{	//This text entry was not a hyphen
				last_text_entry_was_hyphen = 0;	//Reset this status
			}

			if(index < 100)
			{	//If this character will fit in the buffer
				buffer[index++] = character;	//Append it to the buffer
			}
		}
		buffer[index] = '\0';	//Terminate buffer

		if(!undo_made)
		{	//If an undo state hasn't been made yet
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		eof_set_note_name(eof_song, EOF_TRACK_VOCALS, ctr, buffer);	//Update the text of the lyric in the chart
	}

	if(lastlyricend)
	{	//If there was a lyric line in progress
		(void) eof_vocal_track_add_line(tp, linestart, lastlyricend, 0xFF);	//Add a line phrase
	}

	(void) pack_fclose(fp);
	return 1;
}

int eof_menu_remove_statuses(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, eflags;
	int note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected, skip it

		flags = eof_get_note_flags(eof_song, eof_selected_track, i);
		eflags = eof_get_note_eflags(eof_song, eof_selected_track, i);
		if(flags || eflags)
		{	//If this note has any statuses
			if(!((eof_track_is_pro_guitar_track(eof_song, eof_selected_track)) && (flags == EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)))
			{	//Don't bother clearing the note status if the only status it has is string mute, since the fixup routine will just re-apply it automatically
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				eof_set_note_flags(eof_song, eof_selected_track, i, 0);		//Erase all flags for this note
				eof_set_note_eflags(eof_song, eof_selected_track, i, 0);	//Erase all extended flags for this note
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_rocksmith_convert_mute_to_palm_mute_single_note(void)
{
	unsigned long i, ctr, bitmask;
	EOF_PRO_GUITAR_TRACK *tp;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	char first_string;
	int note_selection_updated;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If this note isn't selected, skip it

		first_string = 0;	//Reset this status
		for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
		{	//For each of the 6 supported strings
			if(!(tp->note[i]->note & bitmask))
				continue;	//If this string is not used, skip it

			if(tp->note[i]->frets[ctr] & 0x80)
			{	//If this string is marked as string muted
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				if(tp->note[i]->frets[ctr] == 0xFF)
				{	//If this gem has no fret value
					tp->note[i]->frets[ctr] = 0;	//Set to 0
				}
				else
				{	//Otherwise just remove the string muting
					tp->note[i]->frets[ctr] &= ~0x80;	//Clear the MSB to remove the string mute status
				}
				tp->note[i]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;	//Clear the string mute status
				tp->note[i]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;		//Set the palm mute status
			}
			if(!(tp->note[i]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
				continue;	//If the note wasn't already a palm mute or marked as one, skip it

			if(first_string)
			{	//If this isn't the first string in the note that had a gem
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				tp->note[i]->note &= ~bitmask;	//Erase the gem
				tp->note[i]->ghost &= ~bitmask;	//Erase the ghost flag for this gem
				tp->note[i]->frets[ctr] = 0;	//Clear the fret number for this string
				tp->note[i]->finger[ctr] = 0;	//Clear the fingering for this string
			}
			first_string = 1;
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

void eof_menu_note_cycle_selection_back(unsigned long notenum)
{
	unsigned long ctr;

	if(notenum >= EOF_MAX_NOTES)
		return;	//Invalid parameter

	for(ctr = notenum; ctr + 1 < EOF_MAX_NOTES; ctr++)
	{	//For each entry in the selection array, up to the second to last one
		eof_selection.multi[ctr] = eof_selection.multi[ctr + 1];	//This remaining note will cycled back one in number, so it needs to retain its selection status
	}
	eof_selection.multi[ctr] = 0;	//This last note, if it existed, will no longer exist so it can be deselected

}

int eof_menu_note_simplify_chords(void)
{
	unsigned long ctr, ctr2, bitmask, note;
	unsigned char undo_made = 0;
	int note_selection_updated;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type))
			continue;	//If the note is not selected, skip it
		if(eof_note_count_colors(eof_song, eof_selected_track, ctr) <= 1)
			continue;	//If the note doesn't have at least two gems, skip it

		note = eof_get_note_note(eof_song, eof_selected_track, ctr);	//Get the note's bitmask
		for(ctr2 = 6, bitmask = 32; ctr2 > 0; ctr2--, bitmask >>= 1)
		{	//For each of the 6 supported strings (in reverse order)
			if(note & bitmask)
			{	//When the first (highest numbered) populated lane is reached
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				note &= ~ bitmask;	//Clear that lane
				eof_set_note_note(eof_song, eof_selected_track, ctr, note);	//Update the note
				break;
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_simplify_chords_low(void)
{
	unsigned long ctr, ctr2, bitmask, note;
	unsigned char undo_made = 0;
	int note_selection_updated;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type))
			continue;	//If the note is not selected, skip it
		if(eof_note_count_colors(eof_song, eof_selected_track, ctr) <= 1)
			continue;	//If the note doesn't have at least two gems, skip it

		note = eof_get_note_note(eof_song, eof_selected_track, ctr);	//Get the note's bitmask
		for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
		{	//For each of the 6 supported strings
			if(note & bitmask)
			{	//When the first (lowest numbered) populated lane is reached
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				note &= ~ bitmask;	//Clear that lane
				eof_set_note_note(eof_song, eof_selected_track, ctr, note);	//Update the note
				break;
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_simplify_cymbals(void)
{
	unsigned long ctr, note, newnote, flags, newflags;
	unsigned char undo_made = 0;
	int note_selection_updated;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 0;	//Do not allow this function to run when not in a drum track

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type))
			continue;	//If the note is not selected, skip it

		note = newnote = eof_get_note_note(eof_song, eof_selected_track, ctr);	//Get the note's bitmask
		flags = newflags = eof_get_note_flags(eof_song, eof_selected_track, ctr);	//Get the note's flags
		if((note & 4) && (flags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
		{	//If the note contains a yellow cymbal
			newnote &= ~4;	//Clear this gem
			newflags &= ~EOF_DRUM_NOTE_FLAG_Y_CYMBAL;	//Clear this flag
		}
		if((note & 8) && (flags & EOF_DRUM_NOTE_FLAG_B_CYMBAL))
		{	//If the note contains a blue cymbal
			newnote &= ~8;	//Clear this gem
			newflags &= ~EOF_DRUM_NOTE_FLAG_B_CYMBAL;	//Clear this flag
		}
		if((note & 16) && (flags & EOF_DRUM_NOTE_FLAG_G_CYMBAL))
		{	//If the note contains a green cymbal
			newnote &= ~16;	//Clear this gem
			newflags &= ~EOF_DRUM_NOTE_FLAG_G_CYMBAL;	//Clear this flag
		}
		if(newnote != note)
		{	//If the note is being altered
			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			eof_set_note_note(eof_song, eof_selected_track, ctr, newnote);	//Update the note
			eof_set_note_flags(eof_song, eof_selected_track, ctr, newflags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	if(undo_made)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	return 1;
}

int eof_menu_note_simplify_toms(void)
{
	unsigned long ctr, note, newnote, flags, newflags;
	unsigned char undo_made = 0;
	int note_selection_updated;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 0;	//Do not allow this function to run when not in a drum track

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type))
			continue;	//If the note is not selected, skip it

		note = newnote = eof_get_note_note(eof_song, eof_selected_track, ctr);	//Get the note's bitmask
		flags = newflags = eof_get_note_flags(eof_song, eof_selected_track, ctr);	//Get the note's flags
		if((note & 4) && !(flags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
		{	//If the note contains a yellow tom
			newnote &= ~4;	//Clear this gem
		}
		if((note & 8) && !(flags & EOF_DRUM_NOTE_FLAG_B_CYMBAL))
		{	//If the note contains a blue tom
			newnote &= ~8;	//Clear this gem
		}
		if((note & 16) && !(flags & EOF_DRUM_NOTE_FLAG_G_CYMBAL))
		{	//If the note contains a green tom
			newnote &= ~16;	//Clear this gem
		}
		if(newnote != note)
		{	//If the note is being altered
			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			eof_set_note_note(eof_song, eof_selected_track, ctr, newnote);	//Update the note
			eof_set_note_flags(eof_song, eof_selected_track, ctr, newflags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	if(undo_made)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	return 1;
}

int eof_menu_note_simplify_double_bass_drum(void)
{
	unsigned long ctr, note, newnote, flags, newflags;
	unsigned char undo_made = 0, type;
	int note_selection_updated;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 0;	//Do not allow this function to run when not in a drum track

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		type = eof_get_note_type(eof_song, eof_selected_track, ctr);
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (type != eof_note_type))
			continue;	//If the note is not selected, skip it
		if(type != EOF_NOTE_AMAZING)
			continue;	//if the note is not in the expert difficulty, skip it

		note = newnote = eof_get_note_note(eof_song, eof_selected_track, ctr);	//Get the note's bitmask
		flags = newflags = eof_get_note_flags(eof_song, eof_selected_track, ctr);	//Get the note's flags
		if((note & 1) && (flags & EOF_DRUM_NOTE_FLAG_DBASS))
		{	//If the note contains an Expert+ bass drum note
			newnote &= ~1;	//Clear this gem
			newflags &= ~EOF_DRUM_NOTE_FLAG_DBASS;	//Clear this flag
		}
		if(newnote != note)
		{	//If the note is being altered
			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			eof_set_note_note(eof_song, eof_selected_track, ctr, newnote);	//Update the note
			eof_set_note_flags(eof_song, eof_selected_track, ctr, newflags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	if(undo_made)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	return 1;
}

int eof_menu_note_simplify_bass_drum(void)
{
	unsigned long ctr, note;
	unsigned char undo_made = 0, type;
	int note_selection_updated;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 0;	//Do not allow this function to run when not in a drum track

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		type = eof_get_note_type(eof_song, eof_selected_track, ctr);
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (type != eof_note_type))
			continue;	//If the note is not selected, skip it

		note = eof_get_note_note(eof_song, eof_selected_track, ctr);	//Get the note's bitmask
		if(note & 1)
		{	//If the note contains a bass drum note
			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			note &= ~1;	//Clear the bass drum gem
			eof_set_note_note(eof_song, eof_selected_track, ctr, note);	//Update the note
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	if(undo_made)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	return 1;
}

int eof_menu_note_simplify_string_mute(void)
{
	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	int note_selection_updated;
	EOF_PRO_GUITAR_NOTE *np;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the active track
		if(!eof_selection.multi[ctr])
			continue;	//If the note is not selected, skip it

		np = eof_song->pro_guitar_track[tracknum]->note[ctr];
		for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
		{	//For each of the 6 supported strings
			if(np->note & bitmask)
			{	//If the string is in use
				if(np->frets[ctr2] & 0x80)
				{	//If the MSB is set, it denotes string muted status
					if(!undo_made)
					{	//If an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
						undo_made = 1;
					}

					np->note &= ~bitmask;		//Remove this gem
					np->frets[ctr2] = 0;
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	if(undo_made)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	return 1;
}

int eof_menu_note_simplify_ghost(void)
{
	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;
	int note_selection_updated;
	EOF_PRO_GUITAR_NOTE *np;

 	eof_log("eof_menu_note_toggle_ghost() entered", 1);

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar/bass track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->notes; ctr++)
	{	//For each note in the active pro guitar track
		np = eof_song->pro_guitar_track[tracknum]->note[ctr];	//Simplify
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr] && (np->type == eof_note_type))
		{	//If the note is selected and is in the active difficulty
			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask<<=1)
			{	//For each of the 6 usable strings
				if((np->note & bitmask) && (np->ghost & bitmask))
				{	//If this string is in use and is a ghost gem
					if(!undo_made)
					{	//Make an undo state before making the first change
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					np->note &= ~bitmask;	//Clear this gem
					np->ghost &= ~bitmask;
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	if(undo_made)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	return 1;
}

int eof_menu_pro_guitar_toggle_string_mute(void)
{
	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	int note_selection_updated;
	EOF_PRO_GUITAR_NOTE *np;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if((eof_count_selected_and_unselected_notes(NULL) > 0))
	{
		for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
		{	//For each note in the active track
			if(!eof_selection.multi[ctr])
				continue;	//If the note is not selected, skip it

			np = eof_song->pro_guitar_track[tracknum]->note[ctr];
			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			if(eof_is_string_muted(eof_song, eof_selected_track, ctr))
			{	//If the note is already completely string muted, clear the mute status (MSB) from each gem
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if(np->note & bitmask)
					{	//If the string is in use
						if(np->frets[ctr2] == 0xFF)
						{	//If the fret value is undefined for this note
							np->frets[ctr2] = 0;	//Replace with a value of 0
						}
						else
						{	//Otherwise just clear the MSB to leave the existing fret value in place
							np->frets[ctr2] &= 0x7F;
						}
					}
				}
				np->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;	//Clear this status flag
			}
			else
			{	//At least one gem in the note isn't string muted, set the mute status (MSB) on each gem
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if(np->note & bitmask)
					{	//If the string is in use
						np->frets[ctr2] |= 0x80;	//Set the MSB
					}
				}
				np->flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;	//Set this status flag
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_reflect(char function)
{
	int note_selection_updated;
	unsigned long i, ctr, bitmask1, bitmask2, index1 = 0, index2 = 0, numlanes, tracknum, pos1, pos2, next, previous;
	unsigned char note, newnote, newlegacy, ghost, newghost, accent, newaccent, newfrets[6], newfinger[6], first_found = 0;
	char undo_made = 0;
	EOF_PRO_GUITAR_NOTE *np = NULL;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if((function & 1) && (eof_song->track[eof_selected_track]->track_format != EOF_VOCAL_TRACK_FORMAT))
	{	//Perform vertical reflect (unless a vocal track is active)
		numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
				continue;	//If the note is not selected, skip it

			newnote = newlegacy = newghost = newaccent = 0;	//Reset these
			(void) memset(newfrets, 0, sizeof(newfrets));	//Clear these arrays
			(void) memset(newfinger, 0, sizeof(newfinger));
			index2 = numlanes - 1;	//This is the highest lane's array index (for pro guitar note fret and finger arrays)
			bitmask2 = 1 << (numlanes - 1);	//This is the bit corresponding to the note mask's highest lane
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			ghost = eof_get_note_ghost(eof_song, eof_selected_track, i);
			accent = eof_get_note_accent(eof_song, eof_selected_track, i);
			for(ctr = 0, bitmask1 = 1; ctr <= numlanes / 2; ctr++, bitmask1 <<= 1, bitmask2 >>= 1, index2--)
			{	//For the lower half of the lanes in this track and the center lane
				if((numlanes % 2) && (ctr == (numlanes / 2)))
				{	//If this is the center lane, leave it as-is
					newnote |= (note & bitmask1);
					newghost |= (ghost & bitmask1);
					newaccent |= (accent & bitmask1);
				}
				else
				{	//Otherwise reflect it
					if(note & bitmask1)
					{	//If this lane is in use
						if(!undo_made)
						{	//If an undo hasn't been made
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						newnote |= bitmask2;	//The bit on the other side of the reflection is set
						if(ghost & bitmask1)
						{	//If this lane is ghosted
							newghost |= bitmask2;	//The bit on the other side of the reflection is set for the ghost bitmask
						}
						if(accent & bitmask1)
						{	//If this lane has accent status
							newaccent |= bitmask2;	//The bit on the other side of the reflection is set for the accent bitmask
						}
					}
					if(note & bitmask2)
					{	//If the lane opposite the reflection is set
						if(!undo_made)
						{	//If an undo hasn't been made
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						newnote |= bitmask1;	//The bit on this side of the reflection is set
						if(ghost & bitmask2)
						{	//If this lane opposite the reflection is set in the ghost bitmask
							newghost |= bitmask1;	//The bit on the other side of the reflection is set for the ghost bitmask
						}
						if(accent & bitmask2)
						{	//If this lane opposite the reflection is set in the accent bitmask
							newaccent |= bitmask1;	//The bit on the other side of the reflection is set for the accent bitmask
						}
					}
				}

				if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
					continue;	//If the track is not a pro guitar track, skip the rest of this loop's logic

				np = eof_song->pro_guitar_track[tracknum]->note[i];

				if((numlanes % 2) && (ctr == (numlanes / 2)))
				{	//If this is the center lane, leave it as-is
					newlegacy |= (np->legacymask & bitmask1);
					newfrets[ctr] = np->frets[ctr];
					newfinger[ctr] = np->finger[ctr];
				}
				else
				{	//Otherwise reflect it
					if(note & bitmask1)
					{	//If this lane is in use
						if(np->legacymask & bitmask1)
						{	//If this lane is set in the legacy bitmask
							newlegacy |= bitmask2;	//The bit on the other side of the reflection is set for the legacy bitmask
						}
						newfrets[index2] = np->frets[ctr];			//The fret value on the other side of the reflection is updated
						newfinger[index2] = np->finger[ctr];		//The finger value on the other side of the reflection is updated
					}
					if(note & bitmask2)
					{	//If the lane opposite the reflection is set
						if(np->legacymask & bitmask2)
						{	//If the lane opposite the reflection is set in the legacy bitmask
							newlegacy |= bitmask1;	//The bit on the other side of the reflection is set for the legacy bitmask
						}
						newfrets[ctr] = np->frets[index2];			//The fret value on this side of the reflection is updated
						newfinger[ctr] = np->finger[index2];		//The finger value on this side of the reflection is updated
					}
				}//Otherwise reflect it
			}//For the lower half of the lanes in this track and the center lane

			eof_set_note_note(eof_song, eof_selected_track, i, newnote);		//Update the note bitmask
			eof_set_note_ghost(eof_song, eof_selected_track, i, newghost);		//Update the ghost bitmask
			eof_set_note_accent(eof_song, eof_selected_track, i, newaccent);	//Update the accent bitmask
			if(np && (eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
			{	//If the track is a pro guitar track
				np->legacymask = newlegacy;	//Update the legacy bitmask
				(void) memcpy(np->frets, newfrets, sizeof(newfrets));		//Update the fret array
				(void) memcpy(np->finger, newfinger, sizeof(newfinger));	//Update the finger array
			}
		}//For each note in the active track
	}//Perform vertical reflect (unless a vocal track is active)

	if(function & 2)
	{	//Perform horizontal reflect
		//Determine the first and last selected note
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
			{	//If the note is in the active instrument difficulty and is selected
				if(!first_found)
				{	//If this is the first selected note found so far
					index1 = i;	//Track this as the first selected note
					first_found = 1;
				}
				index2 = i;	//Track this as the last selected note found so far
			}
		}
		if(first_found)
		{	//If at least one note is selected
			while(index1 < index2)
			{	//Until the notes on each side of the reflection (the middle selected note) have been swapped
				if(!undo_made)
				{	//If an undo hasn't been made
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}

				pos1 = eof_get_note_pos(eof_song, eof_selected_track, index1);	//Store the position of the note on the left side of the reflection
				pos2 = eof_get_note_pos(eof_song, eof_selected_track, index2);	//Store the position of the note on the right side of the reflection
				next = eof_track_fixup_next_note(eof_song, eof_selected_track, index1);	//Store the positions of the next and previous selected notes on each side of the reflection
				previous = eof_track_fixup_previous_note(eof_song, eof_selected_track, index2);
				eof_set_note_pos(eof_song, eof_selected_track, index1, pos2);	//Swap the two notes' positions
				eof_set_note_pos(eof_song, eof_selected_track, index2, pos1);
				index1 = next;	//Move one note closer to the center of the reflection
				index2 = previous;
			}
			eof_track_sort_notes(eof_song, eof_selected_track);
		}
	}//Perform horizontal reflect

	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_reflect_vertical(void)
{
	return eof_menu_note_reflect(1);
}

int eof_menu_note_reflect_horizontal(void)
{
	return eof_menu_note_reflect(2);
}

int eof_menu_note_reflect_both(void)
{
	return eof_menu_note_reflect(3);
}

int eof_menu_note_move_tech_note_to_previous_note_pos(void)
{
	unsigned long ctr, ctr2, tracknum;
	int note_selection_updated, u = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	EOF_PRO_GUITAR_NOTE *np, *tnp;

	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(tp->note != tp->technote)
		return 1;	//Do not allow this function to run when tech view is not in effect

	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each tech note in the active track
		tnp = tp->note[ctr];	//Simplify
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (tnp->type != eof_note_type))
			continue;	//If this note is not selected, skip it

		for(ctr2 = tp->pgnotes; ctr2 > 0; ctr2--)
		{	//For each normal note in the active track, in reverse order
			np = tp->pgnote[ctr2 - 1];	//Simplify
			if((np->type == tnp->type) && (np->pos < tnp->pos))
			{	//If this is the first normal note in the same track difficulty that is before the tech note
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				tnp->pos = np->pos;	//Set the tech note's position to match
				break;
			}
		}
	}
	if(u)
	{	//If any changes were made
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Perform track fixup in case tech notes at the same position need to be merged
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_note_move_by_grid_snap(int dir, char *undo_made)
{
	unsigned long target = 0, current, i;

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	//First pass:  Ensure that each selected note can be moved forward/backward one grid snap
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If the note is selected
			if(!eof_find_grid_snap(eof_get_note_pos(eof_song, eof_selected_track, i), dir, &target))
			{	//If there is no previous/next grid snap position that can be determined
				return 0;
			}
		}
	}

	//Second pass:  Move the notes
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If the note is selected
			current = eof_get_note_pos(eof_song, eof_selected_track, i);
			if(!eof_find_grid_snap(current, dir, &target))
			{	//If there is no previous/next grid snap position that can be determined
				break;	//Abort the rest of the operation
			}
			if(undo_made && (*undo_made == 0))
			{	//If an undo state needs to be made
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			eof_set_note_pos(eof_song, eof_selected_track, i, target);
		}
	}

	return 1;
}

int eof_menu_note_move_back_grid_snap(void)
{
	char undo_made = 0;

	return eof_menu_note_move_by_grid_snap(-1, &undo_made);
}

int eof_menu_note_move_forward_grid_snap(void)
{
	char undo_made = 0;

	return eof_menu_note_move_by_grid_snap(1, &undo_made);
}

int eof_menu_note_move_by_millisecond(int dir, char *undo_made)
{
	unsigned long pos, i;

	if(eof_count_selected_and_unselected_notes(NULL) == 0)
	{
		return 1;
	}

	//First pass:  If moving backward, ensure that each selected note can be moved backward
	if(dir < 0)
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
			{	//If the note is selected
				if(eof_get_note_pos(eof_song, eof_selected_track, i) == 0)
				{	//If this note is at 0ms and cannot move backward
					return 0;	//Return failure
				}

				break;	//Otherwise this and all remaining notes are after 0ms
			}
		}
	}

	//Second pass:  Move the notes
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If the note is selected
			pos = eof_get_note_pos(eof_song, eof_selected_track, i);

			if(dir < 0)
			{	//Move note backward
				pos--;
			}
			else
			{	//Move note forward
				pos++;
			}
			if(undo_made && (*undo_made == 0))
			{	//If an undo state needs to be made
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			eof_set_note_pos(eof_song, eof_selected_track, i, pos);
		}
	}

	return 1;
}

int eof_menu_note_move_back_millisecond(void)
{
	char undo_made = 0;

	return eof_menu_note_move_by_millisecond(-1, &undo_made);
}

int eof_menu_note_move_forward_millisecond(void)
{
	char undo_made = 0;

	return eof_menu_note_move_by_millisecond(1, &undo_made);
}

int eof_menu_note_convert_to_ghl_open(void)
{
	unsigned long i;
	char undo_made = 0;
	int note_selection_updated;

	if(!eof_track_is_ghl_mode(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a legacy guitar track with GHL mode enabled is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			if(!eof_legacy_guitar_note_is_open(eof_song, eof_selected_track, i))
			{	//If this note isn't already an open note
				if(!undo_made)
				{	//If an undo state needs to be made
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_set_note_note(eof_song, eof_selected_track, i, 32);	//Change to a lane 6 gem
				eof_or_note_flags(eof_song, eof_selected_track, i, EOF_GUITAR_NOTE_FLAG_GHL_OPEN);	//Set the GHL open note flag
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_note_swap_ghl_black_white_gems(void)
{
	unsigned long i;
	char undo_made = 0;
	int note_selection_updated;

	if(!eof_track_is_ghl_mode(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a legacy guitar track with GHL mode enabled is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			if(!eof_legacy_guitar_note_is_open(eof_song, eof_selected_track, i))
			{	//If this note isn't an open note (which is not modified by this function
				if(!undo_made)
				{	//If an undo state needs to be made
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				(void) eof_note_swap_ghl_black_white_gems(eof_song, eof_selected_track, i);	//Modify the note
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return 1;
}

int eof_menu_note_remove_disjointed(void)
{
	int note_selection_updated, undo_made = 0;
	unsigned long ctr, ctr2, eflags;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[ctr] || (eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type))
			continue;	//If this note isn't selected, skip it

		eflags = eof_get_note_eflags(eof_song, eof_selected_track, ctr);
		if((eflags & EOF_NOTE_EFLAG_DISJOINTED) && !undo_made)
		{	//If this selected note has disjointed status and an undo state hasn't been made yet
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		eflags &= ~EOF_NOTE_EFLAG_DISJOINTED;
		eof_set_note_eflags(eof_song, eof_selected_track, ctr, eflags);	//Clear the selected note's disjointed status

		for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, eof_selected_track); ctr2++)
		{	//For each note in the active track
			if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_get_note_type(eof_song, eof_selected_track, ctr2))
			{	//If this note is in the same difficulty as the one from the selected note identified in the outer loop
				if(eof_get_note_pos(eof_song, eof_selected_track, ctr) == eof_get_note_pos(eof_song, eof_selected_track, ctr2))
				{	//If both notes also start at the same timestamp
					eflags = eof_get_note_eflags(eof_song, eof_selected_track, ctr2);
					if((eflags & EOF_NOTE_EFLAG_DISJOINTED) && !undo_made)
					{	//If this inner loop's note has disjointed status and an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eflags &= ~EOF_NOTE_EFLAG_DISJOINTED;
					eof_set_note_eflags(eof_song, eof_selected_track, ctr2, eflags);	//Clear the inner loop's note's disjointed status
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	eof_legacy_track_fixup_notes(eof_song, eof_selected_track, 1);

	return 1;
}

int eof_menu_note_toggle_disjointed(void)
{
	(void) eof_menu_note_toggle_flag(1, EOF_LEGACY_TRACK_FORMAT, EOF_NOTE_EFLAG_DISJOINTED);
	eof_legacy_track_fixup_notes(eof_song, eof_selected_track, 1);

	return 1;
}

int eof_menu_note_split_lyric_line_after_selected(void)
{
	EOF_PHRASE_SECTION *thisline, *nextline;
	long nextlyric;
	unsigned long thispos, nextpos, i;
	char undo_made = 0;
	unsigned long old_lyric_count;

	if(!eof_song || (eof_selected_track != EOF_TRACK_VOCALS) || (eof_selection.track != EOF_TRACK_VOCALS))
		return 0;	//Don't allow this function to run if the vocal track isn't active or there are no selected notes in the vocal track

	if(eof_vocals_selected)
	{
		unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;	//Simplify
		EOF_VOCAL_TRACK * tp = eof_song->vocal_track[tracknum];

		old_lyric_count = tp->lines;
		eof_log("Splitting lyric line after selected lyric", 1);
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each lyric in the active track
			if(eof_selection.multi[i])
			{	//If the lyric is selected
				thispos = eof_get_note_pos(eof_song, eof_selected_track, i);
				thisline = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_LYRIC_PHRASE_SECTION, thispos);

				if(thisline)
				{	//If the lyric line containing the selected lyric was identified
					nextlyric = eof_track_fixup_next_note(eof_song, eof_selected_track, i);

					if(!undo_made)
					{	//If an undo state was not made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					if(nextlyric > i)
					{	//If there is a next lyric
						nextpos = eof_get_note_pos(eof_song, eof_selected_track, nextlyric);
						nextline = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_LYRIC_PHRASE_SECTION, nextpos);

						if(nextline && (nextline == thisline))
						{	//If the lyric line containing the next lyric was identified and it's in the same line as this lyric
							unsigned long end = thisline->end_pos;
							unsigned char diff = thisline->difficulty;

							//End this lyric line at the end of the selected lyric (do this before creating the new line in case the latter's call to eof_sort_and_merge_overlapping_sections() invalidates the thisline pointer
							thisline->end_pos = thispos + eof_get_note_length(eof_song, eof_selected_track, i);

							//Create a new lyric line starting at the next lyric that ends at the same timestamp as this lyric line
							eof_vocal_track_add_line(tp, nextpos, end, diff);
							if(old_lyric_count >= tp->lines)
								allegro_message("Split after lyric line failed");
						}
						else
						{	//The next lyric is in a different line or no line at all, end the lyric line at the end of the selected lyric
							thisline->end_pos = thispos + eof_get_note_length(eof_song, eof_selected_track, i);
						}
					}
				}
			}
		}
		if(undo_made)
		{	//If any lyric lines were modified
			eof_sort_and_merge_overlapping_sections(tp->line, &tp->lines);	//Sort and remove overlapping instances
			eof_reset_lyric_preview_lines();

			///DEBUG
			eof_check_and_log_lyric_line_errors(eof_song, 0);
		}
	}

	return 1;	//Success
}

int eof_menu_note_move_note_start(void)
{
	int note_selection_updated;
	unsigned long endpos;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if((eof_count_selected_and_unselected_notes(NULL) != 1) || (eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track)))
	{	//If there isn't exactly one note selected in the active track difficulty, or the selected note is otherwise invalid
		return 1;
	}

	endpos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current) + eof_get_note_length(eof_song, eof_selected_track, eof_selection.current);
	if(eof_music_pos.value - eof_av_delay < endpos)
	{	//If the seek position is before the selected note's current end position
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_note_pos(eof_song, eof_selected_track, eof_selection.current, eof_music_pos.value - eof_av_delay);	//Change the note's start position to the seek position
		eof_set_note_length(eof_song, eof_selected_track, eof_selection.current, endpos - (eof_music_pos.value - eof_av_delay));	//Change the note's length to end at the same position it originally did
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return D_O_K;
}

int eof_menu_note_move_note_end(void)
{
	int note_selection_updated;

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if((eof_count_selected_and_unselected_notes(NULL) != 1) || (eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track)))
	{	//If there isn't exactly one note selected in the active track difficulty, or the selected note is otherwise invalid
		return 1;
	}

	if(eof_music_pos.value - eof_av_delay > eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current))
	{	//If the seek position is after the selected note's current start position
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_note_length(eof_song, eof_selected_track, eof_selection.current, eof_music_pos.value - eof_av_delay - eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));	//Change the note's curent length so it ends at the seek position
	}

	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}

	return D_O_K;
}

int eof_menu_note_lyric_line_repair_timing(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long linectr, lyricctr, lyric_endpos = 0;
	EOF_VOCAL_TRACK *tp;
	char undo_made = 0;

	if(!eof_vocals_selected || (eof_selection.track != EOF_TRACK_VOCALS))
		return 1;	//The vocal track is not active or no notes in the vocal track are selected

 	eof_log("eof_menu_note_lyric_line_repair_timing() entered", 1);

	tp = eof_song->vocal_track[tracknum];	//Simplify
	eof_track_sort_notes(eof_song, eof_selected_track);	//Ensure lyrics are sorted
	for(linectr = 0; linectr < tp->lines; linectr++)
	{	//For each lyric line
		for(lyricctr = 0; lyricctr < tp->lyrics; lyricctr++)
		{	//For each lyric
			if(eof_selection.multi[lyricctr])
			{	//If this lyric is selected
				char process = 0;	//By default, do not check/alter the lyric line any further unless selected lyrics put this line in the scope of the function

				lyric_endpos = tp->lyric[lyricctr]->pos + tp->lyric[lyricctr]->length;
				if(!eof_find_lyric_line(lyricctr) && (lyric_endpos >= tp->line[linectr].start_pos) && (lyric_endpos <= tp->line[linectr].end_pos))
				{	//If this lyric does not begin in any line, but it ends in the lyric line being examined
					process = 1;	//This lyric line is to be processed
				}
				else if((tp->lyric[lyricctr]->pos >= tp->line[linectr].start_pos) && (tp->lyric[lyricctr]->pos <= tp->line[linectr].end_pos))
				{	//If this lyric begins in the lyric line being examined
					process = 1;	//This lyric line is to be processed
				}

				if(process)
				{	//If the lyric selection indicates this line should be updated if necesary, parse lyrics again from the start
					//Update lyric line start position
					for(lyricctr = 0; lyricctr < tp->lyrics; lyricctr++)
					{	//For each lyric
						lyric_endpos = tp->lyric[lyricctr]->pos + tp->lyric[lyricctr]->length;
						if(!eof_find_lyric_line(lyricctr) && (lyric_endpos >= tp->line[linectr].start_pos) && (lyric_endpos <= tp->line[linectr].end_pos))
						{	//If this lyric does not begin in any line, but it ends in the lyric line being examined
							if(!undo_made)
							{
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								undo_made = 1;
							}
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAltering lyric line #%lu that had started at pos %lums to %lums", lyricctr, tp->line[linectr].start_pos, tp->lyric[lyricctr]->pos);
							eof_log(eof_log_string, 1);
							tp->line[linectr].start_pos = tp->lyric[lyricctr]->pos;	//Update the line's start position
							break;	//Break from the inner lyric loop
						}
						else if((tp->lyric[lyricctr]->pos >= tp->line[linectr].start_pos) && (tp->lyric[lyricctr]->pos <= tp->line[linectr].end_pos))
						{	//If this is the first lyric found to begin in the lyric line being examined
							if(tp->lyric[lyricctr]->pos != tp->line[linectr].start_pos)
							{	//If this lyric begins at a timestamp other than the start of the lyric line
								if(!undo_made)
								{
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									undo_made = 1;
								}
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAltering lyric line #%lu that had started at pos %lums to %lums", lyricctr, tp->line[linectr].start_pos, tp->lyric[lyricctr]->pos);
								eof_log(eof_log_string, 1);
								tp->line[linectr].start_pos = tp->lyric[lyricctr]->pos;	//Update the line's start position
							}
							break;	//Break from the inner lyric loop
						}
					}

					//Update lyric line end position
					for(lyricctr = 0; lyricctr < tp->lyrics; lyricctr++)
					{	//For each lyric
						if((tp->lyric[lyricctr]->pos >= tp->line[linectr].start_pos) && (tp->lyric[lyricctr]->pos <= tp->line[linectr].end_pos))
						{	//If this lyric begins in the lyric line being examined
							for(;(lyricctr + 1 < tp->lyrics) && (tp->lyric[lyricctr + 1]->pos <= tp->line[linectr].end_pos);lyricctr++);	//Advance lyricctr to reference the last lyric beginning in this line
							lyric_endpos = tp->lyric[lyricctr]->pos + tp->lyric[lyricctr]->length;

							if(lyric_endpos != tp->line[linectr].end_pos)
							{	//If this lyric ends at a timestamp other than the end of the lyric line
								if(!undo_made)
								{
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									undo_made = 1;
								}
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAltering lyric line #%lu that had ended at pos %lums to %lums", lyricctr, tp->line[linectr].end_pos, lyric_endpos);
								eof_log(eof_log_string, 1);
								tp->line[linectr].end_pos = lyric_endpos;	//Update the line's end position
								break;	//Break from the inner lyric loop
							}
						}
					}
					break;	//Break from the outer lyric loop and check the next lyric line
				}
			}
		}
	}

	return 1;
}

int eof_menu_toggle_beatable_snap_status(unsigned long toggleflags)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, note;
	int note_selection_updated;

	if(!eof_track_is_beatable_mode(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a BEATABLE track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if((eof_count_selected_and_unselected_notes(NULL) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
				continue;	//If the note isn't selected, skip it

			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			eof_xor_note_flags(eof_song, eof_selected_track, i, toggleflags);	//Toggle the provided flag
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if((note & 16) && !(flags & (EOF_BEATABLE_NOTE_FLAG_LSNAP | EOF_BEATABLE_NOTE_FLAG_RSNAP)))
			{	//If this note has a gem on lane 5 note and it no longer has either left or right snap status
				note &= ~16;		//Clear the bit for lane 5
				eof_set_note_note(eof_song, eof_selected_track, i, note);
			}
			else
			{	//Otherwise at least one snap status is present, ensure a gem is present on lane 5
				eof_set_note_note(eof_song, eof_selected_track, i, note | 16);
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_beatable_toggle_beatable_lsnap(void)
{
	return eof_menu_toggle_beatable_snap_status(EOF_BEATABLE_NOTE_FLAG_LSNAP);
}

int eof_menu_beatable_toggle_beatable_rsnap(void)
{
	return eof_menu_toggle_beatable_snap_status(EOF_BEATABLE_NOTE_FLAG_RSNAP);
}

int eof_menu_set_beatable_slide(unsigned char lane)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	unsigned long slideflags[5] = {0, EOF_BEATABLE_FLAG_SLIDE_TO_L2, EOF_BEATABLE_FLAG_SLIDE_TO_L1, EOF_BEATABLE_FLAG_SLIDE_TO_R1, EOF_BEATABLE_FLAG_SLIDE_TO_R2};
	int note_selection_updated;

	if(lane > 4)
		return 1;	//Invalid slide to lane
	if(!eof_track_is_beatable_mode(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run unless a BEATABLE track is active

	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if((eof_count_selected_and_unselected_notes(NULL) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
				continue;	//If the note isn't selected, skip it
			if(lane && (eof_get_note_note(eof_song, eof_selected_track, i) & (1 << (lane - 1))))
				continue;	//If setting a slide instead of removing slides, and this note already has a gem on the specified slide to lane, skip it
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(lane)
			{	//If setting a slide to status
				if(flags & slideflags[lane])
					continue;	//If this note already slides to the specified lane, skip it
			}
			else
			{	//If removing all slide to statuses
				if(!(flags & EOF_BEATABLE_FLAG_SLIDE_TO_ANY))
					continue;	//If this note does not slide to any lane, skip it
			}

			if(!undo_made)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
				undo_made = 1;
			}
			flags &= ~EOF_BEATABLE_FLAG_SLIDE_TO_ANY;	//Clear all slide to flags
			flags |= slideflags[lane];	//Set the slide to flag for the specified lane (or no lane in the event of lane being 0)
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return 1;
}

int eof_menu_set_beatable_slide_lane_none(void)
{
	return eof_menu_set_beatable_slide(0);
}

int eof_menu_set_beatable_slide_lane_1(void)
{
	return eof_menu_set_beatable_slide(1);
}

int eof_menu_set_beatable_slide_lane_2(void)
{
	return eof_menu_set_beatable_slide(2);
}

int eof_menu_set_beatable_slide_lane_3(void)
{
	return eof_menu_set_beatable_slide(3);
}

int eof_menu_set_beatable_slide_lane_4(void)
{
	return eof_menu_set_beatable_slide(4);
}
