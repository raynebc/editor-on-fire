#include <allegro.h>

/* begin Windows-specific timing module */
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
	#include "gametime.h"

	static unsigned int clocks_per_tic = 1;
	static unsigned int base_clock;
	int tic_counter;
	static unsigned int last_tic;
	volatile int gametime_tick = 0;
	int gametime_installed = 0;

	static inline volatile unsigned int read_sys_time(void)
	{
		LARGE_INTEGER c;
		QueryPerformanceCounter(&c);
		return (unsigned int)(c.LowPart);
	}

	static inline unsigned int time_per_sec(void)
	{
		LARGE_INTEGER c;
		QueryPerformanceFrequency(&c);
		return (unsigned int)(c.LowPart);
	}

	int gametime_init(unsigned int freq)
	{
		clocks_per_tic = time_per_sec() / freq;
		tic_counter = 0;
		last_tic = 0;
		base_clock = read_sys_time();
		gametime_tick = 0;
		gametime_installed = 1;
		return 1;
	}

	volatile int gametime_get_frames(void)
	{
		if(gametime_installed)
		{
			unsigned int c = (read_sys_time() - base_clock) / clocks_per_tic;
			base_clock += clocks_per_tic * c;

			tic_counter += c;
			return tic_counter;
		}
		return 0;
	}

	void gametime_reset(void)
	{
		gametime_tick = gametime_get_frames();
	}
/* end Windows-specific timing module */

/* begin portable timing module */
#else

	#include "gametime.h"

	int tic_counter;
	volatile int gametime_tick = 0;
	int gametime_installed = 0;

	void gametime_ticker(void)
	{
		tic_counter++;
	}
	
	int gametime_init(unsigned int freq)
	{
		install_int_ex(gametime_ticker, BPS_TO_TIMER(freq));
		gametime_tick = 0;
		gametime_installed = 1;
		return 1;
	}

	volatile int gametime_get_frames(void)
	{
		if(gametime_installed)
		{
			return tic_counter;
		}
		return 0;
	}

	void gametime_reset(void)
	{
		gametime_tick = gametime_get_frames();
	}
#endif
/* end portable timing module */
