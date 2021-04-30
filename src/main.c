#include <curl/curl.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

WINDOW *win;
#define rswin_ANCHOR_TOP 1
#define rswin_ANCHOR_BOTTOM 2
#define rswin_ANCHOR_RIGHT 4
#define rswin_ANCHOR_LEFT 8

/**
 * Handle interrupt signals
 *
 * Make sure to clean up ncurses..
 **/
void sigint_handler(int param) {
  delwin(win);
  endwin();
  exit(1);
}

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
               origin.y + 1, origin.x + 1, origin.y + rswin->height - 1,
               origin.x + rswin->width - 1);
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
  rswin->content = newpad(height - 2, width - 2);

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

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  RESIZE_WINDOW *rswin = (RESIZE_WINDOW *)userp;

  char *buf = calloc(nmemb, size);
  snprintf(buf, nmemb * size, "%s", (char *)buffer);
  for (size_t i = 0; i < nmemb; i++)
    if (buf[i] == '\r')
      buf[i] = ' ';
  rswin_set_text(rswin, buf);
  free(buf);

  return size * nmemb;
}

void send_request(CURL *curl, const char *url, RESIZE_WINDOW *content,
                  RESIZE_WINDOW *status) {
  CURLcode code;

  rswin_set_text(status, "GET %s", url);

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)content);
  code = curl_easy_perform(curl);

  long http_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  rswin_set_text(status, "Status %lu", http_code);
}

void rswin_scroll(RESIZE_WINDOW *rswin, int delta_y, int delta_x) {
  POINT origin = rswin_origin(rswin);

  rswin->content_scroll_y += delta_y;
  rswin->content_scroll_x += delta_x;

  rswin_refresh(rswin, origin);
}

#define MODE_WINDOW 0
#define MODE_WRITE_URL 1

/**
 * The entrypoint
 **/
int main(int argc, char **argv) {
  int ch;
  RESIZE_WINDOW *rswin_url, *rswin_status, *rswin_body;
  CURL *curl;

  signal(SIGINT, sigint_handler);

  /* init curl */
  curl_global_init(CURL_GLOBAL_ALL);

  /* Initialize curses */
  initscr();
  raw();
  noecho();
  keypad(stdscr, TRUE);
  refresh();

  /* Create a window */
  rswin_url = rswin_new(3, 42, 0, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_status = rswin_new(3, 42, 3, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_body =
      rswin_new(40, 100 + 2, 6, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);

  char url[43] = "http://example.com";
  size_t url_index = sizeof("http://example.com") - 1;
  rswin_set_text(rswin_url, url);

  int run = 1;
  int mode = 0;
  while (run == 1) {
    ch = getch();

    if (mode == MODE_WRITE_URL) {
      switch (ch) {
      case 0x0A:
        send_request(curl, url, rswin_body, rswin_status);
      case 0x09:
        mode = MODE_WINDOW;
        break;
      case 127:
        if (url_index > 0) {
          url_index--;
          url[url_index] = 0;
        }
        break;
      default:
        if (url_index < 42) {
          url[url_index] = ch;
          url_index++;
        }
        break;
      }
      rswin_set_text(rswin_url, url);
      continue;
    }

    switch (ch) {
    case 'q':
      run = 0;
      break;
    case 0x0A:
      if (mode == MODE_WINDOW) {
        send_request(curl, url, rswin_body, rswin_status);
      }
      break;
    case 0x09:
      mode = MODE_WRITE_URL;
      break;
    case 258:
      rswin_scroll(rswin_body, 1, 0);
      rswin_set_text(rswin_status, "scroll (%d, %d)",
                     rswin_body->content_scroll_y,
                     rswin_body->content_scroll_x);
      break;
    case 259:
      rswin_scroll(rswin_body, -1, 0);
      rswin_set_text(rswin_status, "scroll (%d, %d)",
                     rswin_body->content_scroll_y,
                     rswin_body->content_scroll_x);
      break;
    case KEY_RESIZE:
      break;
    default:
      rswin_set_text(rswin_status, "Unknown Key Code = %d", ch);
      break;
    }
  }

  delwin(win);
  endwin();

  return EXIT_SUCCESS;
}
