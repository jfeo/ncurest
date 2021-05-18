/******************************************************************************
 * gui module for ncurest                                                     *
 *                                                                            *
 * functions for managing controls built on top of CONTENT_WINDOW structures. *
 ******************************************************************************/

#ifndef __NCUREST_GUI_H__
#define __NCUREST_GUI_H__

#include "ctwin.h"

/**
 * Type of a GUI control, which determines how it is rendered on refresh and
 * how it handles input.
 */
typedef enum { CONTROL_SCROLL, CONTROL_INPUT, CONTROL_BUTTON } CONTROL_TYPE;

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

/**
 * Initialize the given control with the given content window as a scroll
 * control.
 *
 * Arguments:
 *   ctrl   control structure to initialize.
 *   ctwin  content window used to display control.
 */
void gui_ctrl_scroll(CONTROL *ctrl, CONTENT_WINDOW *ctwin);

/**
 * Initialize the given control with the given content window as a button
 * control.
 *
 * Arguments:
 *   ctrl    control structure to initialize.
 *   ctwin   content window used to display control.
 *   label   label to print on the button.
 *   action  the function to call when the button is activated.
 *   args    pointer to the argument array that will be passed to the action.
 */
void gui_ctrl_button(CONTROL *ctrl, CONTENT_WINDOW *ctwin, const char *label,
                     void (*action)(void **), void **args);

/**
 * Initialize the given control with the given content window as an input
 * control.
 *
 * Arguments:
 *   ctrl     control structure to initialize.
 *   ctwin    content window used to display control.
 *   buf      buffer that stores the input value.
 *   bufsize  size of the input value buffer.
 *   initial  initial value of the input value buffer.
 */
void gui_ctrl_input(CONTROL *ctrl, CONTENT_WINDOW *ctwin, char *buf,
                    size_t bufsize, const char *initial);

/**
 * Handle an input character for the control.
 * 
 * Arguments:
 *   ctrl  the control to receive the input character.
 *   c     the character value to handle.
 */
void gui_ctrl_handle_char(CONTROL *ctrl, int c);

/**
 * Refresh the control.
 * 
 * Arguments:
 *   ctrl  control to refresh.
 */
void gui_ctrl_refresh(CONTROL *ctrl);

#endif // __NCUREST_GUI_H__
