// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2010 Andrea Mazzoleni

/** \file
 * Dynamic chained hashtable.
 *
 * This hashtable resizes dynamically. It starts with the minimal size of 16 buckets.
 * It doubles the size when it reaches a load factor greater than or equal to 0.5
 * and it halves the size when the load factor is less than or equal to 0.125.
 *
 * All the elements are reallocated in a single resize operation done inside
 * tommy_hashdyn_insert() or tommy_hashdyn_remove().
 *
 * Note that the resize operation takes approximately 100 [ms] with 1 million elements,
 * and 1 [second] with 10 million elements. This could be a problem in real-time applications.
 *
 * The resize also fragments the heap, as it involves allocating a double-sized table, copying elements,
 * and deallocating the older table, leaving a big hole in the heap.
 *
 * The ::tommy_hashlin hashtable fixes both problems.
 *
 * To initialize the hashtable you have to call tommy_hashdyn_init().
 *
 * \code
 * tommy_hashdyn hashdyn;
 *
 * tommy_hashdyn_init(&hashdyn);
 * \endcode
 *
 * To insert elements in the hashtable you have to call tommy_hashdyn_insert() for
 * each element.
 * In the insertion call you have to specify the address of the node, the
 * address of the element, and the hash value of the key to use.
 * The address of the element is used to initialize the tommy_node::data field
 * of the node, and the hash to initialize the tommy_node::index field.
 *
 * \code
 * struct element {
 *     int value;
 *     // other fields
 *     tommy_node node;
 * };
 *
 * struct element* elem = malloc(sizeof(struct element)); // creates the element
 *
 * elem->value = ...; // initializes the element
 *
 * tommy_hashdyn_insert(&hashdyn, &elem->node, elem, tommy_inthash_u32(elem->value)); // inserts the element
 * \endcode
 *
 * To find an element in the hashtable you have to call tommy_hashdyn_search()
 * providing a comparison function, its argument, and the hash of the key to search.
 *
 * \code
 * int compare(const void* arg, const void* elem)
 * {
 *     return *(const int*)arg != ((const struct element*)elem)->value;
 * }
 *
 * int value_to_find = 1;
 * struct element* elem = tommy_hashdyn_search(&hashdyn, compare, &value_to_find, tommy_inthash_u32(value_to_find));
 * if (!elem) {
 *     // not found
 * } else {
 *     // found
 * }
 * \endcode
 *
 * To iterate over all the elements in the hashtable with the same key, you have to
 * use tommy_hashdyn_bucket() and follow the tommy_node::next pointer until 0.
 * You have also to check explicitly for the key, as the bucket may contain
 * different keys.
 *
 * \code
 * int value_to_find = 1;
 * tommy_node* i = tommy_hashdyn_bucket(&hashdyn, tommy_inthash_u32(value_to_find));
 * while (i) {
 *     struct element* elem = i->data; // gets the element pointer
 *
 *     if (elem->value == value_to_find) {
 *         printf("%d\n", elem->value); // process the element
 *     }
 *
 *     i = i->next; // goes to the next element
 * }
 * \endcode
 *
 * To remove an element from the hashtable you have to call tommy_hashdyn_remove()
 * providing a comparison function, its argument, and the hash of the key to search
 * and remove.
 *
 * \code
 * struct element* elem = tommy_hashdyn_remove(&hashdyn, compare, &value_to_remove, tommy_inthash_u32(value_to_remove));
 * if (elem) {
 *     free(elem); // frees the element allocated memory
 * }
 * \endcode
 *
 * To destroy the hashtable you have to remove all the elements, and deinitialize
 * the hashtable calling tommy_hashdyn_done().
 *
 * \code
 * tommy_hashdyn_done(&hashdyn);
 * \endcode
 *
 * If you need to iterate over all the elements in the hashtable, you can use
 * tommy_hashdyn_foreach() or tommy_hashdyn_foreach_arg().
 * For traversal that permits insertions and removals, use tommy_hashdyn_head()
 * and tommy_hashdyn_next(), or tommy_hashdyn_tail() and tommy_hashdyn_prev().
 * Hashes are compared from the least significant bit to the most significant
 * bit, with 0 before 1. Equal hashes are visited in insertion order.
 * This order is independent of the number of buckets and progressive resizing.
 * It differs from the bucket order used by foreach and to_list.
 *
 * \anchor tommy_hashdyn_iterator_validity
 * Iterators are pointers to nodes currently contained in the hashtable.
 * Inserting elements or removing other nodes does not invalidate them or
 * change the relative iteration order of the remaining elements.
 * A forward or backward traversal visits every original element that remains
 * contained, even if removals cause the table to shrink or insertions grow it.
 * New elements can be visited if inserted after the current position in the
 * traversal direction; elements inserted before it need not be visited.
 *
 * Removing the pointed node invalidates its use as an iterator, even if its
 * storage has not been freed. Obtain the next or previous node before removal,
 * and keep that saved node contained until continuing the traversal.
 *
 * \code
 * tommy_hashdyn_node* i = tommy_hashdyn_head(&hashdyn);
 * while (i) {
 *     tommy_hashdyn_node* next = tommy_hashdyn_next(&hashdyn, i);
 *     if (should_remove(i->data)) {
 *         free(tommy_hashdyn_remove_existing(&hashdyn, i));
 *     }
 *     i = next;
 * }
 * \endcode
 *
 * Changing a hash with tommy_hashdyn_rehash_existing() can change iteration
 * order, so the traversal guarantee does not cover rehashing during traversal.
 * After swapping containers, use the container that now contains the node.
 * Clearing or deinitializing the container invalidates all its iterators.
 * Concurrent traversal and modification require external synchronization.
 *
 * Iterator calls scan bucket contents without allocating or modifying memory.
 * With a good hash function and normal dynamic resizing, a full traversal
 * runs in expected O(n) time. Under the same hash distribution assumption,
 * sparse tables require expected O(n + b) time, where n is the number of
 * elements and b is the number of buckets. Such tables can arise after
 * tommy_hashdyn_reserve(), tommy_hashdyn_clear(), or tommy_hashdyn_to_list().
 * In pathological cases with severe hash collisions, traversal can take
 * O(n^2 + b) time.
 * Use foreach for guaranteed O(n + b) traversal when no insertions or removals
 * are needed.
 */

