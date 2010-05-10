/* aphoton.c
 * 
 * This file is part of the Allegro GUI Un-uglification Project.
 * It emulates the look of the QNX Photon MicroGUI widget set.
 * 
 * Eric Botcazou <ebotcazou@multimania.com>
 */


#include <allegro.h>
#include "aphoton.h"
#include "agupitrn.h"


/*----------------------------------------------------------------------*/
/* Recommend fg/bg colors						*/
/*----------------------------------------------------------------------*/


int aphoton_fg_color;
int aphoton_bg_color;
int aphoton_mg_color;


/*----------------------------------------------------------------------*/
/* Helpers              							*/
/*----------------------------------------------------------------------*/


typedef struct iRGB
{
    int r,g,b;
} iRGB;

typedef struct GOURAUD_RANGE
{
    iRGB from;
    iRGB to;
} GOURAUD_RANGE;


static struct GOURAUD_RANGE button_gray_range, scrollbar_gray_range,
                            menu_gray_range, window_blue_range, slider_gray_range;
static int container_black, container_gray1, container_gray2;
static int button_border_gray;
static int check_black, check_gray1, check_gray2;
static int scrollbar_gray1, scrollbar_gray2;
static int edit_gray, edit_white;
static int list_white, list_green;
static int menu_gray_from, menu_gray_to;
static int checked_gray;
static int slider_white1, slider_white2;
static int slider_gray1, slider_gray2, slider_gray3, slider_gray4, slider_gray5;
static int window_black, window_gray;
static int window_blue1, window_blue2, window_blue3;
static int black, white, normal, highlight, shadow;


#define REDRAW(d)						\
do {								\
    scare_mouse_area(d->x, d->y, d->x + d->w, d->y + d->h);	\
    object_message(d, MSG_DRAW, 0);				\
    unscare_mouse();						\
} while (0)


#define draw_base(bmp, d)  rectfill(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1, normal)


static BITMAP *generate_bitmap(AL_CONST unsigned char templat[], int w, int h)
{
    BITMAP *bmp;
    AL_CONST unsigned char *p;
    int x, y;

    bmp = create_bitmap(w, h);
    p = templat;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (*p == 0)
		putpixel(bmp, x, y, bitmap_mask_color(bmp));
            else
		putpixel(bmp, x, y, makecol(*p, *p, *p));
            p++;
        }
    }            
        
    return bmp;
}


/*----------------------------------------------------------------------*/
/* Photon-lookalike procs						*/
/*----------------------------------------------------------------------*/


static void rectgouraud(BITMAP *bmp, int x1, int y1, int x2, int y2,
                                     GOURAUD_RANGE *range, int vert)
{
    iRGB gamma;
    int x, y;

    if (vert) {
        for (x=x1; x<=x2; x++) {
            gamma.r = range->from.r + (range->to.r - range->from.r) * (x - x1) / (x2 - x1);
            gamma.g = range->from.g + (range->to.g - range->from.g) * (x - x1) / (x2 - x1);
            gamma.b = range->from.b + (range->to.b - range->from.b) * (x - x1) / (x2 - x1);
            
	    vline(bmp, x, y1, y2, makecol(gamma.r, gamma.g, gamma.b));
	}
    }
    else {
        for (y=y1; y<=y2; y++) {
            gamma.r = range->from.r + (range->to.r - range->from.r) * (y - y1) / (y2 - y1);
            gamma.g = range->from.g + (range->to.g - range->from.g) * (y - y1) / (y2 - y1);
            gamma.b = range->from.b + (range->to.b - range->from.b) * (y - y1) / (y2 - y1);
	
            hline(bmp, x1, y, x2, makecol(gamma.r, gamma.g, gamma.b));
        }
    }
}


static INLINE void rectfillwh(BITMAP *bmp, int x, int y, int w, int h, int color)
{
    rectfill(bmp, x, y, x+w-1, y+h-1, color);
}


static void photon_container(BITMAP *bmp, int x, int y, int w, int h)
{
    rect(bmp, x+1, y+1, x+w-2, y+h-2, container_black);
    hline(bmp, x, y, x+w-2, container_gray1);
    vline(bmp, x, y, y+h-2, container_gray1);
    hline(bmp, x+1, y+h-1, x+w-1, container_gray2);
    vline(bmp, x+w-1, y+1, y+h-1, container_gray2);
}


static void photon_box(BITMAP *bmp, int x, int y, int w, int h, int color)
{
    photon_container(bmp, x, y ,w ,h);
    rectfill(bmp, x+2, y+2, x+w-3, y+h-3, color);
}


static void photon_textout(BITMAP *bmp, AL_CONST char *text, int x, int y,
                                        int flags, int centre)
{
    int height, len;

    if (flags & D_DISABLED)
        gui_textout_ex(bmp, text, x+1, y+1, white, -1, centre);
			    
    gui_textout_ex(bmp, text, x, y, (flags & D_DISABLED? shadow : black), -1, centre);
			
    if (flags & D_GOTFOCUS) {
	height = text_height(font);
        len = text_length(font, text);
        
        if (centre)
            hline(bmp, x-len/2, y+height, x+len/2-1, highlight);
        else
            hline(bmp, x, y+height, x+len-1, highlight);
    }
}


