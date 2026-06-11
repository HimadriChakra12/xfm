/*
 * main.c - argument parsing, FM initialisation/teardown, event loop
 */

#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "fm.h"
#include "util.h"
#include "widget.h"
#include "icons.h"
#include "dir.h"
#include "thumb.h"
#include "drop.h"

/* ── Open a regular file with the configured opener ─────────── */

static void
fileopen(struct FM *fm, char *path)
{
	pid_t pid;

	if ((pid = efork()) == 0) {
		if (efork() == 0) {
			eexec((char *[]){ fm->opener, path, NULL });
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}
	waitpid(pid, NULL, 0);
}

/* ── FM teardown ────────────────────────────────────────────── */

static void
freefm(struct FM *fm)
{
	size_t i;

	clearcwd(fm->hist);
	freeentries(fm);
	free(fm->entries);
	free(fm->selitems);
	free(fm->thumbnaildir);
	for (i = 0; i < fm->nuserpatts; i++)
		free(fm->userpatts[i].patt);
	free(fm->userpatts);
}

/* ── Usage ──────────────────────────────────────────────────── */

static void
usage(void)
{
	(void)fprintf(stderr, "usage: xfm [-a] [-N name] [-X resources] [path]\n");
	exit(1);
}

/* ── main ───────────────────────────────────────────────────── */

int
main(int argc, char *argv[])
{
	struct FM fm;
	struct Cwd *cwd;
	int ch, nitems;
	int saveargc, force_refresh;
	int nresources = 0;
	int exitval    = EXIT_SUCCESS;
	const char *resources[MAX_RESOURCES];
	char *name;
	char *path     = NULL;
	char *home     = NULL;
	char **saveargv;
	char *text;
	WidgetEvent event;

	saveargv = argv;
	saveargc = argc;
	home     = getenv("HOME");
	name     = getenv("RESOURCES_NAME");
	if (argv[0] != NULL && argv[0][0] != '\0') {
		if ((name = strrchr(argv[0], '/')) != NULL)
			name++;
		else
			name = argv[0];
	}
	if (name == NULL)
		name = APPNAME;
	resources[0] = getenv("RESOURCES_DATA");
	if (resources[0] != NULL)
		nresources++;

	fm = (struct FM){
		.cwd      = emalloc(sizeof(*fm.cwd)),
		.home     = home,
		.homelen  = ((home != NULL) ? strlen(home) : 0),
		.uid      = getuid(),
		.gid      = getgid(),
		.thumblock = PTHREAD_MUTEX_INITIALIZER,
	};
	(*fm.cwd)  = (struct Cwd){ 0 };
	fm.hist    = fm.cwd;
	fm.ngrps   = getgroups(NGROUPS_MAX, fm.grps);
	if ((fm.opener = getenv("OPENER")) == NULL)
		fm.opener = DEF_OPENER;

	while ((ch = getopt(argc, argv, "aN:X:")) != -1) {
		switch (ch) {
		case 'a':
			hide = 0;
			break;
		case 'N':
			name = optarg;
			break;
		case 'X':
			if (nresources < MAX_RESOURCES - 1)
				resources[nresources++] = optarg;
			break;
		default:
			usage();
			break;
		}
	}
	resources[nresources] = NULL;
	argc -= optind;
	argv += optind;
	if (argc > 1)
		usage();
	else if (argc == 1)
		path = *argv;

	initthumbnailer(&fm);
	if ((fm.widget = widget_create(APPCLASS, name, saveargc, saveargv, resources)) == NULL)
		exit(EXIT_FAILURE);
	fm.widgetfd = widget_fd(fm.widget);

#if __OpenBSD__
	if (pledge("stdio rpath proc exec", NULL) == RETURN_FAILURE)
		err(EXIT_FAILURE, "pledge");
#endif

	inituserpatts(&fm);
	if (diropen(&fm, fm.cwd, path) == RETURN_FAILURE)
		goto error;
	fm.last = fm.cwd;
	if (widget_set(fm.widget, fm.cwd->path, fm.cwd->here,
	               fm.entries, fm.nentries, NULL) == RETURN_FAILURE)
		goto error;
	createthumbthread(&fm);
	widget_map(fm.widget);

	text = NULL;
	while ((event = widget_poll(fm.widget, fm.selitems, &nitems,
	                            &fm.cwd->scrl, &text)) != WIDGET_CLOSE) {
		/* translate "-"/"+" GOTO events to PREV/NEXT */
		if (event == WIDGET_GOTO && strcmp(text, "-") == 0)
			event = WIDGET_PREV;
		else if (event == WIDGET_GOTO && strcmp(text, "+") == 0)
			event = WIDGET_NEXT;

		switch (event) {
		case WIDGET_ERROR:
			exitval = EXIT_FAILURE;
			goto done;

		case WIDGET_CONTEXT:
			if (runcontext(&fm, MENU, nitems) == WIDGET_CLOSE)
				goto done;
			if (changedir(&fm, fm.cwd->path, false) == RETURN_FAILURE) {
				exitval = EXIT_FAILURE;
				goto done;
			}
			break;

		case WIDGET_PREV:
		case WIDGET_NEXT:
			cwd = (event == WIDGET_PREV) ? fm.cwd->prev : fm.cwd->next;
			if (cwd == NULL)
				break;
			fm.cwd = cwd;
			if (changedir(&fm, cwd->path, false) == RETURN_FAILURE) {
				exitval = EXIT_FAILURE;
				goto done;
			}
			break;

		case WIDGET_OPEN:
			if (nitems < 1)
				break;
			if (fm.selitems[0] < 0 || fm.selitems[0] >= fm.nentries)
				break;
			if (isdir(&fm.entries[fm.selitems[0]])) {
				if (changedir(&fm, fm.entries[fm.selitems[0]].fullname,
				              false) == RETURN_FAILURE) {
					exitval = EXIT_FAILURE;
					goto done;
				}
			} else {
				fileopen(&fm, fm.entries[fm.selitems[0]].fullname);
			}
			break;

		case WIDGET_DROPASK:
		case WIDGET_DROPCOPY:
		case WIDGET_DROPMOVE:
		case WIDGET_DROPLINK:
			if (text != NULL) {
				/* cross-window drop: URI list in text */
				if (nitems > 0 &&
				    (fm.selitems[0] < 0 || fm.selitems[0] >= fm.nentries))
					path = NULL;
				else
					path = fm.entries[fm.selitems[0]].fullname;
				if (runexdrop(&fm, event, text, path) == WIDGET_CLOSE)
					goto done;
			} else if (nitems > 1 && isdir(&fm.entries[fm.selitems[0]])) {
				/* same-window drop */
				if (runindrop(&fm, event, nitems) == WIDGET_CLOSE)
					goto done;
			}
			if (changedir(&fm, fm.cwd->path, false) == RETURN_FAILURE) {
				exitval = EXIT_FAILURE;
				goto done;
			}
			break;

		case WIDGET_KEYPRESS:
			if (strcmp(text, "^period") == 0) {
				hide          = !hide;
				force_refresh = true;
			} else {
				if (runcontext(&fm, text, nitems) == WIDGET_CLOSE)
					goto done;
				force_refresh = false;
			}
			if (changedir(&fm, fm.cwd->path, force_refresh) == RETURN_FAILURE) {
				exitval = EXIT_FAILURE;
				goto done;
			}
			break;

		case WIDGET_GOTO:
			if (changedir(&fm, text, true) == RETURN_FAILURE) {
				exitval = EXIT_FAILURE;
				goto done;
			}
			break;

		default:
			break;
		}
		text = NULL;
	}
done:
	closethumbthread(&fm);
error:
	freefm(&fm);
	widget_free(fm.widget);
	return exitval;
}
