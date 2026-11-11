// Host stand-in for C2000Ware driverlib.h. Declares only the driverlib
// functions that the HAL and IPC classes use; FakeDriverlib.cpp implements them.
// Production code includes this header with -std=c++03 -pedantic, so keep it
// C++03.
#ifndef FAKE_DRIVERLIB_H
#define FAKE_DRIVERLIB_H

#include <stdint.h>

typedef enum
{
    CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT = 0x0000,
    CPUTIMER_EMULATIONMODE_STOPATZERO = 0x0400,
    CPUTIMER_EMULATIONMODE_RUNFREE = 0x0800
} CPUTimer_EmulationMode;

// Same names and meaning as driverlib ipc.h. Index 0 is the direction from CPU1
// to CPU2, as seen from IPC_CPU1_L_CPU2_R.
typedef enum
{
    IPC_CPU1_L_CPU2_R,
    IPC_CPU2_L_CPU1_R,
    IPC_TOTAL_NUM
} IPC_Type_t;

#define IPC_ADDR_CORRECTION_ENABLE  true
#define IPC_ADDR_CORRECTION_DISABLE false

void GPIO_writePin(uint32_t pin, uint32_t outVal);
void GPIO_togglePin(uint32_t pin);

void CPUTimer_stopTimer(uint32_t base);
void CPUTimer_startTimer(uint32_t base);
void CPUTimer_setPeriod(uint32_t base, uint32_t periodCount);
void CPUTimer_setPreScaler(uint32_t base, uint16_t prescaler);
void CPUTimer_reloadTimerCounter(uint32_t base);
void CPUTimer_setEmulationMode(uint32_t base, CPUTimer_EmulationMode mode);
void CPUTimer_enableInterrupt(uint32_t base);

void IPC_setFlagLtoR(IPC_Type_t ipcType, uint32_t flags);
void IPC_clearFlagLtoR(IPC_Type_t ipcType, uint32_t flags);
void IPC_ackFlagRtoL(IPC_Type_t ipcType, uint32_t flags);
bool IPC_isFlagBusyLtoR(IPC_Type_t ipcType, uint32_t flags);
bool IPC_isFlagBusyRtoL(IPC_Type_t ipcType, uint32_t flags);
void IPC_sync(IPC_Type_t ipcType, uint32_t flag);
bool IPC_sendCommand(IPC_Type_t ipcType, uint32_t flags, bool addrCorrEnable,
                     uint32_t command, uint32_t addr, uint32_t data);
bool IPC_readCommand(IPC_Type_t ipcType, uint32_t flags, bool addrCorrEnable,
                     uint32_t* command, uint32_t* addr, uint32_t* data);
void IPC_sendResponse(IPC_Type_t ipcType, uint32_t data);
uint32_t IPC_getResponse(IPC_Type_t ipcType);

// ---------------------------------------------------------------------------
// ePWM, ADC, CMPSS and X-BAR types: copied unchanged from C2000Ware 5.02.00.00
// (driverlib/f2837xd/driverlib/epwm.h, adc.h, cmpss.h, xbar.h), so the values
// recorded by the fake are the register values of the device.
// ---------------------------------------------------------------------------
typedef enum
{
    EPWM_COUNTER_MODE_UP = 0,         
    EPWM_COUNTER_MODE_DOWN = 1,       
    EPWM_COUNTER_MODE_UP_DOWN = 2,    
    EPWM_COUNTER_MODE_STOP_FREEZE = 3 
} EPWM_TimeBaseCountMode;

typedef enum
{
    EPWM_CLOCK_DIVIDER_1 = 0,     
    EPWM_CLOCK_DIVIDER_2 = 1,     
    EPWM_CLOCK_DIVIDER_4 = 2,     
    EPWM_CLOCK_DIVIDER_8 = 3,     
    EPWM_CLOCK_DIVIDER_16 = 4,    
    EPWM_CLOCK_DIVIDER_32 = 5,    
    EPWM_CLOCK_DIVIDER_64 = 6,    
    EPWM_CLOCK_DIVIDER_128 = 7    
} EPWM_ClockDivider;

