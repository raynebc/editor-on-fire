#include <allegro.h>
#include "menu/song.h"
#include "main.h"
#include "waveform.h"
#include "utility.h"

int eof_add_silence(const char * oggfn, unsigned long ms)
{
	char sys_command[1024] = {0};
	char backupfn[1024] = {0};
	char wavfn[1024] = {0};
	char soggfn[1024] = {0};
	if(ms == 0)
	{
		return 0;
	}
	
	set_window_title("Adjusting Silence...");
	replace_filename(wavfn, eof_song_path, "silence.wav", 1024);
	
	/* back up original file */
	sprintf(backupfn, "%s.backup", oggfn);
	if(!exists(backupfn))
	{
		eof_copy_file((char *)oggfn, backupfn);
	}
	delete_file(oggfn);
	
	/* encode the silence */
	replace_filename(soggfn, eof_song_path, "silence.ogg", 1024);
	sprintf(sys_command, "oggSilence -d %d -l%lu -o \"%s\"", alogg_get_bitrate_ogg(eof_music_track), ms, soggfn);
	if(system(sys_command))
	{
		eof_copy_file(backupfn, (char *)oggfn);
		eof_fix_window_title();
		return 0;
	}
	
	/* stitch the original file to the silence file */
	sprintf(sys_command, "oggCat \"%s\" \"%s\" \"%s\"", oggfn, soggfn, backupfn);
	if(system(sys_command))
	{
		eof_copy_file(backupfn, (char *)oggfn);
		eof_fix_window_title();
		return 0;
	}
	
	/* clean up */
	delete_file(soggfn);
	if(eof_load_ogg((char *)oggfn))
	{
		eof_fix_waveform_graph();
		eof_fix_window_title();
		return 1;
	}
	eof_fix_window_title();
	return 0;
}
