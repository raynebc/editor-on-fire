#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ID3_parse.h"

int SearchValues(FILE *inf,unsigned long *pos,const unsigned char *phrase,unsigned long phraselen,unsigned char autoseek)
{
	unsigned long originalpos;
	unsigned long matchpos;
	unsigned char c;
	unsigned long ctr=0;		//Used to index into the phrase[] array, beginning with the first character
	unsigned char success=0;

//Validate input
	if(!inf || !phrase)	//These input parameters are not allowed to be NULL
		return -1;

	if(!phraselen)
		return 0;	//There will be no matches to an empty search array

//Initialize for search
	errno=0;
	originalpos=ftell(inf);	//Store the original file position
	if(errno)		//If there was an I/O error
		return -1;

	c=fgetc(inf);		//Read the first character of the file
	if(ferror(inf))		//If there was an I/O error
		return -1;

//Perform search
	while(!feof(inf))	//While end of file hasn't been reached
	{
	//Check if the next character in the search phrase has been matched
		if(c == phrase[ctr])
		{	//The next character was matched
			if(ctr == 0)	//The match was with the first character in the search array
			{
				matchpos=ftell(inf)-1;	//Store the position of this potential match (rewound one byte)
				if(ferror(inf))			//If there was an I/O error
					return -1;
			}
			ctr++;	//Advance to the next character in search array
			if(ctr == phraselen)
			{	//If all characters have been matched
				success=1;
				break;
			}
		}
		else	//Character did not match
			ctr=0;			//Ensure that the first character in the search array is looked for

		c=fgetc(inf);	//Read the next character of the file
	}

//Seek to the appropriate file position
	if(success && autoseek)		//If we should seek to the successful match
		fseek(inf,matchpos,SEEK_SET);
	else				//If we should return to the original file position
		fseek(inf,originalpos,SEEK_SET);

	if(ferror(inf))		//If there was an I/O error
		return -1;

//Return match/non match
	if(success)
	{
		if(pos != NULL)
			*pos=matchpos;
		return 1;	//Return match
	}

	return 0;	//Return no match
}

char *ReadTextInfoFrame(FILE *inf)
{
	//Expects the file position of inf to be at the beginning of an ID3 Frame header for a Text Information Frame
	//The first byte read (which will be from the Frame ID) is expected to be 'T' as per ID3v2 specifications
	//Reads and returns the string from the frame in a newly-allocated string
	//NULL is returned if the encoding is Unicode, if the frame is malformed or if there is an I/O error
	unsigned long originalpos=0;
	char buffer[11]={0};	//Used to store the ID3 frame header and the string encoding byte
	char *string=NULL;
	unsigned long framesize=0;

	if(inf == NULL)
		return NULL;

//Store the original file position
	originalpos=ftell(inf);
	if(ferror(inf))		//If there was a file I/O error
		return NULL;

	if(fread(buffer,11,1,inf) != 1)	//Read the frame header and the following byte (string encoding)
		return NULL;			//If there was an I/O error, return error

//Validate frame ID begins with 'T', ensure that the compression and encryption bits are not set, ensure Unicode string encoding is not specified
	if((buffer[0] != 'T') || ((buffer[9] & 192) != 0) || (buffer[10] != 0))
	{
		fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;				//Return error
	}

//Read the frame size as a Big Endian integer (4 bytes)
	framesize=((unsigned long)buffer[4]<<24) | ((unsigned long)buffer[5]<<16) | ((unsigned long)buffer[6]<<8) | ((unsigned long)buffer[7]);	//Convert to 4 byte integer

	if(framesize < 2)	//The smallest valid frame size is 2, one for the encoding type and one for a null terminator (empty string)
	{
		fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;
	}

	string=malloc(framesize);	//Allocate enough space to store the data (one less than the framesize, yet one more for NULL terminator)
	if(!string)			//Memory allocation failure
	{
		fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;
	}

//Read the data and NULL terminate the string
	if(fread(string,framesize-1,1,inf) != 1)	//If the data failed to be read
	{
		fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		free(string);
		return NULL;
	}
	string[framesize-1]='\0';

	return string;
}

//int ReadTags(FILE *inf,EOF_SONG *sp)
int ReadTags(FILE *inf)
{
	const char *tags[]={"TIT2","TYER","TPE1"};	//The three tags to try to find in the file
	char *temp;
	int ctr;
	unsigned long ctr2,length;	//Used to validate year tag
	unsigned char yearvalid=1;	//Used to validate year tag
	unsigned char tagcount=0;

//	if(!inf || !sp)
	if(!inf)
		return 0;

	for(ctr=0;ctr<3;ctr++)
	{	//For each tag we're looking for
		rewind(inf);		//Rewind to beginning of file
		if(ferror(inf))		//If there was a file I/O error
			return 0;

		if(SearchValues(inf,NULL,tags[ctr],4,1) == 1)
		{	//If there is a match for this tag, seek to it
			temp=ReadTextInfoFrame(inf);	//Attempt to read it

			if(temp != NULL)
			{
				switch(ctr)
				{
					case 0:	//Store Title tag, truncated to fit
						if(strlen(temp) > 255)	//If this string won't fit without overflowing
							temp[255] = '\0';	//Make it fit
//						strcpy(sp->title,temp);
						printf("Tag: %s = \"%s\"\n",tags[ctr],temp);
						free(temp);
					break;

					case 1:	//Store Year tag, after it's validated
						length=strlen(temp);
						if(length < 5)
						{	//If the string is no more than 4 characters
							for(ctr2=0;ctr2<length;ctr2++)	//Check all digits to ensure they're numerical
								if(!isdigit(temp[ctr2]))
									yearvalid=0;

							if(yearvalid)
//								strcpy(sp->year,temp);
								printf("Tag: %s = \"%s\"\n",tags[ctr],temp);

							free(temp);
						}
					break;

					case 2: //Store Performer tag, truncated to fit
						if(strlen(temp) > 255)	//If this string won't fit without overflowing
							temp[255] = '\0';	//Make it fit
//						strcpy(sp->artist,temp);
						printf("Tag: %s = \"%s\"\n",tags[ctr],temp);

						free(temp);
					break;

					default:	//This shouldn't be reachable
						free(temp);
					return tagcount;
				}

				tagcount++;	//One more tag was read
			}//if(temp != NULL)
		}//If there is a match for this tag
	}//For each tag we're looking for

	return tagcount;
}
