/* YAML output format */

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
#include "ifm-util.h"
#include "ifm-vars.h"
#include "ifm-yaml.h"

#define YAML_ATTR(object, attr) yaml_string(vh_sgetref(object, attr))
#define YAML_VAL(iter) yaml_string(vl_iter_svalref(iter))

/* Internal functions */
static char *yaml_string(char *string);
static void yaml_init(void);

/* Output function list */
outputfuncs yaml_outputfuncs = {
    yaml_init,
    NULL
};

/* Map function list */
mapfuncs yaml_mapfuncs = {
    yaml_map_start,
    yaml_map_section,
    yaml_map_room,
    yaml_map_link,
    yaml_map_join,
    NULL,
    NULL
};

/* Item function list */
itemfuncs yaml_itemfuncs = {
    NULL,
    yaml_item_entry,
    NULL
};

/* Task function list */
taskfuncs yaml_taskfuncs = {
    NULL,
    yaml_task_entry,
    NULL
};

/* Current counts of things */
static int sectnum = 0;
static int roomnum = 0;
static int linknum = 0;
static int itemnum = 0;
static int joinnum = 0;
static int tasknum = 0;

/* Map functions */
void
yaml_map_start(void)
{
    char *title = vh_sgetref(map, "TITLE");
    output("\ntitle: %s\n", yaml_string(title));
}

void
yaml_map_section(vhash *sect)
{
    if (sectnum++ == 0)
        output("\nsections:\n");

    roomnum = linknum = 0;

    output("- name: %s\n", YAML_ATTR(sect, "TITLE"));
    output("  size: [%d, %d]\n",
           vh_iget(sect, "XLEN"),
           vh_iget(sect, "YLEN"));
}

void
yaml_map_room(vhash *room)
{
    vlist *ex, *ey;

    if (roomnum++ == 0)
        output("\n  rooms:\n");

    output("  - name: %s\n", YAML_ATTR(room, "DESC"));
    output("    id: %d\n", vh_iget(room, "ID"));
    output("    pos: [%d, %d]\n", vh_iget(room, "X"), vh_iget(room, "Y"));

    ex = vh_pget(room, "EX");
    ey = vh_pget(room, "EY");

    if (ex != NULL && ey != NULL) {
        output("    exit:\n");
        while (vl_length(ex) > 0 && vl_length(ey) > 0)
            output("    - [%d, %d]\n", vl_ishift(ex), vl_ishift(ey));
    }
}

void
yaml_map_link(vhash *link)
{
    vlist *x, *y, *cmds;
    vhash *from, *to;
    viter iter;
    int go;

    if (linknum++ == 0)
        output("\n  links:\n");

    from = vh_pget(link, "FROM");
    to = vh_pget(link, "TO");
    output("  - id: [%d, %d]\n", vh_iget(from, "ID"), vh_iget(to, "ID"));

    x = vh_pget(link, "X");
    y = vh_pget(link, "Y");
    output("    pos:\n");
    while (vl_length(x) > 0 && vl_length(y) > 0)
        output("    - [%d, %d]\n", vl_ishift(x), vl_ishift(y));

    if (vh_iget(link, "ONEWAY"))
        output("    oneway: true\n");

    go = vh_iget(link, "GO");
    if (go != D_NONE)
        output("    go: %s\n", dirinfo[go].sname);

    if ((cmds = vh_pget(link, "CMD")) != NULL) {
        output("    cmd:\n");
        v_iterate(cmds, iter)
            output("    - %s\n", YAML_VAL(iter));
    }
}

void
yaml_map_join(vhash *join)
{
    vhash *from, *to;
    vlist *cmds;
    viter iter;
    int go;

    if (joinnum++ == 0)
        output("\n  joins:\n");

    from = vh_pget(join, "FROM");
    to = vh_pget(join, "TO");
    output("  - id: [%d, %d]\n", vh_iget(from, "ID"), vh_iget(to, "ID"));

    if (vh_iget(join, "ONEWAY"))
        output("    oneway: true\n");

    go = vh_iget(join, "GO");
    if (go != D_NONE)
        output("    go: %s\n", dirinfo[go].sname);

    if ((cmds = vh_pget(join, "CMD")) != NULL) {
        output("    cmd:\n");
        v_iterate(cmds, iter)
            output("    - %s\n", YAML_VAL(iter));
    }
}

