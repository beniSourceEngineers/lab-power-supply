#ifndef IPC_MAILBOX_HPP
#define IPC_MAILBOX_HPP

#include <stdint.h>

#include "driverlib.h"

#include "ipc/IMailboxReader.hpp"
#include "ipc/IMailboxWriter.hpp"
#include "ipc/MailboxStorage.hpp"
#include "ipc/SharedCopy.hpp"
#include "util/StaticAssert.hpp"

namespace ipc {

// Latest-value channel with one writer and one reader, one per core. Both cores
// create a Mailbox on the same MailboxStorage and use one side of it.
//
// Consistency comes from a sequence counter (seqlock): the reader needs no write
// access to the writer's message RAM and the writer never waits for the reader,
// so a slow reader cannot delay the control ISR. Only one context per core may
// use its side: the writer side is not reentrant, and neither is the reader side.
//
// The flag only wakes the reader (flags 0-3 can raise an IPC interrupt); whether
// a message is new is decided by the sequence counter alone. The writer sets the
// flag after the message is complete and the reader acknowledges it before
// reading, so no wake-up gets lost.
//
// The C28x has no cache and keeps writes in order, as it does reads. volatile
// is therefore enough for the ordering, and 32-bit accesses to seq are atomic.
template <typename T, uint16_t Flag>
class Mailbox : public IMailboxWriter<T>, public IMailboxReader<T> {
public:
    Mailbox(MailboxStorage<T>& storage, IPC_Type_t link)
        : storage_(storage)
        , link_(link)
        , lastSeq_(0UL)
    {
    }

    // Writer core only, before syncCores(): message RAM keeps its contents over
    // a reset and the reader core must not see the old values.
    void initWriter()
    {
        storage_.seq = 0UL;
        IPC_clearFlagLtoR(link_, kFlagMask);
    }

    virtual void post(const T& msg)
    {
        const uint32_t seq = storage_.seq;
        storage_.seq = seq + 1UL;
        copyToShared(storage_.payload, msg);
        storage_.seq = seq + 2UL;
        IPC_setFlagLtoR(link_, kFlagMask);
    }

    virtual bool tryRead(T& out)
    {
        IPC_ackFlagRtoL(link_, kFlagMask);

        for (uint16_t attempt = 0U; attempt < kMaxReadAttempts; ++attempt) {
            const uint32_t before = storage_.seq;
            if (!isNew(before)) {
                return false;
            }

            T copy;
            copyFromShared(copy, storage_.payload);

            if (storage_.seq == before) {
                out = copy;
                lastSeq_ = before;
                return true;
            }
        }
        return false;
    }

    virtual bool hasNew() const
    {
        return isNew(storage_.seq);
    }

private:
    STATIC_ASSERT(Flag < 32U);

    static const uint32_t kFlagMask = 1UL << Flag;
    // More than one retry only happens if an ISR on the reader core delays the
    // read. The bound keeps tryRead short in an ISR; the caller just polls again.
    static const uint16_t kMaxReadAttempts = 3U;

    bool isNew(uint32_t seq) const
    {
        return ((seq & 1UL) == 0UL) && (seq != lastSeq_);
    }

    MailboxStorage<T>& storage_;
    const IPC_Type_t link_;
    uint32_t lastSeq_;
};

template <typename T, uint16_t Flag>
const uint32_t Mailbox<T, Flag>::kFlagMask;

template <typename T, uint16_t Flag>
const uint16_t Mailbox<T, Flag>::kMaxReadAttempts;

} // namespace ipc

#endif
