#include <curl/curl.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "http.h"
#include "rswin.h"

WINDOW *win;

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

  if (code == CURLE_OK) {
    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    rswin_set_text(status, "HTTP Status %lu", http_code);
  } else {
    rswin_set_text(status, "CURL error %lu", code);
  }
}

void http_send(const char *url, RESIZE_WINDOW *status, RESIZE_WINDOW *body) {
  http_request req;
  req.method = "GET";
  req.uri = "index.html";
  req.header_count = 1;
  req.headers = calloc(1, sizeof *req.headers);
  if (req.headers == NULL) {
    rswin_set_text(status, "Error! Could not generate request.");
  } else {
    rswin_set_text(status, "GET index.html at %s", url);
  }
  req.headers[0].header = "Host";
  req.headers[0].value = url;

  http_response *resp = http_send_request(url, "80", req);
  if (resp == NULL) {
    rswin_set_text(status, "Error occured!");
    return;
  }

  rswin_set_text(status, "%d %s", resp->status_code, resp->reason_phrase);

  if (resp->body != NULL) {
    rswin_set_text(body, (char *)resp->body); 
  }
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

  char url[43] = "example.com";
  size_t url_index = sizeof("example.com") - 1;
  rswin_set_text(rswin_url, url);

  int run = 1;
  int mode = 0;
  while (run == 1) {
    ch = getch();

    if (mode == MODE_WRITE_URL) {
      switch (ch) {
      case 0x0A:
        rswin_scroll(rswin_body, -rswin_body->content_scroll_y,
                     -rswin_body->content_scroll_x);
        http_send(url, rswin_status, rswin_body);
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
        rswin_scroll(rswin_body, -rswin_body->content_scroll_y,
                     -rswin_body->content_scroll_x);
        http_send(url, rswin_status, rswin_body);
      }
      break;
    case 0x09:
      mode = MODE_WRITE_URL;
      break;
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
