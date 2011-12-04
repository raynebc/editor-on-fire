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
#include "note.h"

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
    {"Mark as non &Hammer on", eof_menu_pro_guitar_remove_hammer_on, NULL, 0, NULL},
    {"Toggle pull off\tP", eof_menu_pro_guitar_toggle_pull_off, NULL, 0, NULL},
    {"Mark as non &Pull off", eof_menu_pro_guitar_remove_pull_off, NULL, 0, NULL},
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

MENU eof_note_freestyle_menu[] =
{
    {"&On", eof_menu_set_freestyle_on, NULL, 0, NULL},
    {"O&ff", eof_menu_set_freestyle_off, NULL, 0, NULL},
    {"&Toggle\tF", eof_menu_toggle_freestyle, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_drum_menu[] =
{
    {"Toggle &Yellow cymbal\tCtrl+Y", eof_menu_note_toggle_rb3_cymbal_yellow, NULL, 0, NULL},
    {"Toggle &Blue cymbal\tCtrl+B", eof_menu_note_toggle_rb3_cymbal_blue, NULL, 0, NULL},
    {"Toggle &Green cymbal\tCtrl+G", eof_menu_note_toggle_rb3_cymbal_green, NULL, 0, NULL},
    {"Mark as &Non cymbal", eof_menu_note_remove_cymbal, NULL, 0, NULL},
    {"Mark new notes as &Cymbals", eof_menu_note_default_cymbal, NULL, 0, NULL},
    {"Toggle expert+ bass drum\tCtrl+E", eof_menu_note_toggle_double_bass, NULL, 0, NULL},
    {"Mark as non &Expert+ bass drum", eof_menu_note_remove_double_bass, NULL, 0, NULL},
    {"&Mark new notes as Expert+", eof_menu_note_default_double_bass, NULL, 0, NULL},
    {"Toggle Y note as &Open hi hat\tShift+O", eof_menu_note_toggle_hi_hat_open, NULL, 0, NULL},
    {"Toggle Y note as &Pedal hi hat\tShift+P",eof_menu_note_toggle_hi_hat_pedal, NULL, 0, NULL},
    {"Toggle Y note as &Sizzle hi hat\tShift+S", eof_menu_note_toggle_hi_hat_sizzle, NULL, 0, NULL},
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
    {"Toggle Slide &Up\tCtrl+Up", eof_menu_note_toggle_slide_up, NULL, 0, NULL},
    {"Toggle Slide &Down\tCtrl+Down", eof_menu_note_toggle_slide_down, NULL, 0, NULL},
    {"Mark as non &Slide", eof_menu_note_remove_slide, NULL, 0, NULL},
    {"&Arpeggio", NULL, eof_arpeggio_menu, 0, NULL},
    {"&Clear legacy bitmask", eof_menu_note_clear_legacy_values, NULL, 0, NULL},
    {"Toggle Strum Up\tShift+Up", eof_pro_guitar_toggle_strum_up, NULL, 0, NULL},
    {"Toggle Strum Down\tShift+Down", eof_pro_guitar_toggle_strum_down, NULL, 0, NULL},
    {"Remove strum direction", eof_menu_note_remove_strum_direction, NULL, 0, NULL},
    {"Toggle tapping\tCtrl+T", eof_menu_note_toggle_tapping, NULL, 0, NULL},
    {"Mark as non &Tapping", eof_menu_note_remove_tapping, NULL, 0, NULL},
    {"Toggle palm muting\tCtrl+M", eof_menu_note_toggle_palm_muting, NULL, 0, NULL},
    {"Mark as non palm &Muting", eof_menu_note_remove_palm_muting, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_lyrics_menu[] =
{
    {"&Edit Lyric\tL", eof_edit_lyric_dialog, NULL, 0, NULL},
    {"&Split Lyric\tShift+L", eof_menu_split_lyric, NULL, 0, NULL},
    {"&Lyric Lines", NULL, eof_lyric_line_menu, 0, NULL},
    {"&Freestyle", NULL, eof_note_freestyle_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_menu[] =
{
    {"&Toggle", NULL, eof_note_toggle_menu, 0, NULL},
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
    {"Thin difficulty to match", NULL, eof_menu_thin_notes_menu, 0, NULL},
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
	int spstart = -1, ssstart = -1, llstart = -1, arpeggiostart = -1, trillstart = -1, tremolostart = -1, sliderstart = -1;
	int spend = -1, ssend = -1, llend = -1, arpeggioend = -1, trillend = -1, tremoloend = -1, sliderend = -1;
	int spp = 0, ssp = 0, llp = 0, arpeggiop = 0, trillp = 0, tremolop = 0, sliderp = 0;
	unsigned long i, j;
	unsigned long tracknum;
	int sel_start = eof_music_length, sel_end = 0;
	int firstnote = 0, lastnote;
	EOF_PHRASE_SECTION *sectionptr = NULL;
	unsigned long track_behavior = 0;

	if(eof_song && eof_song_loaded)
	{
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
				if(firstnote < 0)
				{
					firstnote = i;
				}
				lastnote = i;
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
				if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
				{
					if(firstnote < 0)
					{
						firstnote = i;
					}
					lastnote = i;
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
					if((sel_end >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (sel_start <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos))
					{
						inarpeggio = 1;
						arpeggiostart = sel_start;
						arpeggioend = sel_end;
						arpeggiop = j;
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
						trillstart = sel_start;
						trillend = sel_end;
						trillp = j;
					}
				}
				for(j = 0; j < eof_get_num_tremolos(eof_song, eof_selected_track); j++)
				{	//For each tremolo phrase in the active track
					sectionptr = eof_get_tremolo(eof_song, eof_selected_track, j);
					if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
					{
						intremolo = 1;
						tremolostart = sel_start;
						tremoloend = sel_end;
						tremolop = j;
					}
				}
				for(j = 0; j < eof_get_num_sliders(eof_song, eof_selected_track); j++)
				{	//For each slider phrase in the active track
					sectionptr = eof_get_slider(eof_song, eof_selected_track, j);
					if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
					{
						inslider = 1;
						sliderstart = sel_start;
						sliderend = sel_end;
						sliderp = j;
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

			eof_note_menu[4].flags = 0;	//Note>Solos> submenu
			eof_note_menu[5].flags = 0; //Note>Star Power> submenu
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT))
			{	//If a legacy guitar note is selected
				eof_note_menu[13].flags = 0;		//Note>Slider> submenu
			}
			else
			{
				eof_note_menu[13].flags = D_DISABLED;
			}
		}
		else
		{	//NO NOTES/LYRICS SELECTED
			eof_star_power_menu[0].flags = D_DISABLED;	//Note>Star Power>Mark/Remark
			eof_solo_menu[0].flags = D_DISABLED; 		//Note>Solos>Mark/Remark
			eof_lyric_line_menu[0].flags = D_DISABLED;	//Note>Lyrics>Lyric Lines>Mark/Remark
			eof_note_menu[4].flags = D_DISABLED; 		//Note>Solos> submenu
			eof_note_menu[5].flags = D_DISABLED; 		//Note>Star Power> submenu
			eof_note_menu[13].flags = D_DISABLED;		//Note>Slider> submenu
		}

		/* star power mark/remark */
		if(insp)
		{
			eof_star_power_menu[1].flags = 0;
			ustrcpy(eof_star_power_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_star_power_menu[1].flags = D_DISABLED;
			ustrcpy(eof_star_power_menu_mark_text, "&Mark");
		}

		/* star power copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_sp_copy_menu[i].flags = 0;
			if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				ustrncpy(eof_menu_sp_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				ustrcpy(eof_menu_sp_copy_menu_text[i],"");
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
			ustrcpy(eof_solo_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_solo_menu[1].flags = D_DISABLED;
			ustrcpy(eof_solo_menu_mark_text, "&Mark");
		}

		/* solo copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_solo_copy_menu[i].flags = 0;
			if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				ustrncpy(eof_menu_solo_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				ustrcpy(eof_menu_solo_copy_menu_text[i],"");
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
			ustrcpy(eof_lyric_line_menu_mark_text, "Re-&Mark\tCtrl+M");
		}
		else
		{
			eof_lyric_line_menu[1].flags = D_DISABLED;
			eof_lyric_line_menu[4].flags = D_DISABLED;
			ustrcpy(eof_lyric_line_menu_mark_text, "&Mark\tCtrl+M");
		}

		/* arpeggio mark/remark */
		if(inarpeggio)
		{
			eof_arpeggio_menu[1].flags = 0;				//Note>Pro Guitar>Arpeggio>Remove
			ustrcpy(eof_arpeggio_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_arpeggio_menu[1].flags = D_DISABLED;
			ustrcpy(eof_arpeggio_menu_mark_text, "&Mark");
		}

		/* arpeggio copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_arpeggio_copy_menu[i].flags = 0;
			if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				ustrncpy(eof_menu_arpeggio_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				ustrcpy(eof_menu_arpeggio_copy_menu_text[i],"");
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
			eof_note_menu[3].flags = D_DISABLED;	//Note>Resnap
		}
		else
		{
			eof_note_menu[3].flags = 0;
		}

		if(eof_vocals_selected)
		{	//PART VOCALS SELECTED
			eof_note_menu[0].flags = D_DISABLED;	//Note>Toggle
			eof_note_menu[1].flags = D_DISABLED;	//Note>Transpose Up
			eof_note_menu[2].flags = D_DISABLED;	//Note>Transpose Down
			eof_note_menu[4].flags = D_DISABLED;	//Note>Solos
			eof_note_menu[5].flags = D_DISABLED;	//Note>Star power
			eof_note_menu[7].flags = D_DISABLED;	//Note>Edit Name
			eof_note_menu[9].flags = D_DISABLED;	//Note>Toggle Crazy
			eof_note_menu[10].flags = D_DISABLED;	//Note>HOPO
			eof_note_menu[11].flags = D_DISABLED;	//Note>Trill> submenu
			eof_note_menu[12].flags = D_DISABLED;	//Note>Tremolo> submenu
			eof_note_menu[16].flags = D_DISABLED;	//Note>Drum> submenu
			eof_note_menu[17].flags = D_DISABLED;	//Note>Pro Guitar> submenu

			eof_note_menu[18].flags = 0;	//Note>Lyrics> submenu
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
			eof_note_menu[7].flags = 0;				//Note>Edit Name
			eof_note_menu[11].flags = 0;			//Note>Trill> submenu
			eof_note_menu[12].flags = 0;			//Note>Tremolo> submenu
			eof_note_menu[18].flags = D_DISABLED;	//Note>Lyrics> submenu

			/* transpose up */
			if(eof_transpose_possible(-1))
			{
				eof_note_menu[1].flags = 0;			//Note>Transpose Up
			}
			else
			{
				eof_note_menu[1].flags = D_DISABLED;
			}

			/* transpose down */
			if(eof_transpose_possible(1))
			{
				eof_note_menu[2].flags = 0;			//Note>Transpose Down
			}
			else
			{
				eof_note_menu[2].flags = D_DISABLED;
			}

			/* toggle crazy */
			if((track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR))
			{	//When a guitar track is active
				eof_note_menu[9].flags = 0;				//Note>Toggle Crazy
			}
			else
			{
				eof_note_menu[9].flags = D_DISABLED;
			}

			if(eof_selected_track != EOF_TRACK_DRUM)
			{	//When PART DRUMS is not active
				eof_note_menu[16].flags = D_DISABLED;	//Note>Drum> submenu
			}
			else
			{	//When PART DRUMS is active
				eof_note_menu[16].flags = 0;
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
				eof_note_menu[10].flags = 0;	//Enable Note>HOPO> submenu
				eof_note_menu[10].child = eof_legacy_hopo_menu;	//Set it to the legacy guitar HOPO menu
			}
			else if(track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR)
			{
				eof_note_menu[10].flags = 0;	//Enable Note>HOPO> submenu
				eof_note_menu[10].child = eof_pro_guitar_hopo_menu;	//Set it to the pro guitar HOPO menu
			}
			else
			{
				eof_note_menu[10].flags = D_DISABLED;
			}

			/* Pro Guitar mode notation> */
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If the active track is a pro guitar track
				eof_note_menu[17].flags = 0;			//Note>Pro Guitar> submenu

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
						ustrncpy(eof_menu_thin_notes_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
							//Copy the track name to the menu string
					}
					else
					{	//Write a blank string for the track name
						ustrcpy(eof_menu_thin_notes_menu_text[i],"");
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
				eof_note_menu[17].flags = D_DISABLED;
			}

			/* Trill mark/remark*/
			if(intrill)
			{
				eof_trill_menu[1].flags = 0;	//Note>Trill>Remove
				ustrcpy(eof_trill_menu_mark_text, "Re-&Mark");
			}
			else
			{
				eof_trill_menu[1].flags = D_DISABLED;
				ustrcpy(eof_trill_menu_mark_text, "&Mark");
			}

			/* Trill copy from */
			for(i = 0; i < EOF_TRACKS_MAX; i++)
			{	//For each track supported by EOF
				eof_menu_trill_copy_menu[i].flags = 0;
				if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
				{	//If the track exists, copy its name into the string used by the track menu
					ustrncpy(eof_menu_trill_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
						//Copy the track name to the menu string
				}
				else
				{	//Otherwise write a blank string for the track name
					ustrcpy(eof_menu_trill_copy_menu_text[i],"");
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
				ustrcpy(eof_tremolo_menu_mark_text, "Re-&Mark");
			}
			else
			{
				eof_tremolo_menu[1].flags = D_DISABLED;
				ustrcpy(eof_tremolo_menu_mark_text, "&Mark");
			}

			/* Tremolo copy from */
			for(i = 0; i < EOF_TRACKS_MAX; i++)
			{	//For each track supported by EOF
				eof_menu_tremolo_copy_menu[i].flags = 0;
				if((i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
				{	//If the track exists, copy its name into the string used by the track menu
					ustrncpy(eof_menu_tremolo_copy_menu_text[i], eof_song->track[i + 1]->name, EOF_TRACK_NAME_SIZE);
						//Copy the track name to the menu string
				}
				else
				{	//Otherwise write a blank string for the track name
					ustrcpy(eof_menu_tremolo_copy_menu_text[i],"");
				}
				if(!eof_get_num_tremolos(eof_song, i + 1) || (i + 1 == eof_selected_track))
				{	//If the track has no tremolo phrases or is the active track
					eof_menu_tremolo_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
				}
			}

			/* Rename Trill and Tremolo menus as necessary for the drum track */
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_KEYS_TRACK_BEHAVIOR))
			{	//If a legacy/pro guitar/bass/keys track is active, set the guitar terminology for trill and tremolo sections
				ustrcpy(eof_trill_menu_text, "Trill");
				ustrcpy(eof_tremolo_menu_text, "Tremolo");
			}
			else if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If a legacy drum track is active, set the drum terminology for trill and tremolo sections
				ustrcpy(eof_trill_menu_text, "Special Drum Roll");
				ustrcpy(eof_tremolo_menu_text, "Drum Roll");
			}
			else
			{	//Disable these submenus unless a track that can use them is active
				eof_note_menu[11].flags = D_DISABLED;	//Note>Trill> submenu
				eof_note_menu[12].flags = D_DISABLED;	//Note>Tremolo> submenu
			}
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_KEYS_TRACK_BEHAVIOR))
			{	//If this is a keys track, ensure the tremolo menu still gets disabled
				eof_note_menu[12].flags = D_DISABLED;	//Note>Tremolo> submenu
			}

			/* Slider */
			if(inslider)
			{
				eof_slider_menu[1].flags = 0;	//Note>Slider>Remove
				ustrcpy(eof_slider_menu_mark_text, "Re-&Mark\tShift+S");
			}
			else
			{
				eof_slider_menu[1].flags = D_DISABLED;
				ustrcpy(eof_slider_menu_mark_text, "&Mark\tShift+S");
			}

			/* Toggle>Purple */
			if(eof_count_track_lanes(eof_song, eof_selected_track) > 4)
			{	//If the active track has a sixth usable lane
				eof_note_toggle_menu[4].flags = 0;	//Note>Toggle>Purple
			}
			else
			{
				eof_note_toggle_menu[4].flags = D_DISABLED;
			}

			/* Toggle>Orange */
			if(eof_count_track_lanes(eof_song, eof_selected_track) > 5)
			{	//If the active track has a sixth usable lane
				eof_note_toggle_menu[5].flags = 0;	//Note>Toggle>Orange
			}
			else
			{
				eof_note_toggle_menu[5].flags = D_DISABLED;
			}
		}//PART VOCALS NOT SELECTED
	}//if(eof_song && eof_song_loaded)
}

int eof_menu_note_transpose_up(void)
{
	unsigned long i, j;
	unsigned long max = 31;	//This represents the highest valid note bitmask, based on the current track options (including open bass strumming)
	unsigned long flags, note, tracknum;

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
					for(j = 15; j > 0; j--)
					{	//For the upper 15 frets
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[j] = eof_song->pro_guitar_track[tracknum]->note[i]->frets[j-1];	//Cycle fret values up from lower lane
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[0] = 0xFF;	//Re-initialize lane 1 to the default of (muted)
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_transpose_down(void)
{
	unsigned long i, j;
	unsigned long note, tracknum;

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
					for(j = 0; j < 15; j++)
					{	//For the lower 15 frets
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[j] = eof_song->pro_guitar_track[tracknum]->note[i]->frets[j+1];	//Cycle fret values down from upper lane
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[15] = 0xFF;	//Re-initialize lane 15 to the default of (muted)
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_transpose_up_octave(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_transpose_possible(-12))	//If lyric cannot move up one octave
		return 1;
	if(!eof_vocals_selected)			//If PART VOCALS is not active
		return 1;

	eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (eof_song->vocal_track[tracknum]->lyric[i]->note != EOF_LYRIC_PERCUSSION))
			eof_song->vocal_track[tracknum]->lyric[i]->note+=12;

	return 1;
}

int eof_menu_note_transpose_down_octave(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_transpose_possible(12))		//If lyric cannot move down one octave
		return 1;
	if(!eof_vocals_selected)		//If PART VOCALS is not active
		return 1;


	eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (eof_song->vocal_track[tracknum]->lyric[i]->note != EOF_LYRIC_PERCUSSION))
			eof_song->vocal_track[tracknum]->lyric[i]->note-=12;

	return 1;
}

int eof_menu_note_resnap(void)
{
	unsigned long i;
	unsigned long oldnotes = eof_get_track_size(eof_song, eof_selected_track);

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
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_note_delete(void)
{
	unsigned long i, d = 0;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
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
		eof_determine_phrase_status();
	}
	return 1;
}

int eof_menu_note_toggle_green(void)
{
	unsigned long i;
	unsigned long flags, note, tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(eof_count_selected_notes(NULL, 0) <= 0)
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
	return 1;
}

int eof_menu_note_toggle_red(void)
{
	unsigned long i;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;;

	if(eof_count_selected_notes(NULL, 0) <= 0)
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
	return 1;
}

int eof_menu_note_toggle_yellow(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;

	if(eof_count_selected_notes(NULL, 0) <= 0)
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
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,1);	//Set the yellow cymbal flag on all drum notes at this position
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_blue(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;

	if(eof_count_selected_notes(NULL, 0) <= 0)
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
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,1);	//Set the blue cymbal flag on all drum notes at this position
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_purple(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;

	if(eof_count_selected_notes(NULL, 0) <= 0)
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
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,1);	//Set the green cymbal flag on all drum notes at this position
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_orange(void)
{
	unsigned long i;
	unsigned long flags, note, tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(eof_count_track_lanes(eof_song, eof_selected_track) < 6)
	{
		return 1;	//Don't do anything if there is less than 6 tracks available
	}
	if(eof_count_selected_notes(NULL, 0) <= 0)
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
	return 1;
}

int eof_menu_note_toggle_crazy(void)
{
	unsigned long i;
	int u = 0;	//Is set to nonzero when an undo state has been made
	unsigned long track_behavior = eof_song->track[eof_selected_track]->track_behavior;
	unsigned long flags;

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
	return 1;
}

int eof_menu_note_toggle_double_bass(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

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
	return 1;
}

int eof_menu_note_remove_double_bass(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

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
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_green(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;

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
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,2);	//Toggle the green cymbal flag on all drum notes at this position
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_yellow(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;

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
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,2);	//Toggle the yellow cymbal flag on all drum notes at this position
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_blue(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;

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
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,2);	//Toggle the blue cymbal flag on all drum notes at this position
			}
		}
	}
	return 1;
}

int eof_menu_note_remove_cymbal(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;
	unsigned long flags, oldflags, note;

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
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,0);	//Clear the yellow cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,0);	//Clear the blue cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,0);	//Clear the green cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN,0);	//Clear the open hi hat cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL,0);	//Clear the pedal hi hat cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_SIZZLE,0);			//Clear the sizzle hi hat cymbal flag on all drum notes at this position
			}
		}
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
	return 1;
}

