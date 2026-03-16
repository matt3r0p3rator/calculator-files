/*
 * nspire/dirent.h
 *
 * Shadow wrapper for the Ndless SDK's <dirent.h>.
 *
 * Problem:
 *   The Ndless SDK's dirent.h pulls in syscall-decls.h, which declares:
 *       void string_free(String p1);   // String = Ndless internal type
 *   Angband's z-virt.h independently declares:
 *       void string_free(char *str);
 *   The two signatures conflict and break compilation of z-file.c.
 *
 * Fix:
 *   Use GCC's #include_next to delegate to the real SDK dirent.h, but
 *   bracket that inclusion with a rename of string_free so the conflicting
 *   declaration lands under a private name that Angband code never sees.
 *
 *   Because -I nspire comes before the SDK include paths on the command line,
 *   the compiler finds this file first, allowing us to intercept the include.
 */

#ifndef _NSPIRE_DIRENT_WRAPPER_H
#define _NSPIRE_DIRENT_WRAPPER_H

/* Rename the SDK's string_free before the real header is parsed. */
#define string_free  __ndless_string_free_internal

#include_next <dirent.h>   /* delegate to the real SDK dirent.h */

/* Restore the name so that Angband's own z-virt.h / z-virt.c are unaffected. */
#undef string_free

#endif /* _NSPIRE_DIRENT_WRAPPER_H */
