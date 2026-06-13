/*
 * contextmenu/filetypes.h
 *
 * Maps filename glob patterns to a filetype enum.
 * Edit this file to add, remove, or reassign patterns.
 *
 * FORMAT
 *   FTPATT(FILETYPE_ENUM_VALUE, "glob_pattern")
 *
 * Rules:
 *   • Patterns are tested in order; first match wins.
 *   • Globs are case-insensitive (FNM_CASEFOLD).
 *   • "*" matches any file that didn't match above it.
 *   • Directories are detected by mode bits before this table is
 *     consulted, so you do not need a dir pattern here.
 *
 * Add your own types by:
 *   1. Adding a new FT_xxx value to the FileType enum below.
 *   2. Adding FTPATT(FT_xxx, "*.ext") lines here.
 *   3. Adding an action group for FT_xxx in actions.h.
 */

#ifndef FILETYPES_H
#define FILETYPES_H

/* ── Filetype enum ───────────────────────────────────────────── */
typedef enum {
	FT_UNKNOWN = 0,
	FT_DIR,         /* directory (detected via mode, not pattern) */
	FT_IMAGE,
	FT_AUDIO,
	FT_VIDEO,
	FT_DOCUMENT,
	FT_ARCHIVE,
	FT_CODE,
	FT_TEXT,
	FT_FONT,
	FT_DISK,        /* disk images: iso, img, … */
	FT_EXEC,        /* executable / script */
	FT_CUSTOM,      /* user-defined catch-all before UNKNOWN */
	FT_COUNT,       /* sentinel – keep last */
} FileType;

/* ── Human-readable type names (indexed by FileType) ─────────── */
static const char *const ft_names[FT_COUNT] = {
	[FT_UNKNOWN]  = "Unknown",
	[FT_DIR]      = "Directory",
	[FT_IMAGE]    = "Image",
	[FT_AUDIO]    = "Audio",
	[FT_VIDEO]    = "Video",
	[FT_DOCUMENT] = "Document",
	[FT_ARCHIVE]  = "Archive",
	[FT_CODE]     = "Code",
	[FT_TEXT]     = "Text",
	[FT_FONT]     = "Font",
	[FT_DISK]     = "Disk Image",
	[FT_EXEC]     = "Executable",
	[FT_CUSTOM]   = "Custom",
};

/* ── Pattern table ───────────────────────────────────────────── */
/*
 * Each row: { FileType, "glob" }
 * Tested against the bare filename (not the full path).
 */
typedef struct {
	FileType    type;
	const char *pattern;
} FtPatt;

