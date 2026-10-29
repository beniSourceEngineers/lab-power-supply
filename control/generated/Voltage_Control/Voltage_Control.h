/*
 * File: Voltage_Control.h
 *
 * Code generated for Simulink model 'Voltage_Control'.
 *
 * Model version                  : 1.7
 * Simulink Coder version         : 25.2 (R2025b) 28-Jul-2025
 * C/C++ source code generated on : Thu Oct 29 21:47:30 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Texas Instruments->C2000
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef Voltage_Control_h_
#define Voltage_Control_h_
#ifndef Voltage_Control_COMMON_INCLUDES_
#define Voltage_Control_COMMON_INCLUDES_
#include "rtwtypes.h"
#endif                                 /* Voltage_Control_COMMON_INCLUDES_ */

#include "Voltage_Control_types.h"
#include <string.h>
#include "rt_defines.h"
#include <stddef.h>
#include "zero_crossing_types.h"

/* Macros for accessing real-time model data structure */
#ifndef rtmGetPrevZCSigState
#define rtmGetPrevZCSigState(rtm)      ((rtm)->prevZCSigState)
#endif

#ifndef rtmSetPrevZCSigState
#define rtmSetPrevZCSigState(rtm, val) ((rtm)->prevZCSigState = (val))
#endif

#ifndef rtmGetRootDWork
#define rtmGetRootDWork(rtm)           ((rtm)->dwork)
#endif

#ifndef rtmSetRootDWork
#define rtmSetRootDWork(rtm, val)      ((rtm)->dwork = (val))
#endif

#ifndef rtmGetErrorStatus
#define rtmGetErrorStatus(rtm)         ((rtm)->errorStatus)
#endif

#ifndef rtmSetErrorStatus
#define rtmSetErrorStatus(rtm, val)    ((rtm)->errorStatus = (val))
#endif

/* Block states (default storage) for system '<Root>' */
typedef struct {
  real32_T EnabledDelay_DSTATE;        /* '<S62>/Enabled Delay' */
  real32_T Integrator_DSTATE;          /* '<S43>/Integrator' */
  int16_T Integrator_PrevResetState;   /* '<S43>/Integrator' */
  boolean_T IC_FirstOutputTime;        /* '<S3>/IC' */
  boolean_T IC1_FirstOutputTime;       /* '<S3>/IC1' */
} DW_Voltage_Control_T;

/* Zero-crossing (trigger) state */
typedef struct {
  ZCSigState EnabledDelay_Reset_ZCE;   /* '<S62>/Enabled Delay' */
} PrevZCX_Voltage_Control_T;

/* External inputs (root inport signals with default storage) */
typedef struct {
  VoltageControl_in In1;               /* '<Root>/In1' */
} ExtU_Voltage_Control_T;

/* External outputs (root outports fed by signals with default storage) */
typedef struct {
  VoltageControl_out Out1;             /* '<Root>/Out1' */
} ExtY_Voltage_Control_T;

/* Real-time Model Data Structure */
struct tag_RTM_Voltage_Control_T {
  const char_T * volatile errorStatus;
  PrevZCX_Voltage_Control_T *prevZCSigState;
  DW_Voltage_Control_T *dwork;
};

/* Model entry point functions */
extern void Voltage_Control_initialize(RT_MODEL_Voltage_Control_T *const
  Voltage_Control_M);
extern void Voltage_Control_step(RT_MODEL_Voltage_Control_T *const
  Voltage_Control_M, ExtU_Voltage_Control_T *Voltage_Control_U,
  ExtY_Voltage_Control_T *Voltage_Control_Y);
extern void Voltage_Control_terminate(RT_MODEL_Voltage_Control_T *const
  Voltage_Control_M);
extern volatile boolean_T stopRequested;
extern volatile boolean_T runModel;

/*-
 * These blocks were eliminated from the model due to optimizations:
 *
 * Block '<S51>/Data Type Duplicate' : Unused code path elimination
 * Block '<S51>/Data Type Propagation' : Unused code path elimination
 * Block '<S62>/FixPt Data Type Duplicate' : Unused code path elimination
 * Block '<S63>/Data Type Duplicate' : Unused code path elimination
 * Block '<S63>/Data Type Propagation' : Unused code path elimination
 * Block '<S4>/Constant5' : Unused code path elimination
 * Block '<S4>/Constant6' : Unused code path elimination
 * Block '<S4>/Constant7' : Unused code path elimination
 * Block '<S40>/Integral Gain' : Eliminated nontunable gain of 1
 */

