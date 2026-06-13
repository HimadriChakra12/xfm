/*
 * contextmenu/menuconfig.h
 *
 * Visual and behavioural knobs for the context menu popup.
 * Every value here overrides the corresponding default in menu.c.
 * Add   #include "contextmenu/menuconfig.h"   before menu.h
 * if you want these to take effect; or just edit menu.c defaults.
 *
 * All sizes are in pixels.  Colours are X11 colour names or #RRGGBB.
 */

#ifndef MENUCONFIG_H
#define MENUCONFIG_H

/* ── Geometry ─────────────────────────────────────────────────── */

/* Pixel height of each normal (non-separator) item row */
#define CTX_ITEM_HEIGHT     22

/* Pixel height of a separator rule */
#define CTX_SEP_HEIGHT       7

/* Horizontal text padding (left and right) */
#define CTX_HPAD            12

/* Minimum width of the popup (grows to fit labels) */
#define CTX_MIN_WIDTH       180

/* Border thickness in pixels (0 = no border) */
#define CTX_BORDER           1

/* Gap between the label and the keyboard-shortcut hint */
#define CTX_SHORTCUT_GAP    24

/* ── Font ─────────────────────────────────────────────────────── */

/* Xft font name.  Falls back to "monospace" if not found. */
#define CTX_FONT_FACE       "JetBrains Mono"

/* Font size in points */
#define CTX_FONT_SIZE       11

/* ── Colours ──────────────────────────────────────────────────── */
/* Any X11 colour name or #RRGGBB hex string works.              */

/* Normal item background */
#define CTX_COLOR_BG        "#1d2021"

/* Normal item foreground (text) */
#define CTX_COLOR_FG        "#e9dab1"

/* Highlighted / hovered item background */
#define CTX_COLOR_SEL_BG    "#3c3836"

/* Highlighted / hovered item foreground */
#define CTX_COLOR_SEL_FG    "#fbf1c7"

/* Border colour */
#define CTX_COLOR_BORDER    "#504945"

/* Separator line colour */
#define CTX_COLOR_SEP       "#3c3836"

/* Disabled / keyboard-hint dim colour */
#define CTX_COLOR_DIM       "#665c54"

#endif /* MENUCONFIG_H */
