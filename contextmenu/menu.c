/*
 * contextmenu/menu.c
 *
 * Context menu: filetype detection, X11 popup, command dispatch.
 *
 * KEY DESIGN DECISIONS (fixing the original bugs):
 *   • Reuses the widget's existing Display connection — no second
 *     XOpenDisplay().  Two connections on the same display fight
 *     over grabs and cause instant-dismiss / crashes.
 *   • Grabs are taken AFTER the window is mapped and an Expose has
 *     been processed, so the window is actually visible first.
 *   • Cursor position comes from widget_context_pos() which records
 *     the real root-window coords of the right-click that triggered
 *     the menu — not a hardcoded (0,0).
 *   • The popup window uses override_redirect so the WM never
 *     interferes with it.
 */

#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include "../config.h"
#include "../src/fm.h"
#include "../src/util.h"
#include "../src/widget.h"
#include "filetypes.h"
#include "actions.h"
#include "menuconfig.h"
#include "menu.h"

/* ── Row type ────────────────────────────────────────────────── */

typedef struct {
	const MenuAction *act;
	int               is_sep;
	int               enabled;
} Row;

/* ── Popup state ─────────────────────────────────────────────── */

typedef struct {
	Display    *dpy;       /* borrowed from widget — do NOT close */
	int         screen;
	Visual     *visual;
	Colormap    cmap;
	Window      win;
	GC          gc;
	XftFont    *font;
	XftDraw    *draw;

	XftColor    fg, bg, selfg, selbg;
	XftColor    border_col, sep_col, dim_col;

	Row        *rows;
	int         nrows;
	int         width;
	int         hovered;
} Popup;

/* ── Colour helpers ──────────────────────────────────────────── */

static void
alloc_color(Display *dpy, Visual *vis, Colormap cmap,
            const char *name, XftColor *out)
{
	if (!XftColorAllocName(dpy, vis, cmap, name, out))
		warnx("ctx menu: cannot allocate colour '%s'", name);
}

static void
free_colors(Popup *p)
{
	XftColorFree(p->dpy, p->visual, p->cmap, &p->bg);
	XftColorFree(p->dpy, p->visual, p->cmap, &p->fg);
	XftColorFree(p->dpy, p->visual, p->cmap, &p->selbg);
	XftColorFree(p->dpy, p->visual, p->cmap, &p->selfg);
	XftColorFree(p->dpy, p->visual, p->cmap, &p->border_col);
	XftColorFree(p->dpy, p->visual, p->cmap, &p->sep_col);
	XftColorFree(p->dpy, p->visual, p->cmap, &p->dim_col);
}

/* ── Geometry helpers ────────────────────────────────────────── */

static int
row_height(const Row *r)
{
	return r->is_sep ? CTX_SEP_HEIGHT : CTX_ITEM_HEIGHT;
}

static int
row_y(const Popup *p, int i)
{
	int y = CTX_BORDER;
	for (int j = 0; j < i; j++)
		y += row_height(&p->rows[j]);
	return y;
}

static int
total_height(const Popup *p)
{
	int h = 2 * CTX_BORDER;
	for (int i = 0; i < p->nrows; i++)
		h += row_height(&p->rows[i]);
	return h;
}

static int
row_at_y(const Popup *p, int y)
{
	int cur = CTX_BORDER;
	for (int i = 0; i < p->nrows; i++) {
		int h = row_height(&p->rows[i]);
		if (y >= cur && y < cur + h)
			return i;
		cur += h;
	}
	return -1;
}

static int
compute_width(Popup *p)
{
	int maxw = CTX_MIN_WIDTH;
	for (int i = 0; i < p->nrows; i++) {
		if (p->rows[i].is_sep || !p->rows[i].act)
			continue;
		XGlyphInfo ext;
		const char *lbl = p->rows[i].act->label;
		XftTextExtentsUtf8(p->dpy, p->font,
		                   (const FcChar8 *)lbl, (int)strlen(lbl), &ext);
		int w = ext.width + 2 * (CTX_BORDER + CTX_HPAD);
		if (p->rows[i].act->key)
			w += CTX_SHORTCUT_GAP + 20;
		if (w > maxw) maxw = w;
	}
	return maxw;
}

