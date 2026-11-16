// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2010 Andrea Mazzoleni

/** \file
 * Generic types.
 */

#ifndef TOMMY_TYPES_H
#define TOMMY_TYPES_H

/******************************************************************************/
/* types */

#include <stddef.h>

#ifdef _MSC_VER
#include <limits.h>
typedef unsigned tommy_uint32_t; /**< Generic uint32_t type. */
typedef unsigned _int64 tommy_uint64_t; /**< Generic uint64_t type. */
typedef size_t tommy_uintptr_t; /**< Generic uintptr_t type. */
#ifdef _WIN64
#define TOMMY_SIZE_BIT 64
#define TOMMY_SIZE_MAX _UI64_MAX
typedef unsigned _int64 tommy_size_t; /**< Generic size_t type. */
typedef _int64 tommy_ssize_t; /**< Generic ssize_t type. */
#else
#define TOMMY_SIZE_BIT 32
#define TOMMY_SIZE_MAX UINT_MAX
typedef unsigned tommy_size_t; /**< Generic size_t type. */
typedef int tommy_ssize_t; /**< Generic ssize_t type. */
#endif
#else
#include <stdint.h>
typedef uint32_t tommy_uint32_t; /**< Generic uint32_t type. */
typedef uint64_t tommy_uint64_t; /**< Generic uint64_t type. */
typedef uintptr_t tommy_uintptr_t; /**< Generic uintptr_t type. */
#if SIZE_MAX == UINT64_MAX
#define TOMMY_SIZE_BIT 64
#define TOMMY_SIZE_MAX SIZE_MAX
typedef uint64_t tommy_size_t; /**< Generic size_t type. */
typedef int64_t tommy_ssize_t; /**< Generic ssize_t type. */
#elif SIZE_MAX == UINT32_MAX
#define TOMMY_SIZE_BIT 32
#define TOMMY_SIZE_MAX SIZE_MAX
typedef uint32_t tommy_size_t; /**< Generic size_t type. */
typedef int32_t tommy_ssize_t; /**< Generic ssize_t type. */
#else
#error Unsupported SIZE_MAX
#endif
#endif

typedef ptrdiff_t tommy_ptrdiff_t; /**< Generic ptrdiff_t type. */
typedef int tommy_bool_t; /**< Generic boolean type. */

/**
 * Generic unsigned integer type.
 *
 * It has no specific size, as it is used to store only small values.
 * To make the code more efficient, a full 32-bit integer is used.
 */
typedef tommy_uint32_t tommy_uint_t;

/** \internal
 * Explicit cast helper used to smooth out the difference between C and C++.
 *
 * In C, a void* can be assigned to any pointer type without an explicit cast.
 * This is not allowed in C++, where converting a void* to another pointer type
 * always requires an explicit cast.
 *
 * tommy_cast should be used only for this specific purpose, so it should be
 * applied when converting a void* to a more specific pointer type in code that
 * must compile as both C and C++. It is not intended for arbitrary pointer
 * conversions or for casts that are unrelated to the C to C++ difference.
 */
#ifdef __cplusplus
#define tommy_cast(type, value) static_cast<type>(value)
#else
#define tommy_cast(type, value) (value)
#endif

/******************************************************************************/
/* heap */

/* by default uses malloc/calloc/realloc/free */

