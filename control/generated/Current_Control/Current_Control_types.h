/*
 * File: Current_Control_types.h
 *
 * Code generated for Simulink model 'Current_Control'.
 *
 * Model version                  : 3.1
 * Simulink Coder version         : 26.2 (R2026b) 22-May-2026
 * C/C++ source code generated on : Mon Nov 16 16:11:04 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Texas Instruments->C2000
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef Current_Control_types_h_
#define Current_Control_types_h_
#include "rtwtypes.h"
#ifndef DEFINED_TYPEDEF_FOR_CurrentControl_in_
#define DEFINED_TYPEDEF_FOR_CurrentControl_in_

typedef struct {
  real32_T reference;
  real32_T measurement;
  uint16_T enable;
  real32_T slewrate[2];

  /* Measured DC link 1 voltage (buck input). Used for the input-voltage feedforward: duty = PI output [V] / dcLinkVoltage. */
  real32_T dcLinkVoltage;
} CurrentControl_in;

#endif

#ifndef DEFINED_TYPEDEF_FOR_CurrentControl_out_
#define DEFINED_TYPEDEF_FOR_CurrentControl_out_

typedef struct {
  real32_T ctrlOut;
  uint16_T isAtSetpoint;
  real32_T rampedSetpoint;
} CurrentControl_out;

#endif

/* Forward declaration for rtModel */
typedef struct tag_RTM_Current_Control_T RT_MODEL_Current_Control_T;

#endif                                 /* Current_Control_types_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
