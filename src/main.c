#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#include "http.h"
#include "net.h"
#include "rswin.h"

WINDOW *win;

typedef struct {
  RESIZE_WINDOW *rswin;
  char *buf;
  size_t max_length;
  size_t cursor;
} TEXT_INPUT;

typedef struct {
  RESIZE_WINDOW *rswin;
  void (*action)(void **);
} BUTTON;

#define FOCUS_SCROLLER 1
#define FOCUS_INPUT 2
#define FOCUS_BUTTON 3
typedef struct focus {
  int type;
  union {
    RESIZE_WINDOW *scroller;
    TEXT_INPUT *input;
    BUTTON *button;
  };
} FOCUS;

void btn_send_action(void **arg) {
  RESIZE_WINDOW *rswin_body, *rswin_status;
  http_request *req;
  int socket_fd;
  char *domain;

  endwin();

  rswin_body = (RESIZE_WINDOW *)arg[0];
  rswin_status = (RESIZE_WINDOW *)arg[1];
  req = (http_request *)arg[2];
  domain = (char *)arg[3];

  rswin_scroll(rswin_body, -rswin_body->content_scroll_y,
               -rswin_body->content_scroll_x);

  rswin_set_text(rswin_status, "Establishing connection...");

  socket_fd = net_tcp_connect(domain, "80");

  rswin_set_text(rswin_status, "Sending request...");

  if (http_send(socket_fd, *req) == -1) {
    // error
    rswin_set_text(rswin_status, "Error occured during sending.");
    return;
  }

  rswin_set_text(rswin_status, "Receiving response...");

  http_response *resp = http_recv(socket_fd);
  if (resp == NULL) {
    rswin_set_text(rswin_status, "Error occured during receiving.");
    return;
  }

  rswin_set_text(rswin_status, "%d %s", resp->status_code, resp->reason_phrase);

  if (resp->body != NULL) {
    rswin_set_text(rswin_body, (char *)resp->body);
  } else {
    rswin_set_text(rswin_body, "");
  }
}

/**
 * The entrypoint
 **/
