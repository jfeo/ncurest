#include <stdlib.h>

#include "rswin.h"

POINT rswin_origin(RESIZE_WINDOW *rswin) {
  POINT pt;

  if (rswin->anchor & rswin_ANCHOR_BOTTOM) {
    pt.y = rswin->y + rswin->height;
  } else {
    pt.y = rswin->y;
  }

  if (rswin->anchor & rswin_ANCHOR_RIGHT) {
    pt.x = rswin->x - rswin->width;
  } else {
    pt.x = rswin->x;
  }

  return pt;
}

void rswin_refresh(RESIZE_WINDOW *rswin, POINT origin) {
  wnoutrefresh(rswin->container);
  pnoutrefresh(rswin->content, rswin->content_scroll_y, rswin->content_scroll_x,
               origin.y + 1, origin.x + 1, origin.y + rswin->height - 2,
               origin.x + rswin->width - 2);
  doupdate();
}

RESIZE_WINDOW *rswin_new(int height, int width, int starty, int startx,
                         int anchor) {
  POINT origin;
  RESIZE_WINDOW *rswin = malloc(sizeof(RESIZE_WINDOW));

  rswin->height = height;
  rswin->width = width;
  rswin->y = starty;
  rswin->x = startx;
  rswin->anchor = anchor;
  rswin->content_scroll_y = 0;
  rswin->content_scroll_x = 0;

  origin = rswin_origin(rswin);

  rswin->container = newwin(height, width, origin.y, origin.x);
  rswin->content = newpad(height * 2, width * 2);

  box(rswin->container, 0, 0);
  rswin_refresh(rswin, origin);

  return rswin;
}

void rswin_move(RESIZE_WINDOW *rswin, int y, int x) {
  POINT origin;

  // clear to prevent artifacts
  wclear(rswin->container);
  wrefresh(rswin->container);

  // set new coordinate and compute origin
  rswin->x = x;
  rswin->y = y;
  origin = rswin_origin(rswin);

  // move
  mvwin(rswin->container, origin.y, origin.x);

  // redraw
  box(rswin->container, 0, 0);
  rswin_refresh(rswin, origin);
}

void rswin_resize(RESIZE_WINDOW *rswin, int height, int width) {
  POINT origin;

  // clear to prevent artifacts
  wclear(rswin->container);
  wrefresh(rswin->container);

  rswin->height = height;
  rswin->width = width;
  origin = rswin_origin(rswin);

  // resize the window
  wresize(rswin->container, height, width);
  wresize(rswin->content, height - 2, width - 2);
  mvwin(rswin->container, origin.y, origin.x);

  // redraw
  box(rswin->container, 0, 0);
  rswin_refresh(rswin, origin);
}

void rswin_del(RESIZE_WINDOW *rswin) {
  delwin(rswin->container);
  free(rswin);
}

void rswin_set_text(RESIZE_WINDOW *rswin, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  touchwin(rswin->container);
  wclear(rswin->content);
  wmove(rswin->content, 0, 0);
  vw_printw(rswin->content, fmt, ap);

  va_end(ap);

  POINT origin = rswin_origin(rswin);
  rswin_refresh(rswin, origin);
}

void rswin_scroll(RESIZE_WINDOW *rswin, int delta_y, int delta_x) {
  POINT origin = rswin_origin(rswin);

  rswin->content_scroll_y += delta_y;
  if (rswin->content_scroll_y < 0) {
    rswin->content_scroll_y = 0;
  }
  rswin->content_scroll_x += delta_x;
  if (rswin->content_scroll_x < 0) {
    rswin->content_scroll_x = 0;
  }

  rswin_refresh(rswin, origin);
}
