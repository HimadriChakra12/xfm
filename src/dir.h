/* dir.h - directory listing and navigation */

#ifndef DIR_H
#define DIR_H

#include <sys/stat.h>
#include "fm.h"

/* Global hidden-file toggle (1 = hide dotfiles, 0 = show) */
extern int hide;

int         wchdir(const char *path);
void        freeentries(struct FM *fm);
char       *fullpath(char *dir, char *file);
char       *statusfmt(struct stat *sb);
int         isdir(Item *entry);
unsigned char filemode(struct FM *fm, struct stat *sb, char *name);
size_t      geticon_for_entry(struct FM *fm, Item *entry);
void        inituserpatts(struct FM *fm);
int         diropen(struct FM *fm, struct Cwd *cwd, const char *path);
void        clearcwd(struct Cwd *cwd);
int         changedir(struct FM *fm, const char *path, int force_refresh);

#endif /* DIR_H */
