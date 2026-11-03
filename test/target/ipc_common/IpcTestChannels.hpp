#ifndef IPCTESTCHANNELS_HPP
#define IPCTESTCHANNELS_HPP

#include <stdint.h>

#include "driverlib.h"

#include "ipc/Mailbox.hpp"
#include "ipc/MailboxStorage.hpp"
#include "ipc/MsgRamSections.hpp"
#include "ipc/Queue.hpp"
#include "ipc/QueueStorage.hpp"
#include "util/StaticAssert.hpp"

// Message types and channels of the IPC target test, shared by both cores in
// the same way as the real ones will be.
namespace ipctest {

// All words follow from the counter, so the reader can detect a torn message.
// Long enough that a read overlaps a write of the 10 kHz writer now and then.
const uint16_t kPatternWords = 14U;

struct Pattern {
    uint32_t counter;
    uint16_t words[kPatternWords];
};
STATIC_ASSERT_SIZE_WORDS(Pattern, 16);

inline Pattern makePattern(uint32_t counter)
{
    Pattern p;
    p.counter = counter;
    for (uint16_t i = 0U; i < kPatternWords; ++i) {
        p.words[i] = static_cast<uint16_t>(counter + i);
    }
    return p;
}

inline bool isConsistent(const Pattern& p)
{
    for (uint16_t i = 0U; i < kPatternWords; ++i) {
        if (p.words[i] != static_cast<uint16_t>(p.counter + i)) {
            return false;
        }
    }
    return true;
}

struct Event {
    uint32_t value;
    uint32_t inverted;
};
STATIC_ASSERT_SIZE_WORDS(Event, 4);

// What CPU2 has seen, posted to CPU1.
struct Cpu2Report {
    uint32_t patternReads;
    uint32_t patternTorn;
    uint32_t patternBackwards;
    uint32_t patternInterrupts;
    uint32_t commandsHandled;
    uint32_t addressesOk;       // 1 if the channels are at the expected addresses
};
STATIC_ASSERT_SIZE_WORDS(Cpu2Report, 12);

const uint16_t kEventQueueSize = 16U;
const uint32_t kCommandAddOne = 0x0A01UL;

// IPC flags, per direction. See shared_types/IpcFlags.hpp for the reserved ones.
const uint16_t kFlagPattern = 1U;      // raises the IPC1 interrupt on CPU2
const uint16_t kFlagReport = 4U;
const uint16_t kFlagEvents = 5U;
const uint16_t kFlagCommand = 6U;

struct Cpu1ToCpu2Channels {
    ipc::MailboxStorage<Pattern> pattern;
    ipc::QueueConsumerStorage eventsConsumer;
};

struct Cpu2ToCpu1Channels {
    ipc::MailboxStorage<Cpu2Report> report;
    ipc::QueueProducerStorage<Event, kEventQueueSize> eventsProducer;
};

STATIC_ASSERT(sizeof(Cpu1ToCpu2Channels) <= ipc::kChannelsRegionWords * sizeof(uint16_t));
STATIC_ASSERT(sizeof(Cpu2ToCpu1Channels) <= ipc::kChannelsRegionWords * sizeof(uint16_t));

typedef ipc::Mailbox<Pattern, kFlagPattern> PatternMailbox;
typedef ipc::Mailbox<Cpu2Report, kFlagReport> ReportMailbox;
typedef ipc::Queue<Event, kEventQueueSize, kFlagEvents> EventQueue;

} // namespace ipctest

#endif
