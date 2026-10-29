/*
 * File: Current_Control.h
 *
 * Code generated for Simulink model 'Current_Control'.
 *
 * Model version                  : 1.14
 * Simulink Coder version         : 25.2 (R2025b) 28-Jul-2025
 * C/C++ source code generated on : Thu Oct 29 21:47:12 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Texas Instruments->C2000
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef Current_Control_h_
#define Current_Control_h_
#ifndef Current_Control_COMMON_INCLUDES_
#define Current_Control_COMMON_INCLUDES_
#include "rtwtypes.h"
#endif                                 /* Current_Control_COMMON_INCLUDES_ */

#include "Current_Control_types.h"
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
  real32_T EnabledDelay_DSTATE;        /* '<S61>/Enabled Delay' */
  real32_T Integrator_DSTATE;          /* '<S42>/Integrator' */
  int16_T Integrator_PrevResetState;   /* '<S42>/Integrator' */
  boolean_T IC_FirstOutputTime;        /* '<S3>/IC' */
  boolean_T IC1_FirstOutputTime;       /* '<S3>/IC1' */
} DW_Current_Control_T;

/* Zero-crossing (trigger) state */
typedef struct {
  ZCSigState EnabledDelay_Reset_ZCE;   /* '<S61>/Enabled Delay' */
} PrevZCX_Current_Control_T;

/* External inputs (root inport signals with default storage) */
typedef struct {
  CurrentControl_in In1;               /* '<Root>/In1' */
} ExtU_Current_Control_T;

/* External outputs (root outports fed by signals with default storage) */
typedef struct {
  CurrentControl_out Out1;             /* '<Root>/Out1' */
} ExtY_Current_Control_T;

/* Real-time Model Data Structure */
struct tag_RTM_Current_Control_T {
  const char_T * volatile errorStatus;
  PrevZCX_Current_Control_T *prevZCSigState;
  DW_Current_Control_T *dwork;
};

/* Model entry point functions */
extern void Current_Control_initialize(RT_MODEL_Current_Control_T *const
  Current_Control_M);
extern void Current_Control_step(RT_MODEL_Current_Control_T *const
  Current_Control_M, ExtU_Current_Control_T *Current_Control_U,
  ExtY_Current_Control_T *Current_Control_Y);
extern void Current_Control_terminate(RT_MODEL_Current_Control_T *const
  Current_Control_M);
extern volatile boolean_T stopRequested;
extern volatile boolean_T runModel;

