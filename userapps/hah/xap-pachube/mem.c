/* $Id$
   Copyright (c) Brett England, 2009
   
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mem.h"
#include "log.h"

void *mem_malloc(size_t size, int flags)
{
	void *ptr = malloc(size);

	die_if(ptr == NULL, "ERROR: Unable to allocate memory, aborting.");
	if (flags & M_ZERO)
		memset(ptr, 0, size);

	return ptr;
}

void *mem_realloc(void *ptr, size_t size, int flags)
{
	int change = (size - ((ptr == NULL) ? 0 : sizeof(*ptr)));

	ptr = realloc(ptr, size);
	die_if(ptr == NULL, "Unable to realloc() memory.");

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

void *mem_strdup(void *ptr, int flags)
{
	return mem_dup(ptr, strlen(ptr)+1, flags);
}

void mem_free(void *ptr)
{
	free(ptr);
}
