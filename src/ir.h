#ifndef EOF_IR_H
#define EOF_IR_H

int eof_export_immerrock_midi(EOF_SONG *sp, unsigned long track, unsigned char diff, char *fn);
	//Exports the specified pro guitar track difficulty to a MIDI file suited for use in the game Immerrock
int eof_export_immerrock_diff(EOF_SONG * sp, unsigned long gglead, unsigned long ggrhythm, unsigned long ggbass, unsigned char diff, char *destpath, char option);
	//Exports Immerrock files for the specified pro guitar tracks, for the specified difficulty in a folder with multiple files
	//gglead, ggrhythm and ggbass are the track numbers to export as these arrangements, or 0 if that arrangement is not specified for export
	//destpath will be the folder level at which the folder of Immerrock files will be written
	//If option is 1 (manual export of single arrangement), the created subfolder will include the name of the exported track
	//If the specified tracks are not pro guitar tracks, or have no notes in the specified difficulty, no files or folders are created
void eof_export_immerrock(char silent);
	//Chooses up to one lead, one rhythm and one bass arrangement for export based on the defined arrangement type of the active project's tracks
	//Uses eof_export_immerrock_diff() to export content for these arrangements so that all easy difficulty level content is in one folder, medium in another folder, etc.
	//If silent is nonzero, warnings and prompts are suppressed in order to perform a quick save operation

#endif
