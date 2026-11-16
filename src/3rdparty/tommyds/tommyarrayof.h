// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2013 Andrea Mazzoleni

/** \file
 * Dynamic array based on segments of exponentially growing size.
 *
 * This array is able to grow dynamically upon request, without any reallocation.
 *
 * This is very similar to ::tommy_array, but it allows storing elements of any
 * size and not just pointers.
 *
 * The container allocates space for elements but never copies user data into
 * or out of it. tommy_arrayof_ref(), tommy_arrayof_tail(), and
 * tommy_arrayof_insert_tail() return an element's address so the caller can
 * read or write it directly.
 *
 * \note Only types with fundamental alignment requirements are supported.
 * Storage is allocated using calloc(), which does not guarantee the alignment
 * required by over-aligned types (e.g. _Alignas(32) or _Alignas(64)).
 * When storing typed elements, use sizeof(type) as the element size to
 * preserve alignment for every element.
 */

#ifndef TOMMY_ARRAYOF_H
#define TOMMY_ARRAYOF_H

#include "tommytypes.h"

#include <assert.h> /* for assert */
#include <string.h> /* for memset */

/******************************************************************************/
/* array */

/**
 * Initial and minimal size of the array expressed as a power of 2.
 * The initial size is 2^TOMMY_ARRAYOF_BIT.
 */
#define TOMMY_ARRAYOF_BIT 6

/**
 * Array container type.
 * \note Don't use internal fields directly, but access the container only using functions.
 */
typedef struct tommy_arrayof_struct {
	void* bucket[TOMMY_SIZE_BIT]; /**< Dynamic array of buckets. */
	tommy_size_t element_size; /**< Size of the stored element in bytes. */
	tommy_size_t bucket_max; /**< Number of buckets. */
	tommy_size_t count; /**< Number of initialized elements in the array. */
	tommy_uint_t bucket_bit; /**< Bits used in the bit mask. */
} tommy_arrayof;

/**
 * Initializes the array.
 * \param array The array.
 * \param element_size Size in bytes of the element to store in the array.
 */
TOMMY_API void tommy_arrayof_init(tommy_arrayof* array, tommy_size_t element_size);

/**
 * Deinitializes the array.
 * \param array The array.
 */
TOMMY_API void tommy_arrayof_done(tommy_arrayof* array);

/**
 * Allocates space for at least the specified number of elements.
 * The initialized size and existing elements are unchanged.
 * Existing element references remain valid.
 * \param array The array.
 * \param size Number of elements to reserve space for.
 */
TOMMY_API void tommy_arrayof_reserve(tommy_arrayof* array, tommy_size_t size);

/**
 * Grows the size up to the specified value.
 * All the new elements in the array are initialized with the 0 value.
 * \param array The array.
 * \param size New size of the array.
 */
tommy_inline void tommy_arrayof_grow(tommy_arrayof* array, tommy_size_t size)
{
	if (size > array->count) {
		array->count = size;

		if (size > array->bucket_max)
			tommy_arrayof_reserve(array, size);
	}
}

/**
 * Changes the initialized size, preserving the common prefix.
 * New elements are initialized to zero.
 * Reducing the size does not release allocated capacity.
 * \param array The array.
 * \param size New size of the array.
 */
TOMMY_API void tommy_arrayof_resize(tommy_arrayof* array, tommy_size_t size);

/**
 * Removes all elements, preserving the allocated capacity.
 * Call tommy_arrayof_shrink() after clearing to restore the initial capacity.
 * The array remains initialized and can be reused immediately.
 * \param array The array.
 */
tommy_inline void tommy_arrayof_clear(tommy_arrayof* array)
{
	tommy_arrayof_resize(array, 0);
}

/**
 * Releases unused allocated memory.
 * Preserves the size, values, and addresses of existing elements.
 * \param array The array.
 */
TOMMY_API void tommy_arrayof_shrink(tommy_arrayof* array);

/**
 * Gets a reference to the element at the specified position.
 * You must be sure that space for this position is already
 * allocated by calling tommy_arrayof_grow().
 * \param array The array.
 * \param pos Position of the element.
 * \return Pointer to the slot at the specified position.
 */
