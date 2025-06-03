#include <allegro.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif
#include "beat.h"		//For eof_get_measure()
#include "event.h"
#include "ini_import.h"
#include "ir.h"
#include "main.h"
#include "midi.h"
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

char eof_notes_macro_note_occurs_before_millis[50];
char eof_notes_macro_pitched_slide_missing_linknext[50];
char eof_notes_macro_note_starting_on_tone_change[100];
char eof_notes_macro_note_subceeding_fhp[50];
char eof_notes_macro_note_exceeding_fhp[50];
char eof_notes_macro_pitched_slide_missing_end_fret[50];
char eof_notes_macro_bend_missing_strength[50];
char eof_notes_macro_open_note_bend[50];
char eof_notes_macro_tempo_subceeding_number[50];
char eof_notes_macro_fhp_exceeding_number[50];
char eof_notes_macro_note_exceeding_fret[50];
char eof_notes_macro_note_exceeding_diff[50];
char eof_notes_macro_slide_exceeding_fret[50];
char eof_notes_macro_tech_note_missing_target[50];
char eof_notes_macro_lyric_extending_outside_line[50];
char eof_notes_macro_technique_missing_sustain[50];

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

	panel->logged = 0;	//Enable exhaustive logging of this panel's processing for the next frame
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
	char invert = 0;	//If the first character after a % in the macro is an exclamation point, it will invert the true/false result of any conditional check

	if(!src_buffer || !dest_buffer || (dest_buffer_size < 1) || !panel)
		return 0;	//Invalid parameters

	dest_buffer[0] = '\0';	//Empty the destination buffer
	if(src_buffer[0] == ';')	//A line beginning with a semicolon is a comment
		return 1;
	while((src_buffer[src_index] != '\0') || (dest_index + 1 >= dest_buffer_size))
	{	//Until the end of the source or destination buffer are reached
		invert = 0;	//Reset this status
		src_char = src_buffer[src_index++];	//Read the next character

		if(src_char != '%')
		{	//If this isn't a macro
			dest_buffer[dest_index++] = src_char;	//Append it to the destination buffer
			dest_buffer[dest_index] = '\0';			//Terminate the string so that macro text can be concatenated properly
		}
		else if(src_buffer[src_index] != '%')
		{	//If the next character isn't also a percent sign (to be ignored as padding), parse the content up until the next percent sign and process it as a macro to be expanded
			macro_index = 0;
			if(src_buffer[src_index] == '!')
			{	//If the first character after the opening % is an exclamation point
				src_index++;	//Seek to the next character
				invert = 1;	//Invert the boolean result of this macro if it's a conditional macro
			}
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
							if(panel->newline)
							{	//If the newline macro caused the output to be flushed
								panel->xpos = 2;		//Move output coordinates to beginning of the next line
								panel->ypos +=12;
								if(panel->colorprinted)
									panel->ypos += 3;		//Lower the y coordinate further so that one line of color background doesn't obscure another
								panel->newline = 0;
							}
						}
						break;	//Exit macro parse while loop
					}
					else if((macro_status == 2) || (macro_status == 3))
					{	//If this was a conditional macro
						char *ifptr, *ifnptr, *elseptr, *endifptr, *nextptr;		//Used to track conditional macro handling
						char *eraseptr = NULL;	//Used to erase ELSE condition when the condition was true
						unsigned nested_level = 0;				//Track how many conditional if-else-endif macro levels we're currently nested, so we can make the correct parsing branch
						char nextcondition = 0;					//Tracks whether the next conditional macro is an if (1), an else (2), an endif (3) or none of those (0)
						unsigned long index = src_index;			//Used to iterate through the source buffer for processing macros

						if(invert)
						{	//If an exclamation point is inverting the boolean result
							if(macro_status == 2)
								macro_status = 3;	//Change a true result to false
							else
								macro_status = 2;	//Change a false result to true
						}
						if(!panel->logged)
						{
							if(macro_status == 2)
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tFalse%s", invert ? " (negated true)" : "");
							else
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tTrue%s", invert ? " (negated false)" : "");
							eof_log(eof_log_string, 3);
						}
						while(1)
						{	//Process conditional macros until the correct branching is handled
							nextcondition = 0;				//Reset this status so that if no conditional macros are found, the while loop can exit
							nextptr = &src_buffer[index];	//Unless another conditional macro is found, this pointer will not advance
							ifptr = strcasestr_spec(&src_buffer[index], "%IF_");		//Search for the next instance of an if conditional macro
							ifnptr = strcasestr_spec(&src_buffer[index], "%!IF_");		//Search for the next instance of an if-not conditional macro
							if(ifnptr)
							{	//If there is an if-not macro
								if(!ifptr)
								{	//If there is not a normal if macro
									ifptr = ifnptr;	//The if-not macro is the next conditional macro
								}
								else if(ifnptr < ifptr)
								{	//If there is a normal if macro, but the if-not macro appears earlier in the string
									ifptr = ifnptr;	//The if-not macro is the next conditional macro
								}
							}
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
									if(padindex && !panel->logged)
									{
										snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Updated conditional branching:  \"%s\"", src_buffer);
										eof_log(eof_log_string, 3);
									}
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
										if(padindex && !panel->logged)
										{
											snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Updated conditional branching:  \"%s\"", src_buffer);
											eof_log(eof_log_string, 3);
										}
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
						eof_log(dest_buffer, 3);
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
	unsigned long tracknum, tracksize, ctr;
	EOF_PHRASE_SECTION *phraseptr;
	char album_art_filename[1024];
	char *count_string, *name_string, name_buffer[101];

	if(!macro || !dest_buffer || (dest_buffer_size < 1) || !panel)
		return 0;	//Invalid parameters
	if(!eof_song)
		return 0;	//No chart loaded

	if(eof_selected_beat >= eof_song->beats)
		eof_selected_beat = 0;	//If eof_selected_beat had become invalid, select the first beat

	if((eof_log_level > 2) && !panel->logged)
	{	//If exhaustive logging is enabled and this panel hasn't been logged since it was loaded
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tProcessing Notes macro \"%s\"", macro);
		eof_log(eof_log_string, 3);
	}

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tracksize = eof_get_track_size(eof_song, eof_selected_track);

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

	//A specific note/lyric is selected via mouse click
	if(!ustricmp(macro, "IF_NOTE_IS_SELECTED"))
	{
		if(eof_selection.current < tracksize)
		{
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//One or more notes are selected
	if(!ustricmp(macro, "IF_SELECTED_NOTES"))
	{
		if(eof_count_selected_and_unselected_notes(NULL))
		{	//If at least one note is selected
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
		if(eof_selection.current < tracksize)
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
		if(eof_selection.current < tracksize)
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

	//If the active difficulty has a solo section without any notes
	if(!ustricmp(macro, "IF_ACTIVE_DIFFICULTY_HAS_EMPTY_SOLO"))
	{
		unsigned long numsolos, soloctr, notectr, notepos, solonotes;
		long nextnote;
		EOF_PHRASE_SECTION *soloptr;

		//Start by examining the first note in the active track difficulty
		for(notectr = 0; notectr < tracksize; notectr++)
		{
			if(eof_get_note_type(eof_song, eof_selected_track, notectr) == eof_note_type)
				break;
		}

		numsolos = eof_get_num_solos(eof_song, eof_selected_track);
		for(soloctr = 0; soloctr < numsolos; soloctr++)
		{	//For each solo section in the active track
			solonotes = 0;	//Reset this counter
			soloptr = eof_get_solo(eof_song, eof_selected_track, soloctr);
			if(soloptr)
			{	//If the solo was properly found
				if(notectr >= tracksize)
				{	//If there are no more notes in the active track difficulty, this solo is empty
					dest_buffer[0] = '\0';
					return 3;	//True
				}

				notepos = eof_get_note_pos(eof_song, eof_selected_track, notectr);
				while(notepos <= soloptr->end_pos)
				{	//For all notes at/before the end of this solo
					if(notepos >= soloptr->start_pos)
					{	//If the note is within the scope of this solo, this solo isn't empty
						solonotes++;
						break;	//Exit while loop to examine next solo
					}
					nextnote = eof_track_fixup_next_note(eof_song, eof_selected_track, notectr);
					if(nextnote < 0)
					{	//If there is no next note in the active difficulty
						notectr = ULONG_MAX;	//Set a condition to indicate all notes were exhausted
						break;
					}
					notectr = nextnote;
					notepos = eof_get_note_pos(eof_song, eof_selected_track, notectr);
				}
				if(!solonotes)
				{	//If there were no notes in this solo
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		//No solos were found to be empty
		return 2;	//False
	}

	//If the active difficulty has no cymbal notes
	if(!ustricmp(macro, "IF_ACTIVE_DIFFICULTY_HAS_NO_CYMBALS"))
	{
		unsigned long i;

		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If this is a drum track
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the track
				if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
				{	//If the note is in the active difficulty
					if(eof_note_is_cymbal(eof_song, eof_selected_track, i))
					{	//If this note contains a yellow, blue or green cymbal
						return 2;	//False
					}
				}
			}
		}

		//No cymbals were found
		dest_buffer[0] = '\0';
		return 3;	//True
	}

	//If the active track has any solos with less than 1 second of space between them
	if(!ustricmp(macro, "IF_ANY_SOLOS_CLOSER_THAN_1_SECOND"))
	{
		unsigned long numsolos, soloctr;
		EOF_PHRASE_SECTION *solo1, *solo2;
		int too_close = 0;

		numsolos = eof_get_num_solos(eof_song, eof_selected_track);
		if(numsolos < 2)
			return 2;	//False

		for(soloctr = 0; soloctr + 1 < numsolos; soloctr++)
		{	//For each solo section in the active track, up until the penultimate one
			solo1 = eof_get_solo(eof_song, eof_selected_track, soloctr);
			solo2 = eof_get_solo(eof_song, eof_selected_track, soloctr + 1);
			if(solo1 && solo2)
			{	//If this solo and the next solo were identified
				if((solo1->end_pos < solo2->start_pos) && (solo1->end_pos + 1000 > solo2->start_pos))
				{	//If the first solo ends before the second, but less than one second away
					too_close = 1;
				}
				else if((solo2->end_pos < solo1->start_pos) && (solo2->end_pos + 1000 > solo1->start_pos))
				{	//If the second solo ends before the first, but less than one second away
					too_close = 1;
				}
				if(too_close)
				{
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		return 2;	//False
	}

	//If the active difficulty has notes with a specific gem count
	count_string = strcasestr_spec(macro, "IF_TRACK_DIFF_HAS_NOTES_WITH_GEM_COUNT_");	//Get a pointer to the text that would be the gem count
	if(count_string)
	{	//If the macro is this string
		unsigned long gemcount;

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
			for(ctr = 0; ctr < tracksize; ctr++)
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

	if(!ustricmp(macro, "IF_TRACK_HAS_NO_SLIDERS"))
	{
		if(!eof_get_num_sliders(eof_song, eof_selected_track))
		{	//If there are no sliders in the active track
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the active drum track has no expert+ bass notes
	if(!ustricmp(macro, "IF_TRACK_HAS_NO_EXPERT_PLUS_BASS"))
	{
		unsigned long i;

		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If this is a drum track
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the track
				if((eof_get_note_type(eof_song, eof_selected_track, i) == EOF_NOTE_AMAZING) && (eof_get_note_note(eof_song, eof_selected_track, i) & 1))
				{	//If the note is in the expert difficulty and has a gem on lane 1 (bass drum)
					if(eof_get_note_flags(eof_song, eof_selected_track, i) & EOF_DRUM_NOTE_FLAG_DBASS)
					{	//If the note has the expert+ double bass status
						return 2;	//False
					}
				}
			}
		}

		//No expert+ bass notes were found
		dest_buffer[0] = '\0';
		return 3;	//True
	}

	//If the active track has no defined difficulty
	if(!ustricmp(macro, "IF_TRACK_DIFFICULTY_UNDEFINED"))
	{
		if(eof_song->track[eof_selected_track]->difficulty == 0xFF)
		{	//If the difficulty level of this track is not defined
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	//If the active track has no defined difficulty
	if(!ustricmp(macro, "IF_ANY_TRACK_DIFFICULTY_UNDEFINED"))
	{
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			if(eof_get_track_size_all(eof_song, ctr))
			{	//If the track has any normal or tech notes
				if(eof_song->track[ctr]->difficulty == 0xFF)
				{	//If the difficulty level of the track is not defined
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		return 2;	//False
	}

	//If the active track has at least one note
	if(!ustricmp(macro, "IF_TRACK_IS_POPULATED"))
	{
		if(eof_get_track_size_all(eof_song, eof_selected_track))
		{	//If the active track has any normal or tech notes
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
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
		unsigned long entrynum = 0;
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
		eof_ch_sp_solution_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score && eof_ch_sp_solution->num_deployments)
		{	//If the global star power solution structure is built, the score is nonzero and there is at least one defined SP deployment
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CH_SP_DEPLOYMENTS_MISSING"))
	{
		eof_ch_sp_solution_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score && !eof_ch_sp_solution->num_deployments)
		{	//If the global star power solution structure is built, the score is nonzero but there are no defined SP deployments
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CH_SP_PATH_INVALID"))
	{
		eof_ch_sp_solution_wanted = 1;
		if(eof_ch_sp_solution && !eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built but its score is zero
			if(eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT)
			{	//If the active track's format is supported by the pathing logic
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CH_SP_STATS_AVAILABLE"))
	{
		if(eof_ch_sp_solution && eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built and the score is nonzero
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_GRID_SNAP_ENABLED"))
	{
		if(eof_snap_mode != EOF_SNAP_OFF)
		{	//If grid snap is enabled
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_TRACK_HAS_DD"))
	{
		if(eof_track_has_dynamic_difficulty(eof_song, eof_selected_track))
		{	//If the active track has the difficulty limit removed
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_FLAT_DD_VIEW_ACTIVE"))
	{
		if(eof_flat_dd_view)
		{	//If the flat DD view feature is in effect
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_PROJECT_HAS_IR_ALBUM_ART"))
	{
		album_art_filename[0] = '\0';	//Empth this string
		if(eof_check_for_immerrock_album_art(eof_song_path, album_art_filename, sizeof(album_art_filename) - 1, 0))
		{	//If the project folder has any suitably named album art for use with IMMERROCK
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ANY_LYRICS_ARE_OUTSIDE_LINES"))
	{
		for(ctr = 0; eof_song && (ctr < eof_song->vocal_track[0]->lyrics); ctr++)
		{	//For each lyric
			if((eof_song->vocal_track[0]->lyric[ctr]->note == EOF_LYRIC_PERCUSSION) || (eof_find_lyric_line(ctr) != NULL))
				continue;	//If this lyric is vocal percussion or is within a line, skip it

			dest_buffer[0] = '\0';
			return 3;	//True
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_SONG_TITLE_IS_DEFINED"))
	{
		if(eof_song && eof_check_string(eof_song->tags->title))
		{	//If the song title string has anything other than whitespace
			dest_buffer[0] = '\0';
			return 3;	//True
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ARTIST_NAME_IS_DEFINED"))
	{
		if(eof_song && eof_check_string(eof_song->tags->artist))
		{	//If the artist name string has anything other than whitespace
			dest_buffer[0] = '\0';
			return 3;	//True
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CHART_AUTHOR_IS_DEFINED"))
	{
		if(eof_song && eof_check_string(eof_song->tags->frettist))
		{	//If the charter name string has anything other than whitespace
			dest_buffer[0] = '\0';
			return 3;	//True
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ALBUM_NAME_IS_DEFINED"))
	{
		if(eof_song && eof_check_string(eof_song->tags->album))
		{	//If the album name string has anything other than whitespace
			dest_buffer[0] = '\0';
			return 3;	//True
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_GENRE_IS_DEFINED"))
	{
		if(eof_song && eof_check_string(eof_song->tags->genre))
		{	//If the genre string has anything other than whitespace
			dest_buffer[0] = '\0';
			return 3;	//True
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_YEAR_IS_DEFINED"))
	{
		if(eof_song && eof_check_string(eof_song->tags->year))
		{	//If the year string has anything other than whitespace
			dest_buffer[0] = '\0';
			return 3;	//True
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ANY_LYRIC_LINES_ARE_DEFINED"))
	{
		if(eof_song &&  eof_song->vocal_track[0]->lines)
		{	//If at least one lyric line exists
			dest_buffer[0] = '\0';
			return 3;	//True
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ANY_LYRICS_ARE_NON_ASCII"))
	{
		for(ctr = 0; eof_song && (ctr < eof_song->vocal_track[0]->lyrics); ctr++)
		{	//For each lyric
			if((eof_song->vocal_track[0]->lyric[ctr]->text[0] != '\0') && (eof_string_has_non_ascii(eof_song->vocal_track[0]->lyric[ctr]->text)))
			{	//If any of the lyrics that contain text have non ASCII characters
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_IR_LEAD_ARRANGEMENT_DEFINED"))
	{
		unsigned long lead = 0, rhythm = 0, bass = 0;
		if(eof_detect_immerrock_arrangements(&lead, &rhythm, &bass))
		{	//If the arrangement designations were determined
			if(lead)
			{
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ACTIVE_TRACK_IS_IR_LEAD_ARRANGEMENT"))
	{
		unsigned long lead = 0, rhythm = 0, bass = 0;
		if(eof_detect_immerrock_arrangements(&lead, &rhythm, &bass))
		{	//If the arrangement designations were determined
			if(lead == eof_selected_track)
			{
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_IR_RHYTHM_ARRANGEMENT_DEFINED"))
	{
		unsigned long lead = 0, rhythm = 0, bass = 0;
		if(eof_detect_immerrock_arrangements(&lead, &rhythm, &bass))
		{	//If the arrangement designations were determined
			if(rhythm)
			{
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ACTIVE_TRACK_IS_IR_RHYTHM_ARRANGEMENT"))
	{
		unsigned long lead = 0, rhythm = 0, bass = 0;
		if(eof_detect_immerrock_arrangements(&lead, &rhythm, &bass))
		{	//If the arrangement designations were determined
			if(rhythm == eof_selected_track)
			{
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_IR_BASS_ARRANGEMENT_DEFINED"))
	{
		unsigned long lead = 0, rhythm = 0, bass = 0;
		if(eof_detect_immerrock_arrangements(&lead, &rhythm, &bass))
		{	//If the arrangement designations were determined
			if(bass)
			{
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ACTIVE_TRACK_IS_IR_BASS_ARRANGEMENT"))
	{
		unsigned long lead = 0, rhythm = 0, bass = 0;
		if(eof_detect_immerrock_arrangements(&lead, &rhythm, &bass))
		{	//If the arrangement designations were determined
			if(bass == eof_selected_track)
			{
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_IR_SECTIONS_DEFINED"))
	{
		if(eof_count_immerrock_sections())
		{	//If at least one section event is found
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_SECTIONS_DEFINED"))
	{
		for(ctr = 0; ctr < eof_song->beats; ctr++)
		{	//For each beat in the chart
			if(eof_song->beat[ctr]->contained_rs_section_event >= 0)
			{	//If this beat has a Rocksmith section
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_IR_TRACK_HAS_CHORDS_MISSING_FINGERING"))
	{
		if(eof_count_immerrock_chords_missing_fingering(NULL))
		{	//If at least one chord is missing finger definitions
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_SEEK_POS_IS_IN_IR_SECTION"))
	{
		char section[50];

		if(eof_lookup_immerrock_effective_section_at_pos(eof_song, eof_music_pos.value - eof_av_delay, section, sizeof(section)))
		{	//If a section name was found to be in effect at the seek position
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_SEEK_POS_IS_IN_RS_SECTION"))
	{
		char section[50];

		if(eof_lookup_rocksmith_effective_section_at_pos(eof_song, eof_music_pos.value - eof_av_delay, section, sizeof(section)))
		{	//If a section name was found to be in effect at the seek position
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_IR_EXPORT_ENABLED"))
	{
		if(eof_write_immerrock_files)
		{	//If the "Save separate IMMERROCK files" export preference is enabled
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_EXPORT_ENABLED"))
	{
		if(eof_write_rs_files || eof_write_rs2_files)
		{	//If either the "Save separate Rocksmith 1 files" or "Save separate Rocksmith 2 files" export preferences are enabled
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS1_EXPORT_ENABLED"))
	{
		if(eof_write_rs_files)
		{	//If the "Save separate Rocksmith 1 files" export preference is enabled
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS2_EXPORT_ENABLED"))
	{
		if(eof_write_rs2_files)
		{	//If "Save separate Rocksmith 2 files" export preference is enabled
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	count_string = strcasestr_spec(macro, "IF_PG_NOTE_OCCURS_BEFORE_MILLIS_");	//Get a pointer to the text that would be the millisecond count
	if(count_string)
	{	//If the macro is this string
		unsigned long mscount;
		EOF_PRO_GUITAR_TRACK *tp;

		eof_notes_macro_note_occurs_before_millis[0] = '\0';	//Erase this string
		if(eof_read_macro_number(count_string, &mscount))
		{	//If the millisecond count was successfully parsed
			for(ctr = 1; ctr < eof_song->tracks; ctr++)
			{	//For each track in the project
				if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
					continue;	//Skip non pro guitar tracks

				tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
				if(tp->pgnotes)
				{	//If there's at least one normal note in the track
					if(tp->pgnote[0]->pos < mscount)
					{	//If the first normal note begins before the specified time
						char time_string[15] = {0};
						eof_notes_panel_print_time(tp->pgnote[0]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
						snprintf(eof_notes_macro_note_occurs_before_millis, sizeof(eof_notes_macro_note_occurs_before_millis) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[0]->type, time_string);	//Write a string identifying the offending note
						dest_buffer[0] = '\0';
						return 3;	//True
					}
				}
			}
		}

		return 2;	//False
	}

	//Test this before "IF_RS_PHRASE_DEFINED_", because the latter is a substring and would be matched
	name_string = strcasestr_spec(macro, "IF_RS_PHRASE_DEFINED_MULTIPLE_TIMES_");	//Get a pointer to the text that would be the phrase name
	if(name_string)
	{	//If the macro is this string
		char match = 0;
		unsigned long lastmatchpos = 0, thismatchpos;

		if(eof_read_macro_string(name_string, name_buffer))
		{	//If the phrase name was successfully parsed
			for(ctr = 0; ctr < eof_song->text_events; ctr++)
			{	//For each event in the project
				if(!eof_song->text_event[ctr]->track || (eof_song->text_event[ctr]->track == eof_selected_track))
				{	//If the event applies to either all tracks or the active track
					if(eof_song->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_PHRASE)
					{	//If the event is a Rocksmith phrase
						if(!ustrcmp(eof_song->text_event[ctr]->text, name_buffer))
						{	//If the event text matches the text in the macro
							thismatchpos = eof_get_text_event_pos(eof_song, ctr);	//Get the position of this event in milliseconds
							if(match)
							{	//If this isn't the first matching event
								if(lastmatchpos != thismatchpos)
								{	//If the previous match was at a different timestamp
									dest_buffer[0] = '\0';
									return 3;	//True
								}
							}
							match = 1;
							lastmatchpos = thismatchpos;
						}
					}
				}
			}
		}

		return 2;	//False
	}

	name_string = strcasestr_spec(macro, "IF_RS_PHRASE_DEFINED_");	//Get a pointer to the text that would be the phrase name
	if(name_string)
	{	//If the macro is this string
		if(eof_read_macro_string(name_string, name_buffer))
		{	//If the phrase name was successfully parsed
			if(eof_song_contains_event(eof_song, name_buffer, eof_selected_track, EOF_EVENT_FLAG_RS_PHRASE, 2))
			{	//If the phrase is defined in an event specific to the active track or to the entire project
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	name_string = strcasestr_spec(macro, "IF_RS_SECTION_DEFINED_");	//Get a pointer to the text that would be the section name
	if(name_string)
	{	//If the macro is this string
		if(eof_read_macro_string(name_string, name_buffer))
		{	//If the phrase name was successfully parsed
			if(eof_song_contains_event(eof_song, name_buffer, eof_selected_track, EOF_EVENT_FLAG_RS_SECTION, 2))
			{	//If the phrase is defined in an event specific to the active track or to the entire project
				dest_buffer[0] = '\0';
				return 3;	//True
			}
		}

		return 2;	//False
	}

	name_string = strcasestr_spec(macro, "IF_RS_NOTES_EXIST_BEFORE_PHRASE_");	//Get a pointer to the text that would be the phrase name
	if(name_string)
	{	//If the macro is this string
		if(eof_read_macro_string(name_string, name_buffer))
		{	//If the phrase name was successfully parsed
			unsigned long match = eof_song_lookup_first_event(eof_song, name_buffer, eof_selected_track, EOF_EVENT_FLAG_RS_PHRASE, 2);
			if(match < eof_song->text_events)
			{	//If there was an instance of the specified phrase applicable to the active track (specifically or project-wide)
				unsigned long matchpos = eof_get_text_event_pos(eof_song, match);	//Get the position of this event in milliseconds
				EOF_PRO_GUITAR_TRACK *tp;

				for(ctr = 1; ctr < eof_song->tracks; ctr++)
				{	//For each track in the project
					if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
						continue;	//Skip non pro guitar tracks

					tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
					if(tp->pgnotes)
					{	//If there's at least normal one note in the track
						if(tp->pgnote[0]->pos < matchpos)
						{	//If the first normal note begins before the specified time
							dest_buffer[0] = '\0';
							return 3;	//True
						}
					}
				}
			}
		}

		return 2;	//False
	}

	name_string = strcasestr_spec(macro, "IF_RS_NOTES_AT_OR_AFTER_PHRASE_");	//Get a pointer to the text that would be the phrase name
	if(name_string)
	{	//If the macro is this string
		if(eof_read_macro_string(name_string, name_buffer))
		{	//If the phrase name was successfully parsed
			for(ctr = 1; ctr < eof_song->tracks; ctr++)
			{	//For each track in the project
				unsigned long ctr2, match;

				if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
					continue;	//Skip non pro guitar tracks

				match = eof_song_lookup_first_event(eof_song, name_buffer, ctr, EOF_EVENT_FLAG_RS_PHRASE, 2);
				if(match < eof_song->text_events)
				{	//If there was an instance of the specified phrase applicable to the track (specifically or project-wide)
					unsigned long matchpos = eof_get_text_event_pos(eof_song, match);	//Get the position of this event in milliseconds
					EOF_PRO_GUITAR_TRACK *tp;

					tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
					for(ctr2 = tp->pgnotes; ctr2 > 0; ctr2--)
					{	//For each normal note in the track, in reverse order for better efficiency
						if(tp->pgnote[ctr2 - 1]->pos + tp->pgnote[ctr2 - 1]->length >= matchpos)
						{	//If the note ends at or after the specified time
							dest_buffer[0] = '\0';
							return 3;	//True
						}
					}
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_MIN_NOTE_DISTANCE_NOT_BEAT_OR_MEASURE_BASED"))
	{
		if(!eof_min_note_distance_intervals || !eof_min_note_distance)
		{	//If the minimum note distance is not in beat or measure intervals, or the interval number is zero
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_ANY_PITCHED_SLIDES_LACK_LINKNEXT"))
	{
		unsigned long stringnum, notectr, bitmask;
		EOF_PRO_GUITAR_TRACK *tp;
		EOF_RS_TECHNIQUES tech = {0};

		eof_notes_macro_pitched_slide_missing_linknext[0] = '\0';	//Erase this string
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//Skip non pro guitar tracks

			tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
			for(notectr = 0;  notectr < tp->pgnotes; notectr++)
			{	//For each normal note in the track
				for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
				{	//For each string used in this track
					if(tp->pgnote[notectr]->note & bitmask)
					{	//If this string is used by the note
						(void) eof_get_rs_techniques(eof_song, ctr, notectr, stringnum, &tech, 2, 1);		//Determine techniques used by this note (including applicable technotes using this string)
						if((tech.slideto > 0) && !tech.linknext)
						{	//If this string of the note slides to a fret, but does not have linknext status
							char time_string[15] = {0};
							eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
							snprintf(eof_notes_macro_pitched_slide_missing_linknext, sizeof(eof_notes_macro_pitched_slide_missing_linknext) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
							dest_buffer[0] = '\0';
							return 3;	//True
						}
					}
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_ANY_TONE_CHANGES_ON_NOTE"))
	{
		unsigned long tonectr, notectr;
		EOF_PRO_GUITAR_TRACK *tp;

		eof_notes_macro_note_starting_on_tone_change[0] = '\0';	//Erase this string
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//Skip non pro guitar tracks

			tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
			for(tonectr = 0; tonectr < tp->tonechanges; tonectr++)
			{	//For each tone change in this track
				for(notectr = 0; notectr < tp->pgnotes; notectr++)
				{	//For each normal note in this track
					if(tp->pgnote[notectr]->pos > tp->tonechange[tonectr].start_pos)
						break;	//If this and all remaining notes occur after the tone change, stop checking notes for this track

					if(tp->pgnote[notectr]->pos == tp->tonechange[tonectr].start_pos)
					{	//If this note starts exactly at this tone change's timestamp
						char time_string[15] = {0};
						eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
						snprintf(eof_notes_macro_note_starting_on_tone_change, sizeof(eof_notes_macro_note_starting_on_tone_change) - 1, "%s -diff %u: pos %s (%s)", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string, tp->tonechange[tonectr].name);	//Write a string identifying the offending note
						dest_buffer[0] = '\0';
						return 3;	//True
					}
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_NOTES_TO_SOON_AFTER_COUNT_PHRASE"))
	{
		unsigned long match = eof_song_lookup_first_event(eof_song, "COUNT", eof_selected_track, EOF_EVENT_FLAG_RS_PHRASE, 2);
		if(match < eof_song->text_events)
		{	//If there was an instance of the COUNT phrase applicable to the active track (specifically or project-wide)
			unsigned num = 0, den = 0;
			EOF_PRO_GUITAR_TRACK *tp;
			unsigned long matchbeat = eof_get_text_event_beat(eof_song, match);	//Get the beat containing the event

			if(eof_get_effective_ts(eof_song, &num, &den, matchbeat, 0))
			{	//If the time signature of the beat containing the event could be determined
				if(matchbeat + num < eof_song->beats)
				{	//If the beat that would be one measure further into the chart exists
					unsigned long targetpos = eof_song->beat[matchbeat + num]->pos;	//The position of that beat

					for(ctr = 1; ctr < eof_song->tracks; ctr++)
					{	//For each track in the project
						if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
							continue;	//Skip non pro guitar tracks

						tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
						if(tp->pgnotes)
						{	//If there's at least one normal note in the track
							if(tp->pgnote[0]->pos < targetpos)
							{	//If the first note begins before the specified time
								dest_buffer[0] = '\0';
								return 3;	//True
							}
						}
					}
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_ONLY_ONE_TONE_CHANGE"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If the active track is a pro guitar track
			eof_track_rebuild_rs_tone_names_list_strings(eof_selected_track, 1);
			if(eof_track_rs_tone_names_list_strings_num == 1)
			{	//If only one tone name is used
				eof_track_destroy_rs_tone_names_list_strings();
				dest_buffer[0] = '\0';
				return 3;	//True
			}
			eof_track_destroy_rs_tone_names_list_strings();
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_NO_DEFAULT_TONE_DEFINED"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If the active track is a pro guitar track
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];	//Simplify
			eof_track_rebuild_rs_tone_names_list_strings(eof_selected_track, 1);
			if(eof_track_rs_tone_names_list_strings_num > 1)
			{	//If multiple tones are used
				if(tp->defaulttone[0] == '\0')
				{
					eof_track_destroy_rs_tone_names_list_strings();
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
			eof_track_destroy_rs_tone_names_list_strings();
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_TRACK_DIFF_HAS_NO_FHPS"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If the active track is a pro guitar track
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];	//Simplify

			for(ctr = 0; ctr < tp->handpositions; ctr++)
			{	//For each fret hand position defined in this track
				if(tp->handposition[ctr].difficulty == eof_note_type)
				{	//If the fret hand position is in the active track difficulty
					return 2;	//False
				}
			}
		}

		dest_buffer[0] = '\0';
		return 3;	//True
	}

	if(!ustricmp(macro, "IF_RS_ANY_NOTE_SUBCEEDS_FHP"))
	{
		EOF_PRO_GUITAR_TRACK *tp;
		unsigned char lowestfret, fhp;
		unsigned long ctr2;

		eof_notes_macro_note_subceeding_fhp[0] = '\0';	//Erase this string
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//Skip non pro guitar tracks

			tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
			for(ctr2 = 0; ctr2 < tp->pgnotes; ctr2++)
			{	//For each normal note in the track
				lowestfret = eof_pro_guitar_note_lowest_fret(tp, ctr2);	//Find the lowest used fret for the note
				fhp = eof_pro_guitar_track_find_effective_fret_hand_position(tp, tp->pgnote[ctr2]->type, tp->pgnote[ctr2]->pos);	//Find if there's a fret hand position in effect for the note
				if(lowestfret && fhp && (lowestfret < fhp))
				{	//If there is a fretted string for this note, and it is below the defined FHP (if any)
					char time_string[15] = {0};
					eof_notes_panel_print_time(tp->pgnote[ctr2]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
					snprintf(eof_notes_macro_note_subceeding_fhp, sizeof(eof_notes_macro_note_subceeding_fhp) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[ctr2]->type, time_string);	//Write a string identifying the offending note
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		return 2;	//False
	}

	count_string = strcasestr_spec(macro, "IF_RS_ANY_NON_TAP_NOTE_EXCEEDS_FHP_BY_");	//Get a pointer to the text that would be the fret count
	if(count_string)
	{	//If the macro is this string
		EOF_PRO_GUITAR_TRACK *tp;
		unsigned char highestfret, fhp;
		unsigned long ctr2, fretcount;

		eof_notes_macro_note_exceeding_fhp[0] = '\0';	//Erase this string
		if(eof_read_macro_number(count_string, &fretcount))
		{	//If the millisecond count was successfully parsed
			for(ctr = 1; ctr < eof_song->tracks; ctr++)
			{	//For each track in the project
				if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
					continue;	//Skip non pro guitar tracks

				tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
				for(ctr2 = 0; ctr2 < tp->pgnotes; ctr2++)
				{	//For each normal note in the track
					if(!(tp->pgnote[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP))
					{	//If this isn't a note with tap status
						highestfret = eof_pro_guitar_note_highest_fret(tp, ctr2);	//Find the highest used fret for the note
						fhp = eof_pro_guitar_track_find_effective_fret_hand_position(tp, tp->pgnote[ctr2]->type, tp->pgnote[ctr2]->pos);	//Find if there's a fret hand position in effect for the note
						if(highestfret && fhp && (highestfret > fhp + fretcount))
						{	//If there is a fretted string for this note, and it is exceeds the given the defined FHP (if any) by the given amount
							char time_string[15] = {0};
							eof_notes_panel_print_time(tp->pgnote[ctr2]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
							snprintf(eof_notes_macro_note_exceeding_fhp, sizeof(eof_notes_macro_note_exceeding_fhp) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[ctr2]->type, time_string);	//Write a string identifying the offending note
							dest_buffer[0] = '\0';
							return 3;	//True
						}
					}
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_ANY_PITCHED_SLIDES_LACK_END_FRET"))
	{
		unsigned long stringnum, notectr, bitmask;
		EOF_PRO_GUITAR_TRACK *tp;
		EOF_RS_TECHNIQUES tech = {0};

		eof_notes_macro_pitched_slide_missing_end_fret[0] = '\0';	//Erase this string
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//Skip non pro guitar tracks

			tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
			for(notectr = 0;  notectr < tp->pgnotes; notectr++)
			{	//For each normal note in the track
				for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
				{	//For each string used in this track
					if(tp->pgnote[notectr]->note & bitmask)
					{	//If this string is used by the note
						//Determine techniques used by this note (including applicable technotes using this string), do NOT assume a slide end fret if none is defined
						unsigned long retflags = eof_get_rs_techniques(eof_song, ctr, notectr, stringnum, &tech, 4, 1);
						if(retflags & (EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
						{	//If the note uses slide technique on this string
							if(tech.slideto < 0)
							{	//If no end of slide position is actually defined
								char time_string[15] = {0};
								eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
								snprintf(eof_notes_macro_pitched_slide_missing_end_fret, sizeof(eof_notes_macro_pitched_slide_missing_end_fret) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
								dest_buffer[0] = '\0';
								return 3;	//True
							}
						}
					}
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_ANY_BENDS_LACK_STRENGTH_DEFINITION"))
	{
		unsigned long stringnum, notectr, bitmask;
		EOF_PRO_GUITAR_TRACK *tp;
		EOF_RS_TECHNIQUES tech = {0};

		eof_notes_macro_bend_missing_strength[0] = '\0';	//Erase this string
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct note set can be examined

			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//Skip non pro guitar tracks

			restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, ctr);
			eof_menu_track_set_tech_view_state(eof_song, ctr, 0);	//Disable tech view if applicable
			tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
			for(notectr = 0;  notectr < tp->pgnotes; notectr++)
			{	//For each normal note in the track
				for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
				{	//For each string used in this track
					if(tp->pgnote[notectr]->note & bitmask)
					{	//If this string is used by the note
						if(tp->pgnote[notectr]->frets[stringnum])
						{	//If the string is not an open note
							//Determine techniques used by this note (including applicable technotes using this string)
							unsigned long retflags = eof_get_rs_techniques(eof_song, ctr, notectr, stringnum, &tech, 4, 1);
							if(retflags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
							{	//If the note uses bend technique on this string
								if(tech.bend == 0)
								{	//If no bend strength is actually defined
									char time_string[15] = {0};
									eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
									snprintf(eof_notes_macro_bend_missing_strength, sizeof(eof_notes_macro_bend_missing_strength) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
									eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
									dest_buffer[0] = '\0';
									return 3;	//True
								}
							}
						}
					}
				}
			}
			eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_ANY_OPEN_NOTES_BEND"))
	{
		unsigned long stringnum, notectr, bitmask;
		EOF_PRO_GUITAR_TRACK *tp;
		EOF_RS_TECHNIQUES tech = {0};

		eof_notes_macro_open_note_bend[0] = '\0';	//Erase this string
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct note set can be examined

			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//Skip non pro guitar tracks

			restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, ctr);
			eof_menu_track_set_tech_view_state(eof_song, ctr, 0);	//Disable tech view if applicable
			tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
			for(notectr = 0;  notectr < tp->pgnotes; notectr++)
			{	//For each normal note in the track
				for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
				{	//For each string used in this track
					if(tp->pgnote[notectr]->note & bitmask)
					{	//If this string is used by the note
						if(tp->pgnote[notectr]->frets[stringnum] == 0)
						{	//If the string is an open note
							//Determine techniques used by this note (including applicable technotes using this string)
							unsigned long retflags = eof_get_rs_techniques(eof_song, ctr, notectr, stringnum, &tech, 4, 1);
							if(retflags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
							{	//If the note uses bend technique on this string
								char time_string[15] = {0};
								eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
								snprintf(eof_notes_macro_open_note_bend, sizeof(eof_notes_macro_open_note_bend) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
								eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
								dest_buffer[0] = '\0';
								return 3;	//True
							}
						}
					}
				}
			}
			eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_LEFT_MOUSE_BUTTON_HELD"))
	{
		if(!eof_lclick_released)
		{	//If the left button is not in a released state
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_MOUSE_CONSTRAINED"))
	{
		if(eof_mouse_bound)
		{	//If the mouse coordinates are constrained
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_SELECTED_BEAT_HAS_KEY_SIGNATURE"))
	{
		if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_KEY_SIG)
		{	//If the selected beat has a defined key signature
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_FRET_VALUE_SHORTCUTS_LIMITED"))
	{
		if(eof_pro_guitar_fret_bitmask != 63)
		{	//If the user has reduced the fret value shortcut bitmask from the default of all strings
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	count_string = strcasestr_spec(macro, "IF_RS_SECTION_COUNT_REACHES_");	//Get a pointer to the text that would be the RS section count
	if(count_string)
	{	//If the macro is this string
		if(eof_song)
		{
			unsigned long target_count = 0, count = 0;

			if(eof_read_macro_number(count_string, &target_count))
			{	//If the RS section count was successfully parsed
				for(ctr = 0; ctr < eof_song->beats; ctr++)
				{	//For each beat in the chart
					if(eof_song->beat[ctr]->contained_rs_section_event >= 0)
					{	//If this beat has a Rocksmith section
						count++;	//Update Rocksmith section instance counter
					}
				}

				if(count >= target_count)
				{	//If there are at least as many RS sections as were being tested for
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		return 2;	//False
	}

	count_string = strcasestr_spec(macro, "IF_ANY_TEMPO_SUBCEEDS_");	//Get a pointer to the text that would be the tempo (integer)
	if(count_string)
	{	//If the macro is this string
		unsigned long tempo;
		double thistempo;

		eof_notes_macro_tempo_subceeding_number[0] = '\0';	//Erase this string
		if(eof_read_macro_number(count_string, &tempo))
		{	//If the tempo was successfully parsed
			for(ctr = 0; ctr < eof_song->beats; ctr++)
			{	//For each beat in the project
				thistempo = 60000000.0 / (double)eof_song->beat[ctr]->ppqn;
				if((unsigned long)(thistempo + DBL_EPSILON) < tempo)
				{	//Accounting for floating point precision limitations, if the tempo on this beat is lower than the target
					char time_string[15] = {0};
					eof_notes_panel_print_time(eof_song->beat[ctr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
					snprintf(eof_notes_macro_tempo_subceeding_number, sizeof(eof_notes_macro_tempo_subceeding_number) - 1, "Beat #%lu : pos %s : %.2fBPM", ctr, time_string, thistempo);	//Write a string identifying the offending beat
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		return 2;	//False
	}

	count_string = strcasestr_spec(macro, "IF_ANY_FHP_EXCEEDS_");	//Get a pointer to the text that would be the FHP value
	if(count_string)
	{	//If the macro is this string
		EOF_PRO_GUITAR_TRACK *tp;
		unsigned long target_fhp = 0, ctr2;

		eof_notes_macro_fhp_exceeding_number[0] = '\0';	//Erase this string
		if(eof_read_macro_number(count_string, &target_fhp))
		{	//If the fhp was successfully parsed
			for(ctr = 1; ctr < eof_song->tracks; ctr++)
			{	//For each track in the project
				if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
					continue;	//Skip non pro guitar tracks

				tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
				for(ctr2 = 0; ctr2 < tp->handpositions; ctr2++)
				{	//For each fret hand position in the track
					if(tp->handposition[ctr2].end_pos + tp->capo > target_fhp)
					{	//If the target FHP is surpassed
							char time_string[15] = {0};
							eof_notes_panel_print_time(tp->handposition[ctr2].start_pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
							snprintf(eof_notes_macro_fhp_exceeding_number, sizeof(eof_notes_macro_fhp_exceeding_number) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->handposition[ctr2].difficulty, time_string);	//Write a string identifying the offending FHP
							dest_buffer[0] = '\0';
							return 3;	//True
						}
					}
				}
			}

		return 2;	//False
	}

	count_string = strcasestr_spec(macro, "IF_RS_ANY_NOTE_EXCEEDS_FRET_");	//Get a pointer to the text that would be the fret value
	if(count_string)
	{	//If the macro is this string
		unsigned long notectr, target_fret = 0;
		EOF_PRO_GUITAR_TRACK *tp;

		eof_notes_macro_note_exceeding_fret[0] = '\0';	//Erase this string
		if(eof_read_macro_number(count_string, &target_fret))
		{	//If the fret value was successfully parsed
			for(ctr = 1; ctr < eof_song->tracks; ctr++)
			{	//For each track in the project
				if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
					continue;	//Skip non pro guitar tracks

				tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
				for(notectr = 0;  notectr < tp->pgnotes; notectr++)
				{	//For each normal note in the track
					if(eof_get_pro_guitar_note_highest_fret_value(tp->pgnote[notectr]) > target_fret)
					{	//If this note uses a fret higher than the target
						char time_string[15] = {0};
						eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
						snprintf(eof_notes_macro_note_exceeding_fret, sizeof(eof_notes_macro_note_exceeding_fret) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
						dest_buffer[0] = '\0';
						return 3;	//True
					}
				}
			}
		}

		return 2;	//False
	}

	count_string = strcasestr_spec(macro, "IF_RS_ANY_NOTE_EXCEEDS_DIFF_");	//Get a pointer to the text that would be the difficulty number
	if(count_string)
	{	//If the macro is this string
		unsigned long notectr, target_diff = 0;
		EOF_PRO_GUITAR_TRACK *tp;

		eof_notes_macro_note_exceeding_diff[0] = '\0';	//Erase this string
		if(eof_read_macro_number(count_string, &target_diff))
		{	//If the difficulty number was successfully parsed
			for(ctr = 1; ctr < eof_song->tracks; ctr++)
			{	//For each track in the project
				if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
					continue;	//Skip non pro guitar tracks

				tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
				for(notectr = 0;  notectr < tp->pgnotes; notectr++)
				{	//For each normal note in the track
					if(tp->pgnote[notectr]->type > target_diff)
					{	//If this note uses a difficulty higher than the target
						char time_string[15] = {0};
						eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
						snprintf(eof_notes_macro_note_exceeding_diff, sizeof(eof_notes_macro_note_exceeding_diff) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
						dest_buffer[0] = '\0';
						return 3;	//True
					}
				}
			}
		}

		return 2;	//False
	}

	count_string = strcasestr_spec(macro, "IF_RS_ANY_SLIDES_EXCEED_FRET_");	//Get a pointer to the text that would be the fret number
	if(count_string)
	{	//If the macro is this string
		unsigned long stringnum, notectr, bitmask, target_fret;
		EOF_PRO_GUITAR_TRACK *tp;
		EOF_RS_TECHNIQUES tech = {0};

		eof_notes_macro_slide_exceeding_fret[0] = '\0';	//Erase this string
		if(eof_read_macro_number(count_string, &target_fret))
		{	//If the fret number was successfully parsed
			for(ctr = 1; ctr < eof_song->tracks; ctr++)
			{	//For each track in the project
				if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
					continue;	//Skip non pro guitar tracks

				tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
				for(notectr = 0;  notectr < tp->pgnotes; notectr++)
				{	//For each normal note in the track
					for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
					{	//For each string used in this track
						if(tp->pgnote[notectr]->note & bitmask)
						{	//If this string is used by the note
							//Determine techniques used by this note (including applicable technotes using this string), do NOT assume a slide end fret if none is defined
							unsigned long retflags = eof_get_rs_techniques(eof_song, ctr, notectr, stringnum, &tech, 4, 1);
							if(retflags & (EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE))
							{	//If the note uses slide technique on this string
								if(((tech.slideto > 0) && (tech.slideto > target_fret)) || ((tech.unpitchedslideto > 0) && (tech.unpitchedslideto > target_fret)))
								{	//If a pitched or unpitched slide goes above the target fret
									char time_string[15] = {0};
									eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
									snprintf(eof_notes_macro_slide_exceeding_fret, sizeof(eof_notes_macro_slide_exceeding_fret) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
									dest_buffer[0] = '\0';
									return 3;	//True
								}
							}
						}
					}
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_BASS_TRACK_STRING_COUNT_EXCEEDED"))
	{
		unsigned long notectr;
		EOF_PRO_GUITAR_TRACK *tp;

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If the active track is a pro guitar track
			tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];	//Simplify
			if(tp->arrangement == EOF_BASS_ARRANGEMENT)
			{	//If the active track is a bass arrangement
				for(notectr = 0;  notectr < tp->pgnotes; notectr++)
				{	//For each normal note in the track
					if(tp->pgnote[notectr]->note >= 16)
					{	//If this note uses any more than the first four strings
						dest_buffer[0] = '\0';
						return 3;	//True
					}
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_ANY_TECH_NOTES_LACK_TARGET"))
	{
		unsigned long notectr;
		EOF_PRO_GUITAR_TRACK *tp;

		eof_notes_macro_tech_note_missing_target[0] = '\0';	//Erase this string
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//Skip non pro guitar tracks

			tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
			for(notectr = 0;  notectr < tp->technotes; notectr++)
			{	//For each tech note in the track
				if(!eof_pro_guitar_tech_note_overlaps_a_note(tp, notectr, tp->technote[notectr]->note, NULL))
				{	//If this tech note doesn't overlap a note on any string
					char time_string[15] = {0};
					eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
					snprintf(eof_notes_macro_tech_note_missing_target, sizeof(eof_notes_macro_tech_note_missing_target) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
					dest_buffer[0] = '\0';
					return 3;	//True
				}
			}
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_ANY_LYRICS_EXTEND_OUTSIDE_LINES"))
	{
		eof_notes_macro_lyric_extending_outside_line[0] = '\0';	//Erase this string
		for(ctr = 0; eof_song && (ctr < eof_song->vocal_track[0]->lyrics); ctr++)
		{	//For each lyric
			EOF_LYRIC *lyric = eof_song->vocal_track[0]->lyric[ctr];	//Simplify
			if(lyric->note != EOF_LYRIC_PERCUSSION)
			{	//If this is not a vocal percussion note
				EOF_PHRASE_SECTION *line = eof_find_lyric_line(ctr);
				if(line)
				{	//If this lyric begins within a lyric line
					if(lyric->pos + lyric->length > line->end_pos)
					{	//If this lyric extends beyond the end of the lyric line it begins in
						char time_string[15] = {0};
						eof_notes_panel_print_time(lyric->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
						snprintf(eof_notes_macro_lyric_extending_outside_line, sizeof(eof_notes_macro_lyric_extending_outside_line) - 1, "pos %s : \"%s\"", time_string, lyric->text);	//Write a string identifying the offending lyric
						dest_buffer[0] = '\0';
						return 3;	//True
					}
				}
			}
		}
		return 2;	//False
	}

	if(!ustricmp(macro, "IF_CHART_HAS_ANY_NOTES"))
	{
		if(eof_get_chart_size(eof_song))
		{	//If any of the project's tracks have at least one note/lyric
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_TEMPO_MAP_LOCKED"))
	{
		if(eof_song->tags->tempo_map_locked)
		{	//If the tempo map is locked
			dest_buffer[0] = '\0';
			return 3;	//True
		}

		return 2;	//False
	}

	if(!ustricmp(macro, "IF_RS_ANY_TECHNIQUES_MISSING_SUSTAIN"))
	{	//If the macro is this string
		unsigned long stringnum, notectr, bitmask;
		EOF_PRO_GUITAR_TRACK *tp;
		EOF_RS_TECHNIQUES tech = {0};

		eof_notes_macro_technique_missing_sustain[0] = '\0';	//Erase this string
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct note set can be examined
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//Skip non pro guitar tracks

			restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, ctr);
			eof_menu_track_set_tech_view_state(eof_song, ctr, 0);	//Disable tech view if applicable
			tp = eof_song->pro_guitar_track[eof_song->track[ctr]->tracknum];	//Simplify
			for(notectr = 0;  notectr < tp->pgnotes; notectr++)
			{	//For each normal note in the track
				for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
				{	//For each string used in this track
					if(tp->pgnote[notectr]->note & bitmask)
					{	//If this string is used by the note
						//Determine techniques used by this note (including applicable technotes using this string), do NOT assume a slide end fret if none is defined
						unsigned long retflags = eof_get_rs_techniques(eof_song, ctr, notectr, stringnum, &tech, 2, 1);

						if(retflags & (EOF_PRO_GUITAR_NOTE_FLAG_BEND | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE | EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO))
						{	//If this string uses any techniques that require sustain
							if(tech.length < 2)
							{	//If the gem for this string would export without at least 1ms of sustain
								char time_string[15] = {0};
								eof_notes_panel_print_time(tp->pgnote[notectr]->pos, time_string, sizeof(time_string) - 1, panel->timeformat);	//Build the timestamp in the current time format
								snprintf(eof_notes_macro_technique_missing_sustain, sizeof(eof_notes_macro_technique_missing_sustain) - 1, "%s - diff %u : pos %s", eof_song->track[ctr]->name, tp->pgnote[notectr]->type, time_string);	//Write a string identifying the offending note
								eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
								dest_buffer[0] = '\0';
								return 3;	//True
							}
						}
					}
				}
			}
			eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
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
	count_string = strcasestr_spec(macro, "MOVE_UP_PIXELS_");	//Get a pointer to the text that would be the pixel count
	if(count_string)
	{	//If the macro is this string
		unsigned long pixelcount;

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
	count_string = strcasestr_spec(macro, "MOVE_DOWN_PIXELS_");		//Get a pointer to the text that would be the pixel count
	if(count_string)
	{	//If the macro is this string
		unsigned long pixelcount;

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
		if(panel->xpos)
			panel->xpos--;	//Update the x coordinate for Notes panel printing
		return 1;
	}

	//Move the text output left by a given number of pixels
	count_string = strcasestr_spec(macro, "MOVE_LEFT_PIXELS_");	//Get a pointer to the text that would be the pixel count
	if(count_string)
	{	//If the macro is this string
		unsigned long pixelcount;

		if(eof_read_macro_number(count_string, &pixelcount))
		{	//If the pixel count was successfully parsed
			dest_buffer[0] = '\0';
			if(panel->xpos >= pixelcount)
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
	count_string = strcasestr_spec(macro, "MOVE_RIGHT_PIXELS_");	//Get a pointer to the text that would be the pixel count
	if(count_string)
	{
		unsigned long pixelcount;

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
			panel->definedcolor = newcolor;	//This will be the permanent text color in effect until it is overridden
			return 1;
		}
	}

	//Clear the printed text background color
	if(strcasestr_spec(macro, "TEXT_BACKGROUND_COLOR_NONE"))
	{
		dest_buffer[0] = '\0';
		panel->bgcolor = -1;
		panel->definedbgcolor = -1;	//This will be the permanent background color in effect until it is overridden
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
			panel->definedbgcolor = newcolor;	//This will be the permanent background color in effect until it is overridden
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

	//Reset x position to left edge and move down one line
	if(!ustricmp(macro, "NEWLINE"))
	{
		dest_buffer[0] = '\0';
		panel->flush = 1;
		panel->newline = 1;
		panel->allowempty = 0;
		return 1;
	}

	//Change time print format to milliseconds
	if(!ustricmp(macro, "TIME_FORMAT_MS"))
	{
		dest_buffer[0] = '\0';
		panel->timeformat = 0;
		return 1;
	}

	//Change time print format to mm:ss.ms
	if(!ustricmp(macro, "TIME_FORMAT_MM_SS_MS"))
	{
		dest_buffer[0] = '\0';
		panel->timeformat = 1;
		return 1;
	}

	//Change text printing to white text on red background until the end of the current notes panel line
	if(!ustricmp(macro, "DISPLAY_ERROR"))
	{
		dest_buffer[0] = '\0';
		panel->color = eof_color_white;
		panel->bgcolor = eof_color_red;
		panel->colorprinted = 1;	//Ensure the next printed line is a few pixels lower to avoid obscuring text with solid background color
		return 1;
	}

	//Change text printing to black text on yellow background until the end of the current notes panel line
	if(!ustricmp(macro, "DISPLAY_WARNING"))
	{
		dest_buffer[0] = '\0';
		panel->color = eof_color_black;
		panel->bgcolor = eof_color_yellow;
		panel->colorprinted = 1;	//Ensure the next printed line is a few pixels lower to avoid obscuring text with solid background color
		return 1;
	}

	//Change text printing to black text on green background until the end of the current notes panel line
	if(!ustricmp(macro, "DISPLAY_SUCCESS"))
	{
		dest_buffer[0] = '\0';
		panel->color = eof_color_black;
		panel->bgcolor = eof_color_green;
		panel->colorprinted = 1;	//Ensure the next printed line is a few pixels lower to avoid obscuring text with solid background color
		return 1;
	}

	//Change text printing to yellow text on blue background until the end of the current notes panel line
	if(!ustricmp(macro, "DISPLAY_ALERT"))
	{
		dest_buffer[0] = '\0';
		panel->color = eof_color_yellow;
		panel->bgcolor = eof_color_blue;
		panel->colorprinted = 1;	//Ensure the next printed line is a few pixels lower to avoid obscuring text with solid background color
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
			snprintf(dest_buffer, dest_buffer_size, "(Unset)");
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
				(void) snprintf(dest_buffer, dest_buffer_size, "(Pro: Unset)");
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
				(void) snprintf(dest_buffer, dest_buffer_size, "(Harmony: Unset)");
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

		count = eof_count_track_num_notes_with_tflag(EOF_NOTE_TFLAG_SOLO_NOTE);

		if(count)
		{	//If there are any solo notes in the active track
			if(tracksize)
			{	//Redundant check to satisfy Coverity
				double percent;

				percent = (double)count * 100.0 / tracksize;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));
			}
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
			snprintf(dest_buffer, dest_buffer_size, "%lu/%lu/%.2f (min/max/mean)", min, max, (double)count / solocount);
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
			if(tracksize)
			{	//Redundant check to satisfy Coverity
				double percent;

				percent = (double)count * 100.0 / tracksize;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));
			}
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
		unsigned long count = 0, linecount;

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

	//Track star power note stats
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

	//Track slider count
	if(!ustricmp(macro, "TRACK_SLIDER_COUNT"))
	{
		unsigned long count = eof_get_num_sliders(eof_song, eof_selected_track);

		if(count)
		{	//If there are any slider sections in the active track
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
			if(tracksize)
			{	//Redundant check to satisfy Coverity
				double percent;

				percent = (double)count * 100.0 / tracksize;
				snprintf(dest_buffer, dest_buffer_size, "%lu (~%lu%%)", count, (unsigned long)(percent + 0.5));
			}
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//Track slider note stats
	if(!ustricmp(macro, "TRACK_SLIDER_NOTE_STATS"))
	{
		unsigned long count, slidercount, min = 0, max = 0;

		count = eof_notes_panel_count_section_stats(EOF_SLIDER_SECTION, &min, &max);
		slidercount = eof_get_num_sliders(eof_song, eof_selected_track);	//Redundantly check that this isn't zero to resolve a false positive in Coverity

		if(count && slidercount)
		{	//If there are any solo notes in the active track
			snprintf(dest_buffer, dest_buffer_size, "%lu/%lu/%.2f (min/max/mean)", min, max, (double)count / slidercount);
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
		if(!panel->timeformat)
		{	//If timestamps are to be printed in milliseconds
			snprintf(dest_buffer, dest_buffer_size, "%f", eof_song->beat[eof_selected_beat]->fpos);
		}
		else
		{
			int min, sec, inttime;
			double ms, time;

			inttime = eof_song->beat[eof_selected_beat]->pos;	//Simplify
			time = eof_song->beat[eof_selected_beat]->fpos;	//Simplify
			ms = fmod(time, 1000.0);
			min = (inttime / 1000) / 60;
			sec = (inttime / 1000) % 60;
			snprintf(dest_buffer, dest_buffer_size, "%02d:%02d.%f", min, sec, ms);
		}
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
		if(eof_selection.current < tracksize)
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_selection.current);
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric position
	if(!ustricmp(macro, "SELECTED_NOTE_POS"))
	{
		if(eof_selection.current < tracksize)
			eof_notes_panel_print_time(eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current), dest_buffer, dest_buffer_size, panel->timeformat);	//Print in the current time format
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric position
	if(!ustricmp(macro, "SELECTED_NOTE_LENGTH"))
	{
		if(eof_selection.current < tracksize)
			snprintf(dest_buffer, dest_buffer_size, "%ld", eof_get_note_length(eof_song, eof_selected_track, eof_selection.current));
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric name/text
	if(!ustricmp(macro, "SELECTED_NOTE_NAME"))
	{
		if(eof_selection.current < tracksize)
			snprintf(dest_buffer, dest_buffer_size, "%s", eof_get_note_name(eof_song, eof_selected_track, eof_selection.current));
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric value/tone
	if(!ustricmp(macro, "SELECTED_NOTE_VALUE"))
	{
		if(eof_selection.current < tracksize)
			snprintf(dest_buffer, dest_buffer_size, "%d", eof_get_note_note(eof_song, eof_selected_track, eof_selection.current));
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric flags
	if(!ustricmp(macro, "SELECTED_NOTE_FLAGS"))
	{
		if(eof_selection.current < tracksize)
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_get_note_flags(eof_song, eof_selected_track, eof_selection.current));
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//Selected note/lyric value/tone
	if(!ustricmp(macro, "SELECTED_LYRIC_TONE_NAME"))
	{
		if(eof_vocals_selected && (eof_selection.current < tracksize))
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
		if(eof_selection.current < tracksize)
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
		if(eof_selection.current < tracksize)
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
		if(eof_selection.current < tracksize)
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
		if(eof_selection.current < tracksize)
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

	//Selected pro guitar note MIDI tones
	if(!ustricmp(macro, "PRO_GUITAR_NOTE_MTONES"))
	{
		if(eof_selection.current < tracksize)
		{	//If a note is selected
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				char temp[10];
				unsigned char pitchmask, pitches[6] = {0}, bitmask;

				dest_buffer[0] = '\0';	//Empty this string
				pitchmask = eof_get_midi_pitches(eof_song, eof_selected_track, eof_selection.current, pitches);	//Determine how many exportable pitches this note/lyric has
				for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
				{	//For each of the six usable strings
					if(pitchmask & bitmask)
					{	//If this string has a pitch
						snprintf(temp, sizeof(temp) - 1, "%s%u", ctr ? "," : "", pitches[ctr]);
					}
					else
					{
						snprintf(temp, sizeof(temp) - 1, "_");
					}
					strncat(dest_buffer, temp, dest_buffer_size - strlen(dest_buffer) - 1);	//Append the tone information
				}
			}
		}
		return 1;
	}

	//Selected note NPS
	if(!ustricmp(macro, "SELECTED_NOTE_NPS"))
	{
		unsigned long start = 0, end = 0, selected = 0;

		if(eof_get_selected_note_range(&start, &end, 0))
		{	//If at least one note is selected
			for(ctr = start; ctr <= end; ctr++)
			{	//For each note index between the first and last selected notes
				if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
				{	//If the note is in the active difficulty
					selected++;	//Count how many notes are actively selected or passively select (exist between first and last actively selected notes)
				}
			}
			(void) eof_get_selected_note_range(&start, &end, 1);	//Get the start and stop times of the note selection
			snprintf(dest_buffer, dest_buffer_size, "%.2f", (double)selected * 1000.0 / ((double)end - (double)start));
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//The fret hand position in effect at the pro guitar track's current seek position
	if(!ustricmp(macro, "PRO_GUITAR_TRACK_EFFECTIVE_FHP"))
	{
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
			unsigned char position;

			position = eof_pro_guitar_track_find_effective_fret_hand_position(tp, eof_note_type, eof_music_pos.value - eof_av_delay);	//Find if there's a fret hand position in effect
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

			tone = eof_pro_guitar_track_find_effective_tone(tp, eof_music_pos.value - eof_av_delay);	//Find if there's a tone change in effect
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

		ms = (eof_music_pos.value - eof_av_delay) % 1000;
		if(!eof_display_seek_pos_in_seconds)
		{	//If the seek position is to be displayed as minutes:seconds.milliseconds
			min = ((eof_music_pos.value - eof_av_delay) / 1000) / 60;
			sec = ((eof_music_pos.value - eof_av_delay) / 1000) % 60;
			snprintf(dest_buffer, dest_buffer_size, "%02d:%02d.%03d", min, sec, ms);
		}
		else
		{	//If the seek position is to be displayed as seconds
			sec = (eof_music_pos.value - eof_av_delay) / 1000;
			snprintf(dest_buffer, dest_buffer_size, "%d.%03ds", sec, ms);
		}
		return 1;
	}

	//Seek position in seconds.milliseconds format
	if(!ustricmp(macro, "SEEK_POSITION_SEC"))
	{
		int sec, ms;

		sec = (eof_music_pos.value - eof_av_delay) / 1000;
		ms = (eof_music_pos.value - eof_av_delay) % 1000;
		snprintf(dest_buffer, dest_buffer_size, "%d.%03ds", sec, ms);

		return 1;
	}

	//Seek position in minutes:seconds.milliseconds format
	if(!ustricmp(macro, "SEEK_POSITION_MIN_SEC"))
	{
		eof_notes_panel_print_time(eof_music_pos.value - eof_av_delay, dest_buffer, dest_buffer_size, 0);	//Print in mm:ss.ms format

		return 1;
	}

	//Seek position as a percentage of the chart's total length
	if(!ustricmp(macro, "SEEK_POSITION_PERCENT"))
	{
		double percent = (double)(eof_music_pos.value - eof_av_delay) * 100.0 / eof_chart_length;

		snprintf(dest_buffer, dest_buffer_size, "%.2f", percent);

		return 1;
	}

	//Number of notes selected
	if(!ustricmp(macro, "COUNT_NOTES_SELECTED"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%lu", eof_count_selected_and_unselected_notes(NULL));	//Count the number of selected notes, don't track the count of notes in the active track difficulty

		return 1;
	}

	//Number of notes in active track difficulty
	if(!ustricmp(macro, "TRACK_DIFF_NOTE_COUNT"))
	{
		unsigned long count;

		count = eof_get_track_diff_size(eof_song, eof_selected_track, eof_note_type);
		snprintf(dest_buffer, dest_buffer_size, "%lu", count);
		return 1;
	}

	//Number of notes in the flattened active track difficulty
	if(!ustricmp(macro, "TRACK_FLATTENED_DIFF_NOTE_COUNT"))
	{
		unsigned long count;

		count = eof_get_track_flattened_diff_size(eof_song, eof_selected_track, eof_note_type);
		snprintf(dest_buffer, dest_buffer_size, "%lu", count);
		return 1;
	}

	//The defined start point
	if(!ustricmp(macro, "START_POINT"))
	{
		if(eof_song->tags->start_point != ULONG_MAX)
			eof_notes_panel_print_time(eof_song->tags->start_point, dest_buffer, dest_buffer_size, panel->timeformat);	//Print in the current time format
		else
			snprintf(dest_buffer, dest_buffer_size, "None");
		return 1;
	}

	//The defined start point
	if(!ustricmp(macro, "END_POINT"))
	{
		if(eof_song->tags->end_point != ULONG_MAX)
			eof_notes_panel_print_time(eof_song->tags->end_point, dest_buffer, dest_buffer_size, panel->timeformat);	//Print in the current time format
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
				snprintf(dest_buffer, dest_buffer_size, "1/%d b", eof_snap_interval);
			else
				snprintf(dest_buffer, dest_buffer_size, "1/%d m", eof_snap_interval);
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
			snprintf(dest_buffer, dest_buffer_size, "%s", get_filename(eof_loaded_ogg_name));
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
		eof_ch_sp_solution_wanted = 1;
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
		eof_ch_sp_solution_wanted = 1;
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
		eof_ch_sp_solution_wanted = 1;
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
		eof_ch_sp_solution_wanted = 1;
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
		eof_ch_sp_solution_wanted = 1;
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
		eof_ch_sp_solution_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score && eof_ch_sp_solution->num_deployments)
		{	//If the global star power solution structure is built and a score was determined
			unsigned long sp_start = 0, sp_end = 0;
			if(eof_pos_is_within_sp_deployment(eof_ch_sp_solution, eof_music_pos.value - eof_av_delay, &sp_start, &sp_end))
			{	//If the seek position is within any defined star power deployment
				snprintf(dest_buffer, dest_buffer_size, "Seek pos within %lums to %lums deployment", sp_start, sp_end);
			}
			else
			{
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
		eof_ch_sp_solution_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->score)
		{	//If the global star power solution structure is built and a score was determined
			unsigned long count = 0;

			(void) eof_count_selected_and_unselected_notes(&count);	//Count the number of notes in the active track difficulty
			snprintf(dest_buffer, dest_buffer_size, "%lu (1 : %.2f notes)", eof_ch_sp_solution->deploy_count, (double)count / eof_ch_sp_solution->deploy_count);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "None");
		}
		return 1;
	}

	//The star power meter level in effect after hitting the note at/before the seek position
	if(!ustricmp(macro, "CH_SP_METER_AFTER_LAST_NOTE_HIT"))
	{
		eof_ch_sp_solution_wanted = 1;
		if(eof_ch_sp_solution && eof_ch_sp_solution->resulting_sp_meter)
		{	//If the global star power solution structure is built and the star power meter array is allocated
			unsigned long index, pos, target;

			for(ctr = 0, index = 0, target = ULONG_MAX; ctr < tracksize; ctr++)
			{	//For each note in the active track
				pos = eof_get_note_pos(eof_song, eof_selected_track, ctr);

				if(pos > eof_music_pos.value - eof_av_delay)
				{	//If this note is after the seek position
					break;	//All other notes are as well, stop parsing them
				}
				if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
				{	//If the note is in the active difficulty
					target = index;	//Remember this as the last note at/before the seek position in the active difficulty
					index++;		//Track the number of notes in the target difficulty that have been encountered
				}
			}
			if(target < eof_ch_sp_solution->note_count)
			{	//If the note's star power data was found successfully
				snprintf(dest_buffer, dest_buffer_size, "%.2f%%", eof_ch_sp_solution->resulting_sp_meter[target] * 100.0);
			}
			else
			{
				snprintf(dest_buffer, dest_buffer_size, "(0%%)");
			}
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
//		snprintf(dest_buffer, dest_buffer_size, "CTRL:%c ALT:%c SHIFT:%c,%c (%c) CODE:%d ASCII:%d ('%c')", KEY_EITHER_CTRL ? '*' : ' ', KEY_EITHER_ALT ? '*' : ' ', key[KEY_LSHIFT] ? '*' : ' ', key[KEY_RSHIFT] ? '*' : ' ', key_shifts & KB_SHIFT_FLAG ? '*' : ' ', eof_last_key_code, eof_last_key_char, eof_last_key_char);	//Debugging
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
	count_string = strcasestr_spec(macro, "TRACK_DIFF_NUMBER_NOTES_WITH_GEM_COUNT_");	//Get a pointer to the text that would be the gem count
	if(count_string)
	{	//If the macro is this string
		unsigned long count, gemcount, totalnotecount = 0;

		if(eof_read_macro_number(count_string, &gemcount))
		{	//If the gem count was successfully parsed
			double percent;

			count = eof_count_num_notes_with_gem_count(gemcount);		//Determine how many such notes are in the active track difficulty
			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
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
			(void) eof_count_selected_and_unselected_notes(&totalnotecount);							//Count the number of notes in the active track difficulty
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
			unsigned long count = 0, totalnotecount = 0;
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
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
			unsigned long count = 0, totalnotecount = 0;
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
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
			unsigned long count = 0, totalnotecount = 0;

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tracksize; ctr++)
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
			unsigned long count = 0, totalnotecount = 0;

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tracksize; ctr++)
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

	//Display the number of notes containing cymbal drum notes (and the corresponding percentage that is of all notes) in the active drum difficulty
	if(!ustricmp(macro, "TRACK_DIFF_NOTE_COUNT_AND_RATIO_CYMBALS"))
	{
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			unsigned long count = 0, totalnotecount = 0;

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tracksize; ctr++)
				{	//For each note in the track
					if((eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type) && (eof_note_is_cymbal(eof_song, eof_selected_track, ctr)))
					{	//If the note is in the active difficulty and has a cymbal
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

	//Display the number of pitched lyrics (and the corresponding percentage that is of all notes) in the active vocal track
	if(!ustricmp(macro, "TRACK_DIFF_COUNT_AND_RATIO_PITCHED_LYRIC"))
	{
		if(eof_vocals_selected)
		{	//If the vocal track is active
			unsigned long count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tracksize; ctr++)
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
			unsigned long count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
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
			unsigned long count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
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
			unsigned long count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
			if(totalnotecount)
			{
				double percent;

				for(ctr = 0; ctr < tracksize; ctr++)
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
			unsigned long count = 0, totalnotecount = 0;
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[0];

			(void) eof_count_selected_and_unselected_notes(&totalnotecount);			//Count the number of notes in the active track difficulty
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
	count_string = strcasestr_spec(macro, "COUNT_LYRICS_WITH_PITCH_NUMBER_");	//Get a pointer to the text that would be the pitch
	if(count_string)
	{	//If the macro is this string
		unsigned long count = 0, pitch;

		if(eof_vocals_selected)
		{	//If the vocal track is active
			if(eof_read_macro_number(count_string, &pitch))
			{	//If the pitch number was successfully parsed
				for(ctr = 0; ctr < tracksize; ctr++)
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

	//Display the number of notes (and the corresponding percentage that is of all notes) in the active track difficulty with a specific pitch
	if(strcasestr_spec(macro, "TRACK_COUNT_HIGHLIGHTED_NOTES"))
	{
		unsigned long count;

		for(ctr = 0, count = 0; ctr < tracksize; ctr++)
		{	//For each note in the active track
			if(eof_note_is_highlighted(eof_song, eof_selected_track, ctr))
				count++;	//Count the number of highlighted notes
		}
		snprintf(dest_buffer, dest_buffer_size, "%lu", count);

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

	//The last calculated framerate
	if(!ustricmp(macro, "EOF_FPS"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%.2f", eof_main_loop_fps);
		return 1;
	}

	//The effective note per second rate of the current grid snap at the current seek position
	if(!ustricmp(macro, "GRID_SNAP_SEEK_POS_LENGTH_NPS"))
	{
		unsigned long pos = eof_music_pos.value - eof_av_delay;

		if((eof_snap_mode != EOF_SNAP_OFF) && (pos < eof_song->beat[eof_song->beats - 1]->pos))
		{	//If grid snap is enabled and the seek position is before the last beat marker
			EOF_SNAP_DATA temp = {0, 0.0, 0, 0.0, 0, 0, 0, 0.0, {0.0}, {0.0}, 0, 0, 0, 0};
			unsigned long beat;

			beat = eof_get_beat(eof_song, pos);		//Find which beat the seek position is in
			if(eof_beat_num_valid(eof_song, beat))
			{	//If the beat was found
				eof_snap_logic(&temp, eof_song->beat[beat]->pos);	//Find grid snap length (in ms) starting at this beat
				snprintf(dest_buffer, dest_buffer_size, "%.2fms , %.2fNPS", temp.snap_length, 1000.0 / temp.snap_length);
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

	//The status of the phase cancellation option
	if(!ustricmp(macro, "PHASE_CANCELLATION"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", (eof_phase_cancellation ? "On" : "Off"));
		return 1;
	}

	//The status of the center isolation option
	if(!ustricmp(macro, "CENTER_ISOLATION"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", (eof_center_isolation ? "On" : "Off"));
		return 1;
	}

	//The status of whether the seek position is within a defined star power phrase
	if(!ustricmp(macro, "SEEK_SP_STATUS") || !ustricmp(macro, "SEEK_SP_STATUS_CONDITIONAL"))
	{
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_SP_SECTION, eof_music_pos.value - eof_av_delay);
		dest_buffer[0] = '\0';
		if(phraseptr)
		{	//If the seek position is within a star power phrase
			snprintf(dest_buffer, dest_buffer_size, "Seek pos within SP phrase (%lums - %lums)", phraseptr->start_pos, phraseptr->end_pos);
		}
		else if(!ustricmp(macro, "SEEK_SP_STATUS"))
		{	//If this isn't %SEEK_SP_STATUS_CONDITIONAL%
			snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within an SP phrase");
		}
		return 1;
	}

	//The status of whether the seek position is within a defined lyric line
	if(!ustricmp(macro, "SEEK_LYRIC_LINE_STATUS") || !ustricmp(macro, "SEEK_LYRIC_LINE_STATUS_CONDITIONAL"))
	{
		phraseptr = eof_get_section_instance_at_pos(eof_song, EOF_TRACK_VOCALS, EOF_LYRIC_PHRASE_SECTION, eof_music_pos.value - eof_av_delay);
		dest_buffer[0] = '\0';
		if(phraseptr)
		{	//If the seek position is within a lyric line
			snprintf(dest_buffer, dest_buffer_size, "Seek pos within lyric line (%lums - %lums)", phraseptr->start_pos, phraseptr->end_pos);
		}
		else if(!ustricmp(macro, "SEEK_LYRIC_LINE_STATUS"))
		{	//If this isn't %SEEK_LYRIC_LINE_STATUS_CONDITIONAL%
			snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within a lyric line");
		}
		return 1;
	}

	//The status of whether the seek position is within a defined slider phrase
	if(!ustricmp(macro, "SEEK_SLIDER_STATUS") || !ustricmp(macro, "SEEK_SLIDER_STATUS_CONDITIONAL"))
	{
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_SLIDER_SECTION, eof_music_pos.value - eof_av_delay);
		dest_buffer[0] = '\0';
		if(phraseptr)
		{	//If the seek position is within a slider phrase
			snprintf(dest_buffer, dest_buffer_size, "Seek pos within slider phrase (%lums - %lums)", phraseptr->start_pos, phraseptr->end_pos);
		}
		else if(!ustricmp(macro, "SEEK_SLIDER_STATUS"))
		{	//If this isn't %SEEK_SLIDER_STATUS_CONDITIONAL%
			snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within a slider phrase");
		}
		return 1;
	}

	//The status of whether the seek position is within a defined solo phrase
	if(!ustricmp(macro, "SEEK_SOLO_STATUS") || !ustricmp(macro, "SEEK_SOLO_STATUS_CONDITIONAL"))
	{
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_SOLO_SECTION, eof_music_pos.value - eof_av_delay);
		dest_buffer[0] = '\0';
		if(phraseptr)
		{	//If the seek position is within a solo phrase
			snprintf(dest_buffer, dest_buffer_size, "Seek pos within solo phrase (%lums - %lums)", phraseptr->start_pos, phraseptr->end_pos);
		}
		else if(!ustricmp(macro, "SEEK_SOLO_STATUS"))
		{	//If this isn't %SEEK_SOLO_STATUS_CONDITIONAL%
			snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within a solo phrase");
		}
		return 1;
	}

	//The status of whether the seek position is within a defined trill (special drum roll) phrase
	if(!ustricmp(macro, "SEEK_TRILL_STATUS") || !ustricmp(macro, "SEEK_TRILL_STATUS_CONDITIONAL"))
	{
		char *phrasename1 = "trill phrase";
		char *phrasename2 = "special drum roll";
		char *effectivephrasename = phrasename1;

		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If the active track is a drum track
			effectivephrasename = phrasename2;
		}
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_TRILL_SECTION, eof_music_pos.value - eof_av_delay);
		dest_buffer[0] = '\0';
		if(phraseptr)
		{	//If the seek position is within a trill phrase
			snprintf(dest_buffer, dest_buffer_size, "Seek pos within %s (%lums - %lums)", effectivephrasename, phraseptr->start_pos, phraseptr->end_pos);
		}
		else if(!ustricmp(macro, "SEEK_TRILL_STATUS"))
		{	//If this isn't %SEEK_TRILL_STATUS_CONDITIONAL%
			snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within a %s", effectivephrasename);
		}
		return 1;
	}

	//The status of whether the seek position is within a defined tremolo (drum roll) phrase
	if(!ustricmp(macro, "SEEK_TREMOLO_STATUS") || !ustricmp(macro, "SEEK_TREMOLO_STATUS_CONDITIONAL"))
	{
		char *phrasename1 = "tremolo phrase";
		char *phrasename2 = "drum roll";
		char *effectivephrasename = phrasename1;

		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If the active track is a drum track
			effectivephrasename = phrasename2;
		}
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_TREMOLO_SECTION, eof_music_pos.value - eof_av_delay);
		dest_buffer[0] = '\0';
		if(phraseptr)
		{	//If the seek position is within a trill phrase
			snprintf(dest_buffer, dest_buffer_size, "Seek pos within %s (%lums - %lums)", effectivephrasename, phraseptr->start_pos, phraseptr->end_pos);
		}
		else if(!ustricmp(macro, "SEEK_TREMOLO_STATUS"))
		{	//If this isn't %SEEK_TREMOLO_STATUS_CONDITIONAL%
			snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within a %s", effectivephrasename);
		}
		return 1;
	}

	//The status of whether the seek position is within a defined arpeggio phrase
	if(!ustricmp(macro, "SEEK_ARPEGGIO_STATUS") || !ustricmp(macro, "SEEK_ARPEGGIO_STATUS_CONDITIONAL"))
	{
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_music_pos.value - eof_av_delay);
		dest_buffer[0] = '\0';
		if(phraseptr && !(phraseptr->flags & EOF_RS_ARP_HANDSHAPE))
		{	//If the seek position is within a arpeggio phrase and is NOT marked as a handshape
			snprintf(dest_buffer, dest_buffer_size, "Seek pos within arpeggio (%lums - %lums)", phraseptr->start_pos, phraseptr->end_pos);
		}
		else if(!ustricmp(macro, "SEEK_ARPEGGIO_STATUS"))
		{	//If this isn't %SEEK_ARPEGGIO_STATUS_CONDITIONAL%
			snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within an arpeggio");
		}
		return 1;
	}

	//The status of whether the seek position is within a defined handshape phrase
	if(!ustricmp(macro, "SEEK_HANDSHAPE_STATUS") || !ustricmp(macro, "SEEK_HANDSHAPE_STATUS_CONDITIONAL"))
	{
		phraseptr = eof_get_section_instance_at_pos(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_music_pos.value - eof_av_delay);
		dest_buffer[0] = '\0';
		if(phraseptr && (phraseptr->flags & EOF_RS_ARP_HANDSHAPE))
		{	//If the seek position is within a arpeggio phrase and that phrase has the flag to indicate it is a handshape
			snprintf(dest_buffer, dest_buffer_size, "Seek pos within handshape (%lums - %lums)", phraseptr->start_pos, phraseptr->end_pos);
		}
		else if(!ustricmp(macro, "SEEK_HANDSHAPE_STATUS"))
		{	//If this isn't %SEEK_HANDSHAPE_STATUS_CONDITIONAL%
			snprintf(dest_buffer, dest_buffer_size, "Seek pos is not within a handshape");
		}
		return 1;
	}

	//The active zoom level
	if(!ustricmp(macro, "ZOOM_LEVEL"))
	{
		snprintf(dest_buffer, dest_buffer_size, "1/%u", eof_zoom);
		return 1;
	}

	//Active MIDI tones
	if(!ustricmp(macro, "ACTIVE_MIDI_TONES"))
	{
		unsigned channel;
		char temp[20];
		dest_buffer[0] = '\0';	//Empty this string
		for(channel = 0; channel < 6; channel++)
		{	//For each of the six MIDI channels EOF uses to play tones
			if(!eof_midi_channel_status[channel].on)
			{
				strncat(dest_buffer, "x ", dest_buffer_size - strlen(dest_buffer) - 1);	//Append the tone information
			}
			else
			{
				clock_t timeleft = ((eof_midi_channel_status[channel].stop_time - clock()) * 1000) / CLOCKS_PER_SEC;
				snprintf(temp, sizeof(temp) - 1, "%u (%ldms) ", eof_midi_channel_status[channel].note, timeleft);
				strncat(dest_buffer, temp, dest_buffer_size - strlen(dest_buffer) - 1);	//Append the tone information
			}
		}
		return 1;
	}

	if(!ustricmp(macro, "IR_ALBUM_ART_FILENAME"))
	{
		album_art_filename[0] = '\0';	//Empth this string
		if(eof_check_for_immerrock_album_art(eof_song_path, album_art_filename, sizeof(album_art_filename) - 1, 0))
		{	//If the project folder has any suitably named album art for use with IMMERROCK
			snprintf(dest_buffer, dest_buffer_size, "%s", get_filename(album_art_filename));
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "(Album art not found)");
		}
		return 1;
	}

	if(!ustricmp(macro, "PRINT_LYRIC_LINE_COUNT_STRING"))
	{
		if(eof_song)
		{
			if(eof_song->vocal_track[0]->lines == 1)
				snprintf(dest_buffer, dest_buffer_size, "1 lyric line is defined.");
			else
				snprintf(dest_buffer, dest_buffer_size, "%lu lyric lines are defined.", eof_song->vocal_track[0]->lines);
		}
		return 1;
	}

	if(!ustricmp(macro, "PRINT_IR_SECTION_COUNT_STRING"))
	{
		if(eof_song)
		{
			unsigned long count = eof_count_immerrock_sections();
			if(count == 1)
				snprintf(dest_buffer, dest_buffer_size, "1 section is defined.");
			else
				snprintf(dest_buffer, dest_buffer_size, "%lu sections are defined.", count);
		}
		return 1;
	}

	if(!ustricmp(macro, "PRINT_RS_SECTION_COUNT_STRING"))
	{
		if(eof_song)
		{
			unsigned long count = 0;
			for(ctr = 0; ctr < eof_song->beats; ctr++)
			{	//For each beat in the chart
				if(eof_song->beat[ctr]->contained_rs_section_event >= 0)
				{	//If this beat has a Rocksmith section
					count++;	//Update Rocksmith section instance counter
				}
			}
			if(count == 1)
				snprintf(dest_buffer, dest_buffer_size, "1 / 100 sections is defined.");
			else
				snprintf(dest_buffer, dest_buffer_size, "%lu / 100 sections are defined.", count);
		}
		return 1;
	}

	if(!ustricmp(macro, "PRINT_IR_MISSING_FINGERING_COUNT_STRING"))
	{
		unsigned long count, total = 0;
		count = eof_count_immerrock_chords_missing_fingering(&total);
		if(total == 1)
			snprintf(dest_buffer, dest_buffer_size, "%lu out of %lu chord is missing finger placement.", count, total);
		else
			snprintf(dest_buffer, dest_buffer_size, "%lu out of %lu chords are missing finger placement.", count, total);
		return 1;
	}

	if(!ustricmp(macro, "PRINT_RS_MISSING_FINGERING_COUNT_STRING"))
	{
		unsigned long count, total = 0;
		count = eof_count_immerrock_chords_missing_fingering(&total);
		if(total == 1)
			snprintf(dest_buffer, dest_buffer_size, "%lu out of %lu chord is missing finger definition", count, total);
		else
			snprintf(dest_buffer, dest_buffer_size, "%lu out of %lu chords are missing finger definition", count, total);
		return 1;
	}

	if(!ustricmp(macro, "SEEK_IR_SECTION_CONDITIONAL"))
	{
		char section[50];

		if(eof_lookup_immerrock_effective_section_at_pos(eof_song, eof_music_pos.value - eof_av_delay, section, sizeof(section)))
		{	//If a section name was found to be in effect at the seek position
			snprintf(dest_buffer, dest_buffer_size, "%s", section);
		}

		return 1;
	}

	if(!ustricmp(macro, "SEEK_RS_SECTION_CONDITIONAL"))
	{
		char section[50];

		if(eof_lookup_rocksmith_effective_section_at_pos(eof_song, eof_music_pos.value - eof_av_delay, section, sizeof(section)))
		{	//If a section name was found to be in effect at the seek position
			snprintf(dest_buffer, dest_buffer_size, "%s", section);
		}

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_NOTE_OCCURRING_BEFORE_MILLIS"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_note_occurs_before_millis);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_PITCHED_SLIDE_LACKING_LINKNEXT"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_pitched_slide_missing_linknext);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_TONE_CHANGE_STARTING_ON_A_NOTE"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_note_starting_on_tone_change);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_NOTE_SUBCEEDING_FHP"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_note_subceeding_fhp);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_NON_TAP_NOTE_EXCEEDING_FHP"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_note_exceeding_fhp);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_PITCHED_SLIDE_LACKING_END_FRET"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_pitched_slide_missing_end_fret);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_BEND_LACKING_STRENGTH_DEFINITION"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_bend_missing_strength);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_OPEN_NOTE_WITH_BEND"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_open_note_bend);

		return 1;
	}

	if(!ustricmp(macro, "LEFT_CLICK_X_COORD"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%u", eof_click_x);

		return 1;
	}

	if(!ustricmp(macro, "LEFT_CLICK_Y_COORD"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%u", eof_click_y);

		return 1;
	}

	if(!ustricmp(macro, "MOUSE_AREA"))
	{
		dest_buffer[0] = '\0';
		if(eof_mouse_area == 1)
			snprintf(dest_buffer, dest_buffer_size, "Difficulty tabs (%u, %u) - (%u, %u)", eof_difficulty_tab_boundary_x1, eof_difficulty_tab_boundary_y1, eof_difficulty_tab_boundary_x2, eof_difficulty_tab_boundary_y2);
		else if(eof_mouse_area == 2)
			snprintf(dest_buffer, dest_buffer_size, "Beat markers (%u, %u) - (%u, %u)", eof_beat_marker_boundary_x1, eof_beat_marker_boundary_y1, eof_beat_marker_boundary_x2, eof_beat_marker_boundary_y2);
		else if(eof_mouse_area == 3)
			snprintf(dest_buffer, dest_buffer_size, "Fretboard (%u, %u) - (%u, %u)", eof_fretboard_boundary_x1, eof_fretboard_boundary_y1, eof_fretboard_boundary_x2, eof_fretboard_boundary_y2);
		else if(eof_mouse_area == 4)
			snprintf(dest_buffer, dest_buffer_size, "Mini keyboard (%u, %u) - (%u, %u)", eof_mini_keyboard_boundary_x1, eof_mini_keyboard_boundary_y1, eof_mini_keyboard_boundary_x2, eof_mini_keyboard_boundary_y2);

		return 1;
	}

	if(!ustricmp(macro, "MOUSE_CONSTRAINT"))
	{
		dest_buffer[0] = '\0';
		if(eof_mouse_bound)
		{	//If the mouse coordinates are constrained
			snprintf(dest_buffer, dest_buffer_size, "(%u, %u) - (%u, %u)", eof_mouse_boundary_x1, eof_mouse_boundary_y1, eof_mouse_boundary_x2, eof_mouse_boundary_y2);
		}

		return 1;
	}

	if(!ustricmp(macro, "DIFFICULTY_TAB_AREA"))
	{
		snprintf(dest_buffer, dest_buffer_size, "(%u, %u) - (%u, %u)", eof_difficulty_tab_boundary_x1, eof_difficulty_tab_boundary_y1, eof_difficulty_tab_boundary_x2, eof_difficulty_tab_boundary_y2);

		return 1;
	}

	if(!ustricmp(macro, "FIRST_BEAT_SUBCEEDING_TEMPO"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_tempo_subceeding_number);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_FHP_EXCEEDING_NUMBER"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_fhp_exceeding_number);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_NOTE_EXCEEDING_FRET"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_note_exceeding_fret);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_NOTE_EXCEEDING_DIFF"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_note_exceeding_diff);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_SLIDE_EXCEEDING_FRET"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_slide_exceeding_fret);

		return 1;
	}

	if(!ustricmp(macro, "FIRST_LYRIC_EXTENDING_OUTSIDE_LINE"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_lyric_extending_outside_line);

		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_TECHNIQUE_MISSING_SUSTAIN"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_technique_missing_sustain);

		return 1;
	}


	///DEBUGGING MACROS
	//The selected beat's PPQN value (used to calculate its BPM)
	if(!ustricmp(macro, "DEBUG_BEAT_PPQN"))
	{
		if(eof_selected_beat < eof_song->beats)
		{
			snprintf(dest_buffer, dest_buffer_size, "%lu", eof_song->beat[eof_selected_beat]->ppqn);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "(Error)");
		}
		return 1;
	}

	//The selected beat's length based on its PPQN value and time signature
	if(!ustricmp(macro, "DEBUG_BEAT_TEMPO_LENGTH"))
	{
		if(eof_selected_beat < eof_song->beats)
		{
			unsigned num = 4, den = 4;
			double length = (double)eof_song->beat[eof_selected_beat]->ppqn / 1000.0;	//Calculate the length of the beat from its tempo (this is the formula "beat_length = 60000 / BPM", where BPM = 60000000 / ppqn)

			(void) eof_get_effective_ts(eof_song, &num, &den, eof_selected_beat, 0);	//Get the time signature in effect at the selected beat
			if(eof_song->tags->accurate_ts && (den != 4))
			{	//If the user enabled the accurate time signatures song property, and the time signature necessitates adjustment (isn't #/4)
				length *= 4.0 / (double)den;	//Adjust for the time signature
			}
			snprintf(dest_buffer, dest_buffer_size, "%f", length);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "(Error)");
		}
		return 1;
	}

	//The selected beat's length based on its relative position to the next beat
	if(!ustricmp(macro, "DEBUG_BEAT_POS_LENGTH"))
	{
		if(eof_selected_beat + 1 < eof_song->beats)
		{
			double length = eof_song->beat[eof_selected_beat + 1]->fpos - eof_song->beat[eof_selected_beat]->fpos;
			snprintf(dest_buffer, dest_buffer_size, "%f", length);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "(Error)");
		}
		return 1;
	}

	//The selected beat's expected position based on the previous beat's tempo
	if(!ustricmp(macro, "DEBUG_BEAT_POS_FROM_PREV_BEAT_TEMPO"))
	{
		if(eof_selected_beat < eof_song->beats)
		{
			double length = eof_calculate_beat_pos_by_prev_beat_tempo(eof_song, eof_selected_beat);

			snprintf(dest_buffer, dest_buffer_size, "%f", length);
		}
		else
		{
			snprintf(dest_buffer, dest_buffer_size, "(Error)");
		}
		return 1;
	}

	if(!ustricmp(macro, "DEBUG_eof_lclick_released"))
	{
		snprintf(dest_buffer, dest_buffer_size, "eof_lclick_released is %d", eof_lclick_released);
		return 1;
	}

	if(!ustricmp(macro, "DEBUG_eof_blclick_released"))
	{
		snprintf(dest_buffer, dest_buffer_size, "eof_blclick_released is %d", eof_blclick_released);
		return 1;
	}

	if(!ustricmp(macro, "RS_FIRST_TECH_NOTE_LACKING_TARGET"))
	{
		snprintf(dest_buffer, dest_buffer_size, "%s", eof_notes_macro_tech_note_missing_target);

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

int eof_read_macro_string(char *string, char *output_string)
{
	unsigned long ctr, index;

	if(!string || !output_string)
		return 0;	//Invalid parameters

	for(ctr = 0, index = 0; string[ctr] != '\0'; ctr++)
	{	//For each character in the input string
		if(string[ctr] == '_')
		{	//Underscore has special treatment
			if(string[ctr + 1] == '_')
			{	//If the next character is also underscore
				output_string[index++] = '_';	//Convert to a single underscore
				ctr++;	//An extra input character was used
			}
			else
			{
				output_string[index++] = ' ';	//Convert to a space character
			}
		}
		else
		{
			output_string[index++] = string[ctr];	//Copy the character as-is
		}
	}
	output_string[index] = '\0';	//Terminate the output string

	return 1;	//Success
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

	if((eof_log_level > 2) && !panel->logged)
	{	//If exhaustive logging is enabled and this panel hasn't been logged since it was loaded
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tRendering text panel for file \"%s\"", panel->filename);
		eof_log(eof_log_string, 3);
	}

	if(!panel->logged)
		eof_log("\t\tClearing window to gray", 3);
	if(opaque && !eof_background)
	{	//If the calling function specified opacity for the info panel and a background image is NOT loaded
		clear_to_color(panel->window->screen, eof_color_gray);
	}

	//Initialize the panel structure
	if(!panel->logged)
		eof_log("\t\tInitializing panel variables", 3);
	panel->ypos = 0;
	panel->xpos = 2;
	panel->color = panel->definedcolor = eof_color_white;
	panel->bgcolor = panel->definedbgcolor = -1;	//Transparent background for text
	panel->allowempty = 0;
	panel->timeformat = 0;
	panel->colorprinted = 0;
	panel->flush = 0;
	panel->newline = 0;
	panel->contentprinted = 0;
	panel->symbol = 0;
	panel->endline = 0;
	panel->endpanel = 0;

	if((eof_log_level > 2) && !panel->logged)
	{	//If exhaustive logging is enabled and this panel hasn't been logged since it was loaded
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
				if(!panel->logged)
					eof_log("\t\t\tPrinting line", 3);
				textout_ex(panel->window->screen, font, buffer2, panel->xpos, panel->ypos, panel->color, panel->bgcolor);	//Print this line to the screen
				panel->allowempty = 0;		//Reset this condition, it has to be enabled per-line
				panel->xpos = 2;			//Reset the x coordinate to the beginning of the line
				panel->ypos +=12;
				if(panel->colorprinted)
					panel->ypos += 3;		//Lower the y coordinate further so that one line of color background doesn't obscure another
				panel->contentprinted = 0;
				if(!panel->logged)
					eof_log("\t\t\tLine printed", 3);
			}

			dst_index = 0;	//Reset the destination buffer index
			src_index++;	//Seek past the line feed character in the source buffer
			if(panel->endpanel)
			{	//If the printing of this panel was signaled to end
				break;	//Break from while loop
			}
			linectr++;		//Track the line number being processed for debugging purposes
			if(!panel->logged)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tBeginning processing of panel text line #%lu", linectr);
				eof_log(eof_log_string, 3);
			}
			panel->colorprinted = 0;				//Reset this status
			panel->color = panel->definedcolor;		//Re-apply the last explicit text color (to override error/warning/info display macros)
			panel->bgcolor = panel->definedbgcolor;	//Ditto for background color
		}
		else
		{
			buffer[dst_index++] = thischar;	//Append the character to the destination buffer
		}
	}

	//Print any remaining content in the output buffer
	if(dst_index && (dst_index < TEXT_PANEL_BUFFER_SIZE))
	{	//If there are any characters in the destination buffer, and there is room in the buffer for the NULL terminator
		if(!panel->logged)
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
			if(!panel->logged)
				eof_log("\t\t\tPrinting line", 3);
			textout_ex(panel->window->screen, font, buffer2, panel->xpos, panel->ypos, panel->color, panel->bgcolor);	//Print this line to the screen
			panel->ypos +=12;
			if(!panel->logged)
				eof_log("\t\t\tLine printed", 3);
		}
	}

	//Draw a border around the edge of the notes panel
	if(!panel->logged)
		eof_log("\t\tPrinting completed.  Rendering panel border", 3);
	if(opaque)
	{	//But only if this panel wasn't meant to obscure whatever it's drawn on top of
		rect(panel->window->screen, 0, 0, panel->window->w - 1, panel->window->h - 1, eof_color_dark_silver);
		rect(panel->window->screen, 1, 1, panel->window->w - 2, panel->window->h - 2, eof_color_black);
		hline(panel->window->screen, 1, panel->window->h - 2, panel->window->w - 2, eof_color_white);
		vline(panel->window->screen, panel->window->w - 2, 1, panel->window->h - 2, eof_color_white);
	}

	if(!panel->logged)
		eof_log("\t\tText panel render complete.", 3);
	panel->logged = 1;	//Prevent repeated logging of this panel file
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
		if(eof_get_note_flags(eof_song, eof_selected_track, ctr) & flags)
		{	//If the note has the specified flag(s)
			count++;
		}
	}

	return count;
}

unsigned long eof_count_track_num_notes_with_tflag(unsigned long tflags)
{
	unsigned long ctr, count, tracksize;

	tracksize = eof_get_track_size(eof_song, eof_selected_track);
	for(ctr = 0, count = 0; ctr < tracksize; ctr++)
	{	//For each note in the track
		if(eof_get_note_tflags(eof_song, eof_selected_track, ctr) & tflags)
		{	//If the note has the specified temporary flag(s)
			count++;
		}
	}

	return count;
}

unsigned long eof_notes_panel_count_section_stats(unsigned long sectiontype, unsigned long *minptr, unsigned long *maxptr)
{
	unsigned long ctr, notectr, notepos, totalcount = 0, thiscount, tracknotecount, *sectioncount = NULL, min = 0, max = 0;
	EOF_PHRASE_SECTION *phrase = NULL;

	if(eof_lookup_track_section_type(eof_song, eof_selected_track, sectiontype, &sectioncount, &phrase))
	{	//If the array of this section type was found
		tracknotecount = eof_get_track_size(eof_song, eof_selected_track);

		notectr = 0;	//Start by examining the first note in the track
		notepos = eof_get_note_pos(eof_song, eof_selected_track, notectr);

		for(ctr = 0; ctr < *sectioncount; ctr++)
		{	//For each section of this type in the track
			EOF_PHRASE_SECTION *ptr = &phrase[ctr];	//Get a pointer to this section instance

			thiscount = 0;	//Reset this counter
			while(notepos <= ptr->end_pos)
			{	//For all notes at/before the end of this section instance
				if(notepos >= ptr->start_pos)
				{	//If the note is within the scope of this section instance
					thiscount++;	//Count the number of notes in this section instance
					totalcount++;	//Count the number of notes in all instances of this section
				}

				//Iterate to next note
				notectr++;
				if(notectr >= tracknotecount)
				{	//If all notes in the track have been examined
					break;	//Exit while loop
				}
				notepos = eof_get_note_pos(eof_song, eof_selected_track, notectr);
			}

			if(!min || (thiscount < min))
			{	//If this section has the least number of notes among all examined so far
				min = thiscount;
			}
			if(!max || (thiscount > max))
			{	//If this section has the most notes among all examined so far
				max = thiscount;
			}

			//Iterate to next note
			notectr++;
			if(notectr >= tracknotecount)
			{	//If all notes in the track have been examined
				break;	//Exit for loop
			}
			notepos = eof_get_note_pos(eof_song, eof_selected_track, notectr);
		}
	}

	//Return min and max values by reference if applicable
	if(minptr)
		*minptr = min;
	if(maxptr)
		*maxptr = max;

	return totalcount;
}

void eof_notes_panel_print_time(unsigned long time, char *dest_buffer, unsigned long dest_buffer_size, int timeformat)
{
	int min, sec, ms;

	if(!dest_buffer || !dest_buffer_size)
		return;

	if(!timeformat)
	{	//Display simply in milliseconds
		snprintf(dest_buffer, dest_buffer_size, "%lums", time);
	}
	else
	{	//Display in mm:ss.ms
		ms = time % 1000;
		min = (time / 1000) / 60;
		sec = (time / 1000) % 60;
		snprintf(dest_buffer, dest_buffer_size, "%02d:%02d.%03d", min, sec, ms);
	}
}
