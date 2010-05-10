// Gemini System Idle Controller for C/C++
// ---------------------------------------
// H Header File

// Programmed By: Kris Asick (Gemini) [http://www.pixelships.com]
// If you intend to use this idle system in your own program, due credit is not
// required but would be appreciated! :)



// REQUIRED INCLUDES: allegro.h, winalleg.h, winbase.h



// BASIC INSTRUCTIONS:
//
// 1.	Include the g-idle.h and g-idle.c files in your project.
// 2.	Once you have initialized Allegro and its timers, call InitIdleSystem().
// 3.	Replace any rest() or Sleep() statements in your code with Idle(), or you may even
//		remove them entirely if you are performing vsyncing, page flipping or
//		triple buffering.
// 4.	If you do vsyncing, page flipping, or triple buffering, follow the additional
//		instructions outlined below for the method you are using.
// 5.	Add a call to StopIdleSystem() during shutdown. Make sure you call this BEFORE
//		Allegro is removed from memory, otherwise you may cause program crashes!



// IF YOU ARE VSYNCING:
// 1.	Add a call to PrepVSyncIdle() once you have set your video mode. If you change the
//		video mode at any time make sure you call this again!
// 2.	Add a call to IdleUntilVSync() immediately before you vsync.
// 3.	Add a call to DoneVSync() immediately after you vsync.

// IF YOU ARE PAGE FLIPPING:
// 1.	Add a call to PrepVSyncIdle() once you have set your video mode. If you change the
//		video mode at any time make sure you call this again!
// 2.	Add a call to IdleUntilVSync() immediately before show_video_bitmap().
// 3.	Add a call to DoneVSync() immediately after show_video_bitmap().

// IF YOU ARE TRIPLE BUFFERING:
// 1.	Add a call to IdleUntilPollScroll() immediately before request_video_bitmap().



// DETAILS:
//
// This idle system is designed to alleviate 100% CPU usage with Allegro driven projects
// made for Windows. It is not compatible as is with other platforms.
//
// The most common solutions to 100% CPU usage is to use the rest() or Sleep() statements,
// however, these rely on the thread scheduler, which is not very accurate. Using rest(1)
// can result in wait times as long as 50 msec, possibly destroying the framerate or
// making it erratic on some systems. rest(0) only gives up CPU time to threads of equal
// priority if necessary and thus does not alleviate CPU usage in any way.
//
// The solution to get accurate idling is to use interruptable non-busy waits, and the
// best way to incorporate those is by using semaphores. Semaphores are normally used to
// help keep multi-threaded applications synced. In this case, they are being used to sync
// an Allegro timer, driven by the Windows High Performance Counter, with your application.
//
// By using semaphores, the system at no point needs to rely on the thread scheduler for
// timing purposes and thus the granularity that makes rest() and Sleep() impractical for
// giving up precise amounts of CPU time is avoided entirely!



#ifndef __cplusplus



// ***************************************************************
// ** InitIdleSystem                                            **
// ** --------------------------------------------------------- **
// ** Starts up a special Allegro timer necessary to handle the **
// ** idle system. Also initializes a semaphore object which    **
// ** will be used to handle idling the current thread.         **
// ***************************************************************

int InitIdleSystem (void); // Returns 0 on success, 1 on timer failure, 2 on semaphore failure.



// ****************************************************************
// ** PrepVSyncIdle                                              **
// ** ---------------------------------------------------------- **
// ** In order to process VSyncing properly, the system needs    **
// ** to measure roughly how much time passes between each       **
// ** vertical retrace so that as much time as possible is spent **
// ** at idle before calling vsync() or show_video_bitmap(),     **
// ** You must call this AFTER setting a screen mode, and again  **
// ** if you change screen modes again.                          **
// ****************************************************************

void PrepVSyncIdle (void);



// ****************************************************************
// ** Idle                                                       **
// ** ---------------------------------------------------------- **
// ** Idles the program for the amount of time specified in      **
// ** milliseconds. Far more accurate than the rest() or Sleep() **
// ** functions normally used. Passing a number of 0 or less     **
// ** simply calls Sleep(0).                                     **
// ****************************************************************

void Idle (int msec);



// ***************************************************************
// ** IdleUntil____                                             **
// ** --------------------------------------------------------- **
// ** These functions idle the program until certain conditions **
// ** are met. IdleUntilVSync idles until a vertical retrace is **
// ** expected to begin and is to be used with VSyncing and     **
// ** Page Flipping. IdleUntilPollScroll should be used with    **
// ** Triple Buffering.                                         **
// ***************************************************************

void IdleUntilVSync (void);
void IdleUntilPollScroll (void);



// ****************************************************************
// ** DoneVSync                                                  **
// ** ---------------------------------------------------------- **
// ** In order for the timing to be correct, you must inform the **
// ** idle system manually that a VSync has started or that a    **
// ** page flip has been made. Please see the instructions for   **
// ** more information.                                          **
// ****************************************************************

void DoneVSync (void);



// ***************************************************************
// ** StopIdleSystem                                            **
// ** --------------------------------------------------------- **
// ** Shuts down the timer and closes the semaphore used by the **
// ** idle system. Make sure you call this BEFORE you close     **
// ** Allegro!                                                  **
// ***************************************************************

void StopIdleSystem (void);



#else

// The following is for C++ compatibility. I don't program in C, only C++, so I
// don't rightly know if the extern "C" stuff has to be defined separately or
// not for C++ users to allow it to work for C users, I just know it works this way. ;)

extern "C" int InitIdleSystem (void);
extern "C" void PrepVSyncIdle (void);
extern "C" void Idle (int msec);
extern "C" void IdleUntilVSync (void);
extern "C" void IdleUntilPollScroll (void);
extern "C" void DoneVSync (void);
extern "C" void StopIdleSystem (void);

#endif
