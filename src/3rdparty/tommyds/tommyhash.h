// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2010 Andrea Mazzoleni

/** \file
 * Hash functions for use with ::tommy_hashtable, ::tommy_hashdyn and ::tommy_hashlin.
 *
 * This module provides hash functions designed for in-memory data structures.
 * ::tommy_hash_u32 and ::tommy_strhash_u32 derive from a5hash32, while
 * ::tommy_hash_u64 and ::tommy_strhash_u64 derive from a5hash.
 *
 * Both variants are modified to improve adversarial collision resistance under
 * the seeded collision-score metric described by Thomas Dybdahl Ahle in
 * "Adversarial examples for fast hash functions":
 * https://thomasahle.com/blog/adversarial-examples-for-hashes/
 * The original a5hash32 leaves one state lane independent of the seed, allowing
 * distinct inputs of 9 to 16 bytes to collide for every seed. The original
 * 64-bit a5hash can lose seed dependence during initialization, making specially
 * crafted inputs collide more often across seeds than a uniform 64-bit hash.
 * TommyDS changes initialization and mixing to address these weaknesses,
 * so its hash values differ from the original algorithms.
 *
 * Reversible integer hash functions are also provided for 32-bit and 64-bit keys.
 *
 * ::tommy_hash_u32 is intended for 32-bit platforms and uses 32-bit operands with
 * full 64-bit multiplication results. ::tommy_hash_u64 uses 64-bit operands with
 * full 128-bit multiplication results. Both algorithms pass SMHasher3 tests.
 *
 * Prefer the 64-bit version whenever possible, as it provides better resistance
 * to collisions, particularly with specially crafted inputs. However, neither
 * version is designed to resist deliberate collision attacks. If resistance to
 * such attacks is required, use a dedicated hash function designed for that purpose.
 *
 * All hash functions are intended for runtime in-memory use only and not
 * for persistent storage, as hash compatibility across different versions of the
 * library is not guaranteed. String and sized buffer hash functions produce
 * identical results for 0-terminated strings.
 */

#ifndef TOMMY_HASH_H
#define TOMMY_HASH_H

#include "tommytypes.h"
#include <string.h> /* for strlen */

/******************************************************************************/
/* hash */

/**
 * Hash function with a 32-bit result.
 * Derived from a5hash32 by Aleksey Vaneev from https://github.com/avaneev/a5hash
 * 
 * This algorithm is compact, fast on short inputs, and passes SMHasher3 tests.
 *
 * \param init_val Initialization value.
 * Using a different initialization value, you can generate a completely different set of hash values.
 * Use 0 if not relevant.
 * \param void_key Pointer to the data to hash. Must not be 0, even if key_len is 0.
 * \param key_len Size of the data to hash.
 * \return The 32-bit hash value.
 * \note
 * This function is endianness independent and alignment independent.
 * It produces identical hash results to ::tommy_strhash_u32 for 0-terminated strings.
 * Compatibility across different versions of the library is not guaranteed:
 * this hash is intended for in-memory runtime use only and not for persistent storage.
 * It is non-cryptographic and is not designed to protect against malicious collision attacks.
 */
TOMMY_API tommy_uint32_t tommy_hash_u32(tommy_uint32_t init_val, const void* void_key, tommy_size_t key_len);

/**
 * Hash function with a 64-bit result.
 * Derived from a5hash by Aleksey Vaneev from https://github.com/avaneev/a5hash
 *
 * This algorithm is compact, fast on short inputs, and passes SMHasher3 tests.
 *
 * \param init_val Initialization value.
 * Using a different initialization value, you can generate a completely different set of hash values.
 * Use 0 if not relevant.
 * \param void_key Pointer to the data to hash. Must not be 0, even if key_len is 0.
 * \param key_len Size of the data to hash.
 * \return The 64-bit hash value.
 * \note
 * This function is endianness independent and alignment independent.
 * It produces identical hash results to ::tommy_strhash_u64 for 0-terminated strings.
 * Compatibility across different versions of the library is not guaranteed:
 * this hash is intended for in-memory runtime use only and not for persistent storage.
 * It is non-cryptographic and is not designed to protect against malicious collision attacks.
 */
