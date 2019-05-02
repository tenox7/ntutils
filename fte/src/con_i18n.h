#ifndef CON_I18N_H
#define CON_I18N_H

#include <X11/Xlib.h>

struct i18n_context_t;

/*
 * prototypes for I18N functions
 *
 * C code - need typedef
 */
void i18n_focus_out(i18n_context_t*);
void i18n_focus_in(i18n_context_t*);
int i18n_lookup_sym(i18n_context_t*, XKeyEvent *, char *, int, KeySym *);
i18n_context_t* i18n_open(Display *, Window, unsigned long *);
void i18n_destroy(i18n_context_t**);

#endif // CON_I18N_H
