/* xfm config.h - edit this file to configure xfm at compile time */

#ifndef CONFIG_H
#define CONFIG_H

#define APPCLASS        "Xfm"
#define APPNAME         "xfm"
#define DEF_OPENER      "xdg-open"
#define CONTEXTCMD      "xfilesctl"
#define THUMBNAILERCMD  "xfilesthumb"

#define DROPCOPY        "drop-copy"
#define DROPMOVE        "drop-move"
#define DROPLINK        "drop-link"
#define DROPASK         "drop-ask"
#define MENU            "menu"

/* Internal limits */
/* Maximum -X resource strings on the command line (last slot is NULL) */
#define MAX_RESOURCES   32
#define STATUS_BUFSIZE  1024

/* Chunk size when growing the external-drop argv array */
#define INCRSIZE        512
#define THUMBSIZE       64
#define ICON_MARGIN     (THUMBSIZE / 2)     /* (THUMBSIZE / 2) */
#define ITEM_WIDTH      (THUMBSIZE * 2)     /* (THUMBSIZE / 2) */
#define MARGIN          16                  /* TOP MARGIN - 16 */
#define NLINES          2                   /* Label Lines - 2 */
#define LABELWIDTH      (ITEM_WIDTH - 16)   /* (ITEM_WIDTH - 16) */

/* Timing (milliseconds) */
#define DOUBLECLICK     250
#define SCROLL_TIME     128

/* Scrolling */
#define SCROLL_STEP     32
#define SCROLLER_SIZE   32
#define SCROLLER_MIN    16  /* line-count */
#define HANDLE_MAX_SIZE (SCROLLER_SIZE - 4)

/* Colors */
#define COLOR_NORMAL_BG "#1D2021"
#define COLOR_NORMAL_FG "#E9DAB1"
#define COLOR_SELECT_BG "#282828"
#define COLOR_SELECT_FG "#BAAD8F"

/* Window appearance */
#define DEF_OPACITY     1.0     /* (0.0 to 1.0, where 1.0 is fully opaque) */
#define DEF_WIDTH       600
#define DEF_HEIGHT      460

/* Fonts */
#define DEF_FONT_FACE   "Jetbrains Mono"
#define DEF_FONT_SIZE   0.0     /* (0.0 uses system default) */

#define DEF_STATUSBAR   1
#define STATUSBAR_HEIGHT_MULT   2
#define STATUSBAR_MARGIN_MULT   0.5

/* Busy cursor names to try (in order of preference) */
#define BUSY_CURSOR_NAMES { "progress", "half-busy", "left_ptr_watch" }
#define BUSY_CURSOR_COUNT 3
#define BUSY_CURSOR_FALLBACK XC_watch

#define DEV_NULL        "/dev/null"
#define URI_PREFIX      "file://"
#define UNIT_LAST       7   /* Number of human-readable size units (B K M G T P E) */
#define ELLIPSIS        ".."
#define BREAKABLE_CHARS ".-_"

/* Minimum distance (squared) for drag-and-drop to activate */
#define DND_THRESHOLD   64

/* ── X11 Visual preferences ────────────────────────────────────── */
/* Preferred color depth (falls back to default if unavailable) */
#define PREFERRED_DEPTH 32

/* Visual class to request */
#define PREFERRED_VISUAL TrueColor

/* ── Selection highlight ───────────────────────────────────────── */
#define SELECT_ALPHA    0xC000
#define RECTSEL_ALPHA   0x4000
#define HIGHLIGHT_OFFSET 0.5

/* ── Label rendering ───────────────────────────────────────────── */
#define LABEL_HPADDING  8
#define LABEL_VOFFSET   0.5

/* ── Thumbnail rendering ───────────────────────────────────────── */
#define PPM_MAX_COLOR   255
#define PPM_SIGNATURE   "P6"

#define STATUSBAR_HEIGHT(w) ((w)->fonth * STATUSBAR_HEIGHT_MULT)
#define STATUSBAR_MARGIN(w) ((w)->fonth * STATUSBAR_MARGIN_MULT)

#endif /* CONFIG_H */
