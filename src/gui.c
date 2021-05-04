#include <string.h>

#include "gui.h"

void gui_ctrl_button(CONTROL *ctrl, RESIZE_WINDOW *rswin, const char *label,
                     void (*action)(void **), void **args) {
  ctrl->type = CONTROL_TYPE_BUTTON;
  ctrl->rswin = rswin;
  ctrl->button.action = action;
  ctrl->button.args = args;
  rswin_set_text(rswin, label);
}

void gui_ctrl_input(CONTROL *ctrl, RESIZE_WINDOW *rswin, char *buf,
                    size_t bufsize, const char *initial) {
  ctrl->type = CONTROL_TYPE_INPUT;
  ctrl->rswin = rswin;
  ctrl->input.buf = buf;
  ctrl->input.max_length = bufsize;
  ctrl->input.cursor = snprintf(buf, bufsize, "%s", initial);
  rswin_set_text(rswin, initial);
}

void gui_ctrl_scroll(CONTROL *ctrl, RESIZE_WINDOW *rswin) {
  ctrl->type = CONTROL_TYPE_SCROLL;
  ctrl->rswin = rswin;
}

void gui_ctrl_handle_char(CONTROL *ctrl, int c) {
  switch (ctrl->type) {
  case CONTROL_TYPE_SCROLL:
    switch (c) {
    case 258:
      rswin_scroll(ctrl->rswin, 1, 0);
      break;
    case 259:
      rswin_scroll(ctrl->rswin, -1, 0);
      break;
    case 260:
      rswin_scroll(ctrl->rswin, 0, -1);
      break;
    case 261:
      rswin_scroll(ctrl->rswin, 0, 1);
      break;
    }
    break;
  case CONTROL_TYPE_INPUT:
    if (c == 127) {
      if (ctrl->input.cursor > 0) {
        ctrl->input.cursor--;
        ctrl->input.buf[ctrl->input.cursor] = 0;
      }
    } else if (isprint(c) > 0 &&
               ctrl->input.cursor < ctrl->input.max_length) {
      ctrl->input.buf[ctrl->input.cursor] = c;
      ctrl->input.cursor++;
      ctrl->input.buf[ctrl->input.cursor] = 0;
    }
    rswin_set_text(ctrl->rswin, ctrl->input.buf);
    break;
  case CONTROL_TYPE_BUTTON:
    if (c == 0x0A) {
      ctrl->button.action(ctrl->button.args);
    }
    break;
  }
}
