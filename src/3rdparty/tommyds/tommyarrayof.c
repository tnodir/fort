// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2013 Andrea Mazzoleni

#include "tommyarrayof.h"

#include <string.h> /* for memset */

/******************************************************************************/
/* array */

TOMMY_API void tommy_arrayof_init(tommy_arrayof* array, tommy_size_t element_size)
{
	/* fixed initial size */
	array->element_size = element_size;
	array->bucket_bit = TOMMY_ARRAYOF_BIT;
	array->bucket_max = (tommy_size_t)1 << array->bucket_bit;
	array->bucket[0] = tommy_calloc(array->bucket_max, array->element_size);
	for (tommy_uint_t i = 1; i < TOMMY_ARRAYOF_BIT; ++i)
		array->bucket[i] = array->bucket[0];

	array->count = 0;
}

TOMMY_API void tommy_arrayof_done(tommy_arrayof* array)
{
	tommy_free(array->bucket[0]);
	for (tommy_uint_t i = TOMMY_ARRAYOF_BIT; i < array->bucket_bit; ++i) {
		unsigned char* segment = tommy_cast(unsigned char*, array->bucket[i]);
		tommy_free(segment + ((tommy_ptrdiff_t)1 << i) * array->element_size);
	}
}

TOMMY_API void tommy_arrayof_reserve(tommy_arrayof* array, tommy_size_t size)
{
	while (size > array->bucket_max) {
		/* allocate one more segment */
		unsigned char* segment = tommy_cast(unsigned char*, tommy_calloc(array->bucket_max, array->element_size));

		/* store it adjusting the offset */
		/* no cast to ptrdiff_t is needed: unlike array indexing with unary minus, binary pointer subtraction directly offsets backwards with an unsigned operand */
		array->bucket[array->bucket_bit] = segment - array->bucket_max * array->element_size;

		++array->bucket_bit;
		array->bucket_max = (tommy_size_t)1 << array->bucket_bit;
	}
}

TOMMY_API void tommy_arrayof_resize(tommy_arrayof* array, tommy_size_t size)
{
	if (size >= array->count) {
		tommy_arrayof_grow(array, size);
		return;
	}

	/* clear the unused elements to maintain the invariant that unused slots are zero */
	tommy_size_t pos = size;
	while (pos < array->count) {
		tommy_uint_t bsr = tommy_ilog2(pos | 1);
		tommy_size_t seg_end = (bsr < TOMMY_ARRAYOF_BIT) ? ((tommy_size_t)1 << TOMMY_ARRAYOF_BIT) : ((tommy_size_t)1 << (bsr + 1));
		tommy_size_t chunk_end = array->count < seg_end ? array->count : seg_end;
		unsigned char* ptr = tommy_cast(unsigned char*, array->bucket[bsr]);

		memset(ptr + pos * array->element_size, 0, (chunk_end - pos) * array->element_size);
		pos = chunk_end;
	}

	array->count = size;
}

TOMMY_API void tommy_arrayof_shrink(tommy_arrayof* array)
{
	tommy_uint_t target_bucket_bit = TOMMY_ARRAYOF_BIT;

	if (array->count > (tommy_size_t)1 << TOMMY_ARRAYOF_BIT)
		target_bucket_bit = tommy_ilog2(array->count - 1) + 1;

	while (array->bucket_bit > target_bucket_bit) {
		--array->bucket_bit;
		unsigned char* segment = tommy_cast(unsigned char*, array->bucket[array->bucket_bit]);
		tommy_free(segment + ((tommy_ptrdiff_t)1 << array->bucket_bit) * array->element_size);
	}

	array->bucket_max = (tommy_size_t)1 << array->bucket_bit;
}

TOMMY_API void tommy_arrayof_foreach(tommy_arrayof* array, tommy_foreach_func* func)
{
	/* cache the element size to avoid reloading it after each callback */
	tommy_size_t element_size = array->element_size;
	tommy_size_t pos = 0;

	while (pos < array->count) {
		tommy_uint_t bsr = tommy_ilog2(pos | 1);
		tommy_size_t seg_end = (bsr < TOMMY_ARRAYOF_BIT) ? ((tommy_size_t)1 << TOMMY_ARRAYOF_BIT) : ((tommy_size_t)1 << (bsr + 1));
		tommy_size_t chunk_end = array->count < seg_end ? array->count : seg_end;
		unsigned char* ptr = tommy_cast(unsigned char*, array->bucket[bsr]);

		ptr += pos * element_size;

		while (pos < chunk_end) {
			func(ptr);
			ptr += element_size;
			++pos;
		}
	}
}

TOMMY_API void tommy_arrayof_foreach_arg(tommy_arrayof* array, tommy_foreach_arg_func* func, void* arg)
{
	/* cache the element size to avoid reloading it after each callback */
	tommy_size_t element_size = array->element_size;
	tommy_size_t pos = 0;

	while (pos < array->count) {
		tommy_uint_t bsr = tommy_ilog2(pos | 1);
		tommy_size_t seg_end = (bsr < TOMMY_ARRAYOF_BIT) ? ((tommy_size_t)1 << TOMMY_ARRAYOF_BIT) : ((tommy_size_t)1 << (bsr + 1));
		tommy_size_t chunk_end = array->count < seg_end ? array->count : seg_end;
		unsigned char* ptr = tommy_cast(unsigned char*, array->bucket[bsr]);

		ptr += pos * element_size;

		while (pos < chunk_end) {
			func(arg, ptr);
			ptr += element_size;
			++pos;
		}
	}
}

TOMMY_API tommy_size_t tommy_arrayof_memory_usage(const tommy_arrayof* array)
{
	return array->bucket_max * (tommy_size_t)array->element_size;
}

