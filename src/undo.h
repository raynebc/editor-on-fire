#ifndef EOF_UNDO_H
#define EOF_UNDO_H

#include "main.h"
#include "note.h"
#include "beat.h"
#include "event.h"

#define EOF_MAX_UNDO 100
#define EOF_UNDO_TYPE_NONE         0
#define EOF_UNDO_TYPE_NOTE_LENGTH  1
#define EOF_UNDO_TYPE_NOTE_SEL     2	//Perform a generic undo state and deselect all notes
#define EOF_UNDO_TYPE_LYRIC_NOTE   3
#define EOF_UNDO_TYPE_RECORD       4
#define EOF_UNDO_TYPE_SILENCE      5
#define EOF_UNDO_TYPE_TEMPO_ADJUST 6

extern int eof_undo_count;
extern int eof_redo_count;
extern int eof_undo_last_type;
extern char * eof_undo_filename[EOF_MAX_UNDO];
extern int eof_undo_states_initialized;
extern int eof_undo_current_index;
extern int eof_undo_in_progress;
extern int eof_project_unsaved;

void eof_undo_reset(void);	//Clears all undo states
int eof_undo_add(int type);	//Adds another undo state, except some cases where the requested undo type is the same as the previous undo (ie. a note addition/deletion)
int eof_remove_undo(void);	//Removes the latest undo state.  Returns nonzero if an undo state was removed
int eof_undo_apply(void);	//Saves the redo state and applies the next available undo state
void eof_redo_apply(void);	//Applies the redo state
int eof_undo_load_state(const char * fn);	//Applies the specified undo/redo state, or returns 0 upon error (called by eof_undo_apply() and eof_redo_apply())
void eof_destroy_undo(void);	//Frees the memory for all of the filenames in eof_undo_filename[]

#endif
