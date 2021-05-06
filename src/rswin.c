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
  rswin->focus = 0;

  origin = rswin_origin(rswin);

  rswin->container = newwin(height, width, origin.y, origin.x);
  rswin->content = newpad(1, 1);

  box(rswin->container, 0, 0);
  rswin_refresh(rswin, origin);

  return rswin;
}

void rswin_set_focus(RESIZE_WINDOW *rswin, int focus) {
  POINT origin;
  if (rswin->focus == focus) {
    return;
  }

  rswin->focus = focus;
  if (focus == 1) {
    wattron(rswin->container, COLOR_PAIR(1));
  }
  box(rswin->container, ACS_VLINE, ACS_HLINE);

  origin = rswin_origin(rswin);
  rswin_refresh(rswin, origin);
  wattroff(rswin->container, COLOR_PAIR(1));
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
  delwin(rswin->content);
  delwin(rswin->container);
  free(rswin);
}

int rswin_set_text(RESIZE_WINDOW *rswin, const char *fmt, ...) {
  char *buf;
  int bufsize, i;
  int rows, cols, max_cols;
  va_list ap;

  // compute rows and columns of output
  va_start(ap, fmt);
  bufsize = vsnprintf(NULL, 0, fmt, ap) + 1;
  va_end(ap);
  if (bufsize < 0) {
    return -1;
  }

  buf = malloc(sizeof buf * bufsize);
  if (buf == NULL) {
    return -1;
  }

  va_start(ap, fmt);
  if (vsprintf(buf, fmt, ap) < 0) {
    va_end(ap);
    free(buf);
    return -1;
  }
  va_end(ap);

  rows = 0;
  cols = 0;
  max_cols = 0;
  for (i = 0; i < bufsize; i++) {
    if (buf[i] == '\n' || i == bufsize - 1) {
      rows++;
      if (cols > max_cols) {
        max_cols = cols;
      }
      cols = 0;
    } else {
      cols++;
    }
  }
  wresize(rswin->content, rows + 1, max_cols + 1);

  // print text on window
  touchwin(rswin->container);
  werase(rswin->content);
  mvwprintw(rswin->content, 0, 0, buf);

  // refresh window
  POINT origin = rswin_origin(rswin);
  rswin_refresh(rswin, origin);

  free(buf);

  return bufsize;
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
