// SPDX-License-Identifier: BSD-2-Clause AND MIT
// Copyright (C) 2010 Andrea Mazzoleni

#include "tommyhash.h"

/******************************************************************************/
/* hash */

#include <string.h> /* for memcpy */

/**
 * Swap endianness.
 * They are needed only if BigEndian.
 */
#if defined(__GNUC__)
#define tommy_swap32(x) __builtin_bswap32(x)
#define tommy_swap64(x) __builtin_bswap64(x)
#else
tommy_inline tommy_uint32_t tommy_swap32(tommy_uint32_t v)
{
	return ((v & 0xFF000000) >> 24) |
	       ((v & 0x00FF0000) >> 8) |
	       ((v & 0x0000FF00) << 8) |
	       ((v & 0x000000FF) << 24);
}

tommy_inline tommy_uint64_t tommy_swap64(tommy_uint64_t v)
{
	return ((v & 0xFF00000000000000ULL) >> 56) |
	       ((v & 0x00FF000000000000ULL) >> 40) |
	       ((v & 0x0000FF0000000000ULL) >> 24) |
	       ((v & 0x000000FF00000000ULL) >> 8) |
	       ((v & 0x00000000FF000000ULL) << 8) |
	       ((v & 0x0000000000FF0000ULL) << 24) |
	       ((v & 0x000000000000FF00ULL) << 40) |
	       ((v & 0x00000000000000FFULL) << 56);
}
#endif

tommy_inline tommy_uint32_t tommy_read32(const void* ptr)
{
	tommy_uint32_t v;
	memcpy(&v, ptr, sizeof(v));
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__) || \
	(defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	v = tommy_swap32(v);
#endif
	return v;
}

tommy_inline tommy_uint64_t tommy_read64(const void* ptr)
{
	tommy_uint64_t v;
	memcpy(&v, ptr, sizeof(v));
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__) || \
	(defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	v = tommy_swap64(v);
#endif
	return v;
}

tommy_inline tommy_uint32_t tommy_rotl32(tommy_uint32_t x, int r)
{
	return (x << r) | (x >> (32 - r));
}

tommy_inline tommy_uint64_t tommy_rotl64(tommy_uint64_t x, int r)
{
	return (x << r) | (x >> (64 - r));
}

tommy_inline void tommy_mul(tommy_uint32_t* lo, tommy_uint32_t* hi)
{
	tommy_uint64_t v = (tommy_uint64_t)*lo * *hi;
	*lo = v;
	*hi = v >> 32;
}

/******************************************************************************/
/* hash */