int eof_menu_note_push_up(void)
{
	unsigned long i;
	float porpos;
	long beat;

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
	return 1;
}

int eof_menu_note_create_bre(void)
{
	unsigned long i;
	long first_pos = 0;
	long last_pos = eof_music_length;
	EOF_NOTE * new_note = NULL;

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

	/* create the BRE marking note */
	if((first_pos != 0) && (last_pos != eof_music_length))
	{
		new_note = eof_track_add_create_note(eof_song, eof_selected_track, 31, first_pos, last_pos - first_pos, EOF_NOTE_SPECIAL, NULL);
	}
	return 1;
}

/* split a lyric into multiple pieces (look for ' ' characters) */
static void eof_split_lyric(int lyric)
{
	unsigned long i, l, c = 0, lastc;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int piece = 1;
	int pieces = 1;
	char * token = NULL;
	EOF_LYRIC * new_lyric = NULL;

	if(!eof_vocals_selected)
		return;

	/* see how many pieces there are */
	for(i = 0; i < strlen(eof_song->vocal_track[tracknum]->lyric[lyric]->text); i++)
	{
		lastc = c;
		c = eof_song->vocal_track[tracknum]->lyric[lyric]->text[i];
		if((c == ' ') && (lastc != ' '))
		{
			if((i + 1 < strlen(eof_song->vocal_track[tracknum]->lyric[lyric]->text)) && (eof_song->vocal_track[tracknum]->lyric[lyric]->text[i+1] != ' '))
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
	strtok(eof_song->vocal_track[tracknum]->lyric[lyric]->text, " ");
	do
	{
		token = strtok(NULL, " ");
		if(token)
		{
			new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, eof_song->vocal_track[tracknum]->lyric[lyric]->note, eof_song->vocal_track[tracknum]->lyric[lyric]->pos + ((double)l / pieces) * piece, (double)l / pieces - 20.0, 0, token);
			piece++;
		}
	} while(token != NULL);
	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
}

int eof_menu_split_lyric(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
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
	ustrcpy(eof_etext, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text);
	if(eof_popup_dialog(eof_split_lyric_dialog, 2) == 3)
	{
		if(ustricmp(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext))
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			ustrcpy(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext);
			eof_split_lyric(eof_selection.current);
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_menu_solo_mark(void)
{
	unsigned long i, j;
	long insp = -1;
	long sel_start = -1;
	long sel_end = 0;
	EOF_PHRASE_SECTION *soloptr = NULL;

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
		eof_track_add_solo(eof_song, eof_selected_track, sel_start, sel_end);
	}
	else
	{
		soloptr = eof_get_solo(eof_song, eof_selected_track, insp);
		soloptr->start_pos = sel_start;
		soloptr->end_pos = sel_end;
	}
	return 1;
}

int eof_menu_solo_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *soloptr = NULL;

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
	return 1;
}

int eof_menu_solo_erase_all(void)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *sectionptr;

	if(alert(NULL, "Erase all solos from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = 0; ctr < eof_get_num_solos(eof_song, eof_selected_track); ctr++)
		{	//For each existing solo section
			sectionptr = eof_get_solo(eof_song, eof_selected_track, ctr);
			if(sectionptr->name)
			{	//If this phrase has a name
				free(sectionptr->name);	//Free it
			}
		}
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
		eof_track_add_star_power_path(eof_song, eof_selected_track, sel_start, sel_end);
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
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_star_power_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *starpowerptr = NULL;

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
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_star_power_erase_all(void)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *sectionptr;

	if(alert(NULL, "Erase all star power from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = 0; ctr < eof_get_num_star_power_paths(eof_song, eof_selected_track); ctr++)
		{	//For each existing star power section
			sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, ctr);
			if(sectionptr->name)
			{	//If this phrase has a name
				free(sectionptr->name);	//Free it
			}
		}
		eof_set_num_star_power_paths(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_lyric_line_mark(void)
{
	unsigned long i, j;
	unsigned long tracknum;
	long sel_start = -1;
	long sel_end = 0;
	int originalflags = 0; //Used to apply the line's original flags after the line is recreated

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
				if(sel_end >= eof_music_length)
				{
					sel_end = eof_music_length - 1;
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
	eof_vocal_track_add_line(eof_song->vocal_track[tracknum], sel_start, sel_end);

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
	return 1;
}

int eof_menu_lyric_line_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

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
	return 1;
}

int eof_menu_hopo_auto(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;

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
				{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
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
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_hopo_cycle(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	unsigned long track_behavior = eof_song->track[eof_selected_track]->track_behavior;

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
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
						}
					}
					else if(flags & EOF_NOTE_FLAG_NO_HOPO)
					{	//If the note was a forced off HOPO, make it an auto HOPO
						flags &= ~EOF_NOTE_FLAG_F_HOPO;		//Clear the forced on hopo flag
						flags &= ~EOF_NOTE_FLAG_NO_HOPO;	//Clear the forced off hopo flag
						if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
						{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
						}
					}
					else
					{	//If the note was an auto HOPO, make it a forced on HOPO
						flags |= EOF_NOTE_FLAG_F_HOPO;	//Turn on forced on hopo
						if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
						{	//If this is a pro guitar note
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;		//Set the hammer on flag (default HOPO type)
							flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
							flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
						}
					}
					eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				}
			}
		}
		eof_determine_phrase_status();
	}
	return 1;
}

int eof_menu_hopo_force_on(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;

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
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;		//Set the hammer on flag (default HOPO type)
					flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
					flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
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
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_hopo_force_off(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;

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
				{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
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
	eof_determine_phrase_status();
	return 1;
}

int eof_transpose_possible(int dir)
{
	unsigned long i, note, tracknum;
	unsigned long max = 16;	//This represents the highest note bitmask value that will be allowed to transpose up, based on the current track options (including open bass strumming)

	/* no notes, no transpose */
	if(eof_vocals_selected)
	{
		if(eof_get_track_size(eof_song, eof_selected_track) <= 0)
		{
			return 0;
		}

		if(eof_count_selected_notes(NULL, 1) <= 0)
		{
			return 0;
		}

		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//Test if the lyric can transpose the given amount in the given direction
			if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
			{
				if((eof_get_note_note(eof_song, eof_selected_track, i) == 0) || (eof_get_note_note(eof_song, eof_selected_track, i) == EOF_LYRIC_PERCUSSION))
				{	//Cannot transpose a pitchless lyric or a vocal percussion note
					return 0;
				}
				if(eof_get_note_note(eof_song, eof_selected_track, i) - dir < MINPITCH)
				{
					return 0;
				}
				else if(eof_get_note_note(eof_song, eof_selected_track, i) - dir > MAXPITCH)
				{
					return 0;
				}
			}
		}
		return 1;
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
		if(eof_get_track_size(eof_song, eof_selected_track) <= 0)
		{
			return 0;
		}
		if(eof_count_selected_notes(NULL, 1) <= 0)
		{
			return 0;
		}

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
					return 0;	//Disallow tranposing for this selection of notes
				}
				if((note & 1) && (dir > 0))
				{
					return 0;
				}
				else if((note & max) && (dir < 0))
				{
					return 0;
				}
			}
		}
		return 1;
	}
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
	ustrcpy(eof_etext, "");

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
		ustrcpy(new_lyric->text, eof_etext);
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
	ustrcpy(eof_etext, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text);
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
				ustrcpy(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext);
				eof_fix_lyric(eof_song->vocal_track[tracknum],eof_selection.current);	//Correct the freestyle character if necessary
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_menu_set_freestyle(char status)
{
	unsigned long i=0,ctr=0;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

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

	return 1;
}

