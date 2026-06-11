/*
 * thumb.c - thumbnail generation thread
 *
 * Spawns a background thread that calls THUMBNAILERCMD for each entry
 * in the current directory, then notifies the widget to refresh icons.
 */

#include <sys/stat.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "config.h"
#include "fm.h"
#include "util.h"
#include "widget.h"
#include "thumb.h"

/* ── Path helpers ───────────────────────────────────────────── */

int
setthumbpath(struct FM *fm, char *orig, char *thumb)
{
	char buf[PATH_MAX];
	int i;

	if (realpath(orig, buf) == NULL)
		return RETURN_FAILURE;
	for (i = 0; buf[i] != '\0'; i++)
		if (buf[i] == '/')
			buf[i] = '%';
	snprintf(thumb, PATH_MAX, "%s/%s.ppm", fm->thumbnaildir, buf);
	return RETURN_SUCCESS;
}

/* ── Exit-flag helpers (called from main thread) ────────────── */

static int
thumbexit_check(struct FM *fm)
{
	int ret = 0;
	etlock(&fm->thumblock);
	if (fm->thumbexit)
		ret = 1;
	etunlock(&fm->thumblock);
	return ret;
}

/* ── Fork the external thumbnailer for one file ─────────────── */

static pid_t
forkthumb(char *orig, char *thumb)
{
	pid_t pid;

	if ((pid = efork()) == 0) {
		eclose(STDOUT_FILENO);
		eclose(STDIN_FILENO);
		eexec((char *[]){ THUMBNAILERCMD, orig, thumb, NULL });
		err(EXIT_FAILURE, "%s", THUMBNAILERCMD);
	}
	return pid;
}

/* ── Check / create a thumbnail file ───────────────────────── */

static int
timespeclt(struct timespec *tsp, struct timespec *usp)
{
	return (tsp->tv_sec == usp->tv_sec)
	     ? (tsp->tv_nsec < usp->tv_nsec)
	     : (tsp->tv_sec  < usp->tv_sec);
}

static int
thumbexists(Item *entry, char *mime)
{
	struct stat sb;
	struct timespec origt, mimet;
	pid_t pid;
	int status;

	if (stat(mime, &sb) == -1)
		goto forkthumbnailer;
	mimet = sb.st_mtim;
	if (stat(entry->fullname, &sb) == -1)
		goto forkthumbnailer;
	origt = sb.st_mtim;
	if (timespeclt(&origt, &mimet))
		return true;
forkthumbnailer:
	pid = forkthumb(entry->fullname, mime);
	if (waitpid(pid, &status, 0) == -1)
		return false;
	return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

/* ── Background thread ──────────────────────────────────────── */

static void *
thumbnailer(void *arg)
{
	struct FM *fm = (struct FM *)arg;
	int i;
	char path[PATH_MAX];

	for (i = 0; i < fm->nentries; i++) {
		if (thumbexit_check(fm))
			break;
		if (fm->entries[i].fullname == NULL)
			continue;
		if (strncmp(fm->entries[i].fullname,
		            fm->thumbnaildir, fm->thumbnaildirlen) == 0)
			continue;
		if (setthumbpath(fm, fm->entries[i].fullname, path) == RETURN_FAILURE)
			continue;
		if (thumbexists(&fm->entries[i], path))
			widget_thumb(fm->widget, path, i);
	}
	pthread_exit(0);
}

/* ── Thread lifecycle (called from dir.c / main.c) ──────────── */

void
closethumbthread(struct FM *fm)
{
	if (fm->thumbnaildir == NULL)
		return;
	etlock(&fm->thumblock);
	fm->thumbexit = 1;
	etunlock(&fm->thumblock);
	etjoin(fm->thumbthread, NULL);
	etlock(&fm->thumblock);
	fm->thumbexit = 0;
	etunlock(&fm->thumblock);
}

void
createthumbthread(struct FM *fm)
{
	if (fm->thumbnaildir == NULL)
		return;
	etcreate(&fm->thumbthread, thumbnailer, (void *)fm);
}

/* ── Cache-directory setup ──────────────────────────────────── */

void
initthumbnailer(struct FM *fm)
{
	struct stat sb;
	mode_t mode, dir_mode;
	size_t len;
	int mkdir_errno;
	bool done;
	char path[PATH_MAX];
	char *slash, *str;

	if ((str = getenv("CACHEDIR")) == NULL)
		if ((str = getenv("XDG_CACHE_HOME")) == NULL)
			return;
	len = strlen(str);
	if (PATH_MAX < len + 12)    /* strlen("/thumbnails") + '\0' */
		return;
	mode     = 0700;
	dir_mode = 0755;
	(void)snprintf(path, PATH_MAX, "%s", str);
	slash = strrchr(path, '\0');
	while (--slash > path && *slash == '/')
		*slash = '\0';
	(void)snprintf(path + len, PATH_MAX - len, "/thumbnails");
	fm->thumbnaildir = estrdup(path);
	slash = path;
	for (;;) {
		slash += strspn(slash, "/");
		slash += strcspn(slash, "/");
		done   = (*slash == '\0');
		*slash = '\0';
		if (mkdir(path, done ? mode : dir_mode) == -1) {
			mkdir_errno = errno;
			if (stat(path, &sb) == -1) {
				errno = mkdir_errno;
				warn("%s", fm->thumbnaildir);
				goto error;
			}
			if (!S_ISDIR(sb.st_mode)) {
				errno = ENOTDIR;
				warn("%s", fm->thumbnaildir);
				goto error;
			}
		}
		if (done)
			break;
		*slash = '/';
	}
	fm->thumbnaildirlen = strlen(fm->thumbnaildir);
	return;
error:
	free(fm->thumbnaildir);
	fm->thumbnaildir    = NULL;
	fm->thumbnaildirlen = 0;
}