static const FtPatt ft_patterns[] = {
	/* ── Images ─────────────────────────────────────────── */
	{ FT_IMAGE,    "*.jpg"   },
	{ FT_IMAGE,    "*.jpeg"  },
	{ FT_IMAGE,    "*.png"   },
	{ FT_IMAGE,    "*.gif"   },
	{ FT_IMAGE,    "*.bmp"   },
	{ FT_IMAGE,    "*.tiff"  },
	{ FT_IMAGE,    "*.tif"   },
	{ FT_IMAGE,    "*.webp"  },
	{ FT_IMAGE,    "*.avif"  },
	{ FT_IMAGE,    "*.heic"  },
	{ FT_IMAGE,    "*.heif"  },
	{ FT_IMAGE,    "*.svg"   },
	{ FT_IMAGE,    "*.ico"   },
	{ FT_IMAGE,    "*.ppm"   },
	{ FT_IMAGE,    "*.pgm"   },
	{ FT_IMAGE,    "*.pbm"   },
	{ FT_IMAGE,    "*.pnm"   },
	{ FT_IMAGE,    "*.xpm"   },
	{ FT_IMAGE,    "*.raw"   },
	{ FT_IMAGE,    "*.cr2"   },
	{ FT_IMAGE,    "*.nef"   },
	{ FT_IMAGE,    "*.arw"   },

	/* ── Audio ───────────────────────────────────────────── */
	{ FT_AUDIO,    "*.mp3"   },
	{ FT_AUDIO,    "*.flac"  },
	{ FT_AUDIO,    "*.ogg"   },
	{ FT_AUDIO,    "*.opus"  },
	{ FT_AUDIO,    "*.wav"   },
	{ FT_AUDIO,    "*.aac"   },
	{ FT_AUDIO,    "*.m4a"   },
	{ FT_AUDIO,    "*.wma"   },
	{ FT_AUDIO,    "*.aiff"  },
	{ FT_AUDIO,    "*.ape"   },
	{ FT_AUDIO,    "*.mka"   },

	/* ── Video ───────────────────────────────────────────── */
	{ FT_VIDEO,    "*.mp4"   },
	{ FT_VIDEO,    "*.mkv"   },
	{ FT_VIDEO,    "*.avi"   },
	{ FT_VIDEO,    "*.mov"   },
	{ FT_VIDEO,    "*.webm"  },
	{ FT_VIDEO,    "*.flv"   },
	{ FT_VIDEO,    "*.wmv"   },
	{ FT_VIDEO,    "*.m4v"   },
	{ FT_VIDEO,    "*.mpg"   },
	{ FT_VIDEO,    "*.mpeg"  },
	{ FT_VIDEO,    "*.ts"    },
	{ FT_VIDEO,    "*.vob"   },
	{ FT_VIDEO,    "*.ogv"   },

	/* ── Documents ───────────────────────────────────────── */
	{ FT_DOCUMENT, "*.pdf"   },
	{ FT_DOCUMENT, "*.djvu"  },
	{ FT_DOCUMENT, "*.ps"    },
	{ FT_DOCUMENT, "*.eps"   },
	{ FT_DOCUMENT, "*.doc"   },
	{ FT_DOCUMENT, "*.docx"  },
	{ FT_DOCUMENT, "*.odt"   },
	{ FT_DOCUMENT, "*.rtf"   },
	{ FT_DOCUMENT, "*.xls"   },
	{ FT_DOCUMENT, "*.xlsx"  },
	{ FT_DOCUMENT, "*.ods"   },
	{ FT_DOCUMENT, "*.ppt"   },
	{ FT_DOCUMENT, "*.pptx"  },
	{ FT_DOCUMENT, "*.odp"   },
	{ FT_DOCUMENT, "*.epub"  },
	{ FT_DOCUMENT, "*.mobi"  },
	{ FT_DOCUMENT, "*.md"  },

	/* ── Archives ────────────────────────────────────────── */
	{ FT_ARCHIVE,  "*.tar"   },
	{ FT_ARCHIVE,  "*.gz"    },
	{ FT_ARCHIVE,  "*.bz2"   },
	{ FT_ARCHIVE,  "*.xz"    },
	{ FT_ARCHIVE,  "*.zst"   },
	{ FT_ARCHIVE,  "*.lz4"   },
	{ FT_ARCHIVE,  "*.lzma"  },
	{ FT_ARCHIVE,  "*.tgz"   },
	{ FT_ARCHIVE,  "*.tbz2"  },
	{ FT_ARCHIVE,  "*.txz"   },
	{ FT_ARCHIVE,  "*.zip"   },
	{ FT_ARCHIVE,  "*.7z"    },
	{ FT_ARCHIVE,  "*.rar"   },
	{ FT_ARCHIVE,  "*.lha"   },
	{ FT_ARCHIVE,  "*.cab"   },
	{ FT_ARCHIVE,  "*.deb"   },
	{ FT_ARCHIVE,  "*.rpm"   },
	{ FT_ARCHIVE,  "*.apk"   },
	{ FT_ARCHIVE,  "*.jar"   },
	{ FT_ARCHIVE,  "*.war"   },
	{ FT_ARCHIVE,  "*.zip"   },

	/* ── Source code ─────────────────────────────────────── */
	{ FT_CODE,     "*.c"     },
	{ FT_CODE,     "*.h"     },
	{ FT_CODE,     "*.cc"    },
	{ FT_CODE,     "*.cpp"   },
	{ FT_CODE,     "*.cxx"   },
	{ FT_CODE,     "*.hh"    },
	{ FT_CODE,     "*.hpp"   },
	{ FT_CODE,     "*.rs"    },
	{ FT_CODE,     "*.go"    },
	{ FT_CODE,     "*.py"    },
	{ FT_CODE,     "*.rb"    },
	{ FT_CODE,     "*.js"    },
	{ FT_CODE,     "*.ts"    },
	{ FT_CODE,     "*.lua"   },
	{ FT_CODE,     "*.java"  },
	{ FT_CODE,     "*.kt"    },
	{ FT_CODE,     "*.swift" },
	{ FT_CODE,     "*.zig"   },
	{ FT_CODE,     "*.ml"    },
	{ FT_CODE,     "*.hs"    },
	{ FT_CODE,     "*.sh"    },
	{ FT_CODE,     "*.bash"  },
	{ FT_CODE,     "*.zsh"   },
	{ FT_CODE,     "*.fish"  },
	{ FT_CODE,     "*.html"  },
	{ FT_CODE,     "*.htm"   },
	{ FT_CODE,     "*.css"   },
	{ FT_CODE,     "*.xml"   },
	{ FT_CODE,     "*.json"  },
	{ FT_CODE,     "*.toml"  },
	{ FT_CODE,     "*.yaml"  },
	{ FT_CODE,     "*.yml"   },
	{ FT_CODE,     "Makefile"},
	{ FT_CODE,     "*.mk"    },
	{ FT_CODE,     "*.cmake" },
	{ FT_CODE,     "*.nix"   },

	/* ── Plain text ──────────────────────────────────────── */
	{ FT_TEXT,     "*.txt"   },
	{ FT_TEXT,     "*.md"    },
	{ FT_TEXT,     "*.rst"   },
	{ FT_TEXT,     "*.org"   },
	{ FT_TEXT,     "*.csv"   },
	{ FT_TEXT,     "*.log"   },
	{ FT_TEXT,     "*.conf"  },
	{ FT_TEXT,     "*.cfg"   },
	{ FT_TEXT,     "*.ini"   },
	{ FT_TEXT,     "*.env"   },

	/* ── Fonts ───────────────────────────────────────────── */
	{ FT_FONT,     "*.ttf"   },
	{ FT_FONT,     "*.otf"   },
	{ FT_FONT,     "*.woff"  },
	{ FT_FONT,     "*.woff2" },
	{ FT_FONT,     "*.eot"   },
	{ FT_FONT,     "*.pcf"   },
	{ FT_FONT,     "*.bdf"   },

	/* ── Disk images ─────────────────────────────────────── */
	{ FT_DISK,     "*.iso"   },
	{ FT_DISK,     "*.img"   },
	{ FT_DISK,     "*.bin"   },
	{ FT_DISK,     "*.dmg"   },
	{ FT_DISK,     "*.qcow2" },
	{ FT_DISK,     "*.vmdk"  },
	{ FT_DISK,     "*.vhd"   },
	{ FT_DISK,     "*.vhdx"  },

	/* ── Executables / scripts ───────────────────────────── */
	{ FT_EXEC,     "*.AppImage" },
	{ FT_EXEC,     "*.run"      },
	{ FT_EXEC,     "*.exe"      },  /* Wine users */
	{ FT_EXEC,     "*.bat"      },

	/*
	 * ── Custom user entries ───────────────────────────────
	 * Add your own here before the terminator.
	 * Example:
	 *   { FT_CUSTOM, "*.blend"  },
	 *   { FT_CUSTOM, "*.kra"    },
	 */

	/* terminator — must be last */
	{ FT_UNKNOWN,  NULL }
};

#endif /* FILETYPES_H */
