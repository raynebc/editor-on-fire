#ifndef EOF_PATHING_H
#define EOF_PATHING_H

#include "song.h"

typedef struct
{
	unsigned long note_start;		//The note at which this deployment started the last time it was processed, or ULONG_MAX if this cache entry is undefined
	unsigned long note_end;			//The first note after which the above star power deployment ended the last time it was processed
	unsigned long note_end_native;	//The track's native number for note index note_end
	unsigned long multiplier;		//The score multiplier in effect
	unsigned long hitcounter;		//The note hit counter in effect
	unsigned long score;			//The total score in effect
	unsigned long deployment_notes;	//The number of notes played during star power deployment
	double sp_meter;				//The star power meter level
	double sp_meter_t;				//The uncapped star power meter level
} EOF_SP_PATH_SCORING_STATE;

typedef struct
{
	unsigned long *deployments;					//An array defining the note index number of each deployment, ie deployments[0] being the first SP deployment, deployments[1] being the second, etc.
	EOF_SP_PATH_SCORING_STATE *deploy_cache;	//An array of data about score and star power status at the first note after each end of star power deployments from the previous solution evaluation,
												// allowing much/most of the score processing to be re-used between subsequent solutions where most of the deployments occur at the same indexes
	unsigned long deploy_count;					//The number of entries in the above deployment arrays (the detected maximum number star power deployments)
	unsigned long num_deployments;				//The number of star power deployments in this solution

	//These arrays are sized to only store data about the target track difficulty's notes and not all of the track's notes
	double *note_measure_positions;		//An array of double floats the defines the position (defined in measures) of each note in the specified track difficulty
	double *note_beat_lengths;			//An array of double floats defining the length (defined in beats) of each note in the specified track difficulty
	unsigned long note_count;			//The number of elements in the above arrays and the number of notes in the target track difficulty, used for bounds checking

	unsigned long track;				//The track number being processed
	unsigned char diff;					//The difficulty number being processed
	unsigned long score;				//The estimated score if all notes in the processed track difficulty are hit, and all sustain star power notes are whammied for bonus star power
	unsigned long deployment_notes;		//The number of notes played during star power deployment
	unsigned long solution_number;
} EOF_SP_PATH_SOLUTION;

#define EOF_PERSISTENT_WORKER 1
	//Specifies that worker processes will remain running and able to receive additional #.job files from the supervisor process
	//All jobs after the first will retain common information from the first job instead of requiring it to be defined again in the job
	// removing the need to read and write kilobytes of floating point data per job file
	//When the worker sees a #.kill file in the directory of its initial job, it will exit its main loop and end execution

#define EOF_SP_PATH_WORKER_IDLE 0
#define EOF_SP_PATH_WORKER_WAITING 1
#define EOF_SP_PATH_WORKER_RUNNING 2
#define EOF_SP_PATH_WORKER_FAILED 3
#define EOF_SP_PATH_WORKER_STOPPING 4
#define EOF_SP_PATH_WORKER_STOPPED 5
typedef struct
{
	int status;						//Tracks the process's status by way of one of the above EOF_SP_PATH_WORKER... macros
	unsigned long first_deploy;		//The first solution set being tested by the worker process
	unsigned long last_deploy;		//The last solution set being tested by the worker process
	unsigned long assignment_size;	//The last number of solution sets assigned to the worker process
	clock_t start_time;				//When the worker process was started
	clock_t end_time;				//When the worker process was detected to have completed
	unsigned long job_count;		//After a persistent worker has received its first job, subsequent job files will omit redundant job data (everything except the start/end solution numbers to test)
} EOF_SP_PATH_WORKER;

int eof_note_is_last_longest_gem(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if the note is the longest gem at its position (ie. for identifying the longest gem in a disjointed chord)
	//Returns nonzero if the note does not have disjointed status, as it is the longest note at its position
	//In the event of a tie between this gem and a later one being the same length, zero is returned
	//This allows SP pathing logic to only score SP bonus for one representative gem in a disjointed chord
	//Assumes all notes are sorted
	//Return zero on error

unsigned long eof_note_count_gems_extending_to_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos);
	//Returns the number of notes starting at the specified note's timestamp which extend to or beyond the specified position
	//Returns 1 if the specified note does not have disjointed status
	//Assumes all notes are sorted
	//Returns 0 on error