static void photon_scrollbar(BITMAP *bmp, int x, int y, int w, int h,
                                          int vert, int pos, int len)
{
    int slx, sly, slw, slh;
    
    hline(bmp, x, y, x+w-1, scrollbar_gray2);
    vline(bmp, x, y, y+h-1, scrollbar_gray2);
	
    if (vert) {
	slx = x;
	slw = w;
	sly = y + pos;
	slh = len;
    
	hline(bmp, slx, sly-1, slx+slw-1, black);
	hline(bmp, slx, sly+slh, slx+slw-1, black);
	
	if (sly+slh+1 < y+h)
	    hline(bmp, slx, sly+slh+1, slx+slw-1, scrollbar_gray2);
    }
    else {
    	slx = x + pos;
    	slw = len;
    	sly = y;
    	slh = h;
    
	vline(bmp, slx-1, sly, sly+slh-1, black);
	vline(bmp, slx+slw, sly, sly+slh-1, black);
	
	if (slx+slw+1 < x+w)
	    vline(bmp, slx+slw+1, sly, sly+slh-1, scrollbar_gray2);
    }
    
    hline(bmp, slx, sly, slx+slw-1, white);
    vline(bmp, slx, sly, sly+slh-1, white);
    hline(bmp, slx, sly+slh-1, slx+slw-1, button_border_gray);
    vline(bmp, slx+slw-1, sly, sly+slh-1, button_border_gray);
    rectgouraud(bmp, slx+1, sly+1, slx+slw-2, sly+slh-2, &scrollbar_gray_range, vert);
}


static void photon_slider_mark(BITMAP *bmp, int x, int y)
{
   putpixel(bmp, x, y, slider_white1);
   putpixel(bmp, x+1, y, button_border_gray);
   putpixel(bmp, x, y+1, button_border_gray);
   putpixel(bmp, x+1, y+1, slider_gray3);
}


static void photon_slider_cursor(BITMAP *bmp, int x, int y, int w, int h, int vert)
{
   rect(bmp, x, y, x+w-2, y+h-2, container_black);
   hline(bmp, x+1, y+h-3, x+w-3, slider_gray4);
   vline(bmp, x+w-3, y+1, y+h-3, slider_gray4);
   hline(bmp, x+1, y+1, x+w-3, slider_white2);
   vline(bmp, x+1, y+1, y+h-3, slider_white2);
   rectgouraud(bmp, x+2, y+2, x+w-4, y+h-4, &slider_gray_range, vert);
   hline(bmp, x+1, y+h-1, x+w-1, slider_gray5);
   vline(bmp, x+w-1, y+1, y+h-1, slider_gray5);
}
   

int d_aphoton_box_proc(int msg, DIALOG *d, int c)
{
    (void)c;
    
    if (msg == MSG_DRAW)
	photon_box(gui_get_screen(), d->x, d->y, d->w, d->h, normal);
	
    return D_O_K;
}


int d_aphoton_shadow_box_proc(int msg, DIALOG *d, int c)
{
    (void)c;

    if (msg == MSG_DRAW)
	photon_box(gui_get_screen(), d->x, d->y, d->w, d->h, normal);
	
    return D_O_K;
}