/* ── Drawing ─────────────────────────────────────────────────── */

static void
draw_row(Popup *p, int i)
{
	Row      *r   = &p->rows[i];
	int       y   = row_y(p, i);
	int       h   = row_height(r);
	int       sel = (i == p->hovered && !r->is_sep && r->enabled);
	XftColor *bg  = sel ? &p->selbg : &p->bg;
	XftColor *fg  = (r->enabled && !r->is_sep) ? (sel ? &p->selfg : &p->fg)
	                                            : &p->dim_col;

	/* row background */
	XSetForeground(p->dpy, p->gc, bg->pixel);
	XFillRectangle(p->dpy, p->win, p->gc,
	               CTX_BORDER, y, p->width - 2 * CTX_BORDER, h);

	if (r->is_sep) {
		/* horizontal rule centred in the separator row */
		XSetForeground(p->dpy, p->gc, p->sep_col.pixel);
		XFillRectangle(p->dpy, p->win, p->gc,
		               CTX_BORDER + CTX_HPAD / 2,
		               y + CTX_SEP_HEIGHT / 2,
		               p->width - 2 * (CTX_BORDER + CTX_HPAD / 2), 1);
		return;
	}

	if (!r->act || !r->act->label)
		return;

	int ty = y + (CTX_ITEM_HEIGHT + p->font->ascent - p->font->descent) / 2;
	int lx = CTX_BORDER + CTX_HPAD;

	XftDrawStringUtf8(p->draw, fg, p->font, lx, ty,
	                  (const FcChar8 *)r->act->label,
	                  (int)strlen(r->act->label));

	if (r->act->key) {
		char hint[8];
		if (r->act->key == '\x7f')
			snprintf(hint, sizeof(hint), "Del");
		else if (r->act->key < 0x20)
			snprintf(hint, sizeof(hint), "^%c", r->act->key + 0x40);
		else
			snprintf(hint, sizeof(hint), "%c", (unsigned char)r->act->key);

		XGlyphInfo ext;
		XftTextExtentsUtf8(p->dpy, p->font,
		                   (const FcChar8 *)hint, (int)strlen(hint), &ext);
		XftDrawStringUtf8(p->draw, &p->dim_col, p->font,
		                  p->width - CTX_BORDER - CTX_HPAD - ext.width, ty,
		                  (const FcChar8 *)hint, (int)strlen(hint));
	}
}

static void
draw_all(Popup *p)
{
	/* border frame */
	XSetForeground(p->dpy, p->gc, p->border_col.pixel);
	XFillRectangle(p->dpy, p->win, p->gc, 0, 0, p->width, total_height(p));

	/* background fill */
	XSetForeground(p->dpy, p->gc, p->bg.pixel);
	XFillRectangle(p->dpy, p->win, p->gc,
	               CTX_BORDER, CTX_BORDER,
	               p->width - 2 * CTX_BORDER,
	               total_height(p) - 2 * CTX_BORDER);

	for (int i = 0; i < p->nrows; i++)
		draw_row(p, i);
}

/* ── Popup event loop ────────────────────────────────────────── */

