#ifndef IPC_SHAREDCOPY_HPP
#define IPC_SHAREDCOPY_HPP

#include <stddef.h>

namespace ipc {

// Copies a message to or from shared memory, in unsigned char units: sizeof
// counts these units on every platform (16 bit on the C28x, 8 bit on the host).
// T must be a POD type. The copy is not atomic, so the caller's protocol has to
// make it safe.
template <typename T>
inline void copyToShared(volatile T& dst, const T& src)
{
    volatile unsigned char* to = reinterpret_cast<volatile unsigned char*>(&dst);
    const unsigned char* from = reinterpret_cast<const unsigned char*>(&src);
    for (size_t i = 0U; i < sizeof(T); ++i) {
        to[i] = from[i];
    }
}

template <typename T>
inline void copyFromShared(T& dst, const volatile T& src)
{
    unsigned char* to = reinterpret_cast<unsigned char*>(&dst);
    const volatile unsigned char* from = reinterpret_cast<const volatile unsigned char*>(&src);
    for (size_t i = 0U; i < sizeof(T); ++i) {
        to[i] = from[i];
    }
}

} // namespace ipc

#endif
