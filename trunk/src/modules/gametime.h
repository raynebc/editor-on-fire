#ifndef GAMETIME_H
#define GAMETIME_H

extern volatile int gametime_tick;
extern int tic_counter;

int gametime_init(unsigned int freq);
void gametime_reset(void);
volatile int gametime_get_frames(void);

#endif