/**
 * Generic malloc(), calloc(), realloc() and free() functions.
 *
 * Redefine them to what you need. By default they map to the C malloc(),
 * calloc(), realloc() and free().
 *
 * \par Infallible Allocation Invariant
 * Tommy assumes that dynamic memory allocation never fails and that these
 * functions always return a valid, non-zero pointer. The library does not
 * check for allocation failures and does not return error codes for
 * out-of-memory (OOM) conditions.
 *
 * \par Rationale
 * This choice reflects the memory model of modern general-purpose operating
 * systems:
 * - **Virtual Memory Overcommit:** On modern systems (such as Linux with default
 *   overcommit heuristics), malloc() reserves virtual address space and almost
 *   never returns 0 on 64-bit systems. Physical memory is committed on first write,
 *   meaning actual exhaustion triggers the kernel OOM Killer (SIGKILL) rather
 *   than an allocation failure. Checking for null returns in userspace often
 *   provides a false sense of security.
 * - **Swap Thrashing:** In systems backed by swap, severe memory pressure causes
 *   destructive disk thrashing, degrading throughput by orders of magnitude and
 *   rendering the process unusable long before an allocation call fails.
 *
 * \par Advantages
 * - **Zero Error-Path Overhead:** No branch predictions, conditional jumps, or
 *   status checks inside critical insertion and resizing loops, maximizing CPU
 *   cache efficiency and execution speed.
 * - **Clean and Lean API:** Functions (such as insertion and resizing) can remain
 *   simple (often returning void or direct element pointers) without cluttering
 *   client code with error-propagation boilerplate.
 * - **Deterministic Consistency:** Avoids complex transactional rollback logic
 *   otherwise required if an allocation failed halfway through dynamic table
 *   growth or node migration.
 * - **Industry Alignment:** Follows the established design of high-performance
 *   systems libraries (e.g., GLib's g_malloc, Git's xmalloc, Redis's zmalloc,
 *   and default C++ operator new).
 *
 * \par Trade-offs and Limitations
 * - **Embedded and Bare-Metal Systems:** Systems without an MMU/virtual memory
 *   (e.g., RTOS, microcontrollers) or strict overcommit disabled (vm.overcommit_memory=2)
 *   do return 0 on heap exhaustion.
 * - **Multi-Tenant Services:** Infrastructure daemons (e.g., database engines) that
 *   must isolate a single client's excessive memory request without crashing the
 *   entire process cannot rely on unchecked allocations.
 *
 * \par Custom Interception
 * If your environment requires custom handling (such as memory tracking, structured
 * aborts, or logging), redefine tommy_malloc, tommy_calloc, tommy_realloc, and
 * tommy_free before including Tommy headers (or via compiler flags).
 * Custom allocators must terminate the process (e.g., abort()) or initiate an
 * unwinding non-local exit (e.g., longjmp or C++ exception) rather than returning 0.
 */
#if !defined(tommy_malloc) || !defined(tommy_calloc) || !defined(tommy_realloc) || !defined(tommy_free)
#include <stdlib.h>
#endif
#if !defined(tommy_malloc)
#define tommy_malloc malloc
#endif
#if !defined(tommy_calloc)
#define tommy_calloc calloc
#endif
#if !defined(tommy_realloc)
#define tommy_realloc realloc
#endif
#if !defined(tommy_free)
#define tommy_free free
#endif

/******************************************************************************/
/* modifiers */

/** \internal
 * Definition of TOMMY_API.
 * Provide the ability to override linkage features of the interface.
 */
#if !defined(TOMMY_API)
#define TOMMY_API
#endif

/** \internal
 * Definition of the assert().
 */
#if !defined(tommy_assert)
#define tommy_assert assert
#endif

/** \internal
 * Definition of the inline keyword if available.
 */
#if !defined(tommy_inline)
#if defined(_MSC_VER) || defined(__GNUC__)
#define tommy_inline static __inline
#else
#define tommy_inline static
#endif
#endif

/** \internal
 * Definition of the restrict keyword if available.
 */
#if !defined(tommy_restrict)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define tommy_restrict restrict
#elif defined(_MSC_VER) || defined(__GNUC__)
#define tommy_restrict __restrict
#else
#define tommy_restrict
#endif
#endif

/** \internal
 * Hints the compiler that a condition is likely true.
 */
#if !defined(tommy_likely)
#if defined(__GNUC__)
#define tommy_likely(x) __builtin_expect(!!(x), 1)
#else
#define tommy_likely(x) (x)
#endif
#endif

/** \internal
 * Hints the compiler that a condition is likely false.
 */
#if !defined(tommy_unlikely)
#if defined(__GNUC__)
#define tommy_unlikely(x) __builtin_expect(!!(x), 0)
#else
#define tommy_unlikely(x) (x)
#endif
#endif

/******************************************************************************/
/* key/hash */

/**
 * Type used in indexed data structures to store the key of an element.
 */
typedef tommy_size_t tommy_key_t;

/**
 * Type used in hashtables to store the hash of an element.
 */
typedef tommy_size_t tommy_hash_t;

/******************************************************************************/
/* node */

/**
 * Data structure node.
 * This node type is shared between all the data structures and used to store some
 * info directly into the elements you want to store.
 *
 * A typical declaration is:
 * \code
 * struct element {
 *     tommy_node node;
 *     // other fields
 * };
 * \endcode
 */
typedef struct tommy_node_struct {
	/**
	 * Next node.
	 * The tail node has it at 0, like a 0-terminated list.
	 */
	struct tommy_node_struct* next;

	/**
	 * Previous node.
	 * The head node points to the tail node, like a circular list.
	 */
	struct tommy_node_struct* prev;

	/**
	 * Pointer to the element containing the node.
	 * This field is initialized when inserting nodes into a data structure.
	 * The element pointer passed to insertion functions must not be 0.
	 */
	void* data;

	/**
	 * Index of the node.
	 * With tries this field is used to store the key.
	 * With hashtables this field is used to store the hash value.
	 * With trees this field is used to store the parent pointer and balance factor.
	 * With lists this field is not used.
	 */
	tommy_size_t index;
} tommy_node;