typedef enum
{
    EPWM_HSCLOCK_DIVIDER_1 = 0,     
    EPWM_HSCLOCK_DIVIDER_2 = 1,     
    EPWM_HSCLOCK_DIVIDER_4 = 2,     
    EPWM_HSCLOCK_DIVIDER_6 = 3,     
    EPWM_HSCLOCK_DIVIDER_8 = 4,     
    EPWM_HSCLOCK_DIVIDER_10 = 5,    
    EPWM_HSCLOCK_DIVIDER_12 = 6,    
    EPWM_HSCLOCK_DIVIDER_14 = 7     
} EPWM_HSClockDivider;

typedef enum
{
   EPWM_COUNT_MODE_DOWN_AFTER_SYNC = 0, 
   EPWM_COUNT_MODE_UP_AFTER_SYNC = 1    
} EPWM_SyncCountMode;

typedef enum
{
    EPWM_SYNC_OUT_PULSE_ON_SOFTWARE  = 0,
    EPWM_SYNC_OUT_PULSE_ON_EPWMxSYNCIN = 0,
    EPWM_SYNC_OUT_PULSE_ON_COUNTER_ZERO = 1,
    EPWM_SYNC_OUT_PULSE_ON_COUNTER_COMPARE_B = 2,
    EPWM_SYNC_OUT_PULSE_DISABLED = 4,
    EPWM_SYNC_OUT_PULSE_ON_COUNTER_COMPARE_C = 5,
    EPWM_SYNC_OUT_PULSE_ON_COUNTER_COMPARE_D = 6
} EPWM_SyncOutPulseMode;

typedef enum
{
   EPWM_EMULATION_STOP_AFTER_NEXT_TB = 0,
   EPWM_EMULATION_STOP_AFTER_FULL_CYCLE = 1,
   EPWM_EMULATION_FREE_RUN = 2
} EPWM_EmulationMode;

typedef enum
{
    EPWM_COUNTER_COMPARE_A = 0, 
    EPWM_COUNTER_COMPARE_B = 2, 
    EPWM_COUNTER_COMPARE_C = 5, 
    EPWM_COUNTER_COMPARE_D = 7  
} EPWM_CounterCompareModule;

typedef enum
{
    EPWM_COMP_LOAD_ON_CNTR_ZERO = 0,
    EPWM_COMP_LOAD_ON_CNTR_PERIOD = 1,
    EPWM_COMP_LOAD_ON_CNTR_ZERO_PERIOD = 2,
    EPWM_COMP_LOAD_FREEZE = 3,
    EPWM_COMP_LOAD_ON_SYNC_CNTR_ZERO = 4,
    EPWM_COMP_LOAD_ON_SYNC_CNTR_PERIOD = 5,
    EPWM_COMP_LOAD_ON_SYNC_CNTR_ZERO_PERIOD = 6,
    EPWM_COMP_LOAD_ON_SYNC_ONLY = 8
} EPWM_CounterCompareLoadMode;

typedef enum
{
    EPWM_AQ_OUTPUT_A = 0, 
    EPWM_AQ_OUTPUT_B = 2  
} EPWM_ActionQualifierOutputModule;

typedef enum
{
    EPWM_AQ_OUTPUT_NO_CHANGE = 0,  
    EPWM_AQ_OUTPUT_LOW       = 1,  
    EPWM_AQ_OUTPUT_HIGH      = 2,  
    EPWM_AQ_OUTPUT_TOGGLE    = 3   
} EPWM_ActionQualifierOutput;

typedef enum
{
    EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO       = 0,
    EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD     = 2,
    EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA    = 4,
    EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA  = 6,
    EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB    = 8,
    EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB  = 10,
    EPWM_AQ_OUTPUT_ON_T1_COUNT_UP         = 1,
    EPWM_AQ_OUTPUT_ON_T1_COUNT_DOWN       = 3,
    EPWM_AQ_OUTPUT_ON_T2_COUNT_UP         = 5,
    EPWM_AQ_OUTPUT_ON_T2_COUNT_DOWN       = 7
} EPWM_ActionQualifierOutputEvent;