TOMMY_API tommy_uint64_t tommy_hash_u64(tommy_uint64_t init_val, const void* void_key, tommy_size_t key_len);

/**
 * String hash function with a 32-bit result.
 *
 * This hash is designed to handle strings with an unknown length.
 *
 * \param init_val Initialization value.
 * Using a different initialization value, you can generate a completely different set of hash values.
 * Use 0 if not relevant.
 * \param void_key Pointer to the string to hash. It has to be 0-terminated.
 * \return The 32-bit hash value.
 * \note
 * This function is endianness independent and alignment independent.
 * It produces identical hash results to ::tommy_hash_u32 for 0-terminated strings.
 * Compatibility across different versions of the library is not guaranteed:
 * this hash is intended for in-memory runtime use only and not for persistent storage.
 * It is non-cryptographic and is not designed to protect against malicious collision attacks.
 */
tommy_inline tommy_uint32_t tommy_strhash_u32(tommy_uint32_t init_val, const void* void_key)
{
	return tommy_hash_u32(init_val, void_key, strlen(tommy_cast(const char*, void_key)));
}

/**
 * String hash function with a 64-bit result.
 *
 * This hash is designed to handle strings with an unknown length.
 *
 * \param init_val Initialization value.
 * Using a different initialization value, you can generate a completely different set of hash values.
 * Use 0 if not relevant.
 * \param void_key Pointer to the string to hash. It has to be 0-terminated.
 * \return The 64-bit hash value.
 * \note
 * This function is endianness independent and alignment independent.
 * It produces identical hash results to ::tommy_hash_u64 for 0-terminated strings.
 * Compatibility across different versions of the library is not guaranteed:
 * this hash is intended for in-memory runtime use only and not for persistent storage.
 * It is non-cryptographic and is not designed to protect against malicious collision attacks.
 */
tommy_inline tommy_uint64_t tommy_strhash_u64(tommy_uint64_t init_val, const void* void_key)
{
	return tommy_hash_u64(init_val, void_key, strlen(tommy_cast(const char*, void_key)));
}

/**
 * Reversible 32-bit integer hash function.
 * Implementation of the Robert Jenkins "4-byte Integer Hashing",
 * from http://burtleburtle.net/bob/hash/integer.html
 * \param key The 32-bit integer to hash.
 * \return The 32-bit hash value.
 */
tommy_inline tommy_uint32_t tommy_inthash_u32(tommy_uint32_t key)
{
	key -= key << 6;
	key ^= key >> 17;
	key -= key << 9;
	key ^= key << 4;
	key -= key << 3;
	key ^= key << 10;
	key ^= key >> 15;

	return key;
}

/**
 * Reversible 64-bit integer hash function.
 * Implementation of the Thomas Wang "Integer Hash Function",
 * from http://web.archive.org/web/20071223173210/http://www.concentric.net/~Ttwang/tech/inthash.htm
 * \param key The 64-bit integer to hash.
 * \return The 64-bit hash value.
 */
tommy_inline tommy_uint64_t tommy_inthash_u64(tommy_uint64_t key)
{
	key = ~key + (key << 21);
	key = key ^ (key >> 24);
	key = key + (key << 3) + (key << 8);
	key = key ^ (key >> 14);
	key = key + (key << 2) + (key << 4);
	key = key ^ (key >> 28);
	key = key + (key << 31);

	return key;
}

/**
 * Reversible 32-bit integer hash function using multiplications.
 * Implementation of the MurmurHash3 fmix32 finalizer by Austin Appleby,
 * from https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
 * \param key The 32-bit integer to hash.
 * \return The 32-bit hash value.
 * \note Requires an architecture with fast 32-bit integer multiplication
 * for good performance. See tommy_inthash_u32() for an alternative.
 */
tommy_inline tommy_uint32_t tommy_mulhash_u32(tommy_uint32_t key)
{
	key ^= key >> 16;
	key *= 0x85ebca6bU;
	key ^= key >> 13;
	key *= 0xc2b2ae35U;
	key ^= key >> 16;

	return key;
}

/**
 * Reversible 64-bit integer hash function using multiplications.
 * Implementation of the SplitMix64 finalizer,
 * from https://prng.di.unimi.it/splitmix64.c
 * \param key The 64-bit integer to hash.
 * \return The 64-bit hash value.
 * \note Requires an architecture with fast 64-bit integer multiplication
 * for good performance. It is generally not recommended on 32-bit architectures,
 * where these multiplications are expensive. Consider tommy_inthash_u64() instead.
 */
