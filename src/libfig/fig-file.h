/* File function header */

#ifndef FIG_FILE_H
#define FIG_FILE_H

#include <vars.h>

extern void fig_set_stream(FILE *fp);
extern void fig_set_handler(void (*func)(char *line));
extern void fig_write_figure(vhash *figure);

#endif