typedef enum
{
    EPWM_DB_RED = 1, 
    EPWM_DB_FED = 0  
} EPWM_DeadBandDelayMode;

typedef enum
{
    EPWM_DB_POLARITY_ACTIVE_HIGH = 0, 
    EPWM_DB_POLARITY_ACTIVE_LOW  = 1  
} EPWM_DeadBandPolarity;

typedef enum
{
    EPWM_DB_COUNTER_CLOCK_FULL_CYCLE = 0,
    EPWM_DB_COUNTER_CLOCK_HALF_CYCLE = 1
} EPWM_DeadBandClockMode;

typedef enum
{
    EPWM_SOC_A = 0,  
    EPWM_SOC_B = 1   
} EPWM_ADCStartOfConversionType;

typedef enum
{
    EPWM_SOC_DCxEVT1 = 0,
    EPWM_SOC_TBCTR_ZERO = 1,
    EPWM_SOC_TBCTR_PERIOD = 2,
    EPWM_SOC_TBCTR_ZERO_OR_PERIOD = 3,
    EPWM_SOC_TBCTR_U_CMPA = 4,
    EPWM_SOC_TBCTR_U_CMPC = 8,
    EPWM_SOC_TBCTR_D_CMPA = 5,
    EPWM_SOC_TBCTR_D_CMPC = 10,
    EPWM_SOC_TBCTR_U_CMPB = 6,
    EPWM_SOC_TBCTR_U_CMPD = 12,
    EPWM_SOC_TBCTR_D_CMPB = 7,
    EPWM_SOC_TBCTR_D_CMPD = 14
} EPWM_ADCStartOfConversionSource;

typedef enum
{
    EPWM_TZ_ACTION_EVENT_TZA = 0,     
    EPWM_TZ_ACTION_EVENT_TZB = 2,     
    EPWM_TZ_ACTION_EVENT_DCAEVT1 = 4, 
    EPWM_TZ_ACTION_EVENT_DCAEVT2 = 6, 
    EPWM_TZ_ACTION_EVENT_DCBEVT1 = 8, 
    EPWM_TZ_ACTION_EVENT_DCBEVT2 = 10 
} EPWM_TripZoneEvent;

typedef enum
{
    EPWM_TZ_ACTION_HIGH_Z  = 0, 
    EPWM_TZ_ACTION_HIGH    = 1, 
    EPWM_TZ_ACTION_LOW     = 2, 
    EPWM_TZ_ACTION_DISABLE = 3  
} EPWM_TripZoneAction;

typedef enum
{
    EPWM_DC_TRIP_TRIPIN1 = 0,   
    EPWM_DC_TRIP_TRIPIN2 = 1,   
    EPWM_DC_TRIP_TRIPIN3 = 2,   
    EPWM_DC_TRIP_TRIPIN4 = 3,   
    EPWM_DC_TRIP_TRIPIN5 = 4,   
    EPWM_DC_TRIP_TRIPIN6 = 5,   
    EPWM_DC_TRIP_TRIPIN7 = 6,   
    EPWM_DC_TRIP_TRIPIN8 = 7,   
    EPWM_DC_TRIP_TRIPIN9 = 8,   
    EPWM_DC_TRIP_TRIPIN10 = 9,  
    EPWM_DC_TRIP_TRIPIN11 = 10, 
    EPWM_DC_TRIP_TRIPIN12 = 11, 
    EPWM_DC_TRIP_TRIPIN14 = 13, 
    EPWM_DC_TRIP_TRIPIN15 = 14, 
    EPWM_DC_TRIP_COMBINATION = 15 
} EPWM_DigitalCompareTripInput;

