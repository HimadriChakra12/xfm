/*
 * contextmenu/actions.h
 *
 * Defines every action that can appear in the context menu.
 * One table per filetype group plus a "common" table shown for
 * every file regardless of type.
 *
 * ── HOW TO CUSTOMISE ────────────────────────────────────────────
 *
 * Each entry in a table is a MenuAction:
 *
 *   { "Label", "cmd %s", 'k', ACT_SINGLE | ACT_MULTI, 1 }
 *    │          │          │   │                        └─ enabled (0 = hidden)
 *    │          │          │   └─ selection mode flags
 *    │          │          └─ keyboard shortcut (0 = none)
 *    │          └─ shell command; %s is replaced by the file path(s)
 *    └─ text shown in the menu
 *
 * Selection mode flags (can be OR'd):
 *   ACT_SINGLE  – show when exactly one file is selected
 *   ACT_MULTI   – show when multiple files are selected
 *   ACT_DIR     – also show on directories
 *   ACT_NOSEL   – show when nothing is selected (blank-area click)
 *
 * Command string substitutions (applied before exec):
 *   %s   – space-separated quoted file path(s)
 *   %d   – directory containing the first selected file
 *   %f   – first selected file path only (even with multi-select)
 *   %n   – bare filename of first selected file (no directory)
 *   %%   – literal percent sign
 *
 * Command execution:
 *   Commands are run through the shell (sh -c "...").
 *   Prefix with "TERM " (literally the 5 chars "TERM ") to open in
 *   a new terminal window using the TERMINAL env var (default: xterm).
 *   Prefix with "BG " to run detached in the background (no wait).
 *
 * To add a new action:
 *   1. Write the entry in the appropriate table below.
 *   2. Set enabled = 0 to hide it without deleting it.
 *   3. Recompile.  No other file needs changing.
 *
 * To add a whole new group:
 *   1. Add a FT_xxx value in filetypes.h.
 *   2. Add FTPATT entries in filetypes.h.
 *   3. Add a static table + entry in the groups[] array below.
 */

#ifndef ACTIONS_H
#define ACTIONS_H

#include <stddef.h>

/* ── Selection mode flags ────────────────────────────────────── */
#define ACT_SINGLE  (1 << 0)  /* valid for a single selection     */
#define ACT_MULTI   (1 << 1)  /* valid for multiple selections    */
#define ACT_DIR     (1 << 2)  /* also valid on directories        */
#define ACT_NOSEL   (1 << 3)  /* valid when nothing is selected   */

/* ── Command prefix tokens ───────────────────────────────────── */
#define CMD_TERM    "TERM "   /* run in a new terminal            */
#define CMD_BG      "BG "     /* run detached in background       */

/* ── Single action descriptor ────────────────────────────────── */
typedef struct {
	const char *label;    /* text shown in the menu               */
	const char *cmd;      /* shell command with %s/%d/%f/%n/%% subst */
	char        key;      /* keyboard shortcut (0 = none)         */
	int         flags;    /* ACT_* bitmask                        */
	int         enabled;  /* 0 = hidden, 1 = shown                */
} MenuAction;

/* ── Separator sentinel ──────────────────────────────────────── */
/*  Use  { NULL, NULL, 0, 0, 1 }  as a visual separator line.   */
#define SEPARATOR  { NULL, NULL, 0, 0, 1 }

/* ════════════════════════════════════════════════════════════════
 * COMMON ACTIONS  (shown for every file/dir regardless of type)
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_common[] = {
	{ "Open",            "%s",                    'o', ACT_SINGLE|ACT_DIR, 1 },
	{ "Open With…",      "BG xdg-open %s",        'w', ACT_SINGLE,        1 },
	SEPARATOR,
	{ "Cut",             NULL,                    'x', ACT_SINGLE|ACT_MULTI|ACT_DIR, 1 },
	{ "Copy",            NULL,                    'c', ACT_SINGLE|ACT_MULTI|ACT_DIR, 1 },
	{ "Paste",           NULL,                    'v', ACT_NOSEL,          1 },
	SEPARATOR,
	{ "Copy Path",       "echo -n %f | xclip -selection clipboard", 'p',
	                                                   ACT_SINGLE|ACT_DIR, 1 },
	{ "Copy Name",       "echo -n %n | xclip -selection clipboard",  0,
	                                                   ACT_SINGLE|ACT_DIR, 1 },
	SEPARATOR,
	{ "Rename",          NULL,                    'r', ACT_SINGLE|ACT_DIR, 1 },
	{ "Delete",          NULL,                  0x7F, ACT_SINGLE|ACT_MULTI|ACT_DIR, 1 },
	SEPARATOR,
	{ "New Folder",      NULL,                    'n', ACT_NOSEL,          1 },
	{ "New File",        NULL,                    'f', ACT_NOSEL,          1 },
	SEPARATOR,
	{ "Properties",      NULL,                    'i', ACT_SINGLE|ACT_DIR, 1 },
	{ NULL, NULL, 0, 0, 0 } /* table terminator */
};

