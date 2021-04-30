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
  WINDOW *win;
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

RESIZE_WINDOW *rswin_new(int height, int width, int starty, int startx,
                         int anchor) {
  POINT origin;
  RESIZE_WINDOW *rswin = malloc(sizeof(RESIZE_WINDOW));

  rswin->height = height;
  rswin->width = width;
  rswin->y = starty;
  rswin->x = startx;
  rswin->anchor = anchor;

  origin = rswin_origin(rswin);

  rswin->win = newwin(height, width, origin.y, origin.x);

  box(rswin->win, 0, 0);
  wrefresh(rswin->win);

  return rswin;
}

void rswin_move(RESIZE_WINDOW *rswin, int y, int x) {
  POINT origin;

  // clear to prevent artifacts
  wclear(rswin->win);
  wrefresh(rswin->win);

  // set new coordinate and compute origin
  rswin->x = x;
  rswin->y = y;
  origin = rswin_origin(rswin);

  // move
  mvwin(rswin->win, origin.y, origin.x);

  // redraw
  box(rswin->win, 0, 0);
  wrefresh(rswin->win);
}

void rswin_resize(RESIZE_WINDOW *rswin, int height, int width) {
  POINT origin;

  // clear to prevent artifacts
  wclear(rswin->win);
  wrefresh(rswin->win);

  rswin->height = height;
  rswin->width = width;
  origin = rswin_origin(rswin);

  // resize the window
  wresize(rswin->win, height, width);
  mvwin(rswin->win, origin.y, origin.x);

  // redraw
  box(rswin->win, 0, 0);
  wrefresh(rswin->win);
}

void rswin_del(RESIZE_WINDOW *rswin) {
  delwin(rswin->win);
  free(rswin);
}

void rswin_set_text(RESIZE_WINDOW *rswin, char *text) {
  wclear(rswin->win);
  mvwprintw(rswin->win, 1, 1, "%s", text);
  box(rswin->win, 0, 0);
  wrefresh(rswin->win);
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  RESIZE_WINDOW *rswin = (RESIZE_WINDOW *)userp;

  char* buf = calloc(nmemb, size);
  snprintf(buf, nmemb * size, "%s", (char*)buffer);
  for (size_t i = 0; i < nmemb; i++) {
      if (buf[i] == '\n' || buf[i] == '\r') buf[i] = ' ';
  }
  rswin_set_text(rswin, buf);
  free(buf);

  return size * nmemb;
}

void send_request(CURL *curl, const char *url, RESIZE_WINDOW *rswin) {
  rswin_set_text(rswin, "Sending request... ");
  CURLcode code;
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, "http://emissary.live/");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)rswin);
  code = curl_easy_perform(curl);
}

/**
 * The entrypoint
 **/
int main(int argc, char **argv) {
  int ch;
  RESIZE_WINDOW *rswin_left, *rswin_right;
  CURL *curl;

  signal(SIGINT, sigint_handler);

  /* init curl */
  curl_global_init(CURL_GLOBAL_ALL);

  /* Initialize curses */
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  refresh();

  /* Create a window */
  rswin_right = rswin_new(5, 5, 10, 15, rswin_ANCHOR_TOP | rswin_ANCHOR_RIGHT);

  int run = 1;
  int mode_move = 0;
  while (run == 1) {
    ch = getch();

    switch (ch) {
    case 'q':
      run = 0;
      break;
    case 's':
      send_request(curl, "http://emissary.live", rswin_right);
      break;
    case 'g':
      mvprintw(0, 0, "Testing printing");
      rswin_set_text(rswin_right, "Testing...");
      wrefresh(rswin_right->win);
      break;
    case KEY_LEFT:
      if (mode_move) {
        rswin_move(rswin_right, rswin_right->y, rswin_right->x - 1);
      } else {
        rswin_resize(rswin_right, rswin_right->height, rswin_right->width + 1);
      }
      break;
    case KEY_RIGHT:
      if (mode_move) {
        rswin_move(rswin_right, rswin_right->y, rswin_right->x + 1);
      } else {
        rswin_resize(rswin_right, rswin_right->height, rswin_right->width - 1);
      }
      break;
    case KEY_DOWN:
      if (mode_move) {
        rswin_move(rswin_right, rswin_right->y + 1, rswin_right->x);
      } else {
        rswin_resize(rswin_right, rswin_right->height + 1, rswin_right->width);
      }
      break;
    case KEY_UP:
      if (mode_move) {
        rswin_move(rswin_right, rswin_right->y - 1, rswin_right->x);
      } else {
        rswin_resize(rswin_right, rswin_right->height - 1, rswin_right->width);
      }
    case KEY_RESIZE:
      break;
    case 'm':
      mode_move = mode_move == 0 ? 1 : 0;
      break;
    default:
      break;
    }
  }

  delwin(win);
  endwin();

  return EXIT_SUCCESS;
}