static const MenuAction *
popup_run(Popup *p, int sx, int sy)
{
	int h  = total_height(p);
	int sw = DisplayWidth(p->dpy,  p->screen);
	int sh = DisplayHeight(p->dpy, p->screen);

	/* keep entirely on screen */
	if (sx + p->width > sw) sx = sw - p->width;
	if (sy + h        > sh) sy = sh - h;
	if (sx < 0) sx = 0;
	if (sy < 0) sy = 0;

	XMoveResizeWindow(p->dpy, p->win, sx, sy, p->width, h);
	XMapRaised(p->dpy, p->win);

	/*
	 * Wait for the first Expose before grabbing so the window is
	 * truly mapped and visible.  Without this the grab either fails
	 * or the very first click dismisses the menu.
	 */
	{
		XEvent ev;
		do { XWindowEvent(p->dpy, p->win, ExposureMask, &ev); }
		while (ev.type != Expose);
	}

	/* create Xft draw surface now that the window exists */
	p->draw = XftDrawCreate(p->dpy, p->win, p->visual, p->cmap);
	if (!p->draw) {
		XUnmapWindow(p->dpy, p->win);
		return NULL;
	}

	draw_all(p);
	XFlush(p->dpy);

	/* grab pointer confined to our window */
	int pgrab = XGrabPointer(p->dpy, p->win, False,
	    ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
	    GrabModeAsync, GrabModeAsync,
	    None, None, CurrentTime);
	int kgrab = XGrabKeyboard(p->dpy, p->win, True,
	    GrabModeAsync, GrabModeAsync, CurrentTime);

	if (pgrab != GrabSuccess || kgrab != GrabSuccess) {
		warnx("ctx menu: grab failed (pointer=%d keyboard=%d)", pgrab, kgrab);
		if (pgrab == GrabSuccess) XUngrabPointer(p->dpy, CurrentTime);
		if (kgrab == GrabSuccess) XUngrabKeyboard(p->dpy, CurrentTime);
		XftDrawDestroy(p->draw);
		XUnmapWindow(p->dpy, p->win);
		return NULL;
	}

	const MenuAction *chosen = NULL;
	XEvent ev;
	int running = 1;
	int prev_hover = -1;

	while (running) {
		XNextEvent(p->dpy, &ev);

		switch (ev.type) {
		case Expose:
			draw_all(p);
			XFlush(p->dpy);
			break;

		case MotionNotify: {
			int ri = row_at_y(p, ev.xmotion.y);
			if (ri >= 0 && p->rows[ri].is_sep)  ri = -1;
			if (ri >= 0 && !p->rows[ri].enabled) ri = -1;
			if (ri != p->hovered) {
				prev_hover  = p->hovered;
				p->hovered  = ri;
				if (prev_hover >= 0) draw_row(p, prev_hover);
				if (p->hovered >= 0) draw_row(p, p->hovered);
				XFlush(p->dpy);
			}
			break;
		}

		case ButtonPress:
			/* click outside → dismiss */
			if (ev.xbutton.window != p->win) {
				running = 0;
			}
			break;

		case ButtonRelease:
			if (ev.xbutton.window != p->win) {
				running = 0;
				break;
			}
			if (ev.xbutton.button == Button1) {
				int ri = row_at_y(p, ev.xbutton.y);
				if (ri >= 0 && !p->rows[ri].is_sep && p->rows[ri].enabled
				    && p->rows[ri].act) {
					chosen  = p->rows[ri].act;
				}
				running = 0;
			}
			break;

		case KeyPress: {
			KeySym   ks;
			unsigned state;
			XkbLookupKeySym(p->dpy, ev.xkey.keycode, ev.xkey.state,
			                &state, &ks);

			if (ks == XK_Escape || ks == XK_q) {
				running = 0;
				break;
			}
			/* arrow / vim navigation */
			if (ks == XK_Up || ks == XK_k) {
				int prev = p->hovered;
				int ni   = (prev <= 0) ? p->nrows - 1 : prev - 1;
				while (ni != prev && (p->rows[ni].is_sep || !p->rows[ni].enabled))
					ni = (ni <= 0) ? p->nrows - 1 : ni - 1;
				p->hovered = ni;
				if (prev    >= 0 && prev    < p->nrows) draw_row(p, prev);
				if (p->hovered >= 0) draw_row(p, p->hovered);
				XFlush(p->dpy);
				break;
			}
			if (ks == XK_Down || ks == XK_j) {
				int prev = p->hovered;
				int ni   = (prev < 0 || prev >= p->nrows - 1) ? 0 : prev + 1;
				while (ni != prev && (p->rows[ni].is_sep || !p->rows[ni].enabled))
					ni = (ni >= p->nrows - 1) ? 0 : ni + 1;
				p->hovered = ni;
				if (prev    >= 0 && prev    < p->nrows) draw_row(p, prev);
				if (p->hovered >= 0) draw_row(p, p->hovered);
				XFlush(p->dpy);
				break;
			}
			if (ks == XK_Return || ks == XK_KP_Enter) {
				if (p->hovered >= 0 && !p->rows[p->hovered].is_sep
				    && p->rows[p->hovered].enabled
				    && p->rows[p->hovered].act) {
					chosen = p->rows[p->hovered].act;
				}
				running = 0;
				break;
			}
			/* single-character shortcuts */
			if (ks < 256) {
				char c = (char)(unsigned char)ks;
				for (int i = 0; i < p->nrows; i++) {
					if (!p->rows[i].act || p->rows[i].is_sep) continue;
					if (!p->rows[i].enabled) continue;
					if (p->rows[i].act->key == c) {
						chosen  = p->rows[i].act;
						running = 0;
						break;
					}
				}
			}
			break;
		}

		default:
			break;
		}
	}

	XUngrabPointer(p->dpy,  CurrentTime);
	XUngrabKeyboard(p->dpy, CurrentTime);
	XftDrawDestroy(p->draw);
	p->draw = NULL;
	XUnmapWindow(p->dpy, p->win);
	XFlush(p->dpy);
	return chosen;
}

