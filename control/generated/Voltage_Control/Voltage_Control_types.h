/*
 * File: Voltage_Control_types.h
 *
 * Code generated for Simulink model 'Voltage_Control'.
 *
 * Model version                  : 3.0
 * Simulink Coder version         : 26.2 (R2026b) 22-May-2026
 * C/C++ source code generated on : Thu Nov 12 10:50:32 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Texas Instruments->C2000
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef Voltage_Control_types_h_
#define Voltage_Control_types_h_
#include "rtwtypes.h"
#ifndef DEFINED_TYPEDEF_FOR_VoltageControl_in_
#define DEFINED_TYPEDEF_FOR_VoltageControl_in_

typedef struct {
  real32_T reference;
  real32_T measurement;
  uint16_T enable;
  real32_T slewrate[2];
  uint16_T opmode;
  real32_T openLoopSetpoint;
} VoltageControl_in;

#endif

#ifndef DEFINED_TYPEDEF_FOR_VoltageControl_out_
#define DEFINED_TYPEDEF_FOR_VoltageControl_out_

typedef struct {
  real32_T ctrlOut;
  uint16_T isAtSetpoint;
  real32_T rampedSetpoint;
} VoltageControl_out;

#endif

/* Forward declaration for rtModel */
typedef struct tag_RTM_Voltage_Control_T RT_MODEL_Voltage_Control_T;

#endif                                 /* Voltage_Control_types_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
