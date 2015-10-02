/* Object function header */

#ifndef SVG_OBJECT_H
#define SVG_OBJECT_H

#include <vars.h>

/* Object types */
enum {
    SVG_BOX, SVG_POLYLINE, SVG_SPLINE, SVG_TEXT, SVG_COMPOUND, SVG_ROOT
};

/* Polyline types */
enum {
    SVG_LINE = 1, SVG_POLYGON, SVG_ARCBOX, SVG_PICTURE
};

/* Spline types */
enum {
    SVG_OPEN_APPROX, SVG_CLOSED_APPROX, SVG_OPEN_INTERP, SVG_CLOSED_INTERP,
    SVG_OPEN_XSPLINE, SVG_CLOSED_XSPLINE
};

extern vhash *svg_create(float width, float height);
extern vhash *svg_create_box(vhash *parent, float x, float y,
                             float width, float height);
extern vhash *svg_create_compound(vhash *parent);
extern vhash *svg_create_line(vhash *parent, float x1, float y1, float x2,
                              float y2);
extern int svg_create_point(vhash *parent, float x, float y);
extern vhash *svg_create_polyline(vhash *parent);
extern vhash *svg_create_spline(vhash *parent);
extern vhash *svg_create_text(vhash *parent, float x, float y, char *fmt,
                              ...);
extern vhash *svg_create_textbox(vhash *parent, char *font, float fontsize,
                                 int justify, float x, float y, float width,
                                 float height, char *fmt, ...);
extern void svg_destroy(vhash *figure);

#endif