/* ── Row builder helpers ─────────────────────────────────────── */

static int
append_table(Row *rows, int n, const MenuAction *tbl, int nsel, int has_dir)
{
	int skip_first_sep = (n > 0);
	for (int i = 0; ; i++) {
		/* terminator: both NULL */
		if (tbl[i].label == NULL && tbl[i].cmd == NULL)
			break;
		/* separator */
		if (tbl[i].label == NULL) {
			if (skip_first_sep) { skip_first_sep = 0; continue; }
			if (n > 0 && rows[n - 1].is_sep) continue;
			rows[n++] = (Row){ .is_sep = 1, .enabled = 1 };
			continue;
		}
		skip_first_sep = 0;
		int flags = tbl[i].flags;
		int ok = ((nsel == 0 && (flags & ACT_NOSEL))  ||
		          (nsel == 1 && (flags & ACT_SINGLE))  ||
		          (nsel  > 1 && (flags & ACT_MULTI))   ||
		          (has_dir   && (flags & ACT_DIR)));
		if (!ok) continue;
		rows[n++] = (Row){
			.act     = &tbl[i],
			.is_sep  = 0,
			.enabled = tbl[i].enabled,
		};
	}
	return n;
}

static int
trim_seps(Row *rows, int n)
{
	while (n > 0 && rows[n - 1].is_sep) n--;
	return n;
}

/* ── Public: filetype detection ──────────────────────────────── */

FileType
ctx_detect_type(const char *filename, unsigned char mode)
{
	/* mode byte: bits 0-2 are the type nibble; 0x02 == MODE_DIR */
	if ((mode & 0x07) == 0x02)
		return FT_DIR;
	if (!filename)
		return FT_UNKNOWN;
	for (int i = 0; ft_patterns[i].pattern != NULL; i++) {
		if (fnmatch(ft_patterns[i].pattern, filename, FNM_CASEFOLD) == 0)
			return ft_patterns[i].type;
	}
	return FT_UNKNOWN;
}

/* ── Public: execute one action ──────────────────────────────── */

