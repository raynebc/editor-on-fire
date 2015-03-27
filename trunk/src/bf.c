#include "bf.h"
#include "main.h"
#include "rs.h"
#include "song.h"
#include "foflc/RS_parse.h"	//For expand_xml_text()
#include "menu/track.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int eof_export_bandfuse(EOF_SONG * sp, char * fn, unsigned short *user_warned)
{
	PACKFILE * fp;
	char buffer[600] = {0}, buffer2[600] = {0};
	EOF_PRO_GUITAR_TRACK *tp;
	char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct notes are exported
	unsigned long ctr, ctr2, ctr3, ctr4, ctr5, gemcount, bitmask, guitartracknum = 0, basstracknum = 0;

	eof_log("eof_export_bandfuse() entered", 1);

	if(!sp || !fn || !sp->beats)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open file for writing", 1);
		return 0;	//Return failure
	}

	//Write the song metadata
	(void) pack_fputs("<?xml version='1.0' encoding='UTF-8'?>\n", fp);
	(void) pack_fputs("<Bandfuse>\n", fp);
	(void) pack_fputs("<!-- " EOF_VERSION_STRING " -->\n", fp);	//Write EOF's version in an XML comment
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->title, 64);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <title>%s</title>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->artist, 256);	//Replace any special characters in the artist song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <artistName>%s</artistName>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->album, 256);	//Replace any special characters in the album song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <albumName>%s</albumName>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->year, 32);	//Replace any special characters in the year song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <albumYear>%s</albumYear>\n", buffer2);
	(void) pack_fputs(buffer, fp);

	///Write tempo changes

	for(ctr = 1; ctr < sp->tracks; ctr++)
	{	//For each track
		if(sp->track[ctr]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If this is a pro guitar/bass track
			tp = sp->pro_guitar_track[sp->track[ctr]->tracknum];
			restore_tech_view = eof_menu_track_get_tech_view_state(sp, ctr);
			eof_menu_track_set_tech_view_state(sp, ctr, 0);	//Disable tech view if applicable

			if(tp->notes)
			{	//If this track is populated
				eof_determine_phrase_status(sp, ctr);	//Update the trill and tremolo status of each note
				if(tp->arrangement != 4)
				{	//If this isn't a bass track
					guitartracknum++;	//Keep count of how many of this track type there have been
					(void) snprintf(buffer, sizeof(buffer) - 1, "  <arrangement=\"Guitar %lu (%s)\">\n", guitartracknum, sp->track[ctr]->name);
					(void) pack_fputs(buffer, fp);
				}
				else
				{	//This is a bass track
					basstracknum++;	//Keep count of how many of this track type there have been
					(void) snprintf(buffer, sizeof(buffer) - 1, "  <arrangement=\"Bass %lu (%s)\">\n", guitartracknum, sp->track[ctr]->name);
					(void) pack_fputs(buffer, fp);
				}
				(void) snprintf(buffer, sizeof(buffer) - 1, "    <tuning string0=\"%d\" string1=\"%d\" string2=\"%d\" string3=\"%d\" string4=\"%d\" string5=\"%d\" />\n", tp->tuning[0], tp->tuning[1], tp->tuning[2], tp->tuning[3], tp->tuning[4], tp->tuning[5]);
				(void) pack_fputs(buffer, fp);

				(void) eof_detect_difficulties(sp, ctr);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for this track
				for(ctr2 = 0; ctr2 < 6; ctr2++)
				{	//For each of the first 6 difficulties
					unsigned long anchorcount;
					char anchorsgenerated = 0;	//Is set to nonzero if fret hand positions are automatically generated for this difficulty and will have to be deleted

					if(eof_track_diff_populated_status[ctr2])
					{	//If this difficulty is populated
						(void) snprintf(buffer, sizeof(buffer) - 1, "    <level difficulty=\"%lu\">\n", ctr2);
						(void) pack_fputs(buffer, fp);

						//Count the number of note gems in this track difficulty
						for(ctr3 = 0, gemcount = 0; ctr3 < tp->notes; ctr3++)
						{	//For each note in the track
							if(eof_get_note_type(sp, ctr, ctr3) == ctr2)
							{	//If the note is in this difficulty
								gemcount += eof_note_count_rs_lanes(sp, ctr, ctr3, 2);	//Add this note's number of non-ghosted gems to a counter
							}
						}
						(void) snprintf(buffer, sizeof(buffer) - 1, "      <notes count=\"%lu\">\n", gemcount);
						(void) pack_fputs(buffer, fp);

						//Generate fret hand positions if there are none for this difficulty
						for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)
						{	//For each hand position defined in the track
							if(tp->handposition[ctr3].difficulty == ctr)
							{	//If the hand position is in this difficulty
								anchorcount++;
							}
						}
						if(!anchorcount)
						{	//If there are no anchors in this track difficulty, automatically generate them
							if((*user_warned & 1) == 0)
							{	//If the user wasn't alerted that one or more track difficulties have no fret hand positions defined
								allegro_message("Warning:  At least one track difficulty has no fret hand positions defined.  They will be created automatically.");
								*user_warned |= 1;
							}
							eof_fret_hand_position_list_dialog_undo_made = 1;	//Ensure no undo state is written during export
							eof_generate_efficient_hand_positions(sp, ctr, ctr2, 0, 0);	//Generate the fret hand positions for the track difficulty being currently written (use a static fret range tolerance of 4 for all frets)
							anchorsgenerated = 1;
						}
						for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)	//Re-count the hand positions
						{	//For each hand position defined in the track
							if(tp->handposition[ctr3].difficulty == ctr)
							{	//If the hand position is in this difficulty
								anchorcount++;
							}
						}

						//Write the notes to XML
						for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
						{	//For each note in the track
							if(eof_get_note_type(sp, ctr, ctr3) == ctr2)
							{	//If the note is in this difficulty
								for(ctr4 = 0, bitmask = 1; ctr4 < tp->numstrings; ctr4++, bitmask <<= 1)
								{	//For each string in this track
									if((tp->note[ctr3]->note & bitmask) && !(tp->note[ctr3]->ghost & bitmask))
									{	//If this string has a gem and it's not ghosted
										EOF_RS_TECHNIQUES tech;
										unsigned long notepos;
										unsigned long fret, finger;				//The fret and finger numbers used to play the gem on this string
										unsigned long trill_start, trill_stop;	//The start and stop positions of the note's trill phrase
										int trillwith = -1;						//If the note is in a trill phrase, this is set to the fret value of the next or previous gem on the string
										long next;
										unsigned long stringnum;				//The converted string number (ie. low E string is string 6)
										char *sustainpadding;

										trill_start = trill_stop = tp->note[ctr3]->pos;	//Initialize variables in order to track if the trill phrase can't be properly found
										if(tp->note[ctr3]->flags & EOF_NOTE_FLAG_IS_TRILL)
										{	//If this note is in a trill phrase
											//Find which trill phrase it's in
											for(ctr5 = 0; ctr5 < tp->trills; ctr5++)
											{	//For each trill phrase
												if((tp->trill[ctr5].difficulty == 0xFF) || (tp->trill[ctr5].difficulty == ctr2))
												{	//If this trill phrase applies to all difficulties or if it otherwise applies to this note's difficulty
													if((tp->trill[ctr5].start_pos <= tp->note[ctr3]->pos) && (tp->trill[ctr5].end_pos >= tp->note[ctr3]->pos))
													{	//If the note is inside this trill phrase
														trill_start = tp->trill[ctr5].start_pos;
														trill_stop = tp->trill[ctr5].end_pos;
														break;
													}
												}
											}
											//Determine if this is the first note in the trill phrase, and if so, what note it trills with
											if(trill_start != trill_stop)
											{	//If a trill phrase was identified
												for(ctr5 = 0; ctr5 < tp->notes; ctr5++)
												{	//For each note in the track
													if(eof_get_note_type(sp, ctr, ctr3) == ctr2)
													{	//If the note is in this difficulty
														if(tp->note[ctr5]->pos > trill_stop)
															break;	//Stop looking at notes if the trill has already been passed
														if(tp->note[ctr5]->pos >= trill_start)
														{	//If this note is the first encountered within the target trill phrase
															if(ctr5 != ctr3)
															{	//If the note being written to XML is not this note
																break;	//Don't export it with trill technique
															}
															next = eof_fixup_next_pro_guitar_note(tp, ctr5);
															if((next > 0) && (tp->note[next]->pos <= trill_stop))
															{	//If the next note in the track difficulty is also in this trill phrase
																if((tp->note[next]->note & bitmask) && ((tp->note[next]->frets[next] & 0x7F) > (tp->note[ctr3]->frets[ctr4] & 0x7F)))
																{	//If the next note has a gem on the target string and its fret value is higher than that of the note being currently written to XML
																	trillwith = tp->note[next]->frets[ctr4] & 0x7F;	//This is the fret value being trilled with
																}
															}
															break;
														}
													}//If the note is in this difficulty
												}//For each note in the track
											}//If a trill phrase was identified
										}//If this note is in a trill phrase

										(void) eof_get_rs_techniques(sp, ctr, ctr3, ctr4, &tech, 2, 1);	//Determine techniques used by this note (honoring technotes where applicable)
										notepos = eof_get_note_pos(sp, ctr, ctr3);
										fret = tp->note[ctr3]->frets[ctr4] & 0x7F;	//Get the fret value for this string (mask out the muting bit)

										//Determine the fingering for the note
										finger = 1;									//Unless a more suitable fingering for this note can be determined, assume 1 (index)
										if(fret == 0)
										{	//If this is an open note
											finger = 0;	//The note has no fingering
										}
										else if(tp->note[ctr3]->finger[ctr4])
										{	//If this string's fingering is defined for this note
											finger = tp->note[ctr3]->finger[ctr4];	//Use it
										}
										else
										{
											unsigned char fhp = eof_pro_guitar_track_find_effective_fret_hand_position(tp, ctr2, notepos);	//Determine what fret hand position is in effect at this note
											if(fret >= fhp)
											{	//If the fret hand position in effect is at or below the note's fret
												if(fret < fhp + 4)
												{	//If the fret is within 4 frets of the position
													finger = fret + 1 - fhp;	//The index finger is at the fret hand position, each other finger is one fret further away
												}
												else
												{	//Otherwise assume the pinky is stretching to play this fret
													finger = 4;
												}
											}
										}

										if(tech.length < 10)
										{	//The sustain is one digit
											sustainpadding = "   ";
										}
										else if(tech.length < 100)
										{	//The sustain is two digits
											sustainpadding = "  ";
										}
										else if(tech.length < 1000)
										{	//The sustain is three digits
											sustainpadding = " ";
										}
										else
										{	//It's four or more digits
											sustainpadding = "";
										}
										fret += tp->capo;							//Compensate for the the capo position if applicable
										stringnum = tp->numstrings - ctr4;			//Convert to tab string numbering (low E is string 6 in a 6 string track, string 4 on a 4 string track, etc)
										(void) snprintf(buffer, sizeof(buffer) - 1, "        <note time=\"%lu\" linkNext=\"%d\" bend=\"%lu\" fret=%s\"%lu\" hammerOn=\"%d\" harmonic=\"%d\" hopo=\"%d\" ignore=\"0\" mute=\"%d\" palmMute=\"%d\" pluck=\"%d\" pullOff=\"%d\" slap=\"%d\" slideTo=\"%ld\" string=\"%lu\" sustain=%s\"%lu\" tremolo=\"%d\" harmonicPinch=\"%d\" slideUnpitchTo=\"%ld\" trillwith=\"%d\" finger=\"%lu\"/>\n", notepos, tech.linknext, tech.bendstrength_h, ((fret < 10) ? " " : ""), fret, tech.hammeron, tech.harmonic, tech.hopo, tech.stringmute, tech.palmmute, tech.pop, tech.pulloff, tech.slap, tech.slideto, stringnum, sustainpadding, tech.length, tech.tremolo, tech.pinchharmonic, tech.unpitchedslideto, trillwith, finger);
										(void) pack_fputs(buffer, fp);
									}//If this string has a gem and it's not ghosted
								}//For each string in this track
							}//If the note is in this difficulty
						}//For each note in the track

						//Remove any automatically generated fret hand positions
						if(anchorsgenerated)
						{	//If anchors were automatically generated for this track difficulty, remove them now
							for(ctr3 = tp->handpositions; ctr3 > 0; ctr3--)
							{	//For each hand position defined in the track, in reverse order
								if(tp->handposition[ctr3 - 1].difficulty == ctr2)
								{	//If the hand position is in this difficulty
									eof_pro_guitar_track_delete_hand_position(tp, ctr3 - 1);	//Delete the hand position
								}
							}
						}
						(void) pack_fputs("      </notes>\n", fp);
						(void) pack_fputs("    </level>\n", fp);
					}//If this difficulty is populated
				}//For each of the first 6 difficulties
				(void) pack_fputs("  </arrangement>\n", fp);
			}//If this track is populated

			eof_menu_track_set_tech_view_state(sp, ctr, restore_tech_view);	//Re-enable tech view if applicable
		}//If this is a pro guitar/bass track
	}//For each track
	(void) pack_fputs("</Bandfuse>\n", fp);

	//Cleanup
	(void) pack_fclose(fp);

	return 1;	//Return success
}
