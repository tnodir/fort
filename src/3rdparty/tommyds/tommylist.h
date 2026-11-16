// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2010 Andrea Mazzoleni

/** \file
 * Doubly linked list for collisions into hashtables.
 *
 * This list is a doubly linked list mainly targeted for handling collisions
 * into a hashtable, but usable also as a generic list.
 *
 * The main feature of this list is to require only one pointer to represent the
 * list, compared to a classic implementation requiring both head and tail pointers.
 * This reduces the memory usage in hashtables.
 *
 * Another feature is to support the insertion at the end of the list. This allows
 * storing collisions in a stable order, meaning that equal elements preserve
 * their insertion order.
 *
 * To initialize the list, you have to call tommy_list_init(), or to simply assign
 * to it 0, as an empty list is represented by the 0 value.
 *
 * \code
 * tommy_list list;
 *
 * tommy_list_init(&list); // initializes the list
 * \endcode
 *
 * To insert elements in the list you have to call tommy_list_insert_tail()
 * or tommy_list_insert_head() for each element.
 * In the insertion call you have to specify the address of the node and the
 * address of the element.
 * The address of the element is used to initialize the tommy_node::data field
 * of the node.
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
 * tommy_list_insert_tail(&list, &elem->node, elem); // inserts the element
 * \endcode
 *
 * To iterate over all the elements in the list you have to call
 * tommy_list_head() to get the head of the list and follow the
 * tommy_node::next pointer until 0.
 *
 * \code
 * tommy_node* i = tommy_list_head(&list);
 * while (i) {
 *     struct element* elem = i->data; // gets the element pointer
 *
 *     printf("%d\n", elem->value); // process the element
 *
 *     i = i->next; // go to the next element
 * }
 * \endcode
 *
 * To iterate backward, start at tommy_list_tail() and use tommy_list_prev().
 * Do not follow tommy_node::prev directly past the head, as it points to the tail.
 *
 * \code
 * tommy_node* i = tommy_list_tail(&list);
 * while (i) {
 *     struct element* elem = i->data;
 *
 *     printf("%d\n", elem->value);
 *
 *     i = tommy_list_prev(&list, i);
 * }
 * \endcode
 *
 * \anchor tommy_list_iterator_validity
 * Iterators are pointers to nodes contained in the list.
 * Inserting elements or removing other nodes between iterator calls does not
 * invalidate pointers to nodes that remain in the list.
 * Subsequent next/prev calls use the current contents and ordering;
 * inserted elements may become visible and removed elements are skipped.
 *
 * Removing the pointed node invalidates its use as an iterator, even if
 * its storage has not been freed. To continue traversal after removing
 * the current node, obtain the next or previous node before removal.
 *
 * \code
 * tommy_node* i = tommy_list_head(&list);
 * while (i) {
 *     tommy_node* next = i->next; // obtain the next node before removal
 *
 *     if (condition) {
 *         tommy_list_remove_existing(&list, i);
 *         free(i->data); // safe to deallocate
 *     }
 *
 *     i = next;
 * }
 * \endcode
 *
 * Concurrent traversal and modification require external synchronization.
 *
 * To reorder existing elements, use tommy_list_move_head(), tommy_list_move_tail()
 * or tommy_list_reverse(). To split a list or transfer elements between lists,
 * use tommy_list_split(), tommy_list_prepend(), tommy_list_splice_before()
 * and tommy_list_splice_after().
 * These operations preserve the element pointers and do not allocate memory.
 *
 * \code
 * tommy_list second;
 * tommy_list_init(&second);
 *
 * // transfers node and all following nodes to second
 * tommy_list_split(&list, node, &second);
 *
 * // appends the transferred nodes back to list
 * tommy_list_concat(&list, &second);
 * \endcode
 *
 * Two lists sorted with the same comparison function can be merged with
 * tommy_list_merge(). The merge is stable.
 *
 * To destroy the list you have to remove all the elements,
 * as the list is completely in-place and it doesn't allocate memory.
 * This can be done with the tommy_list_foreach() function.
 *
 * \code
 * // deallocates all the elements iterating the list
 * tommy_list_foreach(&list, free);
 * tommy_list_clear(&list); // resets the list before reuse
 * \endcode
 */

#ifndef TOMMY_LIST_H
#define TOMMY_LIST_H

#include "tommytypes.h"

/******************************************************************************/
/* list */

/**
 * Doubly linked list type.
 */
typedef tommy_node* tommy_list;

/**
 * Temporary list builder with an embedded sentinel.
 * Initialize with tommy_builder_init() and finish with tommy_list_concat_builder().
 * Keep the builder at the same address until finalization, as node links
 * refer to its sentinel.
 */
