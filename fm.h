/* fm.h - internal file manager state types */

#ifndef FM_H
#define FM_H

#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <grp.h>
#include <limits.h>
#include <unistd.h>

#include "widget.h"
#include "icons.h"

/* Navigation history node */
struct Cwd {
	struct Cwd *prev, *next;
	Scroll      scrl;   /* scroll position when we last visited this dir */
	char       *path;   /* absolute path */
	char       *here;   /* display path (~/foo instead of /home/user/foo) */
};

/* Core file manager state */
struct FM {
	Widget  *widget;
	Item    *entries;
	int      widgetfd;      /* fd for widget events (used in poll loops) */
	int     *selitems;      /* array of selected entry indices */
	int      capacity;      /* allocated capacity of entries[] */
	int      nentries;      /* number of entries in entries[] */
	char    *home;
	size_t   homelen;

	struct Cwd *cwd;        /* current working directory node */
	struct Cwd *hist;       /* head of the history list */
	struct Cwd *last;       /* last successfully visited node */
	struct timespec time;   /* ctime of the current directory */

	/* user-defined icon glob patterns (from Xresources/fileIcons) */
	struct IconPatt *userpatts;
	size_t           nuserpatts;

	uid_t uid;
	gid_t gid;
	gid_t grps[NGROUPS_MAX];
	int   ngrps;

	pthread_mutex_t thumblock;
	pthread_t       thumbthread;
	int             thumbexit;
	char           *thumbnaildir;
	size_t          thumbnaildirlen;

	char *opener;
};

#endif /* FM_H */
