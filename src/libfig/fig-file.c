/* Fig file functions */

#include <stdarg.h>

#include "fig-attr.h"
#include "fig-file.h"
#include "fig-object.h"
#include "fig-util.h"

/* Output stream */
static FILE *stream = NULL;

/* Output handler */
static void (*handler)(char *line);

/* Internal functions */
static void fig_write_object(vhash *object);
static void fig_write_arrows(vhash *object);
static void fig_write(char *fmt, ...);

/* Set output stream */
void
fig_set_stream(FILE *fp)
{
    stream = fp;
}

/* Set output handler function */
void
fig_set_handler(void (*func)(char *line))
{
    handler = func;
}

/* Write figure to a stream or handler */
void
fig_write_figure(vhash *figure)
{
    int orient = fig_get_ival(figure, "ORIENTATION");
    vlist *objects;
    vhash *colours;
    viter iter;

    /* Initialize */
    fig_init_colour();

    /* Calculate bounding box */
    fig_calc_bbox(figure);

    /* Write header info */
    fig_write("#FIG 3.2\n");
    fig_write("%s\n", orient == FIG_LANDSCAPE ? "Landscape" : "Portrait");
    fig_write("Center\n");
    fig_write("%s\n", fig_get_sval(figure, "UNITS"));
    fig_write("%s\n", fig_get_sval(figure, "PAPERSIZE"));
    fig_write("100.00\n");
    fig_write("Single\n");
    fig_write("-2\n");
    fig_write("%d 2\n", FIG_RESOLUTION);

    /* Write colour table entries (if any) */
    if ((colours = vh_pget(figure, "COLOURS")) != NULL)
        v_iterate(colours, iter)
            fig_write("%d %d %s\n", FIG_COLOUR,
                      vh_iter_ival(iter),
                      vh_iter_key(iter));

    /* Write objects */
    if ((objects = vh_pget(figure, "OBJECTS")) != NULL)
        v_iterate(objects, iter)
            fig_write_object(vl_iter_pval(iter));
}

