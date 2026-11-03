#ifndef UTIL_STATICASSERT_HPP
#define UTIL_STATICASSERT_HPP

#include <stdint.h>

// Compile-time check for C++03 (negative array size if cond is false).
#define UTIL_CONCAT_IMPL(a, b) a##b
#define UTIL_CONCAT(a, b) UTIL_CONCAT_IMPL(a, b)
#define STATIC_ASSERT(cond) \
    typedef char UTIL_CONCAT(static_assert_line_, __LINE__)[(cond) ? 1 : -1]

// Checks the size of a type in 16-bit words. sizeof counts 16-bit words on the
// C28x and bytes on the host, so the same check holds on both.
#define STATIC_ASSERT_SIZE_WORDS(T, words) \
    STATIC_ASSERT(sizeof(T) == (words) * sizeof(uint16_t))

#endif
