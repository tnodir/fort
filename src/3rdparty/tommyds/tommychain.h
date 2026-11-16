// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2010 Andrea Mazzoleni

/** \file
 * Chain of nodes.
 * A chain of nodes is an abstraction used to implement complex list operations
 * like sorting.
 *
 * Do not use this directly. Use lists instead.
 */

#ifndef TOMMY_CHAIN_H
#define TOMMY_CHAIN_H

#include "tommytypes.h"

/******************************************************************************/
/* chain */

/**
 * Chain of nodes.
 * A chain of nodes is a sequence of nodes with the following properties:
 * - It contains at least one node. A chain of zero nodes cannot exist.
 * - The next field of the tail is of *undefined* value.
 * - The prev field of the head is of *undefined* value.
 * - All the other inner prev and next fields are correctly set.
 */
typedef struct tommy_chain_struct {
	tommy_node* head; /**< Pointer to the head of the chain. */
	tommy_node* tail; /**< Pointer to the tail of the chain. */
} tommy_chain;

/**
 * Splices a chain in the middle of another chain.
 * \param first_before Node before the splice point.
 * \param first_after Node after the splice point.
 * \param second_head Head of the chain to splice in.
 * \param second_tail Tail of the chain to splice in.
 */
tommy_inline void tommy_chain_splice(tommy_node* first_before, tommy_node* first_after, tommy_node* second_head, tommy_node* second_tail)
{
	/* set the prev list */
	first_after->prev = second_tail;
	second_head->prev = first_before;

	/* set the next list */
	first_before->next = second_head;
	second_tail->next = first_after;
}

/**
 * Concatenates two chains.
 * \param first_tail Tail of the first chain.
 * \param second_head Head of the second chain.
 */
tommy_inline void tommy_chain_concat(tommy_node* first_tail, tommy_node* second_head)
{
	/* set the prev list */
	second_head->prev = first_tail;

	/* set the next list */
	first_tail->next = second_head;
}

/**
 * Merges two chains.
 * \param first First chain (will contain the result).
 * \param second Second chain (consumed by the merge; its descriptor is not cleared
 * and must not be used as an independent chain afterward).
 * \param cmp Comparison function.
 */
tommy_inline void tommy_chain_merge(tommy_chain* tommy_restrict first, tommy_chain* tommy_restrict second, tommy_compare_func* cmp)
{
	/*
	 * comparators must not change the chains, so cache these endpoints across calls.
	 * also caching first_tail caused register spills with GCC and Clang on x86-64.
	 */
	tommy_node* first_head = first->head;
	tommy_node* second_tail = second->tail;
	tommy_node* first_i = first_head;
	tommy_node* second_i = second->head;

	/* merge */
	while (1) {
		if (cmp(first_i->data, second_i->data) > 0) {
			tommy_node* next = second_i->next;
			if (first_i == first_head) {
				tommy_chain_concat(second_i, first_i);
				first_head = second_i;
				first->head = first_head;
			} else {
				tommy_chain_splice(first_i->prev, first_i, second_i, second_i);
			}
			if (second_i == second_tail)
				break;
			second_i = next;
		} else {
			if (first_i == first->tail) {
				tommy_chain_concat(first_i, second_i);
				first->tail = second_tail;
				break;
			}
			first_i = first_i->next;
		}
	}
}

/**
 * Merges two chains containing one node each.
 * \param first First singleton chain (will contain the result).
 * \param second Second singleton chain (consumed by the merge; its descriptor is not cleared
 * and must not be used as an independent chain afterward).
 * \param cmp Comparison function.
 */
tommy_inline void tommy_chain_merge_singleton(tommy_chain* tommy_restrict first, tommy_chain* tommy_restrict second, tommy_compare_func* cmp)
{
	tommy_node* first_node = first->head;
	tommy_node* second_node = second->head;

	/* keep equal elements in their original order for a stable sort */
	if (cmp(first_node->data, second_node->data) <= 0) {
		tommy_chain_concat(first_node, second_node);
		first->tail = second_node;
	} else {
		tommy_chain_concat(second_node, first_node);
		first->head = second_node;
	}
}

