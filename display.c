#include "display.h"

int get_display_info(SCREEN *screen)
{
  Display *dpy;

  if (!(dpy = XOpenDisplay(0)))
    return 0;

  screen->snum = DefaultScreen(dpy);
  screen->width = DisplayWidth(dpy, screen->snum);
  screen->height = DisplayHeight(dpy, screen->snum);

  return 1;
}