#ifndef OCD3D_H
#define OCD3D_H

#include <allegro.h>

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct
{
	
	/* scale the final output by this much */
	float scale_x;
	float scale_y;
	
	/* vanishing point */
	float vp_x;
	float vp_y;
	
	/* size of the viewport */
	float view_w;
	float view_h;
	
} OCD3D_STATE;

/* state setting functions, use states so you don't have to keep passing
   the same arguments to the other functions over and over */
void ocd3d_set_projection(float sx, float sy, float vx, float vy, float vw, float vh);
void ocd3d_store_state(OCD3D_STATE * sp);
void ocd3d_restore_state(OCD3D_STATE * sp);

float ocd3d_project_x(float x, float z);
float ocd3d_project_y(float y, float z);
void ocd3d_draw_line(BITMAP * bp, float x, float y, float z, float x2, float y2, float z2, int color);
void ocd3d_draw_bitmap(BITMAP * dest, BITMAP * bp, float x, float y, float z);

#ifdef __cplusplus
   }
#endif

#endif
