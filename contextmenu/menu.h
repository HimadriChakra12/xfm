/*
 * contextmenu/menu.h
 *
 * Public interface to the context menu system.
 *
 * The menu is drawn as a native X11 popup window — no external
 * helper required for the standard entries.  All customisation
 * lives in filetypes.h and actions.h.
 */

#ifndef MENU_H
#define MENU_H

#include "../src/fm.h"
#include "filetypes.h"
#include "actions.h"

/*
 * ctx_show() – detect filetype of the selected items and show the
 * appropriate context menu popup at the given screen coordinates.
 *
 *   fm        – core FM state (for entries, cwd, widget)
 *   selitems  – array of selected entry indices (fm->selitems)
 *   nsel      – number of selected items (0 = blank-area click)
 *   root_x    – X position (relative to root window) for the popup
 *   root_y    – Y position (relative to root window) for the popup
 *
 * Returns 1 if a directory-modifying action was executed and the
 * caller should refresh the directory; 0 otherwise.
 */
int ctx_show(struct FM *fm, int *selitems, int nsel,
             int root_x, int root_y);

/*
 * ctx_detect_type() – return the FileType for a given filename.
 * Useful for callers that want to query the type without showing
 * the menu (e.g. for status-bar icons or key-bound shortcuts).
 */
FileType ctx_detect_type(const char *filename, unsigned char mode);

/*
 * ctx_run_action() – execute a specific action by index on the
 * currently selected files.  Used by key-binding dispatch in
 * main.c when a shortcut bypasses the popup.
 *
 *   fm        – core FM state
 *   selitems  – array of selected entry indices
 *   nsel      – number of selected items
 *   action    – pointer to the MenuAction to execute
 *
 * Returns 1 if the directory should be refreshed afterwards.
 */
int ctx_run_action(struct FM *fm, int *selitems, int nsel,
                   const MenuAction *action);

#endif /* MENU_H */
