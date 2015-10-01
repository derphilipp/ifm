/* Svg output format */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <vars.h>
#include <svg.h>

#include "ifm-format.h"
#include "ifm-svg.h"
#include "ifm-main.h"
#include "ifm-map.h"
#include "ifm-util.h"
#include "ifm-vars.h"

#define SVG_TEXT_DEPTH   100
#define SVG_ROOM_DEPTH   200
#define SVG_LINK_DEPTH   300
#define SVG_SHADOW_DEPTH 400
#define SVG_TITLE_DEPTH  500
#define SVG_MAP_DEPTH    600
#define SVG_PAGE_DEPTH   700

#define MAPX(x) (svg_origin_x + room_size * (svg_xoff + (x)))
#define MAPY(y) (svg_height - (svg_origin_y + room_size * (svg_yoff + (y))))

/* Figure objects */
static vhash *svg = NULL;
static vhash *svg_section = NULL;
static vhash *svg_room = NULL;

/* Page dimensions */
static float svg_width, svg_height;
static float svg_origin_x, svg_origin_y;

/* Room offsets */
static float svg_xoff = 0.0, svg_yoff = 0.0;

/* Map function list */
mapfuncs svg_mapfuncs = {
    svg_map_start,
    svg_map_section,
    svg_map_room,
    svg_map_link,
    NULL,
    NULL,
    svg_map_finish
};

/* Internal functions */
static void set_colour(vhash *object, char *colour);
static void set_fillcolour(vhash *object, char *colour);
static void output_handler(char *line);

/* Map functions */
void
svg_map_start(void)
{
    float ratio, tmp, scale = 1.0, xscale, yscale;
    int ylen, width, height, orient;
    vhash *sect, *box;
    viter iter;

    /* Initialize */
    if (svg != NULL)
        v_destroy(svg);

    /* Allow title space for sections with titles */
    v_iterate(sects, iter) {
        sect = vl_iter_pval(iter);
        if (show_map_title && vh_exists(sect, "TITLE")) {
            ylen = vh_iget(sect, "YLEN");
            vh_istore(sect, "YLEN", ylen + 1);
        }
    }

    /* Set room names */
    setup_room_names();

    /* Determine orientation */
    if (VAR_DEF("page_rotate"))
        orient = var_int("page_rotate") ? SVG_LANDSCAPE : SVG_PORTRAIT;
    else
        orient = SVG_UNDEF;

    /* Get initial dimensions, in rooms */
    svg_width = page_width - 2 * page_margin;
    svg_height = page_height - 2 * page_margin;

    width = (int) (svg_width / room_size) + 1;
    height = (int) (svg_height / room_size) + 1;
    ratio = page_height / page_width;

    /* Increase dimensions until sections fit on one page */
    while (1) {
        svg_debug("trying map size: %d x %d", width, height);

        if (orient != SVG_LANDSCAPE &&
            pack_sections(width, height) == 1) {
            orient = SVG_PORTRAIT;
            svg_debug("using portrait");
            break;
        }

        if (orient != SVG_PORTRAIT &&
            pack_sections(height, width) == 1) {
            orient = SVG_LANDSCAPE;
            tmp = page_width;
            page_width = page_height;
            page_height = tmp;
            svg_debug("using landscape");
            break;
        }

        width++;
        height = (int) (width * ratio) + 1;
    }

    /* Find map size from first section that's output */
    v_iterate(sects, iter) {
        sect = vl_iter_pval(iter);
        if (!vh_iget(sect, "NOPRINT")) {
            width = vh_iget(sect, "PXLEN");
            height = vh_iget(sect, "PYLEN");
            break;
        }
    }

    /* Set dimensions and offsets */
    if (orient == SVG_PORTRAIT) {
        svg_width = width * room_size;
        svg_height = height * room_size;
    } else {
        svg_width = height * room_size;
        svg_height = width * room_size;
    }

    svg_width += 2 * page_margin;
    svg_height += 2 * page_margin;
    svg_width = V_MAX(svg_width, page_width - 2 * page_margin);
    svg_height = V_MAX(svg_height, page_height - 2 * page_margin);

    svg_debug("map size: %d x %d", width, height);
    svg_debug("room size: %g", room_size);
    svg_debug("page size: %g x %g", page_width, page_height);
    svg_debug("svg size: %g x %g", svg_width, svg_height);

    svg_origin_x = (page_width - width * room_size) / 2;
    svg_origin_y = (page_height - height * room_size) / 2;
    svg_origin_x = V_MAX(svg_origin_x, page_margin);
    svg_origin_y = V_MAX(svg_origin_y, page_margin);

    svg_debug("page margin: %g", page_margin);
    svg_debug("svg origin: %g x %g", svg_origin_x, svg_origin_y);

    if (var_int("fit_page")) {
        xscale = (page_width - 2 * page_margin) / svg_width;
        yscale = (page_height - 2 * page_margin) / svg_height;
        if (xscale < 1.0 || yscale < 1.0) {
            scale = V_MIN(xscale, yscale);
            svg_debug("scaling by %g", scale);
        }
    }

    /* Initialise figure */
    svg = svg_create(SVG_METRIC, scale);
    svg_set_orientation(svg, orient);
    svg_set_papersize(svg, page_size);

    /* Draw page border if required */
    if (show_page_border) {
        box = svg_create_box(svg,
                             page_margin, page_margin,
                             svg_width, svg_height);

        set_colour(box, page_border_colour);
        set_fillcolour(box, page_background_colour);
        svg_set_depth(box, SVG_PAGE_DEPTH);
    }
}

