/*
 * nspire/nspire-compat.h
 *
 * Included in every translation unit via  -include nspire/nspire-compat.h
 * so we can paper over small ABI/API differences between Ndless newlib and
 * the POSIX environment that Angband normally targets.
 *
 * Keep this file lean: only put fixes that are genuinely global here.
 */

#ifndef NSPIRE_COMPAT_H
#define NSPIRE_COMPAT_H

/*
 * Intercept fopen() to transparently append ".tns" to every path.
 * nspire-fopen.c undefs these macros before including <stdio.h> so its
 * own function bodies reach the real libc calls.
 */
#include <stdio.h>
FILE *nspire_fopen(const char *path, const char *mode);
#define fopen(p, m) nspire_fopen((p), (m))

/*
 * Intercept rename() and remove() so save-file rotation (which creates
 * temp files without .tns suffixes) works on the Nspire OS.
 */
int nspire_rename(const char *oldpath, const char *newpath);
int nspire_remove(const char *path);
#define rename(o, n) nspire_rename((o), (n))
#define remove(p)    nspire_remove(p)

#endif /* NSPIRE_COMPAT_H */