int d_aphoton_button_proc(int msg, DIALOG *d, int c)
{
    int top_color, bottom_color, shift;
        
    if (msg == MSG_DRAW) {
        BITMAP *bmp = gui_get_screen();
        photon_container(bmp, d->x, d->y, d->w, d->h);
    
        /* internal frame */
        if (d->flags & D_SELECTED) {
            top_color = button_border_gray;
            bottom_color = white;
            shift = 1;   
        }
        else {
            top_color = white;
            bottom_color = button_border_gray;
            shift = 0;
        }
        
        hline(bmp, d->x+2, d->y+2, d->x+d->w-3, top_color);
        vline(bmp, d->x+2, d->y+2, d->y+d->h-3, top_color);
        hline(bmp, d->x+2, d->y+d->h-3, d->x+d->w-3, bottom_color);
        vline(bmp, d->x+d->w-3, d->y+2, d->y+d->h-3, bottom_color);
        rectgouraud(bmp, d->x+3, d->y+3, d->x+d->w-4, d->y+d->h-4, &button_gray_range, FALSE);
	    
	if (d->dp)
	    photon_textout(bmp, (char *)d->dp, d->x+d->w/2+shift,
	                   d->y+d->h/2-text_height(font)/2+shift, d->flags, TRUE);
	    
	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


int d_aphoton_push_proc(int msg, DIALOG *d, int c)
{
    int ret = D_O_K;
    
    d->flags |= D_EXIT;
    
    ret |= d_aphoton_button_proc(msg, d, c);
    
    if (ret & D_CLOSE) {
	ret &= ~D_CLOSE;
	REDRAW(d);
	
	if (d->dp3)
	    ret |= ((int (*)(DIALOG *))d->dp3)(d);
    }
    
    return ret;
}


int d_aphoton_check_proc(int msg, DIALOG *d, int c)
{  
    if (msg == MSG_DRAW) {
        BITMAP *bmp = gui_get_screen();
        draw_base(bmp, d);
        photon_container(bmp, d->x+3, d->y+d->h/2-7, 14, 14);
        rectfillwh(bmp, d->x+5, d->y+d->h/2-5, 10, 10, white);
		
	if (d->flags & D_SELECTED) {
	    /* shadow */
	    line(bmp, d->x+6, d->y+d->h/2-3, d->x+12, d->y+d->h/2+3, check_gray1);
	    line(bmp, d->x+7, d->y+d->h/2-4, d->x+13, d->y+d->h/2+2, check_gray1);
	    line(bmp, d->x+6, d->y+d->h/2+2, d->x+12, d->y+d->h/2-4, check_gray1);
	    line(bmp, d->x+7, d->y+d->h/2+3, d->x+13, d->y+d->h/2-3, check_gray1);
	    
	    /* cross */
	    line(bmp, d->x+6, d->y+d->h/2-4, d->x+13, d->y+d->h/2+3, check_black);
	    line(bmp, d->x+6, d->y+d->h/2+3, d->x+13, d->y+d->h/2-4, check_black);
	}
	else {
  	    rect(bmp, d->x+6, d->y+d->h/2-4, d->x+13, d->y+d->h/2+3, check_gray2);
	}
	
	if (d->dp)
	    photon_textout(bmp, (char *)d->dp, d->x+20,
	                   d->y+d->h/2-text_height(font)/2, d->flags, FALSE);
	
	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


static AL_CONST unsigned char radio[] =
{
    /* 14 x 14 */
    0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 
    0xd8, 0xd8, 0xd8, 0xd8, 0xb4, 0x61, 0x43, 0x43, 0x61, 0xb4, 0xd8, 0xd8, 0xd8, 0xd8, 
    0xd8, 0xd8, 0xd8, 0x7f, 0x82, 0xc5, 0xe0, 0xde, 0xbf, 0x78, 0x78, 0xc7, 0xd8, 0xd8, 
    0xd8, 0xd8, 0x7f, 0xa9, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xde, 0x99, 0x74, 0xcd, 0xd8, 
    0xd8, 0xb4, 0x82, 0xe0, 0xe0, 0xe0, 0xde, 0xde, 0xd6, 0xd6, 0xd3, 0x6e, 0xb2, 0xd8, 
    0xd8, 0x61, 0xc8, 0xe0, 0xe0, 0xe0, 0xde, 0xd6, 0xd3, 0xd0, 0xcd, 0xad, 0x4d, 0xc6, 
    0xd8, 0x42, 0xe0, 0xe0, 0xe0, 0xd6, 0xd6, 0xd3, 0xcd, 0xca, 0xca, 0xb2, 0x34, 0xc6, 
    0xd8, 0x42, 0xde, 0xe0, 0xd6, 0xd6, 0xd0, 0xca, 0xc5, 0xc2, 0xc3, 0xaf, 0x34, 0xc6, 
    0xd8, 0x5e, 0xbf, 0xde, 0xd6, 0xd0, 0xc9, 0xc4, 0xbe, 0xbc, 0xb5, 0x8e, 0x48, 0xc6, 
    0xd8, 0xb2, 0x79, 0xde, 0xd4, 0xcd, 0xc4, 0xbe, 0xb5, 0xb5, 0xae, 0x61, 0xb2, 0xcd, 
    0xd8, 0xc9, 0x6e, 0x99, 0xd4, 0xca, 0xc2, 0xbc, 0xb5, 0xad, 0x73, 0x55, 0xc1, 0xd8, 
    0xd8, 0xcd, 0xc1, 0x66, 0x6e, 0x97, 0xb2, 0xaf, 0x8e, 0x61, 0x55, 0xc1, 0xcd, 0xd8, 
    0xd8, 0xd8, 0xcd, 0xc9, 0xb2, 0x48, 0x32, 0x32, 0x47, 0xb2, 0xc1, 0xcd, 0xd8, 0xd8, 
    0xd8, 0xd8, 0xd8, 0xd8, 0xcd, 0xc4, 0xc4, 0xc4, 0xc4, 0xcd, 0xcd, 0xd8, 0xd8, 0xd8
};


static AL_CONST unsigned char radio_sel[] =
{
    /* 14 x 14 */
    0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 0xd8, 
    0xd8, 0xd8, 0xd8, 0xd8, 0xb4, 0x61, 0x43, 0x43, 0x61, 0xb4, 0xd8, 0xd8, 0xd8, 0xd8, 
    0xd8, 0xd8, 0xd8, 0x7f, 0x82, 0xc5, 0xe0, 0xde, 0xbf, 0x78, 0x78, 0xc7, 0xd8, 0xd8, 
    0xd8, 0xd8, 0x7f, 0xa9, 0xd4, 0xbd, 0xb2, 0xb5, 0xc4, 0xd6, 0x99, 0x74, 0xcd, 0xd8, 
    0xd8, 0xb4, 0x82, 0xd4, 0xae, 0x8f, 0x71, 0x73, 0x99, 0xbd, 0xce, 0x6e, 0xb2, 0xd8, 
    0xd8, 0x61, 0xc5, 0xbe, 0x8f, 0x58, 0x4b, 0x4e, 0x5f, 0xa2, 0xca, 0xad, 0x4d, 0xc6, 
    0xd8, 0x42, 0xe0, 0xb2, 0x70, 0x4b, 0x48, 0x4a, 0x52, 0x81, 0xce, 0xb2, 0x34, 0xc6, 
    0xd8, 0x42, 0xde, 0xb3, 0x72, 0x4d, 0x4a, 0x4c, 0x53, 0x83, 0xcd, 0xaf, 0x34, 0xc6, 
    0xd8, 0x5e, 0xbf, 0xc2, 0x98, 0x5f, 0x51, 0x52, 0x64, 0xab, 0xc4, 0x8e, 0x48, 0xc6, 
    0xd8, 0xb2, 0x79, 0xd3, 0xbc, 0xa0, 0x7d, 0x82, 0xa7, 0xc4, 0xb5, 0x61, 0xb2, 0xcd, 
    0xd8, 0xc9, 0x6e, 0x99, 0xd0, 0xc8, 0xc8, 0xc9, 0xc3, 0xb3, 0x73, 0x55, 0xc1, 0xd8, 
    0xd8, 0xcd, 0xc1, 0x66, 0x6e, 0x97, 0xb2, 0xaf, 0x8e, 0x61, 0x55, 0xc1, 0xcd, 0xd8, 
    0xd8, 0xd8, 0xcd, 0xc9, 0xb2, 0x48, 0x32, 0x32, 0x47, 0xb2, 0xc1, 0xcd, 0xd8, 0xd8, 
    0xd8, 0xd8, 0xd8, 0xd8, 0xcd, 0xc4, 0xc4, 0xc4, 0xc4, 0xcd, 0xcd, 0xd8, 0xd8, 0xd8
};


static BITMAP *radio_bmp, *radio_sel_bmp;


/* d1 used for group number. */
int d_aphoton_radio_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	draw_base(bmp, d);

	if (d->flags & D_SELECTED)
  	    blit(radio_sel_bmp, bmp, 0, 0, d->x+3, d->y+d->h/2-7,
  	         radio_sel_bmp->w, radio_sel_bmp->h);
  	else
  	    blit(radio_bmp, bmp, 0, 0, d->x+3, d->y+d->h/2-7,
  	         radio_bmp->w, radio_bmp->h);

	if (d->dp)
	    photon_textout(bmp, (char *)d->dp, d->x+20,
	                   d->y+d->h/2-text_height(font)/2, d->flags, FALSE);

	return D_O_K;
    }

    return d_radio_proc(msg, d, c);
}


int d_aphoton_icon_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
    BITMAP *bmp = gui_get_screen();
	BITMAP *img = (BITMAP *)d->dp;
	
	photon_container(bmp, d->x, d->y, d->w, d->h);
	
	stretch_sprite(bmp, img, d->x+2, d->y+2, d->w-4, d->h-4);
	return D_O_K;
    }
    
    return d_aphoton_button_proc(msg, d, c);
}