tommy_inline tommy_uint64_t tommy_mulhash_u64(tommy_uint64_t key)
{
	key ^= key >> 30;
	key *= 0xbf58476d1ce4e5b9ULL;
	key ^= key >> 27;
	key *= 0x94d049bb133111ebULL;
	key ^= key >> 31;

	return key;
}

/******************************************************************************/
/* multiplication */

/**
 * Multiplies two 64-bit unsigned numbers to obtain the full 128-bit product.
 *
 * \param A Address of the first operand, overwritten with the low 64 bits of the product.
 * \param B Address of the second operand, overwritten with the high 64 bits of the product.
 */
#if defined(_MSC_VER) && defined(_M_X64)
/*
 * MSVC x64 does not support inline assembly, so use its full-product intrinsic.
 */
#pragma intrinsic(_umul128)
tommy_inline void tommy_mum(tommy_uint64_t* A, tommy_uint64_t* B)
{
	tommy_uint64_t high;
	tommy_uint64_t low = _umul128(*A, *B, &high);
	*A = low;
	*B = high;
}
#elif defined(_MSC_VER) && defined(_M_ARM64)
#pragma intrinsic(__umulh)
tommy_inline void tommy_mum(tommy_uint64_t* A, tommy_uint64_t* B)
{
	tommy_uint64_t a = *A;
	tommy_uint64_t b = *B;
	*A = a * b;
	*B = __umulh(a, b);
}
#elif defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
/**
 * Keep this before __SIZEOF_INT128__ because the explicit mulq generates more efficient code.
 */
tommy_inline void tommy_mum(tommy_uint64_t* A, tommy_uint64_t* B)
{
	tommy_uint64_t a = *A;
	tommy_uint64_t b = *B;
	tommy_uint64_t low, high;

	/* require a register to avoid Clang spilling the operand before mulq */
	__asm__ (
		"mulq %3"
		: "=a" (low), "=d" (high)
		: "a" (a), "r" (b)
		: "cc"
	);

	*A = low;
	*B = high;
}
#elif defined(__SIZEOF_INT128__)
tommy_inline void tommy_mum(tommy_uint64_t* A, tommy_uint64_t* B)
{
	__uint128_t product = (__uint128_t)*A * *B;
	*A = product;
	*B = product >> 64;
}
#elif defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))
/*
 * Keep this after __SIZEOF_INT128__ because the assembly path failed optimized
 * hash tests on Apple clang 21.0.0. See https://github.com/amadvance/snapraid/issues/43.
 */
tommy_inline void tommy_mum(tommy_uint64_t* A, tommy_uint64_t* B)
{
	tommy_uint64_t a = *A;
	tommy_uint64_t b = *B;
	tommy_uint64_t low = a * b;
	tommy_uint64_t high;
	__asm__ (
		"umulh %0, %1, %2"
		: "=r" (high)
		: "r" (a), "r" (b)
	);
	*A = low;
	*B = high;
}
#else
tommy_inline void tommy_mum(tommy_uint64_t* A, tommy_uint64_t* B)
{
	tommy_uint64_t a = *A;
	tommy_uint64_t b = *B;
	tommy_uint64_t a_lo = (tommy_uint32_t)a;
	tommy_uint64_t a_hi = a >> 32;
	tommy_uint64_t b_lo = (tommy_uint32_t)b;
	tommy_uint64_t b_hi = b >> 32;
	tommy_uint64_t p0 = a_lo * b_lo;
	tommy_uint64_t p1 = a_lo * b_hi;
	tommy_uint64_t p2 = a_hi * b_lo;
	tommy_uint64_t p3 = a_hi * b_hi;

	/* retain the carry from the middle words in the high half */
	tommy_uint64_t middle = p1 + (p0 >> 32) + (tommy_uint32_t)p2;
	tommy_uint64_t low = (middle << 32) | (tommy_uint32_t)p0;
	tommy_uint64_t high = p3 + (middle >> 32) + (p2 >> 32);
	*A = low;
	*B = high;
}
#endif

#endif

