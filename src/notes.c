#include <allegro.h>
#include <ctype.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif
#include "beat.h"		//For eof_get_measure()
#include "ini_import.h"
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

#define TEXT_PANEL_BUFFER_SIZE 2047

EOF_TEXT_PANEL *eof_create_text_panel(char *filename, int builtin)
{
	EOF_TEXT_PANEL *panel;
	char recoverypath[1024] = {0};

	if(!filename)
		return NULL;	//Invalid parameter

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Creating text panel from file \"%s\"", filename);
	eof_log(eof_log_string, 2);
	if(!exists(filename))
	{	//If the file doesn't exist
		if(builtin)
		{	//If the calling function wanted to recover the file from eof.dat in this scenario
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tRecreating panel file (%s) from eof.dat", filename);
			eof_log(eof_log_string, 1);
			(void) snprintf(recoverypath, sizeof(recoverypath) - 1, "eof.dat#%s", filename);	//The path into eof.dat from which the file should be recovered
			if(!eof_copy_file(recoverypath, filename) || !exists(filename))
			{	//If the file couldn't be recovered from eof.dat
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCould not recreate panel file (%s) from eof.dat", filename);
				eof_log(eof_log_string, 1);

				return NULL;
			}
		}
	}
	panel = malloc(sizeof(EOF_TEXT_PANEL));
	if(!panel)
		return NULL;	//Couldn't allocate memory

	(void) ustrncpy(panel->filename, filename, sizeof(panel->filename) - 1);	//Copy the input file name
	panel->text = eof_buffer_file(filename, 1);	//Buffer the specified file into memory, appending a NULL terminator
	if(panel->text == NULL)
	{	//Could not buffer file
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error loading:  Cannot open input text file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		free(panel);
		return NULL;
	}

	return panel;
}

void eof_destroy_text_panel(EOF_TEXT_PANEL *panel)
{
	if(panel)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Destroying text panel for file \"%s\"", panel->filename);
		eof_log(eof_log_string, 2);
		if(panel->text)
			free(panel->text);
		free(panel);
	}
}

