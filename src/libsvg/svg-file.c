/* SVG file functions */

#include <stdarg.h>

#include "svg-attr.h"
#include "svg-file.h"
#include "svg-object.h"
#include "svg-util.h"

/* Output stream */
static FILE *stream = NULL;

/* Output handler */
static void (*handler)(char *line);

/* Internal functions */
static void svg_write_object(vhash *object);
static void svg_write_arrows(vhash *object);
static void svg_write(char *fmt, ...);

void
svg_set_stream(FILE *fp)
{
    stream = fp;
}

void
svg_set_handler(void (*func)(char *line))
{
    handler = func;
}

/* Write figure to a stream or handler */
void
svg_write_figure(vhash *figure)
{
    int orient = svg_get_ival(figure, "ORIENTATION");
    vlist *objects;
    vhash *colours;
    viter iter;

    /* Initialize */
    svg_init_colour();

    /* Calculate bounding box */
    svg_calc_bbox(figure);

    /* Write header info */
    svg_write("#SVG 3.2\n");
    svg_write("%s\n", orient == SVG_LANDSCAPE ? "Landscape" : "Portrait");
    svg_write("Center\n");
    svg_write("%s\n", svg_get_sval(figure, "UNITS"));
    svg_write("%s\n", svg_get_sval(figure, "PAPERSIZE"));
    svg_write("100.00\n");
    svg_write("Single\n");
    svg_write("-2\n");
    svg_write("%d 2\n", SVG_RESOLUTION);

    /* Write colour table entries (if any) */
    if ((colours = vh_pget(figure, "COLOURS")) != NULL)
        v_iterate(colours, iter)
            svg_write("%d %d %s\n", SVG_COLOUR,
                      vh_iter_ival(iter),
                      vh_iter_key(iter));

    /* Write objects */
    if ((objects = vh_pget(figure, "OBJECTS")) != NULL)
        v_iterate(objects, iter)
            svg_write_object(vl_iter_pval(iter));
}

