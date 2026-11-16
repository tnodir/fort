// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2010 Andrea Mazzoleni

#include "tommyhashdyn.h"
#include "tommylist.h"

#include <string.h> /* for memset */

/******************************************************************************/
/* hashdyn */

TOMMY_API void tommy_hashdyn_init(tommy_hashdyn* hashdyn)
{
	/* fixed initial size */
	hashdyn->bucket_bit = TOMMY_HASHDYN_BIT;
	hashdyn->bucket_max = (tommy_size_t)1 << hashdyn->bucket_bit;
	hashdyn->bucket_mask = hashdyn->bucket_max - 1;
	hashdyn->bucket = tommy_cast(tommy_hashdyn_node**, tommy_calloc(hashdyn->bucket_max, sizeof(tommy_hashdyn_node*)));

	hashdyn->count = 0;
}

TOMMY_API void tommy_hashdyn_done(tommy_hashdyn* hashdyn)
{
	tommy_free(hashdyn->bucket);
}

TOMMY_API void tommy_hashdyn_clear(tommy_hashdyn* hashdyn)
{
	memset(hashdyn->bucket, 0, hashdyn->bucket_max * sizeof(tommy_hashdyn_node*));
	hashdyn->count = 0;
}

/**
 * Resize the bucket vector.
 */
static void tommy_imp_hashdyn_resize(tommy_hashdyn* hashdyn, tommy_uint_t new_bucket_bit)
{
	tommy_size_t bucket_bit = hashdyn->bucket_bit;
	tommy_size_t bucket_max = hashdyn->bucket_max;
	tommy_size_t new_bucket_max = (tommy_size_t)1 << new_bucket_bit;
	tommy_size_t new_bucket_mask = new_bucket_max - 1;
	tommy_hashdyn_node** new_bucket;

	if (!hashdyn->count) {
		/* empty table: allocate zeroed memory with calloc without looping */
		new_bucket = tommy_cast(tommy_hashdyn_node**, tommy_calloc(new_bucket_max, sizeof(tommy_hashdyn_node*)));
	} else if (new_bucket_bit > bucket_bit) {
		if (new_bucket_bit == bucket_bit + 1) {
			/* grow by 1 bit: zero the two buckets inline in loop to preserve cache locality */
			new_bucket = tommy_cast(tommy_hashdyn_node**, tommy_malloc(new_bucket_max * sizeof(tommy_hashdyn_node*)));

			for (tommy_size_t i = 0; i < bucket_max; ++i) {
				new_bucket[i] = 0;
				new_bucket[i + bucket_max] = 0;

				tommy_hashdyn_node* j = hashdyn->bucket[i];
				if (j && !j->next) {
					/* growth keeps old buckets separate, and singleton links are already valid */
					new_bucket[j->index & new_bucket_mask] = j;
					continue;
				}
				while (j) {
					tommy_hashdyn_node* j_next = j->next;
					tommy_size_t pos = j->index & new_bucket_mask;
					if (new_bucket[pos])
						tommy_list_insert_tail_not_empty(new_bucket[pos], j);
					else
						tommy_list_insert_first(&new_bucket[pos], j);
					j = j_next;
				}
			}
		} else {
			/* grow by multiple bits with elements: allocate zeroed memory and reinsert */
			new_bucket = tommy_cast(tommy_hashdyn_node**, tommy_calloc(new_bucket_max, sizeof(tommy_hashdyn_node*)));

			for (tommy_size_t i = 0; i < bucket_max; ++i) {
				tommy_hashdyn_node* j = hashdyn->bucket[i];

				if (j && !j->next) {
					/* growth keeps old buckets separate, and singleton links are already valid */
					new_bucket[j->index & new_bucket_mask] = j;
					continue;
				}
				while (j) {
					tommy_hashdyn_node* j_next = j->next;
					tommy_size_t pos = j->index & new_bucket_mask;
					if (new_bucket[pos])
						tommy_list_insert_tail_not_empty(new_bucket[pos], j);
					else
						tommy_list_insert_first(&new_bucket[pos], j);
					j = j_next;
				}
			}
		}
	} else {
		/* all buckets are overwritten, no pre-zeroing needed */
		new_bucket = tommy_cast(tommy_hashdyn_node**, tommy_malloc(new_bucket_max * sizeof(tommy_hashdyn_node*)));

		if (new_bucket_bit + 1 == bucket_bit) {
			/* shrink by 1 bit: each new bucket joins exactly two old buckets */
			for (tommy_size_t i = 0; i < new_bucket_max; ++i) {
				new_bucket[i] = hashdyn->bucket[i];
				tommy_list_concat(&new_bucket[i], &hashdyn->bucket[i + new_bucket_max]);
			}
		} else {
			/* shrink by multiple bits */
			for (tommy_size_t i = 0; i < new_bucket_max; ++i) {
				/* all old buckets with the same new modulus must be concatenated. */
				new_bucket[i] = hashdyn->bucket[i];
				for (tommy_size_t j = i + new_bucket_max; j < bucket_max; j += new_bucket_max)
					tommy_list_concat(&new_bucket[i], &hashdyn->bucket[j]);
			}
		}
	}

	tommy_free(hashdyn->bucket);

	/* setup */
	hashdyn->bucket_bit = new_bucket_bit;
	hashdyn->bucket_max = new_bucket_max;
	hashdyn->bucket_mask = new_bucket_mask;
	hashdyn->bucket = new_bucket;
}

