/*
 * dir.c - directory listing, entry metadata, path utilities
 *
 * Handles: scanning a directory, building Item arrays, sorting entries,
 * status-bar string formatting, and the changedir / diropen logic.
 */

#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "fm.h"
#include "util.h"
#include "widget.h"
#include "icons.h"
#include "thumb.h"
#include "dir.h"

/* Hidden-file filter; toggled with ^period at runtime */
int hide = 1;

/* ── Size units for status bar ──────────────────────────────── */
static struct {
	char u;
	long long int n;
} units[UNIT_LAST] = {
	{ 'B', 1LL },
	{ 'K', 1024LL },
	{ 'M', 1024LL * 1024 },
	{ 'G', 1024LL * 1024 * 1024 },
	{ 'T', 1024LL * 1024 * 1024 * 1024 },
	{ 'P', 1024LL * 1024 * 1024 * 1024 * 1024 },
	{ 'E', 1024LL * 1024 * 1024 * 1024 * 1024 * 1024 },
};

/* ── Helpers ────────────────────────────────────────────────── */

int
wchdir(const char *path)
{
	if (chdir(path) == -1) {
		warn("%s", path);
		return RETURN_FAILURE;
	}
	return RETURN_SUCCESS;
}

static int
direntselect(const struct dirent *dp)
{
	if (strcmp(dp->d_name, ".") == 0)
		return false;
	if (strcmp(dp->d_name, "..") == 0)
		return true;
	if (hide && dp->d_name[0] == '.')
		return false;
	return true;
}

void
freeentries(struct FM *fm)
{
	int i;
	for (i = 0; i < fm->nentries; i++) {
		free(fm->entries[i].name);
		free(fm->entries[i].fullname);
		free(fm->entries[i].status);
	}
}

char *
fullpath(char *dir, char *file)
{
	char buf[PATH_MAX];
	if (strcmp(dir, "/") == 0)
		dir = "";
	(void)snprintf(buf, sizeof(buf), "%s/%s", dir, file);
	return estrdup(buf);
}

/* ── Status bar string ──────────────────────────────────────── */

char *
statusfmt(struct stat *sb)
{
	int i;
	time_t time;
	long long int number, fract;
	struct passwd *pw = NULL;
	struct group  *gr = NULL;
	struct tm tm;
	char *user  = "?";
	char *group = "";
	char *sep   = "";
	char timebuf[128];
	char buf[STATUS_BUFSIZE] = "???";

	number = 0;
	if (sb->st_size <= 0)
		goto done;
	for (i = 0; i < UNIT_LAST; i++)
		if (sb->st_size < units[i + 1].n)
			break;
	if (i == UNIT_LAST)
		goto done;
	fract  = (i == 0) ? 0 : sb->st_size % units[i].n;
	fract /= (i == 0) ? 1 : units[i - 1].n;
	fract  = (10 * fract + 512) / 1024;
	number = sb->st_size / units[i].n;
	if (number <= 0)
		goto done;
	if (fract >= 10 || (fract >= 5 && number >= 100)) {
		number++;
		fract = 0;
	} else if (fract < 0) {
		fract = 0;
	}
done:
	pw = getpwuid(sb->st_uid);
	gr = getgrgid(sb->st_gid);
	if (gr != NULL && gr->gr_name != NULL)
		group = gr->gr_name;
	if (pw != NULL && pw->pw_name != NULL) {
		user = pw->pw_name;
		if (strcmp(user, group) == 0) {
			group = "";
		} else {
			sep = ":";
		}
	}
	time = sb->st_mtim.tv_sec;
	(void)localtime_r(&time, &tm);
	(void)strftime(timebuf, sizeof(timebuf), "%F %R", &tm);
	if (number <= 0) {
		(void)snprintf(buf, sizeof(buf),
		    "0B - %s%s%s - %s", user, sep, group, timebuf);
	} else if (number >= 100) {
		(void)snprintf(buf, sizeof(buf),
		    "%lld%c - %s%s%s - %s",
		    number, units[i].u, user, sep, group, timebuf);
	} else {
		(void)snprintf(buf, sizeof(buf),
		    "%lld.%lld%c - %s%s%s - %s",
		    number, fract, units[i].u, user, sep, group, timebuf);
	}
	return estrdup(buf);
}

