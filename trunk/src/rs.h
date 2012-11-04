#ifndef EOF_RS_H
#define EOF_RS_H

int eof_export_rocksmith_track(EOF_SONG * sp, char * fn, unsigned long track);
	//Writes the specified pro guitar track in Rocksmith's XML format
	//fn is expected to point to an array at least 1024 bytes in size.  It's filename is altered to reflect the track's name (ie /PART REAL_GUITAR.xml)

#endif