void
svg_map_section(vhash *sect)
{
    float x, y, width, height;
    vhash *text, *box;

    /* Create section object */
    svg_section = svg_create_compound(svg);

    /* Set section offsets */
    svg_xoff = vh_dget(sect, "XOFF");
    svg_yoff = vh_dget(sect, "YOFF");
    svg_debug("offsets: %g, %g", svg_xoff, svg_yoff);

    /* Draw map border if required */
    if (show_map_border) {
        x = 0.0;
        y = vh_dget(sect, "YLEN") - room_height;
        width = vh_dget(sect, "XLEN") * room_size;
        height = vh_dget(sect, "YLEN") * room_size;
        box = svg_create_box(svg_section, MAPX(x), MAPY(y), width, height);
        set_colour(box, map_border_colour);
        set_fillcolour(box, map_background_colour);
        svg_set_depth(box, SVG_MAP_DEPTH);
    }

    /* Print title if required */
    if (show_map_title && vh_exists(sect, "TITLE")) {
        x = 0.0;
        y = vh_dget(sect, "YLEN") - 0.7;
        width = vh_dget(sect, "XLEN") * room_size;
        height = room_size;
        text = svg_create_textbox(svg_section, map_title_font,
                                  map_title_fontsize,
                                  SVG_JUSTIFY_CENTRE,
                                  MAPX(x), MAPY(y),
                                  width, height,
                                  "%s", vh_sgetref(sect, "TITLE"));
        svg_set_depth(text, SVG_TITLE_DEPTH);
        set_colour(text, map_title_colour);
    }
}

void
svg_map_room(vhash *room)
{
    float xp, yp, width, height, xborder = 0.02, yborder = 0.02;
    static vlist *px = NULL, *py = NULL;
    vhash *line, *box, *text;
    vlist *items, *ex, *ey;
    char *itemlist = NULL;
    int x, y;

    /* Create room object */
    svg_room = svg_create_compound(svg_section);

    /* Get coords */
    x = vh_iget(room, "X");
    y = vh_iget(room, "Y");

    /* Draw room box */
    xp = x + (1 - room_width) / 2;
    yp = y + (1 - room_height) / 2;
    width = room_size * room_width;
    height = room_size * room_height;

    svg_debug("room at %d %d, offset %g %g, size %g %g",
              x, y, xp, yp, width, height);

    box = svg_create_box(svg_room, MAPX(xp), MAPY(yp), width, height);
    svg_set_depth(box, SVG_ROOM_DEPTH);

    set_colour(box, room_border_colour);
    set_fillcolour(box, room_colour);
    svg_set_linewidth(box, (int) room_border_width);

    if (room_border_dashed)
        svg_set_linestyle(box, SVG_DASH);

    /* Draw room shadow if required */
    if (room_shadow_xoff != 0 || room_shadow_yoff != 0) {
        box = svg_create_box(svg_room,
                             MAPX(xp - room_shadow_xoff),
                             MAPY(yp - room_shadow_yoff),
                             width, height);
        svg_set_depth(box, SVG_SHADOW_DEPTH);
        set_colour(box, room_shadow_colour);
        set_fillcolour(box, room_shadow_colour);
    }

    /* Get item list (if any) */
    items = vh_pget(room, "ITEMS");
    if (items != NULL && vl_length(items) > 0) {
        vhash *item;
        vlist *list;
        viter iter;

        list = vl_create();
        v_iterate(items, iter) {
            item = vl_iter_pval(iter);
            if (!vh_iget(item, "HIDDEN"))
                vl_spush(list, vh_sgetref(item, "DESC"));
        }

        if (vl_length(list) > 0)
            itemlist = vl_join(list, ", ");

        vl_destroy(list);
    }

    /* Draw room text (and items if any) */
    if (itemlist == NULL) {
        text = svg_create_textbox(svg_room, room_text_font,
                                  room_text_fontsize,
                                  SVG_JUSTIFY_CENTRE,
                                  MAPX(xp + xborder),
                                  MAPY(yp - yborder),
                                  width * (1 - 2 * xborder),
                                  height * (1 - 2 * yborder),
                                  "%s", vh_sgetref(room, "RDESC"));

        svg_set_depth(text, SVG_TEXT_DEPTH);
        set_colour(text, room_text_colour);
    } else {
        text = svg_create_textbox(svg_room, room_text_font,
                                  room_text_fontsize,
                                  SVG_JUSTIFY_CENTRE,
                                  MAPX(xp + xborder),
                                  MAPY(yp - yborder),
                                  width * (1 - 2 * xborder),
                                  height * (1 - 2 * yborder) / 2,
                                  "%s", vh_sgetref(room, "RDESC"));

        svg_set_depth(text, SVG_TEXT_DEPTH);
        set_colour(text, room_text_colour);

        text = svg_create_textbox(svg_room, item_text_font,
                                  item_text_fontsize,
                                  SVG_JUSTIFY_CENTRE,
                                  MAPX(xp + xborder),
                                  MAPY(yp - yborder - 0.3),
                                  width * (1 - 2 * xborder),
                                  height * (1 - 2 * yborder) / 2,
                                  "%s", itemlist);

        svg_set_depth(text, SVG_TEXT_DEPTH);
        set_colour(text, item_text_colour);
    }

    /* Draw room exits (if any) */
    ex = vh_pget(room, "EX");
    ey = vh_pget(room, "EY");

    if (ex != NULL && ey != NULL) {
        double x1, y1, x2, y2, xoff = 0.5, yoff = 0.5 - room_height;

        vl_init(px);
        vl_init(py);

        while (vl_length(ex) > 0) {
            vl_istore(px, 0, x);
            vl_istore(py, 0, y);
            vl_istore(px, 1, x + vl_ishift(ex));
            vl_istore(py, 1, y + vl_ishift(ey));
            truncate_points(px, py, room_width, room_height);
            x1 = vl_dget(px, 0);
            y1 = vl_dget(py, 0);
            x2 = vl_dget(px, 1);
            y2 = vl_dget(py, 1);

            x2 = x1 + 0.35 * (x2 - x1);
            y2 = y1 + 0.35 * (y2 - y1);

            line = svg_create_line(svg_room,
                                   MAPX(x1 + xoff), MAPY(y1 + yoff),
                                   MAPX(x2 + xoff), MAPY(y2 + yoff));

            set_colour(line, room_exit_colour);
            svg_set_linewidth(line, (int) room_exit_width);
            svg_set_depth(line, SVG_LINK_DEPTH);
        }
    }
}

