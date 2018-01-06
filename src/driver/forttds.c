/* Fort Firewall TommyDS */

#define FORT_TOMMY_POOL_TAG	'TwfF'

#include <assert.h>

#define fort_mem_alloc(size, tag)	ExAllocatePoolWithTag(NonPagedPool, (size), (tag))
#define fort_mem_free(p, tag)		ExFreePoolWithTag((p), (tag))

static PVOID
fort_tommy_malloc (SIZE_T size)
{
  return fort_mem_alloc(size, FORT_TOMMY_POOL_TAG);
}

static void
fort_tommy_free (PVOID p)
{
  fort_mem_free(p, FORT_TOMMY_POOL_TAG);
}

static PVOID
fort_tommy_calloc (SIZE_T count, SIZE_T element_size)
{
  const SIZE_T size = count * element_size;
  PVOID p = fort_tommy_malloc(size);

  if (p != NULL) {
    memset(p, 0, size);
  }
  return p;
}

static PVOID
fort_tommy_realloc (PVOID p, SIZE_T new_size)
{
  UNUSED(p);
  UNUSED(new_size);

  assert(FALSE);  /* not used - not implemented */

  return NULL;
}

#define tommy_malloc	fort_tommy_malloc
#define tommy_free	fort_tommy_free
#define tommy_calloc	fort_tommy_calloc
#define tommy_realloc	fort_tommy_realloc

#include "3rdparty\tommyds\tommyarrayof.c"
#include "3rdparty\tommyds\tommylist.c"
#include "3rdparty\tommyds\tommyhash.c"
#include "3rdparty\tommyds\tommyhashdyn.c"

