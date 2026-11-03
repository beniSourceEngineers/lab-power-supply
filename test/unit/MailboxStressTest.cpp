// The writer and the reader in two threads, as a stand-in for the two cores. A
// read that overlaps a write only happens by chance, so this finds a broken
// seqlock with high probability but cannot pass or fail deterministically for a
// correct one: a correct implementation never fails.
#include <atomic>
#include <thread>

#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "ipc/Mailbox.hpp"

namespace {

const uint16_t kWords = 64U;
const uint32_t kPosts = 2000000UL;

struct Pattern {
    uint32_t words[kWords];
};

Pattern makePattern(uint32_t value)
{
    Pattern p;
    for (uint16_t i = 0U; i < kWords; ++i) {
        p.words[i] = value;
    }
    return p;
}

bool isConsistent(const Pattern& p)
{
    for (uint16_t i = 1U; i < kWords; ++i) {
        if (p.words[i] != p.words[0]) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST(MailboxStressTest, ReaderNeverSeesTornOrOldMessage)
{
    fake::resetIpc();
    static ipc::MailboxStorage<Pattern> storage;
    ipc::Mailbox<Pattern, 1U> writer(storage, IPC_CPU1_L_CPU2_R);
    ipc::Mailbox<Pattern, 1U> reader(storage, IPC_CPU2_L_CPU1_R);
    writer.initWriter();

    std::atomic<bool> done(false);
    std::thread writerThread([&]() {
        for (uint32_t i = 1U; i <= kPosts; ++i) {
            writer.post(makePattern(i));
        }
        done = true;
    });

    uint32_t reads = 0U;
    uint32_t torn = 0U;
    uint32_t backwards = 0U;
    uint32_t last = 0U;
    Pattern out;
    while (!done) {
        if (reader.tryRead(out)) {
            ++reads;
            torn += isConsistent(out) ? 0U : 1U;
            backwards += (out.words[0] > last) ? 0U : 1U;
            last = out.words[0];
        }
    }
    writerThread.join();

    EXPECT_GT(reads, 0U);
    EXPECT_EQ(torn, 0U);
    EXPECT_EQ(backwards, 0U);

    // The final message is always delivered.
    if (reader.tryRead(out)) {
        last = out.words[0];
    }
    EXPECT_EQ(last, kPosts);
}
