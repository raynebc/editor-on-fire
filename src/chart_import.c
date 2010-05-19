#include "main.h"
#include "song.h"
#include "beat.h"
#include "chart_import.h"
#include "feedback.h"

EOF_SONG * eof_import_chart(const char * fn)
{

	struct FeedbackChart * chart = NULL;
	EOF_SONG * sp = NULL;
	int err;
	
	chart = ImportFeedback((char *)fn, &err);
	if(chart)
	{
		printf("chart loaded\n");
		/* load audio */
		if(!eof_load_ogg(chart->audiofile))
		{
			DestroyFeedbackChart(chart, 1);
			return NULL;
		}
		eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
		eof_music_actual_length = eof_music_length;
		
		/* create empty song */
		sp = eof_create_song();
		if(!sp)
		{
			DestroyFeedbackChart(chart, 1);
			return NULL;
		}
		
		/* copy tags */
		if(chart->name)
		{
			strcpy(sp->tags->title, chart->name);
			printf("%s\n", sp->tags->title);
		}
		if(chart->artist)
		{
			strcpy(sp->tags->artist, chart->artist);
			printf("%s\n", sp->tags->artist);
		}
		if(chart->charter)
		{
			strcpy(sp->tags->frettist, chart->charter);
			printf("%s\n", sp->tags->frettist);
		}
		
		/* set up beat markers */
		sp->tags->ogg[0].midi_offset = chart->offset;
		eof_calculate_beats();
	}
	
	return sp;
}
