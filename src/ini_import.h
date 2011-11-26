#ifndef EOF_INI_IMPORT_H
#define EOF_INI_IMPORT_H

extern char eof_ini_pro_drum_tag_present;	//Is set to nonzero if eof_import_ini() finds the "pro_drums = True" tag (to influence MIDI import)
extern char eof_ini_star_power_tag_present;	//Is set to nonzero if eof_import_ini() finds the "multiplier_note = 116" tag (to influence MIDI import)

int eof_import_ini(EOF_SONG * sp, char * fn);

#endif