/**
 * Grow.
 */
tommy_inline void tommy_imp_hashdyn_grow_step(tommy_hashdyn* hashdyn)
{
	/* grow if at least 50% full */
	if (hashdyn->count >= hashdyn->bucket_max / 2)
		tommy_imp_hashdyn_resize(hashdyn, hashdyn->bucket_bit + 1);
}

/**
 * Shrink.
 */
tommy_inline void tommy_imp_hashdyn_shrink_step(tommy_hashdyn* hashdyn)
{
	/* shrink if at most 12.5% full */
	if (hashdyn->count <= hashdyn->bucket_max / 8 && hashdyn->bucket_bit > TOMMY_HASHDYN_BIT)
		tommy_imp_hashdyn_resize(hashdyn, hashdyn->bucket_bit - 1);
}

TOMMY_API void tommy_hashdyn_reserve(tommy_hashdyn* hashdyn, tommy_size_t count)
{
	tommy_uint_t bucket_bit = tommy_ilog2(count | 1) + 2;

	if (bucket_bit < TOMMY_HASHDYN_BIT)
		bucket_bit = TOMMY_HASHDYN_BIT;

	if (bucket_bit <= hashdyn->bucket_bit)
		return;

	tommy_imp_hashdyn_resize(hashdyn, bucket_bit);
}

TOMMY_API void tommy_hashdyn_shrink(tommy_hashdyn* hashdyn)
{
	tommy_uint_t bucket_bit = tommy_ilog2(hashdyn->count | 1) + 2;

	if (bucket_bit < TOMMY_HASHDYN_BIT)
		bucket_bit = TOMMY_HASHDYN_BIT;

	if (bucket_bit >= hashdyn->bucket_bit)
		return;

	tommy_imp_hashdyn_resize(hashdyn, bucket_bit);
}

TOMMY_API void tommy_hashdyn_insert(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node, void* data, tommy_hash_t hash)
{
	tommy_size_t pos = hash & hashdyn->bucket_mask;

	tommy_list_insert_tail(&hashdyn->bucket[pos], node, data);

	node->index = hash;

	++hashdyn->count;

	tommy_imp_hashdyn_grow_step(hashdyn);
}

TOMMY_API void* tommy_hashdyn_insert_unique(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node, void* data, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash)
{
	tommy_hashdyn_node** bucket = &hashdyn->bucket[hash & hashdyn->bucket_mask];
	tommy_hashdyn_node* i = *bucket;

	while (i) {
		/* compare keys only for matching full hashes, as the bucket may contain different hashes */
		if (i->index == hash && cmp(cmp_arg, i->data) == 0)
			return i->data;
		i = i->next;
	}

	/* insert before resizing, while the bucket selected for the search is still valid */
	tommy_list_insert_tail(bucket, node, data);

	node->index = hash;

	++hashdyn->count;

	tommy_imp_hashdyn_grow_step(hashdyn);

	return data;
}

TOMMY_API void tommy_hashdyn_rehash_existing(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node, tommy_hash_t hash)
{
	if (node->index == hash)
		return;

	/* unlink using the stored hash, without invoking the resize policy. */
	tommy_list_remove_existing(&hashdyn->bucket[node->index & hashdyn->bucket_mask], node);
	node->index = hash;
	tommy_list_insert_tail(&hashdyn->bucket[hash & hashdyn->bucket_mask], node, node->data);
}