int
ctx_run_action(struct FM *fm, int *selitems, int nsel,
               const MenuAction *act)
{
	if (!act || !act->cmd) return 0;

	const char *cmd = act->cmd;
	int in_term = 0, in_bg = 0;
	if (strncmp(cmd, CMD_TERM, strlen(CMD_TERM)) == 0) {
		in_term = 1; cmd += strlen(CMD_TERM);
	} else if (strncmp(cmd, CMD_BG, strlen(CMD_BG)) == 0) {
		in_bg = 1; cmd += strlen(CMD_BG);
	}

	/* ── build quoted path list (%s) ── */
	char qpaths[8192] = "";
	for (int i = 0; i < nsel; i++) {
		if (selitems[i] >= fm->nentries) continue;
		char *p = fm->entries[selitems[i]].fullname;
		if (!p) continue;
		if (qpaths[0]) strncat(qpaths, " ",    sizeof(qpaths) - 1);
		strncat(qpaths, "'", sizeof(qpaths) - 1);
		for (char *c = p; *c; c++) {
			if (*c == '\'') strncat(qpaths, "'\\''", sizeof(qpaths) - 1);
			else { char tmp[2] = {*c, 0}; strncat(qpaths, tmp, sizeof(qpaths) - 1); }
		}
		strncat(qpaths, "'", sizeof(qpaths) - 1);
	}

	const char *fpath = (nsel > 0 && selitems[0] < fm->nentries)
	                    ? (fm->entries[selitems[0]].fullname ? fm->entries[selitems[0]].fullname : "") : "";
	const char *cwd   = (fm->cwd && fm->cwd->path) ? fm->cwd->path : ".";
	const char *bname = strrchr(fpath, '/');
	bname = bname ? bname + 1 : fpath;

	/* ── substitute into command template ── */
	size_t cap = strlen(cmd) + strlen(qpaths) + strlen(fpath) +
	             strlen(cwd) + strlen(bname) + 512;
	char *shell_cmd = malloc(cap);
	if (!shell_cmd) return 0;
	shell_cmd[0] = '\0';

	for (const char *s = cmd; *s; s++) {
		if (*s != '%') {
			char tmp[2] = {*s, 0};
			strncat(shell_cmd, tmp, cap - strlen(shell_cmd) - 1);
			continue;
		}
		s++;
		char tmp[4096];
		switch (*s) {
		case 's': strncat(shell_cmd, qpaths, cap - strlen(shell_cmd) - 1); break;
		case 'f': snprintf(tmp,sizeof(tmp),"'%s'",fpath);
		          strncat(shell_cmd, tmp, cap - strlen(shell_cmd) - 1); break;
		case 'd': snprintf(tmp,sizeof(tmp),"'%s'",cwd);
		          strncat(shell_cmd, tmp, cap - strlen(shell_cmd) - 1); break;
		case 'n': strncat(shell_cmd, bname, cap - strlen(shell_cmd) - 1); break;
		case '%': strncat(shell_cmd, "%",   cap - strlen(shell_cmd) - 1); break;
		default: { char t2[3] = {'%', *s, 0};
		           strncat(shell_cmd, t2, cap - strlen(shell_cmd) - 1); break; }
		}
	}

	/* ── fork & exec ── */
	pid_t pid = fork();
	if (pid == 0) {
		if (in_bg) { if (fork() != 0) _exit(0); setsid(); }
		if (in_term) {
			char *term = getenv("TERMINAL");
			if (!term) term = "xterm";
			execlp(term, term, "-e", "sh", "-c", shell_cmd, (char *)NULL);
			execlp("xterm","xterm","-e","sh","-c", shell_cmd, (char *)NULL);
		} else {
			execlp("sh", "sh", "-c", shell_cmd, (char *)NULL);
		}
		_exit(127);
	}
	free(shell_cmd);
	if (pid > 0) waitpid(pid, NULL, 0);

	/* heuristic: does this action modify the filesystem? */
	static const char *refresh_words[] = {
		"delete","rename","compress","extract","install",
		"new","move","copy","cut","paste","rotate",
		"convert","create","burn","mount", NULL
	};
	char lc[256];
	strncpy(lc, act->label ? act->label : "", sizeof(lc) - 1);
	for (char *cp = lc; *cp; cp++) *cp = tolower((unsigned char)*cp);
	for (int i = 0; refresh_words[i]; i++)
		if (strstr(lc, refresh_words[i])) return 1;
	return 0;
}

