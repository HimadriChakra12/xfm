/* xfm config.h - edit this file to configure xfm at compile time */

#ifndef CONFIG_H
#define CONFIG_H

/* ── Application identity ─────────────────────────────────────── */
#define APPCLASS        "Xfm"
#define APPNAME         "xfm"

/* ── External commands ────────────────────────────────────────── */
/* Program used to open files (overridden by $OPENER at runtime) */
#define DEF_OPENER      "xdg-open"

/* Context-menu / file-operation controller script */
#define CONTEXTCMD      "xfilesctl"

/* Thumbnailing helper (receives orig-path thumb-path) */
#define THUMBNAILERCMD  "xfilesthumb"

/* ── Drag-and-drop action strings (passed to CONTEXTCMD) ──────── */
#define DROPCOPY        "drop-copy"
#define DROPMOVE        "drop-move"
#define DROPLINK        "drop-link"
#define DROPASK         "drop-ask"
#define MENU            "menu"

/* ── Internal limits ──────────────────────────────────────────── */
/* Maximum -X resource strings on the command line (last slot is NULL) */
#define MAX_RESOURCES   32

/* Status-bar format buffer size in bytes */
#define STATUS_BUFSIZE  1024

/* Chunk size when growing the external-drop argv array */
#define INCRSIZE        512

/* ── Widget geometry (pixels) ─────────────────────────────────── */
/* Maximum thumbnail size; also controls icon-cell proportions */
#define THUMBSIZE       64

/* Margin around an item's icon (= THUMBSIZE / 2) */
#define ICON_MARGIN     (THUMBSIZE / 2)

/* Total width of one item cell */
#define ITEM_WIDTH      (THUMBSIZE * 2)

/* Top margin above the first row */
#define MARGIN          16

/* Maximum label lines rendered below each icon */
#define NLINES          2

/* Maximum pixel width of a label line (saves 8 px each side) */
#define LABELWIDTH      (ITEM_WIDTH - 16)

/* ── Timing (milliseconds) ────────────────────────────────────── */
/* Maximum gap between two clicks to count as a double-click */
#define DOUBLECLICK     250

/* Scroll animation interval */
#define SCROLL_TIME     128

/* ── Scrolling ────────────────────────────────────────────────── */
/* Pixels moved per scroll step */
#define SCROLL_STEP     32

/* Height/width of the scrollbar track */
#define SCROLLER_SIZE   32

/* Minimum line-count change before the scroll handle moves */
#define SCROLLER_MIN    16

/* Maximum size of the scroll handle */
#define HANDLE_MAX_SIZE (SCROLLER_SIZE - 4)

/* ── Misc ─────────────────────────────────────────────────────── */
/* Path used when stdout/stdin must be redirected to nothing */
#define DEV_NULL        "/dev/null"

/* Prefix stripped from drag-and-drop URIs */
#define URI_PREFIX      "file://"

/* Number of human-readable size units (B K M G T P E) */
#define UNIT_LAST       7

#endif /* CONFIG_H */
