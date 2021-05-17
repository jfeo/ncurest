/*******************************************************************************
 * ctwin module for ncurest                                                    *
 *                                                                             *
 * functions for managing CONTENT_WINDOW structures, wrappers for ncurses      *
 * WINDOW objects.                                                             *
 ******************************************************************************/

#ifndef __NCUREST_CTWIN_H__
#define __NCUREST_CTWIN_H__

#include <ncurses.h>

#define ctwin_ANCHOR_TOP 1
#define ctwin_ANCHOR_BOTTOM 2
#define ctwin_ANCHOR_RIGHT 4
#define ctwin_ANCHOR_LEFT 8

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
  int focus;
} CONTENT_WINDOW;

typedef struct {
  int y;
  int x;
} POINT;

POINT ctwin_origin(CONTENT_WINDOW *ctwin);

CONTENT_WINDOW *ctwin_new(int height, int width, int starty, int startx,
                         int anchor);
void ctwin_refresh(CONTENT_WINDOW *ctwin, POINT origin);
void ctwin_move(CONTENT_WINDOW *ctwin, int y, int x);
void ctwin_resize(CONTENT_WINDOW *ctwin, int height, int width);
void ctwin_del(CONTENT_WINDOW *ctwin);
int ctwin_set_text(CONTENT_WINDOW *ctwin, const char *fmt, ...);
void ctwin_scroll(CONTENT_WINDOW *ctwin, int delta_y, int delta_x);
void ctwin_set_focus(CONTENT_WINDOW *ctwin, int focus);

#endif // __NCUREST_CTWIN_H__