#ifndef TOMMY_HASHDYN_H
#define TOMMY_HASHDYN_H

#include "tommyhash.h"
#include "tommylist.h"

/******************************************************************************/
/* hashdyn */

/** \internal
 * Initial and minimal size of the hashtable expressed as a power of 2.
 * The initial size is 2^TOMMY_HASHDYN_BIT.
 */
#define TOMMY_HASHDYN_BIT 4

/**
 * Hashtable node.
 * This is the node that you have to include inside your elements.
 */
typedef tommy_node tommy_hashdyn_node;

/**
 * Hashtable container type.
 * \note Don't use internal fields directly, but access the container only using functions.
 */
typedef struct tommy_hashdyn_struct {
	tommy_hashdyn_node** bucket; /**< Hash buckets. One list for each hash modulus. */
	tommy_size_t bucket_max; /**< Number of buckets. */
	tommy_size_t bucket_mask; /**< Bit mask to access the buckets. */
	tommy_size_t count; /**< Number of elements. */
	tommy_uint_t bucket_bit; /**< Bits used in the bit mask. */
} tommy_hashdyn;

/**
 * Initializes the hashtable.
 * \param hashdyn The hashtable.
 */
TOMMY_API void tommy_hashdyn_init(tommy_hashdyn* hashdyn);

/**
 * Deinitializes the hashtable.
 *
 * You can call this function with elements still contained,
 * but such elements are not going to be freed by this call.
 * \param hashdyn The hashtable.
 */
TOMMY_API void tommy_hashdyn_done(tommy_hashdyn* hashdyn);

/**
 * Exchanges the complete state of two initialized hashtables, including bucket allocations.
 * The hashtables must not share nodes. Nodes and elements are not accessed or modified.
 * Existing node pointers remain valid and belong to the other hashtable.
 * Passing the same hashtable twice has no effect. Both hashtables remain usable and deinitializable.
 * \param first The first hashtable.
 * \param second The second hashtable.
 * \note This operation is O(1) with respect to the number of elements.
 */
tommy_inline void tommy_hashdyn_swap(tommy_hashdyn* first, tommy_hashdyn* second)
{
	tommy_hashdyn tmp = *first;
	*first = *second;
	*second = tmp;
}

/**
 * Removes all elements, preserving the allocated buckets.
 * Call tommy_hashdyn_shrink() after clearing to restore the initial capacity.
 * The hashtable remains initialized and can be reused immediately.
 * Elements are not freed and nodes are not accessed or modified.
 * Their links must not be used to traverse the previous contents.
 * You can call this function after tommy_hashdyn_foreach() has freed the elements.
 * Subsequent insertions and removals retain the normal resizing policy.
 * \param hashdyn The hashtable.
 * \note This operation is O(b), where b is the number of buckets.
 */
