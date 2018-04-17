#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif
#include "main.h"
#include "mix.h"
#include "notes.h"
#include "song.h"
#include "tuning.h"
#include "foflc/Lyric_storage.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int eof_expand_notes_window_text(char *src_buffer, char *dest_buffer, unsigned long dest_buffer_size)
{
	unsigned long src_index = 0, dest_index = 0, macro_index;
	char src_char, macro[1024], expanded_macro[1024];
	int macro_status;
	int allowempty = 0;	//Tracks whether an %EMPTY% macro was parsed

	if(!src_buffer || !dest_buffer || (dest_buffer_size < 1))
		return 0;	//Invalid parameters

	dest_buffer[0] = '\0';		//Empty the destination buffer
	if(src_buffer[0] == ';')	//A line beginning with a semicolon is a comment
		return 1;
	while((src_buffer[src_index] != '\0') || (dest_index + 1 >= dest_buffer_size))
	{	//Until the end of the source or destination buffer are reached
		src_char = src_buffer[src_index++];	//Read the next character

		if(src_char != '%')
		{	//If this isn't a macro
			dest_buffer[dest_index++] = src_char;	//Append it to the destination buffer
			dest_buffer[dest_index] = '\0';			//Terminate the string so that macro text can be concatenated properly
		}
		else if(src_buffer[src_index] != '%')
		{	//If the next character isn't also a percent sign (to be ignored as padding), parse the content up until the next percent sign and process it as a macro to be expanded
			macro_index = 0;
			while(src_buffer[src_index] != '\0')
			{	//Until the end of the source buffer is reached
				src_char = src_buffer[src_index++];	//Read the next character

				if(src_char == '%')
				{	//The closing percent sign of the macro was reached
					macro[macro_index] = '\0';	//Terminate the macro string
					macro_status = eof_expand_notes_window_macro(macro, expanded_macro, 1024);	//Convert the macro to static text

					if((macro_status == 2) || (macro_status == 3))
					{	//If this was a conditional macro
						char *ifptr, *elseptr, *endifptr, *nextptr;	//Used to track conditional macro handling
						char *eraseptr = NULL;						//Used to erase ELSE condition when the condition was true
						unsigned nested_level = 0;					//Track how many conditional if-else-endif macro levels we're currently nested, so we can make the correct parsing branch
						char nextcondition = 0;						//Tracks whether the next conditional macro is an if (1), an else (2), an endif (3) or none of those (0)
						unsigned long index = src_index;			//Used to iterate through the source buffer for processing macros

						while(1)
						{	//Process conditional macros until the correct branching is handled
							nextcondition = 0;				//Reset this status so that if no conditional macros are found, the while loop can exit
							nextptr = &src_buffer[index];	//Unless another conditional macro is found, this pointer will not advance
							ifptr = strcasestr_spec(&src_buffer[index], "%IF_");		//Search for the next instance of an if conditional macro
							elseptr = strcasestr_spec(&src_buffer[index], "%ELSE%");	//Search for the next instance of the else conditional macro
							endifptr = strcasestr_spec(&src_buffer[index], "%ENDIF%");	//Search for the next instance of the end of conditional test macro

							//Determine what conditional macro type (IF, ELSE, ENDIF) occurs next
							if(ifptr)
							{	//There is an IF macro after the current position
								if(elseptr)
								{	//There is an IF and an ELSE macro after the current position
									if(endifptr)
									{	//There is an IF, an ELSE and an ENDIF macro after the current position
										if(ifptr < elseptr)
										{	//The IF macro occurs before the ELSE macro
											if(ifptr < endifptr)
											{	//The IF macro occurs before the ENDIF macro
												nextcondition = 1;
											}
											else
											{	//The ENDIF macro occurs before the IF macro
												nextcondition = 3;
											}
										}
										else
										{	//The ELSE macro occurs before the IF macro
											if(elseptr < endifptr)
											{	//The ELSE macro occurs before the ENDIF macro
												nextcondition = 2;
											}
											else
											{	//The ENDIF macro occurs before the ELSE macro
												nextcondition = 3;
											}
										}
									}
									else
									{	//There is an IF and an ELSE macro, but no ENDIF macro, after the current position
										if(ifptr < elseptr)
										{	//The IF macro occurs before the ELSE macro
											nextcondition = 1;
										}
										else
										{	//The ELSE macro occurs before the IF macro
											nextcondition = 2;
										}
									}
								}//There is an IF and an ELSE macro after the current position
								else
								{	//There is no ELSE macro after the current position
									if(endifptr)
									{	//There is an IF and an ENDIF macro after the current position
										if(ifptr < endifptr)
										{	//The IF macro occurs before the ENDIF macro
											nextcondition = 1;
										}
										else
										{	//The ENDIF macro occurs before the IF macro
											nextcondition = 3;
										}
									}
									else
									{	//There is only an IF macro after the current position
										nextcondition = 1;
									}
								}
							}//There is an IF macro after the current position
							else
							{	//There is no IF macro after the current position
								if(elseptr)
								{	//There is an ELSE macro after the current position
									if(endifptr)
									{	//There is an ELSE and an ENDIF macro after the current position
										if(elseptr < endifptr)
										{	//The ELSE macro appears before the ENDIF macro
											nextcondition = 2;
										}
										else
										{	//The ENDIF macro appears before the ELSE macro
											nextcondition = 3;
										}
									}
									else
									{	//There is only an ELSE macro, after the current position
										nextcondition = 2;
									}
								}
								else
								{	//There is no ELSE macro after the current position
									if(endifptr)
									{	//There is only an ENDIF macro after the current position
										nextcondition = 3;
									}
								}
							}//There is no IF macro after the current position

							//Determine if the conditional branching is nesting a level deeper with an IF macro, negating with ELSE macro or ending with ENDIF macro
							if(nextcondition == 0)
							{	//There are no other conditional macros, the line will only run if the last conditional test evaluated as true
								if(macro_status == 2)
								{	//If the test evaluated as false
									return 1;	//The rest of the source buffer is to be skipped
								}
								if((macro_status == 3) && eraseptr)
								{	//If the test evaluated as true, and the corresponding ELSE macro was identified
									//Overwrite the ELSE condition (everything in the line beginning with this ELSE macro instance) with padding
									unsigned long padindex;

									for(padindex = 0; eraseptr[padindex] != '\0'; padindex++)
										eraseptr[padindex] = '%';	//Replace the rest of the line with padding characters
								}
								break;	//Exit macro parse while loop
							}
							else if(nextcondition == 1)
							{	//The next conditional macro is an IF macro
								nested_level++;	//The nesting level has increased
								nextptr = ifptr;	//Track the part of the buffer that the next loop iteration will process
							}
							else if(nextcondition == 2)
							{	//The next conditional macro is an ELSE macro
								if((macro_status == 2) && !nested_level)
								{	//If the test evaluated as false, and this is the ELSE macro at the same nesting level
									while(&src_buffer[src_index] != elseptr)	//Seek the main macro processing to this ELSE instance's portion of the line
										src_buffer++;
									break;	//Exit macro parse while loop
								}
								else if((macro_status == 3) && !nested_level)
								{	//If the test evaluated as true, and this is the ELSE macro at the same nesting level
									eraseptr = elseptr - 6;	//Track the pointer at which this ELSE macro begins, so the ELSE portion of the condition can be overwritten with padding if an ENDIF at the same nesting level is reached
								}
								nextptr = elseptr;	//Track the part of the buffer that the next loop iteration will process
							}
							else if(nextcondition == 3)
							{	//The next conditional macro is an ENDIF macro
								if((macro_status == 2) && !nested_level)
								{	//If the test evaluated as false, and this is the ENDIF macro at the same nesting level
									while(&src_buffer[src_index] != endifptr)	//Seek the main macro processing to this ENDIF instance's portion of the line
										src_buffer++;
									break;	//Exit macro parse while loop
								}
								else if((macro_status == 3) && !nested_level)
								{	//If the test evaluated as true, and this is the ENDIF macro at the same nesting level
									if(eraseptr)
									{	//If the corresponding ELSE macro was identified
										//OVERWRITE the ELSE condition (everything from the beginning of this ELSE macro instance to the beginning of the matching ENDIF instance) with padding
										unsigned long padindex;

										for(padindex = 0; &eraseptr[padindex] < endifptr - 7; padindex++)
											eraseptr[padindex] = '%';	//Replace all characters from the %ELSE% macro up until the beginning of this %ENDIF% macro instance with padding characters
										break;	//Exit macro parse while loop
									}
								}

								if(nested_level)
									nested_level--;	//The nesting level has decreased

								nextptr = endifptr;	//Track the part of the buffer that the next loop iteration will process
							}
							while(nextptr > &src_buffer[index])	//Seek the conditional macro processing to the next conditional macro
								index++;
						}//Process conditional macros until the correct branching is handled
						break;	//Exit macro parse while loop
					}//If this was a conditional macro
					else if(macro_status == 4)
					{	//An %EMPTY% macro was parsed
						allowempty = 1;
					}
					else if(macro_status == 1)
					{	//Otherwise if the macro was successfully converted to static text
						strncat(dest_buffer, expanded_macro, dest_buffer_size - strlen(dest_buffer) - 1);	//Append the converted macro text to the destination buffer
						dest_index = strlen(dest_buffer);	//Update the destination buffer index
						break;	//Exit macro parse while loop
					}
					else
					{	//Macro failed to convert
						snprintf(dest_buffer, dest_buffer_size, "Unrecognized macro \"%s\"", macro);
						return 1;
					}
				}//The closing percent sign of the macro was reached
				else
				{
					macro[macro_index++] = src_char;	//Append it to the macro buffer
				}
			}//Until the end of the source buffer is reached
		}//If the next character isn't also a percent sign (to be ignored as padding), parse the content up until the next percent sign and process it as a macro to be expanded
	}//Until the end of the source or destination buffer are reached

	dest_buffer[dest_index] = '\0';	//Terminate the destination buffer

	if(allowempty)	//If an %EMPTY% macro was parsed
		return 2;	//Signal to the calling function that an expanded line is allowed to print if it is empty
	return 1;
}

