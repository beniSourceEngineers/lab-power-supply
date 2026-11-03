// IPC target test, CPU2 side: reads the pattern mailbox in the IPC1 interrupt,
// answers commands, pushes events and reports to CPU1, which decides the result.

#include <stdint.h>

extern "C" {
#include "device.h"
}

#include "IpcTestChannels.hpp"
#include "ipc/CommandLink.hpp"
#include "ipc/ICommandHandler.hpp"
#include "ipc/SyncCores.hpp"
#include "shared_types/IpcFlags.hpp"

using namespace ipctest;

// The same objects as on CPU1, see ipc/MsgRamSections.hpp.
#pragma DATA_SECTION(IPC_SECTION_CPU1_TO_CPU2)
Cpu1ToCpu2Channels cpu1ToCpu2;
#pragma DATA_SECTION(IPC_SECTION_CPU2_TO_CPU1)
Cpu2ToCpu1Channels cpu2ToCpu1;

namespace {

const uint32_t kLoopsPerReport = 256UL;

class AddOneHandler : public ipc::ICommandHandler {
public:
    AddOneHandler()
        : handled_(0UL)
    {
    }

    virtual uint32_t onCommand(uint32_t command, uint32_t, uint32_t data)
    {
        ++handled_;
        return (command == kCommandAddOne) ? (data + 1UL) : 0UL;
    }

    uint32_t handled() const { return handled_; }

private:
    uint32_t handled_;
};

PatternMailbox patternMailbox(cpu1ToCpu2.pattern, IPC_CPU2_L_CPU1_R);
ReportMailbox reportMailbox(cpu2ToCpu1.report, IPC_CPU2_L_CPU1_R);
EventQueue eventQueue(cpu2ToCpu1.eventsProducer, cpu1ToCpu2.eventsConsumer, IPC_CPU2_L_CPU1_R);
ipc::CommandLink commandLink(IPC_CPU2_L_CPU1_R, kFlagCommand);
AddOneHandler commandHandler;

// Written by the ISR, read by the main loop.
volatile uint32_t patternReads = 0UL;
volatile uint32_t patternTorn = 0UL;
volatile uint32_t patternBackwards = 0UL;
volatile uint32_t patternInterrupts = 0UL;

__interrupt void ipc1Isr(void)
{
    static uint32_t lastCounter = 0UL;

    patternInterrupts = patternInterrupts + 1UL;

    Pattern pattern;
    if (patternMailbox.tryRead(pattern)) {
        patternReads = patternReads + 1UL;
        if (!isConsistent(pattern)) {
            patternTorn = patternTorn + 1UL;
        }
        if (pattern.counter <= lastCounter) {
            patternBackwards = patternBackwards + 1UL;
        }
        lastCounter = pattern.counter;
    }
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

void sendEvent()
{
    static uint32_t value = 0UL;
    Event event;
    event.value = value;
    event.inverted = ~value;
    if (eventQueue.push(event)) {
        ++value;
    }
}

void postReport(uint32_t addressesOk)
{
    Cpu2Report report;
    report.patternReads = patternReads;
    report.patternTorn = patternTorn;
    report.patternBackwards = patternBackwards;
    report.patternInterrupts = patternInterrupts;
    report.commandsHandled = commandHandler.handled();
    report.addressesOk = addressesOk;
    reportMailbox.post(report);
}

} // namespace

int main()
{
    Device_init();

    const uint32_t addressesOk =
        ((reinterpret_cast<uint32_t>(&cpu1ToCpu2) == ipc::kChannelsAddrCpu1ToCpu2)
         && (reinterpret_cast<uint32_t>(&cpu2ToCpu1) == ipc::kChannelsAddrCpu2ToCpu1)) ? 1UL : 0UL;

    // Own side of all channels first, then the handshake.
    reportMailbox.initWriter();
    eventQueue.initProducer();
    commandLink.init();
    ipc::syncCores(IPC_CPU2_L_CPU1_R, shared_types::kIpcFlagSync);

    Interrupt_initModule();
    Interrupt_initVectorTable();
    Interrupt_register(INT_IPC_1, &ipc1Isr);
    Interrupt_enable(INT_IPC_1);

    EINT;
    ERTM;

    uint32_t loops = 0UL;
    for (;;) {
        commandLink.poll(commandHandler);
        sendEvent();
        if (++loops >= kLoopsPerReport) {
            loops = 0UL;
            postReport(addressesOk);
        }
    }
}