TOMMY_API void tommy_hashdyn_clear(tommy_hashdyn* hashdyn);

/**
 * Pre-allocates buckets for the specified number of elements.
 *
 * It ensures that the hashtable has enough buckets allocated to contain
 * the specified number of elements without triggering a dynamic resize on insertion.
 * Subsequent removals retain the normal shrinking policy and may reduce
 * the allocated capacity.
 * \param hashdyn The hashtable.
 * \param count Number of elements to reserve space for.
 */
TOMMY_API void tommy_hashdyn_reserve(tommy_hashdyn* hashdyn, tommy_size_t count);

/**
 * Shrinks the allocated buckets to fit the current number of elements.
 * The target is the smallest power of 2, at least 16, for which
 * the number of elements is strictly less than half the number of buckets.
 * If the current allocation is already no larger than the target, nothing is done.
 * An empty hashtable is reduced to 16 buckets.
 * Elements are not freed, and the tommy_node::data and tommy_node::index fields
 * are left unchanged. The order of elements with the same hash is preserved.
 * \param hashdyn The hashtable.
 * \note This operation is O(b), where b is the previous number of buckets.
 */
TOMMY_API void tommy_hashdyn_shrink(tommy_hashdyn* hashdyn);

/**
 * Inserts an element in the hashtable.
 * \param hashdyn The hashtable.
 * \param node Pointer to the node embedded into the element to insert.
 * \param data Pointer to the element to insert.
 * \param hash Hash of the element to insert.
 */
TOMMY_API void tommy_hashdyn_insert(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node, void* data, tommy_hash_t hash);

/**
 * Inserts an element only if no equal element is already contained.
 * If found, the first equal element is returned,
 * and the hashtable and candidate node are left unchanged.
 * Otherwise, the candidate is inserted using the normal insertion policy,
 * and data is returned.
 * Elements are not freed by this call.
 * \param hashdyn The hashtable.
 * \param node The candidate node. It must not belong to any container.
 * \param data Pointer to the element to insert.
 * \param cmp Compare function called with cmp_arg as first argument and with the element to compare as a second one.
 * The function should return 0 for equal elements, non-zero for different elements.
 * \param cmp_arg Compare argument describing the candidate key.
 * \param hash Hash of the candidate key, consistent with the comparison function.
 * \return The first equal element, or data if the candidate was inserted.
 */
TOMMY_API void* tommy_hashdyn_insert_unique(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node, void* data, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash);

/**
 * Searches and removes an element from the hashtable.
 * You have to provide a compare function and the hash of the element you want to remove.
 * If the element is not found, 0 is returned.
 * If more equal elements are present, the first one is removed.
 * \param hashdyn The hashtable.
 * \param cmp Compare function called with cmp_arg as first argument and with the element to compare as a second one.
 * The function should return 0 for equal elements, non-zero for different elements.
 * \param cmp_arg Compare argument passed as first argument of the compare function.
 * \param hash Hash of the element to find and remove.
 * \return The removed element, or 0 if not found.
 */
TOMMY_API void* tommy_hashdyn_remove(tommy_hashdyn* hashdyn, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash);

/**
 * Gets the bucket of the specified hash.
 * The bucket is guaranteed to contain ALL the elements with the specified hash,
 * but it can also contain others.
 * You can access elements in the bucket following the tommy_node::next pointer until 0.
 * \param hashdyn The hashtable.
 * \param hash Hash of the element to find.
 * \return The head of the bucket, or 0 if empty.
 */
tommy_inline tommy_hashdyn_node* tommy_hashdyn_bucket(tommy_hashdyn* hashdyn, tommy_hash_t hash)
{
	return hashdyn->bucket[hash & hashdyn->bucket_mask];
}

/**
 * Finds a node in the hashtable.
 * You have to provide a compare function and the hash of the element you want to find.
 * If more equal elements are present, the first one is returned.
 * \param hashdyn The hashtable.
 * \param cmp Compare function called with cmp_arg as first argument and with the element to compare as a second one.
 * The function should return 0 for equal elements, non-zero for different elements.
 * \param cmp_arg Compare argument passed as first argument of the compare function.
 * \param hash Hash of the element to find.
 * \return The first node found, or 0 if none.
 */
tommy_inline tommy_hashdyn_node* tommy_hashdyn_find(tommy_hashdyn* hashdyn, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash)
{
	tommy_hashdyn_node* i = tommy_hashdyn_bucket(hashdyn, hash);

	while (i) {
		/* we first check if the hash matches, as in the same bucket we may have multiple hash values */
		if (i->index == hash && cmp(cmp_arg, i->data) == 0)
			return i;
		i = i->next;
	}
	return 0;
}