int eof_note_is_last_in_sp_phrase(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if the specified note has star power and either of the following is true:
	// 1.  There is not another note that follows it
	// 2.  There is another note that follows it, and it does not have star power

unsigned long eof_translate_track_diff_note_index(EOF_SONG *sp, unsigned long track, unsigned char diff, unsigned long index);
	//Accepts an index specifying the note number within a track difficulty and returns the real index for that note in the track's note[] array
	//Return ULONG_MAX if the index is invalid or upon error

double eof_get_measure_position(unsigned long pos);
	//Returns the specified timestamp in measures, using beat statistics generated by eof_process_beat_statistics
	//If beat statistics are not already cached, eof_process_beat_statistics() is called with focus on the active track
	//Returns 0.0 on error

unsigned long eof_ch_pathing_find_next_deployable_sp(EOF_SP_PATH_SOLUTION *solution, unsigned long start_index);
	//Given the beat length information about solution's track difficulty, start parsing notes in the track difficulty
	// starting at the specified note index and return the first note index at which 50% or more of the star power meter
	// has been added, allowing for star power to be deployed
	//Expects that the EOF_NOTE_TFLAG_SP_END flag has been set appropriately by eof_menu_track_find_ch_sp_path()
	//Returns ULONG_MAX on error

unsigned long eof_ch_pathing_find_next_sp_note(EOF_SP_PATH_SOLUTION *solution, unsigned long start_index);
	//Parses notes starting with the specified note index and returns the index of the first note having star power
	//Returns ULONG_MAX if there is no such note or upon error

int eof_evaluate_ch_sp_path_solution(EOF_SP_PATH_SOLUTION *solution, unsigned long solution_num, int logging);
	//Determines the validity of the proposed star path solution for the specified track difficulty, setting the score and deployment_notes value in the passed structure
	//The score is calculated with scoring rules for Clone Hero
	//The solution's score and deployment_notes variables are modified to contain the values calculated for the solution
	//Returns 0 on success, or otherwise one of the following error codes:
	// 1 for invalid parameters
	// 2 for invalidated by cache (ie. cached deployment data shows that a deployment was already in progress during one of the other deployment indexes in the solution)
	// 3 for attempting to deploy while star power is in effect
	// 4 for attemptign to deploy without sufficient star power
	// 5 5 for a general logic error
	//Returns zero if the proposed solution is invalid (ie. calling for star power deployment while it is already deployed, or when there is insufficient star power)
	//solution_num is the solution number being tested, to be logged and used for debugging
	// If this value is ULONG_MAX, the solution number is not logged
	//If logging is nonzero, scoring details such as the number points awarded per note, when star power deploys and ends, etc. is logged
	//If logging is greater than 1, verbose logging for each note's scoring, start and end of star power deployment, etc. is performed

int eof_ch_sp_path_single_process_solve(EOF_SP_PATH_SOLUTION *best, EOF_SP_PATH_SOLUTION *testing, unsigned long first_deploy, unsigned long last_deploy, unsigned long *validcount, unsigned long *invalidcount, unsigned long *deployment_notes);
	//Calculates all solutions where the first deployment starts at note index between first_deploy and last_deploy (inclusive),
	// comparing their scores with the provided best solution and updating its content accordingly
	//If all solutions are to be tested, ULONG_MAX should be specified for last_deploy
	//validcount and invalidcount are passed so the calling function can know how many solutions were tested
	//deployment_notes is used to track the highest count of notes that were found to be playable during all star power deployments of any solution
	//The testing structure is used as the working structure to store each solution's score, provided for the calling function to reduce overhead
	//Returns 0 on success, 1 on error or 2 on user cancellation

int eof_ch_sp_path_supervisor_process_solve(EOF_SP_PATH_SOLUTION *best, EOF_SP_PATH_SOLUTION *testing, unsigned long first_deploy, unsigned long worker_count, unsigned long *validcount, unsigned long *invalidcount, unsigned long *deployment_notes, int worker_logging);
	//Finds the best solution as eof_ch_sp_path_single_process_solve() does, but does so by running the specified number of worker EOF processes in parallel
	// to test all solution sets from first_deploy to testing->note_count
	//validcount and invalidcount are passed so the calling function can know how many solutions were tested among all worker processes
	//deployment_notes is used to track the highest count of notes that were found to be playable during all star power deployments of any solution
	//The testing structure is used as the working structure to store each solution's score, provided for the calling function to reduce overhead
	//If worker_logging is nonzero, the worker processes are launched with the -ch_sp_path_worker_logging parameter so they perform logging each in their own file
	//Returns 0 on success, 1 on error or 2 on user cancellation

void eof_ch_pathing_mark_tflags(EOF_SP_PATH_SOLUTION *solution);
	//Parses the notes in the solution's target track difficulty, applying the EOF_NOTE_TFLAG_SOLO_NOTE tflag
	// Applies the EOF_NOTE_TFLAG_SOLO_NOTE tflag to each note that is in a solo phrase
	// Applies the EOF_NOTE_TFLAG_SP_END tflag to the last note of each star power phrase

int eof_ch_sp_path_setup(EOF_SP_PATH_SOLUTION **bestptr, EOF_SP_PATH_SOLUTION **testingptr, char *undo_made);
	//Performs all needed steps to prepare for the use of the pathing functions, such as creating the solution structures, preparing the active track difficulty, etc
	//The addresses for the newly created structures are assigned to *bestptr and *testingptr for use by the calling function
	//If a time signature is missing from the first beat, a 4/4 time signature is applied to it after creating an undo state, and *undo_made is set to 1 if the pointer is not NULL
	//Returns 1 on error or 2 on cancellation (no notes in active track difficulty)

int eof_menu_track_find_ch_sp_path(void);
	//Determines optimum star power deployment for the active track difficulty

void eof_ch_sp_path_worker(char *job_file);
	//When EOF is launched with the ch_sp_path_worker parameter, this function is called after minimal program initialization to process the specified ####.job file
	//If the job completes successfully, a ###.results file is created at the same path as the job file
	//If the job fails due to error, a ###.fail file is created at the same path as the job file
	//If the job is cancelled by the user (ie. Escape keypress), a ###.cancel file is created at the same path as the job file
	//The specified job file is deleted after the above file is created

#endif