tommy_inline void* tommy_arrayof_ref(tommy_arrayof* array, tommy_size_t pos)
{
	tommy_assert(pos < array->count);

	/* get the highest bit set, in case of all 0, return 0 */
	tommy_uint_t bsr = tommy_ilog2(pos | 1);

	unsigned char* ptr = tommy_cast(unsigned char*, array->bucket[bsr]);

	return ptr + pos * array->element_size;
}

/**
 * Gets a reference to the last element.
 * The array must not be empty.
 * \param array The array.
 * \return Pointer to the last element.
 */
tommy_inline void* tommy_arrayof_tail(tommy_arrayof* array)
{
	tommy_assert(array->count != 0);
	return tommy_arrayof_ref(array, array->count - 1);
}

/**
 * Adds a zero-initialized element and returns its reference.
 * \param array The array.
 * \return Pointer to the new element.
 */
tommy_inline void* tommy_arrayof_insert_tail(tommy_arrayof* array)
{
	tommy_size_t pos = array->count;

	++array->count;
	if (array->count > array->bucket_max)
		tommy_arrayof_reserve(array, array->count);

	return tommy_arrayof_ref(array, pos);
}

/**
 * Removes the last element without copying it.
 * The array must not be empty. The removed slot is cleared.
 * \param array The array.
 */
tommy_inline void tommy_arrayof_remove_tail(tommy_arrayof* array)
{
	tommy_assert(array->count != 0);

	tommy_size_t pos = array->count - 1;
	void* ptr = tommy_arrayof_ref(array, pos);

	/* keep unused slots zero so growing within capacity needs no initialization */
	memset(ptr, 0, array->element_size);
	array->count = pos;
}

/**
 * Checks whether the array is empty.
 * \param array The array.
 * \return Nonzero if the array is empty, 0 otherwise.
 */
tommy_inline tommy_bool_t tommy_arrayof_empty(const tommy_arrayof* array)
{
	return array->count == 0;
}

/**
 * Gets the initialized size of the array.
 * \param array The array.
 * \return The initialized size of the array.
 */
tommy_inline tommy_size_t tommy_arrayof_size(const tommy_arrayof* array)
{
	return array->count;
}

/**
 * Gets the number of elements that fit without further allocation.
 * \param array The array.
 * \return The capacity of the array.
 */
tommy_inline tommy_size_t tommy_arrayof_capacity(const tommy_arrayof* array)
{
	return array->bucket_max;
}

/**
 * Exchanges two initialized arrays without copying their elements.
 * Element references remain valid and belong to the other array.
 * Passing the same array twice has no effect.
 * \param first The first array.
 * \param second The second array.
 * \note This operation is O(1).
 */
tommy_inline void tommy_arrayof_swap(tommy_arrayof* first, tommy_arrayof* second)
{
	/* keep record size with the storage it describes */
	tommy_arrayof tmp = *first;
	*first = *second;
	*second = tmp;
}

/**
 * Calls the specified function for each element in the array.
 *
 * You cannot add or remove elements, nor change the size of the array,
 * from inside the callback.
 *
 * Elements are stored inside the array: passing free() as the callback
 * is invalid. A callback may release resources owned by each element,
 * but must not free the element itself. Call tommy_arrayof_clear() before
 * reusing it, or tommy_arrayof_done() to release its storage.
 * \param array The array.
 * \param func Function to call with each element.
 */
TOMMY_API void tommy_arrayof_foreach(tommy_arrayof* array, tommy_foreach_func* func);

/**
 * Calls the specified function with an argument for each element in the array.
 *
 * \param array The array.
 * \param func Function to call with each element.
 * \param arg Argument to pass to the function.
 */
TOMMY_API void tommy_arrayof_foreach_arg(tommy_arrayof* array, tommy_foreach_arg_func* func, void* arg);

/**
 * Gets the size of allocated memory.
 * \param array The array.
 * \return Total memory allocated by the array, in bytes.
 */
TOMMY_API tommy_size_t tommy_arrayof_memory_usage(const tommy_arrayof* array);

#endif

