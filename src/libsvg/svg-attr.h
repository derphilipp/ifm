/* Attribute function header */

#ifndef SVG_ATTR_H
#define SVG_ATTR_H

#include <vars.h>

#define SVG_RESOLUTION 1200

#define SVG_TEXT_WSCALE 7
#define SVG_TEXT_HSCALE 11.25

#define SVG_UNDEF -1

/* Orientation */
enum {
    SVG_LANDSCAPE, SVG_PORTRAIT
};

/* Units */
enum {
    SVG_METRIC, SVG_INCHES
};

/* Indexed colours */
enum {
    SVG_BLACK, SVG_BLUE, SVG_GREEN, SVG_CYAN, SVG_RED, SVG_MAGENTA,
    SVG_YELLOW, SVG_WHITE
};

/* Line styles */
enum {
    SVG_SOLID, SVG_DASH, SVG_DOTTED, SVG_DASH_DOT, SVG_DASH_2_DOTS,
    SVG_DASH_3_DOTS
};

/* Join styles */
enum {
    SVG_MITRE, SVG_BEVEL, SVG_ROUND
};

/* Arrow types */
enum {
    SVG_STICK, SVG_CLOSED, SVG_INDENTED, SVG_POINTED
};

/* Arrow styles */
enum {
    SVG_HOLLOW, SVG_FILLED
};

/* Text justification types */
enum {
    SVG_JUSTIFY_LEFT, SVG_JUSTIFY_CENTRE, SVG_JUSTIFY_RIGHT
};

extern void svg_init_colour(void);
extern void svg_set_arrow(vhash *object, int forward, int backward);
extern void svg_set_arrowstyle(vhash *object, int type, int style,
                               float thick, float width, float height);
extern void svg_set_colour(vhash *object, float r, float g, float b);
extern void svg_set_depth(vhash *object, int depth);
extern void svg_set_fillcolour(vhash *object, float r, float g, float b);
extern void svg_set_fillstyle(vhash *object, int style);
extern void svg_set_font(vhash *object, char *font, float fontsize);
extern void svg_set_joinstyle(vhash *object, int style);
extern void svg_set_justify(vhash *object, int justify);
extern void svg_set_linestyle(vhash *object, int style);
extern void svg_set_linewidth(vhash *object, int width);
extern void svg_set_name(vhash *object, char *fmt, ...);
extern void svg_set_orientation(vhash *figure, int orient);
extern void svg_set_papersize(vhash *figure, char *size);
extern void svg_set_shape(vhash *object, int num, float shape);

#endif