/* ── File mode ──────────────────────────────────────────────── */

int
isdir(Item *entry)
{
	return (entry->mode & MODE_MASK) == MODE_DIR;
}

unsigned char
filemode(struct FM *fm, struct stat *sb, char *name)
{
	bool ismember;
	struct stat lsb;
	size_t perm_off;
	unsigned char mask, type;
	int i;

	mask = 0x00;
	if (S_ISLNK(sb->st_mode)) {
		mask |= MODE_LINK;
		if (stat(name, &lsb) == -1) {
			type = MODE_BROK;
			goto done;
		}
		sb = &lsb;
	}
	switch (sb->st_mode & S_IFMT) {
	case S_IFBLK:  type = MODE_DEV;  break;
	case S_IFCHR:  type = MODE_DEV;  break;
	case S_IFDIR:  type = MODE_DIR;  break;
	case S_IFIFO:  type = MODE_FIFO; break;
	case S_IFSOCK: type = MODE_SOCK; break;
	default:       type = MODE_FILE; break;
	}
done:
	ismember = false;
	for (i = 0; i < fm->ngrps; i++) {
		if (fm->grps[i] == sb->st_gid) {
			ismember = true;
			break;
		}
	}
	if (sb->st_uid == fm->uid)
		perm_off = 0;
	else if (sb->st_gid == fm->gid || ismember)
		perm_off = 3;
	else
		perm_off = 6;
	if (sb->st_mode & (S_IRUSR >> perm_off)) mask |= MODE_READ;
	if (sb->st_mode & (S_IWUSR >> perm_off)) mask |= MODE_WRITE;
	if (sb->st_mode & (S_IXUSR >> perm_off)) mask |= MODE_EXEC;
	return type | mask;
}

/* ── Icon selection ─────────────────────────────────────────── */

static bool
checkicon(struct FM *fm, Item *entry, struct IconPatt *icon)
{
	int flags;
	char *s, *t;

	t = icon->patt;
	if (t == NULL)
		return false;
	if (t[0] == '~' || strchr(t, '/') != NULL) {
		flags = FNM_CASEFOLD | FNM_PATHNAME;
		s = entry->fullname;
	} else {
		flags = FNM_CASEFOLD;
		s = entry->name;
	}
	if (s == NULL)
		return false;
	if (t[0] == '~') {
		if (strncmp(fm->home, s, fm->homelen) != 0)
			return false;
		t++;
		s += fm->homelen;
	}
	return s != NULL &&
	    ((icon->mode & MODE_MASK) == MODE_ANY ||
	    (icon->mode & MODE_MASK) == (entry->mode & MODE_MASK)) &&
	    (!(icon->mode & MODE_LINK) || (entry->mode & MODE_LINK)) &&
	    (!(icon->mode & MODE_EXEC) || (entry->mode & MODE_EXEC)) &&
	    (!(icon->mode & MODE_READ) || (entry->mode & MODE_READ)) &&
	    (!(icon->mode & MODE_WRITE) || (entry->mode & MODE_WRITE)) &&
	    fnmatch(t, s, flags) == 0;
}

size_t
geticon_for_entry(struct FM *fm, Item *entry)
{
	size_t i;

	if (strcmp(entry->name, "..") == 0)
		return icon_for_updir;
	for (i = 0; i < fm->nuserpatts; i++)
		if (checkicon(fm, entry, &fm->userpatts[i]))
			return fm->userpatts[i].index;
	for (i = 0; i < nicon_patts; i++)
		if (checkicon(fm, entry, &icon_patts[i]))
			return icon_patts[i].index;
	if (isdir(entry))
		return icon_for_dir;
	return icon_for_file;
}

