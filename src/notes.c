#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif
#include "beat.h"		//For eof_get_measure()
#include "main.h"
#include "mix.h"
#include "notes.h"
#include "rs.h"			//For eof_pro_guitar_track_find_effective_fret_hand_position()
#include "song.h"
#include "tuning.h"
#include "utility.h"	//For eof_char_is_hex() and eof_string_is_hexadecimal()
#include "foflc/Lyric_storage.h"
#include "menu/track.h"	//For eof_menu_pro_guitar_track_get_tech_view_state()

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

EOF_TEXT_PANEL *eof_create_text_panel(char *filename)
{
	EOF_TEXT_PANEL *panel;

	if(!filename)
		return NULL;	//Invalid parameter

	panel = malloc(sizeof(EOF_TEXT_PANEL));
	if(!panel)
		return NULL;	//Couldn't allocate memory

	(void) ustrncpy(panel->filename, filename, sizeof(panel->filename) - 1);	//Copy the input file name
	panel->text = eof_buffer_file(filename, 1);	//Buffer the specified file into memory, appending a NULL terminator
	if(panel->text == NULL)
	{	//Could not buffer file
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error loading:  Cannot open input text file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
///		allegro_message(eof_log_string);
		free(panel);
		return NULL;
	}

	return panel;
}

void eof_destroy_text_panel(EOF_TEXT_PANEL *panel)
{
	if(panel)
	{
		if(panel->text)
			free(panel->text);
		free(panel);
	}
}

int eof_expand_notes_window_text(char *src_buffer, char *dest_buffer, unsigned long dest_buffer_size, EOF_TEXT_PANEL *panel)
{
	unsigned long src_index = 0, dest_index = 0, macro_index;
	char src_char, macro[2048], expanded_macro[2048];
	int macro_status;

	if(!src_buffer || !dest_buffer || (dest_buffer_size < 1) || !panel)
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
					macro_status = eof_expand_notes_window_macro(macro, expanded_macro, 2048, panel);	//Convert the macro to static text

					if(macro_status == 1)
					{	//Otherwise if the macro was successfully converted to static text
						strncat(dest_buffer, expanded_macro, dest_buffer_size - strlen(dest_buffer) - 1);	//Append the converted macro text to the destination buffer
						dest_index = strlen(dest_buffer);	//Update the destination buffer index

						if(panel->flush)
						{	//If the %FLUSH% macro was just parsed, print the current contents of the output buffer and update the print coordinates
							textout_ex(panel->window->screen, font, dest_buffer, panel->xpos, panel->ypos, panel->color, panel->bgcolor);	//Print this line to the screen
							panel->xpos += text_length(font, dest_buffer);	//Increase the print x coordinate by the number of pixels the output buffer took to render
							panel->flush = 0;	//Reset this condition
							if(dest_buffer[0] != '\0')
							{	//If the output buffer wasn't empty
								panel->contentprinted = 1;	//Track this in case no other content for the line is printed
							}
							if(panel->symbol)
							{	//If a symbol font character is to be printed
								dest_buffer[0] = panel->symbol;
								dest_buffer[1] = '\0';	//Build a string containing just that character
								textout_ex(panel->window->screen, eof_symbol_font, dest_buffer, panel->xpos, panel->ypos, panel->color, panel->bgcolor);	//Print this symbol glyph to the screen
								panel->xpos += text_length(eof_symbol_font, dest_buffer);	//Increase the print x coordinate by the number of pixels the glyph took to render
								panel->symbol = 0;
								panel->contentprinted = 1;
							}
							dest_buffer[0] = '\0';	//Empty the destination buffer
							dest_index = 0;
						}
						break;	//Exit macro parse while loop
					}
					else if((macro_status == 2) || (macro_status == 3))
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

	return 1;
}

