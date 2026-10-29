/*
 * File: Current_Control.c
 *
 * Code generated for Simulink model 'Current_Control'.
 *
 * Model version                  : 1.13
 * Simulink Coder version         : 25.2 (R2025b) 28-Jul-2025
 * C/C++ source code generated on : Wed Oct 28 09:38:23 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Texas Instruments->C2000
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#include "Current_Control.h"
#include <math.h>
#include "rtwtypes.h"
#include "zero_crossing_types.h"
#include <string.h>

/* Model step function */
void Current_Control_step(RT_MODEL_Current_Control_T *const Current_Control_M,
  ExtU_Current_Control_T *Current_Control_U, ExtY_Current_Control_T
  *Current_Control_Y)
{
  DW_Current_Control_T *Current_Control_DW = Current_Control_M->dwork;
  PrevZCX_Current_Control_T *Current_Control_PrevZCX =
    Current_Control_M->prevZCSigState;
  real32_T rtb_IC1;
  real32_T rtb_UkYk1;
  real32_T rtb_lo;
  int16_T tmp;
  int16_T tmp_0;
  int16_T tmp_1;
  uint16_T rtb_Cast1;

  /* DataTypeConversion: '<Root>/Cast1' incorporates:
   *  Logic: '<Root>/Logical Operator'
   */
  rtb_Cast1 = (uint16_T)(Current_Control_U->In1.enable == 0U);

  /* Delay: '<S61>/Enabled Delay' */
  tmp_1 = (rtb_Cast1 > 0U);
  if ((((Current_Control_PrevZCX->EnabledDelay_Reset_ZCE == POS_ZCSIG) != tmp_1)
       && (Current_Control_PrevZCX->EnabledDelay_Reset_ZCE !=
           UNINITIALIZED_ZCSIG)) || (rtb_Cast1 != 0U)) {
    Current_Control_DW->EnabledDelay_DSTATE = 0.0F;
  }

  Current_Control_PrevZCX->EnabledDelay_Reset_ZCE = (ZCSigState)(rtb_Cast1 > 0U);

  /* InitialCondition: '<S3>/IC' incorporates:
   *  Inport: '<Root>/In1'
   */
  if (Current_Control_DW->IC_FirstOutputTime) {
    Current_Control_DW->IC_FirstOutputTime = false;
    rtb_lo = 100000.0F;
  } else {
    rtb_lo = Current_Control_U->In1.slewrate[0];
  }

  /* End of InitialCondition: '<S3>/IC' */

  /* Product: '<S61>/delta rise limit' incorporates:
   *  Abs: '<S61>/Abs'
   *  Constant: '<Root>/Constant'
   */
  rtb_lo = fabsf(rtb_lo) * 0.0001F;

  /* Sum: '<S61>/Difference Inputs1' incorporates:
   *  Delay: '<S61>/Enabled Delay'
   *
   * Block description for '<S61>/Difference Inputs1':
   *
   *  Add in CPU
   */
  rtb_UkYk1 = Current_Control_U->In1.reference -
    Current_Control_DW->EnabledDelay_DSTATE;

  /* InitialCondition: '<S3>/IC1' incorporates:
   *  Inport: '<Root>/In1'
   */
  if (Current_Control_DW->IC1_FirstOutputTime) {
    Current_Control_DW->IC1_FirstOutputTime = false;
    rtb_IC1 = -100000.0F;
  } else {
    rtb_IC1 = Current_Control_U->In1.slewrate[1];
  }

  /* End of InitialCondition: '<S3>/IC1' */

  /* Switch: '<S62>/Switch2' incorporates:
   *  RelationalOperator: '<S62>/LowerRelop1'
   */
  if (rtb_UkYk1 <= rtb_lo) {
    /* Product: '<S61>/delta fall limit' incorporates:
     *  Abs: '<S61>/Abs1'
     *  Constant: '<Root>/Constant'
     *  Gain: '<S61>/Gain'
     */
    rtb_lo = -fabsf(rtb_IC1) * 0.0001F;

    /* Switch: '<S62>/Switch' incorporates:
     *  RelationalOperator: '<S62>/UpperRelop'
     */
    if (rtb_UkYk1 >= rtb_lo) {
      rtb_lo = rtb_UkYk1;
    }

    /* End of Switch: '<S62>/Switch' */
  }

  /* End of Switch: '<S62>/Switch2' */

  /* Sum: '<S61>/Difference Inputs2' incorporates:
   *  Delay: '<S61>/Enabled Delay'
   *
   * Block description for '<S61>/Difference Inputs2':
   *
   *  Add in CPU
   */
  Current_Control_DW->EnabledDelay_DSTATE += rtb_lo;

  /* Sum: '<Root>/Subtract' incorporates:
   *  Delay: '<S61>/Enabled Delay'
   */
  rtb_lo = Current_Control_DW->EnabledDelay_DSTATE -
    Current_Control_U->In1.measurement;

  /* DataTypeConversion: '<Root>/Cast' incorporates:
   *  Abs: '<Root>/Abs'
   *  Constant: '<S1>/Constant'
   *  RelationalOperator: '<S1>/Compare'
   */
  Current_Control_Y->Out1.isAtSetpoint = (uint16_T)(fabsf(rtb_lo) <= 0.5F);

  /* DiscreteIntegrator: '<S42>/Integrator' incorporates:
   *  Constant: '<Root>/Constant4'
   */
  if ((rtb_Cast1 != 0U) || (Current_Control_DW->Integrator_PrevResetState != 0))
  {
    Current_Control_DW->Integrator_DSTATE = 0.0F;
  }

  /* Sum: '<S52>/Sum' incorporates:
   *  DiscreteIntegrator: '<S42>/Integrator'
   *  Gain: '<S47>/Proportional Gain'
   */
  rtb_UkYk1 = 0.01F * rtb_lo + Current_Control_DW->Integrator_DSTATE;

  /* Switch: '<S50>/Switch2' incorporates:
   *  Constant: '<Root>/Constant2'
   *  Constant: '<Root>/Constant3'
   *  RelationalOperator: '<S50>/LowerRelop1'
   *  RelationalOperator: '<S50>/UpperRelop'
   *  Switch: '<S50>/Switch'
   */
  if (rtb_UkYk1 > 0.95F) {
    /* BusCreator generated from: '<Root>/Out1' incorporates:
     *  Outport: '<Root>/Out1'
     */
    Current_Control_Y->Out1.ctrlOut = 0.95F;
  } else if (rtb_UkYk1 < 0.0F) {
    /* Switch: '<S50>/Switch' incorporates:
     *  BusCreator generated from: '<Root>/Out1'
     *  Constant: '<Root>/Constant3'
     *  Outport: '<Root>/Out1'
     */
    Current_Control_Y->Out1.ctrlOut = 0.0F;
  } else {
    /* BusCreator generated from: '<Root>/Out1' incorporates:
     *  Outport: '<Root>/Out1'
     *  Switch: '<S50>/Switch'
     */
    Current_Control_Y->Out1.ctrlOut = rtb_UkYk1;
  }

  /* End of Switch: '<S50>/Switch2' */

  /* BusCreator generated from: '<Root>/Out1' incorporates:
   *  Delay: '<S61>/Enabled Delay'
   *  Outport: '<Root>/Out1'
   */
  Current_Control_Y->Out1.rampedSetpoint =
    Current_Control_DW->EnabledDelay_DSTATE;

  /* Switch: '<S34>/Switch' incorporates:
   *  Constant: '<Root>/Constant2'
   *  Constant: '<Root>/Constant3'
   *  RelationalOperator: '<S34>/u_GTE_up'
   *  RelationalOperator: '<S34>/u_GT_lo'
   *  Switch: '<S34>/Switch1'
   */
  if (rtb_UkYk1 >= 0.95F) {
    rtb_IC1 = 0.95F;
  } else if (rtb_UkYk1 > 0.0F) {
    /* Switch: '<S34>/Switch1' */
    rtb_IC1 = rtb_UkYk1;
  } else {
    rtb_IC1 = 0.0F;
  }

  /* Sum: '<S34>/Diff' incorporates:
   *  Switch: '<S34>/Switch'
   */
  rtb_UkYk1 -= rtb_IC1;

  /* Gain: '<S39>/Integral Gain' */
  rtb_lo *= 0.5F;

  /* Update for DiscreteIntegrator: '<S42>/Integrator' incorporates:
   *  Constant: '<Root>/Constant1'
   *  Product: '<S56>/Uintegral*Ts Prod Out'
   *  Switch: '<S31>/Switch'
   */
  if (rtb_Cast1 == 0U) {
    /* Switch: '<S31>/Switch1' incorporates:
     *  Constant: '<S31>/Clamping_zero'
     *  Constant: '<S31>/Constant'
     *  Constant: '<S31>/Constant2'
     *  RelationalOperator: '<S31>/fix for DT propagation issue'
     */
    if (rtb_UkYk1 > 0.0F) {
      tmp = 1;
    } else {
      tmp = -1;
    }

    /* Switch: '<S31>/Switch2' incorporates:
     *  Constant: '<S31>/Clamping_zero'
     *  Constant: '<S31>/Constant3'
     *  Constant: '<S31>/Constant4'
     *  RelationalOperator: '<S31>/fix for DT propagation issue1'
     */
    if (rtb_lo > 0.0F) {
      tmp_0 = 1;
    } else {
      tmp_0 = -1;
    }

    /* Switch: '<S31>/Switch' incorporates:
     *  Constant: '<S31>/Clamping_zero'
     *  Constant: '<S31>/Constant1'
     *  Logic: '<S31>/AND3'
     *  RelationalOperator: '<S31>/Equal1'
     *  RelationalOperator: '<S31>/Relational Operator'
     *  Switch: '<S31>/Switch1'
     *  Switch: '<S31>/Switch2'
     */
    if ((rtb_UkYk1 != 0.0F) && (tmp == tmp_0)) {
      rtb_lo = 0.0F;
    }

    Current_Control_DW->Integrator_DSTATE += rtb_lo * 0.0001F;
  }

  Current_Control_DW->Integrator_PrevResetState = tmp_1;

  /* End of Update for DiscreteIntegrator: '<S42>/Integrator' */
}

