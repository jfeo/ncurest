#include <stdlib.h>

#include "ctwin.h"

POINT ctwin_origin(CONTENT_WINDOW *ctwin) {
  POINT pt;

  if (ctwin->anchor & ctwin_ANCHOR_BOTTOM) {
    pt.y = ctwin->y + ctwin->height;
  } else {
    pt.y = ctwin->y;
  }

  if (ctwin->anchor & ctwin_ANCHOR_RIGHT) {
    pt.x = ctwin->x - ctwin->width;
  } else {
    pt.x = ctwin->x;
  }

  return pt;
}

void ctwin_refresh(CONTENT_WINDOW *ctwin, POINT origin) {
  wnoutrefresh(ctwin->container);
  pnoutrefresh(ctwin->content, ctwin->content_scroll_y, ctwin->content_scroll_x,
               origin.y + 1, origin.x + 1, origin.y + ctwin->height - 2,
               origin.x + ctwin->width - 2);
  doupdate();
}

CONTENT_WINDOW *ctwin_new(int height, int width, int starty, int startx,
                         int anchor) {
  POINT origin;
  CONTENT_WINDOW *ctwin = malloc(sizeof(CONTENT_WINDOW));

  ctwin->height = height;
  ctwin->width = width;
  ctwin->y = starty;
  ctwin->x = startx;
  ctwin->anchor = anchor;
  ctwin->content_scroll_y = 0;
  ctwin->content_scroll_x = 0;
  ctwin->focus = 0;

  origin = ctwin_origin(ctwin);

  ctwin->container = newwin(height, width, origin.y, origin.x);
  ctwin->content = newpad(1, 1);

  box(ctwin->container, 0, 0);
  ctwin_refresh(ctwin, origin);

  return ctwin;
}

void ctwin_set_focus(CONTENT_WINDOW *ctwin, int focus) {
  POINT origin;
  if (ctwin->focus == focus) {
    return;
  }

  ctwin->focus = focus;
  if (focus == 1) {
    wattron(ctwin->container, COLOR_PAIR(1));
  }
  box(ctwin->container, ACS_VLINE, ACS_HLINE);

  origin = ctwin_origin(ctwin);
  ctwin_refresh(ctwin, origin);
  wattroff(ctwin->container, COLOR_PAIR(1));
}

void ctwin_move(CONTENT_WINDOW *ctwin, int y, int x) {
  POINT origin;

  // clear to prevent artifacts
  wclear(ctwin->container);
  wrefresh(ctwin->container);

  // set new coordinate and compute origin
  ctwin->x = x;
  ctwin->y = y;
  origin = ctwin_origin(ctwin);

  // move
  mvwin(ctwin->container, origin.y, origin.x);

  // redraw
  box(ctwin->container, 0, 0);
  ctwin_refresh(ctwin, origin);
}

void ctwin_resize(CONTENT_WINDOW *ctwin, int height, int width) {
  POINT origin;

  // clear to prevent artifacts
  wclear(ctwin->container);
  wrefresh(ctwin->container);

  ctwin->height = height;
  ctwin->width = width;
  origin = ctwin_origin(ctwin);

  // resize the window
  wresize(ctwin->container, height, width);
  wresize(ctwin->content, height - 2, width - 2);
  mvwin(ctwin->container, origin.y, origin.x);

  // redraw
  box(ctwin->container, 0, 0);
  ctwin_refresh(ctwin, origin);
}

void ctwin_del(CONTENT_WINDOW *ctwin) {
  delwin(ctwin->content);
  delwin(ctwin->container);
  free(ctwin);
}

int ctwin_set_text(CONTENT_WINDOW *ctwin, const char *fmt, ...) {
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
  wresize(ctwin->content, rows + 1, max_cols + 1);

  // print text on window
  touchwin(ctwin->container);
  werase(ctwin->content);
  mvwprintw(ctwin->content, 0, 0, buf);

  // refresh window
  POINT origin = ctwin_origin(ctwin);
  ctwin_refresh(ctwin, origin);

  free(buf);

  return bufsize;
}

void ctwin_scroll(CONTENT_WINDOW *ctwin, int delta_y, int delta_x) {
  POINT origin = ctwin_origin(ctwin);

  ctwin->content_scroll_y += delta_y;
  if (ctwin->content_scroll_y < 0) {
    ctwin->content_scroll_y = 0;
  }
  ctwin->content_scroll_x += delta_x;
  if (ctwin->content_scroll_x < 0) {
    ctwin->content_scroll_x = 0;
  }

  ctwin_refresh(ctwin, origin);
}