typedef enum
{
    EPWM_DC_TYPE_DCAH = 0,  
    EPWM_DC_TYPE_DCAL = 1,  
    EPWM_DC_TYPE_DCBH = 2,  
    EPWM_DC_TYPE_DCBL = 3   
} EPWM_DigitalCompareType;

typedef enum
{
    EPWM_TZ_DC_OUTPUT_A1 = 0, 
    EPWM_TZ_DC_OUTPUT_A2 = 3, 
    EPWM_TZ_DC_OUTPUT_B1 = 6, 
    EPWM_TZ_DC_OUTPUT_B2 = 9  
} EPWM_TripZoneDigitalCompareOutput;

typedef enum
{
    EPWM_TZ_EVENT_DC_DISABLED = 0,       
    EPWM_TZ_EVENT_DCXH_LOW    = 1,       
    EPWM_TZ_EVENT_DCXH_HIGH   = 2,       
    EPWM_TZ_EVENT_DCXL_LOW    = 3,       
    EPWM_TZ_EVENT_DCXL_HIGH   = 4,       
    EPWM_TZ_EVENT_DCXL_HIGH_DCXH_LOW = 5 
} EPWM_TripZoneDigitalCompareOutputEvent;

typedef enum
{
    EPWM_DC_MODULE_A = 0, 
    EPWM_DC_MODULE_B = 1  
} EPWM_DigitalCompareModule;

typedef enum
{
    EPWM_DC_EVENT_1 = 0, 
    EPWM_DC_EVENT_2 = 1  
} EPWM_DigitalCompareEvent;

typedef enum
{
    EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL = 0,
    EPWM_DC_EVENT_SOURCE_FILT_SIGNAL = 1
} EPWM_DigitalCompareEventSource;

#define EPWM_DB_INPUT_EPWMA 0U
#define EPWM_DB_INPUT_EPWMB 1U
#define EPWM_TZ_SIGNAL_OSHT1 0x100U
#define EPWM_TZ_SIGNAL_DCAEVT1 0x4000U
#define EPWM_TZ_FLAG_OST 0x4U
#define EPWM_TZ_INTERRUPT 0x1U
#define EPWM_TZ_FORCE_EVENT_OST 0x4U
typedef enum
{
    ADC_CLK_DIV_1_0 = 0U,            
    ADC_CLK_DIV_2_0 = 2U,            
    ADC_CLK_DIV_2_5 = 3U,            
    ADC_CLK_DIV_3_0 = 4U,            
    ADC_CLK_DIV_3_5 = 5U,            
    ADC_CLK_DIV_4_0 = 6U,            
    ADC_CLK_DIV_4_5 = 7U,            
    ADC_CLK_DIV_5_0 = 8U,            
    ADC_CLK_DIV_5_5 = 9U,            
    ADC_CLK_DIV_6_0 = 10U,           
    ADC_CLK_DIV_6_5 = 11U,           
    ADC_CLK_DIV_7_0 = 12U,           
    ADC_CLK_DIV_7_5 = 13U,           
    ADC_CLK_DIV_8_0 = 14U,           
    ADC_CLK_DIV_8_5 = 15U            
} ADC_ClkPrescale;

typedef enum
{
    ADC_RESOLUTION_12BIT = 0x00U,    
    ADC_RESOLUTION_16BIT = 0x40U     
} ADC_Resolution;

typedef enum
{
    ADC_MODE_SINGLE_ENDED = 0x00U,   
    ADC_MODE_DIFFERENTIAL = 0x80U    
} ADC_SignalMode;

typedef enum
{
    ADC_PULSE_END_OF_ACQ_WIN = 0x00U,
    ADC_PULSE_END_OF_CONV    = 0x04U
} ADC_PulseMode;

