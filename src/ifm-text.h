/* Text output format header */

#ifndef IFM_TEXT_H
#define IFM_TEXT_H

#include "ifm-format.h"

extern outputfuncs text_outputfuncs;
extern mapfuncs text_mapfuncs;
extern itemfuncs text_itemfuncs;
extern taskfuncs text_taskfuncs;

extern void text_map_start(void);
extern void text_map_section(vhash *sect);
extern void text_map_room(vhash *room);

extern void text_item_entry(vhash *item);

extern void text_task_entry(vhash *task);
extern void text_task_finish(void);

#endif