int eof_expand_notes_window_macro(char *macro, char *dest_buffer, unsigned long dest_buffer_size)
{
	unsigned long tracknum;

	if(!macro || !dest_buffer || (dest_buffer_size < 1))
		return 0;	//Invalid parameters
	if(!eof_song)
		return 0;	//No chart loaded

	tracknum = eof_song->track[eof_selected_track]->tracknum;

	///CONDITIONAL TEST MACROS
	//A beat marker is hovered over by the mouse
	if(!ustricmp(macro, "IF_HOVER_BEAT"))
	{
		if(eof_beat_num_valid(eof_song, eof_hover_beat))
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The selected beat marker has a time signature in effect
	if(!ustricmp(macro, "IF_SELECTED_BEAT_HAS_TS"))
	{
		if(eof_song->beat[eof_selected_beat]->has_ts)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The vocal track is active
	if(!ustricmp(macro, "IF_IS_VOCAL_TRACK"))
	{
		if(eof_vocals_selected)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The vocal track is not active
	if(!ustricmp(macro, "IF_IS_NOT_VOCAL_TRACK"))
	{
		if(!eof_vocals_selected)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The pro guitar track is active
	if(!ustricmp(macro, "IF_IS_PRO_GUITAR_TRACK"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The pro guitar track is not active
	if(!ustricmp(macro, "IF_IS_NOT_PRO_GUITAR_TRACK"))
	{
		if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//A note/lyric is selected
	if(!ustricmp(macro, "IF_NOTE_IS_SELECTED"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//A note is hovered over by the mouse
	if(!ustricmp(macro, "IF_HOVER_NOTE"))
	{
		if(eof_hover_note >= 0)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//Feedback input mode is in effect, and the seek position is at a note
	if(!ustricmp(macro, "IF_SEEK_HOVER_NOTE"))
	{
		if(eof_seek_hover_note >= 0)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//A note/lyric is selected and was manually named
	if(!ustricmp(macro, "IF_SELECTED_NOTE_IS_NAMED"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{	//If a note is selected
			char *name = eof_get_note_name(eof_song, eof_selected_track, eof_selection.current);

			if(name[0] != '\0')
			{	//If this note was manually given a name
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	//A pro guitar note is selected and its fret values can be written to a string
	if(!ustricmp(macro, "IF_CAN_DISPLAY_PRO_GUITAR_NOTE_FRETTING"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{	//If a note is selected
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
				char fret_string[30];

				if(eof_get_pro_guitar_note_fret_string(tp, eof_selection.current, fret_string))
				{	//If the note's frets can be represented in string format
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		return 2;	//False
	}

	//Resumes normal macro parsing after a failed conditional macro test
	if(!ustricmp(macro, "ENDIF"))
	{
		dest_buffer[0] = '\0';
		return 1;
	}


	///CONTROL MACROS
	//Allow a blank line to be printer
	if(!ustricmp(macro, "EMPTY"))
	{
		return 4;
	}

	///EXPANSION MACROS
	//Percent sign
	if(!ustricmp(macro, "PERCENT"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%%");
		return 1;
	}

	//Track name
	if(!ustricmp(macro, "TRACK_NAME"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_song->track[eof_selected_track]->name);
		return 1;
	}

	//Track alternate name
	if(!ustricmp(macro, "TRACK_ALT_NAME"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_song->track[eof_selected_track]->altname);
		return 1;
	}

	//Track difficulty level
	if(!ustricmp(macro, "TRACK_DIFFICULTY"))
	{
		if(eof_song->track[eof_selected_track]->difficulty == 0xFF)
			snprintf(dest_buffer, dest_buffer_size, "(Undefined)");
		else
		snprintf(dest_buffer, dest_buffer_size, "%u", eof_song->track[eof_selected_track]->difficulty);
		return 1;
	}

	//Track secondary difficulty level (pro drums, harmonies)
	if(!ustricmp(macro, "TRACK_SECONDARY_DIFFICULTY"))
	{
		if(eof_selected_track == EOF_TRACK_DRUM)
		{	//Write the difficulty string to display for pro drums
			if(((eof_song->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24) != 0x0F)
			{	//If the pro drum difficulty is defined
				(void) snprintf(dest_buffer, dest_buffer_size, "(Pro: %lu)", (eof_song->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24);	//Mask out the low nibble of the high order byte of the drum track's flags (pro drum difficulty)
			}
			else
			{
				(void) snprintf(dest_buffer, dest_buffer_size, "(Pro: Undefined)");
			}
		}
		else if(eof_selected_track == EOF_TRACK_VOCALS)
		{	//Write the difficulty string to display for vocal harmony
			if(((eof_song->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24) != 0x0F)
			{	//If the harmony difficulty is defined
				(void) snprintf(dest_buffer, dest_buffer_size, "(Harmony: %lu)", (eof_song->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24);	//Mask out the high order byte of the vocal track's flags (harmony difficulty)
			}
			else
			{
				(void) snprintf(dest_buffer, dest_buffer_size, "(Harmony: Undefined)");
			}
		}
		else
		{	//There is no secondary difficulty for the active track
			dest_buffer[0] = '\0';	//Empty the output buffer
		}
		return 1;
	}

	//Metronome status
	if(!ustricmp(macro, "METRONOME_STATUS"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_mix_metronome_enabled ? "On" : "Off");
		return 1;
	}

	//Claps status
	if(!ustricmp(macro, "CLAPS_STATUS"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_mix_claps_enabled ? "On" : "Off");
		return 1;
	}

	//Vocal tones status
	if(!ustricmp(macro, "VOCAL_TONES_STATUS"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_mix_vocal_tones_enabled ? "On" : "Off");
		return 1;
	}

	//MIDI tones status
	if(!ustricmp(macro, "MIDI_TONES_STATUS"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_mix_midi_tones_enabled ? "On" : "Off");
		return 1;
	}

	//Selected beat
	if(!ustricmp(macro, "SELECTED_BEAT"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%lu", eof_selected_beat);
		return 1;
	}

	//Selected beat tempo
	if(!ustricmp(macro, "SELECTED_BEAT_TEMPO"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%f", 60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn);
		return 1;
	}

	//Hover beat
	if(!ustricmp(macro, "HOVER_BEAT"))
	{
		if(eof_beat_num_valid(eof_song, eof_hover_beat))
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_hover_beat);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Key signature
	if(!ustricmp(macro, "SELECTED_BEAT_KEY_SIGNATURE"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s maj (%s min)", eof_get_key_signature(eof_song, eof_selected_beat, 1, 0), eof_get_key_signature(eof_song, eof_selected_beat, 1, 1));
		return 1;
	}

	//Selected beat position
	if(!ustricmp(macro, "SELECTED_BEAT_POS"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%f", eof_song->beat[eof_selected_beat]->fpos);
		return 1;
	}

	//Selected beat's measure number
	if(!ustricmp(macro, "SELECTED_BEAT_MEASURE"))
	{
		if(eof_song->beat[eof_selected_beat]->has_ts)
			snprintf(dest_buffer, dest_buffer_size, "%ld", eof_selected_measure);
		else
			snprintf(dest_buffer, dest_buffer_size, "(TS undefined)");

		return 1;
	}

	//Selected beat's position in measure
	if(!ustricmp(macro, "BEAT_POSITION_IN_MEASURE"))
	{
		if(eof_song->beat[eof_selected_beat]->has_ts)
			snprintf(dest_buffer, dest_buffer_size, "(Beat %d/%d)", eof_beat_in_measure + 1, eof_beats_in_measure);
		else
			snprintf(dest_buffer, dest_buffer_size, "(TS undefined)");
		return 1;
	}

	//Selected note/lyric
	if(!ustricmp(macro, "SELECTED_NOTE"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_selection.current);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric position
	if(!ustricmp(macro, "SELECTED_NOTE_POS"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current));
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric position
	if(!ustricmp(macro, "SELECTED_NOTE_LENGTH"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_get_note_length(eof_song, eof_selected_track, eof_selection.current));
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric name/text
	if(!ustricmp(macro, "SELECTED_NOTE_NAME"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
			snprintf(dest_buffer, dest_buffer_size, "%s", eof_get_note_name(eof_song, eof_selected_track, eof_selection.current));
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric value/tone
	if(!ustricmp(macro, "SELECTED_NOTE_VALUE"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
			snprintf(dest_buffer, dest_buffer_size, "%d", eof_get_note_note(eof_song, eof_selected_track, eof_selection.current));
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric value/tone
	if(!ustricmp(macro, "SELECTED_LYRIC_TONE_NAME"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{
			unsigned char tone = eof_get_note_note(eof_song, eof_selected_track, eof_selection.current);

			if(!tone)
				snprintf(dest_buffer, dest_buffer_size, "None");
			else
				snprintf(dest_buffer, dest_buffer_size, "%s", eof_get_tone_name(tone));
		}
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Hover note
	if(!ustricmp(macro, "HOVER_NOTE"))
	{
		if(eof_hover_note >= 0)
			snprintf(dest_buffer, dest_buffer_size, "%d", eof_hover_note);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Seek hover note (the note at which the seek position is if Feedback input mode is in effect)
	if(!ustricmp(macro, "SEEK_HOVER_NOTE"))
	{
		if(eof_seek_hover_note >= 0)
			snprintf(dest_buffer, dest_buffer_size, "%d", eof_seek_hover_note);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Pro guitar note fretting
	if(!ustricmp(macro, "PRO_GUITAR_NOTE_FRETTING"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{	//If a note is selected
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
				char fret_string[30];

				if(eof_get_pro_guitar_note_fret_string(tp, eof_selection.current, fret_string))
				{	//If the note's frets can be represented in string format
					snprintf(dest_buffer, dest_buffer_size, "%s", fret_string);
					return 1;
				}
			}
		}
		snprintf(dest_buffer, dest_buffer_size, "(Error)");

		return 1;
	}

	return 0;	//Macro not supported
}