/* ════════════════════════════════════════════════════════════════
 * DIRECTORY ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_dir[] = {
	{ "Open in Terminal", "TERM cd %f && $SHELL", 't', ACT_SINGLE|ACT_DIR, 1 },
	{ "Open New Window",  "BG xfm %f",            0,  ACT_SINGLE|ACT_DIR, 1 },
	SEPARATOR,
	{ "Compress…",       "BG xarchiver %s",        0,  ACT_SINGLE|ACT_MULTI|ACT_DIR, 1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * IMAGE ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_image[] = {
	{ "View",            "BG imv %s",             'v', ACT_SINGLE|ACT_MULTI, 1 },
	{ "Edit",            "BG gimp %f",            'e', ACT_SINGLE,           1 },
	{ "Slideshow",       "BG imv -s random %s",   's', ACT_MULTI,            1 },
	SEPARATOR,
	{ "Convert to PNG",  "ffmpeg -i %f %f.png",   0,   ACT_SINGLE,           1 },
	{ "Convert to JPG",  "ffmpeg -i %f %f.jpg",   0,   ACT_SINGLE,           1 },
	{ "Convert to WebP", "ffmpeg -i %f %f.webp",  0,   ACT_SINGLE,           1 },
	SEPARATOR,
	{ "Set as Wallpaper","BG feh --bg-scale %f",  0,   ACT_SINGLE,           1 },
	{ "Rotate 90° CW",   "mogrify -rotate 90 %f", 0,   ACT_SINGLE|ACT_MULTI, 1 },
	{ "Rotate 90° CCW",  "mogrify -rotate -90 %f",0,   ACT_SINGLE|ACT_MULTI, 1 },
	{ "Show Metadata",   "TERM exiftool %f | less",0,   ACT_SINGLE,           1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * AUDIO ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_audio[] = {
	{ "Play",            "BG mpv --no-video %s",  'p', ACT_SINGLE|ACT_MULTI, 1 },
	{ "Enqueue",         "BG mpv --no-video --playlist=- <<< %s",
	                                               'q', ACT_MULTI,            0 },
	SEPARATOR,
	{ "Convert to MP3",  "ffmpeg -i %f -q:a 0 -map a %f.mp3", 0, ACT_SINGLE, 1 },
	{ "Convert to FLAC", "ffmpeg -i %f %f.flac",  0,   ACT_SINGLE,           1 },
	{ "Convert to Opus", "ffmpeg -i %f -c:a libopus %f.opus",  0, ACT_SINGLE, 1 },
	SEPARATOR,
	{ "Show Tags",       "TERM ffprobe -v quiet -print_format json -show_format %f | less",
	                                               0,   ACT_SINGLE,           1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * VIDEO ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_video[] = {
	{ "Play",            "BG mpv %s",             'p', ACT_SINGLE|ACT_MULTI, 1 },
	{ "Play (no audio)", "BG mpv --no-audio %f",  0,   ACT_SINGLE,           1 },
	SEPARATOR,
	{ "Extract Audio",   "ffmpeg -i %f -vn -acodec copy %f.mka", 0, ACT_SINGLE, 1 },
	{ "Extract Frame",   "ffmpeg -i %f -vframes 1 %f_thumb.png", 0, ACT_SINGLE, 1 },
	{ "Convert to MP4",  "ffmpeg -i %f -c:v libx264 -preset slow -crf 22 -c:a aac %f.mp4",
	                                               0,   ACT_SINGLE,           1 },
	{ "Convert to WebM", "ffmpeg -i %f -c:v libvpx-vp9 -crf 30 -b:v 0 -c:a libopus %f.webm",
	                                               0,   ACT_SINGLE,           1 },
	SEPARATOR,
	{ "Show Info",       "TERM ffprobe -v quiet -print_format json -show_streams %f | less",
	                                               0,   ACT_SINGLE,           1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * DOCUMENT ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_document[] = {
	{ "Open",            "BG xdg-open %f",        'o', ACT_SINGLE,           1 },
	{ "Open in Zathura", "BG zathura %f",         'z', ACT_SINGLE,           1 },
	{ "Open in Evince",  "BG evince %f",           0,  ACT_SINGLE,           1 },
	SEPARATOR,
	{ "Print",           "lp %f",                  0,  ACT_SINGLE,           1 },
	{ "Extract Text",    "TERM pdftotext %f - | less", 0, ACT_SINGLE,        1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * ARCHIVE ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_archive[] = {
	{ "Extract Here",    "BG atool -x %f",         'e', ACT_SINGLE,          1 },
	{ "Extract To…",     "BG atool -x -O %f",      0,   ACT_SINGLE,          1 },
	{ "List Contents",   "TERM atool -l %f | less", 'l', ACT_SINGLE,         1 },
	SEPARATOR,
	{ "Open in Xarchiver","BG xarchiver %f",        0,   ACT_SINGLE,         1 },
	SEPARATOR,
	/*
	 * Quick-compress: select files, right-click → one of these.
	 * %s expands to all selected paths.
	 */
	{ "Compress to .tar.gz",
	                     "tar czf archive.tar.gz %s", 0, ACT_SINGLE|ACT_MULTI, 1 },
	{ "Compress to .zip","zip archive.zip %s",     0,   ACT_SINGLE|ACT_MULTI, 1 },
	{ "Compress to .7z", "7z a archive.7z %s",     0,   ACT_SINGLE|ACT_MULTI, 1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * CODE / TEXT ACTIONS  (used by FT_CODE and FT_TEXT)
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_text[] = {
	{ "Edit in $EDITOR", "TERM $EDITOR %f",        'e', ACT_SINGLE,          1 },
	{ "Edit in Vim",     "TERM vim %f",            'V', ACT_SINGLE,          1 },
	{ "Edit in Nano",    "TERM nano %f",            0,  ACT_SINGLE,          0 },
	{ "View",            "TERM less %f",           'v', ACT_SINGLE,          1 },
	SEPARATOR,
	{ "Word Count",      "TERM wc %f | less",       0,  ACT_SINGLE|ACT_MULTI,1 },
	{ "Diff (2 files)",  "TERM diff %s | less",     0,  ACT_MULTI,           1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * FONT ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_font[] = {
	{ "Preview",         "BG fontpreview %f",       0,  ACT_SINGLE,          1 },
	{ "Install (user)",
	  "cp %f ~/.local/share/fonts/ && fc-cache -f", 0,  ACT_SINGLE|ACT_MULTI,1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * DISK IMAGE ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_disk[] = {
	{ "Mount",           "BG udisksctl loop-setup -f %f", 'm', ACT_SINGLE,   1 },
	{ "Open in Brasero", "BG brasero -i %f",         0,   ACT_SINGLE,        0 },
	{ "Burn to Disc",    "BG growisofs -dvd-compat -Z /dev/dvdrw=%f",
	                                                  0,   ACT_SINGLE,        0 },
	{ "Checksum (SHA256)","TERM sha256sum %f",         0,   ACT_SINGLE,       1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * EXECUTABLE ACTIONS
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_exec[] = {
	{ "Run",             "BG %f",                  'r', ACT_SINGLE,          1 },
	{ "Run in Terminal", "TERM %f",                't', ACT_SINGLE,          1 },
	{ "Make Executable", "chmod +x %f",             0,  ACT_SINGLE,          1 },
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * CUSTOM GROUP  (FT_CUSTOM – add your own patterns in filetypes.h)
 * ════════════════════════════════════════════════════════════════ */
static const MenuAction actions_custom[] = {
	/*
	 * Add entries for your custom file types here.
	 * Example for Blender files:
	 *   { "Open in Blender", "BG blender %f", 'b', ACT_SINGLE, 1 },
	 */
	{ NULL, NULL, 0, 0, 0 }
};

/* ════════════════════════════════════════════════════════════════
 * GROUP REGISTRY  –  maps FileType → action table
 * Edit the .enabled field to disable an entire group at once.
 * ════════════════════════════════════════════════════════════════ */
#include "filetypes.h"   /* for FT_COUNT, FileType */

typedef struct {
	FileType            type;
	const MenuAction   *actions;
	int                 enabled;  /* 0 = never show this group */
} ActionGroup;

static const ActionGroup action_groups[] = {
	{ FT_DIR,      actions_dir,      1 },
	{ FT_IMAGE,    actions_image,    1 },
	{ FT_AUDIO,    actions_audio,    1 },
	{ FT_VIDEO,    actions_video,    1 },
	{ FT_DOCUMENT, actions_document, 1 },
	{ FT_ARCHIVE,  actions_archive,  1 },
	{ FT_CODE,     actions_text,     1 },
	{ FT_TEXT,     actions_text,     1 },
	{ FT_FONT,     actions_font,     1 },
	{ FT_DISK,     actions_disk,     1 },
	{ FT_EXEC,     actions_exec,     1 },
	{ FT_CUSTOM,   actions_custom,   1 },
	/* terminator */
	{ FT_UNKNOWN,  NULL,             0 },
};

#endif /* ACTIONS_H */
