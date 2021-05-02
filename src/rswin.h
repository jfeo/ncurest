#ifndef __NCURLSES_RSWIN_H__
#define __NCURLSES_RSWIN_H__

#include <ncurses.h>

#define rswin_ANCHOR_TOP 1
#define rswin_ANCHOR_BOTTOM 2
#define rswin_ANCHOR_RIGHT 4
#define rswin_ANCHOR_LEFT 8

typedef struct {
  int y;
  int x;
  int height;
  int width;
  int anchor;
  WINDOW *container;
  WINDOW *content;
  int content_scroll_y;
  int content_scroll_x;
} RESIZE_WINDOW;

typedef struct {
  int y;
  int x;
} POINT;

POINT rswin_origin(RESIZE_WINDOW *rswin);

RESIZE_WINDOW *rswin_new(int height, int width, int starty, int startx,
                         int anchor);
void rswin_refresh(RESIZE_WINDOW *rswin, POINT origin);
void rswin_move(RESIZE_WINDOW *rswin, int y, int x);
void rswin_resize(RESIZE_WINDOW *rswin, int height, int width);
void rswin_del(RESIZE_WINDOW *rswin);
void rswin_set_text(RESIZE_WINDOW *rswin, const char *fmt, ...);
void rswin_scroll(RESIZE_WINDOW *rswin, int delta_y, int delta_x);

#endif // __NCURLSES_HTTP_H__
