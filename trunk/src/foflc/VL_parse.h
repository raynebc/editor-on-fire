#ifndef _vl_parse_h_
#define _vl_parse_h_

//
//STRUCTURE DEFINITIONS
//
struct VL_Text_entry{
	char *text;					//The text associated with this text entry
	struct VL_Text_entry *next;	//Pointer to next link
};

struct VL_Sync_entry{
	unsigned short lyric_number;//The lyric line associated with the sync entry (0xFFFF=no lyrics exist for the entry->skip)
	unsigned short start_char;	//The index within this lyric line that marks the start of the sync entry (0xFFFF=no lyrics exist for the entry->skip)
	unsigned short end_char;	//The index within this lyric line that marks the end of the sync entry (0xFFFF=the entry extends to the end of the lyric line)
	unsigned long start_time;	//The starting time of the lyric line in hundredths of seconds
	unsigned long end_time;		//The ending time of the lyric line in hundredths of seconds
	struct VL_Sync_entry *next;	//Pointer to next link
};

struct _VLSTRUCT_{
	unsigned long numlines;	//The number of lines of lyrics
	unsigned long numsyncs;	//The number of sync entries
	struct VL_Text_entry *Lyrics;	//Pointer to the linked list containing the text chunk entries
	struct VL_Sync_entry *Syncs;	//Pointer to the linked list containing the sync chunk entries
	unsigned long filesize;	//This should be the filesize - file header size (8 bytes)
	unsigned long textsize;	//Text chunk size.  Should be the distance between the text and sync headers - text header size (8 bytes)
	unsigned long syncsize;	//Sync chunk size.  Should be the distance between the sync header and EOF - sync header size (8 bytes)
};


int VL_PreLoad(FILE *inf,char validate);
	//Read from file to build the VL structure.  Performs header loading and validation.
	//If validate is nonzero, function returns nonzero upon error instead of exiting the program
void VL_Load(FILE *inf);
	//Uses the VL structure to build the lyric structure
void Export_VL(FILE *outf);
	//Exports the loaded lyric structure to output file in VL format
struct _VLSTRUCT_ *VL_PreWrite(void);
	//Uses lyric structure to create a VL structure for VL export.  A VL structure is allocated, populated and returned by reference
int ReadSyncEntry(struct VL_Sync_entry *ptr,FILE *inf);
	//Reads sync entry from input file into referenced structure.  Returns zero upon successful read, nonzero upon EOF
void ReleaseVL(void);
	//Releases memory from the VL structure, to be called after VL_Load


//
//EXTERNAL GLOBAL VAR DEFINITIONS
//
extern struct _VLSTRUCT_ VL;


#endif //#ifndef _vl_parse_h_
