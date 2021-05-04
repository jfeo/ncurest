#ifndef __NCUREST_GUI_H__
#define __NCUREST_GUI_H__

#include "rswin.h"

#define CONTROL_TYPE_SCROLL 1
#define CONTROL_TYPE_INPUT 2
#define CONTROL_TYPE_BUTTON 3

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
  int type;
  RESIZE_WINDOW *rswin;
  union {
    struct input_data input;
    struct button_data button;
  };
} CONTROL;

void gui_ctrl_scroll(CONTROL *ctrl, RESIZE_WINDOW *rswin);
void gui_ctrl_button(CONTROL *ctrl, RESIZE_WINDOW *rswin, const char *label,
                     void (*action)(void **), void **args);
void gui_ctrl_input(CONTROL *ctrl, RESIZE_WINDOW *rswin, char *buf,
                    size_t bufsize, const char *initial);
void gui_ctrl_handle_char(CONTROL *ctrl, int c);

#endif // __NCUREST_GUI_H__
