#ifndef FORTTDS_H
#define FORTTDS_H

#include "fortdrv.h"

#define TOMMY_API FORT_API

#define tommy_assert NT_ASSERT

#define tommy_malloc  fort_tommy_malloc
#define tommy_free    fort_tommy_free
#define tommy_calloc  fort_tommy_calloc
#define tommy_realloc fort_tommy_realloc

FORT_API PVOID fort_tommy_malloc(SIZE_T size);

FORT_API void fort_tommy_free(PVOID p);

FORT_API PVOID fort_tommy_calloc(SIZE_T count, SIZE_T element_size);

FORT_API PVOID fort_tommy_realloc(PVOID p, SIZE_T new_size);

#include "../3rdparty/tommyds/tommyarrayof.h"
#include "../3rdparty/tommyds/tommylist.h"
#include "../3rdparty/tommyds/tommyhash.h"
#include "../3rdparty/tommyds/tommyhashdyn.h"

/* Inserts the node keeping its data field, which may hold the owner's fields */
inline static void fort_tommy_list_insert_tail(tommy_list *list, tommy_node *node)
{
    tommy_list_insert_tail(list, node, node->data);
}

/* Calls the function for each node: the data field may hold the owner's fields */
FORT_API void fort_tommy_hashdyn_foreach_node_arg(
        tommy_hashdyn *hashdyn, tommy_foreach_arg_func *func, void *arg);

#endif // FORTTDS_H