int main(int argc, char **argv) {
  int ch;
  char buf_method[6], buf_domain[22], buf_uri[49];
  BUTTON btn_send;
  TEXT_INPUT inp_domain, inp_method, inp_uri;
  RESIZE_WINDOW *rswin_domain, *rswin_method, *rswin_uri, *rswin_status,
      *rswin_body, *rswin_send;

  /* Initialize curses */
  initscr();
  start_color();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  refresh();
  curs_set(0);

  // initialize color
  init_pair(1, COLOR_BLUE, COLOR_BLACK);

  /* Define the user interface layout */
  rswin_method = rswin_new(3, 7, 0, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_domain = rswin_new(3, 23, 0, 7, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_uri = rswin_new(3, 50, 0, 30, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_body = rswin_new(20, 80, 3, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_send = rswin_new(3, 10, 23, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_status = rswin_new(3, 70, 23, 10, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  mvprintw(26, 0, "Exit with CTRL+C or F1.");

  /* Create the text inputs*/
  inp_domain.buf = buf_domain;
  snprintf(buf_domain, 22, "example.com");
  rswin_set_text(rswin_domain, buf_domain);
  inp_domain.max_length = 21;
  inp_domain.cursor = sizeof "example.com" - 1;
  inp_domain.rswin = rswin_domain;

  inp_method.buf = buf_method;
  snprintf(buf_method, 6, "GET");
  rswin_set_text(rswin_method, buf_method);
  inp_method.max_length = 5;
  inp_method.cursor = sizeof "GET" - 1;
  inp_method.rswin = rswin_method;

  inp_uri.buf = buf_uri;
  snprintf(buf_uri, 49, "/index.html");
  rswin_set_text(rswin_uri, buf_uri);
  inp_uri.max_length = 48;
  inp_uri.cursor = sizeof "/index.html" - 1;
  inp_uri.rswin = rswin_uri;

  // Create the send button
  rswin_set_text(rswin_send, "Send");
  btn_send.action = btn_send_action;
  btn_send.rswin = rswin_send;

  // Initialze the request
  http_request req;
  http_header hdr;
  req.method = buf_method;
  req.uri = buf_uri;
  req.header_count = 1;
  req.headers = &hdr;
  hdr.header = "Host";
  hdr.value = buf_domain;

  void *args[4] = {rswin_body, rswin_status, &req, buf_domain};

  FOCUS focuses[5];
  focuses[0].type = FOCUS_INPUT;
  focuses[0].input = &inp_method;
  focuses[1].type = FOCUS_INPUT;
  focuses[1].input = &inp_domain;
  focuses[2].type = FOCUS_INPUT;
  focuses[2].input = &inp_uri;
  focuses[3].type = FOCUS_SCROLLER;
  focuses[3].scroller = rswin_body;
  focuses[4].type = FOCUS_BUTTON;
  focuses[4].button = &btn_send;

  // set initial focus
  int fidx = 0;
  rswin_set_focus(rswin_method, 1);

  while (true) {
    ch = getch();

    if (ch == 3 || ch == 265) {
      break;
    }

    if (ch == 0x09 || ch == 353) {
      curs_set(0);

      if (focuses[fidx].type == FOCUS_SCROLLER) {
        rswin_set_focus(focuses[fidx].scroller, 0);
      }
      if (focuses[fidx].type == FOCUS_INPUT) {
        rswin_set_focus(focuses[fidx].input->rswin, 0);
      }
      if (focuses[fidx].type == FOCUS_BUTTON) {
        rswin_set_focus(focuses[fidx].button->rswin, 0);
      }

      if (ch == 0x09) {
        fidx = (fidx + 1) % 5;
      } else {
        fidx = (fidx - 1);
        if (fidx < 0) {
          fidx = 4;
        }
      }

      if (focuses[fidx].type == FOCUS_SCROLLER) {
        rswin_set_focus(focuses[fidx].scroller, 1);
      }
      if (focuses[fidx].type == FOCUS_INPUT) {
        rswin_set_focus(focuses[fidx].input->rswin, 1);
        curs_set(1);
      }
      if (focuses[fidx].type == FOCUS_BUTTON) {
        rswin_set_focus(focuses[fidx].button->rswin, 1);
      }
      continue;
    }

    switch (focuses[fidx].type) {
    case FOCUS_SCROLLER:
      switch (ch) {
      case 258:
        rswin_scroll(rswin_body, 1, 0);
        break;
      case 259:
        rswin_scroll(rswin_body, -1, 0);
        break;
      case 260:
        rswin_scroll(rswin_body, 0, -1);
        break;
      case 261:
        rswin_scroll(rswin_body, 0, 1);
        break;
      }
      break;
    case FOCUS_INPUT:
      if (ch == 127) {
        if (focuses[fidx].input->cursor > 0) {
          focuses[fidx].input->cursor--;
          focuses[fidx].input->buf[focuses[fidx].input->cursor] = 0;
        }
      } else if (isprint(ch) > 0 && focuses[fidx].input->cursor <
                                        focuses[fidx].input->max_length) {
        focuses[fidx].input->buf[focuses[fidx].input->cursor] = ch;
        focuses[fidx].input->cursor++;
        focuses[fidx].input->buf[focuses[fidx].input->cursor] = 0;
      }
      rswin_set_text(focuses[fidx].input->rswin, focuses[fidx].input->buf);
      break;
    case FOCUS_BUTTON:
      if (ch == 0x0A) {
        focuses[fidx].button->action(args);
      }
      break;
    }
  }

  rswin_del(rswin_body);
  rswin_del(rswin_status);
  rswin_del(rswin_domain);
  rswin_del(rswin_uri);
  rswin_del(rswin_send);
  delwin(win);
  endwin();
  return EXIT_SUCCESS;
}