int d_aphoton_edit_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int l, x, b, f, p, w;
	int fg = (d->flags & D_DISABLED) ? shadow : black;
	char *s = (char *)d->dp;
	char buf[16];
	int fonth;

	agup_edit_adjust_position (d);
	
	fonth = text_height(font);
	
	l = ustrlen(s);
	/* set cursor pos */
	if (d->d2 >= l) {
	    d->d2 = l;
	    usetc(buf+usetc(buf, ' '), 0);
	    x = text_length(font, buf) + 2;
	}
	else
	    x = 2;
	
	b = 0;	  /* num of chars to be blitted */
	/* get the part of the string to be blitted */
	for (p = d->d2; p >= 0; p--) {
	    usetc(buf+usetc(buf, ugetat(s, p)), 0);
	    x += text_length(font, buf);
	    b++;
	    if (x > d->w-4)
		break;
	}

	/* see if length of text is too wide */
	if (x <= d->w-2) {
	    b = l; 
	    p = 0;
	}
	else {
	    b--;
	    p = d->d2-b+1; 
	    b = d->d2; 
	}

	photon_box(bmp, d->x, d->y, d->w, fonth+8, edit_white);
	hline(bmp, d->x+2, d->y+2, d->x+d->w-3, edit_gray);
	vline(bmp, d->x+2, d->y+2, d->y+fonth+5, edit_gray);

	for (x = 4; p<=b; p++) {
	    f = ugetat(s, p);
	    usetc(buf+usetc(buf, (f) ? f : ' '), 0);
	    w = text_length(font, buf);
	    f = ((p == d->d2) && (d->flags & D_GOTFOCUS));
	    textout_ex(bmp, font, buf, d->x+x, d->y+4, fg, edit_white);
	    if (f)
		vline(bmp, d->x+x-1, d->y+3, d->y+fonth+5, black);
	    if ((x += w) + w > d->w - 4)
		break;
	}

        if (d->flags & D_GOTFOCUS)
            hline(bmp, d->x+4, d->y+fonth+4, d->x+d->w-5, highlight);

        agup_edit_restore_position (d);

	return D_O_K;
    }

    return d_agup_adjusted_edit_proc(msg, d, c);
}


