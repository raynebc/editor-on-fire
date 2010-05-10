/*
 (c) 2008 Ryan Dickie
 todo: add error handling
	add windows code
	add multiple user timers *using only userspace calls... like in my ensc351 project
	release under gpl/friendly licence.
	if timer fails.. user should check errno

	windows: link with winmm.lib
	*nix: link with librt
*/
#ifndef _TIMER_H_
#define _TIMER_H_

#ifndef ALLEGRO_WINDOWS

#include <time.h>
#include <signal.h>
#include <semaphore.h>

/* main timer */
timer_t timerid;

/* wakeup and sleep semaphore */
sem_t timer_sem;

int timer_init();
int timer_cleanup();
void timer_callback();

#define timer_wait() sem_wait(&timer_sem)

void timer_callback()
{
	sem_post(&timer_sem);
}

/* on error.. timer returns -1. Check errno in this case */
int timer_init(int milliseconds)
{
	long nanoseconds = ((long) milliseconds) * 1000000;
	sem_init(&timer_sem, 0, 1);
	struct itimerspec timerspec;

	/* first shot */
	timerspec.it_value.tv_sec = 0; 
	timerspec.it_value.tv_nsec = nanoseconds; 
	/* regular interval */
	timerspec.it_interval.tv_sec = 0; 
	timerspec.it_interval.tv_nsec = nanoseconds; 
		
	/*creates 1 always running timer!!! */
	signal(SIGALRM, timer_callback);
	if (timer_create (CLOCK_REALTIME, NULL, &timerid))
		return -1;
 	if (timer_settime (timerid,0,&timerspec,NULL))
		return -1;

	return 0;
}

/*
 * possibly more to clean up
 * eg: check return value for timer_delete!
 */
int timer_cleanup()
{
	timer_delete(timerid);
	return 0;
}

#else


#include <winalleg.h>
#include <Mmsystem.h>

/* main timer */
MMRESULT timerid;

/* wakeup and sleep semaphore */
HANDLE timer_sem;

int timer_init();
int timer_cleanup();
void timer_callback();


#define resolution 1/*windows mm timer resolution in milliseconds*/
#define timer_wait() WaitForSingleObject(timer_sem,INFINITE) /* timeout value = 0 */

void CALLBACK TimerFunction(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2) 
{
	ReleaseSemaphore(timer_sem,1,NULL);
} 

/* todo: add return value */
int timer_init(int milliseconds)
{
	timer_sem = CreateSemaphore( NULL, 0, 3, NULL );
	if (timer_sem == NULL) 
	{
		return -1;
	}
	
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	UINT delay_time = milliseconds;
	timeBeginPeriod(resolution);
	
	DWORD data;
	timerid = timeSetEvent(delay_time, resolution, TimerFunction, data, TIME_PERIODIC );
	if (timerid == 0)
		return -1;

	return 0;
}

int timer_cleanup()
{
	MMRESULT err = timeKillEvent(timerid);
	if (err == TIMERR_NOERROR)
	   return -1;
	err = timeEndPeriod(resolution);
	return 0;
}

#endif /* ALLEGRO_WINDOWS */
#endif /* _TIMER_H_ */
