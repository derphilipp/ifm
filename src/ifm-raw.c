/* Raw output format */

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
#include "ifm-raw.h"
#include "ifm-vars.h"

/* Internal functions */
static void raw_init(void);

/* Output function list */
outputfuncs raw_outputfuncs = {
    raw_init,
    NULL
};

/* Map function list */
mapfuncs raw_mapfuncs = {
    raw_map_start,
    raw_map_section,
    raw_map_room,
    raw_map_link,
    raw_map_join,
    NULL,
    NULL
};

/* Item function list */
itemfuncs raw_itemfuncs = {
    NULL,
    raw_item_entry,
    NULL
};

/* Task function list */
taskfuncs raw_taskfuncs = {
    NULL,
    raw_task_entry,
    NULL
};

static int count = 0;

/* Map functions */
void
raw_map_start(void)
{
    char *title;

    if (vh_exists(map, "TITLE"))
        title = vh_sgetref(map, "TITLE");
    else
        title = "Interactive Fiction map";

    output("title: %s\n", title);
}

void
raw_map_section(vhash *sect)
{
    char buf[100];

    count++;
    if (!vh_exists(sect, "TITLE")) {
        sprintf(buf, "Map section %d", count);
        vh_sstore(sect, "TITLE", buf);
    }

    output("\nsection: %s\n", vh_sgetref(sect, "TITLE"));
    output("width: %d\n", vh_iget(sect, "XLEN"));
    output("height: %d\n", vh_iget(sect, "YLEN"));
}

void
raw_map_room(vhash *room)
{
    vlist *ex, *ey;

    output("\nroom: %d\n", vh_iget(room, "ID"));
    output("name: %s\n", vh_sgetref(room, "DESC"));
    output("rpos: %d %d\n", vh_iget(room, "X"), vh_iget(room, "Y"));

    ex = vh_pget(room, "EX");
    ey = vh_pget(room, "EY");

    if (ex != NULL && ey != NULL)
        while (vl_length(ex) > 0 && vl_length(ey) > 0)
            output("exit: %d %d\n", vl_ishift(ex), vl_ishift(ey));
}

void
raw_map_link(vhash *link)
{
    vlist *x, *y, *cmds;
    vhash *from, *to;
    viter iter;
    int go;

    from = vh_pget(link, "FROM");
    to = vh_pget(link, "TO");
    output("\nlink: %d %d\n", vh_iget(from, "ID"), vh_iget(to, "ID"));

    x = vh_pget(link, "X");
    y = vh_pget(link, "Y");
    while (vl_length(x) > 0 && vl_length(y) > 0)
        output("lpos: %d %d\n", vl_ishift(x), vl_ishift(y));

    if (vh_iget(link, "ONEWAY"))
        output("oneway: 1\n");

    go = vh_iget(link, "GO");
    if (go != D_NONE)
        output("go: %s\n", dirinfo[go].sname);

    if ((cmds = vh_pget(link, "CMD")) != NULL) {
        v_iterate(cmds, iter)
            output("cmd: %s\n", vl_iter_svalref(iter));
    }
}

void
raw_map_join(vhash *join)
{
    vhash *from, *to;
    vlist *cmds;
    viter iter;
    int go;

    from = vh_pget(join, "FROM");
    to = vh_pget(join, "TO");
    output("\njoin: %d %d\n", vh_iget(from, "ID"), vh_iget(to, "ID"));

    if (vh_iget(join, "ONEWAY"))
        output("oneway: 1\n");

    go = vh_iget(join, "GO");
    if (go != D_NONE)
        output("go: %s\n", dirinfo[go].sname);

    if ((cmds = vh_pget(join, "CMD")) != NULL) {
        v_iterate(cmds, iter)
            output("cmd: %s\n", vl_iter_svalref(iter));
    }
}

/* Item functions */
void
raw_item_entry(vhash *item)
{
    vlist *notes = vh_pget(item, "NOTE");
    vhash *room = vh_pget(item, "ROOM");
    int score = vh_iget(item, "SCORE");
    vhash *task, *reach;
    vlist *list;
    viter iter;

    output("\nitem: %d\n", vh_iget(item, "ID"));
    output("name: %s\n", vh_sgetref(item, "DESC"));

    if (vh_exists(item, "TAG"))
        output("tag: %s\n", vh_sgetref(item, "TAG"));

    if (room != NULL)
        output("room: %d\n", vh_iget(room, "ID"));

    if (score > 0)
        output("score: %d\n", score);

    if (vh_exists(item, "LEAVE"))
        output("leave: 1\n");

    if (vh_iget(item, "HIDDEN"))
        output("hidden: 1\n");

    if (vh_iget(item, "FINISH"))
        output("finish: 1\n");

    if ((list = vh_pget(item, "RTASKS")) != NULL) {
        v_iterate(list, iter) {
            task = vl_iter_pval(iter);
            output("after: %d\n", vh_iget(task, "ID"));
        }
    }

    if ((list = vh_pget(item, "TASKS")) != NULL) {
        v_iterate(list, iter) {
            task = vl_iter_pval(iter);
            output("needed: %d\n", vh_iget(task, "ID"));
        }
    }

    if ((list = vh_pget(item, "NROOMS")) != NULL) {
        v_iterate(list, iter) {
            room = vl_iter_pval(iter);
            output("enter: %d\n", vh_iget(room, "ID"));
        }
    }

    if ((list = vh_pget(item, "NLINKS")) != NULL) {
        v_iterate(list, iter) {
            reach = vl_iter_pval(iter);
            room = vh_pget(reach, "FROM");
            output("move: %d", vh_iget(room, "ID"));
            room = vh_pget(reach, "TO");
            output(" %d\n", vh_iget(room, "ID"));
        }
    }

    if (notes != NULL) {
        v_iterate(notes, iter)
            output("note: %s\n", vl_iter_svalref(iter));
    }
}

/* Task functions */
void
raw_task_entry(vhash *task)
{
    vlist *notes = vh_pget(task, "NOTE");
    vhash *room = vh_pget(task, "ROOM");
    vlist *cmds = vh_pget(task, "CMD");
    int score = vh_iget(task, "SCORE");
    viter iter;

    output("\ntask: %d\n", vh_iget(task, "ID"));
    output("name: %s\n", vh_sgetref(task, "DESC"));

    if (vh_exists(task, "TAG"))
        output("tag: %s\n", vh_sgetref(task, "TAG"));

    if (room != NULL)
        output("room: %d\n", vh_iget(room, "ID"));

    if (cmds != NULL) {
        v_iterate(cmds, iter)
            output("cmd: %s\n", vl_iter_svalref(iter));
    }

    if (score > 0)
        output("score: %d\n", score);

    if (notes != NULL) {
        v_iterate(notes, iter)
            output("note: %s\n", vl_iter_svalref(iter));
    }
}

/* Initialise raw output */
static void
raw_init(void)
{
    count = 0;
}
