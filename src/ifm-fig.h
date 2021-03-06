/* Fig output format header */

#ifndef IFM_FIG_H
#define IFM_FIG_H

#include "ifm-format.h"

extern mapfuncs fig_mapfuncs;

extern void fig_map_start(void);
extern void fig_map_section(vhash *sect);
extern void fig_map_room(vhash *room);
extern void fig_map_link(vhash *link);
extern void fig_map_endsection(void);
extern void fig_map_finish(void);

#endif
