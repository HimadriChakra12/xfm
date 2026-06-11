/* thumb.h - thumbnail thread management */

#ifndef THUMB_H
#define THUMB_H

#include <limits.h>
#include "fm.h"

int  setthumbpath(struct FM *fm, char *orig, char *thumb);
void closethumbthread(struct FM *fm);
void createthumbthread(struct FM *fm);
void initthumbnailer(struct FM *fm);

#endif /* THUMB_H */
