/*
 * nspire/syscall-decls.h
 *
 * Shadow wrapper for the Ndless SDK's <syscall-decls.h>.
 *
 * Problem:
 *   syscall-decls.h is included both by the SDK's <dirent.h> (indirectly)
 *   AND directly by <os.h>, which main-nspire.c includes before angband.h.
 *   It contains:
 *       void string_free(String p1);   // String = internal Ndless type
 *   This conflicts with Angband's z-virt.h which declares:
 *       void string_free(char *str);
 *
 * Fix:
 *   Use GCC's #include_next to delegate to the real syscall-decls.h, but
 *   bracket it with a rename so the SDK's declaration lands under a private
 *   name.  Because -I nspire comes before the SDK include paths, the
 *   compiler finds this wrapper first for any  #include <syscall-decls.h>.
 */

#ifndef _NSPIRE_SYSCALL_DECLS_WRAPPER_H
#define _NSPIRE_SYSCALL_DECLS_WRAPPER_H

/* Rename the SDK's string_free before the conflicting declaration is parsed. */
#define string_free  __ndless_string_free_internal

#include_next <syscall-decls.h>   /* delegate to the real SDK header */

/* Restore the name so Angband's z-virt.h / z-virt.c are unaffected. */
#undef string_free

#endif /* _NSPIRE_SYSCALL_DECLS_WRAPPER_H */