char eof_note_edit_name[EOF_NAME_LENGTH+1] = {0};

DIALOG eof_pro_guitar_note_dialog[] =
{
/*	(proc)					(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          (dp2)          (dp3) */
	{d_agup_window_proc,    0,   48,  230, 352,2,   23,  0,    0,      0,         0,   "Edit pro guitar note",NULL, NULL },
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

	{d_agup_text_proc,      10,  292, 64,  8,  2,   23,  0,    0,      0,         0,   "Slide:",     NULL,          NULL },
	{d_agup_text_proc,      10,  312, 64,  8,  2,   23,  0,    0,      0,         0,   "Mute:",      NULL,          NULL },
	{d_agup_text_proc,      10,  332, 64,  8,  2,   23,  0,    0,      0,         0,   "Strum:",     NULL,          NULL },
	{d_agup_radio_proc,		10,  272, 38,  16, 2,   23,  0,    0,      1,         0,   "HO",         NULL,          NULL },
	{d_agup_radio_proc,		58,  272, 38,  16, 2,   23,  0,    0,      1,         0,   "PO",         NULL,          NULL },
	{d_agup_radio_proc,		102, 272, 45,  16, 2,   23,  0,    0,      1,         0,   "Tap",        NULL,          NULL },
	{d_agup_radio_proc,		154, 272, 50,  16, 2,   23,  0,    0,      1,         0,   "None",       NULL,          NULL },
	{d_agup_radio_proc,		58,  292, 38,  16, 2,   23,  0,    0,      2,         0,   "Up",         NULL,          NULL },
	{d_agup_radio_proc,		102, 292, 54,  16, 2,   23,  0,    0,      2,         0,   "Down",       NULL,          NULL },
	{d_agup_radio_proc,		154, 292, 64,  16, 2,   23,  0,    0,      2,         0,   "Neither",    NULL,          NULL },
	{d_agup_radio_proc,		46,  312, 58,  16, 2,   23,  0,    0,      3,         0,   "String",     NULL,          NULL },
	{d_agup_radio_proc,		102, 312, 52,  16, 2,   23,  0,    0,      3,         0,   "Palm",       NULL,          NULL },
	{d_agup_radio_proc,		154, 312, 64,  16, 2,   23,  0,    0,      3,         0,   "Neither",    NULL,          NULL },
	{d_agup_radio_proc,		46,  332, 38,  16, 2,   23,  0,    0,      4,         0,   "Up",         NULL,          NULL },
	{d_agup_radio_proc,		102, 332, 54,  16, 2,   23,  0,    0,      4,         0,   "Down",       NULL,          NULL },
	{d_agup_radio_proc,		154, 332, 64,  16, 2,   23,  0,    0,      4,         0,   "Either",     NULL,          NULL },

	{d_agup_button_proc,    10,  360, 20,  28, 2,   23,  0,    D_EXIT, 0,         0,   "<-",         NULL,          NULL },
	{d_agup_button_proc,    35,  360, 50,  28, 2,   23,  '\r', D_EXIT, 0,         0,   "OK",         NULL,          NULL },
	{d_agup_button_proc,    90,  360, 50,  28, 2,   23,  0,    D_EXIT, 0,         0,   "Apply",      NULL,          NULL },
	{d_agup_button_proc,    145, 360, 50,  28, 2,   23,  0,    D_EXIT, 0,         0,   "Cancel",     NULL,          NULL },
	{d_agup_button_proc,    200, 360, 20,  28, 2,   23,  0,    D_EXIT, 0,         0,   "->",         NULL,          NULL },
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
	unsigned long previous_note = 0, next_note = 0;
	int retval;

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
		eof_pro_guitar_note_dialog[52].flags = D_HIDDEN;	//Until a previous note is found, assume there isn't one and hide the button
		for(ctr = eof_selection.current; ctr > 0; ctr--)
		{	//For each note before the selected note in the active track, in reverse
			if(eof_get_note_type(eof_song, eof_selected_track, ctr - 1) == eof_note_type)
			{	//If a valid previous note was found
				previous_note = ctr - 1;
				eof_pro_guitar_note_dialog[52].flags = D_EXIT;	//Make the previous note button clickable again
				break;
			}
		}
		eof_pro_guitar_note_dialog[56].flags = D_HIDDEN;	//Until a next note is found, assume there isn't one and hide the button
		for(ctr = eof_selection.current + 1; ctr <eof_get_track_size(eof_song, eof_selected_track); ctr++)
		{	//For each note after the selected note in the active track
			if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
			{	//If a valid next note was found
				next_note = ctr;
				eof_pro_guitar_note_dialog[56].flags = D_EXIT;	//Make the next note button clickable again
				break;
			}
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
						snprintf(eof_fret_strings[ctr], 3, "X");
					}
					else
					{
						snprintf(eof_fret_strings[ctr], 3, "%d", eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr] & 0x7F);	//Mask out the MSB to obtain the fret value
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

	//Update the note flag radio buttons
		for(ctr = 0; ctr < 13; ctr++)
		{	//Clear each of the 13 status radio buttons
			eof_pro_guitar_note_dialog[39 + ctr].flags = 0;
		}
		flags = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->flags;
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
		{	//Select "HO"
			eof_pro_guitar_note_dialog[39].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PO)
		{	//Select "PO"
			eof_pro_guitar_note_dialog[40].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
		{	//Select "Tap"
			eof_pro_guitar_note_dialog[41].flags = D_SELECTED;
		}
			else
		{	//Select "None"
			eof_pro_guitar_note_dialog[42].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
		{	//Select Slide "Up"
			eof_pro_guitar_note_dialog[43].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
		{	//Select Slide "Down"
			eof_pro_guitar_note_dialog[44].flags = D_SELECTED;
		}
		else
		{	//Select Slide "Neither"
			eof_pro_guitar_note_dialog[45].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)
		{	//Select Mute "String"
			eof_pro_guitar_note_dialog[46].flags = D_SELECTED;
		}
		else if(flags &EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
		{	//Select Mute "Palm"
			eof_pro_guitar_note_dialog[47].flags = D_SELECTED;
		}
		else
		{	//Select Mute "Neither"
			eof_pro_guitar_note_dialog[48].flags = D_SELECTED;
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
		{	//Select Strum "Up"
			eof_pro_guitar_note_dialog[49].flags = D_SELECTED;
		}
		else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM)
		{	//Selct Strum "Down"
			eof_pro_guitar_note_dialog[50].flags = D_SELECTED;
		}
		else
		{	//Select Strum "Either"
			eof_pro_guitar_note_dialog[51].flags = D_SELECTED;
		}

		bitmask = 0;
		retval = eof_popup_dialog(eof_pro_guitar_note_dialog, 0);	//Run the dialog
		if((retval == 53) || (retval == 54))
		{	//If user clicked OK or Apply
			//Validate and store the input
			if(eof_count_selected_notes(NULL, 0) > 1)
			{	//If multiple notes are selected, warn the user
				if(alert(NULL, "Warning:  This information will be applied to all selected notes.", NULL, "&OK", "&Cancel", 0, 0) == 2)
				{	//If user opts to cancel the operation
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
						eof_menu_note_delete();		//All selected notes must be deleted
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
					if(eof_pro_guitar_note_dialog[39].flags == D_SELECTED)
					{	//HO is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;	//Set the hammer on flag
						flags &= (~EOF_NOTE_FLAG_NO_HOPO);		//Clear the forced HOPO off note
						flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO flag
					}
					else if(eof_pro_guitar_note_dialog[40].flags == D_SELECTED)
					{	//PO is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;	//Set the pull off flag
						flags &= (~EOF_NOTE_FLAG_NO_HOPO);		//Clear the forced HOPO off flag
						flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO flag
					}
					else if(eof_pro_guitar_note_dialog[41].flags == D_SELECTED)
					{	//Tap is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_TAP;	//Set the tap flag
						flags &= (~EOF_NOTE_FLAG_NO_HOPO);		//Clear the forced HOPO off flag
						flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO flag
					}
					if(eof_pro_guitar_note_dialog[43].flags == D_SELECTED)
					{	//Slide Up is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
					}
					else if(eof_pro_guitar_note_dialog[44].flags == D_SELECTED)
					{	//Slide Down is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
					}
					if(eof_pro_guitar_note_dialog[46].flags == D_SELECTED)
					{	//Mute String is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
					}
					else if(eof_pro_guitar_note_dialog[47].flags == D_SELECTED)
					{	//Mute Palm is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
					}
					if(eof_pro_guitar_note_dialog[49].flags == D_SELECTED)
					{	//Strum Up is selected
						flags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
					}
					else if(eof_pro_guitar_note_dialog[50].flags == D_SELECTED)
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
					if((eof_note_compare(eof_selected_track, eof_selection.current, ctr) == 0) && ustrcmp(eof_note_edit_name, eof_get_note_name(eof_song, eof_selected_track, ctr)))
					{	//If this note matches the one that was edited but the name is different
						if(alert(NULL, "Update other matching notes in this track to have the same name?", NULL, "&Yes", "&No", 'y', 'n') == 1)
						{	//If the user opts to use the updated note name on matching notes in this track
							for(; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
							{	//For each note in the active track, starting from the one that just matched the comparison
								if((eof_note_compare(eof_selected_track, eof_selection.current, ctr) == 0) && (eof_selection.current != ctr))
								{	//If this note matches the note that was edited, and we're not comparing the note to itself, copy the edited note's name to this note
									if(!undo_made)
									{	//If an undo state hasn't been made yet
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										undo_made = 1;
									}
									ustrncpy(eof_get_note_name(eof_song, eof_selected_track, ctr), eof_note_edit_name, sizeof(junknote.name));
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
					if((ctr != eof_selection.current) && (eof_note_compare(eof_selected_track, eof_selection.current, ctr) == 0))
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
									snprintf(autoprompt, sizeof(autoprompt), "Set the name of selected notes (%s) to \"%s\"?",pro_guitar_string, newname);
								}
								else
								{	//Otherwise use a generic prompt
									snprintf(autoprompt, sizeof(autoprompt), "Set selected notes' name to \"%s\"?",newname);
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
												ustrncpy(tempptr, newname, sizeof(junknote.name));	//Update the note's name to the user selection
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
					if((ctr != eof_selection.current) && (eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type) && (eof_note_compare(eof_selected_track, eof_selection.current, ctr) == 0))
					{	//If this note isn't the one that was just edited, but it matches it and is in the active track difficulty
						if(legacymask != eof_song->pro_guitar_track[tracknum]->note[ctr]->legacymask)
						{	//If the two notes have different legacy bitmasks
							if(alert(NULL, "Update other matching notes in this track difficulty to have the same legacy bitmask?", NULL, "&Yes", "&No", 'y', 'n') == 1)
							{	//If the user opts to use the updated note legacy bitmask on matching notes in this track difficulty
								for(; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
								{	//For each note in the active track, starting from the one that just matched the comparison
									if((ctr != eof_selection.current) && (eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type) && (eof_note_compare(eof_selected_track, eof_selection.current, ctr) == 0))
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
					if((ctr != eof_selection.current) && (eof_note_compare(eof_selected_track, eof_selection.current, ctr) == 0))
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
									snprintf(autoprompt, sizeof(autoprompt), "Set the legacy bitmask of selected notes (%s) to \"%s\"?",pro_guitar_string, autobitmask);
								}
								else
								{	//Otherwise use a generic prompt
									snprintf(autoprompt, sizeof(autoprompt), "Set selected notes' legacy bitmask to \"%s\"?",autobitmask);
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
			if(retval == 54)
			{	//If the user clicked Apply, re-render the screen to reflect any changes made
				eof_render();
			}
		}//If user clicked OK or Apply
		else if(retval == 52)
		{	//If user clicked previous
			memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
			eof_selection.current = previous_note;	//Set the previous note as the currently selected note
			eof_selection.multi[previous_note] = 1;	//Ensure the note selection includes the previous note
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, previous_note) + eof_av_delay);	//Seek to previous note
			eof_render();	//Redraw the screen
		}
		else if(retval == 56)
		{	//If user clicked next
			memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
			eof_selection.current = next_note;	//Set the next note as the currently selected note
			eof_selection.multi[next_note] = 1;	//Ensure the note selection includes the next note
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, next_note) + eof_av_delay);	//Seek to next note
			eof_render();	//Redraw the screen
		}
	}while((retval == 52) || (retval == 54) || (retval == 56));	//Re-run this dialog if the user clicked previous, apply or next

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_note_toggle_tapping(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

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
				flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO on flag (no strum required for this note)
				flags &= ~(EOF_NOTE_FLAG_NO_HOPO);		//Clear the HOPO off flag
				flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
				flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
			}
			else
			{	//If tapping was just disabled
				flags &= ~(EOF_NOTE_FLAG_F_HOPO);		//Clear the legacy HOPO on flag
			}
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_note_remove_tapping(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

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
				flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
				flags &= ~(EOF_NOTE_FLAG_F_HOPO);			//Clear the legacy HOPO on flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_slide_up(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;			//Toggle the slide up flag
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the slide down flag
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	return 1;
}

int eof_menu_note_toggle_slide_down(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;		//Toggle the slide down flag
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the slide down flag
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	return 1;
}

int eof_menu_note_remove_slide(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;							//Save an extra copy of the original flags
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the tap flag
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the tap flag
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_note_toggle_palm_muting(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

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
	return 1;
}

int eof_menu_note_remove_palm_muting(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;

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
		if((sel_end >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (sel_start <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos))
		{	//If the selection of notes is within this arpeggio's start and end position
			existingphrase = 1;	//Note it
			existingphrasenum = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(!existingphrase)
	{	//If the selected notes are not within an existing arpeggio phrase, create one applying to the active difficulty
		eof_track_add_section(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_note_type, sel_start, sel_end, 0, NULL);
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
	return 1;
}

int eof_menu_arpeggio_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Don't allow this function to run unless a pro guitar track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_song->pro_guitar_track[tracknum]->arpeggios; j++)
			{	//For each arpeggio section in the track
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos))
				{	//If the note is encompassed within this arpeggio section
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_pro_guitar_track_delete_arpeggio(eof_song->pro_guitar_track[tracknum], j);	//Delete the arpeggio section
					break;
				}
			}
		}
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
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

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
	tracknum = eof_song->track[eof_selected_track]->tracknum;
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
		eof_track_add_section(eof_song, eof_selected_track, EOF_TRILL_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		sectionptr = eof_get_trill(eof_song, eof_selected_track, existingphrasenum);
		sectionptr->start_pos = sel_start;
		sectionptr->end_pos = sel_end;
	}
	eof_determine_phrase_status();
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
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

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
	tracknum = eof_song->track[eof_selected_track]->tracknum;
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
		eof_track_add_section(eof_song, eof_selected_track, EOF_TREMOLO_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		sectionptr = eof_get_tremolo(eof_song, eof_selected_track, existingphrasenum);
		sectionptr->start_pos = sel_start;
		sectionptr->end_pos = sel_end;
	}
	eof_determine_phrase_status();
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
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

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
	tracknum = eof_song->track[eof_selected_track]->tracknum;
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
		eof_track_add_section(eof_song, eof_selected_track, EOF_SLIDER_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		sectionptr = eof_get_slider(eof_song, eof_selected_track, existingphrasenum);
		sectionptr->start_pos = sel_start;
		sectionptr->end_pos = sel_end;
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_trill_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
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
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_tremolo_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
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
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_slider_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_format != EOF_LEGACY_TRACK_FORMAT))
		return 1;	//Do not allow this function to run unless a legacy guitar track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
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
	eof_determine_phrase_status();
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
	eof_determine_phrase_status();
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
	eof_determine_phrase_status();
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
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_note_clear_legacy_values(void)
{
	unsigned long i;
	long u = 0;
	unsigned long tracknum;

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
	return 1;
}

int eof_menu_note_edit_name(void)
{
	unsigned long i;
	char *notename = NULL, undo_made = 0;

	if(!eof_music_catalog_playback)
	{
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
			ustrcpy(eof_etext, notename);
		}

		if(eof_popup_dialog(eof_note_name_dialog, 2) == 3)	//User hit OK
		{
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the track
				if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
				{	//If the note is in the active instrument difficulty and is selected
					notename = eof_get_note_name(eof_song, eof_selected_track, i);	//Get the note's name
					if(notename)
					{	//As long as there wasn't an error accessing the note name
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
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_pro_guitar_toggle_strum_down(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;

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
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_note_remove_strum_direction(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;

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
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
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
			eof_track_add_solo(sp, desttrack, ptr->start_pos, ptr->end_pos);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status();
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
			eof_track_add_star_power_path(sp, desttrack, ptr->start_pos, ptr->end_pos);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status();
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
			eof_track_add_section(sp, desttrack, EOF_ARPEGGIO_SECTION, ptr->difficulty, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status();
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
			eof_track_add_section(sp, desttrack, EOF_TRILL_SECTION, ptr->difficulty, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status();
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
			eof_track_add_section(sp, desttrack, EOF_TREMOLO_SECTION, ptr->difficulty, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the destination track
		}
	}
	eof_determine_phrase_status();
	return 1;	//Return completion
}

int eof_menu_note_toggle_hi_hat_open(void)
{
	unsigned long i, flags;
	long u = 0;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
			{	//If this drum note contains a yellow gem
				flags |= EOF_NOTE_FLAG_Y_CYMBAL;	//Automatically mark as a cymbal
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Clear the pedal controlled hi hat status
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;			//Clear the sizzle hi hat status
				flags ^= EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;		//Toggle the open hi hat status
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_hi_hat_pedal(void)
{
	unsigned long i, flags;
	long u = 0;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
			{	//If this drum note contains a yellow gem
				flags |= EOF_NOTE_FLAG_Y_CYMBAL;	//Automatically mark as a cymbal
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;	//Clear the open hi hat status
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;		//Clear the sizzle hi hat status
				flags ^= EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Toggle the pedal controlled hi hat status
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_hi_hat_sizzle(void)
{
	unsigned long i, flags;
	long u = 0;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
			{	//If this drum note contains a yellow gem
				flags |= EOF_NOTE_FLAG_Y_CYMBAL;	//Automatically mark as a cymbal
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;		//Clear the open hi hat status
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Clear the pedal controlled hi hat status
				flags ^= EOF_DRUM_NOTE_FLAG_Y_SIZZLE;			//Toggle the sizzle hi hat status
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_rimshot(void)
{
	unsigned long i, flags;
	long u = 0;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 2)
			{	//If this drum note contains a red note
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags ^= EOF_DRUM_NOTE_FLAG_R_RIMSHOT;	//Toggle the rim shot status
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);	//Apply the flag changes
			}
		}
	}
	return 1;
}

int eof_menu_pro_guitar_toggle_hammer_on(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;

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
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;	//Clear the pull off flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;	//Clear the tap flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
		eof_determine_phrase_status();
	}
	return 1;
}

int eof_menu_pro_guitar_remove_hammer_on(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

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
				flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
				flags &= ~(EOF_NOTE_FLAG_F_HOPO);			//Clear the legacy HOPO on flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_pro_guitar_toggle_pull_off(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;

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
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;	//Clear the hammer on flag
				flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;	//Clear the tap flag
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
		eof_determine_phrase_status();
	}
	return 1;
}

int eof_menu_pro_guitar_remove_pull_off(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

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
	eof_determine_phrase_status();
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
 	eof_log("eof_menu_note_toggle_ghost() entered", 1);

	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;

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
	return 1;
}
