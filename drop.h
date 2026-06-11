/* drop.h - drag-and-drop and context-menu subprocess handling */

#ifndef DROP_H
#define DROP_H

#include "fm.h"
#include "widget.h"

WidgetEvent runxfilesctl(struct FM *fm, char **argv, char *path);
WidgetEvent runcontext(struct FM *fm, char *cmd, int nselitems);
WidgetEvent runindrop(struct FM *fm, WidgetEvent event, int nitems);
WidgetEvent runexdrop(struct FM *fm, WidgetEvent event, char *text, char *path);

#endif /* DROP_H */
