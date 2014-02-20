#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mem.h"
#include "log.h"

void *mem_malloc(size_t size, int flags)
{
	void *ptr;

	if ((ptr = malloc(size)) == NULL) {
		die("Unable to allocate memory, aborting.\n");
	}

	if (flags & M_ZERO)
		memset(ptr, 0, size);

	return ptr;
}

void *mem_realloc(void *ptr, size_t size, int flags)
{
	int change = (size - ((ptr == NULL) ? 0 : sizeof(*ptr)));

	if ((ptr = realloc(ptr, size)) == NULL) {
		die("Unable to realloc() memory.\n");
	}

	if ((flags & M_ZERO) && (change > 0))
		memset(ptr + (size - change), 0, change);

	return ptr;
}

void *mem_dup(void *ptr, size_t size, int flags)
{
	void *copy;

	copy = mem_malloc(size, flags);
	bcopy(ptr, copy, size);

	return copy;
}

void *mem_strdup(void *ptr)
{
	return mem_dup(ptr, strlen(ptr)+1, M_NONE);
}

void mem_free(void *ptr)
{
	free(ptr);
}
