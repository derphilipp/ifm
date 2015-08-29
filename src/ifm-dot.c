/* Graphviz output format */

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

#include "ifm-format.h"
#include "ifm-main.h"
#include "ifm-map.h"
#include "ifm-task.h"
#include "ifm-util.h"
#include "ifm-vars.h"
#include "ifm-dot.h"

/* Task function list */
taskfuncs dot_taskfuncs = {
    dot_task_start,
    NULL,
    dot_task_finish,
};

/* Control variables */
static int show_rooms = 0;
static int show_orphans = 0;
static int word_wrap = 0;
static char *graph_attr = "";
static char *node_attr = "";
static char *link_attr = "";

/* Internal functions */
static void print_label(char *string);

/* Task functions */
void
dot_task_start(void)
{
    double width, height;
    char *title, *font;

    /* Get control variables */
    set_map_vars();

    show_rooms = var_int("task_graph_rooms");
    show_orphans = var_int("task_graph_orphans");
    graph_attr = var_string("task_graph_attr");
    node_attr = var_string("task_graph_node");
    link_attr = var_string("task_graph_link");
    word_wrap = var_int("task_graph_wrap");
    font = var_string("task_graph_font");

    /* Get title */
    title = vh_sgetref(map, "TITLE");

    /* Get page size */
    width = (page_width - page_margin) / 2.54;
    height = (page_height - page_margin) / 2.54;

    /* Write graph header */
    output("digraph \"%s\" {\n", title);

    output("    fontname = \"%s\";\n", font);

    output("    graph [size = \"%g,%g\", ratio = fill, %s];\n",
           height, width, graph_attr);

    output("    node [fontname = \"%s\"];\n", font);
    output("    edge [fontname = \"%s\", len = 1.5];\n", font);

    output("    rankdir = LR;\n");
    output("    rotate = 90;\n");
    output("    concentrate = true;\n");
}

void
dot_task_finish(void)
{
    vlist *list, *tolist, *nodes, *lines;
    vhash *step, *room, *rooms;
    char *node, *name;
    int cluster = 0;
    viter i, j, k;
    V_BUF_DECL;
    vgraph *g;

    /* Build directed graph */
    g = task_graph();

    /* Group tasks by room */
    rooms = vh_create();
    nodes = vg_node_list(g);

    v_iterate(nodes, i) {
        node = vl_iter_svalref(i);
        if (!show_orphans && vg_node_links(g, node, NULL, NULL) == 0)
            continue;

        step = vg_node_pget(g, node);
        if (vh_iget(step, "IGNORE"))
            continue;

        room = vh_pget(step, "ROOM");
        name = room ? vh_sgetref(room, "DESC") : "";

        list = vh_add_list(rooms, name);
        vl_spush(list, node);
    }

    vl_destroy(nodes);

    /* Write nodes */
    output("    node [%s];\n", node_attr);

    v_iterate(rooms, i) {
        name = vh_iter_key(i);
        list = vh_iter_pval(i);

        if (show_rooms) {
            if (strlen(name) > 0) {
                output("    subgraph cluster_%d {\n", ++cluster);
                output("        ");
                print_label(name);
                output(";\n");
            } else {
                output("    subgraph anywhere {\n");
            }
        }

        v_iterate(list, j) {
            node = vl_iter_svalref(j);
            step = vg_node_pget(g, node);
            output("    ");

            if (show_rooms)
                output("    ");

            output("%s [", node);

            V_BUF_SET(vh_sgetref(step, "DESC"));
            if (!show_rooms && strlen(name) > 0)
                V_BUF_ADDF(" [%s]", name);

            if (word_wrap > 0) {
                lines = vl_filltext(V_BUF_VAL, word_wrap);
                V_BUF_SET(vl_join(lines, "\\n"));
            }

            print_label(V_BUF_VAL);
            output("];\n");
        }

        if (show_rooms)
            output("    }\n");
    }

    /* Write links */
    output("    edge [%s];\n", link_attr);

    v_iterate(rooms, i) {
        list = vh_iter_pval(i);

        v_iterate(list, j) {
            node = vl_iter_svalref(j);
            tolist = vg_node_to(g, node);
            v_iterate(tolist, k)
                output("    %s -> %s;\n", node, vl_iter_svalref(k));
        }
    }

    /* Write graph footer */
    output("}\n");

    /* Clean up */
    v_destroy(rooms);
    vg_destroy(g);
}

/* Print a label, escaping doublequotes */
static void
print_label(char *string)
{
    output("label = \"");

    while (*string != '\0') {
        if (*string == '"')
            output("\\");
        output("%c", *string);
        string++;
    }

    output("\"");
}
