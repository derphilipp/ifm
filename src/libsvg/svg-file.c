/* SVG file functions */

#include <stdarg.h>

#include "svg-attr.h"
#include "svg-file.h"
#include "svg-object.h"
#include "svg-util.h"

#define SCALE 100

#define HPOS(obj, key) (vh_dget(obj, key) * SCALE)
#define LPOS(obj, idx) (vl_dget(obj, idx) * SCALE)

/* Output stream */
static FILE *stream = NULL;

/* Output handler */
static void (*handler)(char *line);

/* Internal functions */
static void svg_write_object(vhash *object);
static void svg_write_arrows(vhash *object);
static void svg_write_tag(vhash *object, char *tag);
static void svg_write(char *fmt, ...);

/* Set output stream */
void
svg_set_stream(FILE *fp)
{
    stream = fp;
}

/* Set output handler function */
void
svg_set_handler(void (*func)(char *line))
{
    handler = func;
}

/* Write figure to a stream or handler */
void
svg_write_figure(vhash *figure)
{
    float width = vh_dget(figure, "WIDTH");
    float height = vh_dget(figure, "HEIGHT");
    vlist *objects;
    viter iter;

    /* Write header info */
    svg_write("<?xml version=\"1.0\"?>\n");
    svg_write("<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n");
    svg_write(" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n\n");

    svg_write("<svg width=\"%gcm\" height=\"%gcm\" "
              "viewBox=\"0 0 %g %g\">\n", width, height,
              width * SCALE, height * SCALE);

    /* Write objects */
    if ((objects = vh_pget(figure, "OBJECTS")) != NULL)
        v_iterate(objects, iter)
            svg_write_object(vl_iter_pval(iter));

    /* Finish up */
    svg_write("\n</svg>\n");
}

/* Write an object to a stream */
static void
svg_write_object(vhash *object)
{
    int npoints, type, pen_style = 0, cap_style = 0;
    int i, font_flags = 4, direction = 0;
    vlist *objects, *xp, *yp;
    char *style;
    viter iter;

    type = vh_iget(object, "TYPE");

    xp = vh_pget(object, "XP");
    yp = vh_pget(object, "YP");
    npoints = (xp != NULL ? vl_length(xp) : 0);

    svg_write("\n");
    if (vh_exists(object, "NAME"))
        svg_write("<!-- %s -->\n", vh_sgetref(object, "NAME"));

    switch (type) {

    case SVG_BOX:
        svg_debug("BOX");

        svg_write("<rect x=\"%g\" y=\"%g\" width=\"%g\" height=\"%g\"",
                  HPOS(object, "X"), HPOS(object, "Y"),
                  HPOS(object, "WIDTH"), HPOS(object, "HEIGHT"));

        if ((style = svg_get_style(object)) != NULL)
            svg_write(" style=\"%s\"", style);

        svg_write(" />\n");
        break;

    case SVG_POLYLINE:
        svg_debug("POLYLINE");

#if 0
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
        svg_write("\t");
        for (i = 0; i < npoints; i++)
            svg_write(" %d %d", vl_iget(xp, i), vl_iget(yp, i));
        svg_write("\n");
#endif
        if (npoints == 2) {
            svg_write("<line x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\"",
                      LPOS(xp, 0), LPOS(yp, 0),
                      LPOS(xp, 1), LPOS(yp, 1));
        } else {
            svg_write("<polyline points=\"");

            for (i = 0; i < npoints; i++) {
                svg_write("%g %g", LPOS(xp, i), LPOS(yp, i));
                if (i < npoints - 1)
                    svg_write(", ");
            }

            svg_write("\"");
        }

        if ((style = svg_get_style(object)) != NULL)
            svg_write(" style=\"%s\"", style);

        svg_write(" />\n");
        break;

    case SVG_SPLINE:
        svg_debug("SPLINE");

#if 0
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
#endif
        /* FINISH ME -- this needs to be a bezier path */

        svg_write("<polyline points=\"");

        for (i = 0; i < npoints; i++) {
            svg_write("%g %g", LPOS(xp, i), LPOS(yp, i));
            if (i < npoints - 1)
                svg_write(", ");
        }

        svg_write("\"");

        if ((style = svg_get_style(object)) != NULL)
            svg_write(" style=\"%s\"", style);

        svg_write(" />\n");
        break;

    case SVG_TEXT:
        svg_debug("TEXT");

#if 0
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
#endif
        svg_write("<text x=\"%g\" y=\"%g\"",
                  HPOS(object, "X"), HPOS(object, "Y"));

        if ((style = svg_get_style(object)) != NULL)
            svg_write(" style=\"%s\"", style);

        svg_write(">%s</text>\n", svg_get_sval(object, "TEXT"));
        break;

    case SVG_COMPOUND:
        svg_debug("GROUP");

        svg_write("<g>");

        if ((style = svg_get_style(object)) != NULL)
            svg_write(" style=\"%s\"", style);

        if ((objects = vh_pget(object, "OBJECTS")) != NULL)
            v_iterate(objects, iter)
                svg_write_object(vl_iter_pval(iter));

        svg_write("</g>\n");
        break;
    }
}

/* Write object tag (and optional style) to a stream */
static void
svg_write_tag(vhash *object, char *tag)
{
    char *style;

    svg_write("<%s", tag);

    if ((style = svg_get_style(object)) != NULL)
        svg_write(" style=\"%s\"", style);

    svg_write(">\n");
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

#if 0
    if (arrow_forward)
        svg_write("\t%d %d %g %g %g\n",
                  arrow_type, arrow_style, thickness, width, height);

    if (arrow_backward)
        svg_write("\t%d %d %g %g %g\n",
                  arrow_type, arrow_style, thickness, width, height);
#endif
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
