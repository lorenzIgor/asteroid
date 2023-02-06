#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _screen
{
  u_int16_t snum;
  u_int16_t width;
  u_int16_t height;
} SCREEN;

int get_display_info(SCREEN *screen);

#endif