/******************************************************************************/
/* compare */

/**
 * Compare function for elements.
 *
 * This function is like the C strcmp().
 *
 * \code
 * struct element {
 *     tommy_node node;
 *     int value;
 * };
 *
 * int compare(const void* elem_a, const void* elem_b)
 * {
 *     if (((const struct element*)elem_a)->value < ((const struct element*)elem_b)->value)
 *         return -1;
 *     if (((const struct element*)elem_a)->value > ((const struct element*)elem_b)->value)
 *         return 1;
 *     return 0;
 * }
 *
 * tommy_list_sort(&list, compare);
 * \endcode
 *
 * \param elem_a Pointer to the first element to compare.
 * \param elem_b Pointer to the second element to compare.
 * \return <0 if the first element is less than the second, ==0 if equal, >0 if greater.
 */
typedef int tommy_compare_func(const void* elem_a, const void* elem_b);

/**
 * Search function for elements.
 *
 * \code
 * struct element {
 *     tommy_node node;
 *     int value;
 * };
 *
 * int compare(const void* arg, const void* elem)
 * {
 *     const int* value_to_find = arg;
 *     const struct element* element_to_compare = elem;
 *
 *     return *value_to_find != element_to_compare->value;
 * }
 *
 * int value_to_find = 1;
 * struct element* elem = tommy_hashtable_search(&hashtable, compare, &value_to_find, tommy_inthash_u32(value_to_find));
 * if (!elem) {
 *     // not found
 * } else {
 *     // found
 * }
 * \endcode
 *
 * \param arg Pointer to the value to search as passed to the search function.
 * \param elem Pointer to the element to compare to.
 * \return ==0 if the value matches the element. !=0 if different.
 */
typedef int tommy_search_func(const void* arg, const void* elem);

/**
 * Foreach function.
 *
 * A typical example is to use free() to deallocate all the elements in a list.
 * \code
 * tommy_list_foreach(&list, free);
 * \endcode
 *
 * \param elem Pointer to the element to iterate.
 */
typedef void tommy_foreach_func(void* elem);

/**
 * Foreach function with an argument.
 * \param arg Pointer to a generic argument.
 * \param elem Pointer to the element to iterate.
 */
typedef void tommy_foreach_arg_func(void* arg, void* elem);

/******************************************************************************/
/* bit hacks */

#if defined(_MSC_VER) && !defined(__cplusplus)
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)
#if TOMMY_SIZE_BIT == 64
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_BitScanForward64)
#endif
#endif

/** \internal
 * Integer log2 for constants.
 * You can use it only for exact powers of 2 up to 256.
 */
#define TOMMY_ILOG2(value) ((value) == 256 ? 8 : (value) == 128 ? 7 : (value) == 64 ? 6 : (value) == 32 ? 5 : (value) == 16 ? 4 : (value) == 8 ? 3 : (value) == 4 ? 2 : (value) == 2 ? 1 : 0)

/**
 * Bit scan reverse or integer log2.
 * Return the bit index of the most significant 1 bit.
 *
 * If no bit is set, the result is undefined.
 * To force a return 0 in this case, you can use tommy_ilog2_u32(value | 1).
 *
 * Other interesting ways for bitscan are at:
 *
 * Bit Twiddling Hacks
 * http://graphics.stanford.edu/~seander/bithacks.html
 *
 * Chess Programming BitScan
 * http://chessprogramming.wikispaces.com/BitScan
 *
 * \param value Value to scan. 0 is not allowed.
 * \return The index of the most significant bit set.
 */
