#include <allegro.h>
#include "ocd3d.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

/* This module contains basic functions for projecting 3D points for use on a
   2D surface. A little note: the two functions below you should use the same
   pw and ph if you want the scaling to be 1:1, if you use a 3:4 or something
   else, the object will scale oddly. */

/* projects 3d point onto 2d plane (x)
   x is the x-coordinate of the point in space
   z is the z-coordinate of the point in space
   px is the center of the 2d plane
   pw is the width of the 2d plane */

/* internal state of the 3d engine */
static OCD3D_STATE ocd3d_internal_state;

void ocd3d_set_projection(float sx, float sy, float vx, float vy, float vw, float vh)
{
	ocd3d_internal_state.scale_x = sx;
	ocd3d_internal_state.scale_y = sy;
	ocd3d_internal_state.vp_x = vx;
	ocd3d_internal_state.vp_y = vy;
	ocd3d_internal_state.view_w = vw;
	ocd3d_internal_state.view_h = vh;
}

void ocd3d_store_state(OCD3D_STATE * sp)
{
	memcpy(sp, &ocd3d_internal_state, sizeof(OCD3D_STATE));
}

void ocd3d_restore_state(OCD3D_STATE * sp)
{
	memcpy(&ocd3d_internal_state, sp, sizeof(OCD3D_STATE));
}

float ocd3d_project_x(float x, float z)
{
	float rx;

	if(z + ocd3d_internal_state.view_w > 0)
	{
		rx = ocd3d_internal_state.scale_x * (((x - ocd3d_internal_state.vp_x) * ocd3d_internal_state.view_w) / (z + ocd3d_internal_state.view_w) + ocd3d_internal_state.vp_x);
		return rx;
	}
	else
	{
		return -65536;
	}
}

/* projects 3d point onto 2d plane (y)
   y is the y-coordinate of the point in space
   z is the z-coordinate of the point in space
   py is the center of the 2d plane
   ph is the height of the 2d plane */
float ocd3d_project_y(float y, float z)
{
	float ry;

	if(z + ocd3d_internal_state.view_h > 0)
	{
		ry = ocd3d_internal_state.scale_y * (((y - ocd3d_internal_state.vp_y) * ocd3d_internal_state.view_h) / (z + ocd3d_internal_state.view_h) + ocd3d_internal_state.vp_y);
		return ry;
	}
	else
	{
		return -65536;
	}
}

void ocd3d_draw_line(BITMAP * bp, float x, float y, float z, float x2, float y2, float z2, int color)
{
	float obx, oby, oex, oey;

	obx = ocd3d_project_x(x, z);
	oex = ocd3d_project_x(x2, z2);
	oby = ocd3d_project_y(y, z);
	oey = ocd3d_project_y(y2, z2);

	line(bp, obx, oby, oex, oey, color);
}

void ocd3d_draw_bitmap(BITMAP * dest, BITMAP * bp, float x, float y, float z)
{
	/* upper left and bottom right points in 3d */
	float obj_x[2], obj_y[2], obj_z[2];

	/* upper left and bottom right points in 2d */
//	int screen_x[2], screen_y[2];
	float screen_w, screen_h;

	obj_x[0] = ocd3d_project_x(x, z);
	obj_x[1] = ocd3d_project_x(x + bp->w, z);
	obj_y[0] = ocd3d_project_y(y, z);
	obj_y[1] = ocd3d_project_y(y + bp->h, z);
	obj_z[0] = z + dest->w;
	obj_z[1] = z + dest->h;

	/* clip sprites at z = 0 */
	if(obj_z[0] > 0)
	{
		screen_w = obj_x[1] - obj_x[0];
		screen_h = obj_y[1] - obj_y[0];
		stretch_sprite(dest, bp, obj_x[0], obj_y[0], screen_w, screen_h);
	}
}