/**
 * Based on a5hash and a5hash32 by Aleksey Vaneev (https://github.com/avaneev/a5hash),
 * modified to improve adversarial collision resistance under the seeded
 * collision-score metric:
 *
 *   score = log2(L / p), where L = max(1, ceil(max(lenA, lenB) / 8))
 *
 * and p is the collision probability of two fixed distinct messages over a
 * uniformly random seed, as defined in Thomas Dybdahl Ahle's analysis:
 * https://thomasahle.com/blog/adversarial-examples-for-hashes/#appendix-chainhash128
 *
 * L is measured in 8-byte units, including for the 32-bit hash.
 *
 * Weaknesses in the original a5hash family:
 * - a5hash (64-bit) has a collision score of 48.3 bits (p = 156,800 / 2^64
 *   ~= 2^-46.74 at L = 3, len = 23): because the seed is split into odd/even bit
 *   masks (init_val & val10, init_val & val01) and mixed with a single 64x64->128
 *   multiplication, the low product word h[0] is a 2-adic triangular function of
 *   the seed that loses high seed bits when key_len cancels low constant bits and
 *   admits a 2-adic branching tree of preimages. A message word matching h[0]
 *   zeroes one multiplication operand (0 * x = 0), erasing differences in the
 *   other operand.
 * - a5hash32 (32-bit) has a trivial seed-independent collision with score ~1 bit
 *   (p = 1 for all 2^32 seeds): its second lane (Seed3, Seed4) is initialized
 *   solely from public constants and MsgLen >> 32 without the seed. In the 9..16
 *   byte path (and in the first loop iteration), choosing the word added to Seed3
 *   equal to -Seed3 (mod 2^32) forces (c + Seed3) * (d + Seed4) = 0 * (d + Seed4) = 0
 *   for every seed, completely erasing any difference in d and yielding p = 1.
 *
 * Modifications in tommy_hash_u64 (64-bit):
 * 1. Shift key_len by 2 in initialization (len_mix = key_len << 2) so bit 0 of
 *    h[0] and bit 1 of h[1] remain fixed to 1 for all lengths, preventing
 *    length-dependent trailing zeros from shifting high seed bits out of h[0].
 * 2. Cross-mix h[0] and h[1] with asymmetric rotations after the initial
 *    tommy_mum(&h[0], &h[1]) to couple the LSB-to-MSB lower half and MSB-to-LSB
 *    upper half, breaking the 2-adic preimage tree on both state words.
 * 3. Swap the state words XORed into val01 and val10 (val01 ^= h[1], val10 ^= h[0])
 *    and swap the 32-bit halves in the tail load of h[1] to break commutative
 *    operand-swap symmetries on 4..8-byte tails and repeated blocks.
 * 4. Add cross feed-forward in the finalizer:
 *      h[0] = (h[0] + b) ^ val01;
 *      h[1] = (h[1] + a) ^ val10;
 *    so that a zero or unit operand in the first finalizer multiplication cannot
 *    erase the other state word before the final mixing step.
 *
 * Modifications in tommy_hash_u32 (32-bit):
 * 1. Asymmetric length initialization: set h[0]'s initial constant MSB to 1
 *    (0xa43f6a88 instead of 0x243f6a88) and mix key_len asymmetrically into h[0]
 *    and h[1] via h[0] ^= key_len and h[1] += key_len ^ (key_len << 16), avoiding
 *    simultaneous low-bit cancellation in the initial seed multiplication.
 * 2. Seed-dependent initialization of all four lanes: after the initial
 *    tommy_mul(&h[0], &h[1]), cross-mix h[0] and h[1] (h[0] ^= h[1];
 *    h[1] += rotl32(h[0], 15)) to break the 2-adic preimage tree, and diffuse
 *    both words into the second lane (h[2] += h[1] ^ rotl32(h[0], 7);
 *    h[3] += h[0] ^ rotl32(h[1], 13)). This eliminates the unseeded (Seed3, Seed4)
 *    vulnerability that caused the score ~1 universal collision in a5hash32.
 * 3. Asymmetric tail mixing on 9..16-byte inputs: combine the middle words into
 *    h[2] and h[3] using distinct operations (h[2] += ... and h[3] ^= ... instead
 *    of two additions) before tommy_mul(&h[2], &h[3]), breaking add/carry and
 *    operand-swap symmetries between the two lanes.
 * 4. Strengthened finalizer: mix both halves before the final multiplication via
 *    h[0] ^= val01 + h[1] and h[1] ^= val10 (instead of only h[0] ^= val01),
 *    ensuring neither half of the intermediate product is left unmixed or erased
 *    when one operand is small or zero.
 *
 *
 * Known adversarial collisions in tommy_hash_u64:
 *
 * The following are the strongest collision pairs found for each tested length.
 * Counts are lower bounds from explicitly identified and verified colliding
 * seeds, not exhaustive counts over the full 2^64 seed space.
 * The score uses L = max(1, ceil(length / 8)).
 *
 *   Length(bytes)  Colliding seeds   Score(bits)
 *   ------------   ---------------   -----------
 *       8                >=2         <=63.000000
 *      14                >=6         <=62.415037
 *      15                >=4         <=63.000000
 *      16                >=49        <=59.385290  <-- best found
 *      17                >=6         <=63.000000
 *      18                >=4         <=63.584963
 *      19                >=4         <=63.584963
 *      20                >=4         <=63.584963
 *      21                >=4         <=63.584963
 *      22                >=4         <=63.584963
 *      23                >=4         <=63.584963
 *      24                >=4         <=63.584963
 *      25                >=4         <=64.000000
 *      26                >=4         <=64.000000
 *      27                >=6         <=63.415037
 *      28                >=4         <=64.000000
 *      29                >=4         <=64.000000
 *      30                >=4         <=64.000000
 *      31                >=4         <=64.000000
 *      32                >=4         <=64.000000
 *      33                >=6         <=63.736966
 *      34                >=6         <=63.736966
 *      38                >=9         <=63.152003
 *      40                >=6         <=63.736966
 *      41                >=6         <=64.000000
 *      47                >=6         <=64.000000
 *      48                >=6         <=64.000000
 *      49                >=8         <=63.807355
 *      50                >=8         <=63.807355
 *      51                >=12        <=63.222392
 *      56                >=8         <=63.807355
 *      57                >=8         <=64.000000
 *      63                >=8         <=64.000000
 *      64                >=12        <=63.415037
 *      65                >=10        <=63.847997
 *      69                >=15        <=63.263034
 *      72                >=10        <=63.847997
 *      73                >=10        <=64.000000
 *      74                >=15        <=63.415037
 *      80                >=10        <=64.000000
 *      81                >=12        <=63.874469
 *      88                >=12        <=63.874469
 *      89                >=12        <=64.000000
 *      96                >=12        <=64.000000
 *      97                >=14        <=63.893085
 *      98                >=21        <=63.308122
 *     128                >=16        <=64.000000
 *     129                >=18        <=63.917538
 *     134                >=27        <=63.332575
 *     144                >=18        <=64.000000
 *     145                >=20        <=63.925999
 *     147                >=30        <=63.341037
 *     172                >=33        <=63.415037
 *     185                >=36        <=63.415037
 *     192                >=24        <=64.000000
 *     193                >=26        <=63.943416
 *     255                >=32        <=64.000000
 *     256                >=32        <=64.000000
 *     257                >=34        <=63.956931
 *     512                >=64        <=64.000000
 *     513                >=66        <=63.977974
 *     625                >=76        <=64.055853
 *     652                >=123       <=63.415037
 *    1024                >=192       <=63.415037
 *    1025                >=130       <=63.988859
 *    2043                >=384       <=63.415037
 *    2048                >=256       <=64.000000
 *    2049                >=258       <=63.994397
 *    4036                >=759       <=63.412184
 *    4096                >=512       <=64.000000
 *    4097                >=514       <=63.997190
 *    8182                >=1536      <=63.413628
 *    8193                >=1026      <=63.998593
 *   16385                >=2050      <=63.999296
 *
 * The collisions exploit equivalent initial states, zero multiplication
 * operands, operand-swap symmetries, and degeneracies in the final XOR-fold
 * of 64x64->128 products. Distinct seeds can produce the same initial
 * 128-bit product, resulting in identical initialized states for all
 * messages of that length.
 *
 * The initial multiplication also admits many seeds sharing its low
 * 64-bit product. However, the subsequent asymmetric rotations mix the
 * high product word into both state words, so equal low products alone
 * do not imply hash collisions.
 *
 * The final XOR-fold has several useful degeneracies:
 * - A zero operand always produces zero.
 * - An operand of 2^64-1 produces an XOR-fold of 2^64-1 for any nonzero
 *   second operand.
 * - Divisors of 2^64-1 permit families of distinct operands with the
 *   same XOR-fold.
 * - Products divisible by 2^64 have a zero low half, which can contribute
 *   to degeneracies in subsequent mixing operations.
 *
 * An 8-byte construction produces 8192 distinct messages hashing to zero
 * for the same two seeds. At 16 bytes, constructions include four distinct
 * messages hashing to zero for four seeds, and 51 distinct messages hashing
 * to UINT64_MAX for another set of four seeds. These multicollisions do not
 * improve the pairwise collision-score bounds shown above.
 *
 * A 14-byte construction combines two independent classes of
 * three seeds, each producing an identical initial 128-bit product within
 * its class. The same pair of messages makes the first operand of the final
 * multiplication equal to UINT64_MAX for both classes, yielding six
 * colliding seeds and a score of at most 62.415037 bits.
 *
 * The strongest known construction uses 16-byte messages and the zero
 * divisors of the low product word in the finalizer. For a 16-byte message
 * the tail loads two independent words m0 and m1, and the finalizer computes:
 *
 *   a = s0 ^ m0, b = s1 ^ m1, (lo, hi) = a * b
 *   h[0] = (lo + b) ^ val01, with val01 = 0x5555555555555555 ^ s1
 *
 * where (s0, s1) is the seed-dependent initialized state. Choosing
 * m1 = 0x5555555555555555 makes b equal to val01, so h[0] is zero, and the
 * hash is zero, whenever lo = 0, that is whenever a * b = 0 (mod 2^64), or
 * equivalently ctz(a) + ctz(b) >= 64. This congruence has about 65 * 2^63
 * solutions (a, b) instead of the 2^64 of a generic 64-bit condition, so a
 * single message with m0 = X is expected to hash to zero for about 32 seeds,
 * for any X.
 *
 * Flipping bit 63 of m0 gives a second message that hashes to zero for
 * the same seeds, excluding only the ones with b odd, because with b even
 * the top bit of a does not affect lo. The colliding seeds are then the
 * ones with ctz(s1 ^ 0x5555555555555555) = t >= 1 and
 * s0 = X (mod 2^(64 - t)).
 *
 * The seeds can be found with about 2^35 work instead of 2^64. The low 29
 * bits of s0 are lo[0..28] ^ hi[35..63] of the initial product, where
 * lo[0..28] depends only on the low 29 seed bits and hi[35..63] is determined,
 * up to a carry, by the high 35 seed bits. Fixing the low 29 bits of X, the
 * 2^35 seeds matching them are enumerated directly. Each of these seeds
 * constrains the bits 29..63-t of X, so the remaining bits of X are chosen
 * afterward, as the root-to-leaf path of the binary prefix tree that
 * collects the maximum number of seeds. A random path collects about 18
 * seeds, the best path of a single enumeration 49.
 *
 * The choice of the low 29 bits of X is arbitrary, and only one value was
 * tested.
 *
 * For messages longer than 16 bytes, a cross-zero construction can make
 * two different message blocks produce the same zero multiplication product
 * for distinct classes of equivalent seeds, causing their states to
 * converge after the first 16-byte block.
 *
 * This construction extends to arbitrarily many blocks: each subsequent
 * identical 16-byte block can force another equivalent seed class to
 * converge while preserving all previous collisions. With
 * t = floor((length - 1) / 16) complete blocks and length > 16:
 *
 *   Pair classes:
 *     colliding seeds >= 2 * (t + 1)
 *     score <= 64 + log2(ceil(length / 8) / (2 * (t + 1)))
 *
 *   Triple classes (when suitable classes are available):
 *     colliding seeds >= 3 * (t + 1)
 *     score <= 64 + log2(ceil(length / 8) / (3 * (t + 1)))
 *
 * The pair-class construction approaches a score of 64 bits as the
 * length increases. The triple-class construction approaches
 * 64 + log2(2 / 3) = 63.415037 bits.
 *
 * Verified triple-class examples include nine colliding seeds at 38 bytes,
 * 12 at 51 bytes, 123 at 652 bytes, 192 at 1024 bytes, 384 at 2043 bytes,
 * and 1536 at 8182 bytes. The number of colliding seeds grows linearly
 * with message length, but these constructions do not improve the best
 * overall score of 59.385290 bits.
 *
 * Best known collision example (16 bytes, >=49 / 2^64 seeds):
 *
 *   Input A: 76 3d 9d 6f 55 55 55 55 55 55 55 55 00 00 00 c0
 *   Input B: 76 3d 9d ef 55 55 55 55 55 55 55 55 00 00 00 c0
 *
 * Both inputs hash to 0x0000000000000000 with the seeds:
 *
 *   0x091a8f028ab1715d  0x0c1fa74906f21be5  0x1016e8bd084c55c4  0x105aa6a6d643cd62
 *   0x139c20a77692c539  0x209a0fdedfae59cc  0x2db59ab720276d81  0x31ff4e13f994e049
 *   0x33d493028426c59f  0x35964ca8b6fd63af  0x3636eb5414b21740  0x398581599bd4df58
 *   0x3caf5262adaf0ded  0x400810545dc679ad  0x4d2c8b8415f76887  0x548a081af966ef4a
 *   0x609de1e82f3c620a  0x652a551266aec67f  0x6837e6ea0e8b7281  0x6f527da37a2fc8d5
 *   0x7028ff0f4ae87a96  0x71955c34d65b1344  0x733048613274cebc  0x742dde9616b2ca3b
 *   0x7a32d57a15c492d7  0x7bc903c183459572  0x7d08fb5a0d8a152c  0x84ef2dd576b15c91
 *   0x8844b27c7fcace9f  0x8e8942d00ce00999  0x93a1f19492ed2a55  0x95d07216b29e654e
 *   0x95dfe9f7eb0f0371  0x97b6c6ad40df2c3d  0x9c59fa72c9f4c19e  0x9dce78113121fdfd
 *   0xad879fcf43704748  0xade966ff287138f8  0xafda30ba61accf70  0xc617316b1091eece
 *   0xcbbbe7badd71cd3b  0xd80cb9fb6d522919  0xd9fcb89d3068668f  0xdc847c31b50f43ec
 *   0xe6b5151bda425a67  0xe9eb57bf15d63712  0xecd8b0e0461ef5f4  0xed40e9f2bb4f3e5a
 *   0xf0133932053fd936
 *
 *   p >= 49 / 2^64
 *   score <= log2(2 * 2^64 / 49) = 59.385290 bits
 *
 * Entries marked >= or <= are bounds from identified collision classes,
 * rather than exact counts of all colliding seeds for the selected pair.
 * These are observed upper bounds on the minimum collision score, not
 * proven global minima.
 *
 *
 * Known adversarial collisions in tommy_hash_u32:
 *
 * The following are the strongest collision pairs found for each tested length.
 * Counts are over the full 2^32 seed space for a fixed pair of distinct inputs.
 * The score uses L = max(1, ceil(length / 8)).
 *
 *   Length(bytes)  Colliding seeds   Score(bits)
 *   ------------   ---------------   -----------
 *       5                 22           27.540568
 *       6                  9           28.830075
 *       7                 10           28.678072
 *       8                 35           26.870717  <-- best found
 *       9                 14           29.192645
 *      10                 10           29.678072
 *      11                  3           31.415037
 *      12                  7           30.192645
 *      13                  7           30.192645
 *      14                  8           30.000000
 *      15                 10           29.678072
 *      16                 12           29.415037
 *      17                 >=6         <=31.000000
 *      22                  9           30.415037
 *      23                  6           31.000000
 *      24                 12           30.000000
 *      25                  5           31.678072
 *      26                  8           31.000000
 *      27                  5           31.678072
 *      30                  8           31.000000
 *      33                 10           31.000000
 *      40                  9           31.152003
 *      41                 10           31.263034
 *     121                  7           33.192645
 *     625                >=103        <=31.617280
 *
 * Entries marked >= or <= are bounds from identified collision classes,
 * rather than exact counts of all colliding seeds for the selected pair.
 * These are observed upper bounds on the minimum collision score, not
 * proven global minima.
 *
 * Best known collision example (8 bytes, 35 / 2^32 seeds):
 *
 *   Input A: 67 6c 28 3b 47 7d c7 f1
 *   Input B: 16 63 28 3b 47 7d c7 f1
 *
 *   Seed          Hash(A)       Hash(B)
 *   0x095e58fd    0xffffffff    0xffffffff
 *   0x23f6980e    0x4924924a    0x4924924a
 *
 *   p = 35 / 2^32
 *   score = log2(2^32 / 35) = 26.870716983 bits
 *
 * The collision counts and example seeds have been verified using the
 * complete hash implementations. The remaining weaknesses are primarily
 * associated with zero multiplication operands and degeneracies in the
 * final XOR-fold of multiplication products.
 *
 * Copyright (c) 2025 Aleksey Vaneev
 * Copyright (c) 2026 Andrea Mazzoleni
 */

