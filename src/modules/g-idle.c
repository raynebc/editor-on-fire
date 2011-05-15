// Gemini System Idle Controller for C/C++
// ---------------------------------------
// C Source File

// Programmed By: Kris Asick (Gemini) [http://www.pixelships.com]
// If you intend to use this idle system in your own program, due credit is not
// required but would be appreciated! :)

// Please see the g-idle.h file for instructions on how to use.



#include <allegro.h>

#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
	#include <winbase.h>
#endif

#include "g-idle.h"



#ifdef ALLEGRO_WINDOWS
	HANDLE hSema;
#endif

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

volatile int tickcount = 0;
int vsync_ticks = 0;
int sys_active = 0;



void _timerticker (void)
{
	#ifdef ALLEGRO_WINDOWS
		ReleaseSemaphore(hSema,1,NULL);
		tickcount++;
	#endif
}
END_OF_FUNCTION(_timerticker);



int allegro_InitIdleSystem (void)
{
	return 0;
}

void allegro_PrepVSyncIdle (void)
{
}

void allegro_Idle (int msec)
{
	rest(msec);
}

void allegro_IdleUntilVSync (void)
{
}

void allegro_IdleUntilPollScroll (void)
{
}

void allegro_DoneVSync (void)
{
}

void allegro_StopIdleSystem (void)
{
}

int InitIdleSystem (void)
{
	#ifdef ALLEGRO_WINDOWS
		LOCK_FUNCTION(_timerticker);
		LOCK_VARIABLE(hSema);
		LOCK_VARIABLE(tickcount);
		if ((hSema = CreateSemaphore(NULL,0,100,NULL)) == NULL) return 2;
		if (install_int_ex(_timerticker,BPS_TO_TIMER(2000)))
		{
			CloseHandle(hSema);
			return 1;
		}
		sys_active = 1;
		return 0;
	#else
		return allegro_InitIdleSystem();
	#endif
}

void PrepVSyncIdle (void)
{
	#ifdef ALLEGRO_WINDOWS
		int z;

		if (!sys_active) return;

		Idle(100);

		vsync_ticks = 0;
		vsync(); tickcount = 0;

		for (z = 0; z < 32; z++)
		{
			vsync();
			vsync_ticks += tickcount;
			tickcount = 0;
		}

		vsync_ticks = (vsync_ticks >> 5) * 3 / 5; // 32 Samples taken, divide by 32 by shifting right 5.
	#else
		allegro_PrepVSyncIdle();
	#endif
}

void Idle (int msec)
{
	#ifdef ALLEGRO_WINDOWS
		// NOTE: msec is multiplied by 2 and processed at twice the speed for greater accuracy.
		if ((msec <= 0) || !sys_active) { Sleep(0); return; }
		msec <<= 1;
		while (msec > 0) {
			WaitForSingleObject(hSema,1000);
			msec--;
		}
	#else
		allegro_Idle(msec);
	#endif
}

void IdleUntilVSync (void)
{
	#ifdef ALLEGRO_WINDOWS
		if (!sys_active) { Sleep(0); return; }
		while (tickcount < vsync_ticks)
			WaitForSingleObject(hSema,1000);
	#else
		allegro_IdleUntilVSync();
	#endif
}

void DoneVSync (void)
{
	#ifdef ALLEGRO_WINDOWS
		tickcount = 0;
	#else
		allegro_DoneVSync();
	#endif
}

void IdleUntilPollScroll (void)
{
	#ifdef ALLEGRO_WINDOWS
		if (!sys_active) { Sleep(0); return; }
		while (poll_scroll())
			WaitForSingleObject(hSema,1000);
	#else
		allegro_IdleUntilPollScroll();
	#endif
}

void StopIdleSystem (void)
{
	#ifdef ALLEGRO_WINDOWS
		if (!sys_active) return;
		remove_int(_timerticker);
		CloseHandle(hSema);
	#else
		allegro_StopIdleSystem();
	#endif
}