/*-
 * The generated code includes comments that allow you to trace directly
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : 'Voltage_Control'
 * '<S1>'   : 'Voltage_Control/Compare To Constant'
 * '<S2>'   : 'Voltage_Control/PID Controller'
 * '<S3>'   : 'Voltage_Control/Slew Rate 2'
 * '<S4>'   : 'Voltage_Control/Subsystem'
 * '<S5>'   : 'Voltage_Control/Subsystem1'
 * '<S6>'   : 'Voltage_Control/PID Controller/Anti-windup'
 * '<S7>'   : 'Voltage_Control/PID Controller/D Gain'
 * '<S8>'   : 'Voltage_Control/PID Controller/External Derivative'
 * '<S9>'   : 'Voltage_Control/PID Controller/Filter'
 * '<S10>'  : 'Voltage_Control/PID Controller/Filter ICs'
 * '<S11>'  : 'Voltage_Control/PID Controller/I Gain'
 * '<S12>'  : 'Voltage_Control/PID Controller/Ideal P Gain'
 * '<S13>'  : 'Voltage_Control/PID Controller/Ideal P Gain Fdbk'
 * '<S14>'  : 'Voltage_Control/PID Controller/Integrator'
 * '<S15>'  : 'Voltage_Control/PID Controller/Integrator ICs'
 * '<S16>'  : 'Voltage_Control/PID Controller/N Copy'
 * '<S17>'  : 'Voltage_Control/PID Controller/N Gain'
 * '<S18>'  : 'Voltage_Control/PID Controller/P Copy'
 * '<S19>'  : 'Voltage_Control/PID Controller/Parallel P Gain'
 * '<S20>'  : 'Voltage_Control/PID Controller/Reset Signal'
 * '<S21>'  : 'Voltage_Control/PID Controller/Saturation'
 * '<S22>'  : 'Voltage_Control/PID Controller/Saturation Fdbk'
 * '<S23>'  : 'Voltage_Control/PID Controller/Sum'
 * '<S24>'  : 'Voltage_Control/PID Controller/Sum Fdbk'
 * '<S25>'  : 'Voltage_Control/PID Controller/Tracking Mode'
 * '<S26>'  : 'Voltage_Control/PID Controller/Tracking Mode Sum'
 * '<S27>'  : 'Voltage_Control/PID Controller/Tsamp - Integral'
 * '<S28>'  : 'Voltage_Control/PID Controller/Tsamp - Ngain'
 * '<S29>'  : 'Voltage_Control/PID Controller/postSat Signal'
 * '<S30>'  : 'Voltage_Control/PID Controller/preInt Signal'
 * '<S31>'  : 'Voltage_Control/PID Controller/preSat Signal'
 * '<S32>'  : 'Voltage_Control/PID Controller/Anti-windup/Disc. Clamping Parallel'
 * '<S33>'  : 'Voltage_Control/PID Controller/Anti-windup/Disc. Clamping Parallel/Dead Zone'
 * '<S34>'  : 'Voltage_Control/PID Controller/Anti-windup/Disc. Clamping Parallel/Dead Zone/External'
 * '<S35>'  : 'Voltage_Control/PID Controller/Anti-windup/Disc. Clamping Parallel/Dead Zone/External/Dead Zone Dynamic'
 * '<S36>'  : 'Voltage_Control/PID Controller/D Gain/Disabled'
 * '<S37>'  : 'Voltage_Control/PID Controller/External Derivative/Disabled'
 * '<S38>'  : 'Voltage_Control/PID Controller/Filter/Disabled'
 * '<S39>'  : 'Voltage_Control/PID Controller/Filter ICs/Disabled'
 * '<S40>'  : 'Voltage_Control/PID Controller/I Gain/Internal Parameters'
 * '<S41>'  : 'Voltage_Control/PID Controller/Ideal P Gain/Passthrough'
 * '<S42>'  : 'Voltage_Control/PID Controller/Ideal P Gain Fdbk/Disabled'
 * '<S43>'  : 'Voltage_Control/PID Controller/Integrator/Discrete'
 * '<S44>'  : 'Voltage_Control/PID Controller/Integrator ICs/External IC'
 * '<S45>'  : 'Voltage_Control/PID Controller/N Copy/Disabled wSignal Specification'
 * '<S46>'  : 'Voltage_Control/PID Controller/N Gain/Disabled'
 * '<S47>'  : 'Voltage_Control/PID Controller/P Copy/Disabled'
 * '<S48>'  : 'Voltage_Control/PID Controller/Parallel P Gain/Internal Parameters'
 * '<S49>'  : 'Voltage_Control/PID Controller/Reset Signal/External Reset'
 * '<S50>'  : 'Voltage_Control/PID Controller/Saturation/External'
 * '<S51>'  : 'Voltage_Control/PID Controller/Saturation/External/Saturation Dynamic'
 * '<S52>'  : 'Voltage_Control/PID Controller/Saturation Fdbk/Disabled'
 * '<S53>'  : 'Voltage_Control/PID Controller/Sum/Sum_PI'
 * '<S54>'  : 'Voltage_Control/PID Controller/Sum Fdbk/Disabled'
 * '<S55>'  : 'Voltage_Control/PID Controller/Tracking Mode/Disabled'
 * '<S56>'  : 'Voltage_Control/PID Controller/Tracking Mode Sum/Passthrough'
 * '<S57>'  : 'Voltage_Control/PID Controller/Tsamp - Integral/External Ts'
 * '<S58>'  : 'Voltage_Control/PID Controller/Tsamp - Ngain/Passthrough'
 * '<S59>'  : 'Voltage_Control/PID Controller/postSat Signal/Forward_Path'
 * '<S60>'  : 'Voltage_Control/PID Controller/preInt Signal/Internal PreInt'
 * '<S61>'  : 'Voltage_Control/PID Controller/preSat Signal/Forward_Path'
 * '<S62>'  : 'Voltage_Control/Slew Rate 2/linear'
 * '<S63>'  : 'Voltage_Control/Slew Rate 2/linear/Saturation Dynamic'
 * '<S64>'  : 'Voltage_Control/Subsystem1/Compare To Constant1'
 */
#endif                                 /* Voltage_Control_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
