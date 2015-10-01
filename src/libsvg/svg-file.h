/* File function header */

#ifndef SVG_FILE_H
#define SVG_FILE_H

#include <vars.h>

extern void svg_set_stream(FILE *fp);
extern void svg_set_handler(void (*func)(char *line));
extern void svg_write_figure(vhash *figure);

#endif
