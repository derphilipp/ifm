/* YAML output driver header */

#ifndef IFM_YAML_H
#define IFM_YAML_H

#include "ifm-driver.h"

extern outputfuncs yaml_outputfuncs;
extern mapfuncs yaml_mapfuncs;
extern itemfuncs yaml_itemfuncs;
extern taskfuncs yaml_taskfuncs;

extern void yaml_map_start(void);
extern void yaml_map_section(vhash *sect);
extern void yaml_map_room(vhash *room);
extern void yaml_map_link(vhash *link);
extern void yaml_map_join(vhash *join);

extern void yaml_item_entry(vhash *item);

extern void yaml_task_entry(vhash *task);

#endif