typedef enum
{
    ADC_SOC_NUMBER0  = 0U,        
    ADC_SOC_NUMBER1  = 1U,        
    ADC_SOC_NUMBER2  = 2U,        
    ADC_SOC_NUMBER3  = 3U,        
    ADC_SOC_NUMBER4  = 4U,        
    ADC_SOC_NUMBER5  = 5U,        
    ADC_SOC_NUMBER6  = 6U,        
    ADC_SOC_NUMBER7  = 7U,        
    ADC_SOC_NUMBER8  = 8U,        
    ADC_SOC_NUMBER9  = 9U,        
    ADC_SOC_NUMBER10 = 10U,       
    ADC_SOC_NUMBER11 = 11U,       
    ADC_SOC_NUMBER12 = 12U,       
    ADC_SOC_NUMBER13 = 13U,       
    ADC_SOC_NUMBER14 = 14U,       
    ADC_SOC_NUMBER15 = 15U        
} ADC_SOCNumber;

typedef enum
{
    ADC_TRIGGER_SW_ONLY     = 0U,     
    ADC_TRIGGER_CPU1_TINT0  = 1U,     
    ADC_TRIGGER_CPU1_TINT1  = 2U,     
    ADC_TRIGGER_CPU1_TINT2  = 3U,     
    ADC_TRIGGER_GPIO        = 4U,     
    ADC_TRIGGER_EPWM1_SOCA  = 5U,     
    ADC_TRIGGER_EPWM1_SOCB  = 6U,     
    ADC_TRIGGER_EPWM2_SOCA  = 7U,     
    ADC_TRIGGER_EPWM2_SOCB  = 8U,     
    ADC_TRIGGER_EPWM3_SOCA  = 9U,     
    ADC_TRIGGER_EPWM3_SOCB  = 10U,    
    ADC_TRIGGER_EPWM4_SOCA  = 11U,    
    ADC_TRIGGER_EPWM4_SOCB  = 12U,    
    ADC_TRIGGER_EPWM5_SOCA  = 13U,    
    ADC_TRIGGER_EPWM5_SOCB  = 14U,    
    ADC_TRIGGER_EPWM6_SOCA  = 15U,    
    ADC_TRIGGER_EPWM6_SOCB  = 16U,    
    ADC_TRIGGER_EPWM7_SOCA  = 17U,    
    ADC_TRIGGER_EPWM7_SOCB  = 18U,    
    ADC_TRIGGER_EPWM8_SOCA  = 19U,    
    ADC_TRIGGER_EPWM8_SOCB  = 20U,    
    ADC_TRIGGER_EPWM9_SOCA  = 21U,    
    ADC_TRIGGER_EPWM9_SOCB  = 22U,    
    ADC_TRIGGER_EPWM10_SOCA = 23U,    
    ADC_TRIGGER_EPWM10_SOCB = 24U,    
    ADC_TRIGGER_EPWM11_SOCA = 25U,    
    ADC_TRIGGER_EPWM11_SOCB = 26U,    
    ADC_TRIGGER_EPWM12_SOCA = 27U,    
    ADC_TRIGGER_EPWM12_SOCB = 28U,    
    ADC_TRIGGER_CPU2_TINT0  = 29U,    
    ADC_TRIGGER_CPU2_TINT1  = 30U,    
    ADC_TRIGGER_CPU2_TINT2  = 31U     
} ADC_Trigger;

typedef enum
{
    ADC_CH_ADCIN0  = 0U,          
    ADC_CH_ADCIN1  = 1U,          
    ADC_CH_ADCIN2  = 2U,          
    ADC_CH_ADCIN3  = 3U,          
    ADC_CH_ADCIN4  = 4U,          
    ADC_CH_ADCIN5  = 5U,          
    ADC_CH_ADCIN6  = 6U,          
    ADC_CH_ADCIN7  = 7U,          
    ADC_CH_ADCIN8  = 8U,          
    ADC_CH_ADCIN9  = 9U,          
    ADC_CH_ADCIN10 = 10U,         
    ADC_CH_ADCIN11 = 11U,         
    ADC_CH_ADCIN12 = 12U,         
    ADC_CH_ADCIN13 = 13U,         
    ADC_CH_ADCIN14 = 14U,         
    ADC_CH_ADCIN15 = 15U,         
    ADC_CH_ADCIN0_ADCIN1   = 0U,  
    ADC_CH_ADCIN2_ADCIN3   = 2U,  
    ADC_CH_ADCIN4_ADCIN5   = 4U,  
    ADC_CH_ADCIN6_ADCIN7   = 6U,  
    ADC_CH_ADCIN8_ADCIN9   = 8U,  
    ADC_CH_ADCIN10_ADCIN11 = 10U, 
    ADC_CH_ADCIN12_ADCIN13 = 12U, 
    ADC_CH_ADCIN14_ADCIN15 = 14U  
} ADC_Channel;

