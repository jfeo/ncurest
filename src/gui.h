#ifndef __NCUREST_GUI_H__
#define __NCUREST_GUI_H__

#include "ctwin.h"

typedef enum {
    CONTROL_SCROLL,
    CONTROL_INPUT,
    CONTROL_BUTTON
} CONTROL_TYPE;

struct input_data {
  char *buf;
  size_t max_length;
  size_t cursor;
};

struct button_data {
  void (*action)(void **);
  void **args;
};

typedef struct {
  CONTROL_TYPE type;
  CONTENT_WINDOW *ctwin;
  union {
    struct input_data input;
    struct button_data button;
  };
} CONTROL;

void gui_ctrl_scroll(CONTROL *ctrl, CONTENT_WINDOW *ctwin);
void gui_ctrl_button(CONTROL *ctrl, CONTENT_WINDOW *ctwin, const char *label,
                     void (*action)(void **), void **args);
void gui_ctrl_input(CONTROL *ctrl, CONTENT_WINDOW *ctwin, char *buf,
                    size_t bufsize, const char *initial);
void gui_ctrl_handle_char(CONTROL *ctrl, int c);
void gui_ctrl_refresh(CONTROL *ctrl);

#endif // __NCUREST_GUI_H__