/* ── Public: show context menu ───────────────────────────────── */

int
ctx_show(struct FM *fm, int *selitems, int nsel, int root_x, int root_y)
{
	/* reuse the widget's display — never open a second connection */
	Display *dpy = widget_display(fm->widget);
	if (!dpy) { warnx("ctx menu: no display"); return 0; }
	int screen = DefaultScreen(dpy);

	/* detect filetype of first selected item */
	FileType ftype   = FT_UNKNOWN;
	int      has_dir = 0;
	if (nsel > 0 && selitems[0] < fm->nentries) {
		Item *it = &fm->entries[selitems[0]];
		ftype    = ctx_detect_type(it->name, it->mode);
		has_dir  = (ftype == FT_DIR);
	}

	/* find type-specific action group */
	const MenuAction *type_acts = NULL;
	for (int i = 0; action_groups[i].actions != NULL; i++) {
		if (action_groups[i].type == ftype && action_groups[i].enabled) {
			type_acts = action_groups[i].actions;
			break;
		}
	}

	/* build flat row list */
	Row rows[512];
	int nrows = 0;
	if (type_acts)
		nrows = append_table(rows, nrows, type_acts, nsel, has_dir);
	nrows = append_table(rows, nrows, actions_common, nsel, has_dir);
	nrows = trim_seps(rows, nrows);
	if (nrows == 0) return 0;

	/* visuals */
	Visual  *vis  = DefaultVisual(dpy, screen);
	Colormap cmap = DefaultColormap(dpy, screen);

	/* font */
	char fspec[256];
	snprintf(fspec, sizeof(fspec), "%s:size=%d", CTX_FONT_FACE, CTX_FONT_SIZE);
	XftFont *font = XftFontOpenName(dpy, screen, fspec);
	if (!font) font = XftFontOpenName(dpy, screen, "monospace:size=11");
	if (!font) { warnx("ctx menu: cannot open font"); return 0; }

	/* create override_redirect window (invisible until mapped) */
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixel  = BlackPixel(dpy, screen),
		.event_mask        = ExposureMask | KeyPressMask |
		                     ButtonPressMask | ButtonReleaseMask |
		                     PointerMotionMask,
	};
	Window win = XCreateWindow(
	    dpy, RootWindow(dpy, screen),
	    root_x, root_y, CTX_MIN_WIDTH, 10,   /* height updated later */
	    0, DefaultDepth(dpy, screen), InputOutput, vis,
	    CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);

	GC gc = XCreateGC(dpy, win, 0, NULL);

	Popup p = {
		.dpy     = dpy,
		.screen  = screen,
		.visual  = vis,
		.cmap    = cmap,
		.win     = win,
		.gc      = gc,
		.font    = font,
		.draw    = NULL,
		.rows    = rows,
		.nrows   = nrows,
		.hovered = -1,
	};
	alloc_color(dpy, vis, cmap, CTX_COLOR_BG,     &p.bg);
	alloc_color(dpy, vis, cmap, CTX_COLOR_FG,     &p.fg);
	alloc_color(dpy, vis, cmap, CTX_COLOR_SEL_BG, &p.selbg);
	alloc_color(dpy, vis, cmap, CTX_COLOR_SEL_FG, &p.selfg);
	alloc_color(dpy, vis, cmap, CTX_COLOR_BORDER, &p.border_col);
	alloc_color(dpy, vis, cmap, CTX_COLOR_SEP,    &p.sep_col);
	alloc_color(dpy, vis, cmap, CTX_COLOR_DIM,    &p.dim_col);

	p.width = compute_width(&p);

	const MenuAction *chosen = popup_run(&p, root_x, root_y);

	free_colors(&p);
	XftFontClose(dpy, font);
	XFreeGC(dpy, gc);
	XDestroyWindow(dpy, win);
	/* do NOT call XCloseDisplay — display is owned by the widget */
	XFlush(dpy);

	if (!chosen) return 0;
	return ctx_run_action(fm, selitems, nsel, chosen);
}