/* Write an object to a stream */
static void
fig_write_object(vhash *object)
{
    int radius = 1, npoints, type, subtype, pen_style = 0, cap_style = 0;
    vlist *objects, *xp, *yp, *shape;
    int i, font_flags = 4;
    float style_val = 1.0;
    viter iter;

    type = vh_iget(object, "TYPE");
    subtype = vh_iget(object, "SUBTYPE");

    xp = vh_pget(object, "XP");
    yp = vh_pget(object, "YP");
    shape = vh_pget(object, "SHAPE");
    npoints = (xp != NULL ? vl_length(xp) : 0);

    if (vh_exists(object, "NAME"))
        fig_write("# %s\n", vh_sgetref(object, "NAME"));

    switch (type) {

    case FIG_POLYLINE:
        fig_write("%d ", type);
        fig_write("%d ", subtype);
        fig_write("%d ", fig_get_ival(object, "LINESTYLE"));
        fig_write("%d ", fig_get_ival(object, "LINEWIDTH"));
        fig_write("%d ", fig_get_ival(object, "PENCOLOUR"));
        fig_write("%d ", fig_get_ival(object, "FILLCOLOUR"));
        fig_write("%d ", fig_get_ival(object, "DEPTH"));
        fig_write("%d ", pen_style);
        fig_write("%d ", fig_get_ival(object, "FILLSTYLE"));
        fig_write("%.3f ", style_val);
        fig_write("%d ", fig_get_ival(object, "JOINSTYLE"));
        fig_write("%d ", cap_style);
        fig_write("%d ", radius);
        fig_write("%d ", fig_get_ival(object, "ARROW_FORWARD"));
        fig_write("%d ", fig_get_ival(object, "ARROW_BACKWARD"));
        fig_write("%d\n", npoints);

        if (subtype == FIG_PICTURE)
            fig_write("\t0 %s\n", fig_get_sval(object, "PICFILE"));
        else
            fig_write_arrows(object);

        fig_write("\t");
        for (i = 0; i < npoints; i++)
            fig_write(" %d %d", vl_iget(xp, i), vl_iget(yp, i));
        fig_write("\n");

        break;

    case FIG_SPLINE:
        fig_write("%d ", type);
        fig_write("%d ", subtype);
        fig_write("%d ", fig_get_ival(object, "LINESTYLE"));
        fig_write("%d ", fig_get_ival(object, "LINEWIDTH"));
        fig_write("%d ", fig_get_ival(object, "PENCOLOUR"));
        fig_write("%d ", fig_get_ival(object, "FILLCOLOUR"));
        fig_write("%d ", fig_get_ival(object, "DEPTH"));
        fig_write("%d ", pen_style);
        fig_write("%d ", fig_get_ival(object, "FILLSTYLE"));
        fig_write("%.3f ", style_val);
        fig_write("%d ", cap_style);
        fig_write("%d ", fig_get_ival(object, "ARROW_FORWARD"));
        fig_write("%d ", fig_get_ival(object, "ARROW_BACKWARD"));
        fig_write("%d\n", npoints);

        fig_write_arrows(object);

        fig_write("\t");
        for (i = 0; i < npoints; i++)
            fig_write(" %d %d", vl_iget(xp, i), vl_iget(yp, i));
        fig_write("\n");

        fig_write("\t");
        for (i = 0; i < npoints; i++)
            fig_write(" %g", vl_dget(shape, i));
        fig_write("\n");

        break;

    case FIG_TEXT:
        fig_write("%d ", type);
        fig_write("%d ", fig_get_ival(object, "JUSTIFY"));
        fig_write("%d ", fig_get_ival(object, "PENCOLOUR"));
        fig_write("%d ", fig_get_ival(object, "DEPTH"));
        fig_write("%d ", pen_style);
        fig_write("%d ", fig_get_ival(object, "FONT"));
        fig_write("%g ", fig_get_fval(object, "FONTSIZE"));
        fig_write("%g ", fig_get_fval(object, "ANGLE"));
        fig_write("%d ", font_flags);
        fig_write("%g ", fig_get_fval(object, "HEIGHT"));
        fig_write("%g ", fig_get_fval(object, "WIDTH"));
        fig_write("%d ", fig_get_ival(object, "X"));
        fig_write("%d ", fig_get_ival(object, "Y"));
        fig_write("%s\\001\n", fig_get_sval(object, "TEXT"));
        break;

    case FIG_COMPOUND:
        fig_write("%d ", type);
        fig_write("%d ", fig_get_ival(object, "XMAX"));
        fig_write("%d ", fig_get_ival(object, "YMAX"));
        fig_write("%d ", fig_get_ival(object, "XMIN"));
        fig_write("%d\n", fig_get_ival(object, "YMIN"));

        if ((objects = vh_pget(object, "OBJECTS")) != NULL)
            v_iterate(objects, iter)
                fig_write_object(vl_iter_pval(iter));

        fig_write("-6\n");
        break;
    }
}

/* Write object arrows to a stream */
static void
fig_write_arrows(vhash *object)
{
    int arrow_forward = fig_get_ival(object, "ARROW_FORWARD");
    int arrow_backward = fig_get_ival(object, "ARROW_BACKWARD");
    int arrow_type = fig_get_ival(object, "ARROW_TYPE");
    int arrow_style = fig_get_ival(object, "ARROW_STYLE");
    float thickness = fig_get_fval(object, "ARROW_THICK");
    float width = fig_get_fval(object, "ARROW_WIDTH");
    float height = fig_get_fval(object, "ARROW_HEIGHT");

    if (arrow_forward)
        fig_write("\t%d %d %g %g %g\n",
                  arrow_type, arrow_style, thickness, width, height);

    if (arrow_backward)
        fig_write("\t%d %d %g %g %g\n",
                  arrow_type, arrow_style, thickness, width, height);
}

static void
fig_write(char *fmt, ...)
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
