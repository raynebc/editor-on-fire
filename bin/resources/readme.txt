You can save files in here to be used instead of default graphics that EOF keeps in eof.dat.


Graphics:  EOF was designed with the gem graphics being 48x48, but other sizes may be suitable as long as you save the images in PCX format.  "Selected" means that the note is selected, is being moused over or is the note underneath the seek line during chart playback.  File names are not case-sensitive in Windows, but they are in Linux and probably also in Mac OS:

Gems that are not selected:
note_green.pcx
note_red.pcx
note_yellow.pcx
note_blue.pcx
note_orange.pcx
note_purple.pcx
note_white.pcx
note_black.pcx

Gems that are selected:
note_green_hit.pcx
note_red_hit.pcx
note_yellow_hit.pcx
note_blue_hit.pcx
note_orange_hit.pcx
note_purple_hit.pcx
note_white_hit.pcx
note_black_hit.pcx

HOPO gems that are not selected:
note_green_hopo.pcx
note_red_hopo.pcx
note_yellow_hopo.pcx
note_blue_hopo.pcx
note_orange_hopo.pcx
note_purple_hopo.pcx
note_white_hopo.pcx

HOPO gems that are selected:
note_green_hopo_hit.pcx
note_red_hopo_hit.pcx
note_yellow_hopo_hit.pcx
note_blue_hopo_hit.pcx
note_orange_hopo_hit.pcx
note_purple_hopo_hit.pcx
note_white_hopo_hit.pcx

Dance notes that are not selected:
note_green_arrow.pcx
note_red_arrow.pcx
note_yellow_arrow.pcx
note_blue_hit_arrow.pcx

Dance notes that are selected:
note_green_hit_arrow.pcx
note_red_hit_arrow.pcx
note_yellow_hit_arrow.pcx
note_blue_hit_arrow.pcx

Cymbals that are not selected:
note_green_cymbal.pcx
note_yellow_cymbal.pcx
note_blue_cymbal.pcx
note_orange_cymbal.pcx
note_purple_cymbal.pcx
note_white_cymbal.pcx

Cymbals that are selected:
note_green_hit_cymbal.pcx
note_yellow_hit_cymbal.pcx
note_blue_hit_cymbal.pcx
note_orange_hit_cymbal.pcx
note_purple_hit_cymbal.pcx
note_white_hit_cymbal.pcx

GHL gems that are not selected:
note_ghl_black.pcx
note_ghl_white.pcx
note_ghl_barre.pcx

GHL gems that are selected:
note_ghl_black_hit.pcx
note_ghl_white_hit.pcx
note_ghl_barre_hit.pcx

HOPO GHL gems that are not selected:
note_ghl_black_hopo.pcx
note_ghl_white_hopo.pcx
note_ghl_barre_hopo.pcx

HOPO GHL gems that are selected:
note_ghl_black_hopo_hit.pcx
note_ghl_white_hopo_hit.pcx
note_ghl_barre_hopo_hit.pcx

GHL gems that have star power:
note_ghl_black_sp.pcx
note_ghl_white_sp.pcx
note_ghl_barre_sp.pcx

GHL gems that have star power and are selected:
note_ghl_black_sp_hit.pcx
note_ghl_white_sp_hit.pcx
note_ghl_barre_sp_hit.pcx

HOPO GHL gems that have star power and are not selected:
note_ghl_black_sp_hopo.pcx
note_ghl_white_sp_hopo.pcx
note_ghl_barre_sp_hopo.pcx

HOPO GHL gems that have star power and are selected:
note_ghl_black_sp_hopo_hit.pcx
note_ghl_white_sp_hopo_hit.pcx
note_ghl_barre_sp_hopo_hit.pcx

Slider notes:
note_green_slider.pcx
note_red_slider.pcx
note_yellow_slider.pcx
note_blue_slider.pcx
note_orange_slider.pcx
note_purple_slider.pcx
note_white_slider.pcx
note_black_slider.pcx

Slider GHL notes:
note_ghl_white_slider.pcx
note_ghl_black_slider.pcx
note_ghl_barre_slider.pcx

Slider GHL notes that have star power:
note_ghl_white_sp_slider.pcx
note_ghl_black_sp_slider.pcx
note_ghl_barre_sp_slider.pcx


Fonts:  The fonts are defined as PCX image files where each glyph is white pixels with purple transparent background.  Glyphs are separated by the yellow background of the PCX file so that Allegro can distinguish them from each other.

font_times_new_roman.pcx	The main font in EOF that is used in most situations.
font_courier_new.pcx		A mono-space font used to print various text in the piano roll (timestamps, tempos, time signatures, key signatures measure numbers, beat marker arrows, capo position), the seek time next to the playback controls.
font_symbols.pcx		A font used to draw fret numbers and any special symbols used for displaying a note's applied techniques and statuses below the piano roll.


Sound effects:  The audio files are generally defined as 16 bit, mono, 44.1KHz sample rate PCM WAV.

clap.wav		The clap sound cue.
metronome.wav		The metronome sound cue.
metronome_low.wav	The metronome sound cue used for beats that are not the first beat in a measure if the "Use multi-pitch metronome" option is enabled in "Sound>Audio cues".
gridsnap.wav		The sound played when changing the grid snap or seeking by grid snap when Feedback input mode is in effect.


A background image displayed in place of the gray background behind the piano roll, 3D preview, info panel, etc:
background.pcx
