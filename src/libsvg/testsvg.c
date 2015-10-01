/* Svglib test */

#include "svg.h"

#define WIDTH 3.0
#define HEIGHT 2.0

#define TXT "Hello there!"
#define FNT 23.0

#define TEXT \
        "Hello there.  This is a very long line to test out the word " \
        "wrapping gimmick of the textbox function.  Neat huh?  Let's " \
        "try another sentence to make sure it works properly."

int
main(void)
{
    vhash *figure, *box, *group, *text, *line, *spline;
    float width, height;

    v_debug(V_DBG_PTRS);

    figure = svg_create(SVG_INCHES, 1.0);
    svg_set_orientation(figure, SVG_PORTRAIT);

    text = svg_create_textbox(figure, "Helvet", 20, SVG_JUSTIFY_CENTRE,
                              1.0, 1.0, WIDTH, HEIGHT, TEXT);
    svg_set_name(text, "text");
    svg_set_colour(text, 1.0, 0.0, 0.0);
    box = svg_create_box(figure, 1.0, 1.0, WIDTH, HEIGHT);
    svg_set_name(box, "box around text");
    svg_set_depth(box, 200);

#if 0
    group = svg_create_compound(figure);
    text = svg_create_text(group, 6.0, 6.0, "Hi there");
    box = svg_create_box(group, 3.0, 3.0, 2.0, 2.0);
    line = svg_create_line(figure, 1.0, 1.0, 7.0, 3.0);
    spline = svg_create_spline(figure, SVG_OPEN_INTERP);

    svg_create_point(spline, 1.0, 1.0);
    svg_create_point(spline, 1.0, 2.0);
    svg_create_point(spline, 2.0, 2.0);
    svg_create_point(spline, 2.0, 3.0);

    svg_set_colour(spline, 0.0, 0.0, 1.0);
    svg_set_font(text, "Helvet", 20.0);
    svg_set_fillcolour(box, 1.0, 1.0, 0.0);
    svg_set_colour(box, 0.0, 1.0, 0.0);
    svg_set_colour(text, 1.0, 0.0, 0.0);
    svg_set_name(group, "My group");
    svg_set_name(box, "My box");
    svg_set_arrow(line, 1, 0);
    svg_set_arrow(spline, 1, 0);
#endif

    svg_write_figure(figure, stdout);

    return 0;
}
