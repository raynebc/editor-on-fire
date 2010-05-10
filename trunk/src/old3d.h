#ifndef NCD_3D_H
#define NCD_3D_H

#include <allegro.h>

int project_2d_x(int x, int z, int px, int pw);
int project_2d_y(int y, int z, int py, int ph);
int line_3d(BITMAP * bp, int x, int y, int z, int x2, int y2, int z2, int color);
void draw_sprite_3d(BITMAP * bp, BITMAP * sprite, int x, int y, int z, int px, int py);
void draw_sprite_3d_ex(BITMAP * bp, BITMAP * sprite, int x, int y, int z, int px, int py, int pw);

#endif