/**
 * Searches an element in the hashtable.
 * You have to provide a compare function and the hash of the element you want to find.
 * If more equal elements are present, the first one is returned.
 * \param hashdyn The hashtable.
 * \param cmp Compare function called with cmp_arg as first argument and with the element to compare as a second one.
 * The function should return 0 for equal elements, non-zero for different elements.
 * \param cmp_arg Compare argument passed as first argument of the compare function.
 * \param hash Hash of the element to find.
 * \return The first element found, or 0 if none.
 */
tommy_inline void* tommy_hashdyn_search(tommy_hashdyn* hashdyn, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash)
{
	tommy_hashdyn_node* node = tommy_hashdyn_find(hashdyn, cmp, cmp_arg, hash);

	return node ? node->data : 0;
}

/**
 * Removes an element from the hashtable.
 * You must already have the address of the node to remove.
 * \param hashdyn The hashtable.
 * \param node The node to remove.
 * \return The removed element.
 */
TOMMY_API void* tommy_hashdyn_remove_existing(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node);

/**
 * Gets the first node in iteration order, or 0 if empty.
 * See \ref tommy_hashdyn_iterator_validity for ordering and iterator validity.
 * \param hashdyn The hashtable.
 * \return The first node in iteration order, or 0 if empty.
 * \note This operation is expected O(1) under suitable hash distribution and
 * normal dynamic resizing. In the worst case, scanning bucket contents and
 * empty buckets takes O(n + b), where n is the number of elements and b is
 * the number of buckets.
 */
TOMMY_API tommy_hashdyn_node* tommy_hashdyn_head(tommy_hashdyn* hashdyn);

/**
 * Gets the last node in iteration order, or 0 if empty.
 * See \ref tommy_hashdyn_iterator_validity for ordering and iterator validity.
 * \param hashdyn The hashtable.
 * \return The last node in iteration order, or 0 if empty.
 * \note This operation is expected O(1) under suitable hash distribution and
 * normal dynamic resizing. In the worst case, scanning bucket contents and
 * empty buckets takes O(n + b), where n is the number of elements and b is
 * the number of buckets.
 */
TOMMY_API tommy_hashdyn_node* tommy_hashdyn_tail(tommy_hashdyn* hashdyn);

/**
 * Gets the following node in iteration order, or 0 for the tail.
 * See \ref tommy_hashdyn_iterator_validity for ordering and iterator validity.
 * \param hashdyn The hashtable.
 * \param node Node currently contained in the hashtable. It must not be 0.
 * \return The following node in iteration order, or 0 for the tail.
 * \note This operation is expected O(1) under suitable hash distribution and
 * normal dynamic resizing. In the worst case, scanning bucket contents and
 * empty buckets takes O(n + b), where n is the number of elements and b is
 * the number of buckets.
 */
TOMMY_API tommy_hashdyn_node* tommy_hashdyn_next(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node);

/**
 * Gets the preceding node in iteration order, or 0 for the head.
 * See \ref tommy_hashdyn_iterator_validity for ordering and iterator validity.
 * \param hashdyn The hashtable.
 * \param node Node currently contained in the hashtable. It must not be 0.
 * \return The preceding node in iteration order, or 0 for the head.
 * \note This operation is expected O(1) under suitable hash distribution and
 * normal dynamic resizing. In the worst case, scanning bucket contents and
 * empty buckets takes O(n + b), where n is the number of elements and b is
 * the number of buckets.
 */
TOMMY_API tommy_hashdyn_node* tommy_hashdyn_prev(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node);

/**
 * Updates the hash of an element already contained in the hashtable.
 * The node must belong to this hashtable. The caller updates the element key
 * and provides its new hash, without modifying tommy_node::index directly.
 * If the hash is unchanged, the node and its position are left unchanged.
 * Otherwise, the node is moved to the tail of the destination bucket,
 * even if the old and new hashes identify the same bucket.
 * The tommy_node::data field and the number of elements are left unchanged.
 * No memory allocation, deallocation or resize is performed.
 * Equal keys are allowed; no uniqueness check is performed.
 * \param hashdyn The hashtable.
 * \param node The node whose hash is updated.
 * \param hash The new hash of the element.
 * \note This operation is O(1).
 */
TOMMY_API void tommy_hashdyn_rehash_existing(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node, tommy_hash_t hash);