TOMMY_API void* tommy_hashdyn_remove_existing(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node)
{
	tommy_size_t pos = node->index & hashdyn->bucket_mask;

	tommy_list_remove_existing(&hashdyn->bucket[pos], node);

	--hashdyn->count;

	tommy_imp_hashdyn_shrink_step(hashdyn);

	return node->data;
}

TOMMY_API void* tommy_hashdyn_remove(tommy_hashdyn* hashdyn, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash)
{
	tommy_size_t pos = hash & hashdyn->bucket_mask;
	tommy_hashdyn_node* node = hashdyn->bucket[pos];

	while (node) {
		/* we first check if the hash matches, as in the same bucket we may have multiples hash values */
		if (node->index == hash && cmp(cmp_arg, node->data) == 0) {
			tommy_list_remove_existing(&hashdyn->bucket[pos], node);

			--hashdyn->count;

			tommy_imp_hashdyn_shrink_step(hashdyn);

			return node->data;
		}
		node = node->next;
	}

	return 0;
}

TOMMY_API void tommy_hashdyn_foreach(tommy_hashdyn* hashdyn, tommy_foreach_func* func)
{
	tommy_size_t bucket_max = hashdyn->bucket_max;
	tommy_hashdyn_node** bucket = hashdyn->bucket;

	for (tommy_size_t pos = 0; pos < bucket_max; ++pos) {
		tommy_hashdyn_node* node = bucket[pos];

		while (node) {
			void* data = node->data;
			node = node->next;
			func(data);
		}
	}
}

TOMMY_API void tommy_hashdyn_foreach_arg(tommy_hashdyn* hashdyn, tommy_foreach_arg_func* func, void* arg)
{
	tommy_size_t bucket_max = hashdyn->bucket_max;
	tommy_hashdyn_node** bucket = hashdyn->bucket;

	for (tommy_size_t pos = 0; pos < bucket_max; ++pos) {
		tommy_hashdyn_node* node = bucket[pos];

		while (node) {
			void* data = node->data;
			node = node->next;
			func(arg, data);
		}
	}
}

TOMMY_API tommy_size_t tommy_hashdyn_memory_usage(const tommy_hashdyn* hashdyn)
{
	return hashdyn->bucket_max * (tommy_size_t)sizeof(hashdyn->bucket[0])
	       + tommy_hashdyn_count(hashdyn) * (tommy_size_t)sizeof(tommy_hashdyn_node);
}

TOMMY_API void tommy_hashdyn_to_list(tommy_hashdyn* hashdyn, tommy_list* list)
{
	tommy_builder builder;
	tommy_node* builder_tail = tommy_builder_init(&builder);

	/* clear buckets as they are transferred to avoid a second scan */
	for (tommy_size_t pos = 0; pos < hashdyn->bucket_max; ++pos) {
		tommy_node* head = hashdyn->bucket[pos];
		if (head) {
			tommy_node* tail = head->prev;
			builder_tail = tommy_builder_concat(builder_tail, head, tail);
		}
		hashdyn->bucket[pos] = 0;
	}

	tommy_list_concat_builder(list, &builder, builder_tail);
	hashdyn->count = 0;
}

/**
 * Compares hashes in least-significant-bit-first order, with 0 before 1.
 *
 * The first differing bit decides the order: first precedes second when
 * second has a 1 there. Equal hashes compare false.
 */
tommy_inline tommy_bool_t tommy_imp_hashdyn_before(tommy_hash_t first, tommy_hash_t second)
{
	/* mark the positions where the hashes differ using XOR */
	tommy_hash_t diff = first ^ second;

	/* isolate the lowest differing bit using two's complement */
	tommy_hash_t bit = diff & -diff;

	/* a 1 in second means first has 0 there; equal hashes yield bit == 0 */
	return (second & bit) != 0;
}

/**
 * Gets the adjacent active bucket in least-significant-bit-first order.
 * Returns bucket_max when there is no adjacent bucket.
 *
 * Traverses the virtual binary tree of buckets in bit-reversed order.
 *
 * For example, with 8 buckets the forward traversal order is:
 *   0, 4, 2, 6, 1, 5, 3, 7
 *
 * When resizing to 16 buckets:
 *   0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15
 */
tommy_inline tommy_size_t tommy_imp_hashdyn_step(tommy_hashdyn* hashdyn, tommy_size_t pos, tommy_bool_t reverse)
{
	tommy_size_t bit = (hashdyn->bucket_mask + 1) / 2;

	/* propagate binary carry in bit-reversed order from MSB downwards */
	while (bit && ((pos & bit) != 0) != reverse) {
		pos ^= bit;
		bit >>= 1;
	}

	/* carry wrapped around the root of the tree: end of traversal */
	if (!bit)
		return hashdyn->bucket_max;
	pos ^= bit;

	return pos;
}

