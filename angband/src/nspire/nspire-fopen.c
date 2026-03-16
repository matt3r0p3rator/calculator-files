/**
 * \file nspire/nspire-fopen.c
 * \brief Transparent .tns suffix for all file I/O on the Nspire.
 *
 * The TI-Nspire OS only allows files whose names end in ".tns".  Every
 * fopen(), rename(), and remove() call is routed through wrappers that
 * ensure the path always ends in ".tns".
 *
 * For READ mode  : try exact path first, then path+".tns".
 * For WRITE/APPEND: always use path+".tns" (the OS refuses other names).
 *
 * On-calculator file layout:
 *   /documents/angband/lib/gamedata/monster.txt.tns
 *   /documents/angband/lib/save/PLAYER.tns
 */

/* Undef our own macros so function bodies call real libc. */
#undef fopen
#undef rename
#undef remove

#include <stdio.h>
#include <string.h>

#define TNS      ".tns"
#define TNS_LEN  4

static int has_tns(const char *path, size_t len)
{
	return len >= TNS_LEN && strcmp(path + len - TNS_LEN, TNS) == 0;
}

static int tns_path(char *buf, size_t bufsz, const char *path, size_t len)
{
	if (has_tns(path, len)) {
		if (len + 1 > bufsz) return 0;
		memcpy(buf, path, len + 1);
	} else {
		if (len + TNS_LEN + 1 > bufsz) return 0;
		memcpy(buf, path, len);
		memcpy(buf + len, TNS, TNS_LEN + 1);
	}
	return 1;
}

FILE *nspire_fopen(const char *path, const char *mode)
{
	char buf[1024];
	size_t len = strlen(path);

	if (mode[0] == 'r') {
		/* Try exact path first, then with .tns appended. */
		FILE *f = fopen(path, mode);
		if (f) return f;
		if (!tns_path(buf, sizeof(buf), path, len)) return NULL;
		return fopen(buf, mode);
	} else {
		/* Write/append: always use the .tns path. */
		if (!tns_path(buf, sizeof(buf), path, len)) return NULL;
		return fopen(buf, mode);
	}
}

int nspire_rename(const char *oldpath, const char *newpath)
{
	char oldbuf[1024], newbuf[1024];
	size_t olen = strlen(oldpath), nlen = strlen(newpath);
	if (!tns_path(oldbuf, sizeof(oldbuf), oldpath, olen)) return -1;
	if (!tns_path(newbuf, sizeof(newbuf), newpath, nlen)) return -1;
	return rename(oldbuf, newbuf);
}

int nspire_remove(const char *path)
{
	char buf[1024];
	size_t len = strlen(path);
	/* Try plain path first (in case it somehow exists), then .tns. */
	if (remove(path) == 0) return 0;
	if (!tns_path(buf, sizeof(buf), path, len)) return -1;
	return remove(buf);
}