/**
 * Calls the specified function for each element in the hashtable.
 *
 * You cannot add or remove elements from the inside of the callback,
 * but can use it to deallocate them.
 *
 * \code
 * tommy_hashdyn hashdyn;
 *
 * // initializes the hashtable
 * tommy_hashdyn_init(&hashdyn);
 *
 * ...
 *
 * // creates an element
 * struct element* elem = malloc(sizeof(struct element));
 *
 * ...
 *
 * // insert it in the hashtable
 * tommy_hashdyn_insert(&hashdyn, &elem->node, elem, tommy_inthash_u32(elem->value));
 *
 * ...
 *
 * // deallocates all the elements iterating the hashtable
 * tommy_hashdyn_foreach(&hashdyn, free);
 *
 * // deallocates the hashtable
 * tommy_hashdyn_done(&hashdyn);
 * \endcode
 *
 * If the callback frees the elements, the hashtable retains dangling
 * node pointers. Call tommy_hashdyn_clear() before reusing it, or
 * tommy_hashdyn_done() to release its storage.
 * \param hashdyn The hashtable.
 * \param func Function to call with each element.
 */
TOMMY_API void tommy_hashdyn_foreach(tommy_hashdyn* hashdyn, tommy_foreach_func* func);

/**
 * Calls the specified function with an argument for each element in the hashtable.
 * The iteration order and callback rules are the same as tommy_hashdyn_foreach().
 * The callback may deallocate the current element.
 * Adding or removing elements from inside the callback is not allowed.
 * \param hashdyn The hashtable.
 * \param func Function to call with each element.
 * \param arg Argument to pass to the function.
 */
TOMMY_API void tommy_hashdyn_foreach_arg(tommy_hashdyn* hashdyn, tommy_foreach_arg_func* func, void* arg);

/**
 * Gets the number of elements.
 * \param hashdyn The hashtable.
 * \return The number of elements in the hashtable.
 * \note This operation is O(1).
 */
tommy_inline tommy_size_t tommy_hashdyn_count(const tommy_hashdyn* hashdyn)
{
	return hashdyn->count;
}

/**
 * Checks whether the hashtable is empty.
 * \param hashdyn The hashtable.
 * \return Nonzero if the hashtable is empty, 0 otherwise.
 * \note This operation is O(1).
 */
tommy_inline tommy_bool_t tommy_hashdyn_empty(const tommy_hashdyn* hashdyn)
{
	return hashdyn->count == 0;
}

/**
 * Gets the number of buckets.
 * \param hashdyn The hashtable.
 * \return The number of buckets.
 * \note This operation is O(1).
 */
tommy_inline tommy_size_t tommy_hashdyn_bucket_count(const tommy_hashdyn* hashdyn)
{
	return hashdyn->bucket_max;
}

/**
 * Gets the size of allocated memory.
 * It includes the size of the ::tommy_hashdyn_node of the stored elements.
 * \param hashdyn The hashtable.
 * \return Total memory allocated by the hashtable, in bytes.
 */
TOMMY_API tommy_size_t tommy_hashdyn_memory_usage(const tommy_hashdyn* hashdyn);

/**
 * Transfers all elements from the hashtable into a tommy_list.
 *
 * Removes every element from the \p hashdyn hashtable and inserts them
 * into the provided \p list (at the tail), preserving the per-bucket order
 * but not guaranteeing any particular global order.
 *
 * After the call:
 * - the hashtable is left empty and initialized, preserving its allocated buckets
 * - the target list contains all the elements that were previously in the hashtable
 *
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 *
 * This function is useful when you need to:
 * - extract all elements to process/sort them outside the hashtable
 * - convert the hashtable into a list for sequential iteration
 * - prepare for a full clear + re-insertion with different hash/ordering
 * - move ownership of the nodes to a list-based container
 *
 * Typical usage pattern:
 * \code
 * tommy_list all_elements;
 * tommy_list_init(&all_elements);
 *
 * // move everything out of the hashtable into the list
 * tommy_hashdyn_to_list(&hashdyn, &all_elements);
 *
 * // now you can sort, filter, process sequentially, etc.
 * tommy_list_sort(&all_elements, compare_by_value);
 * \endcode
 *
 * \param hashdyn The hashtable to drain.
 * \param list The destination list. It must be initialized and must not share
 * nodes with the hashtable. Existing elements remain at the head of the list.
 * \note This operation is O(b), where b is the number of buckets.
 * \note No memory allocation or deallocation is performed.
 * \note The relative order of elements that were in the same bucket is preserved,
 *       but the order among different buckets is bucket-order dependent.
 */
TOMMY_API void tommy_hashdyn_to_list(tommy_hashdyn* hashdyn, tommy_list* list);

#endif