/*-
 * These blocks were eliminated from the model due to optimizations:
 *
 * Block '<S50>/Data Type Duplicate' : Unused code path elimination
 * Block '<S50>/Data Type Propagation' : Unused code path elimination
 * Block '<S61>/FixPt Data Type Duplicate' : Unused code path elimination
 * Block '<S62>/Data Type Duplicate' : Unused code path elimination
 * Block '<S62>/Data Type Propagation' : Unused code path elimination
 * Block '<S4>/Constant5' : Unused code path elimination
 * Block '<S4>/Constant6' : Unused code path elimination
 * Block '<S4>/Constant7' : Unused code path elimination
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
 * '<Root>' : 'Current_Control'
 * '<S1>'   : 'Current_Control/Compare To Constant'
 * '<S2>'   : 'Current_Control/PID Controller'
 * '<S3>'   : 'Current_Control/Slew Rate 2'
 * '<S4>'   : 'Current_Control/Subsystem'
 * '<S5>'   : 'Current_Control/PID Controller/Anti-windup'
 * '<S6>'   : 'Current_Control/PID Controller/D Gain'
 * '<S7>'   : 'Current_Control/PID Controller/External Derivative'
 * '<S8>'   : 'Current_Control/PID Controller/Filter'
 * '<S9>'   : 'Current_Control/PID Controller/Filter ICs'
 * '<S10>'  : 'Current_Control/PID Controller/I Gain'
 * '<S11>'  : 'Current_Control/PID Controller/Ideal P Gain'
 * '<S12>'  : 'Current_Control/PID Controller/Ideal P Gain Fdbk'
 * '<S13>'  : 'Current_Control/PID Controller/Integrator'
 * '<S14>'  : 'Current_Control/PID Controller/Integrator ICs'
 * '<S15>'  : 'Current_Control/PID Controller/N Copy'
 * '<S16>'  : 'Current_Control/PID Controller/N Gain'
 * '<S17>'  : 'Current_Control/PID Controller/P Copy'
 * '<S18>'  : 'Current_Control/PID Controller/Parallel P Gain'
 * '<S19>'  : 'Current_Control/PID Controller/Reset Signal'
 * '<S20>'  : 'Current_Control/PID Controller/Saturation'
 * '<S21>'  : 'Current_Control/PID Controller/Saturation Fdbk'
 * '<S22>'  : 'Current_Control/PID Controller/Sum'
 * '<S23>'  : 'Current_Control/PID Controller/Sum Fdbk'
 * '<S24>'  : 'Current_Control/PID Controller/Tracking Mode'
 * '<S25>'  : 'Current_Control/PID Controller/Tracking Mode Sum'
 * '<S26>'  : 'Current_Control/PID Controller/Tsamp - Integral'
 * '<S27>'  : 'Current_Control/PID Controller/Tsamp - Ngain'
 * '<S28>'  : 'Current_Control/PID Controller/postSat Signal'
 * '<S29>'  : 'Current_Control/PID Controller/preInt Signal'
 * '<S30>'  : 'Current_Control/PID Controller/preSat Signal'
 * '<S31>'  : 'Current_Control/PID Controller/Anti-windup/Disc. Clamping Parallel'
 * '<S32>'  : 'Current_Control/PID Controller/Anti-windup/Disc. Clamping Parallel/Dead Zone'
 * '<S33>'  : 'Current_Control/PID Controller/Anti-windup/Disc. Clamping Parallel/Dead Zone/External'
 * '<S34>'  : 'Current_Control/PID Controller/Anti-windup/Disc. Clamping Parallel/Dead Zone/External/Dead Zone Dynamic'
 * '<S35>'  : 'Current_Control/PID Controller/D Gain/Disabled'
 * '<S36>'  : 'Current_Control/PID Controller/External Derivative/Disabled'
 * '<S37>'  : 'Current_Control/PID Controller/Filter/Disabled'
 * '<S38>'  : 'Current_Control/PID Controller/Filter ICs/Disabled'
 * '<S39>'  : 'Current_Control/PID Controller/I Gain/Internal Parameters'
 * '<S40>'  : 'Current_Control/PID Controller/Ideal P Gain/Passthrough'
 * '<S41>'  : 'Current_Control/PID Controller/Ideal P Gain Fdbk/Disabled'
 * '<S42>'  : 'Current_Control/PID Controller/Integrator/Discrete'
 * '<S43>'  : 'Current_Control/PID Controller/Integrator ICs/External IC'
 * '<S44>'  : 'Current_Control/PID Controller/N Copy/Disabled wSignal Specification'
 * '<S45>'  : 'Current_Control/PID Controller/N Gain/Disabled'
 * '<S46>'  : 'Current_Control/PID Controller/P Copy/Disabled'
 * '<S47>'  : 'Current_Control/PID Controller/Parallel P Gain/Internal Parameters'
 * '<S48>'  : 'Current_Control/PID Controller/Reset Signal/External Reset'
 * '<S49>'  : 'Current_Control/PID Controller/Saturation/External'
 * '<S50>'  : 'Current_Control/PID Controller/Saturation/External/Saturation Dynamic'
 * '<S51>'  : 'Current_Control/PID Controller/Saturation Fdbk/Disabled'
 * '<S52>'  : 'Current_Control/PID Controller/Sum/Sum_PI'
 * '<S53>'  : 'Current_Control/PID Controller/Sum Fdbk/Disabled'
 * '<S54>'  : 'Current_Control/PID Controller/Tracking Mode/Disabled'
 * '<S55>'  : 'Current_Control/PID Controller/Tracking Mode Sum/Passthrough'
 * '<S56>'  : 'Current_Control/PID Controller/Tsamp - Integral/External Ts'
 * '<S57>'  : 'Current_Control/PID Controller/Tsamp - Ngain/Passthrough'
 * '<S58>'  : 'Current_Control/PID Controller/postSat Signal/Forward_Path'
 * '<S59>'  : 'Current_Control/PID Controller/preInt Signal/Internal PreInt'
 * '<S60>'  : 'Current_Control/PID Controller/preSat Signal/Forward_Path'
 * '<S61>'  : 'Current_Control/Slew Rate 2/linear'
 * '<S62>'  : 'Current_Control/Slew Rate 2/linear/Saturation Dynamic'
 */
#endif                                 /* Current_Control_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