/**
 * Finds the first or last node starting at an active bucket.
 *
 * Scans buckets in least-significant-bit-first order until a non-empty bucket
 * is found, then returns its extreme element (minimum for forward, maximum for reverse).
 */
tommy_inline tommy_hashdyn_node* tommy_imp_hashdyn_limit(tommy_hashdyn* hashdyn, tommy_size_t pos, tommy_bool_t reverse)
{
	while (pos != hashdyn->bucket_max) {
		tommy_hashdyn_node* best = 0;
		for (tommy_hashdyn_node* i = hashdyn->bucket[pos]; i; i = i->next) {
			/* equal hashes retain insertion order, so the reverse limit selects the last */
			if (reverse) {
				if (!best || !tommy_imp_hashdyn_before(i->index, best->index))
					best = i;
			} else {
				if (!best || tommy_imp_hashdyn_before(i->index, best->index))
					best = i;
			}
		}
		if (best)
			return best;
		pos = tommy_imp_hashdyn_step(hashdyn, pos, reverse);
	}
	return 0;
}

/**
 * Finds a neighbor using the current bucket layout and a resize-independent order.
 *
 * First searches for the closest neighbor within the current bucket.
 * If none exists, advances to the next non-empty active bucket in LSB order.
 */
tommy_inline tommy_hashdyn_node* tommy_imp_hashdyn_neighbor(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node, tommy_bool_t reverse)
{
	tommy_size_t pos = node->index & hashdyn->bucket_mask;
	tommy_hashdyn_node* best = 0;
	tommy_bool_t passed = 0;

	/*
	 * Bucket links can change during grow and shrink. Compare full hashes instead
	 * of following those links as iteration order, so no remaining node moves
	 * behind the iterator. Equal hashes always stay in one bucket in insertion order.
	 */
	for (tommy_hashdyn_node* i = hashdyn->bucket[pos]; i; i = i->next) {
		if (i == node) {
			passed = 1;
		} else if (i->index == node->index) {
			/* for identical hashes, follow insertion order relative to the current node */
			if (reverse && !passed)
				best = i;
			else if (!reverse && passed)
				return i;
		} else if (reverse) {
			if (tommy_imp_hashdyn_before(i->index, node->index)) {
				/* find the closest neighbor within the bucket in LSB order */
				if (!best || !tommy_imp_hashdyn_before(i->index, best->index))
					best = i;
			}
		} else {
			if (tommy_imp_hashdyn_before(node->index, i->index)) {
				/* find the closest neighbor within the bucket in LSB order */
				if (!best || tommy_imp_hashdyn_before(i->index, best->index))
					best = i;
			}
		}
	}
	if (best)
		return best;

	/* bucket exhausted: advance to the next non-empty bucket in LSB order */
	return tommy_imp_hashdyn_limit(hashdyn, tommy_imp_hashdyn_step(hashdyn, pos, reverse), reverse);
}

TOMMY_API tommy_hashdyn_node* tommy_hashdyn_head(tommy_hashdyn* hashdyn)
{
	/* start forward traversal at bucket 0, which has all LSB bits at zero */
	return tommy_imp_hashdyn_limit(hashdyn, 0, 0);
}

TOMMY_API tommy_hashdyn_node* tommy_hashdyn_tail(tommy_hashdyn* hashdyn)
{
	/* start reverse traversal at the all-ones bucket, which has the highest LSB value */
	return tommy_imp_hashdyn_limit(hashdyn, hashdyn->bucket_mask, 1);
}

TOMMY_API tommy_hashdyn_node* tommy_hashdyn_next(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node)
{
	/* adjacent equal hashes are consecutive in insertion order */
	if (node->next && node->next->index == node->index)
		return node->next;

	return tommy_imp_hashdyn_neighbor(hashdyn, node, 0);
}

TOMMY_API tommy_hashdyn_node* tommy_hashdyn_prev(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node)
{
	/* equal hashes follow insertion order, but the bucket head's prev points to the tail */
	if (node != hashdyn->bucket[node->index & hashdyn->bucket_mask] && node->prev->index == node->index)
		return node->prev;

	return tommy_imp_hashdyn_neighbor(hashdyn, node, 1);
}