typedef struct tommy_builder_struct {
	tommy_node dummy; /**< Sentinel excluded from the resulting list. */
} tommy_builder;

/**
 * Initializes an empty builder in O(1).
 * \param builder Builder to initialize in its final location.
 * \return Initial tail, pointing to the builder's sentinel.
 */
tommy_inline tommy_node* tommy_builder_init(tommy_builder* builder)
{
	builder->dummy.next = 0;
	return &builder->dummy;
}

/**
 * Appends a non-empty chain to a builder in O(1), preserving element order.
 * \param tail Current tail, initially returned by tommy_builder_init().
 * \param head Head of the non-empty source chain.
 * \param last Tail of the source chain. For a single node, pass head again.
 * The chains must not share nodes. Inner links must be valid;
 * head's prev and last's next may be undefined.
 * \return Updated tail to pass to the next append or tommy_list_concat_builder().
 */
tommy_inline tommy_node* tommy_builder_concat(tommy_node* tail, tommy_node* head, tommy_node* last)
{
	head->prev = tail;
	tail->next = head;
	return last;
}

/**
 * Initializes the list.
 * The list is completely in-place, so it doesn't need to be deinitialized.
 * \param list The list.
 */
tommy_inline void tommy_list_init(tommy_list* list)
{
	*list = 0;
}

/**
 * Removes all elements.
 * The list remains initialized and can be reused immediately.
 * Elements are not freed and nodes are not accessed or modified.
 * Their links must not be used to traverse the previous contents.
 * You can call this function after tommy_list_foreach() has freed the elements.
 * \param list The list.
 * \note This operation is O(1).
 */
tommy_inline void tommy_list_clear(tommy_list* list)
{
	*list = 0;
}

/**
 * Gets the head of the list.
 * \param list The list.
 * \return The head node, or 0 if the list is empty.
 * \note This operation is O(1).
 */
tommy_inline tommy_node* tommy_list_head(tommy_list* list)
{
	return *list;
}

/**
 * Gets the tail of the list.
 * \param list The list.
 * \return The tail node, or 0 if the list is empty.
 * \note This operation is O(1).
 */
tommy_inline tommy_node* tommy_list_tail(tommy_list* list)
{
	tommy_node* head = tommy_list_head(list);

	if (!head)
		return 0;

	return head->prev;
}

/**
 * Gets the previous node in the list.
 * Unlike tommy_node::prev, this function returns 0 at the head of the list,
 * allowing backward iteration starting from tommy_list_tail().
 * See \ref tommy_list_iterator_validity for iterator validity across insertions and removals.
 * \param list The list.
 * \param node The current node. The node must be in the list.
 * \return The previous node, or 0 if the current node is the head.
 * \note This operation is O(1).
 */
tommy_inline tommy_node* tommy_list_prev(tommy_list* list, tommy_node* node)
{
	if (node == tommy_list_head(list))
		return 0;

	return node->prev;
}

/** \internal
 * Creates a new list with a single element.
 * \param list The list.
 * \param node The node to insert.
 */
tommy_inline void tommy_list_insert_first(tommy_list* list, tommy_node* node)
{
	/* one element "circular" prev list */
	node->prev = node;

	/* one element "0 terminated" next list */
	node->next = 0;

	*list = node;
}

/** \internal
 * Inserts an element at the head of a non-empty list.
 * The element is inserted at the head of the list. The list cannot be empty.
 * \param list The list.
 * \param node The node to insert.
 */
tommy_inline void tommy_list_insert_head_not_empty(tommy_list* list, tommy_node* node)
{
	tommy_node* head = tommy_list_head(list);

	/* insert in the "circular" prev list */
	node->prev = head->prev;
	head->prev = node;

	/* insert in the "0 terminated" next list */
	node->next = head;

	*list = node;
}

/** \internal
 * Inserts an element at the tail of a non-empty list.
 * The element is inserted at the tail of the list. The list cannot be empty.
 * \param head The node at the list head. It cannot be 0.
 * \param node The node to insert.
 */
tommy_inline void tommy_list_insert_tail_not_empty(tommy_node* head, tommy_node* node)
{
	/* insert in the "circular" prev list */
	node->prev = head->prev;
	head->prev = node;

	/* insert in the "0 terminated" next list */
	node->next = 0;
	node->prev->next = node;
}

/**
 * Inserts an element at the head of a list.
 * \param list The list.
 * \param node Pointer to the node embedded into the element to insert.
 * \param data Pointer to the element to insert.
 */
