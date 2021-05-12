#include <ctype.h>
#include <string.h>

#include "gui.h"

void gui_ctrl_button(CONTROL *ctrl, CONTENT_WINDOW *ctwin, const char *label,
                     void (*action)(void **), void **args) {
  ctrl->type = CONTROL_TYPE_BUTTON;
  ctrl->ctwin = ctwin;
  ctrl->button.action = action;
  ctrl->button.args = args;
  ctwin_set_text(ctwin, label);
}

void gui_ctrl_input(CONTROL *ctrl, CONTENT_WINDOW *ctwin, char *buf,
                    size_t bufsize, const char *initial) {
  ctrl->type = CONTROL_TYPE_INPUT;
  ctrl->ctwin = ctwin;
  ctrl->input.buf = buf;
  ctrl->input.max_length = bufsize;
  ctrl->input.cursor = snprintf(buf, bufsize, "%s", initial);
  ctwin_set_text(ctwin, initial);
}

void gui_ctrl_scroll(CONTROL *ctrl, CONTENT_WINDOW *ctwin) {
  ctrl->type = CONTROL_TYPE_SCROLL;
  ctrl->ctwin = ctwin;
}

void gui_ctrl_refresh(CONTROL *ctrl) {
  if (ctrl->type == CONTROL_TYPE_INPUT) {
    ctrl->input.cursor = strlen(ctrl->input.buf);
    ctwin_set_text(ctrl->ctwin, "%s", ctrl->input.buf);
  }
}

void gui_ctrl_handle_char(CONTROL *ctrl, int c) {
  switch (ctrl->type) {
  case CONTROL_TYPE_SCROLL:
    switch (c) {
    case 258:
      ctwin_scroll(ctrl->ctwin, 1, 0);
      break;
    case 259:
      ctwin_scroll(ctrl->ctwin, -1, 0);
      break;
    case 260:
      ctwin_scroll(ctrl->ctwin, 0, -1);
      break;
    case 261:
      ctwin_scroll(ctrl->ctwin, 0, 1);
      break;
    }
    break;
  case CONTROL_TYPE_INPUT:
    if (c == 127) {
      if (ctrl->input.cursor > 0) {
        ctrl->input.cursor--;
        ctrl->input.buf[ctrl->input.cursor] = 0;
      }
    } else if (isprint(c) > 0 && ctrl->input.cursor < ctrl->input.max_length) {
      ctrl->input.buf[ctrl->input.cursor] = c;
      ctrl->input.cursor++;
      ctrl->input.buf[ctrl->input.cursor] = 0;
    }
    ctwin_set_text(ctrl->ctwin, "%s", ctrl->input.buf);
    break;
  case CONTROL_TYPE_BUTTON:
    if (c == 0x0A) {
      ctrl->button.action(ctrl->button.args);
    }
    break;
  }
}
