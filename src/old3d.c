#include <allegro.h>
#include "3d.h"

/* This module contains basic functions for projecting 3D points for use on a
   2D surface. A little note: the two functions below you should use the same
   pw and ph if you want the scaling to be 1:1, if you use a 3:4 or something
   else, the object will scale oddly. */

/* projects 3d point onto 2d plane (x)
   x is the x-coordinate of the point in space
   z is the z-coordinate of the point in space
   px is the center of the 2d plane
   pw is the width of the 2d plane */
int project_2d_x(int x, int z, int px, int pw)
{
	int space_x = pw;
	int rx;
	
	if(z + pw > 0)
	{
		rx = ((x - px) * pw) / (z + pw) + px;
//		rx *= space_x;
//		rx /= (z + pw);
//		rx += px;
	
		return rx;
	}
	else
	{
		return 0xFFFFFFFF;
	}
}

/* projects 3d point onto 2d plane (y)
   y is the y-coordinate of the point in space
   z is the z-coordinate of the point in space
   py is the center of the 2d plane
   ph is the height of the 2d plane */
int project_2d_y(int y, int z, int py, int ph)
{
	int space_y = ph;
	int ry;
	
	if(z + ph > 0)
	{
		ry = ((y - py) * ph) / (z + ph) + py;
//		ry = y - py;
//		ry *= space_y;
//		ry /= (z + ph);
//		ry += py;
	
		return ry;
	}
	else
	{
		return 0xFFFFFFFF;
	}
}

int line_3d(BITMAP * bp, int x, int y, int z, int x2, int y2, int z2, int color)
{
	int obx, oby, oex, oey;
	int px, py, pw;
	
	px = bp->w / 2;
	py = bp->h / 2;
	pw = bp->w;
	
	obx = project_2d_x(x, z, px, pw);
	oex = project_2d_x(x2, z2, px, pw);
	oby = project_2d_y(y, z, py, pw);
	oey = project_2d_y(y2, z2, py, pw);
	
	line(bp, obx, oby, oex, oey, color);
}

void draw_sprite_3d(BITMAP * bp, BITMAP * sprite, int x, int y, int z, int px, int py)
{
	float space_x, space_y;
	
	space_x = bp->w;
	space_y = bp->w;
	
	/* upper left and bottom right points in 3d */
	int obj_x[2], obj_y[2], obj_z[2];
	
	/* upper left and bottom right points in 2d */
	int screen_x[2], screen_y[2];
	int screen_w, screen_h;

	obj_x[0] = project_2d_x(x, z, px, bp->w);
	obj_x[1] = project_2d_x(x + sprite->w, z, px, bp->w);
	obj_y[0] = project_2d_y(y, z, py, bp->w);
	obj_y[1] = project_2d_y(y + sprite->w, z, py, bp->w);
	obj_z[0] = z + bp->w;
	obj_z[1] = z + bp->w;
	
	/* clip sprites at z = 0 */
	if(obj_z[0] > 0)
	{
		screen_w = obj_x[1] - obj_x[0];
		screen_h = obj_y[1] - obj_y[0];
		masked_stretch_blit(sprite, bp, 0, 0, sprite->w, sprite->h, obj_x[0], obj_y[0], screen_w, screen_h);
	}
}

void draw_sprite_3d_ex(BITMAP * bp, BITMAP * sprite, int x, int y, int z, int px, int py, int pw)
{
	float space_x, space_y;
	
	space_x = pw;
	space_y = pw;
	
	/* upper left and bottom right points in 3d */
	int obj_x[2], obj_y[2], obj_z[2];
	
	/* upper left and bottom right points in 2d */
	int screen_x[2], screen_y[2];
	int screen_w, screen_h;

	obj_x[0] = project_2d_x(x, z, px, pw);
	obj_x[1] = project_2d_x(x + sprite->w, z, px, pw);
	obj_y[0] = project_2d_y(y, z, py, pw);
	obj_y[1] = project_2d_y(y + sprite->w, z, py, pw);
	obj_z[0] = z + pw;
	obj_z[1] = z + pw;
	
	/* clip sprites at z = 0 */
	if(obj_z[0] > 0)
	{
		screen_w = obj_x[1] - obj_x[0];
		screen_h = obj_y[1] - obj_y[0];
		masked_stretch_blit(sprite, bp, 0, 0, sprite->w, sprite->h, obj_x[0], obj_y[0], screen_w, screen_h);
	}
}