/* ── Sorting ────────────────────────────────────────────────── */

static int
entrycmp(const void *ap, const void *bp)
{
	int aisdir, bisdir;
	Item *a, *b;

	a = (Item *)ap;
	b = (Item *)bp;
	if (strcmp(a->name, "..") == 0) return -1;
	if (strcmp(b->name, "..") == 0) return  1;
	aisdir = isdir(a);
	bisdir = isdir(b);
	if (aisdir && !bisdir) return -1;
	if (bisdir && !aisdir) return  1;
	if (a->name[0] == '.' && b->name[0] != '.') return -1;
	if (b->name[0] == '.' && a->name[0] != '.') return  1;
	return strcoll(a->name, b->name);
}

/* ── User icon patterns (from Xresources fileIcons) ─────────── */

void
inituserpatts(struct FM *fm)
{
	size_t i, j, npatts;
	char *s, *t, *icon, *patt, *buf;
	unsigned char type, mask;

	fm->nuserpatts = 0;
	fm->userpatts  = NULL;
	buf = NULL;
	if ((buf = widget_geticons(fm->widget)) == NULL)
		return;
	npatts = 1;
	for (s = buf; *s != '\0'; s++)
		if (*s == ':' || *s == '\n')
			npatts++;
	fm->userpatts = calloc(npatts, sizeof(*fm->userpatts));
	if (fm->userpatts == NULL) {
		warn("could not set file icons");
		goto error;
	}
	i = 0;
	for (s = strtok(buf, ":\n"); s != NULL; s = strtok(NULL, ":\n")) {
		while (*s == ' ' || *s == '\t')
			s++;
		type = MODE_FILE;
		mask = 0x00;
		icon = strchr(s, '=');
		patt = s;
		if (icon == NULL) continue;
		if (icon == patt)  continue;
		for (t = icon - 1; t > patt; t--) {
			switch (*t) {
			case '-': type = MODE_ANY;  break;
			case '/': type = MODE_DIR;  break;
			case '|': type = MODE_FIFO; break;
			case '=': type = MODE_SOCK; break;
			case '#': type = MODE_DEV;  break;
			case '^': type = MODE_BROK; break;
			case '@': mask |= MODE_LINK; break;
			case '!': mask |= MODE_EXEC; break;
			default: goto loopout;
			}
			*t = '\0';
		}
loopout:
		*icon = '\0';
		icon++;
		icon[strcspn(icon, " \t")] = '\0';
		for (j = 0; j < nicon_types; j++) {
			if (strcmp(icon, icon_types[j].name) != 0)
				continue;
			fm->userpatts[i].index = j;
			fm->userpatts[i].patt  = strdup(patt);
			fm->userpatts[i].mode  = type | mask;
			if (fm->userpatts[i].patt == NULL)
				warn("strdup");
			else
				i++;
			break;
		}
	}
	fm->nuserpatts = i;
	free(buf);
	return;
error:
	free(fm->userpatts);
	free(buf);
	fm->nuserpatts = 0;
	fm->userpatts  = NULL;
}

/* ── Directory open / change ────────────────────────────────── */