void
svg_map_link(vhash *link)
{
    int go = vh_iget(link, "GO");
    int updown = (go == D_UP || go == D_DOWN);
    int inout = (go == D_IN || go == D_OUT);

    vlist *x = vh_pget(link, "X");
    vlist *y = vh_pget(link, "Y");

    int i, np = vl_length(x);
    vhash *line, *text;
    double xp, yp;
    char *str;

    /* Draw link line */
    truncate_points(x, y, room_width, room_height);

    if (link_spline && np > 2)
        line = svg_create_spline(svg_section, SVG_OPEN_XSPLINE);
    else
        line = svg_create_polyline(svg_section, SVG_LINE);

    for (i = 0; i < np; i++) {
        xp = vl_dget(x, i) + 0.5;
        yp = vl_dget(y, i) + 0.5 - room_height;
        svg_create_point(line, MAPX(xp), MAPY(yp));
    }

    svg_set_linewidth(line, (int) link_line_width);
    set_colour(line, link_colour);

    if (link_dashed)
        svg_set_linestyle(line, SVG_DASH);

    svg_set_depth(line, SVG_LINK_DEPTH);
    svg_set_arrow(line, vh_iget(link, "ONEWAY"), 0);

    /* Add text if required */
    if (updown || inout) {
        xp = (vl_dget(x, 0) + vl_dget(x, 1)) / 2;
        yp = (vl_dget(y, 0) + vl_dget(y, 1)) / 2;

        xp += 0.5;
        yp += 0.5 - room_height;

        str = updown ? link_updown_string : link_inout_string;
        text = svg_create_text(svg_section, MAPX(xp), MAPY(yp), str);

        svg_set_font(text, link_text_font, link_text_fontsize);
        svg_set_depth(text, SVG_LINK_DEPTH);
        set_colour(text, link_text_colour);
        set_fillcolour(text, page_background_colour);
    }
}

void
svg_map_finish(void)
{
    svg_set_handler(output_handler);
    svg_write_figure(svg);
}

/* Set the colour of an object */
static void
set_colour(vhash *object, char *colour)
{
    float r, g, b;

    if (sscanf(colour, "%f %f %f", &r, &g, &b) == 3)
        svg_set_colour(object, r, g, b);
}

/* Set the fill colour of an object */
static void
set_fillcolour(vhash *object, char *colour)
{
    float r, g, b;

    if (sscanf(colour, "%f %f %f", &r, &g, &b) == 3)
        svg_set_fillcolour(object, r, g, b);
}

static void
output_handler(char *line)
{
    output(line);
}