tommy_inline void tommy_list_insert_head(tommy_list* list, tommy_node* node, void* data)
{
	tommy_node* head = tommy_list_head(list);

	if (head)
		tommy_list_insert_head_not_empty(list, node);
	else
		tommy_list_insert_first(list, node);

	node->data = data;
}

/**
 * Inserts an element at the tail of a list.
 * \param list The list.
 * \param node Pointer to the node embedded into the element to insert.
 * \param data Pointer to the element to insert.
 */
tommy_inline void tommy_list_insert_tail(tommy_list* list, tommy_node* node, void* data)
{
	tommy_node* head = tommy_list_head(list);

	if (head)
		tommy_list_insert_tail_not_empty(head, node);
	else
		tommy_list_insert_first(list, node);

	node->data = data;
}

/**
 * Inserts an element before the specified reference node.
 * \param list The list.
 * \param reference The reference node. The new node will be inserted before this node. The reference node must be in the list.
 * \param node Pointer to the node embedded into the element to insert.
 * \param data Pointer to the element to insert.
 */
tommy_inline void tommy_list_insert_before(tommy_list* list, tommy_node* reference, tommy_node* node, void* data)
{
	tommy_node* head = tommy_list_head(list);

	/* if inserting before the head, update the list pointer */
	if (head == reference) {
		tommy_list_insert_head_not_empty(list, node);
	} else {
		/* insert in the "circular" prev list */
		node->prev = reference->prev;
		reference->prev = node;

		/* insert in the "0 terminated" next list */
		node->next = reference;
		node->prev->next = node;
	}

	node->data = data;
}

/**
 * Inserts an element after the specified reference node.
 * \param list The list.
 * \param reference The reference node. The new node will be inserted after this node. The reference node must be in the list.
 * \param node Pointer to the node embedded into the element to insert.
 * \param data Pointer to the element to insert.
 */
tommy_inline void tommy_list_insert_after(tommy_list* list, tommy_node* reference, tommy_node* node, void* data)
{
	tommy_node* head = tommy_list_head(list);

	/* if inserting after the tail (reference->next == 0), handle specially */
	if (reference->next == 0) {
		tommy_list_insert_tail_not_empty(head, node);
	} else {
		/* insert in the "circular" prev list */
		node->prev = reference;
		reference->next->prev = node;

		/* insert in the "0 terminated" next list */
		node->next = reference->next;
		reference->next = node;
	}

	node->data = data;
}

/**
 * Removes an element from the list.
 * You must already have the address of the node to remove.
 * \param list The list.
 * \param node The node to remove.
 * \return The removed element.
 * \note The node content is left unchanged, including the tommy_node::next
 * and tommy_node::prev fields that still contain pointers in the list.
 */
tommy_inline void* tommy_list_remove_existing(tommy_list* list, tommy_node* node)
{
	tommy_node* head = tommy_list_head(list);

	/* remove from the "circular" prev list */
	if (node->next)
		node->next->prev = node->prev;
	else
		head->prev = node->prev; /* the last */

	/* remove from the "0 terminated" next list */
	if (head == node)
		*list = node->next; /* the new head, in case 0 */
	else
		node->prev->next = node->next;

	return node->data;
}

/**
 * Removes the element at the head of the list.
 * \param list The list.
 * \return The removed element, or 0 if the list is empty.
 */
tommy_inline void* tommy_list_remove_head(tommy_list* list)
{
	tommy_node* node = tommy_list_head(list);

	if (!node)
		return 0;

	/* the new head must retain the tail pointer in the "circular" prev list */
	if (node->next)
		node->next->prev = node->prev;

	*list = node->next;

	return node->data;
}

/**
 * Removes the element at the tail of the list.
 * \param list The list.
 * \return The removed element, or 0 if the list is empty.
 */
tommy_inline void* tommy_list_remove_tail(tommy_list* list)
{
	tommy_node* head = tommy_list_head(list);

	if (!head)
		return 0;

	tommy_node* node = head->prev;

	/* a single node is also the head, and its links must remain unchanged */
	if (node == head) {
		*list = 0;
	} else {
		head->prev = node->prev;
		node->prev->next = 0;
	}

	return node->data;
}

/**
 * Moves an existing node to the head of the list.
 * If the node is already at the head, nothing is done.
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 * \param list The list.
 * \param node The node to move. The node must be in the list.
 */
tommy_inline void tommy_list_move_head(tommy_list* list, tommy_node* node)
{
	if (node == tommy_list_head(list))
		return;

	tommy_list_remove_existing(list, node);
	tommy_list_insert_head_not_empty(list, node);
}