int
diropen(struct FM *fm, struct Cwd *cwd, const char *path)
{
	struct dirent **array;
	struct stat sb;
	int i;
	char buf[PATH_MAX];

	if (path != NULL && wchdir(path) == RETURN_FAILURE)
		return RETURN_FAILURE;
	free(cwd->path);
	free(cwd->here);
	egetcwd(buf, sizeof(buf));
	if (stat(buf, &sb) == -1)
		err(EXIT_FAILURE, "stat");
	cwd->path = estrdup(buf);
	fm->time  = sb.st_ctim;
	freeentries(fm);
	fm->nentries = escandir(cwd->path, &array, direntselect, NULL);
	if (fm->nentries > fm->capacity) {
		fm->selitems = erealloc(fm->selitems, fm->nentries * sizeof(*fm->selitems));
		fm->entries  = erealloc(fm->entries,  fm->nentries * sizeof(*fm->entries));
		fm->capacity = fm->nentries;
	}
	for (i = 0; i < fm->nentries; i++) {
		if (lstat(array[i]->d_name, &sb) == -1) {
			warn("%s", array[i]->d_name);
			fm->entries[i].status = NULL;
			fm->entries[i].mode   = 0;
		} else {
			fm->entries[i].status = statusfmt(&sb);
			fm->entries[i].mode   = filemode(fm, &sb, array[i]->d_name);
		}
		fm->entries[i].name     = estrdup(array[i]->d_name);
		fm->entries[i].fullname = fullpath(cwd->path, array[i]->d_name);
		fm->entries[i].icon     = geticon_for_entry(fm, &fm->entries[i]);
		free(array[i]);
	}
	free(array);
	qsort(fm->entries, fm->nentries, sizeof(*fm->entries), entrycmp);
	if (strstr(cwd->path, fm->home) == cwd->path &&
	    (cwd->path[fm->homelen] == '/' || cwd->path[fm->homelen] == '\0')) {
		snprintf(buf, PATH_MAX, "~%s", cwd->path + fm->homelen);
	} else {
		snprintf(buf, PATH_MAX, "%s", cwd->path);
	}
	cwd->here = estrdup(buf);
	return RETURN_SUCCESS;
}

/* ── History / changedir ────────────────────────────────────── */

static int
timespeclt(struct timespec *tsp, struct timespec *usp)
{
	return (tsp->tv_sec == usp->tv_sec)
	     ? (tsp->tv_nsec < usp->tv_nsec)
	     : (tsp->tv_sec  < usp->tv_sec);
}

void
clearcwd(struct Cwd *cwd)
{
	struct Cwd *tmp;
	while (cwd != NULL) {
		tmp = cwd;
		cwd = cwd->next;
		free(tmp->path);
		free(tmp->here);
		free(tmp);
	}
}

static void
newcwd(struct FM *fm)
{
	struct Cwd *cwd;

	clearcwd(fm->cwd->next);
	cwd = emalloc(sizeof(*cwd));
	*cwd = (struct Cwd){
		.next = NULL,
		.prev = fm->cwd,
		.path = NULL,
		.here = NULL,
	};
	fm->cwd->next = cwd;
	fm->cwd       = cwd;
}

int
changedir(struct FM *fm, const char *path, int force_refresh)
{
	Scroll *scrl;
	struct stat sb;
	int keepscroll, retval;
	struct Cwd cwd = { .prev = NULL, .next = NULL, .path = NULL, .here = NULL };

	if (!force_refresh && fm->last != NULL && path == fm->last->path) {
		if (stat(path, &sb) == -1)
			return RETURN_FAILURE;
		if (!timespeclt(&fm->time, &sb.st_ctim))
			return RETURN_SUCCESS;
	}
	widget_busy(fm->widget);
	retval = RETURN_SUCCESS;
	closethumbthread(fm);
	if (diropen(fm, &cwd, path) == RETURN_FAILURE)
		goto done;
	if (fm->cwd->path != NULL && strcmp(cwd.path, fm->cwd->path) == 0) {
		keepscroll = true;
	} else if (fm->cwd->prev != NULL && fm->cwd->prev->path != NULL &&
	           strcmp(cwd.path, fm->cwd->prev->path) == 0) {
		fm->cwd   = fm->cwd->prev;
		keepscroll = true;
	} else {
		newcwd(fm);
		keepscroll = false;
	}
	free(fm->cwd->path);
	free(fm->cwd->here);
	fm->cwd->path = cwd.path;
	fm->cwd->here = cwd.here;
	fm->last       = fm->cwd;
	scrl = keepscroll ? &fm->cwd->scrl : NULL;
	retval = widget_set(fm->widget, fm->cwd->path, fm->cwd->here,
	                    fm->entries, fm->nentries, scrl);
done:
	createthumbthread(fm);
	return retval;
}
