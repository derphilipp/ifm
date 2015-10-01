/* Utility function header */

#ifndef SVG_UTIL_H
#define SVG_UTIL_H

#include <vars.h>

#define POINTS_PER_INCH 72.27

#define svg_value(exp)                                                  \
        svg_debug(#exp ": %g", exp)

#define svg_get_ival(object, attr)                                      \
        vs_iget(svg_get_attr(object, attr))

#define svg_get_fval(object, attr)                                      \
        vs_fget(svg_get_attr(object, attr))

#define svg_get_sval(object, attr)                                      \
        vs_sget(svg_get_attr(object, attr))

extern int svg_calc_bbox(vhash *object);
extern void svg_debug(char *fmt, ...);
extern void svg_fatal(char *fmt, ...);
extern vscalar *svg_get_attr(vhash *object, char *attr);
extern vhash *svg_get_figure(vhash *object);
extern float svg_get_height(vhash *figure, float fontsize);
extern float svg_get_width(vhash *figure, float fontsize);

#endif
