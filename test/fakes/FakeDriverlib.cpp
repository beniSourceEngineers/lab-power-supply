#include "FakeDriverlib.hpp"

#include <atomic>
#include <map>
#include <utility>
#include <sstream>

#include "driverlib.h"

namespace fake {

std::vector<std::string>& driverlibCalls()
{
    static std::vector<std::string> calls;
    return calls;
}

std::string call(const char* name, std::initializer_list<uint32_t> args)
{
    std::ostringstream s;
    s << name << "(";
    const char* separator = "";
    for (uint32_t arg : args) {
        s << separator << arg;
        separator = ", ";
    }
    s << ")";
    return s.str();
}

} // namespace fake

namespace {

// One per direction, indexed by the IPC_Type_t of the sending core. Atomic flags,
// because the stress tests run the two sides in two threads.
struct IpcDirection {
    std::atomic<uint32_t> flags;
    uint32_t command;
    uint32_t addr;
    uint32_t data;
    uint32_t reply;
};

IpcDirection ipcDirections[IPC_TOTAL_NUM];

IpcDirection& localToRemote(IPC_Type_t ipcType) { return ipcDirections[ipcType]; }

IpcDirection& remoteToLocal(IPC_Type_t ipcType)
{
    return ipcDirections[ipcType == IPC_CPU1_L_CPU2_R ? IPC_CPU2_L_CPU1_R : IPC_CPU1_L_CPU2_R];
}

void record(const char* name, uint32_t a)
{
    std::ostringstream s;
    s << name << "(" << a << ")";
    fake::driverlibCalls().push_back(s.str());
}

void record(const char* name, uint32_t a, uint32_t b)
{
    std::ostringstream s;
    s << name << "(" << a << ", " << b << ")";
    fake::driverlibCalls().push_back(s.str());
}

void record(const char* name, uint32_t a, uint32_t b, uint32_t c)
{
    std::ostringstream s;
    s << name << "(" << a << ", " << b << ", " << c << ")";
    fake::driverlibCalls().push_back(s.str());
}

void record(const char* name, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    std::ostringstream s;
    s << name << "(" << a << ", " << b << ", " << c << ", " << d << ")";
    fake::driverlibCalls().push_back(s.str());
}

void record(const char* name, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{
    std::ostringstream s;
    s << name << "(" << a << ", " << b << ", " << c << ", " << d << ", " << e << ")";
    fake::driverlibCalls().push_back(s.str());
}

std::map<std::pair<uint32_t, uint16_t>, uint16_t> adcResults;
bool adcInterruptOverflow = false;
std::map<uint32_t, uint16_t> tripZoneFlags;

} // namespace

void GPIO_writePin(uint32_t pin, uint32_t outVal) { record("GPIO_writePin", pin, outVal); }
void GPIO_togglePin(uint32_t pin) { record("GPIO_togglePin", pin); }

void CPUTimer_stopTimer(uint32_t base) { record("CPUTimer_stopTimer", base); }
void CPUTimer_startTimer(uint32_t base) { record("CPUTimer_startTimer", base); }
void CPUTimer_setPeriod(uint32_t base, uint32_t periodCount) { record("CPUTimer_setPeriod", base, periodCount); }
void CPUTimer_setPreScaler(uint32_t base, uint16_t prescaler) { record("CPUTimer_setPreScaler", base, prescaler); }
void CPUTimer_reloadTimerCounter(uint32_t base) { record("CPUTimer_reloadTimerCounter", base); }
void CPUTimer_setEmulationMode(uint32_t base, CPUTimer_EmulationMode mode) { record("CPUTimer_setEmulationMode", base, mode); }
void CPUTimer_enableInterrupt(uint32_t base) { record("CPUTimer_enableInterrupt", base); }

void fake::resetIpc()
{
    for (IpcDirection& direction : ipcDirections) {
        direction.flags = 0U;
        direction.command = 0U;
        direction.addr = 0U;
        direction.data = 0U;
        direction.reply = 0U;
    }
}

void IPC_setFlagLtoR(IPC_Type_t ipcType, uint32_t flags) { localToRemote(ipcType).flags |= flags; }
void IPC_clearFlagLtoR(IPC_Type_t ipcType, uint32_t flags) { localToRemote(ipcType).flags &= ~flags; }
void IPC_ackFlagRtoL(IPC_Type_t ipcType, uint32_t flags) { remoteToLocal(ipcType).flags &= ~flags; }
bool IPC_isFlagBusyLtoR(IPC_Type_t ipcType, uint32_t flags) { return (localToRemote(ipcType).flags & flags) != 0U; }
bool IPC_isFlagBusyRtoL(IPC_Type_t ipcType, uint32_t flags) { return (remoteToLocal(ipcType).flags & flags) != 0U; }

// Records only: the real one blocks until the other core has called it too.
void IPC_sync(IPC_Type_t ipcType, uint32_t flag) { record("IPC_sync", ipcType, flag); }

bool IPC_sendCommand(IPC_Type_t ipcType, uint32_t flags, bool, uint32_t command, uint32_t addr, uint32_t data)
{
    IpcDirection& direction = localToRemote(ipcType);
    if ((direction.flags & flags) != 0U) {
        return false;
    }
    direction.command = command;
    direction.addr = addr;
    direction.data = data;
    direction.flags |= flags;
    return true;
}

bool IPC_readCommand(IPC_Type_t ipcType, uint32_t flags, bool, uint32_t* command, uint32_t* addr, uint32_t* data)
{
    const IpcDirection& direction = remoteToLocal(ipcType);
    if ((direction.flags & flags) == 0U) {
        return false;
    }
    *command = direction.command;
    *addr = direction.addr;
    *data = direction.data;
    return true;
}

void IPC_sendResponse(IPC_Type_t ipcType, uint32_t data) { remoteToLocal(ipcType).reply = data; }
uint32_t IPC_getResponse(IPC_Type_t ipcType) { return localToRemote(ipcType).reply; }

void fake::setAdcResult(uint32_t resultBase, uint16_t soc, uint16_t value)
{
    adcResults[std::make_pair(resultBase, soc)] = value;
}

void fake::setAdcInterruptOverflow(bool overflow) { adcInterruptOverflow = overflow; }

void fake::setTripZoneFlags(uint32_t base, uint16_t flags) { tripZoneFlags[base] = flags; }

void fake::resetPeripherals()
{
    adcResults.clear();
    adcInterruptOverflow = false;
    tripZoneFlags.clear();
}

void EPWM_setTimeBasePeriod(uint32_t base, uint16_t periodCount) { record("EPWM_setTimeBasePeriod", base, periodCount); }
void EPWM_setTimeBaseCounterMode(uint32_t base, EPWM_TimeBaseCountMode counterMode) { record("EPWM_setTimeBaseCounterMode", base, counterMode); }
void EPWM_setClockPrescaler(uint32_t base, EPWM_ClockDivider prescaler, EPWM_HSClockDivider highSpeedPrescaler) { record("EPWM_setClockPrescaler", base, prescaler, highSpeedPrescaler); }
void EPWM_disablePhaseShiftLoad(uint32_t base) { record("EPWM_disablePhaseShiftLoad", base); }
void EPWM_enablePhaseShiftLoad(uint32_t base) { record("EPWM_enablePhaseShiftLoad", base); }
void EPWM_setPhaseShift(uint32_t base, uint16_t phaseCount) { record("EPWM_setPhaseShift", base, phaseCount); }
void EPWM_setCountModeAfterSync(uint32_t base, EPWM_SyncCountMode mode) { record("EPWM_setCountModeAfterSync", base, mode); }
void EPWM_setSyncOutPulseMode(uint32_t base, EPWM_SyncOutPulseMode mode) { record("EPWM_setSyncOutPulseMode", base, mode); }
void EPWM_setTimeBaseCounter(uint32_t base, uint16_t count) { record("EPWM_setTimeBaseCounter", base, count); }
void EPWM_setEmulationMode(uint32_t base, EPWM_EmulationMode emulationMode) { record("EPWM_setEmulationMode", base, emulationMode); }
void EPWM_setCounterCompareShadowLoadMode(uint32_t base, EPWM_CounterCompareModule compModule, EPWM_CounterCompareLoadMode loadMode) { record("EPWM_setCounterCompareShadowLoadMode", base, compModule, loadMode); }
void EPWM_setCounterCompareValue(uint32_t base, EPWM_CounterCompareModule compModule, uint16_t compCount) { record("EPWM_setCounterCompareValue", base, compModule, compCount); }
void EPWM_setActionQualifierAction(uint32_t base, EPWM_ActionQualifierOutputModule epwmOutput, EPWM_ActionQualifierOutput output, EPWM_ActionQualifierOutputEvent event) { record("EPWM_setActionQualifierAction", base, epwmOutput, output, event); }
void EPWM_setDeadBandDelayMode(uint32_t base, EPWM_DeadBandDelayMode delayMode, bool enableDelayMode) { record("EPWM_setDeadBandDelayMode", base, delayMode, enableDelayMode ? 1U : 0U); }
void EPWM_setDeadBandDelayPolarity(uint32_t base, EPWM_DeadBandDelayMode delayMode, EPWM_DeadBandPolarity polarity) { record("EPWM_setDeadBandDelayPolarity", base, delayMode, polarity); }
void EPWM_setRisingEdgeDeadBandDelayInput(uint32_t base, uint16_t input) { record("EPWM_setRisingEdgeDeadBandDelayInput", base, input); }
void EPWM_setFallingEdgeDeadBandDelayInput(uint32_t base, uint16_t input) { record("EPWM_setFallingEdgeDeadBandDelayInput", base, input); }
void EPWM_setRisingEdgeDelayCount(uint32_t base, uint16_t redCount) { record("EPWM_setRisingEdgeDelayCount", base, redCount); }
void EPWM_setFallingEdgeDelayCount(uint32_t base, uint16_t fedCount) { record("EPWM_setFallingEdgeDelayCount", base, fedCount); }
void EPWM_setDeadBandCounterClock(uint32_t base, EPWM_DeadBandClockMode clockMode) { record("EPWM_setDeadBandCounterClock", base, clockMode); }
void EPWM_enableADCTrigger(uint32_t base, EPWM_ADCStartOfConversionType adcSOCType) { record("EPWM_enableADCTrigger", base, adcSOCType); }
void EPWM_setADCTriggerSource(uint32_t base, EPWM_ADCStartOfConversionType adcSOCType, EPWM_ADCStartOfConversionSource socSource) { record("EPWM_setADCTriggerSource", base, adcSOCType, socSource); }
void EPWM_setADCTriggerEventPrescale(uint32_t base, EPWM_ADCStartOfConversionType adcSOCType, uint16_t preScaleCount) { record("EPWM_setADCTriggerEventPrescale", base, adcSOCType, preScaleCount); }
void EPWM_setTripZoneAction(uint32_t base, EPWM_TripZoneEvent tzEvent, EPWM_TripZoneAction tzAction) { record("EPWM_setTripZoneAction", base, tzEvent, tzAction); }
void EPWM_forceTripZoneEvent(uint32_t base, uint16_t tzForceEvent) { record("EPWM_forceTripZoneEvent", base, tzForceEvent); }
void EPWM_clearTripZoneFlag(uint32_t base, uint16_t tzFlags) { record("EPWM_clearTripZoneFlag", base, tzFlags); }
uint16_t EPWM_getTripZoneFlagStatus(uint32_t base) { return tripZoneFlags[base]; }
void EPWM_enableTripZoneSignals(uint32_t base, uint16_t tzSignal) { record("EPWM_enableTripZoneSignals", base, tzSignal); }
void EPWM_selectDigitalCompareTripInput(uint32_t base, EPWM_DigitalCompareTripInput tripSource, EPWM_DigitalCompareType dcType) { record("EPWM_selectDigitalCompareTripInput", base, tripSource, dcType); }
void EPWM_setTripZoneDigitalCompareEventCondition(uint32_t base, EPWM_TripZoneDigitalCompareOutput dcType, EPWM_TripZoneDigitalCompareOutputEvent dcEvent) { record("EPWM_setTripZoneDigitalCompareEventCondition", base, dcType, dcEvent); }
void EPWM_setDigitalCompareEventSource(uint32_t base, EPWM_DigitalCompareModule dcModule, EPWM_DigitalCompareEvent dcEvent, EPWM_DigitalCompareEventSource dcEventSource) { record("EPWM_setDigitalCompareEventSource", base, dcModule, dcEvent, dcEventSource); }

void ADC_setPrescaler(uint32_t base, ADC_ClkPrescale clkPrescale) { record("ADC_setPrescaler", base, clkPrescale); }
void ADC_setMode(uint32_t base, ADC_Resolution resolution, ADC_SignalMode signalMode) { record("ADC_setMode", base, resolution, signalMode); }
void ADC_setInterruptPulseMode(uint32_t base, ADC_PulseMode pulseMode) { record("ADC_setInterruptPulseMode", base, pulseMode); }
void ADC_enableConverter(uint32_t base) { record("ADC_enableConverter", base); }
void ADC_setupSOC(uint32_t base, ADC_SOCNumber socNumber, ADC_Trigger trigger, ADC_Channel channel, uint32_t sampleWindow) { record("ADC_setupSOC", base, socNumber, trigger, channel, sampleWindow); }
void ADC_setInterruptSource(uint32_t base, ADC_IntNumber adcIntNum, uint16_t intTrigger) { record("ADC_setInterruptSource", base, adcIntNum, intTrigger); }
void ADC_enableInterrupt(uint32_t base, ADC_IntNumber adcIntNum) { record("ADC_enableInterrupt", base, adcIntNum); }
void ADC_clearInterruptStatus(uint32_t base, ADC_IntNumber adcIntNum) { record("ADC_clearInterruptStatus", base, adcIntNum); }
void ADC_enableContinuousMode(uint32_t base, ADC_IntNumber adcIntNum) { record("ADC_enableContinuousMode", base, adcIntNum); }
uint16_t ADC_readResult(uint32_t resultBase, ADC_SOCNumber socNumber) { return adcResults[std::make_pair(resultBase, static_cast<uint16_t>(socNumber))]; }
bool ADC_getInterruptOverflowStatus(uint32_t, ADC_IntNumber) { return adcInterruptOverflow; }
void ADC_clearInterruptOverflowStatus(uint32_t base, ADC_IntNumber adcIntNum) { record("ADC_clearInterruptOverflowStatus", base, adcIntNum); }

void CMPSS_enableModule(uint32_t base) { record("CMPSS_enableModule", base); }
void CMPSS_configHighComparator(uint32_t base, uint16_t config) { record("CMPSS_configHighComparator", base, config); }
void CMPSS_configDAC(uint32_t base, uint16_t config) { record("CMPSS_configDAC", base, config); }
void CMPSS_setDACValueHigh(uint32_t base, uint16_t value) { record("CMPSS_setDACValueHigh", base, value); }
void CMPSS_configOutputsHigh(uint32_t base, uint16_t config) { record("CMPSS_configOutputsHigh", base, config); }
void CMPSS_configFilterHigh(uint32_t base, uint16_t samplePrescale, uint16_t sampleWindow, uint16_t threshold) { record("CMPSS_configFilterHigh", base, samplePrescale, sampleWindow, threshold); }
void CMPSS_initFilterHigh(uint32_t base) { record("CMPSS_initFilterHigh", base); }

void XBAR_setEPWMMuxConfig(XBAR_TripNum trip, XBAR_EPWMMuxConfig muxConfig) { record("XBAR_setEPWMMuxConfig", trip, muxConfig); }
void XBAR_enableEPWMMux(XBAR_TripNum trip, uint32_t muxes) { record("XBAR_enableEPWMMux", trip, muxes); }