typedef enum
{
    ADC_INT_NUMBER1 = 0U,        
    ADC_INT_NUMBER2 = 1U,        
    ADC_INT_NUMBER3 = 2U,        
    ADC_INT_NUMBER4 = 3U         
} ADC_IntNumber;

#define CMPSS_INSRC_DAC 0x0000U
#define CMPSS_TRIPOUT_FILTER 0x0020U
#define CMPSS_TRIP_FILTER 0x0008U
#define CMPSS_DACVAL_SYSCLK 0x0000U
#define CMPSS_DACREF_VDDA 0x0000U
#define CMPSS_DACSRC_SHDW 0x0000U
typedef enum
{
    XBAR_TRIP4  = 0,        
    XBAR_TRIP5  = 2,        
    XBAR_TRIP7  = 4,        
    XBAR_TRIP8  = 6,        
    XBAR_TRIP9  = 8,        
    XBAR_TRIP10 = 10,       
    XBAR_TRIP11 = 12,       
    XBAR_TRIP12 = 14        
} XBAR_TripNum;

#define XBAR_MUX00 0x00000001U
// XBAR_EPWMMuxConfig: only the entry used
typedef enum
{
    XBAR_EPWM_MUX00_CMPSS1_CTRIPH = 0x0000
} XBAR_EPWMMuxConfig;

void EPWM_setTimeBasePeriod(uint32_t base, uint16_t periodCount);
void EPWM_setTimeBaseCounterMode(uint32_t base, EPWM_TimeBaseCountMode counterMode);
void EPWM_setClockPrescaler(uint32_t base, EPWM_ClockDivider prescaler,
                            EPWM_HSClockDivider highSpeedPrescaler);
void EPWM_disablePhaseShiftLoad(uint32_t base);
void EPWM_enablePhaseShiftLoad(uint32_t base);
void EPWM_setPhaseShift(uint32_t base, uint16_t phaseCount);
void EPWM_setCountModeAfterSync(uint32_t base, EPWM_SyncCountMode mode);
void EPWM_setSyncOutPulseMode(uint32_t base, EPWM_SyncOutPulseMode mode);
void EPWM_setTimeBaseCounter(uint32_t base, uint16_t count);
void EPWM_setEmulationMode(uint32_t base, EPWM_EmulationMode emulationMode);
void EPWM_setCounterCompareShadowLoadMode(uint32_t base, EPWM_CounterCompareModule compModule,
                                          EPWM_CounterCompareLoadMode loadMode);
void EPWM_setCounterCompareValue(uint32_t base, EPWM_CounterCompareModule compModule,
                                 uint16_t compCount);
void EPWM_setActionQualifierAction(uint32_t base, EPWM_ActionQualifierOutputModule epwmOutput,
                                   EPWM_ActionQualifierOutput output,
                                   EPWM_ActionQualifierOutputEvent event);
void EPWM_setDeadBandDelayMode(uint32_t base, EPWM_DeadBandDelayMode delayMode,
                               bool enableDelayMode);
void EPWM_setDeadBandDelayPolarity(uint32_t base, EPWM_DeadBandDelayMode delayMode,
                                   EPWM_DeadBandPolarity polarity);