/* Model initialize function */
void Current_Control_initialize(RT_MODEL_Current_Control_T *const
  Current_Control_M)
{
  DW_Current_Control_T *Current_Control_DW = Current_Control_M->dwork;
  PrevZCX_Current_Control_T *Current_Control_PrevZCX =
    Current_Control_M->prevZCSigState;

  /* Registration code */

  /* states (dwork) */
  (void) memset((void *)Current_Control_DW, 0,
                sizeof(DW_Current_Control_T));

  /* Start for InitialCondition: '<S3>/IC' */
  Current_Control_DW->IC_FirstOutputTime = true;

  /* Start for InitialCondition: '<S3>/IC1' */
  Current_Control_DW->IC1_FirstOutputTime = true;
  Current_Control_PrevZCX->EnabledDelay_Reset_ZCE = UNINITIALIZED_ZCSIG;

  /* InitializeConditions for Delay: '<S61>/Enabled Delay' */
  Current_Control_DW->EnabledDelay_DSTATE = 0.0F;

  /* InitializeConditions for DiscreteIntegrator: '<S42>/Integrator' */
  Current_Control_DW->Integrator_DSTATE = 0.0F;
  Current_Control_DW->Integrator_PrevResetState = 0;
}

/* Model terminate function */
void Current_Control_terminate(RT_MODEL_Current_Control_T *const
  Current_Control_M)
{
  /* (no terminate code required) */
  UNUSED_PARAMETER(Current_Control_M);
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
