/* Utility functions */

#include <stdio.h>
#include <stdarg.h>
#include "svg-attr.h"
#include "svg-object.h"
#include "svg-util.h"

/* Calculate bounding box of an object */
int
svg_calc_bbox(vhash *object)
{
    int type, bxmin = 0, bxmax = 0, bymin = 0, bymax = 0, width, height;
    int i, num, xmin, xmax, ymin, ymax, x, y, justify, count = 0;
    vlist *objects, *xp, *yp;
    vhash *obj, *figure;
    float fontsize;
    viter iter;
    char *text;

    type = vh_iget(object, "TYPE");

    switch (type) {

    case SVG_ROOT:
    case SVG_COMPOUND:
        if ((objects = vh_pget(object, "OBJECTS")) == NULL)
            return 0;

        v_iterate(objects, iter) {
            obj = vl_iter_pval(iter);
            if (!svg_calc_bbox(obj))
                continue;

            xmin = vh_iget(obj, "XMIN");
            xmax = vh_iget(obj, "XMAX");
            ymin = vh_iget(obj, "YMIN");
            ymax = vh_iget(obj, "YMAX");

            if (count++ == 0) {
                bxmin = xmin;
                bxmax = xmax;
                bymin = ymin;
                bymax = ymax;
            } else {
                bxmin = V_MIN(xmin, bxmin);
                bxmax = V_MAX(xmax, bxmax);
                bymin = V_MIN(ymin, bymin);
                bymax = V_MAX(ymax, bymax);
            }
        }

        break;

    case SVG_TEXT:
        x = vh_iget(object, "X");
        y = vh_iget(object, "Y");

        figure = svg_get_figure(object);
        fontsize = svg_get_fval(object, "FONTSIZE");

        text = svg_get_sval(object, "TEXT");
        width = svg_get_width(figure, fontsize) * strlen(text);
        height = svg_get_height(figure, fontsize);
        justify = svg_get_ival(object, "JUSTIFY");

        vh_fstore(object, "WIDTH", width);
        vh_fstore(object, "HEIGHT", height);

        switch (justify) {
        case SVG_JUSTIFY_LEFT:
            bxmin = x;
            bxmax = x + width;
            break;
        case SVG_JUSTIFY_CENTRE:
            bxmin = x - width / 2;
            bxmax = x + width / 2;
            break;
        case SVG_JUSTIFY_RIGHT:
            bxmin = x - width;
            bxmax = x;
            break;
        }

        bymin = y - height;
        bymax = y;
        break;

    default:
        if ((xp = vh_pget(object, "XP")) == NULL)
            return 0;

        if ((yp = vh_pget(object, "YP")) == NULL)
            return 0;

        num = vl_length(xp);
        for (i = 0; i < num; i++) {
            x = vl_iget(xp, i);
            y = vl_iget(yp, i);

            if (count++ == 0) {
                bxmin = bxmax = x;
                bymin = bymax = y;
            } else {
                bxmin = V_MIN(x, bxmin);
                bxmax = V_MAX(x, bxmax);
                bymin = V_MIN(y, bymin);
                bymax = V_MAX(y, bymax);
            }
        }

        break;
    }

    vh_istore(object, "XMIN", bxmin);
    vh_istore(object, "XMAX", bxmax);
    vh_istore(object, "YMIN", bymin);
    vh_istore(object, "YMAX", bymax);

    svg_debug("bounding box of %s: %d-%d, %d-%d",
              vh_exists(object, "NAME") ? vh_sgetref(object, "NAME") : "object",
              bxmin, bxmax, bymin, bymax);

    return 1;
}

/* Print a debugging message */
void
svg_debug(char *fmt, ...)
{
    char *str;

    if (getenv("SVG_DEBUG") != NULL) {
        V_ALLOCA_FMT(str, fmt);
        fprintf(stderr, "Svg: %s\n", str);
    }
}

/* Print a fatal error and die */
void
svg_fatal(char *fmt, ...)
{
    char *str;

    V_ALLOCA_FMT(str, fmt);
    fprintf(stderr, "Svg fatal: %s\n", str);
    exit(2);
}

/* Get an attribute of an object */
vscalar *
svg_get_attr(vhash *object, char *attr)
{
    vhash *parent;

    while (!vh_exists(object, attr) &&
           (parent = vh_pget(object, "PARENT")) != NULL)
        object = parent;

#if 0
    if (!vh_exists(object, attr))
        svg_fatal("no such attribute: %s", attr);
#endif

    svg_debug("%s = %s", attr, vh_sget(object, attr));

    return vh_get(object, attr);
}

/* Get the root figure of an object */
vhash *
svg_get_figure(vhash *object)
{
    vhash *parent;

    while ((parent = vh_pget(object, "PARENT")) != NULL)
        object = parent;

    return object;
}

/* Return the height of a character in a given fontsize */
float
svg_get_height(vhash *figure, float fontsize)
{
    return fontsize * SVG_TEXT_HSCALE;
}

/* Return the width of a character in a given fontsize */
float
svg_get_width(vhash *figure, float fontsize)
{
    return fontsize * SVG_TEXT_WSCALE;
}
