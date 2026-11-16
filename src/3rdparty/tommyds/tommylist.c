// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2010 Andrea Mazzoleni

#include "tommylist.h"
#include "tommychain.h"

/** \internal
 * Setup a list.
 */
tommy_inline void tommy_imp_list_set(tommy_list* list, tommy_node* head, tommy_node* tail)
{
	head->prev = tail;
	tail->next = 0;
	*list = head;
}

TOMMY_API void tommy_list_splice_before(tommy_list* list, tommy_node* reference, tommy_list* second)
{
	tommy_node* second_head = tommy_list_head(second);

	if (!second_head)
		return;

	if (reference == tommy_list_head(list)) {
		/* prepend */
		tommy_list_prepend(list, second);
	} else {
		/* splice in the middle */
		tommy_chain_splice(reference->prev, reference, second_head, second_head->prev);
	}
}

TOMMY_API void tommy_list_splice_after(tommy_list* list, tommy_node* reference, tommy_list* second)
{
	tommy_node* second_head = tommy_list_head(second);

	if (!second_head)
		return;

	if (!reference->next) {
		/* append */
		tommy_list_concat(list, second);
	} else {
		/* splice in the middle */
		tommy_chain_splice(reference, reference->next, second_head, second_head->prev);
	}
}

TOMMY_API void tommy_list_merge(tommy_list* first, tommy_list* second, tommy_compare_func* cmp)
{
	tommy_node* second_head = tommy_list_head(second);

	if (!second_head)
		return;

	tommy_node* first_head = tommy_list_head(first);

	if (!first_head) {
		*first = *second;
		return;
	}

	/* create chains from the non-empty lists */
	tommy_chain first_chain;
	tommy_chain second_chain;
	first_chain.head = first_head;
	first_chain.tail = first_head->prev;
	second_chain.head = second_head;
	second_chain.tail = second_head->prev;

	tommy_chain_merge_degenerated(&first_chain, &second_chain, cmp);

	/* restore the destination list */
	tommy_imp_list_set(first, first_chain.head, first_chain.tail);
}

TOMMY_API void tommy_list_reverse(tommy_list* list)
{
	tommy_node* head = tommy_list_head(list);
	tommy_node* node = head;
	tommy_node* prev = 0;

	if (!head)
		return;

	while (node) {
		tommy_node* next = node->next;
		node->next = prev;
		node->prev = next;
		prev = node;
		node = next;
	}

	/* the old head is the new tail */
	tommy_imp_list_set(list, prev, head);
}

TOMMY_API void tommy_list_sort(tommy_list* list, tommy_compare_func* cmp)
{
	if (tommy_list_empty(list))
		return;

	tommy_node* head = tommy_list_head(list);

	/* create a chain from the list */
	tommy_chain chain;
	chain.head = head;
	chain.tail = head->prev;

	tommy_chain_mergesort(&chain, cmp);

	/* restore the list */
	tommy_imp_list_set(list, chain.head, chain.tail);
}

