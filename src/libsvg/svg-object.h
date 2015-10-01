/* Object function header */

#ifndef SVG_OBJECT_H
#define SVG_OBJECT_H

#include <vars.h>

/* Object types */
enum {
    SVG_COLOUR, SVG_ELLIPSE, SVG_POLYLINE, SVG_SPLINE, SVG_TEXT, SVG_ARC,
    SVG_COMPOUND, SVG_ROOT
};

/* Polyline types */
enum {
    SVG_LINE = 1, SVG_BOX, SVG_POLYGON, SVG_ARCBOX, SVG_PICTURE
};

/* Ellipse types */
enum {
    SVG_ELLIPSE_RADII = 1, SVG_ELLIPSE_DIAM, SVG_CIRCLE_RADIUS,
    SVG_CIRCLE_DIAM
};

/* Arc types */
enum {
    SVG_WEDGE, SVG_OPEN
};

/* Spline types */
enum {
    SVG_OPEN_APPROX, SVG_CLOSED_APPROX, SVG_OPEN_INTERP, SVG_CLOSED_INTERP,
    SVG_OPEN_XSPLINE, SVG_CLOSED_XSPLINE
};

extern vhash *svg_create(int orient, float scale);
extern vhash *svg_create_arc(vhash *parent, int subtype, float cx, float cy,
                             float x1, float y1, float x2, float y2, float x3,
                             float y3);
extern vhash *svg_create_box(vhash *parent, float x, float y,
                             float width, float height);
extern vhash *svg_create_compound(vhash *parent);
extern vhash *svg_create_ellipse(vhash *parent, int subtype);
extern vhash *svg_create_line(vhash *parent, float x1, float y1, float x2,
                              float y2);
extern vhash *svg_create_picture(vhash *parent, float x, float y,
                                 float width, float height, char *file);
extern int svg_create_point(vhash *parent, float x, float y);
extern vhash *svg_create_polygon(vhash *parent, float x1, float y1, float x2,
                                 float y2, float x3, float y3);
extern vhash *svg_create_polyline(vhash *parent, int subtype);
extern vhash *svg_create_spline(vhash *parent, int subtype);
extern vhash *svg_create_text(vhash *parent, float x, float y, char *fmt,
                              ...);
extern vhash *svg_create_textbox(vhash *parent, char *font, float fontsize,
                                 int justify, float x, float y, float width,
                                 float height, char *fmt, ...);
extern void svg_destroy(vhash *figure);

#endif
