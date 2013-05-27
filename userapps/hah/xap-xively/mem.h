/* $Id$
 */
#ifndef __MEM_H__
#define __MEM_H__

/* Memory allocation flags */
#define M_NONE			0x00	/* No options specified */
#define M_ZERO			0x01	/* Zero out the allocation */

void *mem_malloc(size_t, int);
void *mem_realloc(void *, size_t, int);
void *mem_dup(void *, size_t, int);
void *mem_strdup(void *, int);
void mem_free(void *);

#endif
