/* SVG object functions */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "svg-attr.h"
#include "svg-object.h"
#include "svg-util.h"

/* Internal functions */
static vhash *svg_create_object(vhash *parent, int type);

/* Create a new figure */
vhash *
svg_create(float width, float height)
{
    vhash *obj;

    obj = svg_create_object(NULL, SVG_ROOT);

    vh_dstore(obj, "WIDTH", width);
    vh_dstore(obj, "HEIGHT", height);

    vh_sstore(obj, "PENCOLOUR", "black");
    vh_sstore(obj, "FILLCOLOUR", "none");
    vh_istore(obj, "LINEWIDTH", 1);

    svg_set_font(obj, "Times", 10.0);
    vh_fstore(obj, "ANGLE", 0.0);

    svg_set_depth(obj, 100);
    svg_set_justify(obj, SVG_JUSTIFY_CENTRE);
    svg_set_arrowstyle(obj, SVG_CLOSED, SVG_FILLED, 1.0, 4.0, 8.0);

    return obj;
}

/* Create a box object */
vhash *
svg_create_box(vhash *parent, float x, float y, float width, float height)
{
    vhash *obj;

    obj = svg_create_object(parent, SVG_BOX);

    vh_dstore(obj, "X", x);
    vh_dstore(obj, "Y", y);
    vh_dstore(obj, "WIDTH", width);
    vh_dstore(obj, "HEIGHT", height);

    vh_sstore(obj, "PENCOLOUR", "black");
    vh_sstore(obj, "FILLCOLOUR", "none");
    vh_istore(obj, "LINEWIDTH", 1);

    return obj;
}

/* Create a compound object */
vhash *
svg_create_compound(vhash *parent)
{
    return svg_create_object(parent, SVG_COMPOUND);
}

/* Create a line object */
vhash *
svg_create_line(vhash *parent, float x1, float y1, float x2, float y2)
{
    vhash *obj;

    obj = svg_create_polyline(parent);

    svg_create_point(obj, x1, y1);
    svg_create_point(obj, x2, y2);

    return obj;
}

/* Add a new object */
static vhash *
svg_create_object(vhash *parent, int type)
{
    vlist *objects;
    vhash *obj;

    obj = vh_create();
    vh_istore(obj, "TYPE", type);

    if (parent != NULL) {
        vh_pstore(obj, "PARENT", parent);
        objects = vh_add_list(parent, "OBJECTS");
        vl_ppush(objects, obj);
    }

    return obj;
}

/* Add an X,Y point to an object */
int
svg_create_point(vhash *parent, float x, float y)
{
    vlist *list;
    int num;

    list = vh_add_list(parent, "XP");
    vl_dpush(list, x);

    list = vh_add_list(parent, "YP");
    vl_dpush(list, y);

    list = vh_add_list(parent, "SHAPE");
    num = vl_length(list);
    vl_fpush(list, num == 0 ? 0.0 : 1.0);

    return num;
}

/* Create a polyline object */
vhash *
svg_create_polyline(vhash *parent)
{
    vhash *obj;

    obj = svg_create_object(parent, SVG_POLYLINE);

    vh_sstore(obj, "PENCOLOUR", "black");
    vh_sstore(obj, "FILLCOLOUR", "none");
    vh_istore(obj, "LINEWIDTH", 1);

    return obj;
}

/* Create a spline object */
vhash *
svg_create_spline(vhash *parent)
{
    vhash *obj;

    obj = svg_create_object(parent, SVG_SPLINE);

    vh_sstore(obj, "PENCOLOUR", "black");
    vh_sstore(obj, "FILLCOLOUR", "none");
    vh_istore(obj, "LINEWIDTH", 1);

    return obj;
}

/* Create a text object */
vhash *
svg_create_text(vhash *parent, float x, float y, char *fmt, ...)
{
    vhash *obj;
    char *str;

    obj = svg_create_object(parent, SVG_TEXT);

    V_ALLOCA_FMT(str, fmt);
    vh_sstore(obj, "TEXT", str);
    vh_sstore(obj, "TEXTANCHOR", "middle");
    vh_dstore(obj, "X", x);
    vh_dstore(obj, "Y", y);

    return obj;
}

/* Create text that fits in a box */
vhash *
svg_create_textbox(vhash *parent,
                   char *font, float fontsize, int justify,
                   float x, float y, float width, float height,
                   char *fmt, ...)
{
    float xt = 0.0, yt = 0.0, linegap, offset;
    unsigned int i, nrows, ncols;
    vlist *lines;
    vhash *obj;
    char *str;

    V_ALLOCA_FMT(str, fmt);

    /* Reduce font size until it fits the box */
    while (1) {
        linegap = 1 * fontsize / POINTS_PER_INCH;
        ncols = 4 * width * POINTS_PER_INCH / fontsize;

        lines = vl_filltext(str, ncols);
        nrows = vl_length(lines);

        ncols = 0;
        for (i = 0; i < nrows; i++)
            ncols = V_MAX(ncols, strlen(vl_sgetref(lines, i)));

        vl_destroy(lines);

        svg_debug("trying fontsize %g: %d rows, %d cols",
                  fontsize, nrows, ncols);

        if (nrows * linegap <= height)
            break;

        if (fontsize <= 4)
            break;

        fontsize--;
    }

    /* Create text objects */
    obj = svg_create_compound(parent);
    svg_set_font(obj, font, fontsize);
    svg_set_justify(obj, justify);

    switch (justify) {
    case SVG_JUSTIFY_LEFT:
        xt = x;
        break;
    case SVG_JUSTIFY_CENTRE:
        xt = x + width / 2;
        break;
    case SVG_JUSTIFY_RIGHT:
        xt = x + width;
        break;
    }

    lines = vl_filltext(str, ncols);
    nrows = vl_length(lines);

    for (i = 0; i < nrows; i++) {
        offset = i + 0.7 - nrows / 2.0;
        yt = y + height / 2;
        yt += offset * linegap;
        svg_create_text(obj, xt, yt, "%s", vl_sgetref(lines, i));
    }

    vl_destroy(lines);
    return obj;
}

/* Destroy a figure */
void
svg_destroy(vhash *figure)
{
    v_destroy(figure);
}
