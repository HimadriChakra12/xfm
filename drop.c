/*
 * drop.c - drag-and-drop and context-menu subprocess handling
 *
 * Spawns xfilesctl (CONTEXTCMD) via a self-pipe trick so the widget
 * can keep servicing X events while the helper runs.
 */

#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "fm.h"
#include "util.h"
#include "widget.h"
#include "dir.h"
#include "drop.h"

/* ── Internal: run CONTEXTCMD and pump widget events ─────────── */

WidgetEvent
runxfilesctl(struct FM *fm, char **argv, char *path)
{
	enum { FILE_WIDGET, FILE_CHILD };   /* pollfd indices */
	enum { END_READ,    END_WRITE  };   /* pipe-end indices */
	struct pollfd pollfds[2];
	int pipefds[2];
	pid_t pid;
	WidgetEvent retval = WIDGET_NONE;
	extern char **environ;

	/*
	 * Self-pipe trick (credit: @emanuele6):
	 *   child  – spawns xfilesctl, waits for it, then exits (closing
	 *            the write end of the pipe).
	 *   parent – polls both the widget fd and the read end; EOF on
	 *            the pipe means xfilesctl is done.
	 */
	if (pipe2(pipefds, O_CLOEXEC) == RETURN_FAILURE)
		err(EXIT_FAILURE, "pipe2");
	if ((pid = efork()) == 0) {
		/* waiting child */
		eclose(pipefds[END_READ]);
		if (path != NULL)
			wchdir(path);
		if (posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ) != 0)
			err(EXIT_FAILURE, "posix_spawnp");
		(void)ewaitpid(pid);
		exit(EXIT_SUCCESS);
	}
	eclose(pipefds[END_WRITE]);
	pollfds[FILE_CHILD].fd   = pipefds[END_READ];
	pollfds[FILE_WIDGET].fd  = fm->widgetfd;
	pollfds[FILE_WIDGET].events = pollfds[FILE_CHILD].events = POLLIN;
	for (;;) {
		if (poll(pollfds, LEN(pollfds), -1) == -1) {
			if (errno == EINTR)
				continue;
			err(EXIT_FAILURE, "poll");
		}
		if (pollfds[FILE_WIDGET].revents & (POLLERR | POLLNVAL))
			errx(EXIT_FAILURE, "%d: bad fd", pollfds[FILE_WIDGET].fd);
		if (pollfds[FILE_CHILD].revents & (POLLERR | POLLNVAL))
			errx(EXIT_FAILURE, "%d: bad fd", pollfds[FILE_CHILD].fd);
		if (pollfds[FILE_WIDGET].revents & POLLHUP)
			pollfds[FILE_WIDGET].fd = -1;
		if (pollfds[FILE_CHILD].revents & POLLHUP) {
			(void)ewaitpid(pid);
			break;          /* xfilesctl terminated */
		}
		if (pollfds[FILE_WIDGET].revents & POLLIN) {
			if ((retval = widget_wait(fm->widget)) == WIDGET_CLOSE)
				break;  /* window closed */
		}
	}
	eclose(pipefds[END_READ]);
	return retval;
}

/* ── Context menu (right-click / keyboard shortcut) ──────────── */

WidgetEvent
runcontext(struct FM *fm, char *cmd, int nselitems)
{
	int i;
	char **argv;
	WidgetEvent retval;

	argv    = emalloc((nselitems + 3) * sizeof(*argv));
	argv[0] = CONTEXTCMD;
	argv[1] = cmd;
	for (i = 0; i < nselitems; i++)
		argv[i + 2] = fm->entries[fm->selitems[i]].fullname;
	argv[i + 2] = NULL;
	retval = runxfilesctl(fm, argv, NULL);
	free(argv);
	return retval;
}

/* ── Drag-and-drop within the same window ────────────────────── */

WidgetEvent
runindrop(struct FM *fm, WidgetEvent event, int nitems)
{
	int i;
	char **argv;
	char *path;
	WidgetEvent retval;

	/*
	 * selitems[0] is the drop-target directory.
	 * selitems[1..nitems-1] are the files being dropped.
	 */
	path = fm->entries[fm->selitems[0]].fullname;
	if ((argv = malloc((nitems + 2) * sizeof(*argv))) == NULL)
		return WIDGET_NONE;
	argv[0] = CONTEXTCMD;
	switch (event) {
	case WIDGET_DROPCOPY: argv[1] = DROPCOPY; break;
	case WIDGET_DROPMOVE: argv[1] = DROPMOVE; break;
	case WIDGET_DROPLINK: argv[1] = DROPLINK; break;
	default:              argv[1] = DROPASK;  break;
	}
	for (i = 1; i < nitems; i++)
		argv[i + 1] = fm->entries[fm->selitems[i]].fullname;
	argv[nitems + 1] = NULL;
	retval = runxfilesctl(fm, argv, path);
	free(argv);
	return retval;
}

/* ── Drag-and-drop from a different window (URI list) ────────── */

WidgetEvent
runexdrop(struct FM *fm, WidgetEvent event, char *text, char *path)
{
	size_t capacity, argc, i, j;
	char **argv, **p;
	WidgetEvent retval;

	argc     = 0;
	capacity = INCRSIZE;
	if ((argv = malloc(capacity * sizeof(*argv))) == NULL)
		return WIDGET_NONE;
	argv[argc++] = CONTEXTCMD;
	switch (event) {
	case WIDGET_DROPCOPY: argv[argc++] = DROPCOPY; break;
	case WIDGET_DROPMOVE: argv[argc++] = DROPMOVE; break;
	case WIDGET_DROPLINK: argv[argc++] = DROPLINK; break;
	default:              argv[argc++] = DROPASK;  break;
	}
	/* Parse "file:///path\r\n" URI list into plain paths */
	for (i = 0; text[i] != '\0'; i++) {
		if (strncmp(text + i, URI_PREFIX, sizeof(URI_PREFIX) - 1) == 0)
			i += sizeof(URI_PREFIX) - 1;
		if (argc + 1 > capacity) {
			capacity += INCRSIZE;
			p = realloc(argv, capacity * sizeof(*argv));
			if (p == NULL) {
				warn("realloc");
				free(argv);
				return WIDGET_NONE;
			}
			argv = p;
		}
		argv[argc++] = text + i;
		j = i + strcspn(text + i, "\r\n");
		if (text[j] == '\0')
			break;
		i = j;
		if (text[j] == '\r' && text[j + 1] == '\n')
			i++;
		text[j] = '\0';
	}
	argv[argc++] = NULL;
	retval = runxfilesctl(fm, argv, path);
	free(argv);
	return retval;
}