tommy_inline tommy_uint_t tommy_ilog2_u32(tommy_uint32_t value)
{
#if defined(_MSC_VER)
	unsigned long count;
	_BitScanReverse(&count, value);
	return count;
#elif defined(__GNUC__)
	/*
	 * GCC implements __builtin_clz(x) as "__builtin_clz(x) = bsr(x) ^ 31"
	 *
	 * Where "x ^ 31 = 31 - x", but gcc does not optimize "31 - __builtin_clz(x)" to bsr(x),
	 * but generates 31 - (bsr(x) xor 31).
	 *
	 * So we write "__builtin_clz(x) ^ 31" instead of "31 - __builtin_clz(x)",
	 * to allow the double xor to be optimized out.
	 */
	return __builtin_clz(value) ^ 31;
#else
	/* find the log base 2 of an N-bit integer in O(lg(N)) operations with multiply and lookup */
	/* from http://graphics.stanford.edu/~seander/bithacks.html */
	static const unsigned char TOMMY_DE_BRUIJN_INDEX_ILOG2[32] = {
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;

	return TOMMY_DE_BRUIJN_INDEX_ILOG2[(tommy_uint32_t)(value * 0x07C4ACDDU) >> 27];
#endif
}

/**
 * Bit scan reverse or integer log2 for 64 bits.
 * Return the bit index of the most significant 1 bit.
 *
 * If no bit is set, the result is undefined.
 * \param value Value to scan. 0 is not allowed.
 * \return The index of the most significant bit set.
 */
tommy_inline tommy_uint_t tommy_ilog2_u64(tommy_uint64_t value)
{
#if defined(_MSC_VER) && defined(_WIN64)
	unsigned long count;
	_BitScanReverse64(&count, value);
	return count;
#elif defined(__GNUC__)
	return __builtin_clzll(value) ^ 63;
#else
	tommy_uint32_t l = value & 0xFFFFFFFFU;
	tommy_uint32_t h = value >> 32;
	if (h)
		return tommy_ilog2_u32(h) + 32;
	else
		return tommy_ilog2_u32(l);
#endif
}

/**
 * Bit scan forward or trailing zero count.
 * Return the bit index of the least significant 1 bit.
 *
 * If no bit is set, the result is undefined.
 * \param value Value to scan. 0 is not allowed.
 * \return The index of the least significant bit set.
 */
tommy_inline tommy_uint_t tommy_ctz_u32(tommy_uint32_t value)
{
#if defined(_MSC_VER)
	unsigned long count;
	_BitScanForward(&count, value);
	return count;
#elif defined(__GNUC__)
	return __builtin_ctz(value);
#else
	/* count the consecutive zero bits (trailing) on the right with multiply and lookup */
	/* from http://graphics.stanford.edu/~seander/bithacks.html */
	static const unsigned char TOMMY_DE_BRUIJN_INDEX_CTZ[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

	return TOMMY_DE_BRUIJN_INDEX_CTZ[(tommy_uint32_t)(((value & -value) * 0x077CB531U)) >> 27];
#endif
}

/**
 * Bit scan forward or trailing zero count for 64 bits.
 * Return the bit index of the least significant 1 bit.
 *
 * If no bit is set, the result is undefined.
 * \param value Value to scan. 0 is not allowed.
 * \return The index of the least significant bit set.
 */
tommy_inline tommy_uint_t tommy_ctz_u64(tommy_uint64_t value)
{
#if defined(_MSC_VER) && defined(_WIN64)
	unsigned long count;
	_BitScanForward64(&count, value);
	return count;
#elif defined(__GNUC__)
	return __builtin_ctzll(value);
#else
	tommy_uint32_t l = value & 0xFFFFFFFFU;
	tommy_uint32_t h = value >> 32;
	if (l)
		return tommy_ctz_u32(l);
	else
		return tommy_ctz_u32(h) + 32;
#endif
}

/**
 * Rounds up to the next power of 2.
 * For the value 0, the result is undefined.
 * \param value Value to round up.
 * \return The smallest power of 2 not less than the specified value.
 */
tommy_inline tommy_uint32_t tommy_roundup_pow2_u32(tommy_uint32_t value)
{
	/* round up to the next highest power of 2 */
	/* from http://graphics.stanford.edu/~seander/bithacks.html */

	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	++value;

	return value;
}

/**
 * Rounds up to the next power of 2 for 64 bits.
 * For the value 0, the result is undefined.
 * \param value Value to round up.
 * \return The smallest power of 2 not less than the specified value.
 */
tommy_inline tommy_uint64_t tommy_roundup_pow2_u64(tommy_uint64_t value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value |= value >> 32;
	++value;

	return value;
}

/**
 * Checks if the specified word has a byte at 0.
 * \param value Value to check.
 * \return 0 or 1.
 */
tommy_inline int tommy_haszero_u32(tommy_uint32_t value)
{
	return ((value - 0x01010101) & ~value & 0x80808080) != 0;
}

/**
 * Checks if the specified 64-bit word has a byte at 0.
 * \param value Value to check.
 * \return 0 or 1.
 */
tommy_inline int tommy_haszero_u64(tommy_uint64_t value)
{
	return ((value - 0x0101010101010101ULL) & ~value & 0x8080808080808080ULL) != 0;
}

/**
 * Bit depth mapping.
 */
#if TOMMY_SIZE_BIT == 64
#define tommy_ilog2 tommy_ilog2_u64
#define tommy_ctz tommy_ctz_u64
#define tommy_roundup_pow2 tommy_roundup_pow2_u64
#else
#define tommy_ilog2 tommy_ilog2_u32
#define tommy_ctz tommy_ctz_u32
#define tommy_roundup_pow2 tommy_roundup_pow2_u32
#endif

#endif

