/**
 * \file nspire/nspire-mem-alt.c
 * \brief Provide the "alt" allocator stubs required when -DNDS is defined.
 *
 * z-virt.h declares mem_alloc_alt / mem_zalloc_alt / mem_free_alt /
 * mem_realloc_alt / mem_is_alt_alloc as real functions (not macros) whenever
 * the NDS preprocessor symbol is defined.  The NDS port uses them to route
 * certain allocations into external RAM; on the TI-Nspire CX II all RAM is
 * homogeneous, so these simply delegate to the standard Angband allocators.
 */

#include "z-virt.h"

void *mem_alloc_alt(size_t n)
{
	return mem_alloc(n);
}

void *mem_zalloc_alt(size_t n)
{
	return mem_zalloc(n);
}

void mem_free_alt(void *p)
{
	mem_free(p);
}

void *mem_realloc_alt(void *p, size_t n)
{
	return mem_realloc(p, n);
}

bool mem_is_alt_alloc(void *p)
{
	(void)p;
	return false;
}