TOMMY_API tommy_uint32_t tommy_hash_u32(tommy_uint32_t init_val, const void* void_key, tommy_size_t key_len)
{
	const unsigned char* key = tommy_cast(const unsigned char*, void_key);
	tommy_uint32_t val01 = 0x55555555;
	tommy_uint32_t val10 = 0xaaaaaaaa;
	tommy_uint32_t h[4] = { 0xa43f6a88, 0x85a308d3, 0xfb0bd3ea, 0x0f58fd47 };
	tommy_uint32_t a, b;
	tommy_size_t len = key_len;

	h[0] ^= key_len;
	h[1] += key_len ^ (key_len << 16);
#if TOMMY_SIZE_BIT == 64
	h[2] = (key_len >> 32) ^ 0x452821e6;
	h[3] = (key_len >> 32) ^ 0x38d01377;
	tommy_mul(&h[2], &h[3]);
#endif
	h[0] ^= init_val & val01;
	h[1] ^= init_val & val10;
	tommy_mul(&h[0], &h[1]);

	h[0] ^= h[1];
	h[1] += tommy_rotl32(h[0], 15);
	h[2] += h[1] ^ tommy_rotl32(h[0], 7);
	h[3] += h[0] ^ tommy_rotl32(h[1], 13);

	if (len <= 16) {
		if (len >= 4) {
			const unsigned char* tail = key + len - 4;
			a = tommy_read32(key);
			b = tommy_read32(tail);
			if (len >= 9) {
				h[2] += tommy_read32(key + 4);
				h[3] ^= tommy_read32(tail - 4);
				tommy_mul(&h[2], &h[3]);
			}
		} else {
			a = 0;
			b = 0;
			if (len != 0) {
				a = key[0];
				if (len >= 2)
					a |= key[1] << 8;
				if (len == 3)
					a |= key[2] << 16;
			}
		}
	} else {
		val01 ^= h[0];
		val10 ^= h[1];
		/* leave 1..16 bytes for the tail, including a full final block */
		tommy_size_t blocks = (len - 1) / 16;
		len -= blocks * 16;
		do {
			tommy_uint32_t s1 = h[0];
			tommy_uint32_t s4 = h[3];
			h[0] += tommy_read32(key);
			h[1] += tommy_read32(key + 4);
			tommy_mul(&h[0], &h[1]);
			h[2] += tommy_read32(key + 8);
			h[3] += tommy_read32(key + 12);
			tommy_mul(&h[2], &h[3]);
			h[0] += val01;
			h[1] += s4;
			h[2] += s1;
			h[3] += val10;
			key += 16;
		} while (--blocks);

		/* the last words may overlap the previous block, but stay within the input */
		const unsigned char* tail = key + len;
		a = tommy_read32(tail - 8);
		b = tommy_read32(tail - 4);
		if (len >= 9) {
			h[2] += tommy_read32(tail - 16);
			h[3] += tommy_read32(tail - 12);
			tommy_mul(&h[2], &h[3]);
		}
	}

	h[0] ^= h[2];
	h[1] ^= h[3];
	h[0] += a;
	h[1] += b;
	tommy_mul(&h[0], &h[1]);
	h[0] ^= val01 + h[1];
	h[1] ^= val10;
	tommy_mul(&h[0], &h[1]);
	return h[0] ^ h[1];
}

