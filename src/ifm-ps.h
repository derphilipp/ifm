/* Postscript output format header */

#ifndef IFM_PS_H
#define IFM_PS_H

#include "ifm-format.h"

extern mapfuncs ps_mapfuncs;

extern void ps_map_start(void);
extern void ps_map_section(vhash *sect);
extern void ps_map_room(vhash *room);
extern void ps_map_link(vhash *link);
extern void ps_map_endsection(void);
extern void ps_map_finish(void);

#endif
