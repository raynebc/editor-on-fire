#include <allegro.h>
#include <ctype.h>
#include "../agup/agup.h"
#include "../undo.h"
#include "../dialog.h"
#include "../dialog/proc.h"
#include "../player.h"
#include "../utility.h"
#include "../foflc/Lyric_storage.h"
#include "../main.h"
#include "../mix.h"
#include "../rs.h"	//For eof_is_string_muted()
#include "edit.h"
#include "note.h"
#include "song.h"	//For eof_menu_track_selected_track_number()

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char eof_solo_menu_mark_text[32] = "&Mark";
char eof_star_power_menu_mark_text[32] = "&Mark";
char eof_lyric_line_menu_mark_text[32] = "&Mark";
char eof_arpeggio_menu_mark_text[32] = "&Mark";
char eof_trill_menu_mark_text[32] = "&Mark";
char eof_tremolo_menu_mark_text[32] = "&Mark";
char eof_slider_menu_mark_text[32] = "&Mark";
char eof_trill_menu_text[32] = "Trill";
char eof_tremolo_menu_text[32] = "Tremolo";

char eof_menu_solo_copy_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_solo_copy_menu[EOF_TRACKS_MAX] =
{
    {eof_menu_solo_copy_menu_text[0], eof_menu_copy_solos_track_1, NULL, D_SELECTED, NULL},
    {eof_menu_solo_copy_menu_text[1], eof_menu_copy_solos_track_2, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[2], eof_menu_copy_solos_track_3, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[3], eof_menu_copy_solos_track_4, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[4], eof_menu_copy_solos_track_5, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[5], eof_menu_copy_solos_track_6, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[6], eof_menu_copy_solos_track_7, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[7], eof_menu_copy_solos_track_8, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[8], eof_menu_copy_solos_track_9, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[9], eof_menu_copy_solos_track_10, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[10], eof_menu_copy_solos_track_11, NULL, 0, NULL},
    {eof_menu_solo_copy_menu_text[11], eof_menu_copy_solos_track_12, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_solo_menu[] =
{
    {eof_solo_menu_mark_text, eof_menu_solo_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_solo_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_solo_erase_all, NULL, 0, NULL},
    {"&Copy From", NULL, eof_menu_solo_copy_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

char eof_menu_sp_copy_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_sp_copy_menu[EOF_TRACKS_MAX] =
{
    {eof_menu_sp_copy_menu_text[0], eof_menu_copy_sp_track_1, NULL, D_SELECTED, NULL},
    {eof_menu_sp_copy_menu_text[1], eof_menu_copy_sp_track_2, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[2], eof_menu_copy_sp_track_3, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[3], eof_menu_copy_sp_track_4, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[4], eof_menu_copy_sp_track_5, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[5], eof_menu_copy_sp_track_6, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[6], eof_menu_copy_sp_track_7, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[7], eof_menu_copy_sp_track_8, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[8], eof_menu_copy_sp_track_9, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[9], eof_menu_copy_sp_track_10, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[10], eof_menu_copy_sp_track_11, NULL, 0, NULL},
    {eof_menu_sp_copy_menu_text[11], eof_menu_copy_sp_track_12, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_star_power_menu[] =
{
    {eof_star_power_menu_mark_text, eof_menu_star_power_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_star_power_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_star_power_erase_all, NULL, 0, NULL},
    {"&Copy From", NULL, eof_menu_sp_copy_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_lyric_line_menu[] =
{
    {eof_lyric_line_menu_mark_text, eof_menu_lyric_line_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_lyric_line_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_lyric_line_erase_all, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Toggle Overdrive", eof_menu_lyric_line_toggle_overdrive, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

char eof_menu_arpeggio_copy_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_arpeggio_copy_menu[EOF_TRACKS_MAX] =
{
    {eof_menu_arpeggio_copy_menu_text[0], eof_menu_copy_arpeggio_track_1, NULL, D_SELECTED, NULL},
    {eof_menu_arpeggio_copy_menu_text[1], eof_menu_copy_arpeggio_track_2, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[2], eof_menu_copy_arpeggio_track_3, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[3], eof_menu_copy_arpeggio_track_4, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[4], eof_menu_copy_arpeggio_track_5, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[5], eof_menu_copy_arpeggio_track_6, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[6], eof_menu_copy_arpeggio_track_7, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[7], eof_menu_copy_arpeggio_track_8, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[8], eof_menu_copy_arpeggio_track_9, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[9], eof_menu_copy_arpeggio_track_10, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[10], eof_menu_copy_arpeggio_track_11, NULL, 0, NULL},
    {eof_menu_arpeggio_copy_menu_text[11], eof_menu_copy_arpeggio_track_12, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_arpeggio_menu[] =
{
    {eof_arpeggio_menu_mark_text, eof_menu_arpeggio_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_arpeggio_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_arpeggio_erase_all, NULL, 0, NULL},
    {"&Copy From", NULL, eof_menu_arpeggio_copy_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_pro_guitar_slide_menu[] =
{
    {"Toggle slide &Up\t" CTRL_NAME "+Up", eof_menu_note_toggle_slide_up, NULL, 0, NULL},
    {"Toggle slide &Down\t" CTRL_NAME "+Down", eof_menu_note_toggle_slide_down, NULL, 0, NULL},
    {"&Remove slide", eof_menu_note_remove_slide, NULL, 0, NULL},
    {"Set &End fret\t" CTRL_NAME "+Shift+L", eof_pro_guitar_note_slide_end_fret_save, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_pro_guitar_strum_menu[] =
{
    {"Toggle strum &Up\tShift+Up", eof_pro_guitar_toggle_strum_up, NULL, 0, NULL},
    {"Toggle strum &Mid\tShift+M", eof_pro_guitar_toggle_strum_mid, NULL, 0, NULL},
    {"Toggle strum &Down\tShift+Down", eof_pro_guitar_toggle_strum_down, NULL, 0, NULL},
    {"&Remove strum direction", eof_menu_note_remove_strum_direction, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

char eof_menu_trill_copy_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_trill_copy_menu[EOF_TRACKS_MAX] =
{
    {eof_menu_trill_copy_menu_text[0], eof_menu_copy_trill_track_1, NULL, D_SELECTED, NULL},
    {eof_menu_trill_copy_menu_text[1], eof_menu_copy_trill_track_2, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[2], eof_menu_copy_trill_track_3, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[3], eof_menu_copy_trill_track_4, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[4], eof_menu_copy_trill_track_5, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[5], eof_menu_copy_trill_track_6, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[6], eof_menu_copy_trill_track_7, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[7], eof_menu_copy_trill_track_8, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[8], eof_menu_copy_trill_track_9, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[9], eof_menu_copy_trill_track_10, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[10], eof_menu_copy_trill_track_11, NULL, 0, NULL},
    {eof_menu_trill_copy_menu_text[11], eof_menu_copy_trill_track_12, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_trill_menu[] =
{
    {eof_trill_menu_mark_text, eof_menu_trill_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_trill_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_trill_erase_all, NULL, 0, NULL},
    {"&Copy From", NULL, eof_menu_trill_copy_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

char eof_menu_tremolo_copy_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_tremolo_copy_menu[EOF_TRACKS_MAX] =
{
    {eof_menu_tremolo_copy_menu_text[0], eof_menu_copy_tremolo_track_1, NULL, D_SELECTED, NULL},
    {eof_menu_tremolo_copy_menu_text[1], eof_menu_copy_tremolo_track_2, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[2], eof_menu_copy_tremolo_track_3, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[3], eof_menu_copy_tremolo_track_4, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[4], eof_menu_copy_tremolo_track_5, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[5], eof_menu_copy_tremolo_track_6, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[6], eof_menu_copy_tremolo_track_7, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[7], eof_menu_copy_tremolo_track_8, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[8], eof_menu_copy_tremolo_track_9, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[9], eof_menu_copy_tremolo_track_10, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[10], eof_menu_copy_tremolo_track_11, NULL, 0, NULL},
    {eof_menu_tremolo_copy_menu_text[11], eof_menu_copy_tremolo_track_12, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_tremolo_menu[] =
{
    {eof_tremolo_menu_mark_text, eof_menu_tremolo_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_tremolo_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_tremolo_erase_all, NULL, 0, NULL},
    {"&Copy From", NULL, eof_menu_tremolo_copy_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_slider_menu[] =
{
    {eof_slider_menu_mark_text, eof_menu_slider_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_slider_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_slider_erase_all, NULL, 0, NULL},
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

MENU eof_note_drum_menu[] =
{
    {"Toggle Yellow cymbal\t" CTRL_NAME "+Y", eof_menu_note_toggle_rb3_cymbal_yellow, NULL, 0, NULL},
    {"Toggle Blue cymbal\t" CTRL_NAME "+B", eof_menu_note_toggle_rb3_cymbal_blue, NULL, 0, NULL},
    {"Toggle Green cymbal\t" CTRL_NAME "+G", eof_menu_note_toggle_rb3_cymbal_green, NULL, 0, NULL},
    {"Mark as &Non cymbal", eof_menu_note_remove_cymbal, NULL, 0, NULL},
    {"Mark new notes as &Cymbals", eof_menu_note_default_cymbal, NULL, 0, NULL},
    {"Toggle expert+ bass drum\t" CTRL_NAME "+E", eof_menu_note_toggle_double_bass, NULL, 0, NULL},
    {"Remove &Expert+ bass drum", eof_menu_note_remove_double_bass, NULL, 0, NULL},
    {"&Mark new notes as Expert+", eof_menu_note_default_double_bass, NULL, 0, NULL},
    {"Toggle Y note as &Open hi hat\tShift+O", eof_menu_note_toggle_hi_hat_open, NULL, 0, NULL},
    {"Toggle Y note as &Pedal hi hat\tShift+P",eof_menu_note_toggle_hi_hat_pedal, NULL, 0, NULL},
    {"Toggle Y note as &Sizzle hi hat\tShift+S", eof_menu_note_toggle_hi_hat_sizzle, NULL, 0, NULL},
    {"Remove &Hi hat status", eof_menu_note_remove_hi_hat_status, NULL, 0, NULL},
    {"Mark new &Y notes as", NULL, eof_note_drum_hi_hat_menu, 0, NULL},
    {"Toggle R note as &Rim shot\tShift+R",eof_menu_note_toggle_rimshot, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

char eof_menu_thin_notes_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_thin_notes_menu[EOF_TRACKS_MAX] =
{
    {eof_menu_thin_notes_menu_text[0], eof_menu_thin_notes_track_1, NULL, D_SELECTED, NULL},
    {eof_menu_thin_notes_menu_text[1], eof_menu_thin_notes_track_2, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[2], eof_menu_thin_notes_track_3, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[3], eof_menu_thin_notes_track_4, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[4], eof_menu_thin_notes_track_5, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[5], eof_menu_thin_notes_track_6, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[6], eof_menu_thin_notes_track_7, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[7], eof_menu_thin_notes_track_8, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[8], eof_menu_thin_notes_track_9, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[9], eof_menu_thin_notes_track_10, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[10], eof_menu_thin_notes_track_11, NULL, 0, NULL},
    {eof_menu_thin_notes_menu_text[11], eof_menu_thin_notes_track_12, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_proguitar_menu[] =
{
    {"Edit pro guitar &Note\tN", eof_menu_note_edit_pro_guitar_note, NULL, 0, NULL},
    {"Edit pro guitar &Frets/Fingers\tF", eof_menu_note_edit_pro_guitar_note_frets_fingers_menu, NULL, 0, NULL},
    {"Clear finger information", eof_menu_pro_guitar_remove_fingering, NULL, 0, NULL},
    {"&Arpeggio", NULL, eof_arpeggio_menu, 0, NULL},
    {"s&Lide", NULL, eof_pro_guitar_slide_menu, 0, NULL},
    {"&Strum", NULL, eof_pro_guitar_strum_menu, 0, NULL},
    {"&Clear legacy bitmask", eof_menu_note_clear_legacy_values, NULL, 0, NULL},
    {"Toggle tapping\t" CTRL_NAME "+T", eof_menu_note_toggle_tapping, NULL, 0, NULL},
    {"Remove &Tapping", eof_menu_note_remove_tapping, NULL, 0, NULL},
    {"Toggle bend\t" CTRL_NAME "+B", eof_menu_note_toggle_bend, NULL, 0, NULL},
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
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_lyrics_menu[] =
{
    {"&Edit Lyric\tL", eof_edit_lyric_dialog, NULL, 0, NULL},
    {"&Split Lyric\tShift+L", eof_menu_split_lyric, NULL, 0, NULL},
    {"&Lyric Lines", NULL, eof_lyric_line_menu, 0, NULL},
    {"&Freestyle", NULL, eof_note_freestyle_menu, 0, NULL},
    {"Import GP style lyric text from file", eof_note_menu_read_gp_lyric_texts, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_menu[] =
{
    {"&Toggle", NULL, eof_note_toggle_menu, 0, NULL},
    {"&Clear", NULL, eof_note_clear_menu, 0, NULL},
    {"Transpose Up\tUp", eof_menu_note_transpose_up, NULL, 0, NULL},
    {"Transpose Down\tDown", eof_menu_note_transpose_down, NULL, 0, NULL},
    {"&Resnap", eof_menu_note_resnap, NULL, 0, NULL},
    {"&Solos", NULL, eof_solo_menu, 0, NULL},
    {"Star &Power", NULL, eof_star_power_menu, 0, NULL},
    {"Delete\tDel", eof_menu_note_delete, NULL, 0, NULL},
    {"Edit &Name", eof_menu_note_edit_name, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Toggle &Crazy\tT", eof_menu_note_toggle_crazy, NULL, 0, NULL},
    {"&HOPO", NULL, eof_legacy_hopo_menu, 0, NULL},
    {eof_trill_menu_text, NULL, eof_trill_menu, 0, NULL},
    {eof_tremolo_menu_text, NULL, eof_tremolo_menu, 0, NULL},
    {"Slider", NULL, eof_slider_menu, 0, NULL},
    {"Thin diff. to match", NULL, eof_menu_thin_notes_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Drum", NULL, eof_note_drum_menu, 0, NULL},
    {"Pro &Guitar", NULL, eof_note_proguitar_menu, 0, NULL},
    {"&Lyrics", NULL, eof_note_lyrics_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_lyric_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  204 + 110, 106, 2,   23,  0,    0,      0,   0,   "Edit Lyric",               NULL, NULL },
   { d_agup_text_proc,   12,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Text:",         NULL, NULL },
   { d_agup_edit_proc,   48, 80,  144 + 110,  20,  2,   23,  0,    0,      255,   0,   eof_etext,           NULL, NULL },
   { d_agup_button_proc, 12 + 55,  112, 84,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 108 + 55, 112, 78,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_split_lyric_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  204 + 110, 106, 2,   23,  0,    0,      0,   0,   "Split Lyric",               NULL, NULL },
   { d_agup_text_proc,   12,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Text:",         NULL, NULL },
   { d_agup_edit_proc,   48, 80,  144 + 110,  20,  2,   23,  0,    0,      255,   0,   eof_etext,           NULL, NULL },
   { d_agup_button_proc, 12 + 55,  112, 84,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 108 + 55, 112, 78,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_note_name_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  204 + 110, 106, 2,   23,  0,    0,      0,   0,   "Edit note name",               NULL, NULL },
   { d_agup_text_proc,   12,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Text:",         NULL, NULL },
   { d_agup_edit_proc,   48, 80,  144 + 110,  20,  2,   23,  0,    0,      EOF_NAME_LENGTH,   0,   eof_etext,           NULL, NULL },
   { d_agup_button_proc, 12 + 55,  112, 84,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 108 + 55, 112, 78,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_prepare_note_menu(void)
{
	int vselected;
	int insp = 0, insolo = 0, inll = 0, inarpeggio = 0, intrill = 0, intremolo = 0, inslider = 0;
	int spstart = -1, ssstart = -1, llstart = -1;
	int spend = -1, ssend = -1, llend = -1;
	int spp = 0, ssp = 0, llp = 0;
	unsigned long i, j;
	unsigned long tracknum;
	int sel_start = eof_chart_length, sel_end = 0;
	EOF_PHRASE_SECTION *sectionptr = NULL;
	unsigned long track_behavior = 0;

	if(eof_song && eof_song_loaded)
	{
		int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

		track_behavior = eof_song->track[eof_selected_track]->track_behavior;
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		if(eof_vocals_selected)
		{	//PART VOCALS SELECTED
			for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
			{
				if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
				{
					if(eof_song->vocal_track[tracknum]->lyric[i]->pos < sel_start)
					{
						sel_start = eof_song->vocal_track[tracknum]->lyric[i]->pos;
					}
					if(eof_song->vocal_track[tracknum]->lyric[i]->pos > sel_end)
					{
						sel_end = eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length;
					}
				}
			}
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
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the active track
				if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
				{
					if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
					{
						sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
					}
					if(eof_get_note_pos(eof_song, eof_selected_track, i) > sel_end)
					{
						sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
					}
				}
			}
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
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				for(j = 0; j < eof_song->pro_guitar_track[tracknum]->arpeggios; j++)
				{	//For each arpeggio phrase in the active track
					if((sel_end >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (sel_start <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos) && (eof_song->pro_guitar_track[tracknum]->arpeggio[j].difficulty == eof_note_type))
					{
						inarpeggio = 1;
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
			}
		}//PART VOCALS NOT SELECTED
		vselected = eof_count_selected_notes(NULL, 1);
		if(vselected)
		{	//ONE OR MORE NOTES/LYRICS SELECTED
			/* star power mark */
			sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, spp);
			if((sectionptr != NULL) && (spstart == sectionptr->start_pos) && (spend == sectionptr->end_pos))
			{
				eof_star_power_menu[0].flags = D_DISABLED;	//Note>Star Power>Mark/Remark
			}
			else
			{
				eof_star_power_menu[0].flags = 0;
			}

			/* solo mark */
			sectionptr = eof_get_solo(eof_song, eof_selected_track, ssp);
			if((sectionptr != NULL) && (ssstart == sectionptr->start_pos) && (ssend == sectionptr->end_pos))
			{
				eof_solo_menu[0].flags = D_DISABLED;		//Note>Solos>Mark/Remark
			}
			else
			{
				eof_solo_menu[0].flags = 0;
			}

			/* lyric line mark */
			if((llstart == eof_song->vocal_track[0]->line[llp].start_pos) && (llend == eof_song->vocal_track[0]->line[llp].end_pos))
			{
				eof_lyric_line_menu[0].flags = D_DISABLED;	//Note>Lyrics>Lyric Lines>Mark/Remark
			}
			else
			{
				eof_lyric_line_menu[0].flags = 0;
			}

			eof_note_menu[5].flags = 0;	//Note>Solos> submenu
			eof_note_menu[6].flags = 0; //Note>Star Power> submenu
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT))
			{	//If a legacy guitar note is selected
				eof_note_menu[14].flags = 0;		//Note>Slider> submenu
			}
			else
			{
				eof_note_menu[14].flags = D_DISABLED;
			}
		}
		else
		{	//NO NOTES/LYRICS SELECTED
			eof_star_power_menu[0].flags = D_DISABLED;	//Note>Star Power>Mark/Remark
			eof_solo_menu[0].flags = D_DISABLED; 		//Note>Solos>Mark/Remark
			eof_lyric_line_menu[0].flags = D_DISABLED;	//Note>Lyrics>Lyric Lines>Mark/Remark
			eof_note_menu[5].flags = D_DISABLED; 		//Note>Solos> submenu
			eof_note_menu[6].flags = D_DISABLED; 		//Note>Star Power> submenu
			eof_note_menu[14].flags = D_DISABLED;		//Note>Slider> submenu
		}

		/* star power mark/remark */
		if(insp)
		{
			eof_star_power_menu[1].flags = 0;
			(void) ustrcpy(eof_star_power_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_star_power_menu[1].flags = D_DISABLED;
			(void) ustrcpy(eof_star_power_menu_mark_text, "&Mark");
		}

		/* star power copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_sp_copy_menu[i].flags = 0;
			if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				(void) ustrncpy(eof_menu_sp_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				(void) ustrcpy(eof_menu_sp_copy_menu_text[i],"");
			}
			if(!eof_get_num_star_power_paths(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has no star power phrases or is the active track
				eof_menu_sp_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* solo mark/remark */
		if(insolo)
		{
			eof_solo_menu[1].flags = 0;
			(void) ustrcpy(eof_solo_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_solo_menu[1].flags = D_DISABLED;
			(void) ustrcpy(eof_solo_menu_mark_text, "&Mark");
		}

		/* solo copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_solo_copy_menu[i].flags = 0;
			if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				(void) ustrncpy(eof_menu_solo_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				(void) ustrcpy(eof_menu_solo_copy_menu_text[i],"");
			}
			if(!eof_get_num_solos(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has no solos or is the active track
				eof_menu_solo_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* lyric line */
		if(inll)
		{
			eof_lyric_line_menu[1].flags = 0;	//Note>Lyrics>Lyric Lines>Remove
			eof_lyric_line_menu[4].flags = 0; 	//Note>Lyrics>Lyric Lines>Toggle Overdrive
			(void) ustrcpy(eof_lyric_line_menu_mark_text, "Re-&Mark\t" CTRL_NAME "+M");
		}
		else
		{
			eof_lyric_line_menu[1].flags = D_DISABLED;
			eof_lyric_line_menu[4].flags = D_DISABLED;
			(void) ustrcpy(eof_lyric_line_menu_mark_text, "&Mark\t" CTRL_NAME "+M");
		}

		/* arpeggio mark/remark */
		if(inarpeggio)
		{
			eof_arpeggio_menu[1].flags = 0;				//Note>Pro Guitar>Arpeggio>Remove
			(void) ustrcpy(eof_arpeggio_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_arpeggio_menu[1].flags = D_DISABLED;
			(void) ustrcpy(eof_arpeggio_menu_mark_text, "&Mark");
		}

		/* arpeggio copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_arpeggio_copy_menu[i].flags = 0;
			if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				(void) ustrncpy(eof_menu_arpeggio_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				(void) ustrcpy(eof_menu_arpeggio_copy_menu_text[i],"");
			}
			if(!eof_get_num_arpeggios(eof_song, i + 1) || (i + 1 == eof_selected_track) || (eof_song->track[i + 1]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
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

		/* resnap */
		if(eof_snap_mode == EOF_SNAP_OFF)
		{
			eof_note_menu[4].flags = D_DISABLED;	//Note>Resnap
		}
		else
		{
			eof_note_menu[4].flags = 0;
		}

		if(eof_vocals_selected)
		{	//PART VOCALS SELECTED
			eof_note_menu[0].flags = D_DISABLED;	//Note>Toggle
			eof_note_menu[1].flags = D_DISABLED;	//Note>Clear
			eof_note_menu[2].flags = D_DISABLED;	//Note>Transpose Up
			eof_note_menu[3].flags = D_DISABLED;	//Note>Transpose Down
			eof_note_menu[5].flags = D_DISABLED;	//Note>Solos
			eof_note_menu[6].flags = D_DISABLED;	//Note>Star power
			eof_note_menu[8].flags = D_DISABLED;	//Note>Edit Name
			eof_note_menu[10].flags = D_DISABLED;	//Note>Toggle Crazy
			eof_note_menu[11].flags = D_DISABLED;	//Note>HOPO
			eof_note_menu[12].flags = D_DISABLED;	//Note>Trill> submenu
			eof_note_menu[13].flags = D_DISABLED;	//Note>Tremolo> submenu
			eof_note_menu[17].flags = D_DISABLED;	//Note>Drum> submenu
			eof_note_menu[18].flags = D_DISABLED;	//Note>Pro Guitar> submenu

			eof_note_menu[19].flags = 0;	//Note>Lyrics> submenu
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
			eof_note_menu[0].flags = 0; 			//Note>Toggle
			eof_note_menu[1].flags = 0; 			//Note>Clear
			eof_note_menu[8].flags = 0;				//Note>Edit Name
			eof_note_menu[12].flags = 0;			//Note>Trill> submenu
			eof_note_menu[13].flags = 0;			//Note>Tremolo> submenu
			eof_note_menu[19].flags = D_DISABLED;	//Note>Lyrics> submenu

			/* transpose up */
			if(eof_transpose_possible(-1))
			{
				eof_note_menu[2].flags = 0;			//Note>Transpose Up
			}
			else
			{
				eof_note_menu[2].flags = D_DISABLED;
			}

			/* transpose down */
			if(eof_transpose_possible(1))
			{
				eof_note_menu[3].flags = 0;			//Note>Transpose Down
			}
			else
			{
				eof_note_menu[3].flags = D_DISABLED;
			}

			/* toggle crazy */
			if((track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR))
			{	//When a guitar track is active
				eof_note_menu[10].flags = 0;				//Note>Toggle Crazy
			}
			else
			{
				eof_note_menu[10].flags = D_DISABLED;
			}

			if(eof_selected_track != EOF_TRACK_DRUM)
			{	//When PART DRUMS is not active
				eof_note_menu[17].flags = D_DISABLED;	//Note>Drum> submenu
			}
			else
			{	//When PART DRUMS is active
				eof_note_menu[17].flags = 0;
			}

			/* toggle Expert+ bass drum */
			if((eof_selected_track == EOF_TRACK_DRUM) && (eof_note_type == EOF_NOTE_AMAZING))
			{	//If the Amazing difficulty of PART DRUMS is active
				eof_note_drum_menu[5].flags = 0;			//Note>Drum>Toggle Expert+ Bass Drum
			}
			else
			{
				eof_note_drum_menu[5].flags = D_DISABLED;
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
			{
				eof_note_menu[11].flags = D_DISABLED;
			}

			/* Pro Guitar mode notation> */
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If the active track is a pro guitar track
				eof_note_menu[18].flags = 0;			//Note>Pro Guitar> submenu

				/* Arpeggio>Erase all */
				if(eof_song->pro_guitar_track[tracknum]->arpeggios)
				{	//If there's at least one arpeggio phrase
					eof_arpeggio_menu[2].flags = 0;		//Note>Pro Guitar>Arpeggio>Erase All
				}
				else
				{
					eof_arpeggio_menu[2].flags = D_DISABLED;
				}

				/* Thin difficulty to match */
				for(i = 0; i < EOF_TRACKS_MAX; i++)
				{	//For each track supported by EOF
					eof_menu_thin_notes_menu[i].flags = D_DISABLED;
					if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
					{	//If the track exists, copy its name into the string used by the track menu
						(void) ustrncpy(eof_menu_thin_notes_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
							//Copy the track name to the menu string
					}
					else
					{	//Write a blank string for the track name
						(void) ustrcpy(eof_menu_thin_notes_menu_text[i],"");
					}
					for(j = 0; j < eof_get_track_size(eof_song, i + 1); j++)
					{	//For each note in the track
						if(eof_get_note_type(eof_song,i + 1, j) == eof_note_type)
						{	//If the note is in the active track's difficulty
							eof_menu_thin_notes_menu[i].flags = 0;	//Enable the track from the submenu
							break;
						}
					}
				}
			}
			else
			{
				eof_note_menu[18].flags = D_DISABLED;
			}

			/* Trill mark/remark*/
			if(intrill)
			{
				eof_trill_menu[1].flags = 0;	//Note>Trill>Remove
				(void) ustrcpy(eof_trill_menu_mark_text, "Re-&Mark");
			}
			else
			{
				eof_trill_menu[1].flags = D_DISABLED;
				(void) ustrcpy(eof_trill_menu_mark_text, "&Mark");
			}

			/* Trill copy from */
			for(i = 0; i < EOF_TRACKS_MAX; i++)
			{	//For each track supported by EOF
				eof_menu_trill_copy_menu[i].flags = 0;
				if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
				{	//If the track exists, copy its name into the string used by the track menu
					(void) ustrncpy(eof_menu_trill_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
						//Copy the track name to the menu string
				}
				else
				{	//Otherwise write a blank string for the track name
					(void) ustrcpy(eof_menu_trill_copy_menu_text[i],"");
				}
				if(!eof_get_num_trills(eof_song, i + 1) || (i + 1 == eof_selected_track))
				{	//If the track has no trill phrases or is the active track
					eof_menu_trill_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
				}
			}

			/* Tremolo mark/remark */
			if(intremolo)
			{
				eof_tremolo_menu[1].flags = 0;	//Note>Tremolo>Remove
				(void) ustrcpy(eof_tremolo_menu_mark_text, "Re-&Mark");
			}
			else
			{
				eof_tremolo_menu[1].flags = D_DISABLED;
				(void) ustrcpy(eof_tremolo_menu_mark_text, "&Mark");
			}

			/* Tremolo copy from */
			for(i = 0; i < EOF_TRACKS_MAX; i++)
			{	//For each track supported by EOF
				eof_menu_tremolo_copy_menu[i].flags = 0;
				if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
				{	//If the track exists, copy its name into the string used by the track menu
					(void) ustrncpy(eof_menu_tremolo_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
						//Copy the track name to the menu string
				}
				else
				{	//Otherwise write a blank string for the track name
					(void) ustrcpy(eof_menu_tremolo_copy_menu_text[i],"");
				}
				if(!eof_get_num_tremolos(eof_song, i + 1) || (i + 1 == eof_selected_track))
				{	//If the track has no tremolo phrases or is the active track
					eof_menu_tremolo_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
				}
			}

			/* Rename Trill and Tremolo menus as necessary for the drum track */
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_KEYS_TRACK_BEHAVIOR))
			{	//If a legacy/pro guitar/bass/keys track is active, set the guitar terminology for trill and tremolo sections
				(void) ustrcpy(eof_trill_menu_text, "Trill");
				(void) ustrcpy(eof_tremolo_menu_text, "Tremolo");
			}
			else if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If a legacy drum track is active, set the drum terminology for trill and tremolo sections
				(void) ustrcpy(eof_trill_menu_text, "Special Drum Roll");
				(void) ustrcpy(eof_tremolo_menu_text, "Drum Roll");
			}
			else
			{	//Disable these submenus unless a track that can use them is active
				eof_note_menu[12].flags = D_DISABLED;	//Note>Trill> submenu
				eof_note_menu[13].flags = D_DISABLED;	//Note>Tremolo> submenu
			}
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_KEYS_TRACK_BEHAVIOR))
			{	//If this is a keys track, ensure the tremolo menu still gets disabled
				eof_note_menu[13].flags = D_DISABLED;	//Note>Tremolo> submenu
			}

			/* Slider */
			if(inslider)
			{
				eof_slider_menu[1].flags = 0;	//Note>Slider>Remove
				(void) ustrcpy(eof_slider_menu_mark_text, "Re-&Mark\tShift+S");
			}
			else
			{
				eof_slider_menu[1].flags = D_DISABLED;
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
		if(note_selection_updated)
		{	//If the only note modified was the seek hover note
			eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
			eof_selection.current = EOF_MAX_NOTES - 1;
		}
	}//if(eof_song && eof_song_loaded)
}

int eof_menu_note_transpose_up(void)
{
	unsigned long i, j;
	unsigned long max = 31;	//This represents the highest valid note bitmask, based on the current track options (including open bass strumming)
	unsigned long flags, note, tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_transpose_possible(-1))
	{
		return 1;
	}

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
	{
		if(eof_open_bass_enabled() || (eof_count_track_lanes(eof_song, eof_selected_track) > 5))
		{	//If open bass is enabled, or the track has more than 5 lanes, lane 6 is valid for use
			max = 63;
		}
		if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
		{	//Special case:  Legacy view is in effect, display pro guitar notes as 5 lane notes
			max = 31;
		}
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
			{	//If the note is in the same track as the selected notes, is selected and is the same difficulty as the selected notes
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, get the note's legacy bitmask
					note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
				}
				else
				{	//Otherwise get the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
				}
				note = (note << 1) & max;
				if((eof_selected_track == EOF_TRACK_BASS) && eof_open_bass_enabled() && (note & 32))
				{	//If open bass is enabled, and this transpose operation resulted in a bass guitar gem in lane 6
					flags = eof_get_note_flags(eof_song, eof_selected_track, i);
					eof_set_note_note(eof_song, eof_selected_track, i, 32);		//Clear all lanes except lane 6
					flags &= ~(EOF_NOTE_FLAG_CRAZY);	//Clear the crazy flag, which is invalid for open strum notes
					flags &= ~(EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO flags, which are invalid for open strum notes
					flags &= ~(EOF_NOTE_FLAG_NO_HOPO);
					eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				}
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, set the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise set the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
				if(!eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If a pro guitar note was tranposed, move the fret values accordingly (only if legacy view isn't in effect)
					for(j = 7; j > 0; j--)
					{	//For the upper 7 frets
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[j] = eof_song->pro_guitar_track[tracknum]->note[i]->frets[j-1];	//Cycle fret values up from lower lane
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[0] = 0xFF;	//Re-initialize lane 1 to the default of (muted)
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_transpose_down(void)
{
	unsigned long i, j;
	unsigned long note, tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_transpose_possible(1))
	{
		return 1;
	}

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
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
			{	//If the note is in the same track as the selected notes, is selected and is the same difficulty as the selected notes
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, get the note's legacy bitmask
					note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
				}
				else
				{	//Otherwise get the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
				}
				note = (note >> 1) & 63;
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, set the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise set the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
				if(!eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If a pro guitar note was tranposed, move the fret values accordingly (only if legacy view isn't in effect)
					for(j = 0; j < 7; j++)
					{	//For the 7 supported lower frets
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[j] = eof_song->pro_guitar_track[tracknum]->note[i]->frets[j+1];	//Cycle fret values down from upper lane
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[7] = 0xFF;	//Re-initialize lane 15 to the default of (muted)
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_transpose_up_octave(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_transpose_possible(-12))	//If selected lyrics cannot move up one octave
		return 1;
	if(!eof_vocals_selected)			//If PART VOCALS is not active
		return 1;

	eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (eof_song->vocal_track[tracknum]->lyric[i]->note != EOF_LYRIC_PERCUSSION))
			eof_song->vocal_track[tracknum]->lyric[i]->note+=12;

	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_transpose_down_octave(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_transpose_possible(12))		//If selected lyrics cannot move down one octave
		return 1;
	if(!eof_vocals_selected)		//If PART VOCALS is not active
		return 1;

	eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (eof_song->vocal_track[tracknum]->lyric[i]->note != EOF_LYRIC_PERCUSSION))
			eof_song->vocal_track[tracknum]->lyric[i]->note-=12;

	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_resnap(void)
{
	unsigned long i;
	unsigned long oldnotes = eof_get_track_size(eof_song, eof_selected_track);
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_snap_mode == EOF_SNAP_OFF)
	{
		return 1;
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			/* snap the note itself */
			eof_snap_logic(&eof_tail_snap, eof_get_note_pos(eof_song, eof_selected_track, i));
			eof_set_note_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);

			/* snap the tail */
			eof_snap_logic(&eof_tail_snap, eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i));
			eof_snap_length_logic(&eof_tail_snap);
			if(eof_get_note_length(eof_song, eof_selected_track, i) > 1)
			{
				eof_snap_logic(&eof_tail_snap, eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i));
				eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
	if(oldnotes != eof_get_track_size(eof_song, eof_selected_track))
	{
		allegro_message("Warning! Some %s snapped to the same position and were automatically combined.", (eof_song->track[eof_selected_track]->track_format == EOF_VOCAL_TRACK_FORMAT) ? "lyrics" : "notes");
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_delete(void)
{
	unsigned long i, d = 0;

	(void) eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If this note is in the active track, is selected and is in the active difficulty
			d++;
		}
	}
	if(d)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
		for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
		{
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i-1] && (eof_get_note_type(eof_song, eof_selected_track, i-1) == eof_note_type))
			{
				eof_track_delete_note(eof_song, eof_selected_track, i-1);
				eof_selection.multi[i-1] = 0;
			}
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_track_fixup_notes(eof_song, eof_selected_track, 0);
		eof_reset_lyric_preview_lines();
		eof_detect_difficulties(eof_song);
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	return 1;
}

int eof_menu_note_toggle_green(void)
{
	unsigned long i;
	unsigned long flags, note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 1;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 1);
			}
			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If green drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags &= (~EOF_NOTE_FLAG_DBASS);		//Clear the Expert+ status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
			else if(eof_selected_track == EOF_TRACK_BASS)
			{	//When a lane 1 bass note is added, open bass must be forced clear, because they use conflicting MIDI notation
				note &= ~(32);	//Clear the bit for lane 6 (open bass)
				eof_set_note_note(eof_song, eof_selected_track, i, note);
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_red(void)
{
	unsigned long i;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 2;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i);
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 2);
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_yellow(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If this note is in the currently active track, is selected and is in the active difficulty
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 4;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 4);
			}
			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If yellow drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags &= (~EOF_NOTE_FLAG_Y_CYMBAL);	//Clear the Pro yellow cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				if(eof_mark_drums_as_cymbal && (note & 4))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,1,0);	//Set the yellow cymbal flag on all drum notes at this position
				}
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_blue(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If this note is in the currently active track, is selected and is in the active difficulty
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 8;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 8);
			}
			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If blue drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags &= (~EOF_NOTE_FLAG_B_CYMBAL);	//Clear the Pro blue cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				if(eof_mark_drums_as_cymbal && (note & 8))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,1,0);	//Set the blue cymbal flag on all drum notes at this position
				}
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_purple(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If this note is in the currently active track, is selected and is in the active difficulty
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 16;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 16);
			}
			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If green drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags &= (~EOF_NOTE_FLAG_G_CYMBAL);	//Clear the Pro green cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				if(eof_mark_drums_as_cymbal && (note & 16))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,1,0);	//Set the green cymbal flag on all drum notes at this position
				}
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_orange(void)
{
	unsigned long i;
	unsigned long flags, note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_track_lanes(eof_song, eof_selected_track) < 6)
	{
		return 1;	//Don't do anything if there is less than 6 tracks available
	}
	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_selected_track == EOF_TRACK_BASS)
			{	//When an open bass note is added, all other lanes must be forced clear, because they use conflicting MIDI notation
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				eof_set_note_note(eof_song, eof_selected_track, i, 32);	//Clear all lanes except lane 6
				flags &= ~(EOF_NOTE_FLAG_CRAZY);		//Clear the crazy flag, which is invalid for open strum notes
				flags &= ~(EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO flags, which are invalid for open strum notes
				flags &= ~(EOF_NOTE_FLAG_NO_HOPO);
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
			else
			{
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, alter the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 32;
				}
				else
				{	//Otherwise alter the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
					eof_set_note_note(eof_song, eof_selected_track, i, note ^ 32);
				}
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_clear_green(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active track, is selected and is in the active difficulty
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, check the note's legacy bitmask
				note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
			}
			else
			{	//Otherwise check the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i);
			}
			if(note & 1)
			{	//If this note has a green gem
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				note &= ~1;	//Clear the 1st lane gem
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, alter the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise alter the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
			}
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_clear_red(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active track, is selected and is in the active difficulty
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, check the note's legacy bitmask
				note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
			}
			else
			{	//Otherwise check the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i);
			}
			if(note & 2)
			{	//If this note has a red gem
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				note &= ~2;	//Clear the 2nd lane gem
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, alter the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise alter the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
			}
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_clear_yellow(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active track, is selected and is in the active difficulty
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, check the note's legacy bitmask
				note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
			}
			else
			{	//Otherwise check the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i);
			}
			if(note & 3)
			{	//If this note has a yellow gem
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				note &= ~4;	//Clear the 3rd lane gem
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, alter the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise alter the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
			}
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_clear_blue(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active track, is selected and is in the active difficulty
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, check the note's legacy bitmask
				note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
			}
			else
			{	//Otherwise check the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i);
			}
			if(note & 8)
			{	//If this note has a blue gem
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				note &= ~8;	//Clear the 4th lane gem
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, alter the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise alter the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
			}
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_clear_purple(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active track, is selected and is in the active difficulty
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, check the note's legacy bitmask
				note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
			}
			else
			{	//Otherwise check the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i);
			}
			if(note & 16)
			{	//If this note has a purple gem
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				note &= ~16;	//Clear the 5th lane gem
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, alter the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise alter the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
			}
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_clear_orange(void)
{
	unsigned long i, u = 0;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_track_lanes(eof_song, eof_selected_track) < 6)
	{
		return 1;	//Don't do anything if there is less than 6 tracks available
	}
	if(eof_count_selected_notes(NULL, 0) == 0)
	{
		return 1;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active track, is selected and is in the active difficulty
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, check the note's legacy bitmask
				note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
			}
			else
			{	//Otherwise check the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i);
			}
			if(note & 32)
			{	//If this note has an orange gem
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				note &= ~32;	//Clear the 6th lane gem
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, alter the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise alter the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
			}
		}
	}
	if(u)
	{	//If at least one gem was cleared
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to ensure notes with all lanes clear are deleted
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_crazy(void)
{
	unsigned long i;
	int u = 0;	//Is set to nonzero when an undo state has been made
	unsigned long track_behavior = eof_song->track[eof_selected_track]->track_behavior;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_DANCE_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a guitar or dance track is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
			{	//If the note is not an open bass strum note (lane 6)
				if(!u)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags ^= EOF_NOTE_FLAG_CRAZY;
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_double_bass(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == EOF_NOTE_AMAZING) && (eof_get_note_note(eof_song, eof_selected_track, i) & 1))
		{	//If this note is in the currently active track, is selected, is in the Expert difficulty and has a green gem
			if(!u)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_NOTE_FLAG_DBASS;
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_double_bass(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == EOF_NOTE_AMAZING) && (eof_get_note_note(eof_song, eof_selected_track, i) & 1))
		{	//If this note is in the currently active track, is selected, is in the Expert difficulty and has a green gem
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_NOTE_FLAG_DBASS)
			{	//If this note has double bass status
				if(!u)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_NOTE_FLAG_DBASS;	//Remove double bass status
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_green(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 16)
			{	//If this drum note is purple (represents a green drum in Rock Band)
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,2,0);	//Toggle the green cymbal flag on all drum notes at this position
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_yellow(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
			{	//If this drum note is yellow
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,2,0);	//Toggle the yellow cymbal flag on all drum notes at this position
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_blue(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 8)
			{	//If this drum note is blue
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,2,0);	//Toggle the blue cymbal flag on all drum notes at this position
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_cymbal(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;
	unsigned long flags, oldflags, note;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;	//Save an extra copy of the original flags
			if(	((note & 4) && (flags & EOF_NOTE_FLAG_Y_CYMBAL)) ||
				((note & 8) && (flags & EOF_NOTE_FLAG_B_CYMBAL)) ||
				((note & 16) && (flags & EOF_NOTE_FLAG_G_CYMBAL)))
			{	//If this note has a cymbal notation
				if(!u && (oldflags != flags))
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,0,0);	//Clear the yellow cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,0,0);	//Clear the blue cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,0,0);	//Clear the green cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN,0,0);	//Clear the open hi hat cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL,0,0);	//Clear the pedal hi hat cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_SIZZLE,0,0);			//Clear the sizzle hi hat cymbal flag on all drum notes at this position
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_default_cymbal(void)
{
	if(eof_mark_drums_as_cymbal)
	{
		eof_mark_drums_as_cymbal = 0;
		eof_note_drum_menu[4].flags = 0;
	}
	else
	{
		eof_mark_drums_as_cymbal = 1;
		eof_note_drum_menu[4].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_note_default_double_bass(void)
{
	if(eof_mark_drums_as_double_bass)
	{
		eof_mark_drums_as_double_bass = 0;
		eof_note_drum_menu[7].flags = 0;
	}
	else
	{
		eof_mark_drums_as_double_bass = 1;
		eof_note_drum_menu[7].flags = D_SELECTED;
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

float eof_menu_note_push_get_offset(void)
{
	switch(eof_snap_mode)
	{
		case EOF_SNAP_QUARTER:
		{
			return 100.0;
		}
		case EOF_SNAP_EIGHTH:
		{
			return 50.0;
		}
		case EOF_SNAP_TWELFTH:
		{
			return 100.0 / 3.0;
		}
		case EOF_SNAP_SIXTEENTH:
		{
			return 25.0;
		}
		case EOF_SNAP_TWENTY_FOURTH:
		{
			return 100.0 / 6.0;
		}
		case EOF_SNAP_THIRTY_SECOND:
		{
			return 12.5;
		}
		case EOF_SNAP_FORTY_EIGHTH:
		{
			return 100.0 / 12.0;
		}
		case EOF_SNAP_CUSTOM:
		{
			return 100.0 / (float)eof_snap_interval;
		}
	}
	return 0.0;
}

int eof_menu_note_push_back(void)
{
	unsigned long i;
	float porpos;
	long beat;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) > 0)
	{	//If notes are selected
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_selection.multi[i])
			{
				beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				porpos = eof_get_porpos(eof_get_note_pos(eof_song, eof_selected_track, i));
				eof_set_note_pos(eof_song, eof_selected_track, i, eof_put_porpos(beat, porpos, eof_menu_note_push_get_offset()));
			}
		}
	}
	else
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{
			if(eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_music_pos - eof_av_delay)
			{
				beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				porpos = eof_get_porpos(eof_get_note_pos(eof_song, eof_selected_track, i));
				eof_set_note_pos(eof_song, eof_selected_track, i, eof_put_porpos(beat, porpos, eof_menu_note_push_get_offset()));
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_push_up(void)
{
	unsigned long i;
	float porpos;
	long beat;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_count_selected_notes(NULL, 0) > 0)
	{	//If notes are selected
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_selection.multi[i])
			{
				beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				porpos = eof_get_porpos(eof_get_note_pos(eof_song, eof_selected_track, i));
				eof_set_note_pos(eof_song, eof_selected_track, i, eof_put_porpos(beat, porpos, -eof_menu_note_push_get_offset()));
			}
		}
	}
	else
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_music_pos - eof_av_delay)
			{
				beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				porpos = eof_get_porpos(eof_get_note_pos(eof_song, eof_selected_track, i));
				eof_set_note_pos(eof_song, eof_selected_track, i, eof_put_porpos(beat, porpos, -eof_menu_note_push_get_offset()));
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

///Unused function
/*
int eof_menu_note_create_bre(void)
{
	unsigned long i;
	long first_pos = 0;
	long last_pos = eof_chart_length;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_get_note_pos(eof_song, eof_selected_track, i) > first_pos)
			{
				first_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
			if(eof_get_note_pos(eof_song, eof_selected_track, i) < last_pos)
			{
				last_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
		}
	}

	// create the BRE marking note
	if((first_pos != 0) && (last_pos != eof_chart_length))
	{
		(void) eof_track_add_create_note(eof_song, eof_selected_track, 31, first_pos, last_pos - first_pos, EOF_NOTE_SPECIAL, NULL);
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}
*/

/* split a lyric into multiple pieces (look for ' ' characters) */
static void eof_split_lyric(int lyric)
{
	unsigned long i, l, c = 0, lastc;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long piece = 1, pieces = 1;
	char * token = NULL;

	if(!eof_vocals_selected)
		return;

	/* see how many pieces there are */
	for(i = 0; i < (unsigned long)strlen(eof_song->vocal_track[tracknum]->lyric[lyric]->text); i++)
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
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_vocals_selected)
		return 1;
	if(eof_count_selected_notes(NULL, 0) != 1)
	{
		return 1;
	}

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_split_lyric_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_split_lyric_dialog);
	(void) ustrcpy(eof_etext, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text);
	if(eof_popup_dialog(eof_split_lyric_dialog, 2) == 3)
	{
		if(ustricmp(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext))
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			(void) ustrcpy(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext);
			eof_split_lyric(eof_selection.current);
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return D_O_K;
}

int eof_menu_solo_mark(void)
{
	unsigned long i, j;
	long insp = -1;
	long sel_start = -1;
	long sel_end = 0;
	EOF_PHRASE_SECTION *soloptr = NULL;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
			{
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
			if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
			{
				sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
			}
		}
	}
	for(j = 0; j < eof_get_num_solos(eof_song, eof_selected_track); j++)
	{	//For each solo in the track
		soloptr = eof_get_solo(eof_song, eof_selected_track, j);
		if((sel_end >= soloptr->start_pos) && (sel_start <= soloptr->end_pos))
		{
			insp = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(insp < 0)
	{
		(void) eof_track_add_solo(eof_song, eof_selected_track, sel_start, sel_end);
	}
	else
	{
		soloptr = eof_get_solo(eof_song, eof_selected_track, insp);
		soloptr->start_pos = sel_start;
		soloptr->end_pos = sel_end;
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_solo_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *soloptr = NULL;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_solo_erase_all(void)
{
	if(eof_get_num_solos(eof_song, eof_selected_track) && (alert(NULL, "Erase all solos from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1))
	{	//If the active track has solos and the user opts to delete them
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_solos(eof_song, eof_selected_track, 0);
	}
	return 1;
}

int eof_menu_star_power_mark(void)
{
	unsigned long i, j;
	long insp = -1;
	long sel_start = -1;
	long sel_end = 0;
	EOF_PHRASE_SECTION *starpowerptr = NULL;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
			{
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
			if(eof_get_note_pos(eof_song, eof_selected_track, i) > sel_end)
			{
				sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + (eof_get_note_length(eof_song, eof_selected_track, i) > 20 ? eof_get_note_length(eof_song, eof_selected_track, i) : 20);
			}
		}
	}
	for(j = 0; j < eof_get_num_star_power_paths(eof_song, eof_selected_track); j++)
	{	//For each star power path in the active track
		starpowerptr = eof_get_star_power_path(eof_song, eof_selected_track, j);
		if((sel_end >= starpowerptr->start_pos) && (sel_start <= starpowerptr->end_pos))
		{
			insp = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(insp < 0)
	{
		(void) eof_track_add_star_power_path(eof_song, eof_selected_track, sel_start, sel_end);
	}
	else
	{
		starpowerptr = eof_get_star_power_path(eof_song, eof_selected_track, insp);
		if(starpowerptr != NULL)
		{
			starpowerptr->start_pos = sel_start;
			starpowerptr->end_pos = sel_end;
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_star_power_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *starpowerptr = NULL;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_star_power_erase_all(void)
{
	if(eof_get_num_star_power_paths(eof_song, eof_selected_track) && (alert(NULL, "Erase all star power from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1))
	{	//If the active track has star power sections and the user opts to delete them
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_star_power_paths(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;
}

int eof_menu_lyric_line_mark(void)
{
	unsigned long i, j;
	unsigned long tracknum;
	long sel_start = -1;
	long sel_end = 0;
	int originalflags = 0; //Used to apply the line's original flags after the line is recreated
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_song || !eof_vocals_selected)
		return 1;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{
			if(eof_song->vocal_track[tracknum]->lyric[i]->pos < sel_start)
			{
				sel_start = eof_song->vocal_track[tracknum]->lyric[i]->pos;
				if(sel_start < eof_song->tags->ogg[eof_selected_ogg].midi_offset)
				{
					sel_start = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
				}
			}
			if(eof_song->vocal_track[tracknum]->lyric[i]->pos > sel_end)
			{
				sel_end = eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length;
				if(sel_end >= eof_chart_length)
				{
					sel_end = eof_chart_length - 1;
				}
			}
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Create the undo state before removing/adding phrase(s)
	for(j = eof_song->vocal_track[tracknum]->lines; j > 0; j--)
	{
		if((sel_end >= eof_song->vocal_track[tracknum]->line[j-1].start_pos) && (sel_start <= eof_song->vocal_track[tracknum]->line[j-1].end_pos))
		{
			originalflags=eof_song->vocal_track[tracknum]->line[j-1].flags;	//Save this line's flags before deleting it
			eof_vocal_track_delete_line(eof_song->vocal_track[tracknum], j-1);
		}
	}
	(void) eof_vocal_track_add_line(eof_song->vocal_track[tracknum], sel_start, sel_end);

	if(eof_song->vocal_track[tracknum]->lines >0)
		eof_song->vocal_track[tracknum]->line[eof_song->vocal_track[tracknum]->lines-1].flags = originalflags;	//Restore the line's flags

	/* check for overlapping lines */
	for(i = 0; i < eof_song->vocal_track[tracknum]->lines; i++)
	{
		for(j = i; j < eof_song->vocal_track[tracknum]->lines; j++)
		{
			if((i != j) && (eof_song->vocal_track[tracknum]->line[i].start_pos <= eof_song->vocal_track[tracknum]->line[j].end_pos) && (eof_song->vocal_track[tracknum]->line[j].start_pos <= eof_song->vocal_track[tracknum]->line[i].end_pos))
			{
				eof_song->vocal_track[tracknum]->line[i].start_pos = eof_song->vocal_track[tracknum]->line[j].end_pos + 1;
			}
		}
	}
	eof_reset_lyric_preview_lines();
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_lyric_line_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_vocals_selected)
		return 1;

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_lyric_line_erase_all(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_vocals_selected)
		return 1;

	if(alert(NULL, "Erase all lyric lines?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->vocal_track[tracknum]->lines = 0;
		eof_reset_lyric_preview_lines();
	}
	return 1;
}

int eof_menu_lyric_line_toggle_overdrive(void)
{
	char used[1024] = {0};
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_vocals_selected)
		return 1;

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_hopo_auto(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_selected_track == EOF_TRACK_DRUM) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when PART DRUMS or PART VOCALS is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_selection.multi[i])
		{	//If the note is selected
			if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
			{	//If the note is not an open bass strum note
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				oldflags = flags;					//Save another copy of the original flags
				flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO on flag
				flags &= (~EOF_NOTE_FLAG_NO_HOPO);	//Clear the HOPO off flag
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If this is a pro guitar note, ensure that various statuses are cleared (they would be invalid if this note was a HO/PO)
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
				}
				if(!undo_made && (flags != oldflags))
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_hopo_cycle(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	unsigned long track_behavior = eof_song->track[eof_selected_track]->track_behavior;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a guitar track is active

	if((eof_count_selected_notes(NULL, 0) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_selection.multi[i])
			{	//If the note is selected
				if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
				{	//If the note is not an open bass strum note
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
						if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
						{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
						}
					}
					else if(flags & EOF_NOTE_FLAG_NO_HOPO)
					{	//If the note was a forced off HOPO, make it an auto HOPO
						flags &= ~EOF_NOTE_FLAG_F_HOPO;		//Clear the forced on hopo flag
						flags &= ~EOF_NOTE_FLAG_NO_HOPO;	//Clear the forced off hopo flag
						if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
						{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
						}
					}
					else
					{	//If the note was an auto HOPO, make it a forced on HOPO
						flags |= EOF_NOTE_FLAG_F_HOPO;	//Turn on forced on hopo
						if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
						{	//If this is a pro guitar note
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Set the hammer on flag (default HOPO type)
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
							flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
						}
					}
					eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				}
			}
		}
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_hopo_force_on(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_selected_track == EOF_TRACK_DRUM) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when PART DRUMS or PART VOCALS is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_selection.multi[i])
		{
			if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
			{	//If the note is not an open bass strum note
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				oldflags = flags;					//Save another copy of the original flags
				flags |= EOF_NOTE_FLAG_F_HOPO;		//Set the HOPO on flag
				flags &= (~EOF_NOTE_FLAG_NO_HOPO);	//Clear the HOPO off flag
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If this is a pro guitar note
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Set the hammer on flag (default HOPO type)
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
				}
				if(!undo_made && (flags != oldflags))
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_hopo_force_off(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_selected_track == EOF_TRACK_DRUM) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when PART DRUMS or PART VOCALS is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_selection.multi[i])
		{
			if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
			{	//If the note is not an open bass strum note
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				oldflags = flags;					//Save another copy of the original flags
				flags |= EOF_NOTE_FLAG_NO_HOPO;		//Set the HOPO off flag
				flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO on flag
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If this is a pro guitar note, ensure that Hammer on, Pull off, Tap and bend statuses are cleared
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
					flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
				}
				if(!undo_made && (flags != oldflags))
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_transpose_possible(int dir)
{
	unsigned long i, note, tracknum;
	unsigned long max = 16;	//This represents the highest note bitmask value that will be allowed to transpose up, based on the current track options (including open bass strumming)
	int retval = 1;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	/* no notes, no transpose */
	if(eof_vocals_selected)
	{
		if(eof_get_track_size(eof_song, eof_selected_track) == 0)
		{
			retval = 0;
		}
		else if(eof_count_selected_notes(NULL, 1) == 0)
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
					}
					else if(eof_get_note_note(eof_song, eof_selected_track, i) - dir < MINPITCH)
					{
						retval = 0;
					}
					else if(eof_get_note_note(eof_song, eof_selected_track, i) - dir > MAXPITCH)
					{
						retval = 0;
					}
				}
			}
		}
	}
	else
	{
		if(eof_open_bass_enabled() || (eof_count_track_lanes(eof_song, eof_selected_track) > 5))
		{	//If open bass is enabled, or the track has more than 5 lanes, lane 5 can transpose up to lane 6
			max = 32;
		}
		if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
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
		else if(eof_count_selected_notes(NULL, 1) == 0)
		{
			retval = 0;
		}
		else
		{
			tracknum = eof_song->track[eof_selected_track]->tracknum;
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the active track
				if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
				{	//If the note is selected and in the active instrument difficulty
					if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
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
					}
					else if((note & 1) && (dir > 0))
					{
						retval = 0;
					}
					else if((note & max) && (dir < 0))
					{
						retval = 0;
					}
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
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
	centre_dialog(eof_lyric_dialog);
	(void) ustrcpy(eof_etext, "");

	if(eof_pen_lyric.note != EOF_LYRIC_PERCUSSION)		//If not entering a percussion note
	{
		ret = eof_popup_dialog(eof_lyric_dialog, 2);	//prompt for lyric text
		if(!eof_check_string(eof_etext) && !eof_pen_lyric.note)	//If the placed lyric is both pitchless AND textless
			return D_O_K;	//Return without adding
	}

	if((ret == 3) || (eof_pen_lyric.note == EOF_LYRIC_PERCUSSION))
	{
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
		eof_detect_difficulties(eof_song);
		eof_reset_lyric_preview_lines();
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_edit_lyric_dialog(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_vocals_selected)
		return 1;

	if(eof_count_selected_notes(NULL, 0) != 1)
	{
		return 1;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_lyric_dialog);
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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return D_O_K;
}

int eof_menu_set_freestyle(char status)
{
	unsigned long i=0,ctr=0;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_vocals_selected)
		return 1;

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
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
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_vocals_selected && (eof_selection.track == EOF_TRACK_VOCALS) && eof_count_selected_notes(NULL, 0))
	{	//If lyrics are selected
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state

		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{	//For each lyric...
			if(eof_selection.multi[i])
			{	//...that is selected, toggle its freestyle status
				eof_toggle_freestyle(eof_song->vocal_track[tracknum],i);
			}
		}
	}

	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

char eof_note_edit_name[EOF_NAME_LENGTH+1] = {0};

DIALOG eof_pro_guitar_note_dialog[] =
{
/*	(proc)					(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          (dp2)          (dp3) */
	{d_agup_window_proc,    0,   48,  230, 392,2,   23,  0,    0,      0,         0,   "Edit pro guitar note",NULL, NULL },
	{d_agup_text_proc,      16,  80,  64,  8,  2,   23,  0,    0,      0,         0,   "Name:",      NULL,          NULL },
	{d_agup_edit_proc,		74,  76,  134, 20, 2,   23,  0,    0, EOF_NAME_LENGTH,0,eof_note_edit_name,       NULL, NULL },

	//Note:  In guitar theory, string 1 refers to high e
	{d_agup_text_proc,      16,  128, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_6_number, NULL, NULL },
	{eof_verified_edit_proc,74,  124, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_6,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  152, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_5_number, NULL, NULL },
	{eof_verified_edit_proc,74,  148, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_5,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  176, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_4_number, NULL, NULL },
	{eof_verified_edit_proc,74,  172, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_4,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  200, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_3_number, NULL, NULL },
	{eof_verified_edit_proc,74,  196, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_3,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  224, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_2_number, NULL, NULL },
	{eof_verified_edit_proc,74,  220, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_2,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  248, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_1_number, NULL, NULL },
	{eof_verified_edit_proc,74,  244, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_1,  "0123456789Xx",NULL },

	{d_agup_text_proc,      16,  108, 64,  8,  2,   23,  0,    0,      0,         0,   "Fret #",     NULL,          NULL },
	{d_agup_text_proc,      176, 127, 64,  8,  2,   23,  0,    0,      0,         0,   "Legacy",     NULL,          NULL },
	{d_agup_check_proc,		158, 151, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 5",     NULL,          NULL },
	{d_agup_check_proc,		158, 175, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 4",     NULL,          NULL },
	{d_agup_check_proc,		158, 199, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 3",     NULL,          NULL },
	{d_agup_check_proc,		158, 223, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 2",     NULL,          NULL },
	{d_agup_check_proc,		158, 247, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 1",     NULL,          NULL },

	{d_agup_text_proc,      98,  108, 64,  8,  2,   23,  0,    0,      0,         0,   "Ghost",      NULL,          NULL },
	{d_agup_check_proc,		100, 127, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 151, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 175, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 199, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 223, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 247, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },

	{d_agup_text_proc,      140, 108, 64,  8,  2,   23,  0,    0,      0,         0,   "Mute",       NULL,          NULL },
	{d_agup_check_proc,		140, 127, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		140, 151, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		140, 175, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		140, 199, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		140, 223, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		140, 247, 20,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },

	{d_agup_text_proc,      10,  312, 64,  8,  2,   23,  0,    0,      0,         0,   "Slide:",     NULL,          NULL },
	{d_agup_check_proc,		58,  332, 150, 16, 2,   23,  0,    0,      0,         0,   "Reverse slide",NULL,       NULL },
	{d_agup_text_proc,      10,  352, 64,  8,  2,   23,  0,    0,      0,         0,   "Mute:",      NULL,          NULL },
	{d_agup_text_proc,      10,  372, 64,  8,  2,   23,  0,    0,      0,         0,   "Strum:",     NULL,          NULL },
	{d_agup_radio_proc,		6,   272, 38,  16, 2,   23,  0,    0,      1,         0,   "HO",         NULL,          NULL },
	{d_agup_radio_proc,		43,  272, 38,  16, 2,   23,  0,    0,      1,         0,   "PO",         NULL,          NULL },
	{d_agup_radio_proc,		78,  272, 45,  16, 2,   23,  0,    0,      1,         0,   "Tap",        NULL,          NULL },
	{d_agup_radio_proc,		120, 272, 50,  16, 2,   23,  0,    0,      1,         0,   "Bend",       NULL,          NULL },
	{d_agup_radio_proc,		6,   292, 80,  16, 2,   23,  0,    0,      1,         0,   "Harmonic",   NULL,          NULL },
	{d_agup_radio_proc,		85,  292, 64,  16, 2,   23,  0,    0,      1,         0,   "Vibrato",    NULL,          NULL },
	{d_agup_radio_proc,		154, 292, 50,  16, 2,   23,  0,    0,      1,         0,   "None",       NULL,          NULL },
	{d_agup_radio_proc,		58,  312, 38,  16, 2,   23,  0,    0,      2,         0,   "Up",         NULL,          NULL },
	{d_agup_radio_proc,		102, 312, 54,  16, 2,   23,  0,    0,      2,         0,   "Down",       NULL,          NULL },
	{d_agup_radio_proc,		154, 312, 64,  16, 2,   23,  0,    0,      2,         0,   "Neither",    NULL,          NULL },
	{d_agup_radio_proc,		46,  352, 58,  16, 2,   23,  0,    0,      3,         0,   "String",     NULL,          NULL },
	{d_agup_radio_proc,		102, 352, 52,  16, 2,   23,  0,    0,      3,         0,   "Palm",       NULL,          NULL },
	{d_agup_radio_proc,		154, 352, 64,  16, 2,   23,  0,    0,      3,         0,   "Neither",    NULL,          NULL },
	{d_agup_radio_proc,		49,  372, 38,  16, 2,   23,  0,    0,      4,         0,   "Up",         NULL,          NULL },
	{d_agup_radio_proc,		85,  372, 46,  16, 2,   23,  0,    0,      4,         0,   "Mid",        NULL,          NULL },
	{d_agup_radio_proc,		128, 372, 54,  16, 2,   23,  0,    0,      4,         0,   "Down",       NULL,          NULL },
	{d_agup_radio_proc,		180, 372, 46,  16, 2,   23,  0,    0,      4,         0,   "Any",        NULL,          NULL },

	{d_agup_button_proc,    10,  400, 20,  28, 2,   23,  0,    D_EXIT, 0,         0,   "<-",         NULL,          NULL },
	{d_agup_button_proc,    35,  400, 50,  28, 2,   23,  '\r', D_EXIT, 0,         0,   "OK",         NULL,          NULL },
	{d_agup_button_proc,    90,  400, 50,  28, 2,   23,  0,    D_EXIT, 0,         0,   "Apply",      NULL,          NULL },
	{d_agup_button_proc,    145, 400, 50,  28, 2,   23,  0,    D_EXIT, 0,         0,   "Cancel",     NULL,          NULL },
	{d_agup_button_proc,    200, 400, 20,  28, 2,   23,  0,    D_EXIT, 0,         0,   "->",         NULL,          NULL },
	{NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_note_edit_pro_guitar_note(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long ctr, ctr2, stringcount, i;
	char undo_made = 0;	//Set to nonzero when an undo state is created
	long fretvalue;
	char allmuted;					//Used to track whether all used strings are string muted
	unsigned long bitmask = 0;		//Used to build the updated pro guitar note bitmask
	unsigned char legacymask;		//Used to build the updated legacy note bitmask
	unsigned char ghostmask;		//Used to build the updated ghost bitmask
	unsigned long flags;			//Used to build the updated flag bitmask
	EOF_PRO_GUITAR_NOTE junknote;	//Just used with sizeof() to get the name string's length to guarantee a safe string copy
	char *newname = NULL, *tempptr;
	char autoprompt[100] = {0};
	char previously_refused;
	char declined_list[EOF_MAX_NOTES] = {0};	//This lists all notes whose names/legacy bitmasks the user declined to have applied to the edited note
	char autobitmask[10] = {0};
	unsigned long index = 0;
	char pro_guitar_string[30] = {0};
	long previous_note = 0, next_note = 0;
	int retval;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless the pro guitar track is active
	if(eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run if a valid note isn't selected

	if(!eof_music_paused)
	{
		eof_music_play();
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pro_guitar_note_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_pro_guitar_note_dialog);

	do
	{	//Prepare the dialog
		retval = 0;
	//Update the note name text box to match the last selected note
		memcpy(eof_note_edit_name, eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->name, sizeof(eof_note_edit_name));

	//Find the next/previous notes if applicable
		previous_note = eof_track_fixup_previous_note(eof_song, eof_selected_track, eof_selection.current);
		if(previous_note >= 0)
		{	//If there is a previous note
			eof_pro_guitar_note_dialog[57].flags = D_EXIT;		//Make the previous note button clickable
		}
		else
		{
			eof_pro_guitar_note_dialog[57].flags = D_HIDDEN;	//Otherwise hide it
		}
		next_note = eof_track_fixup_next_note(eof_song, eof_selected_track, eof_selection.current);
		if(next_note >= 0)
		{	//If there is a next note
			eof_pro_guitar_note_dialog[61].flags = D_EXIT;		//Make the next note button clickable
		}
		else
		{
			eof_pro_guitar_note_dialog[61].flags = D_HIDDEN;	//Otherwise hide it
		}

	//Update the fret text boxes (listed from top to bottom as string 1 through string 6)
		stringcount = eof_count_track_lanes(eof_song, eof_selected_track);
		if(eof_legacy_view)
		{	//Special case:  If legacy view is enabled, correct stringcount
			stringcount = eof_song->pro_guitar_track[tracknum]->numstrings;
		}
		ghostmask = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->ghost;
		for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask<<=1)
		{	//For each of the 6 supported strings
			if(ctr < stringcount)
			{	//If this track uses this string, copy the fret value to the appropriate string
				eof_pro_guitar_note_dialog[13 - (2 * ctr)].flags = 0;	//Ensure this text boxes' label is enabled
				eof_fret_string_numbers[ctr][7] = '0' + (stringcount - ctr);	//Correct the string number for this label
				eof_pro_guitar_note_dialog[14 - (2 * ctr)].flags = 0;	//Ensure this text box is enabled
				eof_pro_guitar_note_dialog[28 - ctr].flags = (ghostmask & bitmask) ? D_SELECTED : 0;	//Ensure the ghost check box is enabled and cleared/checked appropriately
				eof_pro_guitar_note_dialog[35 - ctr].flags = 0;		//Ensure the mute check box is enabled and cleared (it will be checked below if applicable)
				if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->note & bitmask)
				{	//If this string is already defined as being in use, copy its fret value to the string
					if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr] & 0x80)
					{	//If this string is muted (most significant bit is set)
						eof_pro_guitar_note_dialog[35 - ctr].flags = D_SELECTED;	//Check the string's muted checkbox
					}
					if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr] == 0xFF)
					{	//If this string is muted with no fret value specified
						(void) snprintf(eof_fret_strings[ctr], sizeof(eof_fret_strings[ctr]) - 1, "X");
					}
					else
					{
						(void) snprintf(eof_fret_strings[ctr], sizeof(eof_fret_strings[ctr]) - 1, "%d", eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr] & 0x7F);	//Mask out the MSB to obtain the fret value
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
				eof_fret_strings[ctr][0] = '\0';
			}
		}

	//Update the legacy bitmask checkboxes
		legacymask = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->legacymask;
		eof_pro_guitar_note_dialog[17].flags = (legacymask & 16) ? D_SELECTED : 0;
		eof_pro_guitar_note_dialog[18].flags = (legacymask & 8) ? D_SELECTED : 0;
		eof_pro_guitar_note_dialog[19].flags = (legacymask & 4) ? D_SELECTED : 0;
		eof_pro_guitar_note_dialog[20].flags = (legacymask & 2) ? D_SELECTED : 0;
		eof_pro_guitar_note_dialog[21].flags = (legacymask & 1) ? D_SELECTED : 0;

	//Clear the reverse slide checkbox
		eof_pro_guitar_note_dialog[37].flags = 0;

	//Update the note flag radio buttons
		for(ctr = 0; ctr < 16; ctr++)
		{	//Clear each of the 16 status radio buttons
			eof_pro_guitar_note_dialog[40 + ctr].flags = 0;
		}
		flags = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->flags;
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
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
		{	//Select "Bend"
			eof_pro_guitar_note_dialog[43].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
		{	//Select "Harmonic"
			eof_pro_guitar_note_dialog[44].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
		{	//Select "Vibrato"
			eof_pro_guitar_note_dialog[45].flags = D_SELECTED;
		}
		else
		{	//Select "None"
			eof_pro_guitar_note_dialog[46].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
		{	//Select Slide "Up"
			eof_pro_guitar_note_dialog[47].flags = D_SELECTED;
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)
				eof_pro_guitar_note_dialog[37].flags = D_SELECTED;	//Reverse slide
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
		{	//Select Slide "Down"
			eof_pro_guitar_note_dialog[48].flags = D_SELECTED;
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)
				eof_pro_guitar_note_dialog[37].flags = D_SELECTED;
		}
		else
		{	//Select Slide "Neither"
			eof_pro_guitar_note_dialog[49].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)
		{	//Select Mute "String"
			eof_pro_guitar_note_dialog[50].flags = D_SELECTED;
		}
		else if(flags &EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
		{	//Select Mute "Palm"
			eof_pro_guitar_note_dialog[51].flags = D_SELECTED;
		}
		else
		{	//Select Mute "Neither"
			eof_pro_guitar_note_dialog[52].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
		{	//Select Strum "Up"
			eof_pro_guitar_note_dialog[53].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM)
		{	//Select Strum "Mid"
			eof_pro_guitar_note_dialog[54].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM)
		{	//Select Strum "Down"
			eof_pro_guitar_note_dialog[55].flags = D_SELECTED;
		}
		else
		{	//Select Strum "Any"
			eof_pro_guitar_note_dialog[56].flags = D_SELECTED;
		}

		bitmask = 0;
		retval = eof_popup_dialog(eof_pro_guitar_note_dialog, 0);	//Run the dialog
		if((retval == 58) || (retval == 59))
		{	//If user clicked OK or Apply
			//Validate and store the input
			if(eof_note_edit_name[0] != '\0')
			{	//If the user specified a name, ensure it does not include brackets
				for(i = 0; i < ustrlen(eof_note_edit_name); i++)
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

			if(eof_count_selected_notes(NULL, 0) > 1)
			{	//If multiple notes are selected, warn the user
				if(alert(NULL, "Warning:  This information will be applied to all selected notes.", NULL, "&OK", "&Cancel", 'y', 'n') == 2)
				{	//If user opts to cancel the operation
					if(note_selection_updated)
					{	//If the only note modified was the seek hover note
						eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
						eof_selection.current = EOF_MAX_NOTES - 1;
					}
					eof_show_mouse(NULL);
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					return 1;
				}
			}

			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the track
				if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
				{	//If the note is in the active instrument difficulty and is selected
	//Save the updated note name  (listed from top to bottom as string 1 through string 6)
					if(ustrcmp(eof_note_edit_name, eof_song->pro_guitar_track[tracknum]->note[i]->name))
					{	//If the name was changed
						if(!undo_made)
						{
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						memcpy(eof_song->pro_guitar_track[tracknum]->note[i]->name, eof_note_edit_name, sizeof(eof_note_edit_name));
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
								if((fretvalue < 0) || (fretvalue > eof_song->pro_guitar_track[tracknum]->numfrets))
								{	//If the conversion to number failed, or an invalid fret number was entered, enter a value of (muted) for the string
									fretvalue = 0xFF;
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
							if(fretvalue != eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr])
							{	//If this fret value changed
								if(!undo_made)
								{	//If an undo state hasn't been made yet
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									undo_made = 1;
								}
								eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] = fretvalue;
								memset(eof_song->pro_guitar_track[tracknum]->note[i]->finger, 0, 8);	//Initialize all fingers to undefined
							}
							if(fretvalue != 0xFF)
							{	//Track whether the all used strings in this note/chord are muted
								allmuted = 0;
							}
						}
						else
						{	//Clear the fret value and return the fret back to its default of 0 (open)
							bitmask &= ~(1 << ctr);	//Clear the appropriate bit for this lane
							if(eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] != 0)
							{	//If this fret value changed
								if(!undo_made)
								{	//If an undo state hasn't been made yet
									eof_prepare_undo(EOF_UNDO_TYPE_NONE);
									undo_made = 1;
								}
								eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] = 0;
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
					if(eof_song->pro_guitar_track[tracknum]->note[i]->note != bitmask)
					{	//If the note bitmask changed
						if(!undo_made)
						{	//If an undo state hasn't been made yet
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_song->pro_guitar_track[tracknum]->note[i]->note = bitmask;
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
					if(legacymask != eof_song->pro_guitar_track[tracknum]->note[i]->legacymask)
					{	//If the legacy bitmask changed
						if(!undo_made)
						{	//If an undo state hasn't been made yet
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = legacymask;
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
					if(ghostmask != eof_song->pro_guitar_track[tracknum]->note[i]->ghost)
					{	//If the ghost mask changed
						if(!undo_made)
						{	//If an undo state hasn't been made yet
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_song->pro_guitar_track[tracknum]->note[i]->ghost = ghostmask;
					}

	//Save the updated note flag bitmask
					flags = flags & (EOF_NOTE_FLAG_HOPO | EOF_NOTE_FLAG_SP);	//Clear the flags variable, except retain the note's existing SP and HOPO flag statuses
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
					else if(eof_pro_guitar_note_dialog[43].flags == D_SELECTED)
					{	//Bend is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;			//Set the bend flag
					}
					else if(eof_pro_guitar_note_dialog[44].flags == D_SELECTED)
					{	//Harmonic is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;		//Set the harmonic flag
					}
					else if(eof_pro_guitar_note_dialog[45].flags == D_SELECTED)
					{	//Vibrato is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Set the vibrato flag
					}
					if(eof_pro_guitar_note_dialog[47].flags == D_SELECTED)
					{	//Slide Up is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
						if(eof_pro_guitar_note_dialog[37].flags == D_SELECTED)	//Reverse slide is selected
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE;
					}
					else if(eof_pro_guitar_note_dialog[48].flags == D_SELECTED)
					{	//Slide Down is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
						if(eof_pro_guitar_note_dialog[37].flags == D_SELECTED)	//Reverse slide is selected
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE;
					}
					if(eof_pro_guitar_note_dialog[50].flags == D_SELECTED)
					{	//Mute String is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
					}
					else if(eof_pro_guitar_note_dialog[51].flags == D_SELECTED)
					{	//Mute Palm is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
					}
					if(eof_pro_guitar_note_dialog[53].flags == D_SELECTED)
					{	//Strum Up is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
					}
					else if(eof_pro_guitar_note_dialog[54].flags == D_SELECTED)
					{	//Strum Mid is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM;
					}
					else if(eof_pro_guitar_note_dialog[55].flags == D_SELECTED)
					{	//Strum Down is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;
					}
					if(!allmuted)
					{	//If any used strings in this note/chord weren't string muted
						flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE);	//Clear the string mute flag
					}
					else if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
					{	//If all strings are muted and the user didn't specify a palm mute
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;		//Set the string mute flag
					}
					flags |= (eof_song->pro_guitar_track[tracknum]->note[i]->flags & EOF_NOTE_FLAG_CRAZY);	//Retain the note's original crazy status
					if(flags != eof_song->pro_guitar_track[tracknum]->note[i]->flags)
					{	//If the flags changed
						if(!undo_made)
						{	//If an undo state hasn't been made yet
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_song->pro_guitar_track[tracknum]->note[i]->flags = flags;
					}
				}//If the note is in the active instrument difficulty and is selected
			}//For each note in the track

	//Prompt whether matching notes need to have their name updated
			if(eof_note_edit_name[0] != '\0')
			{	//If the user entered a name
				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the active track
					if((eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) == 0) && ustrcmp(eof_note_edit_name, eof_get_note_name(eof_song, eof_selected_track, ctr)))
					{	//If this note matches the one that was edited but the name is different
						if(alert(NULL, "Update other matching notes in this track to have the same name?", NULL, "&Yes", "&No", 'y', 'n') == 1)
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
									(void) ustrncpy(eof_get_note_name(eof_song, eof_selected_track, ctr), eof_note_edit_name, (int)sizeof(junknote.name));
								}
							}
						}
						break;	//Break from loop
					}
				}//For each note in the active track
			}//If the user entered a name

	//Or prompt whether the selected notes' name should be updated from the other existing notes
			else
			{	//The user did not enter a name
				memset(declined_list, 0, sizeof(declined_list));	//Clear the declined list
				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the active track
					if((ctr != eof_selection.current) && (eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) == 0))
					{	//If this note isn't the one that was just edited, but it matches it
						newname = eof_get_note_name(eof_song, eof_selected_track, ctr);
						if(newname && (newname[0] != '\0'))
						{	//If this note has a name
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
							if(!previously_refused)
							{	//If this name isn't one the user already refused
								if(eof_get_pro_guitar_note_fret_string(eof_song->pro_guitar_track[tracknum], eof_selection.current, pro_guitar_string))
								{	//If the note's frets can be represented in string format, specify it in the prompt
									(void) snprintf(autoprompt, sizeof(autoprompt) - 1, "Set the name of selected notes (%s) to \"%s\"?",pro_guitar_string, newname);
								}
								else
								{	//Otherwise use a generic prompt
									(void) snprintf(autoprompt, sizeof(autoprompt) - 1, "Set selected notes' name to \"%s\"?",newname);
								}
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
												(void) ustrncpy(tempptr, newname, (int)sizeof(junknote.name));	//Update the note's name to the user selection
											}
										}
									}
									break;	//Break from "For each note in the active track" loop
								}
								else
								{	//Otherwise mark this note's name as refused
									declined_list[ctr] = 1;	//Mark this note's name as having been declined
								}
							}//If this name isn't one the user already refused
						}//If this note has a name
					}//If this note isn't the one that was just edited, but it matches it
				}//For each note in the active track
			}//The user did not enter a name

	//Prompt whether matching notes need to have their legacy bitmask updated
			if(legacymask)
			{	//If the user entered a legacy bitmask
				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the active track
					if((ctr != eof_selection.current) && (eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type) && (eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) == 0))
					{	//If this note isn't the one that was just edited, but it matches it and is in the active track difficulty
						if(legacymask != eof_song->pro_guitar_track[tracknum]->note[ctr]->legacymask)
						{	//If the two notes have different legacy bitmasks
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
										eof_song->pro_guitar_track[tracknum]->note[ctr]->legacymask = legacymask;
									}
								}
							}
							break;	//Break from loop
						}
					}
				}//For each note in the active track
			}//If the user entered a legacy bitmask

	//Or prompt whether the selected notes' legacy bitmask should be updated from the other existing notes
			else
			{	//The user did not enter a legacy bitmask
				memset(declined_list, 0, sizeof(declined_list));	//Clear the declined list
				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the active track
					if((ctr != eof_selection.current) && (eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr) == 0))
					{	//If this note isn't the one that was just edited, but it matches it
						legacymask = eof_song->pro_guitar_track[tracknum]->note[ctr]->legacymask;
						if(legacymask)
						{	//If this note has a legacy bitmask
							previously_refused = 0;
							for(ctr2 = 0; ctr2 < ctr; ctr2++)
							{	//For each previous note that was checked
								if(declined_list[ctr2] && (legacymask == eof_song->pro_guitar_track[tracknum]->note[ctr2]->legacymask))
								{	//If this note's legacy mask matches one the user previously rejected to assign to the edited note
									declined_list[ctr] = 1;	//Automatically decline this instance of the same legacy bitmask
									previously_refused = 1;
									break;
								}
							}
							if(!previously_refused)
							{	//If this legacy bitmask isn't one the user already refused
								for(ctr2 = 0, bitmask = 1, index = 0; ctr2 < 5; ctr2++, bitmask<<=1)
								{	//For each of the legacy bitmasks 5 usable bits
									if(legacymask & bitmask)
									{	//If this bit is set, append the fret number to the autobitmask string
										autobitmask[index++] = '1' + ctr2;
									}
									autobitmask[index] = '\0';	//Ensure the string is terminated
								}
								if(eof_get_pro_guitar_note_fret_string(eof_song->pro_guitar_track[tracknum], eof_selection.current, pro_guitar_string))
								{	//If the note's frets can be represented in string format, specify it in the prompt
									(void) snprintf(autoprompt, sizeof(autoprompt) - 1, "Set the legacy bitmask of selected notes (%s) to \"%s\"?",pro_guitar_string, autobitmask);
								}
								else
								{	//Otherwise use a generic prompt
									(void) snprintf(autoprompt, sizeof(autoprompt) - 1, "Set selected notes' legacy bitmask to \"%s\"?",autobitmask);
								}
								if(alert(NULL, autoprompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
								{	//If the user opts to assign this note's legacy bitmask to the edited note
									for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, eof_selected_track); ctr2++)
									{	//For each note in the track
										if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr2] && (eof_get_note_type(eof_song, eof_selected_track, ctr2) == eof_note_type))
										{	//If the note is in the active instrument difficulty and is selected
											if(legacymask != eof_song->pro_guitar_track[tracknum]->note[ctr2]->legacymask)
											{	//If the note's legacy mask doesn't match the one the user selected from the prompt
												if(!undo_made)
												{
													eof_prepare_undo(EOF_UNDO_TYPE_NONE);
													undo_made = 1;
												}
												eof_song->pro_guitar_track[tracknum]->note[ctr2]->legacymask = legacymask;	//Update the note's legacy mask to the user selection
											}
										}
									}
									break;	//Break from "For each note in the active track" loop
								}
								else
								{	//Otherwise mark this note's name as refused
									declined_list[ctr] = 1;	//Mark this note's name as having been declined
								}
							}
						}//If this note has a legacy bitmask
					}//If this note isn't the one that was just edited, but it matches it
				}//For each note in the active track
			}//The user did not enter a legacy bitmask
			if(retval == 59)
			{	//If the user clicked Apply, re-render the screen to reflect any changes made
				eof_render();
			}
		}//If user clicked OK or Apply
		else if(retval == 57)
		{	//If user clicked <- (previous note)
			memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
			eof_selection.current = previous_note;	//Set the previous note as the currently selected note
			eof_selection.multi[previous_note] = 1;	//Ensure the note selection includes the previous note
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, previous_note) + eof_av_delay);	//Seek to previous note
			eof_render();	//Redraw the screen
		}
		else if(retval == 61)
		{	//If user clicked -> (next note)
			memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
			eof_selection.current = next_note;	//Set the next note as the currently selected note
			eof_selection.multi[next_note] = 1;	//Ensure the note selection includes the next note
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, next_note) + eof_av_delay);	//Seek to next note
			eof_render();	//Redraw the screen
		}
	}while((retval == 57) || (retval == 59) || (retval == 61));	//Re-run this dialog if the user clicked previous, apply or next

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
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

DIALOG eof_pro_guitar_note_frets_dialog[] =
{
/*	(proc)					(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          (dp2)          (dp3) */
	{d_agup_window_proc,    0,   48,  184, 248,2,   23,  0,    0,      0,         0,   "Edit note frets / fingering",NULL, NULL },

	//Note:  In guitar theory, string 1 refers to high e
	{d_agup_text_proc,      60,  80,  64,  8,  2,   23,  0,    0,      0,         0,   "Fret #",     NULL,          NULL },
	{d_agup_text_proc,      16,  108, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_6_number, NULL, NULL },
	{eof_verified_edit_proc,74,  104, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_6,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  132, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_5_number, NULL, NULL },
	{eof_verified_edit_proc,74,  128, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_5,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  156, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_4_number, NULL, NULL },
	{eof_verified_edit_proc,74,  152, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_4,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  180, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_3_number, NULL, NULL },
	{eof_verified_edit_proc,74,  176, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_3,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  204, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_2_number, NULL, NULL },
	{eof_verified_edit_proc,74,  200, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_2,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  228, 64,  8,  2,   23,  0,    0,      0,         0,   eof_string_lane_1_number, NULL, NULL },
	{eof_verified_edit_proc,74,  224, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string_lane_1,  "0123456789Xx",NULL },

	{d_agup_text_proc,      120, 80,  64,  8,  2,   23,  0,    0,      0,         0,   "Finger #",     NULL,          NULL },
	{eof_verified_edit_proc,140, 104, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_6, "12345",NULL },
	{eof_verified_edit_proc,140, 128, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_5, "12345",NULL },
	{eof_verified_edit_proc,140, 152, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_4, "12345",NULL },
	{eof_verified_edit_proc,140, 176, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_3, "12345",NULL },
	{eof_verified_edit_proc,140, 200, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_2, "12345",NULL },
	{eof_verified_edit_proc,140, 224, 22,  20, 2,   23,  0,    0,      1,         0,   eof_finger_string_lane_1, "12345",NULL },

	{d_agup_button_proc,    12,  256, 68,  28, 2,   23,  '\r', D_EXIT, 0,         0,   "OK",         NULL,          NULL },
	{d_agup_button_proc,    104, 256, 68,  28, 2,   23,  0,    D_EXIT, 0,         0,   "Cancel",     NULL,          NULL },
	{NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_note_edit_pro_guitar_note_frets_fingers_menu(void)
{
	char undo_made = 0;
	return eof_menu_note_edit_pro_guitar_note_frets_fingers(0, &undo_made);
}

int eof_menu_note_edit_pro_guitar_note_frets_fingers(char function, char *undo_made)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long ctr, ctr2, stringcount, i;
	long fretvalue, fingervalue;
	unsigned long bitmask = 0;		//Used to build the updated pro guitar note bitmask
	char allmuted;					//Used to track whether all used strings are string muted
	unsigned long flags;			//Used to build the updated flag bitmask
	int retval;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 0;	//Do not allow this function to run unless the pro guitar track is active
	if(eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track))
		return 0;	//Do not allow this function to run if a valid note isn't selected
	if(!undo_made)
		return 0;

	if(!eof_music_paused)
	{
		eof_music_play();
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pro_guitar_note_frets_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_pro_guitar_note_frets_dialog);

	//Update the fret text boxes (listed from top to bottom as string 1 through string 6)
	stringcount = eof_count_track_lanes(eof_song, eof_selected_track);
	if(eof_legacy_view)
	{	//Special case:  If legacy view is enabled, correct stringcount
		stringcount = eof_song->pro_guitar_track[tracknum]->numstrings;
	}
	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask<<=1)
	{	//For each of the 6 supported strings
		if(ctr < stringcount)
		{	//If this track uses this string, copy the fret value to the appropriate string
			eof_pro_guitar_note_frets_dialog[12 - (2 * ctr)].flags = 0;		//Ensure this text boxes' label is enabled
			eof_fret_string_numbers[ctr][7] = '0' + (stringcount - ctr);	//Correct the string number for this label
			eof_pro_guitar_note_frets_dialog[13 - (2 * ctr)].flags = 0;		//Ensure this fret # input box is enabled
			eof_pro_guitar_note_frets_dialog[20 - ctr].flags = 0;			//Ensure this finger # input box is enabled
			if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->note & bitmask)
			{	//If this string is already defined as being in use, copy its fret value to the string
				if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr] == 0xFF)
				{	//If this string is muted with no fret value specified
					(void) snprintf(eof_fret_strings[ctr], sizeof(eof_fret_strings[ctr]) - 1, "X");
				}
				else
				{
					(void) snprintf(eof_fret_strings[ctr], sizeof(eof_fret_strings[ctr]) - 1, "%d", eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr] & 0x7F);	//Mask out the MSB to obtain the fret value
				}
				if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->finger[ctr] != 0)
				{	//If the finger used to fret this string is defined
					(void) snprintf(eof_finger_strings[ctr], sizeof(eof_finger_strings[ctr]) - 1, "%d", eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->finger[ctr]);	//Create the finger string
				}
				else
				{
					eof_finger_strings[ctr][0] = '\0';	//Otherwise empty the string
				}
			}
			else
			{	//Otherwise empty the fret and finger strings
				eof_fret_strings[ctr][0] = '\0';
				eof_finger_strings[ctr][0] = '\0';
				if(function)
				{	//If the calling function wanted finger input boxes for unused strings hidden
					eof_pro_guitar_note_frets_dialog[20 - ctr].flags = D_HIDDEN;		//Ensure this finger # input box is hidden
				}
				else
				{
					eof_pro_guitar_note_frets_dialog[20 - ctr].flags = 0;		//Ensure this finger # input box is enabled
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
		{	//Otherwise disable the text box for this fret and empty the string
			eof_pro_guitar_note_frets_dialog[12 - (2 * ctr)].flags = D_HIDDEN;	//Ensure this text boxes' label is hidden
			eof_pro_guitar_note_frets_dialog[13 - (2 * ctr)].flags = D_HIDDEN;	//Ensure this text box is hidden
			eof_pro_guitar_note_frets_dialog[20 - ctr].flags = D_HIDDEN;		//Ensure this finger # input box is hidden
			eof_fret_strings[ctr][0] = '\0';
		}
	}

	bitmask = 0;
	while(1)
	{	//Until user explicitly cancels, or provides proper input and clicks OK
		char retry = 0, fingeringdefined = 0, offerupdatefingering = 0;
		retval = eof_popup_dialog(eof_pro_guitar_note_frets_dialog, 0);
		if(retval == 21)
		{	//If user clicked OK
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
					if(	((eof_fret_strings[i][0] != '\0') && (eof_finger_strings[i][0] == '\0') && (atol(eof_fret_strings[i]) != 0)) ||
						((eof_fret_strings[i][0] == '\0') && (eof_finger_strings[i][0] != '\0')) ||
						((eof_fret_strings[i][0] != '\0') && ((eof_finger_strings[i][0] != '\0') && (atol(eof_fret_strings[i]) == 0))))
					{	//If this string has a fret value but no finger value for a string that isn't played open, a finger value for a string that isn't played or a finger is specified for a string that is played open
						allegro_message("If any fingering is specified, it must be done for all fretted strings, and only the fretted strings.\nCorrect or erase the finger numbers before clicking OK.");
						retry = 1;	//Flag that the user must enter valid finger information
						break;
					}
				}
			}
			//Validate and store the input
			if(!retry)
			{	//If the finger entries weren't invalid
				if(eof_count_selected_notes(NULL, 0) > 1)
				{	//If multiple notes are selected, warn the user
					if(alert(NULL, "Warning:  This information will be applied to all selected notes.", NULL, "&OK", "&Cancel", 0, 0) == 2)
					{	//If user opts to cancel the operation
						if(note_selection_updated)
						{	//If the only note modified was the seek hover note
							eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
							eof_selection.current = EOF_MAX_NOTES - 1;
						}
						eof_show_mouse(NULL);
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						return 0;	//Return Cancel selected
					}
				}
				for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
				{	//For each note in the track
					if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
					{	//If the note is in the active instrument difficulty and is selected
						for(ctr = 0, allmuted = 1; ctr < 6; ctr++)
						{	//For each of the 6 supported strings
							if(eof_fret_strings[ctr][0] != '\0')
							{	//If this string isn't empty
								fretvalue = fingervalue = 0;
								if(eof_finger_strings[ctr][0] != '\0')
								{	//If there is a finger value specified for this string
									fingervalue = eof_finger_strings[ctr][0] - '0';	//Convert the ASCII numerical character to decimal
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
								if(!fretvalue)
								{	//If the string didn't contain an X, convert it to an integer value
									fretvalue = atol(eof_fret_strings[ctr]);
									if((fretvalue < 0) || (fretvalue > eof_song->pro_guitar_track[tracknum]->numfrets))
									{	//If the conversion to number failed, or an invalid fret number was entered, enter a value of (muted) for the string
										fretvalue = 0xFF;
									}
								}
								if(fretvalue != eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr])
								{	//If this fret value changed
									if(!*undo_made)
									{	//If an undo state hasn't been made yet
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										*undo_made = 1;
									}
									eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] = fretvalue;
								}
								if(fingervalue != eof_song->pro_guitar_track[tracknum]->note[i]->finger[ctr])
								{	//IF the finger value changed
									if(!*undo_made)
									{	//If an undo state hasn't been made yet
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										*undo_made = 1;
									}
									eof_song->pro_guitar_track[tracknum]->note[i]->finger[ctr] = fingervalue;
								}
								if(fretvalue != 0xFF)
								{	//Track whether the all used strings in this note/chord are muted
									allmuted = 0;
								}
							}
							else
							{	//Clear the fret value and return the fret back to its default of 0 (open)
								bitmask &= ~(1 << ctr);	//Clear the appropriate bit for this lane
								if(eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] != 0)
								{	//If this fret value changed
									if(!*undo_made)
									{	//If an undo state hasn't been made yet
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										*undo_made = 1;
									}
									eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] = 0;
									eof_song->pro_guitar_track[tracknum]->note[i]->finger[ctr] = 0;
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
						if(eof_song->pro_guitar_track[tracknum]->note[i]->note != bitmask)
						{	//If the note bitmask changed
							if(!*undo_made)
							{	//If an undo state hasn't been made yet
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								*undo_made = 1;
							}
							eof_song->pro_guitar_track[tracknum]->note[i]->note = bitmask;
						}

			//Update the flags to reflect string muting
						flags = eof_song->pro_guitar_track[tracknum]->note[i]->flags;
						if(!allmuted)
						{	//If any used strings in this note/chord weren't string muted
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE);	//Clear the string mute flag
						}
						else if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
						{	//If all strings are muted and the user didn't specify a palm mute
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;		//Set the string mute flag
						}
						eof_song->pro_guitar_track[tracknum]->note[i]->flags = flags;
					}//If the note is in the active instrument difficulty and is selected
				}//For each note in the track

				//Offer to update the fingering for all notes in the track matching the selected note (which all selected notes now match because they were altered if they didn't)
				if(fingeringdefined)
				{	//If the fingering is defined
					for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->notes; ctr++)
					{	//For each note in the track
						if((ctr != eof_selection.current) && !eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr))
						{	//If this note isn't the one that was just edited, but it matches
							if(eof_pro_guitar_note_fingering_valid(eof_song->pro_guitar_track[tracknum], ctr) != 1)
							{	//If the fingering for the note is not fully defined
								offerupdatefingering = 1;	//Note that the user should be prompted whether to update the fingering array of all matching notes
								break;
							}
						}
					}
					if(function || (offerupdatefingering && (alert(NULL, "Update all instances of this note to use this fingering?", NULL, "&Yes", "&No", 'y', 'n') == 1)))
					{	//If the user opts to update the fingering array of all matching notes (or if the calling function wanted all instances to be updated automatically)
						for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->notes; ctr++)
						{	//For each note in the track
							if((ctr != eof_selection.current) && !eof_note_compare_simple(eof_song, eof_selected_track, eof_selection.current, ctr))
							{	//If this note isn't the one that was just edited, but it matches
								if(eof_pro_guitar_note_fingering_valid(eof_song->pro_guitar_track[tracknum], ctr) != 1)
								{	//If the fingering for the note is not fully defined
									if(!*undo_made)
									{	//If an undo state hasn't been made yet
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										*undo_made = 1;
									}
									memcpy(eof_song->pro_guitar_track[tracknum]->note[ctr]->finger, eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->finger, 8);	//Copy the finger array
								}
							}
						}
					}
				}//If the fingering is defined
				break;
			}//If the finger entries weren't invalid
		}//If user clicked OK
		else
		{
			eof_cursor_visible = 0;
			eof_pen_visible = 0;
			eof_render();
			return 0;	//Return Cancel selected
		}
	}//Until user explicitly cancels, or provides proper input and clicks OK

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;	//Return OK selected
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
	unsigned long ctr, ctr2, tracknum;
	EOF_PRO_GUITAR_TRACK *tp;
	char cancelled, user_prompted = 0;

	if(!eof_song)
		return 0;	//Error

	if(report)
	{	//If the user invoked this function from the Song menu
		user_prompted = 1;	//Suppress asking to make changes since the user already gave intention to make any needed corrections
	}
	eof_song_fix_fingerings(eof_song, undo_made);		//Erase partial note fingerings, replicate valid finger definitions to matching notes without finger definitions
	memset(&eof_selection, 0, sizeof(EOF_SELECTION_DATA));	//Clear the note selection
	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track (skipping the global track, 0)
		if(eof_song->track[ctr]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If this is a pro guitar track
			tracknum = eof_song->track[ctr]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in this track
				if((eof_note_count_colors(eof_song, ctr, ctr2) > 1) && !eof_is_string_muted(eof_song, ctr, ctr2))
				{	//If this note is a chord that isn't completely string muted
					if(eof_pro_guitar_note_fingering_valid(tp, ctr2) != 1)
					{	//If the fingering for this chord isn't valid or is undefined
						if(!user_prompted && (alert(NULL, "Some chords don't have correct finger information.", "Update them now?", "&Yes", "&No", 'y', 'n') != 1))
						{	//If the user does not opt to update the fingering
							return 0;
						}
						user_prompted = 1;
						eof_selection.current = ctr2;	//Select this note
						eof_selection.track = ctr;		//Select this track
						eof_selection.multi[ctr2] = 1;	//Select this note in the selection array
						(void) eof_menu_track_selected_track_number(ctr);	//Change the active track
						eof_note_type = tp->note[ctr2]->type;	//Set the note's difficulty as the active difficulty
						eof_set_seek_position(tp->note[ctr2]->pos + eof_av_delay);
						eof_render();					//Render the track being checked so the user knows which track a note is being edited from
						cancelled = !eof_menu_note_edit_pro_guitar_note_frets_fingers(1, undo_made);	//Open the edit fret/finger dialog where only the necessary finger fields can be altered
						eof_selection.multi[ctr2] = 0;	//Unselect this note
						eof_selection.current = EOF_MAX_NOTES - 1;
						if(cancelled)
						{	//If the user cancelled updating the chord fingering
							return 0;
						}
					}
				}//If this note is a chord that isn't completely string muted
			}//For each note in this track
		}//If this is a pro guitar track
	}//For each track (skipping the global track, 0)
	if(report && !(*undo_made))
	{	//If no alterations were necessary and the calling function wanted this reported
		allegro_message("All fingerings are already defined");
	}
	return 1;
}

int eof_menu_note_toggle_tapping(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_TAP;	//Toggle the tap flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
			{	//If tapping was just enabled
				flags |= EOF_NOTE_FLAG_F_HOPO;				//Set the legacy HOPO on flag (no strum required for this note)
				flags &= ~EOF_NOTE_FLAG_NO_HOPO;			//Clear the HOPO off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;		//Clear the hammer on flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;		//Clear the pull off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Clear the bend flag
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
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_tapping(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_bend(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	char bends_present = 0;		//Will be set to nonzero if any selected notes become bend notes
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Toggle the bend flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
			{	//If bend status was just enabled
				flags &= ~EOF_NOTE_FLAG_F_HOPO;					//Clear the legacy HOPO on flag
				flags &= ~EOF_NOTE_FLAG_NO_HOPO;				//Clear the HOPO off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;			//Clear the hammer on flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;			//Clear the pull off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Clear the tap flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
				bends_present = 1;
			}
			else
			{
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag
			}
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND))
			{	//If this is not a bend anymore
				eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->note[i]->bendstrength = 0;	//Reset the strength of the bend
			}
		}
	}
	if(eof_write_rs_files && bends_present)
	{	//If the user wants to save Rocksmith capable files, prompt to set the strength of bend notes
		(void) eof_pro_guitar_note_bend_strength_no_save();	//Don't make another undo state
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_bend(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
			{	//If this note has bend status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Clear the bend flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_harmonic(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Toggle the harmonic flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
			{	//If harmonic status was just enabled
				flags &= ~EOF_NOTE_FLAG_F_HOPO;				//Clear the legacy HOPO on flag
				flags &= ~EOF_NOTE_FLAG_NO_HOPO;			//Clear the HOPO off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;		//Clear the hammer on flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;		//Clear the pull off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;		//Clear the tap flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Clear the bend flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;	//Clear the vibrato flag
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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_harmonic(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
			{	//If this note has harmonic status
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_slide_up(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	char slides_present = 0;	//Will be set to nonzero if any selected notes become slide notes
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;			//Toggle the slide up flag
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the slide down flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
			{	//If the note now slides up
				slides_present = 1;	//Track that at least one selected note is an up slide
			}
			else
			{	//If the note doesn't slide, clear the slide end fret status
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag
			}
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP))
			{	//If this is not a slide note anymore
				eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->note[i]->slideend = 0;	//Reset the ending fret number of the slide
			}
		}
	}
	if(eof_write_rs_files && slides_present)
	{	//If the user wants to save Rocksmith capable files, prompt to set the ending fret for the slide notes
		(void) eof_pro_guitar_note_slide_end_fret_no_save();	//Don't make another undo state
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_slide_down(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	char slides_present = 0;	//Will be set to nonzero if any selected notes become slide notes
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;		//Toggle the slide down flag
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the slide down flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
			{	//If the note now slides down
				slides_present = 1;	//Track that at least one selected note is a down slide
			}
			else
			{	//If the note doesn't slide, clear the slide end fret status
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag
			}
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
			{	//If this is not a slide note anymore
				eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->note[i]->slideend = 0;	//Reset the ending fret number of the slide
			}
		}
	}
	if(eof_write_rs_files && slides_present)
	{	//If the user wants to save Rocksmith capable files, prompt to set the ending fret for the slide notes
		(void) eof_pro_guitar_note_slide_end_fret_no_save();	//Don't make another undo state
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_slide(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;							//Save an extra copy of the original flags
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the slide up flag
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the slide down flag
			flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;		//Clear this flag
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->note[i]->slideend = 0;	//Reset the ending fret number of the slide
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_palm_muting(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_palm_muting(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_arpeggio_mark(void)
{
	unsigned long i, j;
	unsigned long sel_start = 0;
	unsigned long sel_end = 0;
	char firstnote = 0;						//Is set to nonzero when the first selected note in the active track difficulty is found
	char existingphrase = 0;				//Is set to nonzero if any selected notes are within an existing phrase
	unsigned long existingphrasenum = 0;	//Is set to the last arpeggio phrase number that encompasses existing notes
	unsigned long tracknum;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	//Find the start and end position of the collection of selected notes in the active difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track and difficulty
			if(firstnote == 0)
			{	//This is the first encountered selected note
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				sel_end = sel_start + eof_get_note_length(eof_song, eof_selected_track, i);
				firstnote = 1;
			}
			else
			{
				if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
				{
					sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
				if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
				{
					sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
			}
		}
	}
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(j = 0; j < eof_song->pro_guitar_track[tracknum]->arpeggios; j++)
	{	//For each arpeggio section in the track
		if((sel_end >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (sel_start <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos) && (eof_song->pro_guitar_track[tracknum]->arpeggio[j].difficulty == eof_note_type))
		{	//If the selection of notes is within this arpeggio's start and end position, and the arpeggio is also in the active difficulty
			existingphrase = 1;	//Note it
			existingphrasenum = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(!existingphrase)
	{	//If the selected notes are not within an existing arpeggio phrase, create one applying to the active difficulty
		(void) eof_track_add_section(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_note_type, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		eof_song->pro_guitar_track[tracknum]->arpeggio[existingphrasenum].start_pos = sel_start;
		eof_song->pro_guitar_track[tracknum]->arpeggio[existingphrasenum].end_pos = sel_end;
	}
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && ((eof_get_note_pos(eof_song, eof_selected_track, i) >= sel_start) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= sel_end)))
		{	//If the note is in the active track difficulty and is within the created/edited phrase
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags |= EOF_NOTE_FLAG_CRAZY;	//Set the note's crazy flag
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_arpeggio_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Don't allow this function to run unless a pro guitar track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_song->pro_guitar_track[tracknum]->arpeggios; j++)
			{	//For each arpeggio section in the track
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos) && (eof_song->pro_guitar_track[tracknum]->arpeggio[j].difficulty == eof_note_type))
				{	//If the note is encompassed within this arpeggio section, and the arpeggio section exists in the active difficulty
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_pro_guitar_track_delete_arpeggio(eof_song->pro_guitar_track[tracknum], j);	//Delete the arpeggio section
					break;
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

void eof_pro_guitar_track_delete_arpeggio(EOF_PRO_GUITAR_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(index >= tp->arpeggios)
		return;

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

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	if(alert(NULL, "Erase all arpeggios from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->arpeggios; ctr++)
		{	//For each arpeggio section in this track
			eof_song->pro_guitar_track[tracknum]->arpeggio[ctr].name[0] = '\0';	//Empty the name string
		}
		eof_song->pro_guitar_track[tracknum]->arpeggios = 0;
	}
	return 1;
}

int eof_menu_trill_mark(void)
{
	unsigned long i, j;
	unsigned long sel_start = 0;
	unsigned long sel_end = 0;
	char firstnote = 0;						//Is set to nonzero when the first selected note in the active track difficulty is found
	char existingphrase = 0;				//Is set to nonzero if any selected notes are within an existing phrase
	unsigned long existingphrasenum = 0;	//Is set to the last trill phrase number that encompasses existing notes
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_KEYS_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_KEYS_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum/keys track is active

	//Find the start and end position of the collection of selected notes in the active difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track and difficulty
			if(firstnote == 0)
			{	//This is the first encountered selected note
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				sel_end = sel_start + eof_get_note_length(eof_song, eof_selected_track, i);
				firstnote = 1;
			}
			else
			{
				if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
				{
					sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
				if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
				{
					sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
			}
		}
	}
	for(j = 0; j < eof_get_num_trills(eof_song, eof_selected_track); j++)
	{	//For each trill section in the track
		sectionptr = eof_get_trill(eof_song, eof_selected_track, j);
		if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
		{	//If the selection of notes is within this trill section's start and end position
			existingphrase = 1;	//Note it
			existingphrasenum = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(!existingphrase)
	{	//If the selected notes are not within an existing trill phrase, create one
		(void) eof_track_add_section(eof_song, eof_selected_track, EOF_TRILL_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		sectionptr = eof_get_trill(eof_song, eof_selected_track, existingphrasenum);
		sectionptr->start_pos = sel_start;
		sectionptr->end_pos = sel_end;
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_tremolo_mark(void)
{
	unsigned long i, j;
	unsigned long sel_start = 0;
	unsigned long sel_end = 0;
	char firstnote = 0;						//Is set to nonzero when the first selected note in the active track difficulty is found
	char existingphrase = 0;				//Is set to nonzero if any selected notes are within an existing phrase
	unsigned long existingphrasenum = 0;	//Is set to the last tremolo phrase number that encompasses existing notes
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	//Find the start and end position of the collection of selected notes in the active difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track and difficulty
			if(firstnote == 0)
			{	//This is the first encountered selected note
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				sel_end = sel_start + eof_get_note_length(eof_song, eof_selected_track, i);
				firstnote = 1;
			}
			else
			{
				if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
				{
					sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
				if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
				{
					sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
			}
		}
	}
	for(j = 0; j < eof_get_num_tremolos(eof_song, eof_selected_track); j++)
	{	//For each tremolo section in the track
		sectionptr = eof_get_tremolo(eof_song, eof_selected_track, j);
		if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
		{	//If the selection of notes is within this tremolo section's start and end position
			existingphrase = 1;	//Note it
			existingphrasenum = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(!existingphrase)
	{	//If the selected notes are not within an existing tremolo phrase, create one
		(void) eof_track_add_section(eof_song, eof_selected_track, EOF_TREMOLO_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		sectionptr = eof_get_tremolo(eof_song, eof_selected_track, existingphrasenum);
		sectionptr->start_pos = sel_start;
		sectionptr->end_pos = sel_end;
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_slider_mark(void)
{
	unsigned long i, j;
	unsigned long sel_start = 0;
	unsigned long sel_end = 0;
	char firstnote = 0;						//Is set to nonzero when the first selected note in the active track difficulty is found
	char existingphrase = 0;				//Is set to nonzero if any selected notes are within an existing phrase
	unsigned long existingphrasenum = 0;	//Is set to the last slider phrase number that encompasses existing notes
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_format != EOF_LEGACY_TRACK_FORMAT))
		return 1;	//Do not allow this function to run unless a legacy guitar track is active

	//Find the start and end position of the collection of selected notes in the active difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track and difficulty
			if(firstnote == 0)
			{	//This is the first encountered selected note
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				sel_end = sel_start + eof_get_note_length(eof_song, eof_selected_track, i);
				firstnote = 1;
			}
			else
			{
				if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
				{
					sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
				if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
				{
					sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
			}
		}
	}
	for(j = 0; j < eof_get_num_sliders(eof_song, eof_selected_track); j++)
	{	//For each slider section in the track
		sectionptr = eof_get_slider(eof_song, eof_selected_track, j);
		if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
		{	//If the selection of notes is within this slider section's start and end position
			existingphrase = 1;	//Note it
			existingphrasenum = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(!existingphrase)
	{	//If the selected notes are not within an existing slider phrase, create one
		(void) eof_track_add_section(eof_song, eof_selected_track, EOF_SLIDER_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		sectionptr = eof_get_slider(eof_song, eof_selected_track, existingphrasenum);
		sectionptr->start_pos = sel_start;
		sectionptr->end_pos = sel_end;
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_trill_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_get_num_trills(eof_song, eof_selected_track); j++)
			{	//For each trill section in the track
				sectionptr = eof_get_trill(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= sectionptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= sectionptr->end_pos))
				{	//If the note is encompassed within this trill section
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_track_delete_trill(eof_song, eof_selected_track, j);	//Delete the trill section
					break;
				}
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_tremolo_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_get_num_tremolos(eof_song, eof_selected_track); j++)
			{	//For each tremolo section in the track
				sectionptr = eof_get_tremolo(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= sectionptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= sectionptr->end_pos))
				{	//If the note is encompassed within this tremolo section
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_track_delete_tremolo(eof_song, eof_selected_track, j);	//Delete the tremolo section
					break;
				}
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_slider_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *sectionptr;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_format != EOF_LEGACY_TRACK_FORMAT))
		return 1;	//Do not allow this function to run unless a legacy guitar track is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_get_num_sliders(eof_song, eof_selected_track); j++)
			{	//For each slider section in the track
				sectionptr = eof_get_slider(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= sectionptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= sectionptr->end_pos))
				{	//If the note is encompassed within this slider section
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_track_delete_slider(eof_song, eof_selected_track, j);	//Delete the slider section
					break;
				}
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
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

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) & (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		prompt = drum_track_prompt;	//If this is a drum track, refer to the sections as "drum rolls" instead of "tremolos"

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
	if((eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_format != EOF_LEGACY_TRACK_FORMAT))
		return 1;	//Do not allow this function to run unless a legacy guitar track is active

	if(alert(NULL, "Erase all slider sections from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_sliders(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;
}

int eof_menu_note_clear_legacy_values(void)
{
	unsigned long i;
	long u = 0;
	unsigned long tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_edit_name(void)
{
	unsigned long i;
	char *notename = NULL, undo_made = 0, auto_apply = 0;

	if(!eof_music_catalog_playback)
	{
		int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

		eof_cursor_visible = 0;
		eof_render();
		eof_color_dialog(eof_note_name_dialog, gui_fg_color, gui_bg_color);
		centre_dialog(eof_note_name_dialog);

		notename = eof_get_note_name(eof_song, eof_selected_track, eof_selection.current);	//Get the last selected note's name
		if(notename == NULL)
		{	//If there was an error getting the name
			eof_etext[0] = '\0';	//Empty the edit field
		}
		else
		{	//Otherwise copy the note name into the edit field
			(void) ustrcpy(eof_etext, notename);
		}

		if(eof_popup_dialog(eof_note_name_dialog, 2) == 3)	//User hit OK
		{
			if((eof_etext[0] == '\0') && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If the user kept the name field empty and this is a pro guitar track, offer to apply auto-detected names
				if(alert(NULL, "Apply automatically-detected chord names?", NULL, "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user opts to apply auto-detected names
					auto_apply = 1;
				}
			}
			else
			{
				for(i = 0; i < ustrlen(eof_etext); i++)
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
				if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
				{	//If the note is in the active instrument difficulty and is selected
					notename = eof_get_note_name(eof_song, eof_selected_track, i);	//Get the note's name
					if(notename)
					{	//As long as there wasn't an error accessing the note name
						if(auto_apply)
						{	//If applying automatically detected names, get the name of this note
							eof_etext[0] = '\0';	//Empty the string, so that it won't assign a name unless it is detected
							(void) eof_build_note_name(eof_song, eof_selected_track, i, eof_etext);	//Detect the name of this chord
						}
						if(ustricmp(notename, eof_etext))
						{	//If the updated string (eof_etext) is different from the note's existing name
							if(!undo_made)
							{
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								undo_made = 1;
							}
							eof_set_note_name(eof_song, eof_selected_track, i, eof_etext);	//Update the note's name
						}
					}
				}
			}
		}
		if(note_selection_updated)
		{	//If the only note modified was the seek hover note
			eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
			eof_selection.current = EOF_MAX_NOTES - 1;
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
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
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
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_pro_guitar_toggle_strum_down(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
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
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_pro_guitar_toggle_strum_mid(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
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
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_strum_direction(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
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

int eof_menu_copy_solos_track_number(EOF_SONG *sp, int sourcetrack, int desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_solos(sp, sourcetrack))
		return 0;	//Source track has no solo phrases
	if(eof_get_num_solos(sp, desttrack))
	{	//If there are already solos in the destination track
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
	eof_determine_phrase_status(eof_song, eof_selected_track);
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

int eof_menu_copy_sp_track_number(EOF_SONG *sp, int sourcetrack, int desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_star_power_paths(sp, sourcetrack))
		return 0;	//Source track has no star power phrases
	if(eof_get_num_star_power_paths(sp, desttrack))
	{	//If there are already star power phrases in the destination track
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
	eof_determine_phrase_status(eof_song, eof_selected_track);
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

int eof_menu_copy_arpeggio_track_number(EOF_SONG *sp, int sourcetrack, int desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_arpeggios(sp, sourcetrack))
		return 0;	//Source track has no arpeggio phrases
	if(eof_get_num_arpeggios(sp, desttrack))
	{	//If there are already arpeggio phrases in the destination track
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
		if(ptr)
		{	//If this phrase could be found
			(void) eof_track_add_section(sp, desttrack, EOF_ARPEGGIO_SECTION, ptr->difficulty, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
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

int eof_menu_copy_trill_track_number(EOF_SONG *sp, int sourcetrack, int desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_trills(sp, sourcetrack))
		return 0;	//Source track has no trill phrases
	if(eof_get_num_trills(sp, desttrack))
	{	//If there are already trill phrases in the destination track
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
			(void) eof_track_add_section(sp, desttrack, EOF_TRILL_SECTION, ptr->difficulty, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
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

int eof_menu_copy_tremolo_track_number(EOF_SONG *sp, int sourcetrack, int desttrack)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(!eof_get_num_tremolos(sp, sourcetrack))
		return 0;	//Source track has no tremolo phrases
	if(eof_get_num_tremolos(sp, desttrack))
	{	//If there are already tremolo phrases in the destination track
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
			(void) eof_track_add_section(sp, desttrack, EOF_TREMOLO_SECTION, ptr->difficulty, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;	//Return completion
}

int eof_menu_note_toggle_hi_hat_open(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 6)
			{	//If this drum note contains a yellow gem (or red gem, to allow for notation during disco flips)
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
				{	//If this is a yellow gem
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,1,0);	//Automatically mark as a cymbal
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
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_hi_hat_pedal(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 6)
			{	//If this drum note contains a yellow gem (or red gem, to allow for notation during disco flips)
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
				{	//If this is a yellow gem
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,1,0);	//Automatically mark as a cymbal
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
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_hi_hat_sizzle(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 6)
			{	//If this drum note contains a yellow gem (or red gem, to allow for notation during disco flips)
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
				{	//If this is a yellow gem
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,1,0);	//Automatically mark as a cymbal
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
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_hi_hat_status(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 6)
			{	//If this drum note contains a yellow gem (or red gem, to allow for notation during disco flips)
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
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_rimshot(void)
{
	unsigned long i, flags, tracknum;
	long u = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 2)
			{	//If this drum note contains a red note
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
					flags = eof_get_note_flags(eof_song, eof_selected_track, i);
					flags ^= EOF_DRUM_NOTE_FLAG_R_RIMSHOT;	//Toggle the rim shot status
					eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_pro_guitar_toggle_hammer_on(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	if((eof_count_selected_notes(NULL, 0) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_selection.multi[i])
			{	//If the note is selected
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
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_pro_guitar_remove_hammer_on(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

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
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_pro_guitar_toggle_pull_off(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	if((eof_count_selected_notes(NULL, 0) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_selection.multi[i])
			{	//If the note is selected
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
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Clear the bend flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;	//Clear the harmonic flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Clear the vibrato flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_pro_guitar_remove_pull_off(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_thin_notes_track_1(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 1, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_2(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 2, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_3(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 3, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_4(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 4, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_5(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 5, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_6(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 6, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_7(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 7, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_8(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 8, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_9(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 9, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_10(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 10, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_11(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 11, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_12(void)
{
	return eof_thin_notes_to_match__target_difficulty(eof_song, 12, eof_selected_track, 2, eof_note_type);
}

int eof_menu_note_toggle_ghost(void)
{
	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

 	eof_log("eof_menu_note_toggle_ghost() entered", 1);

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar/bass track is active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_ghost(void)
{
	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

 	eof_log("eof_menu_note_remove_ghost() entered", 1);

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar/bass track is active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_remove_vibrato(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->pro_guitar_track[tracknum]->note[i]->type == eof_note_type))
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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_note_toggle_vibrato(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, tracknum;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->pro_guitar_track[tracknum]->note[i]->type == eof_note_type))
		{	//If this note is selected and is in the active difficulty
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;	//Toggle the vibrato flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
			{	//If harmonic status was just enabled
				flags &= ~EOF_NOTE_FLAG_F_HOPO;				//Clear the legacy HOPO on flag
				flags &= ~EOF_NOTE_FLAG_NO_HOPO;			//Clear the HOPO off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;		//Clear the hammer on flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;		//Clear the pull off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;		//Clear the tap flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Clear the bend flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;	//Clear the vibrato flag
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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_feedback_mode_update_note_selection(void)
{
	unsigned long i;

	if((eof_seek_hover_note < 0) || (eof_seek_hover_note >= eof_get_track_size(eof_song, eof_selected_track)) || (eof_input_mode != EOF_INPUT_FEEDBACK))
	{	//If there is no valid seek hover note, or Feedback input mode isn't in effect
		return 0;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_selection.multi[i])
		{	//If a note is selected
			return 0;
		}
	}

	eof_selection.multi[eof_seek_hover_note] = 1;
	eof_selection.current = eof_seek_hover_note;
	eof_selection.track = eof_selected_track;
	return 1;
}

DIALOG eof_pro_guitar_note_slide_end_fret_dialog[] =
{
   /* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
   { d_agup_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Edit slide end fret",      NULL, NULL },
   { d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "End slide at fret #",                NULL, NULL },
   { eof_verified_edit_proc,12,  56,  50,  20,  0,   0,   0,    0,      7,   0,   eof_etext,     "0123456789", NULL },
   { d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,               NULL, NULL }
};

int eof_pro_guitar_note_slide_end_fret(char undo)
{
	unsigned long newend, i, flags, bitmask, ctr;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect
	unsigned long tracknum;
	EOF_PRO_GUITAR_NOTE *np;
	char undo_made = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_selection.current >= eof_song->pro_guitar_track[tracknum]->notes)
		return 1;	//Invalid selected note number
	np = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current];

	eof_render();
	eof_color_dialog(eof_pro_guitar_note_slide_end_fret_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_pro_guitar_note_slide_end_fret_dialog);
	if(np->slideend == 0)
	{	//If the selected note has no ending fret defined
		eof_etext[0] = '\0';	//Empty this string
	}
	else
	{	//Otherwise write the ending fret into the string
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", np->slideend);
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

		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
			{	//If this note is in the currently active track and is selected
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				if((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
				{	//If this note is a slide
					//Determine the lowest fret used in the note
					unsigned char lowestfret = 0, thisfret;
					for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
					{	//For each of the 6 usable strings, from lowest to highest gauge
						if(eof_get_note_note(eof_song, eof_selected_track, i) & bitmask)
						{	//If this string is used
							thisfret = eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] & 0x7F;	//Store the fret (mask out the MSB, used for mute status)
							if(thisfret)
							{	//If a fret is used on this string (note played open)
								if(!lowestfret)
								{	//If no fret number has been recorded so far
									lowestfret = thisfret;
								}
								else if(thisfret < lowestfret)
								{	//Otherwise store this gem's fret number if it is lower than the others checked for this note
									lowestfret = thisfret;
								}
							}
						}
					}
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
								return 1;	//Return error
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
								return 1;	//Return error
							}
						}
					}
					if(newend != eof_song->pro_guitar_track[tracknum]->note[i]->slideend)
					{	//If the slide ending is different than what the note already has
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
							eof_song->pro_guitar_track[tracknum]->note[i]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag to indicate that the slide's ending fret is undefined
						}
					}
				}//If this note is a slide
			}//If this note is in the currently active track and is selected
		}//For each note in the active track
	}//User clicked OK

	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
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

DIALOG eof_pro_guitar_note_bend_strength_dialog[] =
{
   /* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                          (dp2) (dp3) */
   { d_agup_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Edit bend strength",         NULL, NULL },
   { d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "Bends this # of half steps:",NULL, NULL },
   { eof_verified_edit_proc,12,  56,  20,  20,  0,   0,   0,    0,      1,   0,   eof_etext,     "0123456789",  NULL },
   { d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                         NULL, NULL },
   { d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                     NULL, NULL },
   { NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                         NULL, NULL }
};

int eof_pro_guitar_note_bend_strength(char undo)
{
	unsigned long newstrength, i, flags;
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect
	unsigned long tracknum;
	EOF_PRO_GUITAR_NOTE *np;
	char undo_made = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_selection.current >= eof_song->pro_guitar_track[tracknum]->notes)
		return 1;	//Invalid selected note number
	np = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current];

	eof_render();
	eof_color_dialog(eof_pro_guitar_note_bend_strength_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_pro_guitar_note_bend_strength_dialog);
	if(np->bendstrength == 0)
	{	//If the selected note has no ending fret defined
		eof_etext[0] = '\0';	//Empty this string
	}
	else
	{	//Otherwise write the ending fret into the string
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", np->bendstrength);
	}

	if(eof_popup_dialog(eof_pro_guitar_note_bend_strength_dialog, 2) == 3)
	{	//User clicked OK
		if(eof_etext[0] == '\0')
		{	//If the user did not define the bend strength
			newstrength = 0;
		}
		else
		{
			newstrength = atol(eof_etext);
		}

		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
			{	//If this note is in the currently active track and is selected
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
				{	//If this note is a bend
					if(newstrength != eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength)
					{	//If the bend strength is different than what the note already has
						if(undo && !undo_made)
						{	//Make a back up before changing the first note (but only if the calling function specified to create an undo state)
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_song->pro_guitar_track[tracknum]->note[i]->bendstrength = newstrength;
						if(newstrength)
						{	//If the bend strength is nonzero, it is now defined
							eof_song->pro_guitar_track[tracknum]->note[i]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Set this flag to indicate that the bend's strength is defined
						}
						else
						{	//Otherwise it is now undefined
							eof_song->pro_guitar_track[tracknum]->note[i]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Clear this flag to indicate that the bend's strength is undefined
						}
					}
				}//If this note is a bend
			}//If this note is in the currently active track and is selected
		}//For each note in the active track
	}//User clicked OK

	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
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
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

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
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_note_menu_read_gp_lyric_texts(void)
{
	unsigned long ctr, tracknum, linestart, lastlyricend;
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
	returnedfn = ncd_file_select(0, eof_filename, "Import GP format lyric text file", eof_filter_text_files);
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
					(void) eof_vocal_track_add_line(tp, linestart, lastlyricend);	//Add a line phrase
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
				else
				{	//Otherwise it is included in the text read for this lyric so far and ends the current lyric text
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
		(void) eof_vocal_track_add_line(tp, linestart, lastlyricend);	//Add a line phrase
	}

	(void) pack_fclose(fp);
	return 1;
}