int eof_expand_notes_window_text(char *src_buffer, char *dest_buffer, unsigned long dest_buffer_size, EOF_TEXT_PANEL *panel)
{
	unsigned long src_index = 0, dest_index = 0, macro_index;
	char src_char, macro[TEXT_PANEL_BUFFER_SIZE+1], expanded_macro[TEXT_PANEL_BUFFER_SIZE+1];
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
					macro_status = eof_expand_notes_window_macro(macro, expanded_macro, TEXT_PANEL_BUFFER_SIZE, panel);	//Convert the macro to static text

					if(macro_status == 1)
					{	//Otherwise if the macro was successfully converted to static text
						strncat(dest_buffer, expanded_macro, dest_buffer_size - strlen(dest_buffer) - 1);	//Append the converted macro text to the destination buffer
						dest_index = strlen(dest_buffer);	//Update the destination buffer index

						if(panel->flush)
						{	//If the flush attribute was set (such as by the %FLUSH% or %BEND% macros), print the current contents of the output buffer and update the print coordinates
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
							if(panel->endline)
							{	//If the printing of this line was signaled to end
								panel->endline = 0;	//Reset this condition
								return 1;
							}
							if(panel->endpanel)
							{	//If the printing of this panel was signaled to end
								return 1;
							}
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

	if(eof_log_level > 2)
	{	//If exhaustive logging is in effect
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tProcessing Notes macro \"%s\"", macro);
		eof_log(eof_log_string, 3);
	}

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

	//The active track is a legacy guitar track
	if(!ustricmp(macro, "IF_IS_LEGACY_GUITAR_TRACK"))
	{
		if(eof_track_is_legacy_guitar(eof_song, eof_selected_track))
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The active track is not a legacy guitar track
	if(!ustricmp(macro, "IF_IS_NOT_LEGACY_GUITAR_TRACK"))
	{
		if(!eof_track_is_legacy_guitar(eof_song, eof_selected_track))
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

	//One of the drum tracks is not active
	if(!ustricmp(macro, "IF_IS_NOT_DRUM_TRACK"))
	{
		if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//A GHL format track is active
	if(!ustricmp(macro, "IF_IS_GHL_TRACK"))
	{
		if(eof_track_is_ghl_mode(eof_song, eof_selected_track))
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//A GHL format track is not active
	if(!ustricmp(macro, "IF_IS_NOT_GHL_TRACK"))
	{
		if(!eof_track_is_ghl_mode(eof_song, eof_selected_track))
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//A non GHL mode legacy guitar track
	if(!ustricmp(macro, "IF_IS_FIVE_BUTTON_GUITAR_TRACK"))
	{
		if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		{	//If the active track is a guitar track
			if(!eof_track_is_ghl_mode(eof_song, eof_selected_track) && (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If it's not a GHL track or a pro guitar track
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	//A GHL mode legacy guitar track or a pro guitar track is active
	if(!ustricmp(macro, "IF_IS_NON_FIVE_BUTTON_GUITAR_TRACK"))
	{
		if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		{	//If the active track is a guitar track
			if(eof_track_is_ghl_mode(eof_song, eof_selected_track) || (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	//A non GHL mode legacy guitar track, or the keys track
	if(!ustricmp(macro, "IF_IS_FIVE_BUTTON_GUITAR_OR_KEYS_TRACK"))
	{
		if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (eof_selected_track == EOF_TRACK_KEYS))
		{	//If the active track is a guitar track, or the keys track
			if(!eof_track_is_ghl_mode(eof_song, eof_selected_track) && (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If it's not a GHL track or a pro guitar track
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	//A legacy, GHL or pro guitar track is active
	if(!ustricmp(macro, "IF_IS_ANY_GUITAR_TRACK"))
	{
		if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		{	//If the active track is a guitar track
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//A legacy, GHL or pro guitar track is not active
	if(!ustricmp(macro, "IF_IS_NOT_ANY_GUITAR_TRACK"))
	{
		if((eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		{	//If the active track is not a guitar track
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The dance track is active
	if(!ustricmp(macro, "IF_IS_DANCE_TRACK"))
	{
		if(eof_selected_track == EOF_TRACK_DANCE)
		{	//If the active track is the dance track
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The dance track is not active
	if(!ustricmp(macro, "IF_IS_NOT_DANCE_TRACK"))
	{
		if(eof_selected_track != EOF_TRACK_DANCE)
		{	//If the active track is not the dance track
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The keys track is active
	if(!ustricmp(macro, "IF_IS_KEYS_TRACK"))
	{
		if(eof_selected_track == EOF_TRACK_KEYS)
		{	//If the active track is the dance track
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//The keys track is not active
	if(!ustricmp(macro, "IF_IS_NOT_KEYS_TRACK"))
	{
		if(eof_selected_track != EOF_TRACK_KEYS)
		{	//If the active track is not the dance track
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

	//If the active difficulty level is a specific number
	if(strcasestr_spec(macro, "IF_ACTIVE_DIFFICULTY_IS_NUMBER_"))
	{
		unsigned long diff;
		char *number_string = strcasestr_spec(macro, "IF_ACTIVE_DIFFICULTY_IS_NUMBER_");	//Get a pointer to the text that is expected to be the difficulty number

		if(eof_read_macro_number(number_string, &diff))
		{	//If the difficulty number was successfully parsed
			if(eof_note_type == diff)
			{	//If the number is the active difficulty number
				return 3;	//True
			}

			return 2;	//False
		}
	}

	//If the active difficulty is Easy (Rock Band difficulty labeling)
	if(!ustricmp(macro, "IF_ACTIVE_DIFFICULTY_IS_EASY"))
	{
		if(eof_note_type == 0)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the active difficulty is Medium (Rock Band difficulty labeling)
	if(!ustricmp(macro, "IF_ACTIVE_DIFFICULTY_IS_MEDIUM"))
	{
		if(eof_note_type == 1)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the active difficulty is Hard (Rock Band difficulty labeling)
	if(!ustricmp(macro, "IF_ACTIVE_DIFFICULTY_IS_HARD"))
	{
		if(eof_note_type == 2)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the active difficulty is Expert (Rock Band difficulty labeling)
	if(!ustricmp(macro, "IF_ACTIVE_DIFFICULTY_IS_EXPERT"))
	{
		if(eof_note_type == 3)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the active difficulty has notes with a specific gem count
	if(strcasestr_spec(macro, "IF_TRACK_DIFF_HAS_NOTES_WITH_GEM_COUNT_"))
	{
		unsigned long gemcount;
		char *count_string = strcasestr_spec(macro, "IF_TRACK_DIFF_HAS_NOTES_WITH_GEM_COUNT_");	//Get a pointer to the text that is expected to be the gem count

		if(eof_read_macro_number(count_string, &gemcount))
		{	//If the gem count was successfully parsed
			if(eof_count_num_notes_with_gem_count(gemcount))
			{	//If there's at least one note in this track difficulty with the specified number of gems
				dest_buffer[0] = '\0';
				return 3;	//True
			}

			return 2;	//False
		}
	}

	if(!ustricmp(macro, "IF_TRACK_DIFF_HAS_INVALID_DRUM_CHORDS"))
	{
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If either drum track is active
			unsigned long ctr;

			for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
			{	//For each note in the track
				if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
				{	//If the note is in the active difficulty
					unsigned char mask = eof_get_note_note(eof_song, eof_selected_track, ctr);

					mask &= ~1;	//Clear any gem on lane 1, since it is not played with the players hands
					if(eof_note_count_colors_bitmask(mask) > 2)
					{	//If this drum note would take more than two hands to play
						dest_buffer[0] = '\0';
						return 3;	//True
					}
				}
			}

			return 2;	//False
		}
	}

	if(!ustricmp(macro, "IF_NONZERO_MIDI_DELAY"))
	{
		if(eof_song->beat[0]->pos != 0)
		{	//If the first beat marker is at any timestamp other than 0ms
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_TRACK_HAS_NO_SOLOS"))
	{
		if(!eof_get_num_solos(eof_song, eof_selected_track))
		{	//If there are no solos in the active track
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_TRACK_HAS_NO_STAR_POWER"))
	{
		if(eof_selected_track == EOF_TRACK_VOCALS)
		{	//The vocals track handles star power on a per lyric line basis
			unsigned long ctr;
			for(ctr = 0; ctr < eof_get_num_lyric_sections(eof_song, eof_selected_track); ctr++)
			{	//For each lyric line
				EOF_PHRASE_SECTION *ptr = eof_get_lyric_section(eof_song, eof_selected_track, ctr);

				if(ptr && (ptr->flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE))
				{	//If the lyric line was found and it has overdrive status
					return 2;	//False
				}
			}
		}
		else if(eof_get_num_star_power_paths(eof_song, eof_selected_track))
		{	//If there are star power phrases in the active track
			return 2;	//False
		}

		dest_buffer[0] = '\0';
		return 3;	//True
	}

	if(!ustricmp(macro, "IF_NO_LOADING_TEXT"))
	{
		if(!eof_check_string(eof_song->tags->loading_text))
		{	//If there is no loading text defined or it is just space characters
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_NO_PREVIEW_START_TIME"))
	{
		unsigned long entrynum = 0, ctr;
		char *ptr, *temp;
		int nonzeroes = 0;

		ptr = eof_find_ini_setting_tag(eof_song, &entrynum, "preview_start_time");
		if(ptr)
		{	//If there is a preview_start_time INI entry
			for(temp = ptr; *temp == ' '; temp++);	//Skip past any leading whitespace in the value portion of the INI setting
			//Manually check if the string is just zeroes, because strtoul() returns zero on error and also zero if the converted number is zero
			for(ctr = 0; temp[ctr] != '\0'; ctr++)
			{	//For each character until the end of the string
				if(temp[ctr] != '0')
				{	//If it's a character other than zero
					nonzeroes = 1;	//Track this
				}
			}
			if(!nonzeroes)
			{	//If the only characters in the string were the digit zero, this counts as a preview start time
				return 2;	//False
			}
			if(strtoul(temp, NULL, 10))
			{	//If the string was converted to a number that wasn't zero, this counts as a preview start time
				return 2;	//False
			}
		}

		dest_buffer[0] = '\0';
		return 3;	//True
	}

	if(!ustricmp(macro, "IF_NOTE_GAP_IS_IN_MS"))
	{
		if(!eof_min_note_distance_intervals)
		{	//If the configured note gap is in milliseconds instead of a per beat/measure interval
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CH_SP_TS_MISSING"))
	{
		if(!eof_beat_stats_cached)
			eof_process_beat_statistics(eof_song, eof_selected_track);
		if(!eof_song->beat[0]->has_ts)
		{	//If the first beat does not have a time signature
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CH_SP_TS_DEFINED"))
	{
		if(!eof_beat_stats_cached)
			eof_process_beat_statistics(eof_song, eof_selected_track);
		if(eof_song->beat[0]->has_ts)
		{	//If the first beat has a time signature
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CH_SP_PATH_VALID"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score && eof_ch_sp_solution->num_deployments)
		{	//If the global star power solution structure is built, the score is nonzero and there is at least one defined SP deployment
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CH_SP_DEPLOYMENTS_MISSING"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score && !eof_ch_sp_solution->num_deployments)
		{	//If the global star power solution structure is built, the score is nonzero but there are no defined SP deployments
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CH_SP_PATH_INVALID"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && !eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built but its score is zero
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

	//Move the text output up by a given number of pixels
	if(strcasestr_spec(macro, "MOVE_UP_PIXELS_"))
	{
		unsigned long pixelcount;
		char *count_string = strcasestr_spec(macro, "MOVE_UP_PIXELS_");	//Get a pointer to the text that is expected to be the pixel count

		if(eof_read_macro_number(count_string, &pixelcount))
		{	//If the pixel count was successfully parsed
			dest_buffer[0] = '\0';
			if(panel->ypos)
				panel->ypos -= pixelcount;	//Update the y coordinate for Notes panel printing
			return 1;
		}
	}

	//Move the text output down one pixel
	if(!ustricmp(macro, "MOVE_DOWN_ONE_PIXEL"))
	{
		dest_buffer[0] = '\0';
		panel->ypos++;	//Update the y coordinate for Notes panel printing
		return 1;
	}

	//Move the text output down by a given number of pixels
	if(strcasestr_spec(macro, "MOVE_DOWN_PIXELS_"))
	{
		unsigned long pixelcount;
		char *count_string = strcasestr_spec(macro, "MOVE_DOWN_PIXELS_");	//Get a pointer to the text that is expected to be the pixel count

		if(eof_read_macro_number(count_string, &pixelcount))
		{	//If the pixel count was successfully parsed
			dest_buffer[0] = '\0';
			panel->ypos += pixelcount;	//Update the y coordinate for Notes panel printing
			return 1;
		}
	}

	//Move the text output left one pixel
	if(!ustricmp(macro, "MOVE_LEFT_ONE_PIXEL"))
	{
		dest_buffer[0] = '\0';
		panel->xpos--;	//Update the x coordinate for Notes panel printing
		return 1;
	}

	//Move the text output left by a given number of pixels
	if(strcasestr_spec(macro, "MOVE_LEFT_PIXELS_"))
	{
		unsigned long pixelcount;
		char *count_string = strcasestr_spec(macro, "MOVE_LEFT_PIXELS_");	//Get a pointer to the text that is expected to be the pixel count

		if(eof_read_macro_number(count_string, &pixelcount))
		{	//If the pixel count was successfully parsed
			dest_buffer[0] = '\0';
			if(panel->xpos)
				panel->xpos -= pixelcount;	//Update the x coordinate for Notes panel printing
			return 1;
		}
	}

	//Move the text output left one pixel
	if(!ustricmp(macro, "MOVE_RIGHT_ONE_PIXEL"))
	{
		dest_buffer[0] = '\0';
		panel->xpos++;	//Update the x coordinate for Notes panel printing
		return 1;
	}

	//Move the text output right by a given number of pixels
	if(strcasestr_spec(macro, "MOVE_RIGHT_PIXELS_"))
	{
		unsigned long pixelcount;
		char *count_string = strcasestr_spec(macro, "MOVE_RIGHT_PIXELS_");	//Get a pointer to the text that is expected to be the pixel count

		if(eof_read_macro_number(count_string, &pixelcount))
		{	//If the pixel count was successfully parsed
			dest_buffer[0] = '\0';
			panel->xpos += pixelcount;	//Update the x coordinate for Notes panel printing
			return 1;
		}
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

	//End printing of the current line
	if(!ustricmp(macro, "ENDLINE"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->endline = 1;
		return 1;
	}

	//End printing of the entire panel
	if(!ustricmp(macro, "ENDPANEL"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->endpanel = 1;
		return 1;
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

	//Track solo count
	if(!ustricmp(macro, "TRACK_SOLO_COUNT"))
	{
		unsigned long count = eof_get_num_solos(eof_song, eof_selected_track);

		if(count)
		{	//If there are any solos in the active track
			snprintf(dest_buffer, dest_buffer_size, "%lu", count);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Track solo count
	if(!ustricmp(macro, "TRACK_SOLO_NOTE_COUNT"))
	{
		unsigned long count;

		count = eof_notes_panel_count_section_stats(EOF_SOLO_SECTION, NULL, NULL);

		if(count)
		{	//If there are any solo notes in the active track
			double percent;

			percent = (double)count * 100.0 / eof_get_track_size(eof_song, eof_selected_track);
			snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Track solo note stats
	if(!ustricmp(macro, "TRACK_SOLO_NOTE_STATS"))
	{
		unsigned long count, solocount, min = 0, max = 0;

		count = eof_notes_panel_count_section_stats(EOF_SOLO_SECTION, &min, &max);
		solocount = eof_get_num_solos(eof_song, eof_selected_track);	//Redundantly check that this isn't zero to resolve a false positive in Coverity

		if(count && solocount)
		{	//If there are any solo notes in the active track
			snprintf(dest_buffer, dest_buffer_size, "%lu/%lu/%.2f (min/max/mean)", min, max, (double)count/solocount);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Track star power count
	if(!ustricmp(macro, "TRACK_SP_COUNT"))
	{
		unsigned long count = eof_get_num_star_power_paths(eof_song, eof_selected_track);

		if(count)
		{	//If there are any star power sections in the active track
			snprintf(dest_buffer, dest_buffer_size, "%lu", count);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Track star power note count
	if(!ustricmp(macro, "TRACK_SP_NOTE_COUNT"))
	{
		unsigned long count;

		count = eof_count_track_num_notes_with_flag(EOF_NOTE_FLAG_SP);

		if(count)
		{	//If there are any star power sections in the active track
			double percent;

			percent = (double)count * 100.0 / eof_get_track_size(eof_song, eof_selected_track);
			snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Star power lyric line count
	if(!ustricmp(macro, "SP_LYRIC_LINE_COUNT"))
	{
		unsigned long ctr, count = 0, linecount;

		linecount = eof_get_num_lyric_sections(eof_song, EOF_TRACK_VOCALS);	//Redundantly check that this isn't zero to resolve a false positive in Coverity
		for(ctr = 0; ctr < linecount; ctr++)
		{	//For each lyric line
			EOF_PHRASE_SECTION *ptr = eof_get_lyric_section(eof_song, EOF_TRACK_VOCALS, ctr);

			if(ptr && (ptr->flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE))
			{	//If the lyric line was found and it has overdrive status
				count++;
			}
		}
		if(count && linecount)
		{
			double percent;

			percent = (double)count * 100.0 / linecount;
			snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Track solo note stats
	if(!ustricmp(macro, "TRACK_SP_NOTE_STATS"))
	{
		unsigned long count, min = 0, max = 0, spcount;

		count = eof_notes_panel_count_section_stats(EOF_SP_SECTION, &min, &max);
		spcount = eof_get_num_star_power_paths(eof_song, eof_selected_track);	//Redundantly check that this isn't zero to resolve a false positive in Coverity

		if(count && spcount)
		{	//If there are any star power notes in the active track
			snprintf(dest_buffer, dest_buffer_size, "%lu/%lu/%.2f (min/max/mean)", min, max, (double)count/spcount);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Track star power count
	if(!ustricmp(macro, "TRACK_SLIDER_COUNT"))
	{
		unsigned long count = eof_get_num_sliders(eof_song, eof_selected_track);

		if(count)
		{	//If there are any star power sections in the active track
			snprintf(dest_buffer, dest_buffer_size, "%lu", count);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Track slider note count
	if(!ustricmp(macro, "TRACK_SLIDER_NOTE_COUNT"))
	{
		unsigned long count;

		count = eof_count_track_num_notes_with_flag(EOF_GUITAR_NOTE_FLAG_IS_SLIDER);

		if(count)
		{	//If there are any slider sections in the active track
			double percent;

			percent = (double)count * 100.0 / eof_get_track_size(eof_song, eof_selected_track);
			snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
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

	//Selected note/lyric flags
	if(!ustricmp(macro, "SELECTED_NOTE_FLAGS"))
	{
		if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_get_note_flags(eof_song, eof_selected_track, eof_selection.current));
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

	//Seek position as a percentage of the chart's total length
	if(!ustricmp(macro, "SEEK_POSITION_PERCENT"))
	{
		double percent = (double)(eof_music_pos - eof_av_delay) * 100.0 / eof_chart_length;

		snprintf(dest_buffer, dest_buffer_size, "%.2f", percent);

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

	//The defined star power path's estimated score
	if(!ustricmp(macro, "CH_SP_PATH_SCORE"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built and a score was determined
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_ch_sp_solution->score);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//The defined star power path's base score (used in star calculation)
	if(!ustricmp(macro, "CH_SP_PATH_BASE_SCORE"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built and a score was determined
			unsigned long base_score = 0;

			if(eof_ch_sp_path_calculate_stars(eof_ch_sp_solution, &base_score, NULL) != ULONG_MAX)
			{	//If the base score was calculated
				snprintf(dest_buffer, dest_buffer_size, "%lu", base_score);
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "(Error)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//The defined star power path's estimated average multiplier
	if(!ustricmp(macro, "CH_SP_PATH_AVG_MULTIPLIER"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built and a score was determined
			unsigned long base_score = 0, effective_score = 0;

			if(eof_ch_sp_path_calculate_stars(eof_ch_sp_solution, &base_score, &effective_score) != ULONG_MAX)
			{	//If the base and effective scores were calculated
				snprintf(dest_buffer, dest_buffer_size, "%.2f", (double)effective_score / base_score);
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "(Error)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//The defined star power path's estimated number of awarded stars
	if(!ustricmp(macro, "CH_SP_PATH_STARS"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built and a score was determined
			unsigned long stars = eof_ch_sp_path_calculate_stars(eof_ch_sp_solution, NULL, NULL);

			if(stars != ULONG_MAX)
			{	//If the number of stars was determined
				snprintf(dest_buffer, dest_buffer_size, "%lu star%s", stars, (stars == 1) ? "" : "s");
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "(Error)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//The estimated number of notes that are played during the defined star power path's deployments
	if(!ustricmp(macro, "CH_SP_PATH_DEPLOYMENT_NOTES"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built and a score was determined
			snprintf(dest_buffer, dest_buffer_size, "%lu notes (%.2f%%)", eof_ch_sp_solution->deployment_notes, (double)eof_ch_sp_solution->deployment_notes * 100.0 / eof_ch_sp_solution->note_count);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//The status of whether the seek position is within a defined SP deployment
	if(!ustricmp(macro, "CH_SP_SEEK_SP_PATH_STATUS"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score && eof_ch_sp_solution->num_deployments)
		{	//If the global star power solution structure is built and a score was determined
			unsigned long ctr, target = ULONG_MAX, notenum, sp_start = ULONG_MAX, sp_end;
			int status = 0;

			for(ctr = 0; ctr < eof_ch_sp_solution->num_deployments; ctr++)
			{	//For each defined SP deployment
				notenum = eof_translate_track_diff_note_index(eof_song, eof_ch_sp_solution->track, eof_ch_sp_solution->diff, eof_ch_sp_solution->deployments[ctr]);	//Find the real note number for this index
				sp_start = eof_get_note_pos(eof_song, eof_selected_track, notenum);	//Get the start position of this deployment
				if(sp_start <= eof_music_pos - eof_av_delay)
				{	//If this deployment begins at or before the seek position
					target = ctr;	//Remember the last deployment that meets this criterion
				}
			}
			if(target < eof_ch_sp_solution->num_deployments)
			{	//If a star power deployment before the seek position was found
				sp_end = eof_get_realtime_position(eof_ch_sp_solution->deployment_endings[target]);	//Get the end position of this deployment
				if(sp_end >= eof_music_pos - eof_av_delay)
				{	//If this deployment ends at or after the seek position
					status = 1;	//Track that the seek position was found to be inside a defined SP deployment
					snprintf(dest_buffer, dest_buffer_size, "Seek pos within %lums to %lums deployment", sp_start, sp_end);
				}
			}
			if(!status)
			{	//If the seek position was not within a defined SP deployment
				snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within a deployment");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "No valid SP path is defined");
		}
		return 1;
	}

	//The maximum number of SP deployments based on the currently defined star power notes
	if(!ustricmp(macro, "CH_SP_MAX_DEPLOYMENT_COUNT_AND_RATIO"))
	{
		eof_ch_sp_solution_macros_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built and a score was determined
			unsigned long count = 0;

			(void) eof_count_selected_notes(&count);	//Count the number of notes in the active track difficulty
			snprintf(dest_buffer, dest_buffer_size, "%lu (1 : %.2f notes)", eof_ch_sp_solution->deploy_count, (double)count / eof_ch_sp_solution->deploy_count);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
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

	//The active difficulty number
	if(!ustricmp(macro, "ACTIVE_DIFFICULTY_NUMBER"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%u", eof_note_type);
		return 1;
	}

	//The active difficulty name
	if(!ustricmp(macro, "ACTIVE_DIFFICULTY_NAME"))
	{
		if(eof_note_type < 5)
		{	//If one of the named difficulties is active
			snprintf(dest_buffer, dest_buffer_size, "%s", &eof_note_type_name[eof_note_type][1]);	//Skip the leading space in the difficulty name string (used to mark difficulty populated status)
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "(Unnamed)");
		}
		return 1;
	}

	//Display the number of notes (and the corresponding percentage that is of all notes) in the active track difficulty with a specific gem count
	if(strcasestr_spec(macro, "TRACK_DIFF_NUMBER_NOTES_WITH_GEM_COUNT_"))
	{
		unsigned long count, gemcount, totalnotecount = 0;
		char *count_string = strcasestr_spec(macro, "TRACK_DIFF_NUMBER_NOTES_WITH_GEM_COUNT_");	//Get a pointer to the text that is expected to be the gem count

		if(eof_read_macro_number(count_string, &gemcount))
		{	//If the gem count was successfully parsed
			double percent;

			count = eof_count_num_notes_with_gem_count(gemcount);		//Determine how many such notes are in the active track difficulty
			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{	//If there's at least one note in the active track difficulty
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
			return 1;
		}
	}

	//Display the number of notes in the active track difficulty with a specific gem combination
	if(strcasestr_spec(macro, "TRACK_DIFF_NOTE_COUNT_INSTANCES_"))
	{
		unsigned long count;
		unsigned char gems, toms, cymbals;
		char *gem_string = strcasestr_spec(macro, "TRACK_DIFF_NOTE_COUNT_INSTANCES_");	//Get a pointer to the text that is expected to be the gem combination

		if(eof_read_macro_gem_designations(gem_string, &gems, &toms, &cymbals))
		{	//If the gem designations were successfully parsed
			count = eof_count_num_notes_with_gem_designation(gems, toms, cymbals);		//Determine how many such notes are in the active track difficulty
			snprintf(dest_buffer, dest_buffer_size, "%lu", count);

			return 1;
		}
	}

	//Display the number of notes (and the corresponding percentage that is of all notes) in the active track difficulty with a specific gem combination
	if(strcasestr_spec(macro, "TRACK_DIFF_NOTE_COUNT_AND_RATIO_INSTANCES_"))
	{
		unsigned long count, totalnotecount = 0;
		unsigned char gems, cymbals, toms;
		char *gem_string = strcasestr_spec(macro, "TRACK_DIFF_NOTE_COUNT_AND_RATIO_INSTANCES_");	//Get a pointer to the text that is expected to be the gem combination

		if(eof_read_macro_gem_designations(gem_string, &gems, &toms, &cymbals))
		{	//If the gem designations were successfully parsed
			double percent;

			count = eof_count_num_notes_with_gem_designation(gems, toms, cymbals);		//Determine how many such notes are in the active track difficulty
			(void) eof_count_selected_notes(&totalnotecount);							//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{	//If there's at least one note in the active track difficulty
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}

			return 1;
		}
	}

	//Display the number of open chords (and the corresponding percentage that is of all notes) in the active pro guitar track difficulty
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_OPEN_CHORDS"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			unsigned long ctr, count = 0, totalnotecount = 0;
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tp->notes; ctr++)
				{	//For each note in the pro guitar track
					if((tp->note[ctr]->type == eof_note_type) && eof_pro_guitar_note_is_open_chord(tp, ctr))
						count++;	//Count the number of notes in the active difficulty that are open chords
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of barre chords (and the corresponding percentage that is of all notes) in the active pro guitar track difficulty
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_BARRE_CHORDS"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			unsigned long ctr, count = 0, totalnotecount = 0;
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tp->notes; ctr++)
				{	//For each note in the pro guitar track
					if((tp->note[ctr]->type == eof_note_type) && eof_pro_guitar_note_is_barre_chord(tp, ctr))
						count++;	//Count the number of notes in the active difficulty that are barre chords
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of fully string muted notes/chords (and the corresponding percentage that is of all notes) in the active pro guitar track difficulty
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_STRING_MUTES"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			unsigned long ctr, count = 0, totalnotecount = 0;

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the track
					if((eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type) && eof_is_string_muted(eof_song, eof_selected_track, ctr))
						count++;	//Count the number of notes in the active difficulty that are fully string muted
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of notes containing expert+ bass drum (and the corresponding percentage that is of all notes) in the active expert drum difficulty
	if(!ustricmp(macro, "TRACK_DIFF_NOTE_COUNT_AND_RATIO_EXPERT_PLUS_BASS"))
	{
		if((eof_note_type == EOF_NOTE_AMAZING) && (eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR))
		{	//If the expert difficulty in either drum track is active
			unsigned long ctr, count = 0, totalnotecount = 0;

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each note in the track
					if((eof_get_note_type(eof_song, eof_selected_track, ctr) == EOF_NOTE_AMAZING) && (eof_get_note_note(eof_song, eof_selected_track, ctr) & 1))
					{	//If the note is in the expert difficulty and has a gem on lane 1 (bass drum)
						if(eof_get_note_flags(eof_song, eof_selected_track, ctr) & EOF_DRUM_NOTE_FLAG_DBASS)
						{	//If the note has the expert+ double bass status
							count++;
						}
					}
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of pitched lyrics (and the corresponding percentage that is of all notes) in the active vocal track
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_PITCHED_LYRIC"))
	{
		if(eof_vocals_selected)
		{	//If the vocal track is active
			unsigned long ctr, count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each lyric in the track
					if(eof_lyric_is_pitched(tp, ctr))
					{	//If the lyric has a valid pitch and isn't freestyle
						count++;
					}
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of unpitched lyrics (and the corresponding percentage that is of all notes) in the active vocal track
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_UNPITCHED_LYRIC"))
	{
		if(eof_vocals_selected)
		{	//If the vocal track is active
			unsigned long ctr, count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tp->lyrics; ctr++)
				{	//For each lyric in the track
					if(tp->lyric[ctr]->note == 0)
					{	//If the lyric has no defined pitch
						count++;
					}
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of vocal percussion lyrics (and the corresponding percentage that is of all notes) in the active vocal track
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_PERCUSSION_LYRIC"))
	{
		if(eof_vocals_selected)
		{	//If the vocal track is active
			unsigned long ctr, count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tp->lyrics; ctr++)
				{	//For each lyric in the track
					if(tp->lyric[ctr]->note == EOF_LYRIC_PERCUSSION)
					{	//If the lyric is percussion
						count++;
					}
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of freestyle lyrics (and the corresponding percentage that is of all notes) in the active vocal track
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_FREESTYLE_LYRIC"))
	{
		if(eof_vocals_selected)
		{	//If the vocal track is active
			unsigned long ctr, count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each lyric in the track
					if(eof_lyric_is_freestyle(tp, ctr) == 1)
					{	//If the lyric has a freestyle marker (# or ^)
						count++;
					}
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of pitch shifts (and the corresponding percentage that is of all notes) in the active vocal track
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_PITCH_SHIFT_LYRIC"))
	{
		if(eof_vocals_selected)
		{	//If the vocal track is active
			unsigned long ctr, count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 1; ctr < tp->lyrics; ctr++)
				{	//For each lyric in the track after the first
					if(tp->lyric[ctr]->text[0] == '+')
					{	//If the lyric's text begins with a + sign
						count++;
					}
				}
				percent = (double)count * 100.0 / (double)totalnotecount;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));	//Round to the nearest percent
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "0 (0%%)");
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//Display the number of notes (and the corresponding percentage that is of all notes) in the active track difficulty with a specific pitch
	if(strcasestr_spec(macro, "COUNT_LYRICS_WITH_PITCH_NUMBER_"))
	{
		unsigned long ctr, count = 0, pitch;
		char *count_string = strcasestr_spec(macro, "COUNT_LYRICS_WITH_PITCH_NUMBER_");	//Get a pointer to the text that is expected to be the pitch

		if(eof_vocals_selected)
		{	//If the vocal track is active
			if(eof_read_macro_number(count_string, &pitch))
			{	//If the pitch number was successfully parsed
				for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
				{	//For each lyric
					if(eof_get_note_note(eof_song, eof_selected_track, ctr) == pitch)
					{	//If the lyric has the target pitch
						count++;
					}
				}
				snprintf(dest_buffer, dest_buffer_size, "%lu", count);
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}

		return 1;
	}

	//The current 3D maximum depth
	if(!ustricmp(macro, "3D_MAX_DEPTH"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%d", eof_3d_max_depth);
		return 1;
	}

	//The current mouse x coordinate
	if(!ustricmp(macro, "MOUSE_X"))
	{
		char name[20] = {0};
		EOF_WINDOW *ptr = eof_coordinates_identify_window(mouse_x, mouse_y, name);	//Determine which subwindow the mouse is in
		if(ptr)
		{	//If it is in a subwindow
			snprintf(dest_buffer, dest_buffer_size, "%d (%s: %d)", mouse_x, name, mouse_x - ptr->x);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "%d", mouse_x);
		}
		return 1;
	}

	//The current mouse y coordinate
	if(!ustricmp(macro, "MOUSE_Y"))
	{
		char name[20] = {0};
		EOF_WINDOW *ptr = eof_coordinates_identify_window(mouse_x, mouse_y, name);	//Determine which subwindow the mouse is in
		if(ptr)
		{	//If it is in a subwindow
			snprintf(dest_buffer, dest_buffer_size, "%d (%s: %d)", mouse_y, name, mouse_y - ptr->y);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "%d", mouse_y);
		}
		return 1;
	}

	//The currently hovered over lane
	if(!ustricmp(macro, "EOF_HOVER_PIECE"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%d", eof_hover_piece);
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

int eof_read_macro_number(char *string, unsigned long *number)
{
	unsigned long index, value = 0;
	int ch;

	if(!string || !number)
		return 0;	//Invalid parameters

	for(index = 0; string[index] != '\0'; index++)
	{	//For each character in the string
		ch = string[index];	//Read the character

		if(!isdigit(ch))	//If this isn't a numerical character
			return 0;		//Not a valid number macro

		value *= 10;		//Another digit was read, meaning the previous ones are worth ten times as much
		value += ch - '0';	//Add the numerical conversion of this digit
	}

	if(!index)	//If there were no digits parsed
		return 0;	//Not a valid number macro

	*number = value;
	return 1;		//Return success
}

int eof_read_macro_gem_designations(char *string, unsigned char *bitmask, unsigned char *tomsmask, unsigned char *cymbalsmask)
{
	unsigned long index;
	unsigned char mask = 0, toms = 0, cymbals = 0, oldmask;
	int ch;

	if(!string || !bitmask || !tomsmask || !cymbalsmask)
		return 0;	//Invalid parameters

	for(index = 0; string[index] != '\0'; index++)
	{	//For each character in the string
		ch = string[index];	//Read the character
		oldmask = mask;		//Back up the mask string to determine if the mask was altered in each loop iteration

		if(!isalnum(ch))	//If this isn't an alphabetical or numerical character
			return 0;		//Not a valid gem designation
		if(isalpha(ch))
			ch = toupper(ch);	//Convert letters to uppercase

		//Check for lane number designations
		if(ch == '1')
		{
			mask |= 1;
		}
		else if(ch == '2')
		{
			mask |= 2;
		}
		else if(ch == '3')
		{
			mask |= 4;
		}
		else if(ch == '4')
		{
			mask |= 8;
		}
		else if(ch == '5')
		{
			mask |= 16;
		}
		else if(ch == '6')
		{
			mask |= 32;
		}
		if(mask != oldmask)	//If the mask was updated
			continue;		//Skip the rest of the checks for this character

		//Check for lane color designations
		if(eof_track_is_ghl_mode(eof_song, eof_selected_track))
		{	//GHL tracks have these gem designations:  "B1" = lane 1, "B2" = lane 2, "B3" = lane 3, "W1" = lane 4, "W2" = lane 5, "W3" = lane 6, 'S' = open strum
			if(ch == 'W')
			{	//One of the white gems
				index++;	//Iterate to next character
				ch = string[index];	//Read the character

				if(ch == '1')
				{
					mask |= 8;		//W1 is lane 4
				}
				else if(ch == '2')
				{
					mask |= 16;		//W2 is lane 5
				}
				else if(ch == '3')
				{
					mask |= 32;		//W3 is lane 6
				}
				else
				{
					return 0;	//Invalid gem designation
				}
			}
			else if(ch == 'B')
			{	//One of the black gems
				index++;	//Iterate to next character
				ch = string[index];	//Read the character

				if(ch == '1')
				{
					mask |= 1;		//B1 is lane 1
				}
				else if(ch == '2')
				{
					mask |= 2;		//B2 is lane 2
				}
				else if(ch == '3')
				{
					mask |= 4;		//B3 is lane 3
				}
				else
				{
					return 0;	//Invalid gem designation
				}
			}
			else if(ch == 'S')
			{
				*bitmask = 255;	//Any inclusion of open strum designation will cause it to override any other content in the string
				return 1;		//Return success
			}
		}
		else
		{	//Non GHL tracks have these gem designations:  'G' = lane 1, 'R' = lane 2, 'Y' = lane 3, 'B' = lane 4, 'O' = lane 5, 'P' = lane 6, 'S' = open strum
			//Or:  '1' = lane 1, '2' = lane 2, '3' = lane 3, '4' = lane 4, '5' = lane 5, '6' = lane 6
			if(ch == 'G')
			{
				mask |= 1;
			}
			else if(ch == 'R')
			{
				mask |= 2;
			}
			else if(ch == 'Y')
			{
				mask |= 4;
			}
			else if(ch == 'B')
			{
				mask |= 8;
			}
			else if(ch == 'O')
			{
				mask |= 16;
			}
			else if(ch == 'P')
			{
				mask |= 32;
			}
			else if(ch == 'S')
			{
				*bitmask = 255;	//Any inclusion of open strum designation will cause it to override any other content in the string
				return 1;		//Return success
			}
		}
		if(mask != oldmask)	//If the mask was updated
			continue;		//Skip the rest of the checks for this character

		//Check for drum track specific tom/cymbal designations
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//Drum tracks have these cymbal and tom designations:  "T3" = lane 3 tom, "T4" = lane 4 tom, "T5" = lane 5 tom, "C3" = lane 3 cymbal, "C4" = lane 4 cymbal, "C5" = lane 5 cymbal
			if(ch == 'T')
			{	//One of the tom gems
				index++;	//Iterate to next character
				ch = string[index];	//Read the character

				if(ch == '3')
				{
					mask |= 4;
					toms |= 4;
				}
				else if(ch == '4')
				{
					mask |= 8;
					toms |= 8;
				}
				else if(ch == '5')
				{
					mask |= 16;
					toms |= 16;
				}
				else
				{
					return 0;	//Invalid gem designation
				}
			}
			else if(ch == 'C')
			{	//One of the cymbal gems
				index++;	//Iterate to next character
				ch = string[index];	//Read the character

				if(ch == '3')
				{
					mask |= 4;
					cymbals |= 4;
				}
				else if(ch == '4')
				{
					mask |= 8;
					cymbals |= 8;
				}
				else if(ch == '5')
				{
					mask |= 16;
					cymbals |= 16;
				}
				else
				{
					return 0;	//Invalid gem designation
				}
			}
		}
	}//For each character in the string

	*bitmask = mask;
	*tomsmask = toms;
	*cymbalsmask = cymbals;
	return 1;	//Return success
}

void eof_render_text_panel(EOF_TEXT_PANEL *panel, int opaque)
{
	char buffer[TEXT_PANEL_BUFFER_SIZE+1], buffer2[TEXT_PANEL_BUFFER_SIZE+1];
	unsigned long src_index, dst_index, linectr = 1;
	int retval;

	if(!eof_song_loaded || !eof_song)	//If a project isn't loaded
		return;			//Return immediately
	if(!panel || !panel->window || !panel->text)	//If the provided panel isn't valid
		return;			//Invalid parameter

	if(eof_log_level > 2)
	{	//If exhaustive logging is enabled
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRendering text panel for file \"%s\"", panel->filename);
		eof_log(eof_log_string, 3);
	}

	eof_log("\t\tClearing window to gray", 3);
	if(opaque)
		clear_to_color(panel->window->screen, eof_color_gray);

	//Initialize the panel array
	eof_log("\t\tInitializing panel variables", 3);
	panel->ypos = 0;
	panel->xpos = 2;
	panel->color = eof_color_white;
	panel->bgcolor = -1;	//Transparent background for text
	panel->allowempty = 0;
	panel->flush = 0;
	panel->contentprinted = 0;
	panel->symbol = 0;
	panel->endline = 0;
	panel->endpanel = 0;

	if(eof_log_level > 2)
	{	//If exhaustive logging is enabled
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tBeginning processing of panel text beginning with line:  %.20s", panel->text);
		eof_log(eof_log_string, 3);
	}

	//Parse the contents of the buffered file one line at a time and print each to the screen
	src_index = dst_index = 0;	//Reset these indexes
	while(panel->text[src_index] != '\0')
	{	//Until the end of the text file is reached
		char thischar = panel->text[src_index++];	//Read one character from the source buffer

		if(dst_index >= TEXT_PANEL_BUFFER_SIZE)
		{	//If the output buffer is full
			buffer[TEXT_PANEL_BUFFER_SIZE] = '\0';	//NULL terminate the buffer at its last byte
			eof_log("\t\t\tBuffer full", 3);
			return;
		}
		if((thischar == '\r') && (panel->text[src_index] == '\n'))
		{	//Carriage return and line feed characters represent a new line
			buffer[dst_index] = '\0';	//NULL terminate the buffer
			retval = eof_expand_notes_window_text(buffer, buffer2, TEXT_PANEL_BUFFER_SIZE, panel);
			if(!retval)
			{	//If the buffer's content was not successfully parsed to expand macros, disable the notes panel
				eof_enable_notes_panel = 0;
				eof_log("\t\t\tMacro expansion error", 3);
				return;
			}
			if(panel->allowempty || panel->contentprinted || (buffer2[0] != '\0'))
			{	//If the printing of an empty line was allowed by the %EMPTY% macro or this line isn't empty
				//If content was printed earlier in the line and flushed to the Notes panel, allow the coordinates to reset to the next line
				eof_log("\t\t\tPrinting line", 3);
				textout_ex(panel->window->screen, font, buffer2, panel->xpos, panel->ypos, panel->color, panel->bgcolor);	//Print this line to the screen
				panel->allowempty = 0;	//Reset this condition, it has to be enabled per-line
				panel->xpos = 2;			//Reset the x coordinate to the beginning of the line
				panel->ypos +=12;
				panel->contentprinted = 0;
				eof_log("\t\t\tLine printed", 3);
			}

			dst_index = 0;	//Reset the destination buffer index
			src_index++;	//Seek past the line feed character in the source buffer
			if(panel->endpanel)
			{	//If the printing of this panel was signaled to end
				break;	//Break from while loop
			}
			linectr++;		//Track the line number being processed for debugging purposes
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tBeginning processing of panel text line #%lu", linectr);
			eof_log(eof_log_string, 3);
		}
		else
		{
			buffer[dst_index++] = thischar;	//Append the character to the destination buffer
		}
	}

	//Print any remaining content in the output buffer
	if(dst_index && (dst_index < TEXT_PANEL_BUFFER_SIZE))
	{	//If there are any characters in the destination buffer, and there is room in the buffer for the NULL terminator
		eof_log("\t\tProcessing remainder of buffer", 3);
		buffer[dst_index] = '\0';	//NULL terminate the buffer
		retval = eof_expand_notes_window_text(buffer, buffer2, TEXT_PANEL_BUFFER_SIZE, panel);
		if(!retval)
		{	//If the buffer's content was not successfully parsed to expand macros, disable the notes panel
			eof_enable_notes_panel = 0;
			eof_log("\t\t\tMacro expansion error", 3);
			return;
		}
		if((retval == 2) || (buffer2[0] != '\0'))
		{	//If the printing of an empty line was allowed by the %EMPTY% macro or this line isn't empty
			eof_log("\t\t\tPrinting line", 3);
			textout_ex(panel->window->screen, font, buffer2, panel->xpos, panel->ypos, panel->color, panel->bgcolor);	//Print this line to the screen
			panel->ypos +=12;
			eof_log("\t\t\tLine printed", 3);
		}
	}

	//Draw a border around the edge of the notes panel
	eof_log("\t\tPrinting completed.  Rendering panel border", 3);
	if(opaque)
	{	//But only if this panel wasn't meant to obscure whatever it's drawn on top of
		rect(panel->window->screen, 0, 0, panel->window->w - 1, panel->window->h - 1, eof_color_dark_silver);
		rect(panel->window->screen, 1, 1, panel->window->w - 2, panel->window->h - 2, eof_color_black);
		hline(panel->window->screen, 1, panel->window->h - 2, panel->window->w - 2, eof_color_white);
		vline(panel->window->screen, panel->window->w - 2, 1, panel->window->h - 2, eof_color_white);
	}

	eof_log("\t\tText panel render complete.", 3);
}

unsigned long eof_count_num_notes_with_gem_count(unsigned long gemcount)
{
	unsigned long ctr, count, notenum;

	if(!eof_song)
		return 0;	//Error

	notenum = eof_get_track_size(eof_song, eof_selected_track);	//Store this count
	for(ctr = 0, count = 0; ctr < notenum; ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type)	//If this note isn't in the active difficulty
			continue;	//Skip it

		if(eof_legacy_guitar_note_is_open(eof_song, eof_selected_track, ctr))
		{	//If this is an open note
			if(!gemcount)	//If the function is meant to count open notes
				count++;
			continue;		//Skip the rest of the processing for this note
		}
		else
		{	//This is not an open note
			if(!gemcount)	//If the function is meant to count open notes
				continue;	//Skip this note
		}

		if(eof_note_count_colors_bitmask(eof_get_note_note(eof_song, eof_selected_track, ctr)) == gemcount)
		{	//If this note has the target number of gems
			count++;
		}
	}

	return count;
}

unsigned long eof_count_num_notes_with_gem_designation(unsigned char gems, unsigned char toms, unsigned char cymbals)
{
	unsigned long ctr, count, notenum;
	int is_open;

	if(!eof_song)
		return 0;	//Error

	notenum = eof_get_track_size(eof_song, eof_selected_track);	//Store this count
	for(ctr = 0, count = 0; ctr < notenum; ctr++)
	{	//For each note in the active track
		is_open = 0;

		if(eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type)	//If this note isn't in the active difficulty
			continue;	//Skip it

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			if(eof_pro_guitar_note_highest_fret(eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum], ctr) == 0)
			{	//If no gems in this note had a fret value defined (open note)
				if(gems == 255)
				{	//If the function is meant to count open notes
					count++;
					continue;		//Skip the rest of the processing for this note
				}
			}
		}
		else if(eof_legacy_guitar_note_is_open(eof_song, eof_selected_track, ctr))
		{	//If this is an open note in a legacy track
			is_open = 1;
		}
		if(is_open)
		{	//If this is an pro guitar open note
			if(gems == 255)	//If the function is meant to count open notes
				count++;
			continue;		//Skip the rest of the processing for this note
		}
		else
		{	//This is not an open note
			if(gems == 255)	//If the function is meant to count open notes
				continue;	//Skip this note
		}

		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active, check if the tom and cymbal criteria are met
			unsigned char thistoms, thiscymbals;

			(void) eof_get_drum_note_masks(eof_song, eof_selected_track, ctr, &thistoms, &thiscymbals);	//Look up the toms and cymbals used by this note
			if(((toms & thistoms) != toms) || ((cymbals & thiscymbals) != cymbals))
			{	//If this note does not have the required toms or cymbals
				continue;	//Skip it
			}
		}

		if(eof_get_note_note(eof_song, eof_selected_track, ctr) == gems)
		{	//If this note has the target bitmask
			count++;
		}
	}

	return count;
}

unsigned long eof_count_track_num_notes_with_flag(unsigned long flags)
{
	unsigned long ctr, count, tracksize;

	tracksize = eof_get_track_size(eof_song, eof_selected_track);
	for(ctr = 0, count = 0; ctr < tracksize; ctr++)
	{	//For each note in the track
		unsigned long noteflags = eof_get_note_flags(eof_song, eof_selected_track, ctr);

		if(noteflags & flags)
		{	//If the note has the specified flag(s)
			count++;
		}
	}

	return count;
}

unsigned long eof_notes_panel_count_section_stats(unsigned long sectiontype, unsigned long *minptr, unsigned long *maxptr)
{
	unsigned long ctr, ctr2, totalcount = 0, thiscount, tracknotecount, sectioncount, min = 0, max = 0;
	EOF_PHRASE_SECTION *phrase = NULL;

	if(!eof_lookup_track_section_type(eof_song, eof_selected_track, sectiontype, &sectioncount, &phrase) || !phrase)
	{	//If there was an error looking up how many of this section type are present in the active track
		sectioncount = 0;	//Ensure this is zero
	}
	tracknotecount = eof_get_track_size(eof_song, eof_selected_track);

	for(ctr = 0; ctr < sectioncount; ctr++)
	{	//For each section of this type in the track
		EOF_PHRASE_SECTION *ptr = &phrase[ctr];	//Get a pointer to this section instance

		thiscount = 0;	//Reset this counter
		for(ctr2 = 0; ctr2 < tracknotecount; ctr2++)
		{	//For each note in the track
			unsigned long notepos = eof_get_note_pos(eof_song, eof_selected_track, ctr2);

			if(notepos > ptr->end_pos)	//If this note and all others are past the end of the section being examined
				break;					//Stop looking for notes in this section

			if(notepos >= ptr->start_pos)
			{	//If this note is not after the section, and starts at or after the section's beginning, it's in the section
				thiscount++;	//Count the number of notes in this section
				totalcount++;	//Count the number of notes in all solos
			}
		}
		if(!min || (thiscount < min))
		{	//If this section has the least number of notes among all examined so far
			min = thiscount;
		}
		if(!max || (thiscount > max))
		{	//If this section has the most notes among all examined so far
			max = thiscount;
		}
	}

	//Return min and max values by reference if applicable
	if(minptr)
		*minptr = min;
	if(maxptr)
		*maxptr = max;

	return totalcount;
}