typedef char *(*getfuncptr)(int, int *);


static void photon_draw_scrollable_frame(DIALOG *d, int listsize, int offset,
                                         int height)
{
    BITMAP *bmp = gui_get_screen();
    int pos, len;

    photon_container(bmp, d->x, d->y, d->w, d->h);

    /* possibly draw scrollbar */
    if (listsize > height) {
        rect(bmp, d->x+d->w-13, d->y+1, d->x+d->w-2, d->y+d->h-2, container_black);
	rectfill(bmp, d->x+d->w-12, d->y+2, d->x+d->w-3, d->y+d->h-3, scrollbar_gray1);
	
	if (offset > 0)
	    pos = ((d->h-4) * offset + listsize/2) / listsize;
	else
	    pos = 0;
	
	len = ((d->h-4) * height + listsize/2) / listsize;

	photon_scrollbar(bmp, d->x+d->w-12, d->y+2, 10, d->h-4, TRUE, pos, len);
    }
}


int d_aphoton_list_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int height, listsize, i, len, bar, x, y, w, char_height;
	int fg, bg;
	char *sel = (char *)d->dp2;
	char s[1024];

	(*(getfuncptr)d->dp)(-1, &listsize);
	char_height = text_height(font);
	height = (d->h-4) / char_height;
	bar = (listsize > height);
	w = (bar ? d->w-14 : d->w-3);

	/* draw box contents */
	for (i=0; i<height; i++) {
	    if (d->d2+i < listsize) {
		if (d->d2+i == d->d1) {
		    fg = list_white;
		    bg = list_green;
		}
		else if ((sel) && (sel[d->d2+i])) {
		    fg = list_white;
		    bg = list_green;
		}
		else {
		    fg = black;
		    bg = list_white;
		}
		
		ustrzcpy(s, sizeof(s), (*(getfuncptr)d->dp)(i+d->d2, NULL));
		
		x = d->x + 2;
		y = d->y + 2 + i*char_height;

		rectfill(bmp, x, y, x+7, y+char_height-1, bg);
		x += 8;
		
		len = ustrlen(s);
		while (text_length(font, s) >= MAX(d->w - 1 - (bar ? 22 : 10), 1)) {
		    len--;
		    usetat(s, len, 0);
		}
		
		textout_ex(bmp, font, s, x, y, fg, bg);
		
		x += text_length(font, s);
		if (x <= d->x+w)
		    rectfill(bmp, x, y, d->x+w, y+char_height-1, bg);
	    }
	    else {
		rectfill(bmp, d->x+2,  d->y+2+i*char_height,
			 d->x+w, d->y+1+(i+1)*char_height, list_white);
	    }
	}

	if (d->y+2+i*char_height <= d->y+d->h-3)
	    rectfill(bmp, d->x+2, d->y+2+i*char_height,
		     d->x+w, d->y+d->h-3, list_white);

	/* draw frame, maybe with scrollbar */
	photon_draw_scrollable_frame(d, listsize, d->d2, height);

	return D_O_K;
    }

    return d_list_proc(msg, d, c);
}


int d_aphoton_text_list_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
	return d_aphoton_list_proc(msg, d, c);
	
    return d_text_list_proc(msg, d, c);
}


int d_aphoton_textbox_proc(int msg, DIALOG *d, int c)
{
    int height, bar, fg_color;
	
    if (msg == MSG_DRAW) {

	fg_color = (d->flags & D_DISABLED) ? aphoton_mg_color : black;
	height = (d->h-8) / text_height(font);

	/* tell the object to sort of draw, but only calculate the listsize */
	_draw_textbox((char *)d->dp, &d->d1, 
		      0, /* DONT DRAW anything */
		      d->d2, !(d->flags & D_SELECTED), 8,
		      d->x, d->y, d->w, d->h,
		      (d->flags & D_DISABLED),
		      0, 0, 0);

	if (d->d1 > height) {
	    bar = 11;
	}
	else {
	    bar = 0;
	    d->d2 = 0;
	}

	/* now do the actual drawing */
	_draw_textbox((char *)d->dp, &d->d1, 1, d->d2,
		      !(d->flags & D_SELECTED), 8,
		      d->x, d->y, d->w-bar, d->h,
		      (d->flags & D_DISABLED),
		      fg_color, white, aphoton_mg_color);

	/* draw the frame around */
	photon_draw_scrollable_frame(d, d->d1, d->d2, height);
	
	return D_O_K;
    }
    
    return d_textbox_proc(msg, d, c);
}


#define HHEIGHT  13  /* handle height (width for horizontal sliders) */
#define MSPACE   15  /* space between marks */

