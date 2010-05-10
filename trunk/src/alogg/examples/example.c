/* Example program for AllegroOGG */


#include <string.h>
#include <allegro.h>
#include "alogg.h"


void putstr(char *s);


#define DATASZ  (1<<15) /* (32768) amount of data to read from disk each time */
#define BUFSZ   (1<<16) /* (65536) size of audiostream buffer */


typedef struct {
  PACKFILE *f;
  ALOGG_OGGSTREAM *s;
} OGGFILE;


OGGFILE *open_ogg_file(char *filename) {
  OGGFILE *p = NULL;
  PACKFILE *f = NULL;
  ALOGG_OGGSTREAM *s = NULL;
  char data[DATASZ];
  int len;

  if (!(p = (OGGFILE *)malloc(sizeof(OGGFILE))))
    goto error;
  if (!(f = pack_fopen(filename, F_READ)))
    goto error;
  if ((len = pack_fread(data, DATASZ, f)) <= 0)
    goto error;
  if (len < DATASZ) {
    if (!(s = alogg_create_oggstream(data, len, TRUE)))
      goto error;
  }
  else {
    if (!(s = alogg_create_oggstream(data, DATASZ, FALSE)))
      goto error;
  }
  p->f = f;
  p->s = s;
  return p;

  error:
  pack_fclose(f);
  free(p);
  return NULL;
}


int play_ogg_file(OGGFILE *ogg, int buflen, int vol, int pan) {
  return alogg_play_oggstream(ogg->s, buflen, vol, pan);
}


void close_ogg_file(OGGFILE *ogg) {
  if (ogg) {
    pack_fclose(ogg->f);
    alogg_destroy_oggstream(ogg->s);
    free(ogg);
  }
}


int poll_ogg_file(OGGFILE *ogg) {
  char *data;
  long len;

  data = (char *)alogg_get_oggstream_buffer(ogg->s);
  if (data) {
    len = pack_fread(data, DATASZ, ogg->f);
    if (len < DATASZ)
      alogg_free_oggstream_buffer(ogg->s, len);
    else
      alogg_free_oggstream_buffer(ogg->s, -1);
  }

  return alogg_poll_oggstream(ogg->s);
}


void do_example(int n, char *filenames[]) {
  OGGFILE **ogg = (OGGFILE **)malloc(sizeof(OGGFILE *) * n);
  int remain, i;

  remain = n;
  for (i = 0; i < n; i++) {
    putstr(filenames[i]);
    if ((ogg[i] = open_ogg_file(filenames[i])))
      play_ogg_file(ogg[i], BUFSZ, 255, 128);
    else {
      remain--;
      putstr("Error opening.");
    }
  }

  while ((!keypressed()) && (remain > 0)) {
    for (i = 0; i < n; i++) {
      if ((ogg[i]) && (poll_ogg_file(ogg[i]) != ALOGG_OK)) {
        close_ogg_file(ogg[i]);
        ogg[i] = NULL;
        remain--;
      }
    }
  }

  for (i = 0; i < n; i++)
    close_ogg_file(ogg[i]);

  free(ogg);
}


int main(int argc, char *argv[]) { 
  allegro_init();

  if (argc < 2) {
    allegro_message("Usage: %s files ...\n", argv[0]);
    return 1;
  }

  install_timer();
  install_keyboard();

  if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) < 0) {
    allegro_message("Error setting video mode.\n");
    return 1;
  }
  clear_to_color(screen, makecol(255, 255, 255));

  /* to achieve the max possible volume */
  set_volume_per_voice(0);

  if (install_sound(DIGI_AUTODETECT, MIDI_NONE, 0) < 0) {
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
    allegro_message("Error installing sound.\n");
    return 1;
  }

  do_example(argc-1, argv+1);

  allegro_exit();

  return 0;
}
END_OF_MAIN();


/* a hack */
void putstr(char *s) {
  static int y = 0;
  text_mode(-1);
  textout(screen, font, s, 0, y, makecol(0, 0, 0));
  y += text_height(font);
}
