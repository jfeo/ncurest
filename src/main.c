#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ctwin.h"
#include "gui.h"
#include "http.h"
#include "net.h"

void btn_send_action(void **arg) {
  CONTENT_WINDOW *ctwin_body, *ctwin_status;
  http_request *req;
  int socket_fd;
  char *domain;

  ctwin_body = (CONTENT_WINDOW *)arg[0];
  ctwin_status = (CONTENT_WINDOW *)arg[1];
  req = (http_request *)arg[2];
  domain = (char *)arg[3];

  ctwin_set_text(ctwin_body, "");
  ctwin_scroll(ctwin_body, -ctwin_body->content_scroll_y,
               -ctwin_body->content_scroll_x);

  ctwin_set_text(ctwin_status, "Establishing connection...");

  socket_fd = net_tcp_connect(domain, "80");
  if (socket_fd == -1) {
    ctwin_set_text(ctwin_status, "Could not establish connection.");
    return;
  }

  ctwin_set_text(ctwin_status, "Sending request...");

  if (http_send(socket_fd, *req) == -1) {
    // error
    ctwin_set_text(ctwin_status, "Error occured during sending.");
    close(socket_fd);
    return;
  }

  ctwin_set_text(ctwin_status, "Receiving response...");

  http_response *resp = http_recv(socket_fd);
  if (resp == NULL) {
    ctwin_set_text(ctwin_status, "Error occured during receiving.");
    close(socket_fd);
    return;
  }

  ctwin_set_text(ctwin_status, "%d %s", resp->status_code, resp->reason_phrase);

  if (resp->body != NULL) {
    ctwin_set_text(ctwin_body, (char *)resp->body);
  } else {
    ctwin_set_text(ctwin_body, "");
  }

  close(socket_fd);
}

void btn_header_action(void **args) {
  CONTROL *btn = args[0];
  http_header **hdr = args[1];
  CONTROL *inp_hdr = args[2];
  CONTROL *inp_val = args[3];
  int *index = args[4];

  *index = ((*index) + 1) % 2;

  ctwin_set_text(btn->ctwin, "%d", *index + 1);

  inp_hdr->input.buf = hdr[*index]->header;
  inp_val->input.buf = hdr[*index]->value;

  gui_ctrl_refresh(inp_hdr);
  gui_ctrl_refresh(inp_val);
}

/**
 * The entrypoint
 **/
int main(int argc, char **argv) {
  int ch;
  char buf_method[6], buf_domain[22], buf_uri[49];
  char buf_hdr_name_1[24];
  char buf_hdr_value_1[49];
  char buf_hdr_name_2[24];
  char buf_hdr_value_2[49];

  CONTROL btn_send, btn_header, inp_domain, inp_method, inp_uri, inp_hdr_name,
      inp_hdr_value, scr_body;
  CONTENT_WINDOW *ctwin_domain, *ctwin_method, *ctwin_uri, *ctwin_status,
      *ctwin_body, *ctwin_send, *ctwin_hdr_number, *ctwin_hdr_name,
      *ctwin_hdr_value;

  // Initialze the request
  http_request req;
  http_header h1 = {.header = buf_hdr_name_1, .value = buf_hdr_value_1};
  http_header h2 = {.header = buf_hdr_name_2, .value = buf_hdr_value_2};
  http_header *hdr[] = {&h1, &h2};
  req.method = buf_method;
  req.uri = buf_uri;
  req.header_count = 2;
  req.headers = *hdr;

  /* Initialize curses */
  initscr();
  start_color();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  refresh();

  // initialize color
  init_pair(1, COLOR_BLUE, COLOR_BLACK);

  /* Define the user interface layout */
  ctwin_method = ctwin_new(3, 7, 0, 0, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  ctwin_domain = ctwin_new(3, 23, 0, 7, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  ctwin_uri = ctwin_new(3, 50, 0, 30, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  ctwin_hdr_number =
      ctwin_new(3, 5, 3, 0, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  ctwin_hdr_name = ctwin_new(3, 25, 3, 5, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  ctwin_hdr_value =
      ctwin_new(3, 50, 3, 30, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  ctwin_body = ctwin_new(20, 80, 6, 0, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  ctwin_send = ctwin_new(3, 10, 26, 0, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  ctwin_status = ctwin_new(3, 70, 26, 10, ctwin_ANCHOR_TOP | ctwin_ANCHOR_LEFT);
  mvprintw(29, 0, "Exit with CTRL+C or F1.");

  int header_index = 0;
  void *btn_send_args[4] = {ctwin_body, ctwin_status, &req, buf_domain};
  void *btn_header_args[5] = {&btn_header, &hdr, &inp_hdr_name, &inp_hdr_value,
                              &header_index};

  sprintf(hdr[1]->header, "User-Agent");
  sprintf(hdr[1]->value, "ncurest/0.0");

  // Create GUI widgets
  gui_ctrl_input(&inp_domain, ctwin_domain, buf_domain, 22, "example.com");
  gui_ctrl_input(&inp_method, ctwin_method, buf_method, 6, "GET");
  gui_ctrl_input(&inp_uri, ctwin_uri, buf_uri, 49, "/index.html");
  gui_ctrl_input(&inp_hdr_name, ctwin_hdr_name, hdr[0]->header, 24, "Host");
  gui_ctrl_input(&inp_hdr_value, ctwin_hdr_value, hdr[0]->value, 49,
                 "example.com");
  gui_ctrl_button(&btn_send, ctwin_send, "Send", btn_send_action,
                  btn_send_args);
  gui_ctrl_button(&btn_header, ctwin_hdr_number, "1", btn_header_action,
                  btn_header_args);
  gui_ctrl_scroll(&scr_body, ctwin_body);

  CONTROL *foci[] = {&inp_method,   &inp_domain,    &inp_uri,  &btn_header,
                     &inp_hdr_name, &inp_hdr_value, &scr_body, &btn_send};

  // set initial focus
  ctwin_set_focus(ctwin_method, 1);
  CONTROL **focus = &foci[0];

  while (true) {
    ch = getch();

    if (ch == 3 || ch == 265) {
      break;
    }

    if (ch == 0x09 || ch == 353) {
      curs_set(0);

      ctwin_set_focus((*focus)->ctwin, 0);
      if (ch == 0x09) {
        focus++;
        if (focus >= &foci[8]) {
          focus = &foci[0];
        }
      } else {
        focus--;
        if (focus < &foci[0]) {
          focus = &foci[7];
        }
      }
      ctwin_set_focus((*focus)->ctwin, 1);
      if ((*focus)->type == CONTROL_INPUT) {
        curs_set(1);
      }
      continue;
    }

    // pass control to focused gui widget
    gui_ctrl_handle_char(*focus, ch);
  }

  ctwin_del(ctwin_body);
  ctwin_del(ctwin_status);
  ctwin_del(ctwin_domain);
  ctwin_del(ctwin_uri);
  ctwin_del(ctwin_send);

  endwin();

  return EXIT_SUCCESS;
}