int eof_expand_notes_window_macro(char *macro, char *dest_buffer, unsigned long dest_buffer_size, EOF_TEXT_PANEL *panel)
{
	unsigned long tracknum;

	if(!macro || !dest_buffer || (dest_buffer_size < 1) || !panel)
		return 0;	//Invalid parameters
	if(!eof_song)
		return 0;	//No chart loaded

	tracknum = eof_song->track[eof_selected_track]->tracknum;

	///CONDITIONAL TEST MACROS
	///The conditional macro handling requires that all conditional macros other than %ELSE% and %ENDIF% begin with %IF_
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

	//A pro guitar track is active
	if(!ustricmp(macro, "IF_IS_PRO_GUITAR_TRACK"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//One of the drum tracks is active
	if(!ustricmp(macro, "IF_IS_DRUM_TRACK"))
	{
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//A pro guitar track is active and tech view is in effect
	if(!ustricmp(macro, "IF_IS_TECH_VIEW"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];

			if(eof_menu_pro_guitar_track_get_tech_view_state(tp))
			{	//If tech view is in effect
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	//Tech view is not in effect, regardless of which track is active
	if(!ustricmp(macro, "IF_IS_NOT_TECH_VIEW"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];

			if(eof_menu_pro_guitar_track_get_tech_view_state(tp))
			{	//If tech view is in effect
				dest_buffer[0] = '\0';
				return 2;	//False
			}
		}

		return 3;	//True
	}

	//A pro guitar track is not active
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

	//A pro guitar chord is selected and its has at least one chord name lookup match
	if(!ustricmp(macro, "IF_CAN_LOOKUP_SELECTED_CHORD_NAME"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{	//If a note is selected
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];

				if(eof_count_chord_lookup_matches(tp, eof_selected_track, eof_selection.current))
				{	//If there's at least one chord lookup match
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		return 2;	//False
	}

	//If this track's tuning is not being reflected in chord name lookups
	if(!ustricmp(macro, "IF_TUNING_IGNORED"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
			if(tp->ignore_tuning)
			{
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	//If sound effects are disabled in preferences
	if(!ustricmp(macro, "IF_SOUND_CUES_DISABLED"))
	{
		if(eof_disable_sound_processing)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the start point is defined
	if(!ustricmp(macro, "IF_START_POINT_DEFINED"))
	{
		if(eof_song->tags->start_point != ULONG_MAX)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the end point is defined
	if(!ustricmp(macro, "IF_END_POINT_DEFINED"))
	{
		if(eof_song->tags->end_point != ULONG_MAX)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the selected fret catalog entry is named
	if(!ustricmp(macro, "IF_SELECTED_CATALOG_ENTRY_NAMED"))
	{
		if((eof_selected_catalog_entry < eof_song->catalog->entries) && (eof_song->catalog->entry[eof_selected_catalog_entry].name[0] != '\0'))
		{	//If the active fret catalog has a defined name
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the program window height is 480
	if(!ustricmp(macro, "IF_WINDOW_HEIGHT_IS_480"))
	{
		if(eof_screen_height == 480)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the program window height is 600
	if(!ustricmp(macro, "IF_WINDOW_HEIGHT_IS_600"))
	{
		if(eof_screen_height == 600)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the program window height is 768
	if(!ustricmp(macro, "IF_WINDOW_HEIGHT_IS_768"))
	{
		if(eof_screen_height == 768)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
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
	//Allow a blank line to be printed
	if(!ustricmp(macro, "EMPTY"))
	{
		dest_buffer[0] = '\0';
		panel->allowempty = 1;	//Signal this to the calling function
		return 1;
	}

	//Move the text output up one pixel
	if(!ustricmp(macro, "MOVE_UP_ONE_PIXEL"))
	{
		dest_buffer[0] = '\0';
		if(panel->ypos)
			panel->ypos--;	//Update the y coordinate for Notes panel printing
		return 1;
	}

	//Move the text output down one pixel
	if(!ustricmp(macro, "MOVE_DOWN_ONE_PIXEL"))
	{
		dest_buffer[0] = '\0';
		panel->ypos++;	//Update the y coordinate for Notes panel printing
		return 1;
	}

	//Print the current content of the destination buffer, then resume parsing macros in that line
	if(!ustricmp(macro, "FLUSH"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		return 1;
	}

	//Change the printed text color
	if(strcasestr_spec(macro, "TEXT_COLOR_"))
	{
		int newcolor;
		char *color_string = strcasestr_spec(macro, "TEXT_COLOR_");	//Get a pointer to the text that is expected to be the color name

		if(eof_read_macro_color(color_string, &newcolor))
		{	//If the color was successfully parsed
			dest_buffer[0] = '\0';
			panel->color = newcolor;
			return 1;
		}
	}

	//Clear the printed text background color
	if(strcasestr_spec(macro, "TEXT_BACKGROUND_COLOR_NONE"))
	{
		dest_buffer[0] = '\0';
		panel->bgcolor = -1;
		return 1;
	}

	//Change the printed text background color
	if(strcasestr_spec(macro, "TEXT_BACKGROUND_COLOR_"))
	{
		int newcolor;
		char *color_string = strcasestr_spec(macro, "TEXT_BACKGROUND_COLOR_");	//Get a pointer to the text that is expected to be the color name

		if(eof_read_macro_color(color_string, &newcolor))
		{	//If the color was successfully parsed
			dest_buffer[0] = '\0';
			panel->bgcolor = newcolor;
			return 1;
		}
	}


	///SYMBOL MACROS
	//Bend character
	if(!ustricmp(macro, "BEND"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'a';	//In the symbols font, a is the bend character
		return 1;
	}

	//Harmonic character
	if(!ustricmp(macro, "HARMONIC"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'b';	//In the symbols font, b is the harmonic character
		return 1;
	}

	//Down strum character
	if(!ustricmp(macro, "DOWNSTRUM"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'c';	//In the symbols font, c is the down strum character
		return 1;
	}

	//Up strum character
	if(!ustricmp(macro, "UPSTRUM"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'd';	//In the symbols font, d is the up strum character
		return 1;
	}

	//Tremolo character
	if(!ustricmp(macro, "TREMOLO"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'e';	//In the symbols font, e is the tremolo character
		return 1;
	}

	//Pinch harmonic character
	if(!ustricmp(macro, "PHARMONIC"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'f';	//In the symbols font, f is the pinch harmonic character
		return 1;
	}

	//Linknext character
	if(!ustricmp(macro, "LINKNEXT"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'g';	//In the symbols font, f is the linknext indicator
		return 1;
	}

	//Unpitched slide up character
	if(!ustricmp(macro, "USLIDEUP"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'i';	//In the symbols font, i is the unpitched slide up indicator
		return 1;
	}

	//Unpitched slide down character
	if(!ustricmp(macro, "USLIDEDOWN"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'j';	//In the symbols font, j is the unpitched slide down indicator
		return 1;
	}

	//Stop character
	if(!ustricmp(macro, "STOP"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'k';	//In the symbols font, k is the stop status indicator
		return 1;
	}

	//Pre-bend character
	if(!ustricmp(macro, "PREBEND"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->symbol = 'l';	//In the symbols font, l is the pre-bend character
		return 1;
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
		if((eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_ALT_NAME) && (eof_song->track[eof_selected_track]->altname[0] != '\0'))
			snprintf(dest_buffer, dest_buffer_size, "%s", eof_song->track[eof_selected_track]->altname);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
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
			snprintf(dest_buffer, dest_buffer_size, "%ld", eof_get_note_length(eof_song, eof_selected_track, eof_selection.current));
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
		if(eof_vocals_selected && (eof_selection.current < eof_get_track_size(eof_song, eof_selected_track)))
		{
			unsigned char tone = eof_get_note_note(eof_song, eof_selected_track, eof_selection.current);

			if(tone)
			{
				snprintf(dest_buffer, dest_buffer_size, "%s", eof_get_tone_name(tone));
				return 1;
			}
		}
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

	//Selected pro guitar note fretting
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
				else
				{
					snprintf(dest_buffer, dest_buffer_size, "(Error)");
					return 1;
				}
			}
		}
		snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Pro guitar selected chord name lookup
	if(!ustricmp(macro, "SELECTED_CHORD_NAME_LOOKUP"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{	//If a note is selected
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
				unsigned long matchcount;
				char chord_match_string[30] = {0};
				int scale = 0, chord = 0, isslash = 0, bassnote = 0;

				matchcount = eof_count_chord_lookup_matches(tp, eof_selected_track, eof_selection.current);
				if(matchcount)
				{	//If there's at least one chord lookup match, obtain the user's selected match
					eof_lookup_chord(tp, eof_selected_track, eof_selection.current, &scale, &chord, &isslash, &bassnote, eof_selected_chord_lookup, 1);	//Run a cache-able lookup
					scale %= 12;	//Ensure this is a value from 0 to 11
					bassnote %= 12;
					if(matchcount > 1)
					{	//If there's more than one match
						(void) snprintf(chord_match_string, sizeof(chord_match_string) - 1, " (match %lu/%lu)", eof_selected_chord_lookup + 1, matchcount);
					}
					if(!isslash)
					{	//If it's a normal chord
						snprintf(dest_buffer, dest_buffer_size, "[%s%s]%s", eof_note_names[scale], eof_chord_names[chord].chordname, chord_match_string);
					}
					else
					{	//If it's a slash chord
						snprintf(dest_buffer, dest_buffer_size, "[%s%s%s]%s", eof_note_names[scale], eof_chord_names[chord].chordname, eof_slash_note_names[bassnote], chord_match_string);
					}

					return 1;
				}
			}
		}
		snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected pro guitar note fingering
	if(!ustricmp(macro, "PRO_GUITAR_NOTE_FINGERING"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{	//If a note is selected
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
				char finger_string[30] = {0};

				if(eof_get_note_eflags(eof_song, eof_selected_track, eof_selection.current) & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
				{	//If this note has fingerless status
					snprintf(dest_buffer, dest_buffer_size, "None");
				}
				else if(eof_get_pro_guitar_note_finger_string(tp, eof_selection.current, finger_string))
				{	//If the note's fingering can be represented in string format
					snprintf(dest_buffer, dest_buffer_size, "%s", finger_string);
				}
				else
				{
					snprintf(dest_buffer, dest_buffer_size, "(Error)");
				}

				return 1;
			}
		}
		snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected pro guitar note tones
	if(!ustricmp(macro, "PRO_GUITAR_NOTE_TONES"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
		{	//If a note is selected
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
				char tone_string[30] = {0};

				if(eof_get_pro_guitar_note_tone_string(tp, eof_selection.current, tone_string))
				{	//If the note's tones can be represented in string format
					snprintf(dest_buffer, dest_buffer_size, "%s", tone_string);
				}
				else
				{
					snprintf(dest_buffer, dest_buffer_size, "(Error)");
				}

				return 1;
			}
		}
		snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//The fret hand position in effect at the pro guitar track's current seek position
	if(!ustricmp(macro, "PRO_GUITAR_TRACK_EFFECTIVE_FHP"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
			unsigned char position;

			position = eof_pro_guitar_track_find_effective_fret_hand_position(tp, eof_note_type, eof_music_pos - eof_av_delay);	//Find if there's a fret hand position in effect
			if(position)
			{	//If a fret hand position is in effect
				snprintf(dest_buffer, dest_buffer_size, "%u", position);
				return 1;
			}
		}
		snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//The tone in effect at the pro guitar track's current seek position
	if(!ustricmp(macro, "PRO_GUITAR_TRACK_EFFECTIVE_TONE"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
			unsigned long tone;

			tone = eof_pro_guitar_track_find_effective_tone(tp, eof_music_pos - eof_av_delay);	//Find if there's a tone change in effect
			if(tone < EOF_MAX_PHRASES)
			{	//If a tone change is in effect
				snprintf(dest_buffer, dest_buffer_size, "%s", tp->tonechange[tone].name);
				return 1;
			}
			else
			{
				if(tp->defaulttone[0] != '\0')
				{	//If a default tone is defined for the track
					snprintf(dest_buffer, dest_buffer_size, "(%s)", tp->defaulttone);
					return 1;
				}
			}
		}
		snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Seek position (honoring the seek timing format specified in user preferences)
	if(!ustricmp(macro, "SEEK_POSITION"))
	{
		int min, sec, ms;

		ms = (eof_music_pos - eof_av_delay) % 1000;
		if(!eof_display_seek_pos_in_seconds)
		{	//If the seek position is to be displayed as minutes:seconds
			min = ((eof_music_pos - eof_av_delay) / 1000) / 60;
			sec = ((eof_music_pos - eof_av_delay) / 1000) % 60;
			snprintf(dest_buffer, dest_buffer_size, "%02d:%02d.%03d", min, sec, ms);
		}
		else
		{	//If the seek position is to be displayed as seconds
			sec = (eof_music_pos - eof_av_delay) / 1000;
			snprintf(dest_buffer, dest_buffer_size, "%d.%03ds", sec, ms);
		}
		return 1;
	}

	//Seek position in seconds.milliseconds format
	if(!ustricmp(macro, "SEEK_POSITION_SEC"))
	{
		int sec, ms;

		sec = (eof_music_pos - eof_av_delay) / 1000;
		ms = (eof_music_pos - eof_av_delay) % 1000;
		snprintf(dest_buffer, dest_buffer_size, "%d.%03ds", sec, ms);

		return 1;
	}

	//Seek position in minutes:seconds.milliseconds format
	if(!ustricmp(macro, "SEEK_POSITION_MIN_SEC"))
	{
		int min, sec, ms;

		min = ((eof_music_pos - eof_av_delay) / 1000) / 60;
		sec = ((eof_music_pos - eof_av_delay) / 1000) % 60;
		ms = (eof_music_pos - eof_av_delay) % 1000;
		snprintf(dest_buffer, dest_buffer_size, "%02d:%02d.%03d", min, sec, ms);

		return 1;
	}

	//Number of notes selected
	if(!ustricmp(macro, "COUNT_NOTES_SELECTED"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%lu", eof_count_selected_notes(NULL));	//Count the number of selected notes, don't track the count of notes in the active track difficulty

		return 1;
	}

	//Number of notes selected
	if(!ustricmp(macro, "TRACK_DIFF_NOTE_COUNT"))
	{
		unsigned long count = 0;

		(void) eof_count_selected_notes(&count);	//Count the number of notes in the active track difficulty
		snprintf(dest_buffer, dest_buffer_size, "%lu", count);	//Count the number of selected notes, don't track the count of notes in the active track difficulty
		return 1;
	}

	//The defined start point
	if(!ustricmp(macro, "START_POINT"))
	{
		if(eof_song->tags->start_point != ULONG_MAX)
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_song->tags->start_point);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//The defined start point
	if(!ustricmp(macro, "END_POINT"))
	{
		if(eof_song->tags->end_point != ULONG_MAX)
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_song->tags->end_point);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//The name of the current input mode
	if(!ustricmp(macro, "INPUT_MODE_NAME"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_input_name[eof_input_mode]);

		return 1;
	}

	//The current playback speed
	if(!ustricmp(macro, "PLAYBACK_SPEED"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%d", eof_playback_speed / 10);

		return 1;
	}

	//The current grid snap setting
	if(!ustricmp(macro, "GRID_SNAP_SETTING"))
	{
		if(eof_snap_mode != EOF_SNAP_CUSTOM)
			snprintf(dest_buffer, dest_buffer_size, "%s", eof_snap_name[(int)eof_snap_mode]);
		else
		{
			if(eof_custom_snap_measure == 0)
				snprintf(dest_buffer, dest_buffer_size, "%s (1/%d beat)", eof_snap_name[(int)eof_snap_mode], eof_snap_interval);
			else
				snprintf(dest_buffer, dest_buffer_size, "%s (1/%d measure)", eof_snap_name[(int)eof_snap_mode], eof_snap_interval);
		}

		return 1;
	}

	//The selected fret catalog entry
	if(!ustricmp(macro, "SELECTED_CATALOG_ENTRY"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%lu of %lu", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries);
		return 1;
	}

	//The selected fret catalog entry's named
	if(!ustricmp(macro, "SELECTED_CATALOG_ENTRY_NAME"))
	{
		if((eof_selected_catalog_entry < eof_song->catalog->entries) && (eof_song->catalog->entry[eof_selected_catalog_entry].name[0] != '\0'))
		{	//If the active fret catalog has a defined name
			snprintf(dest_buffer, dest_buffer_size, "%s", eof_song->catalog->entry[eof_selected_catalog_entry].name);
			return 1;
		}

		snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//The file name of the loaded chart audio
	if(!ustricmp(macro, "LOADED_OGG_NAME"))
	{
		if(!eof_silence_loaded)
			snprintf(dest_buffer, dest_buffer_size, "%s", eof_song->tags->ogg[eof_selected_ogg].filename);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");

		return 1;
	}

	//The strings that are currently affected by fret value shortcuts
	if(!ustricmp(macro, "FRET_VALUE_SHORTCUTS_SETTING"))
	{
		char shortcut_string[55] = {0};

		if(eof_get_pro_guitar_fret_shortcuts_string(shortcut_string))
		{	//If the note's fingering can be represented in string format
			snprintf(dest_buffer, dest_buffer_size, "%s", shortcut_string);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "(Error)");
		}

		return 1;
	}

	//The status of modifier keys and the last keypress's scan and ASCII codes
	if(!ustricmp(macro, "KEY_INPUT_STATUS"))
	{
		snprintf(dest_buffer, dest_buffer_size, "CTRL:%c ALT:%c SHIFT:%c CODE:%d ASCII:%d ('%c')", KEY_EITHER_CTRL ? '*' : ' ', KEY_EITHER_ALT ? '*' : ' ', KEY_EITHER_SHIFT ? '*' : ' ', eof_last_key_code, eof_last_key_char, eof_last_key_char);
		return 1;
	}

	//The number of beats in the project
	if(!ustricmp(macro, "BEAT_COUNT"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%lu", eof_song->beats - 1);
		return 1;
	}

	//The number of beats in the project
	if(!ustricmp(macro, "MEASURE_COUNT"))
	{
		unsigned long measurecount = eof_get_measure(0, 1);	//Count the number of measures

		if(measurecount)
			snprintf(dest_buffer, dest_buffer_size, "%lu", measurecount);
		else
			snprintf(dest_buffer, dest_buffer_size, "(No TS)");

		return 1;
	}

	//The current minimum note distance setting
	if(!ustricmp(macro, "NOTE_GAP"))
	{
		if(!eof_min_note_distance_intervals)
			snprintf(dest_buffer, dest_buffer_size, "%u ms", eof_min_note_distance);
		else if(eof_min_note_distance_intervals == 1)
			snprintf(dest_buffer, dest_buffer_size, "1/%u measure", eof_min_note_distance);
		else
			snprintf(dest_buffer, dest_buffer_size, "1/%u beat", eof_min_note_distance);
		return 1;
	}

	//The current program window height
	if(!ustricmp(macro, "WINDOW_HEIGHT"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%lu", eof_screen_height);
		return 1;
	}

	//The current program window width
	if(!ustricmp(macro, "WINDOW_WIDTH"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%lu", eof_screen_width);
		return 1;
	}

	//The relative file name of the panel's text file
	if(!ustricmp(macro, "TEXT_FILE"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", get_filename(panel->filename));
		return 1;
	}

	return 0;	//Macro not supported
}

int eof_read_macro_color(char *string, int *color)
{
	if(!string || !color)
		return 0;	//Invalid parameters

	if(!ustricmp(string, "BLACK"))
		*color = eof_color_black;
	else if(!ustricmp(string, "WHITE"))
		*color = eof_color_white;
	else if(!ustricmp(string, "GRAY"))
		*color = eof_color_gray2;
	else if(!ustricmp(string, "RED"))
		*color = eof_color_red;
	else if(!ustricmp(string, "GREEN"))
		*color = eof_color_green;
	else if(!ustricmp(string, "BLUE"))
		*color = eof_color_blue;
	else if(!ustricmp(string, "TURQUOISE"))
		*color = eof_color_turquoise;
	else if(!ustricmp(string, "YELLOW"))
		*color = eof_color_yellow;
	else if(!ustricmp(string, "PURPLE"))
		*color = eof_color_purple;
	else if(!ustricmp(string, "ORANGE"))
		*color = eof_color_orange;
	else if(!ustricmp(string, "SILVER"))
		*color = eof_color_silver;
	else if(!ustricmp(string, "CYAN"))
		*color = eof_color_cyan;
	else if((ustrlen(string) == 6) && eof_string_is_hexadecimal(string))
	{	//If this is a 6 digit hexadecimal string
		int r, g, b;
		char hex[3] = {0};

		hex[0] = string[0];
		hex[1] = string[1];
		r = (int) strtol(hex, NULL, 16);	//Convert these two hex characters into decimal
		hex[0] = string[2];
		hex[1] = string[3];
		g = (int) strtol(hex, NULL, 16);	//Convert these two hex characters into decimal
		hex[0] = string[4];
		hex[1] = string[5];
		b = (int) strtol(hex, NULL, 16);	//Convert these two hex characters into decimal
		*color = makecol(r, g, b);
	}
	else
		return 0;	//Not a valid color

	return 1;
}

void eof_render_text_panel(EOF_TEXT_PANEL *panel, int opaque)
{
	char buffer[2048], buffer2[2048];
	unsigned long src_index, dst_index;
	int retval;

	if(!eof_song_loaded || !eof_song)	//If a project isn't loaded
		return;			//Return immediately
	if(!panel || !panel->window || !panel->text)	//If the provided panel isn't valid
		return;			//Invalid parameter

	if(opaque)
		clear_to_color(panel->window->screen, eof_color_gray);

	//Initialize the panel array
	panel->ypos = 0;
	panel->xpos = 2;
	panel->color = eof_color_white;
	panel->bgcolor = -1;	//Transparent background for text
	panel->allowempty = 0;
	panel->contentprinted = 0;
	panel->symbol = 0;

	//Parse the contents of the buffered file one line at a time and print each to the screen
	src_index = dst_index = 0;	//Reset these indexes
	while(panel->text[src_index] != '\0')
	{	//Until the end of the text file is reached
		char thischar = panel->text[src_index++];	//Read one character from the source buffer

		if((thischar == '\r') && (panel->text[src_index] == '\n'))
		{	//Carriage return and line feed characters represent a new line
			buffer[dst_index] = '\0';	//NULL terminate the buffer
			retval = eof_expand_notes_window_text(buffer, buffer2, 2048, panel);
			if(!retval)
			{	//If the buffer's content was not successfully parsed to expand macros, disable the notes panel
				eof_enable_notes_panel = 0;
				return;
			}
			if(panel->allowempty || panel->contentprinted || (buffer2[0] != '\0'))
			{	//If the printing of an empty line was allowed by the %EMPTY% macro or this line isn't empty
				//If content was printed earlier in the line and flushed to the Notes panel, allow the coordinates to reset to the next line
				textout_ex(panel->window->screen, font, buffer2, panel->xpos, panel->ypos, panel->color, panel->bgcolor);	//Print this line to the screen
				panel->allowempty = 0;	//Reset this condition, it has to be enabled per-line
				panel->xpos = 2;			//Reset the x coordinate to the beginning of the line
				panel->ypos +=12;
				panel->contentprinted = 0;
			}
			dst_index = 0;	//Reset the destination buffer index
			src_index++;	//Seek past the line feed character in the source buffer
		}
		else
		{
			if(dst_index >= 1023)
				return;	//Don't support lines longer than 1023 characters, plus one character for the NULL terminator
			buffer[dst_index++] = thischar;	//Append the character to the destination buffer
		}
	}

	//Print any remaining content in the output buffer
	if(dst_index && (dst_index < 1023))
	{	//If there are any characters in the destination buffer, and there is room in the buffer for the NULL terminator
		buffer[dst_index] = '\0';	//NULL terminate the buffer
		retval = eof_expand_notes_window_text(buffer, buffer2, 2048, panel);
		if(!retval)
		{	//If the buffer's content was not successfully parsed to expand macros, disable the notes panel
			eof_enable_notes_panel = 0;
			return;
		}
		if((retval == 2) || (buffer2[0] != '\0'))
		{	//If the printing of an empty line was allowed by the %EMPTY% macro or this line isn't empty
			textout_ex(panel->window->screen, font, buffer2, panel->xpos, panel->ypos, panel->color, panel->bgcolor);	//Print this line to the screen
			panel->ypos +=12;
		}
	}

	//Draw a border around the edge of the notes panel
	if(opaque)
	{	//But only if this panel wasn't meant to obscure whatever it's drawn on top of
		rect(panel->window->screen, 0, 0, panel->window->w - 1, panel->window->h - 1, eof_color_dark_silver);
		rect(panel->window->screen, 1, 1, panel->window->w - 2, panel->window->h - 2, eof_color_black);
		hline(panel->window->screen, 1, panel->window->h - 2, panel->window->w - 2, eof_color_white);
		vline(panel->window->screen, panel->window->w - 2, 1, panel->window->h - 2, eof_color_white);
	}
}