int d_aphoton_slider_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int vert = TRUE;    /* flag: is slider vertical? */
	int slp;            /* slider position */
	int irange, inmark, ismark, i;
	fixed slratio, slmax, slpos;

	/* check for slider direction */
	if (d->h < d->w)
	    vert = FALSE;

	irange = (vert ? d->h-2 : d->w-2);
	inmark = irange/MSPACE;
	ismark = irange/inmark;
	slmax = itofix(irange - HHEIGHT);
	slratio = slmax / (d->d1);
	slpos = slratio * d->d2;
	slp = 1+fixtoi(slpos);

        rectfillwh(bmp, d->x, d->y, d->w, d->h, normal);

        if (vert) {
           vline(bmp, d->x+d->w/2-3, d->y+1, d->y+d->h-2, check_gray2);
           vline(bmp, d->x+d->w/2-2, d->y+1, d->y+d->h-2, slider_gray2);
           vline(bmp, d->x+d->w/2-1, d->y+1, d->y+d->h-2, slider_gray1);
           vline(bmp, d->x+d->w/2+0, d->y+1, d->y+d->h-2, container_black);
           vline(bmp, d->x+d->w/2+1, d->y+1, d->y+d->h-2, slider_white1);
           hline(bmp, d->x+d->w/2-1, d->y, d->x+d->w/2+0, slider_gray2);
           hline(bmp, d->x+d->w/2-1, d->y+d->h-1, d->x+d->w/2+0, slider_gray2);
           
           /* marks */
           for (i=0; i<inmark; i++)
              photon_slider_mark(bmp, d->x+d->w-3, d->y+1 + i*ismark);
           
           photon_slider_mark(bmp, d->x+d->w-3, d->y+d->h-2);

	   photon_slider_cursor(bmp, d->x, d->y+d->h-slp-HHEIGHT, d->w, HHEIGHT, TRUE);
	}
        else {
           /* support */
           hline(bmp, d->x+1, d->y+d->h/2-3, d->x+d->w-2, check_gray2);
           hline(bmp, d->x+1, d->y+d->h/2-2, d->x+d->w-2, slider_gray2);
           hline(bmp, d->x+1, d->y+d->h/2-1, d->x+d->w-2, slider_gray1);
           hline(bmp, d->x+1, d->y+d->h/2+0, d->x+d->w-2, container_black);
           hline(bmp, d->x+1, d->y+d->h/2+1, d->x+d->w-2, slider_white1);
           vline(bmp, d->x, d->y+d->h/2-1, d->y+d->h/2+0, slider_gray2);
           vline(bmp, d->x+d->w-1, d->y+d->h/2-1, d->y+d->h/2+0, slider_gray2);
           
           /* marks */
           for (i=0; i<inmark; i++)
              photon_slider_mark(bmp, d->x+1 + i*ismark, d->y+d->h-3);
           
           photon_slider_mark(bmp, d->x+d->w-2, d->y+d->h-3);
           
	   photon_slider_cursor(bmp, d->x+slp, d->y, HHEIGHT, d->h, FALSE);
        }
	
	return D_O_K;
    }
    
    return d_slider_proc(msg, d, c);
}