TOMMY_API tommy_uint64_t tommy_hash_u64(tommy_uint64_t init_val, const void* void_key, tommy_size_t key_len)
{
	const unsigned char* key = tommy_cast(const unsigned char*, void_key);
	tommy_uint64_t val01 = 0x5555555555555555ULL;
	tommy_uint64_t val10 = 0xaaaaaaaaaaaaaaaaULL;
	tommy_uint64_t len_mix = (tommy_uint64_t)key_len << 2;
	tommy_uint64_t h[2];
	tommy_size_t len = key_len;
	tommy_uint64_t a, b;

	h[0] = 0x452821e638d01377ULL ^ len_mix;
	h[1] = 0x243f6a8885a308d3ULL ^ len_mix;
	h[0] ^= init_val & val10;
	h[1] ^= init_val & val01;
	tommy_mum(&h[0], &h[1]);
	h[0] ^= tommy_rotl64(h[1], 29);
	h[1] ^= tommy_rotl64(h[0], 47);
	val01 ^= h[1];
	val10 ^= h[0];

	if (len > 16) {
		/* leave 1..16 bytes for the tail, including a full final block */
		tommy_size_t blocks = (len - 1) / 16;
		len -= blocks * 16;
		do {
			h[0] ^= tommy_read64(key);
			h[1] ^= tommy_read64(key + 8);
			tommy_mum(&h[0], &h[1]);
			h[0] += val01;
			h[1] += val10;
			key += 16;
		} while (--blocks);
	}

	if (len >= 4) {
		const unsigned char* tail = key + len - 4;
		tommy_size_t offset = (len / 8) * 4;
		h[0] ^= (tommy_uint64_t)tommy_read32(key) << 32 | tommy_read32(tail);
		h[1] ^= (tommy_uint64_t)tommy_read32(tail - offset) << 32 | tommy_read32(key + offset);
	} else if (len != 0) {
		tommy_uint32_t tail = key[0];
		if (len >= 2)
			tail |= key[1] << 8;
		if (len == 3)
			tail |= key[2] << 16;
		h[0] ^= tail;
	}

	a = h[0];
	b = h[1];
	tommy_mum(&h[0], &h[1]);
	h[0] = (h[0] + b) ^ val01;
	h[1] = (h[1] + a) ^ val10;
	tommy_mum(&h[0], &h[1]);
	return h[0] ^ h[1];
}