/**
 * Merges two chains managing special degenerate cases.
 * It's functionally equivalent to tommy_chain_merge() but faster with already ordered chains.
 * \param first First chain (will contain the result).
 * \param second Second chain (consumed by the merge; its descriptor is not cleared
 * and must not be used as an independent chain afterward).
 * \param cmp Comparison function.
 */
tommy_inline void tommy_chain_merge_degenerated(tommy_chain* tommy_restrict first, tommy_chain* tommy_restrict second, tommy_compare_func* cmp)
{
	/* identify the condition first <= second */
	if (cmp(first->tail->data, second->head->data) <= 0) {
		tommy_chain_concat(first->tail, second->head);
		first->tail = second->tail;
		return;
	}

	/* identify the condition second < first */
	/* here we must be strict on comparison to keep the sort stable */
	if (cmp(second->tail->data, first->head->data) < 0) {
		tommy_chain_concat(second->tail, first->head);
		first->head = second->head;
		return;
	}

	tommy_chain_merge(first, second, cmp);
}

/**
 * Sorts a chain.
 * It's a stable merge sort using power of 2 buckets, with O(n log n) complexity,
 * similar to the one used in the SGI STL libraries and in the Linux Kernel,
 * but faster on degenerate cases like already ordered lists.
 *
 * SGI STL stl_list.h
 * http://www.sgi.com/tech/stl/stl_list.h
 *
 * Linux Kernel lib/list_sort.c
 * http://lxr.linux.no/#linux+v2.6.36/lib/list_sort.c
 *
 * \param chain The chain.
 * \param cmp Comparison function.
 */
tommy_inline void tommy_chain_mergesort(tommy_chain* chain, tommy_compare_func* cmp)
{
	/*
	 * Bit buckets of chains.
	 * Each bucket contains 2^(i+1) nodes or it's empty.
	 * The bucket containing an unpaired final node has one node less.
	 * The chain at address TOMMY_BIT_MAX is an independent variable operating as "carry".
	 * We keep it in the same "bit" vector to avoid reports from the valgrind tool sgcheck.
	 */
	tommy_chain bit[TOMMY_SIZE_BIT + 1];

	/*
	 * Value stored inside the bit bucket.
	 * It's used to know which bucket is empty or full.
	 */
	tommy_size_t counter = 0;
	tommy_node* node = chain->head;
	tommy_node* tail = chain->tail;

	while (1) {
		/* carry bit to add */
		tommy_chain* last = &bit[TOMMY_SIZE_BIT];
		bit[TOMMY_SIZE_BIT].head = node;
		bit[TOMMY_SIZE_BIT].tail = node;
		tommy_node* next = node->next;

		/* start with a sorted pair to halve the number of bucket insertions */
		if (node != tail) {
			node = next;
			/* save the next input node before merging can change the links */
			next = node->next;
			tommy_chain second = { node, node };
			tommy_chain_merge_singleton(last, &second, cmp);
		}

		/* add the bit, propagating the carry */
		tommy_size_t i = 0;
		tommy_size_t mask = counter;

		while ((mask & 1) != 0) {
			tommy_chain_merge_degenerated(&bit[i], last, cmp);
			mask >>= 1;
			last = &bit[i];
			++i;
		}

		/* copy the carry in the first empty bit */
		bit[i] = *last;

		/* add the carry in the counter */
		++counter;

		if (node == tail)
			break;
		node = next;
	}

	/* merge the buckets */
	tommy_size_t i = tommy_ctz(counter);
	tommy_size_t mask = counter >> i;
	while (mask != 1) {
		mask >>= 1;
		if (mask & 1)
			tommy_chain_merge_degenerated(&bit[i + 1], &bit[i], cmp);
		else
			bit[i + 1] = bit[i];
		++i;
	}

	*chain = bit[i];
}

#endif

