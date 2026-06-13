.SUFFIXES: .c .o .dbg

PROG      = xfm
DEBUG_PROG = ${PROG:=_dbg}

# ── Source files ───────────────────────────────────────────────
# main.c                   - arg parsing, event loop
# src/dir.c                - directory listing, entry metadata, history
# src/thumb.c              - thumbnail thread
# src/drop.c               - drag-and-drop / context-menu subprocess
# src/widget.c             - X11 widget (drawing, input, DnD protocol)
# src/util.c               - safe wrappers (malloc, fork, …)
# src/icons.c              - icon table (XPM data)
# contextmenu/menu.c       - X11 context menu popup + dispatcher

OBJS = \
	main.o \
	src/dir.o \
	src/thumb.o \
	src/drop.o \
	src/widget.o \
	src/util.o \
	src/icons.o \
	contextmenu/menu.o \
	control/dragndrop.o \
	control/selection.o \
	control/font.o

DEBUG_OBJS = ${OBJS:.o=.dbg}
SRCS       = ${OBJS:.o=.c}

MANS = \
	xfm.1 \
	control/ctrldnd.3 \
	control/ctrlfnt.3 \
	control/ctrlsel.3

SCRIPTS = \
	examples/xfilesctl \
	examples/xfilesthumb

WINICONS = \
	icons/winicon16x16.abgr \
	icons/winicon32x32.abgr \
	icons/winicon48x48.abgr \
	icons/winicon64x64.abgr

ICONS = \
	icons/file-app.xpm \
	icons/file-archive.xpm \
	icons/file-audio.xpm \
	icons/file-code.xpm \
	icons/file-config.xpm \
	icons/file-core.xpm \
	icons/file-gear.xpm \
	icons/file-image.xpm \
	icons/file-info.xpm \
	icons/file-object.xpm \
	icons/file-text.xpm \
	icons/file-video.xpm \
	icons/file.xpm \
	icons/folder-apps.xpm \
	icons/folder-book.xpm \
	icons/folder-code.xpm \
	icons/folder-db.xpm \
	icons/folder-download.xpm \
	icons/folder-game.xpm \
	icons/folder-gear.xpm \
	icons/folder-home.xpm \
	icons/folder-image.xpm \
	icons/folder-link.xpm \
	icons/folder-mail.xpm \
	icons/folder-meme.xpm \
	icons/folder-mount.xpm \
	icons/folder-music.xpm \
	icons/folder-trash.xpm \
	icons/folder-up.xpm \
	icons/folder-video.xpm \
	icons/folder.xpm

PROG_CPPFLAGS = \
	-D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_GNU_SOURCE -D_DEFAULT_SOURCE \
	-I. -I/usr/local/include -I/usr/X11R6/include \
	-I/usr/include/freetype2 -I/usr/X11R6/include/freetype2 \
	${CPPFLAGS}

PROG_CFLAGS = \
	-std=c99 -pedantic \
	${PROG_CPPFLAGS} \
	${CFLAGS}

PROG_LDFLAGS = \
	-L/usr/local/lib -L/usr/X11R6/lib \
	-lfontconfig -lXft -lX11 -lXext -lXcursor -lXrender -lXpm -lpthread \
	${LDFLAGS} ${LDLIBS}

DEBUG_FLAGS = \
	-g -O0 -DDEBUG -Wall -Wextra -Wpedantic

# ── Targets ────────────────────────────────────────────────────

all: ${PROG}

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${PROG_LDFLAGS}

.c.o:
	${CC} ${PROG_CFLAGS} -o $@ -c $<

debug: ${DEBUG_PROG}

${DEBUG_PROG}: ${DEBUG_OBJS}
	${CC} -o $@ ${DEBUG_OBJS} ${PROG_LDFLAGS} ${DEBUG_FLAGS}

.c.dbg:
	${CC} ${PROG_CFLAGS} ${DEBUG_FLAGS} -o $@ -c $<

# ── Per-file dependencies ──────────────────────────────────────

control/selection.o control/selection.dbg: \
	control/selection.h
control/dragndrop.o control/dragndrop.dbg: \
	control/dragndrop.h control/selection.h
control/font.o control/font.dbg: \
	control/font.h

main.o main.dbg: \
	config.h src/fm.h src/util.h src/widget.h src/icons.h \
	src/dir.h src/thumb.h src/drop.h \
	contextmenu/menu.h contextmenu/menuconfig.h

src/dir.o src/dir.dbg: \
	config.h src/fm.h src/util.h src/widget.h src/icons.h \
	src/dir.h src/thumb.h
src/thumb.o src/thumb.dbg: \
	config.h src/fm.h src/util.h src/widget.h src/thumb.h
src/drop.o src/drop.dbg: \
	config.h src/fm.h src/util.h src/widget.h src/dir.h src/drop.h
src/widget.o src/widget.dbg: \
	config.h src/util.h src/widget.h src/icons.h \
	control/selection.h control/dragndrop.h control/font.h
src/icons.o src/icons.dbg: \
	${ICONS} ${WINICONS}
src/util.o src/util.dbg: \
	src/util.h

contextmenu/menu.o contextmenu/menu.dbg: \
	contextmenu/menu.c contextmenu/menu.h \
	contextmenu/menuconfig.h contextmenu/actions.h contextmenu/filetypes.h \
	config.h src/fm.h src/util.h src/widget.h

# ── Misc ───────────────────────────────────────────────────────

lint: ${SCRIPTS} ${MANS}
	-shellcheck ${SCRIPTS}
	-mandoc -T lint -W warning ${MANS}

tags: ${SRCS}
	ctags ${SRCS}

clean:
	rm -f ${OBJS} ${PROG} ${PROG:=.core}

distclean: clean
	rm -f ${DEBUG_OBJS} ${DEBUG_PROG} ${DEBUG_PROG:=.core} tags

.PHONY: all debug lint clean distclean