int
d_aphoton_clear_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        clear_to_color (gui_get_screen(), aphoton_bg_color);
        return D_O_K;
    }
    return d_clear_proc (msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Menus                                                                */
/*----------------------------------------------------------------------*/


/* These menus don't look much like GTK menus.  Unfortunately, it's
   probably about as good as we can do without reimplementing menus
   for ourselves.  */


static AL_CONST unsigned char menu_arrow[] =
{
    /* 6 x 8 */
    0x00, 0x00, 0x33, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x33, 0x33, 0x00, 0x00,
    0x33, 0x33, 0x33, 0x33, 0x33, 0x00,
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 
    0x33, 0x33, 0x33, 0x33, 0x33, 0x00,
    0x00, 0x00, 0x33, 0x33, 0x00, 0x00,
    0x00, 0x00, 0x33, 0x00, 0x00, 0x00
};


static BITMAP *menu_arrow_bmp;

static int last_x, last_y, last_w, last_h, menu_done;


static void photon_draw_menu(int x, int y, int w, int h)
{
    /* ugly hack needed because we lack the 'bar' member
     * of the underlying MENU_INFO structure from which
     * the arguments of the functions are passed.
     */
    last_x = x;
    last_y = y;
    last_w = w;
    last_h = h;
    menu_done = FALSE;
}
     

static void photon_do_draw_menu(int x, int y, int w, int h, int bar)
{
    BITMAP *bmp = gui_get_screen();
    rect(bmp, x, y, x+w-1, y+h-1, container_black);
    hline(bmp, x+1, y+1, x+w-2, white);
    vline(bmp, x+1, y+1, y+h-2, white);
    hline(bmp, x+1, y+h-2, x+w-2, check_gray1);
    vline(bmp, x+w-2, y+1, y+h-2, check_gray1);
    
    if (bar)
	rectgouraud(bmp, x+2, y+2, x+w-3, y+h-3, &menu_gray_range, FALSE);
    else
	rectfill(bmp, x+2, y+2, x+w-3, y+h-3, menu_gray_to);
}


static void photon_draw_menu_item(MENU *m, int x, int y, int w, int h, int bar, int sel)
{
    BITMAP *bmp = gui_get_screen();
    int i, j, height;
    char buf[256], *tok;

    if (!menu_done) {
	photon_do_draw_menu(last_x, last_y, last_w, last_h, bar);
	menu_done = TRUE;
    }

    if (ugetc(m->text)) {
	if (bar) {
	    if (sel) {
		rect(bmp, x+6, y, x+w-4, y+h, container_black);
		hline(bmp, x+7, y+1, x+w-5, white);
		vline(bmp, x+7, y+1, y+h-2, white);
		hline(bmp, x+7, y+h-1, x+w-5, check_gray1);
		vline(bmp, x+w-5, y, y+h-1, check_gray1);
		rectgouraud(bmp, x+8, y+2, x+w-6, y+h-2, &scrollbar_gray_range, FALSE);
	    }
	    else {
		hline(bmp, x+6, y, x+w-4, white);
		hline(bmp, x+6, y+h-1, x+w-4, check_gray1);
		rectgouraud(bmp, x+6, y+1, x+w-4, y+h-2, &menu_gray_range, FALSE);
	    }
	}
	else {
	    if (!(m->flags & D_DISABLED)) {
		if (sel)
		    rectfill(bmp, x+3, y+1, x+w-5, y+h-4, highlight);
		else
		    rectfill(bmp, x+3, y+1, x+w-5, y+h-4, menu_gray_to);
	    }
	}

	i = 0;
	j = ugetc(m->text);

	while ((j) && (j != '\t')) {
	    i += usetc(buf+i, j);
	    j = ugetc(m->text+i);
	}

	usetc(buf+i, 0);

	if (bar)
	    photon_textout(bmp, buf, x+9, y+1, m->flags, FALSE);
	else
	    photon_textout(bmp, buf, x+4, y+1, m->flags, FALSE);

	if (j == '\t') {
	    tok = m->text+i + uwidth(m->text+i);
	    photon_textout(bmp, tok, x+w-gui_strlen(tok)-15, y+1, m->flags, FALSE);
	}

	if ((m->child) && (!bar))
	    draw_sprite(bmp, menu_arrow_bmp, x+w-12, y+(h-menu_arrow_bmp->h-1)/2);
    }
    else {
	height = text_height(font);
	hline(bmp, x, y+height/2-1, x+w-1, check_gray1);
	hline(bmp, x, y+height/2, x+w-1, container_black);
	hline(bmp, x, y+height/2+1, x+w-1, white);
    }

    if (m->flags & D_SELECTED) {
	height = text_height(font);
	line(bmp, x+w-10, y+height/2, x+w-8, y+height, checked_gray);
	line(bmp, x+w-8, y+height, x+w-5, y+1, checked_gray);
    }
}


int d_aphoton_menu_proc(int msg, DIALOG *d, int c)
{
    return d_menu_proc(msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Window frames								*/
/*----------------------------------------------------------------------*/


int d_aphoton_window_proc(int msg, DIALOG *d, int c)
{
    int cl, ct, cr, cb;
    (void)c;

    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	rect(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1, black);
	rect(bmp, d->x+1, d->y+1, d->x+d->w-2, d->y+d->h-2, window_black);
	
	/* gray frame */
	vline(bmp, d->x+2, d->y+2, d->y+d->h-3, white);
	hline(bmp, d->x+2, d->y+d->h-3, d->x+d->w-3, window_gray);
	vline(bmp, d->x+d->w-3, d->y+2, d->y+d->h-3, window_gray);
	
	vline(bmp, d->x+3, d->y+3, d->y+d->h-4, menu_gray_to);
	hline(bmp, d->x+3, d->y+d->h-4, d->x+d->w-4, menu_gray_to);
	vline(bmp, d->x+d->w-4, d->y+3, d->y+d->h-4, menu_gray_to);
	
	vline(bmp, d->x+4, d->y+4, d->y+d->h-5, window_gray);
	hline(bmp, d->x+4, d->y+d->h-5, d->x+d->w-5, white);
	vline(bmp, d->x+d->w-5, d->y+4, d->y+d->h-5, white);

	/* blue frame */
	hline(bmp, d->x+2, d->y+2, d->x+d->w-3, window_blue1);
	hline(bmp, d->x+2, d->y+3, d->x+d->w-3, window_blue2);
	hline(bmp, d->x+2, d->y+4, d->x+d->w-3, window_blue3);
	hline(bmp, d->x+2, d->y+5, d->x+d->w-3, window_blue2);
	
	rectgouraud(bmp, d->x+2, d->y+6, d->x+d->w-3, d->y+19, &window_blue_range, FALSE);
	hline(bmp, d->x+2, d->y+20, d->x+d->w-3, window_blue3);
	
	vline(bmp, d->x+2, d->y+6, d->y+20, window_blue1);
	vline(bmp, d->x+d->w-3, d->y+6, d->y+20, window_blue3);
	
	/* background */
	rectfill(bmp, d->x+5, d->y+21, d->x+d->w-6, d->y+d->h-6, normal);

	if (d->dp) {
	    cl = bmp->cl;
	    ct = bmp->ct;
	    cr = bmp->cr;
	    cb = bmp->cb;
	    
	    set_clip_rect(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1);

	    textout_centre_ex(bmp, font, (char *)d->dp, d->x+d->w/2, d->y+8, black, -1);

	    set_clip_rect(bmp, cl, ct, cr, cb);
	}
    }

    return D_O_K;
}


/*----------------------------------------------------------------------*/
/* Initialisation                                                       */
/*----------------------------------------------------------------------*/


#define MAKE_GRAY_RANGE(range, gamma_from, gamma_to) \
{                                                    \
    range.from.r = gamma_from;                       \
    range.from.g = gamma_from;                       \
    range.from.b = gamma_from;                       \
    range.to.r = gamma_to;                           \
    range.to.g = gamma_to;                           \
    range.to.b = gamma_to;                           \
}


#define MAKECOL_RANGE_FROM(range) \
    makecol(range.from.r, range.from.g, range.from.b)
    
   
#define MAKECOL_RANGE_TO(range) \
    makecol(range.to.r, range.to.g, range.to.b)


void aphoton_init(void)
{
    MAKE_GRAY_RANGE(button_gray_range, 235, 195);
    button_border_gray = makecol(176, 176, 176);

    container_black = makecol(75, 75, 75);
    container_gray1 = makecol(177, 177, 177);
    container_gray2 = makecol(201, 201, 201);
    
    check_black = makecol(24, 24, 24);
    check_gray1 = makecol(153, 153, 153);
    check_gray2 = makecol(204, 204, 204);
    
    MAKE_GRAY_RANGE(scrollbar_gray_range, 195, 235);
    scrollbar_gray1 = makecol(181, 181, 181);
    scrollbar_gray2 = makecol(161, 161, 161);
    
    edit_gray = makecol(192, 192, 192);
    edit_white = makecol(244, 244, 244);
    
    list_white = makecol(248, 248, 248);
    list_green = makecol(142, 162, 155);
    
    MAKE_GRAY_RANGE(menu_gray_range, 197, 217);
    menu_gray_from = MAKECOL_RANGE_FROM(menu_gray_range);
    menu_gray_to = MAKECOL_RANGE_TO(menu_gray_range);
    
    checked_gray = makecol(92, 92, 92);

    slider_white1 = makecol(235, 235, 235);
    slider_white2 = makecol(237, 237, 237);
    slider_gray1 = makecol(140, 140, 140);
    slider_gray2 = makecol(172, 172, 172);
    slider_gray3 = makecol(108, 108, 108);
    slider_gray4 = makecol(197, 197, 197);
    slider_gray5 = makecol(162, 162, 162);
    MAKE_GRAY_RANGE(slider_gray_range, 237, 197);

    window_black = makecol(63, 63, 63);
    window_gray = makecol(157, 157, 157);

    window_blue1 = makecol(142, 189, 255);
    window_blue2 = makecol(92, 141, 223);
    window_blue3 = makecol(42, 89, 173);

    window_blue_range.from.r = 102;
    window_blue_range.from.g = 149;
    window_blue_range.from.b = 233;
    window_blue_range.to.r = 71;
    window_blue_range.to.g = 118;
    window_blue_range.to.b = 202;
    
    black = makecol(0, 0, 0);    
    white = makecol(255, 255, 255);
    normal = makecol(216, 216, 216);
    highlight = makecol(144, 152, 248);
    shadow = makecol(127, 127, 127);
    
    aphoton_fg_color = black;
    aphoton_bg_color = normal;
    aphoton_mg_color = shadow;

    radio_bmp = generate_bitmap(radio, 14, 14);
    radio_sel_bmp = generate_bitmap(radio_sel, 14, 14);

    menu_arrow_bmp = generate_bitmap(menu_arrow, 6, 8);
    gui_menu_draw_menu = photon_draw_menu;
    gui_menu_draw_menu_item = photon_draw_menu_item;
}


void aphoton_shutdown(void)
{
    destroy_bitmap(menu_arrow_bmp);
    gui_menu_draw_menu_item = NULL;
    gui_menu_draw_menu = NULL;

    destroy_bitmap(radio_bmp);
    destroy_bitmap(radio_sel_bmp);
}


/*----------------------------------------------------------------------*/
/* Theme                                                                */
/*----------------------------------------------------------------------*/

static struct AGUP_THEME the_theme =
{
    "Photon",
    &aphoton_fg_color,
    &aphoton_bg_color,
    &aphoton_mg_color,
    aphoton_init,
    aphoton_shutdown,
    d_aphoton_box_proc,
    d_aphoton_shadow_box_proc,
    d_aphoton_button_proc,
    d_aphoton_push_proc,
    d_aphoton_check_proc,
    d_aphoton_radio_proc,
    d_aphoton_icon_proc,
    d_aphoton_edit_proc,
    d_aphoton_list_proc,
    d_aphoton_text_list_proc,
    d_aphoton_textbox_proc,
    d_aphoton_slider_proc,
    d_aphoton_menu_proc,
    d_aphoton_window_proc,
    d_aphoton_clear_proc,
    d_agup_common_text_proc,
    d_agup_common_ctext_proc,
    d_agup_common_rtext_proc
};


AL_CONST struct AGUP_THEME *aphoton_theme = &the_theme;