/* Write an object to a stream */
static void
svg_write_object(vhash *object)
{
    int radius = 1, npoints, type, subtype, pen_style = 0, cap_style = 0;
    int i, font_flags = 4, direction = 0;
    vlist *objects, *xp, *yp, *shape;
    float style_val = 1.0;
    viter iter;

    type = vh_iget(object, "TYPE");
    subtype = vh_iget(object, "SUBTYPE");

    xp = vh_pget(object, "XP");
    yp = vh_pget(object, "YP");
    shape = vh_pget(object, "SHAPE");
    npoints = (xp != NULL ? vl_length(xp) : 0);

    if (vh_exists(object, "NAME"))
        svg_write("# %s\n", vh_sgetref(object, "NAME"));

    switch (type) {

    case SVG_ELLIPSE:
        /* FINISH ME */
        break;

    case SVG_POLYLINE:
        svg_write("%d ", type);
        svg_write("%d ", subtype);
        svg_write("%d ", svg_get_ival(object, "LINESTYLE"));
        svg_write("%d ", svg_get_ival(object, "LINEWIDTH"));
        svg_write("%d ", svg_get_ival(object, "PENCOLOUR"));
        svg_write("%d ", svg_get_ival(object, "FILLCOLOUR"));
        svg_write("%d ", svg_get_ival(object, "DEPTH"));
        svg_write("%d ", pen_style);
        svg_write("%d ", svg_get_ival(object, "FILLSTYLE"));
        svg_write("%.3f ", style_val);
        svg_write("%d ", svg_get_ival(object, "JOINSTYLE"));
        svg_write("%d ", cap_style);
        svg_write("%d ", radius);
        svg_write("%d ", svg_get_ival(object, "ARROW_FORWARD"));
        svg_write("%d ", svg_get_ival(object, "ARROW_BACKWARD"));
        svg_write("%d\n", npoints);

        if (subtype == SVG_PICTURE)
            svg_write("\t0 %s\n", svg_get_sval(object, "PICFILE"));
        else
            svg_write_arrows(object);

        svg_write("\t");
        for (i = 0; i < npoints; i++)
            svg_write(" %d %d", vl_iget(xp, i), vl_iget(yp, i));
        svg_write("\n");

        break;

    case SVG_SPLINE:
        svg_write("%d ", type);
        svg_write("%d ", subtype);
        svg_write("%d ", svg_get_ival(object, "LINESTYLE"));
        svg_write("%d ", svg_get_ival(object, "LINEWIDTH"));
        svg_write("%d ", svg_get_ival(object, "PENCOLOUR"));
        svg_write("%d ", svg_get_ival(object, "FILLCOLOUR"));
        svg_write("%d ", svg_get_ival(object, "DEPTH"));
        svg_write("%d ", pen_style);
        svg_write("%d ", svg_get_ival(object, "FILLSTYLE"));
        svg_write("%.3f ", style_val);
        svg_write("%d ", cap_style);
        svg_write("%d ", svg_get_ival(object, "ARROW_FORWARD"));
        svg_write("%d ", svg_get_ival(object, "ARROW_BACKWARD"));
        svg_write("%d\n", npoints);

        svg_write_arrows(object);

        svg_write("\t");
        for (i = 0; i < npoints; i++)
            svg_write(" %d %d", vl_iget(xp, i), vl_iget(yp, i));
        svg_write("\n");

        svg_write("\t");
        for (i = 0; i < npoints; i++)
            svg_write(" %g", vl_dget(shape, i));
        svg_write("\n");

        break;

    case SVG_TEXT:
        svg_write("%d ", type);
        svg_write("%d ", svg_get_ival(object, "JUSTIFY"));
        svg_write("%d ", svg_get_ival(object, "PENCOLOUR"));
        svg_write("%d ", svg_get_ival(object, "DEPTH"));
        svg_write("%d ", pen_style);
        svg_write("%d ", svg_get_ival(object, "FONT"));
        svg_write("%g ", svg_get_fval(object, "FONTSIZE"));
        svg_write("%g ", svg_get_fval(object, "ANGLE"));
        svg_write("%d ", font_flags);
        svg_write("%g ", svg_get_fval(object, "HEIGHT"));
        svg_write("%g ", svg_get_fval(object, "WIDTH"));
        svg_write("%d ", svg_get_ival(object, "X"));
        svg_write("%d ", svg_get_ival(object, "Y"));
        svg_write("%s\\001\n", svg_get_sval(object, "TEXT"));
        break;

    case SVG_ARC:
        svg_write("%d ", type);
        svg_write("%d ", subtype);
        svg_write("%d ", svg_get_ival(object, "LINESTYLE"));
        svg_write("%d ", svg_get_ival(object, "LINEWIDTH"));
        svg_write("%d ", svg_get_ival(object, "PENCOLOUR"));
        svg_write("%d ", svg_get_ival(object, "FILLCOLOUR"));
        svg_write("%d ", svg_get_ival(object, "DEPTH"));
        svg_write("%d ", pen_style);
        svg_write("%d ", svg_get_ival(object, "FILLSTYLE"));
        svg_write("%.3f ", style_val);
        svg_write("%d ", cap_style);
        svg_write("%d ", direction);
        svg_write("%d ", svg_get_ival(object, "ARROW_FORWARD"));
        svg_write("%d ", svg_get_ival(object, "ARROW_BACKWARD"));

        for (i = 0; i < npoints; i++)
            svg_write(" %g %g", vl_fget(xp, i), vl_fget(yp, i));

        svg_write("\n");
        svg_write_arrows(object);
        break;

    case SVG_COMPOUND:
        svg_write("%d ", type);
        svg_write("%d ", svg_get_ival(object, "XMAX"));
        svg_write("%d ", svg_get_ival(object, "YMAX"));
        svg_write("%d ", svg_get_ival(object, "XMIN"));
        svg_write("%d\n", svg_get_ival(object, "YMIN"));

        if ((objects = vh_pget(object, "OBJECTS")) != NULL)
            v_iterate(objects, iter)
                svg_write_object(vl_iter_pval(iter));

        svg_write("-6\n");
        break;
    }
}

/* Write object arrows to a stream */
static void
svg_write_arrows(vhash *object)
{
    int arrow_forward = svg_get_ival(object, "ARROW_FORWARD");
    int arrow_backward = svg_get_ival(object, "ARROW_BACKWARD");
    int arrow_type = svg_get_ival(object, "ARROW_TYPE");
    int arrow_style = svg_get_ival(object, "ARROW_STYLE");
    float thickness = svg_get_fval(object, "ARROW_THICK");
    float width = svg_get_fval(object, "ARROW_WIDTH");
    float height = svg_get_fval(object, "ARROW_HEIGHT");

    if (arrow_forward)
        svg_write("\t%d %d %g %g %g\n",
                  arrow_type, arrow_style, thickness, width, height);

    if (arrow_backward)
        svg_write("\t%d %d %g %g %g\n",
                  arrow_type, arrow_style, thickness, width, height);
}

static void
svg_write(char *fmt, ...)
{
    FILE *fp = stream ? stream : stdout;
    static char line[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);

    if (handler != NULL) {
        vsnprintf(line, BUFSIZ - 1, fmt, ap);
        (*handler)(line);
    } else {
        vfprintf(fp, fmt, ap);
    }

    va_end(ap);
}       