void EPWM_setRisingEdgeDeadBandDelayInput(uint32_t base, uint16_t input);
void EPWM_setFallingEdgeDeadBandDelayInput(uint32_t base, uint16_t input);
void EPWM_setRisingEdgeDelayCount(uint32_t base, uint16_t redCount);
void EPWM_setFallingEdgeDelayCount(uint32_t base, uint16_t fedCount);
void EPWM_setDeadBandCounterClock(uint32_t base, EPWM_DeadBandClockMode clockMode);
void EPWM_enableADCTrigger(uint32_t base, EPWM_ADCStartOfConversionType adcSOCType);
void EPWM_setADCTriggerSource(uint32_t base, EPWM_ADCStartOfConversionType adcSOCType,
                              EPWM_ADCStartOfConversionSource socSource);
void EPWM_setADCTriggerEventPrescale(uint32_t base, EPWM_ADCStartOfConversionType adcSOCType,
                                     uint16_t preScaleCount);
void EPWM_setTripZoneAction(uint32_t base, EPWM_TripZoneEvent tzEvent,
                            EPWM_TripZoneAction tzAction);
void EPWM_forceTripZoneEvent(uint32_t base, uint16_t tzForceEvent);
void EPWM_clearTripZoneFlag(uint32_t base, uint16_t tzFlags);
uint16_t EPWM_getTripZoneFlagStatus(uint32_t base);
void EPWM_enableTripZoneSignals(uint32_t base, uint16_t tzSignal);
void EPWM_selectDigitalCompareTripInput(uint32_t base, EPWM_DigitalCompareTripInput tripSource,
                                       EPWM_DigitalCompareType dcType);
void EPWM_setTripZoneDigitalCompareEventCondition(uint32_t base,
                                                  EPWM_TripZoneDigitalCompareOutput dcType,
                                                  EPWM_TripZoneDigitalCompareOutputEvent dcEvent);
void EPWM_setDigitalCompareEventSource(uint32_t base, EPWM_DigitalCompareModule dcModule,
                                       EPWM_DigitalCompareEvent dcEvent,
                                       EPWM_DigitalCompareEventSource dcEventSource);

void ADC_setPrescaler(uint32_t base, ADC_ClkPrescale clkPrescale);
void ADC_setMode(uint32_t base, ADC_Resolution resolution, ADC_SignalMode signalMode);
void ADC_setInterruptPulseMode(uint32_t base, ADC_PulseMode pulseMode);
void ADC_enableConverter(uint32_t base);
void ADC_setupSOC(uint32_t base, ADC_SOCNumber socNumber, ADC_Trigger trigger,
                  ADC_Channel channel, uint32_t sampleWindow);
void ADC_setInterruptSource(uint32_t base, ADC_IntNumber adcIntNum, uint16_t intTrigger);
void ADC_enableInterrupt(uint32_t base, ADC_IntNumber adcIntNum);
void ADC_clearInterruptStatus(uint32_t base, ADC_IntNumber adcIntNum);
void ADC_enableContinuousMode(uint32_t base, ADC_IntNumber adcIntNum);
uint16_t ADC_readResult(uint32_t resultBase, ADC_SOCNumber socNumber);
bool ADC_getInterruptOverflowStatus(uint32_t base, ADC_IntNumber adcIntNum);
void ADC_clearInterruptOverflowStatus(uint32_t base, ADC_IntNumber adcIntNum);

void CMPSS_enableModule(uint32_t base);
void CMPSS_configHighComparator(uint32_t base, uint16_t config);
void CMPSS_configDAC(uint32_t base, uint16_t config);
void CMPSS_setDACValueHigh(uint32_t base, uint16_t value);
void CMPSS_configOutputsHigh(uint32_t base, uint16_t config);
void CMPSS_configFilterHigh(uint32_t base, uint16_t samplePrescale, uint16_t sampleWindow,
                            uint16_t threshold);
void CMPSS_initFilterHigh(uint32_t base);

void XBAR_setEPWMMuxConfig(XBAR_TripNum trip, XBAR_EPWMMuxConfig muxConfig);
void XBAR_enableEPWMMux(XBAR_TripNum trip, uint32_t muxes);

#endif