/**
 * Moves an existing node to the tail of the list.
 * If the node is already at the tail, nothing is done.
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 * \param list The list.
 * \param node The node to move. The node must be in the list.
 */
tommy_inline void tommy_list_move_tail(tommy_list* list, tommy_node* node)
{
	if (node->next == 0)
		return;

	tommy_list_remove_existing(list, node);
	tommy_list_insert_tail_not_empty(tommy_list_head(list), node);
}

/**
 * Splits a list before the specified node.
 * The list retains the nodes preceding the specified node. The second list
 * receives the specified node and all following nodes. The order of the elements is preserved.
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 * \param list The list to split.
 * \param node The first node to transfer. The node must be in the list,
 * or 0 to leave list unchanged and set second to 0.
 * Specifying the head transfers the whole list.
 * \param second Output list receiving node and all following nodes.
 * It must be distinct from list and need not be initialized.
 * Its previous value is overwritten. Any previously referenced nodes
 * are left unchanged and are not deallocated.
 * \note This operation is O(1).
 */
tommy_inline void tommy_list_split(tommy_list* list, tommy_node* node, tommy_list* second)
{
	tommy_node* head = tommy_list_head(list);
	tommy_node* tail;
	tommy_node* before;

	if (!node) {
		*second = 0;
		return;
	}

	if (node == head) {
		*second = *list;
		*list = 0;
		return;
	}

	tail = head->prev;
	before = node->prev;

	/* terminate the retained list */
	head->prev = before;
	before->next = 0;

	/* create the list of transferred nodes */
	node->prev = tail;
	*second = node;
}

/**
 * Exchanges the contents of two lists.
 * The lists must not share nodes. Nodes and elements are not accessed or modified.
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 * Passing the same list twice has no effect.
 * \param first The first list.
 * \param second The second list.
 * \note This operation is O(1).
 */
tommy_inline void tommy_list_swap(tommy_list* first, tommy_list* second)
{
	tommy_list tmp = *first;
	*first = *second;
	*second = tmp;
}

/**
 * Concatenates two lists.
 * The second list is concatenated to the first list.
 * \param first The first list.
 * \param second The second list. After this call the list content is undefined,
 * and you should not use it anymore.
 * \note This operation is O(1).
 */
tommy_inline void tommy_list_concat(tommy_list* first, tommy_list* second)
{
	/* if the second is empty, nothing to do */
	tommy_node* second_head = tommy_list_head(second);
	if (second_head == 0)
		return;

	/* if the first is empty, copy the second */
	tommy_node* first_head = tommy_list_head(first);
	if (first_head == 0) {
		*first = *second;
		return;
	}

	/* tail of the first list */
	tommy_node* first_tail = first_head->prev;

	/* set the "circular" prev list */
	first_head->prev = second_head->prev;
	second_head->prev = first_tail;

	/* set the "0 terminated" next list */
	first_tail->next = second_head;
}

/**
 * Appends a builder's chain to a list, preserving element order and restoring list links.
 * \param list Destination list, possibly empty. It must not share nodes with the builder.
 * \param builder Initialized builder, possibly empty. Its contents are consumed;
 * reinitialize it before reuse.
 * \param tail Current tail returned by the builder helpers, ignored when empty.
 * \note This operation is O(1). An empty builder has no effect.
 */
tommy_inline void tommy_list_concat_builder(tommy_list* list, const tommy_builder* builder, tommy_node* tail)
{
	tommy_node* head = builder->dummy.next;

	if (!head)
		return;

	tommy_node* first = *list;

	/* replace the sentinel link so no list node refers to the temporary builder */
	if (first) {
		tommy_node* first_tail = first->prev;
		first_tail->next = head;
		head->prev = first_tail;
		first->prev = tail;
	} else {
		*list = head;
		head->prev = tail;
	}

	tail->next = 0;
}

/**
 * Prepends a list to another list.
 * The second list is prepended to the first list.
 * \param first The first list.
 * \param second The second list. After this call the list content is undefined,
 * and you should not use it anymore.
 * \note This operation is O(1).
 */
tommy_inline void tommy_list_prepend(tommy_list* first, tommy_list* second)
{
	tommy_list_concat(second, first);
	*first = *second;
}

/**
 * Transfers all elements of a list before the specified reference node.
 * The order of the transferred elements is preserved.
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 * \param list The destination list.
 * \param reference The node before which to insert. It must not be 0 and must be in the destination list.
 * \param second The source list. After this call the list content is undefined,
 * and you should not use it anymore.
 * The lists must be distinct and must not share nodes.
 * \note This operation is O(1). An empty source list has no effect.
 */
TOMMY_API void tommy_list_splice_before(tommy_list* list, tommy_node* reference, tommy_list* second);