/* Item functions */
void
yaml_item_entry(vhash *item)
{
    vlist *notes = vh_pget(item, "NOTE");
    vhash *room = vh_pget(item, "ROOM");
    int score = vh_iget(item, "SCORE");
    vhash *task, *reach;
    vlist *list;
    viter iter;

    if (itemnum++ == 0)
        output("\nitems:\n");

    output("- name: %s\n", YAML_ATTR(item, "DESC"));
    output("  id: %d\n", vh_iget(item, "ID"));

    if (vh_exists(item, "TAG"))
        output("  tag: %s\n", YAML_ATTR(item, "TAG"));

    if (room != NULL)
        output("  room: %d\n", vh_iget(room, "ID"));

    if (score > 0)
        output("  score: %d\n", score);

    if (vh_exists(item, "LEAVE"))
        output("  leave: true\n");

    if (vh_iget(item, "HIDDEN"))
        output("  hidden: true\n");

    if (vh_iget(item, "FINISH"))
        output("  finish: true\n");

    if ((list = vh_pget(item, "RTASKS")) != NULL) {
        output("  after: [");

        v_iterate(list, iter) {
            task = vl_iter_pval(iter);
            if (v_iter_count(iter) > 0)
                output(", ");
            output("%d", vh_iget(task, "ID"));
        }

        output("]\n");
    }

    if ((list = vh_pget(item, "TASKS")) != NULL) {
        output("  needed: [");

        v_iterate(list, iter) {
            task = vl_iter_pval(iter);
            if (v_iter_count(iter) > 0)
                output(", ");
            output("%d", vh_iget(task, "ID"));
        }

        output("]\n");
    }

    if ((list = vh_pget(item, "NROOMS")) != NULL) {
        output("  enter: [");

        v_iterate(list, iter) {
            room = vl_iter_pval(iter);
            if (v_iter_count(iter) > 0)
                output(", ");
            output("%d", vh_iget(room, "ID"));
        }

        output("]\n");
    }

    if ((list = vh_pget(item, "NLINKS")) != NULL) {
        output("  move:\n");

        v_iterate(list, iter) {
            reach = vl_iter_pval(iter);
            room = vh_pget(reach, "FROM");
            output("  - [%d, ", vh_iget(room, "ID"));
            room = vh_pget(reach, "TO");
            output(" %d]\n", vh_iget(room, "ID"));
        }
    }

    if (notes != NULL) {
        output("  note:\n");

        v_iterate(notes, iter)
            output("  - %s\n", YAML_VAL(iter));
    }
}

/* Task functions */
void
yaml_task_entry(vhash *task)
{
    vlist *notes = vh_pget(task, "NOTE");
    vhash *room = vh_pget(task, "ROOM");
    vlist *cmds = vh_pget(task, "CMD");
    int score = vh_iget(task, "SCORE");
    viter iter;

    if (tasknum++ == 0)
        output("\ntasks:\n");

    output("- name: %s\n", YAML_ATTR(task, "DESC"));
    output("  id: %d\n", vh_iget(task, "ID"));

    if (vh_exists(task, "TAG"))
        output("  tag: %s\n", YAML_ATTR(task, "TAG"));

    if (room != NULL)
        output("  room: %d\n", vh_iget(room, "ID"));

    if (cmds != NULL) {
        output("  cmd:\n");
        v_iterate(cmds, iter)
            output("  - %s\n", YAML_VAL(iter));
    }

    if (score > 0)
        output("  score: %d\n", score);

    if (notes != NULL) {
        output("  note:\n");

        v_iterate(notes, iter)
            output("  - %s\n", YAML_VAL(iter));
    }
}

/* Initialise YAML output */
static void
yaml_init(void)
{
    output("%YAML 1.1\n");
    output("---\n");

    sectnum = 0;
    roomnum = 0;
    linknum = 0;
    itemnum = 0;
    joinnum = 0;
    tasknum = 0;
}

/* Convert a string for YAML output */
static char *
yaml_string(char *string)
{
    V_BUF_DECL;

    if (strchr(string, '"'))
        return V_BUF_SETF("'%s'", string);

    return V_BUF_SETF("\"%s\"", string);
}
