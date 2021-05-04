#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#include "gui.h"
#include "http.h"
#include "net.h"
#include "rswin.h"

void btn_send_action(void **arg) {
  RESIZE_WINDOW *rswin_body, *rswin_status;
  http_request *req;
  int socket_fd;
  char *domain;

  rswin_body = (RESIZE_WINDOW *)arg[0];
  rswin_status = (RESIZE_WINDOW *)arg[1];
  req = (http_request *)arg[2];
  domain = (char *)arg[3];

  rswin_set_text(rswin_body, "");
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
  CONTROL btn_send, inp_domain, inp_method, inp_uri, scr_body;
  RESIZE_WINDOW *rswin_domain, *rswin_method, *rswin_uri, *rswin_status,
      *rswin_body, *rswin_send;

  /* Initialize curses */
  initscr();
  start_color();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  refresh();

  // initialize color
  init_pair(1, COLOR_BLUE, COLOR_BLACK);

  // Initialze the request
  http_request req;
  http_header hdr;
  req.method = buf_method;
  req.uri = buf_uri;
  req.header_count = 1;
  req.headers = &hdr;
  hdr.header = "Host";
  hdr.value = buf_domain;

  /* Define the user interface layout */
  rswin_method = rswin_new(3, 7, 0, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_domain = rswin_new(3, 23, 0, 7, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_uri = rswin_new(3, 50, 0, 30, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_body = rswin_new(20, 80, 3, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_send = rswin_new(3, 10, 23, 0, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  rswin_status = rswin_new(3, 70, 23, 10, rswin_ANCHOR_TOP | rswin_ANCHOR_LEFT);
  mvprintw(26, 0, "Exit with CTRL+C or F1.");

  void *args[4] = {rswin_body, rswin_status, &req, buf_domain};

  // Create GUI widgets
  gui_ctrl_input(&inp_domain, rswin_domain, buf_domain, 22, "example.com");
  gui_ctrl_input(&inp_method, rswin_method, buf_method, 6, "GET");
  gui_ctrl_input(&inp_uri, rswin_uri, buf_uri, 49, "/index.html");
  gui_ctrl_button(&btn_send, rswin_send, "Send", btn_send_action, args);
  gui_ctrl_scroll(&scr_body, rswin_body);

  CONTROL foci[] = {inp_method, inp_domain, inp_uri, scr_body, btn_send};

  // set initial focus
  rswin_set_focus(rswin_method, 1);
  CONTROL *focus = &foci[0];

  while (true) {
    ch = getch();

    if (ch == 3 || ch == 265) {
      break;
    }

    if (ch == 0x09 || ch == 353) {
      curs_set(0);

      rswin_set_focus(focus->rswin, 0);
      if (ch == 0x09) {
        focus++;
        if (focus >= &foci[5]) {
          focus = &foci[0];
        }
      } else {
        focus--;
        if (focus < &foci[0]) {
          focus = &foci[4];
        }
      }
      rswin_set_focus(focus->rswin, 1);
      if (focus->type == CONTROL_TYPE_INPUT) {
        curs_set(1);
      }
      continue;
    }

    // pass control to focused gui widget
    gui_ctrl_handle_char(focus, ch);
  }

  rswin_del(rswin_body);
  rswin_del(rswin_status);
  rswin_del(rswin_domain);
  rswin_del(rswin_uri);
  rswin_del(rswin_send);

  endwin();

  return EXIT_SUCCESS;
}