/**
 * Transfers all elements of a list after the specified reference node.
 * The order of the transferred elements is preserved.
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 * \param list The destination list.
 * \param reference The node after which to insert. It must not be 0 and must be in the destination list.
 * \param second The source list. After this call the list content is undefined,
 * and you should not use it anymore.
 * The lists must be distinct and must not share nodes.
 * \note This operation is O(1). An empty source list has no effect.
 */
TOMMY_API void tommy_list_splice_after(tommy_list* list, tommy_node* reference, tommy_list* second);

/**
 * Merges two sorted lists.
 * Both lists must be sorted using the same comparison function.
 * The merge is stable: equal elements from the first list precede those from
 * the second list, and the relative order within each list is preserved.
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 * \param first The destination list, receiving the sorted result.
 * \param second The source list. After this call the list content is undefined,
 * and you should not use it anymore.
 * The lists must be distinct and must not share nodes.
 * \param cmp Compare function called with two elements.
 * The function should return <0 if the first element is less than the second,
 * ==0 if equal, and >0 if greater. It must not modify the lists.
 * \note This operation is O(n+m), or O(1) when one list is empty or the ranges
 * can be concatenated directly. It uses O(1) auxiliary space.
 */
TOMMY_API void tommy_list_merge(tommy_list* first, tommy_list* second, tommy_compare_func* cmp);

/**
 * Reverses the order of the elements in a list.
 * Empty lists and lists containing one element are left unchanged.
 * The tommy_node::data and tommy_node::index fields are left unchanged.
 * \param list The list.
 * \note This operation is O(n) and uses O(1) auxiliary space.
 */
TOMMY_API void tommy_list_reverse(tommy_list* list);

/**
 * Sorts a list.
 * It's a stable merge sort with O(n log n) worst complexity.
 * It's faster on degenerate cases like partially ordered lists.
 * \param list The list.
 * \param cmp Compare function called with two elements.
 * The function should return <0 if the first element is less than the second, ==0 if equal, and >0 if greater.
 * \note This operation is O(n log n) in the worst case.
 */
TOMMY_API void tommy_list_sort(tommy_list* list, tommy_compare_func* cmp);

/**
 * Checks whether the list is empty.
 * \param list The list.
 * \return Nonzero if the list is empty, 0 otherwise.
 * \note This operation is O(1).
 */
tommy_inline tommy_bool_t tommy_list_empty(const tommy_list* list)
{
	return *list == 0;
}

/**
 * Gets the number of elements.
 * \param list The list.
 * \return The number of elements in the list.
 * \note This operation is O(n).
 */
tommy_inline tommy_size_t tommy_list_count(const tommy_list* list)
{
	tommy_size_t count = 0;
	const tommy_node* i = *list;

	while (i) {
		++count;
		i = i->next;
	}

	return count;
}

/**
 * Calls the specified function for each element in the list.
 *
 * You cannot add or remove elements from the inside of the callback,
 * but can use it to deallocate them.
 *
 * \code
 * tommy_list list;
 *
 * // initializes the list
 * tommy_list_init(&list);
 *
 * ...
 *
 * // creates an element
 * struct element* elem = malloc(sizeof(struct element));
 *
 * ...
 *
 * // insert it in the list
 * tommy_list_insert_tail(&list, &elem->node, elem);
 *
 * ...
 *
 * // deallocates all the elements iterating the list
 * tommy_list_foreach(&list, free);
 * tommy_list_clear(&list); // resets the list before reuse
 * \endcode
 *
 * If the callback frees the elements, the list retains dangling node
 * pointers. Call tommy_list_clear() or tommy_list_init() before reusing it.
 * The list has no storage to release.
 * \param list The list.
 * \param func Function to call with each element.
 */
tommy_inline void tommy_list_foreach(tommy_list* list, tommy_foreach_func* func)
{
	tommy_node* node = tommy_list_head(list);

	while (node) {
		void* data = node->data;
		node = node->next;
		func(data);
	}
}

/**
 * Calls the specified function with an argument for each element in the list.
 * The iteration order and callback rules are the same as tommy_list_foreach().
 * The callback may deallocate the current element.
 * Adding or removing elements from inside the callback is not allowed.
 * \param list The list.
 * \param func Function to call with each element.
 * \param arg Argument to pass to the function.
 */
tommy_inline void tommy_list_foreach_arg(tommy_list* list, tommy_foreach_arg_func* func, void* arg)
{
	tommy_node* node = tommy_list_head(list);

	while (node) {
		void* data = node->data;
		node = node->next;
		func(arg, data);
	}
}

#endif

