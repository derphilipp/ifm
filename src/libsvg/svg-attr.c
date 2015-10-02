/* SVG attribute functions */

#include <stdarg.h>
#include "svg-attr.h"
#include "svg-object.h"
#include "svg-util.h"

/* Font list */
static char *fontlist[] = {
    "Times-Roman",
    "Times-Italic",
    "Times-Bold",
    "Times-BoldItalic",
    "AvantGarde-Book",
    "AvantGarde-BookOblique",
    "AvantGarde-Demi",
    "AvantGarde-DemiOblique",
    "Bookman-Light",
    "Bookman-LightItalic",
    "Bookman-Demi",
    "Bookman-DemiItalic",
    "Courier",
    "Courier-Oblique",
    "Courier-Bold",
    "Courier-BoldOblique",
    "Helvetica",
    "Helvetica-Oblique",
    "Helvetica-Bold",
    "Helvetica-BoldOblique",
    "Helvetica-Narrow",
    "Helvetica-Narrow-Oblique",
    "Helvetica-Narrow-Bold",
    "Helvetica-Narrow-Bold-Oblique",
    "New-Century-Schoolbook-Roman",
    "New-Century-Schoolbook-Italic",
    "New-Century-Schoolbook-Bold",
    "New-Century-Schoolbook-BoldItalic",
    "Palatino-Roman",
    "Palatino-Italic",
    "Palatino-Bold",
    "Palatino-BoldItalic",
    "Symbol",
    "Zapf-Chancery-MediumItalic",
    "Zapf-Dingbats",
    NULL
};

/* Internal functions */
static char *svg_colour(float r, float g, float b);
static int svg_lookup_font(char *name);

/* Return SVG representation of a colour */
static char *
svg_colour(float r, float g, float b)
{
    V_BUF_DECL;

    int ri = (int) (r * 255);
    int gi = (int) (g * 255);
    int bi = (int) (b * 255);

    return V_BUF_SETF("rgb(%d, %d, %d)", ri, gi, bi);
}

/* Look up a font ID given its name */
static int
svg_lookup_font(char *name)
{
    int i, len;

    len = strlen(name);
    for (i = 0; fontlist[i] != NULL; i++)
        if (strncmp(name, fontlist[i], len) == 0)
            return i;

    return -1;
}

/* Set the arrow flags of an object */
void
svg_set_arrow(vhash *object, int forward, int backward)
{
    vh_istore(object, "ARROW_FORWARD", forward);
    vh_istore(object, "ARROW_BACKWARD", backward);
}

/* Set the arrow style of an object */
void
svg_set_arrowstyle(vhash *object, int type, int style,
                   float thick, float width, float height)
{
    vh_istore(object, "ARROW_TYPE", type);
    vh_istore(object, "ARROW_STYLE", style);
    vh_fstore(object, "ARROW_THICK", thick);
    vh_fstore(object, "ARROW_WIDTH", width * 15);
    vh_fstore(object, "ARROW_HEIGHT", height * 15);
}

/* Set the colour of an object */
void
svg_set_colour(vhash *object, float r, float g, float b)
{
    char *colour = svg_colour(r, g, b);
    vh_sstore(object, "PENCOLOUR", colour);
}

/* Set the depth of an object */
void
svg_set_depth(vhash *object, int depth)
{
    vh_istore(object, "DEPTH", depth);
}

/* Set the fill colour of an object */
void
svg_set_fillcolour(vhash *object, float r, float g, float b)
{
    char *colour = svg_colour(r, g, b);
    vh_sstore(object, "FILLCOLOUR", colour);
    vh_istore(object, "FILLSTYLE", 20);
}

/* Set the fill style of an object */
void
svg_set_fillstyle(vhash *object, int style)
{
    vh_istore(object, "FILLSTYLE", style);
}

/* Set the font of an object */
void
svg_set_font(vhash *object, char *font, float fontsize)
{
    vh_istore(object, "FONT", svg_lookup_font(font));
    vh_fstore(object, "FONTSIZE", fontsize);
}

/* Set the join style of an object */
void
svg_set_joinstyle(vhash *object, int style)
{
    vh_istore(object, "JOINSTYLE", style);
}

/* Set the justification of an object */
void
svg_set_justify(vhash *object, int justify)
{
    vh_istore(object, "JUSTIFY", justify);
}

/* Set the line style of an object */
void
svg_set_linestyle(vhash *object, int style)
{
    vh_istore(object, "LINESTYLE", style);
}

/* Set the line width of an object */
void
svg_set_linewidth(vhash *object, int width)
{
    vh_istore(object, "LINEWIDTH", width);
}

/* Set the name of an object */
void
svg_set_name(vhash *object, char *fmt, ...)
{
    char *str;
    V_ALLOCA_FMT(str, fmt);
    vh_sstore(object, "NAME", str);
}

/* Set the spline shape of a point */
void
svg_set_shape(vhash *object, int num, float shape)
{
    vlist *list = vh_add_list(object, "SHAPE");
    vl_fstore(list, num, shape);
}
