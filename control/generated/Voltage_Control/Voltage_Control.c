/*
 * File: Voltage_Control.c
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

#include "Voltage_Control.h"
#include <math.h>
#include "rtwtypes.h"
#include "zero_crossing_types.h"
#include <string.h>

/* Model step function */
void Voltage_Control_step(RT_MODEL_Voltage_Control_T *const Voltage_Control_M,
  ExtU_Voltage_Control_T *Voltage_Control_U, ExtY_Voltage_Control_T
  *Voltage_Control_Y)
{
  DW_Voltage_Control_T *Voltage_Control_DW = Voltage_Control_M->dwork;
  PrevZCX_Voltage_Control_T *Voltage_Control_PrevZCX =
    Voltage_Control_M->prevZCSigState;
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
  rtb_Cast1 = (uint16_T)(Voltage_Control_U->In1.enable == 0U);

  /* Delay: '<S62>/Enabled Delay' */
  tmp_1 = (rtb_Cast1 > 0U);
  if ((((Voltage_Control_PrevZCX->EnabledDelay_Reset_ZCE == POS_ZCSIG) != tmp_1)
       && (Voltage_Control_PrevZCX->EnabledDelay_Reset_ZCE !=
           UNINITIALIZED_ZCSIG)) || (rtb_Cast1 != 0U)) {
    Voltage_Control_DW->EnabledDelay_DSTATE = 0.0F;
  }

  Voltage_Control_PrevZCX->EnabledDelay_Reset_ZCE = (ZCSigState)(rtb_Cast1 > 0U);

  /* InitialCondition: '<S3>/IC' incorporates:
   *  Inport: '<Root>/In1'
   */
  if (Voltage_Control_DW->IC_FirstOutputTime) {
    Voltage_Control_DW->IC_FirstOutputTime = false;
    rtb_lo = 100000.0F;
  } else {
    rtb_lo = Voltage_Control_U->In1.slewrate[0];
  }

  /* End of InitialCondition: '<S3>/IC' */

  /* Product: '<S62>/delta rise limit' incorporates:
   *  Abs: '<S62>/Abs'
   *  Constant: '<Root>/Constant'
   */
  rtb_lo = fabsf(rtb_lo) * 0.0001F;

  /* Sum: '<S62>/Difference Inputs1' incorporates:
   *  Delay: '<S62>/Enabled Delay'
   *
   * Block description for '<S62>/Difference Inputs1':
   *
   *  Add in CPU
   */
  rtb_UkYk1 = Voltage_Control_U->In1.reference -
    Voltage_Control_DW->EnabledDelay_DSTATE;

  /* InitialCondition: '<S3>/IC1' incorporates:
   *  Inport: '<Root>/In1'
   */
  if (Voltage_Control_DW->IC1_FirstOutputTime) {
    Voltage_Control_DW->IC1_FirstOutputTime = false;
    rtb_IC1 = -100000.0F;
  } else {
    rtb_IC1 = Voltage_Control_U->In1.slewrate[1];
  }

  /* End of InitialCondition: '<S3>/IC1' */

  /* Switch: '<S63>/Switch2' incorporates:
   *  RelationalOperator: '<S63>/LowerRelop1'
   */
  if (rtb_UkYk1 <= rtb_lo) {
    /* Product: '<S62>/delta fall limit' incorporates:
     *  Abs: '<S62>/Abs1'
     *  Constant: '<Root>/Constant'
     *  Gain: '<S62>/Gain'
     */
    rtb_lo = -fabsf(rtb_IC1) * 0.0001F;

    /* Switch: '<S63>/Switch' incorporates:
     *  RelationalOperator: '<S63>/UpperRelop'
     */
    if (rtb_UkYk1 >= rtb_lo) {
      rtb_lo = rtb_UkYk1;
    }

    /* End of Switch: '<S63>/Switch' */
  }

  /* End of Switch: '<S63>/Switch2' */

  /* Sum: '<S62>/Difference Inputs2' incorporates:
   *  Delay: '<S62>/Enabled Delay'
   *
   * Block description for '<S62>/Difference Inputs2':
   *
   *  Add in CPU
   */
  Voltage_Control_DW->EnabledDelay_DSTATE += rtb_lo;

  /* Sum: '<Root>/Subtract' incorporates:
   *  Delay: '<S62>/Enabled Delay'
   */
  rtb_lo = Voltage_Control_DW->EnabledDelay_DSTATE -
    Voltage_Control_U->In1.measurement;

  /* DataTypeConversion: '<Root>/Cast' incorporates:
   *  Abs: '<Root>/Abs'
   *  Constant: '<S1>/Constant'
   *  RelationalOperator: '<S1>/Compare'
   */
  Voltage_Control_Y->Out1.isAtSetpoint = (uint16_T)(fabsf(rtb_lo) <= 0.5F);

  /* DiscreteIntegrator: '<S43>/Integrator' incorporates:
   *  Constant: '<Root>/Constant4'
   */
  if ((rtb_Cast1 != 0U) || (Voltage_Control_DW->Integrator_PrevResetState != 0))
  {
    Voltage_Control_DW->Integrator_DSTATE = 0.0F;
  }

  /* Sum: '<S53>/Sum' incorporates:
   *  DiscreteIntegrator: '<S43>/Integrator'
   *  Gain: '<S48>/Proportional Gain'
   */
  rtb_UkYk1 = 0.01F * rtb_lo + Voltage_Control_DW->Integrator_DSTATE;

  /* Switch: '<S5>/Switch' incorporates:
   *  Constant: '<Root>/Constant2'
   *  Constant: '<Root>/Constant3'
   *  Constant: '<S64>/Constant'
   *  RelationalOperator: '<S51>/LowerRelop1'
   *  RelationalOperator: '<S51>/UpperRelop'
   *  RelationalOperator: '<S64>/Compare'
   *  Switch: '<S51>/Switch'
   *  Switch: '<S51>/Switch2'
   */
  if (Voltage_Control_U->In1.opmode == 2U) {
    /* BusCreator generated from: '<Root>/Out1' incorporates:
     *  Outport: '<Root>/Out1'
     */
    Voltage_Control_Y->Out1.ctrlOut = Voltage_Control_U->In1.openLoopSetpoint;
  } else if (rtb_UkYk1 > 15.0F) {
    /* Switch: '<S51>/Switch2' incorporates:
     *  BusCreator generated from: '<Root>/Out1'
     *  Constant: '<Root>/Constant2'
     *  Outport: '<Root>/Out1'
     */
    Voltage_Control_Y->Out1.ctrlOut = 15.0F;
  } else if (rtb_UkYk1 < 0.0F) {
    /* Switch: '<S51>/Switch' incorporates:
     *  BusCreator generated from: '<Root>/Out1'
     *  Constant: '<Root>/Constant3'
     *  Outport: '<Root>/Out1'
     *  Switch: '<S51>/Switch2'
     */
    Voltage_Control_Y->Out1.ctrlOut = 0.0F;
  } else {
    /* BusCreator generated from: '<Root>/Out1' incorporates:
     *  Outport: '<Root>/Out1'
     *  Switch: '<S51>/Switch'
     *  Switch: '<S51>/Switch2'
     */
    Voltage_Control_Y->Out1.ctrlOut = rtb_UkYk1;
  }

  /* End of Switch: '<S5>/Switch' */

  /* BusCreator generated from: '<Root>/Out1' incorporates:
   *  Delay: '<S62>/Enabled Delay'
   *  Outport: '<Root>/Out1'
   */
  Voltage_Control_Y->Out1.rampedSetpoint =
    Voltage_Control_DW->EnabledDelay_DSTATE;

  /* Switch: '<S35>/Switch' incorporates:
   *  Constant: '<Root>/Constant2'
   *  Constant: '<Root>/Constant3'
   *  RelationalOperator: '<S35>/u_GTE_up'
   *  RelationalOperator: '<S35>/u_GT_lo'
   *  Switch: '<S35>/Switch1'
   */
  if (rtb_UkYk1 >= 15.0F) {
    rtb_IC1 = 15.0F;
  } else if (rtb_UkYk1 > 0.0F) {
    /* Switch: '<S35>/Switch1' */
    rtb_IC1 = rtb_UkYk1;
  } else {
    rtb_IC1 = 0.0F;
  }

  /* Sum: '<S35>/Diff' incorporates:
   *  Switch: '<S35>/Switch'
   */
  rtb_UkYk1 -= rtb_IC1;

  /* Update for DiscreteIntegrator: '<S43>/Integrator' incorporates:
   *  Constant: '<Root>/Constant1'
   *  Product: '<S57>/Uintegral*Ts Prod Out'
   *  Switch: '<S32>/Switch'
   */
  if (rtb_Cast1 == 0U) {
    /* Switch: '<S32>/Switch1' incorporates:
     *  Constant: '<S32>/Clamping_zero'
     *  Constant: '<S32>/Constant'
     *  Constant: '<S32>/Constant2'
     *  RelationalOperator: '<S32>/fix for DT propagation issue'
     */
    if (rtb_UkYk1 > 0.0F) {
      tmp = 1;
    } else {
      tmp = -1;
    }

    /* Switch: '<S32>/Switch2' incorporates:
     *  Constant: '<S32>/Clamping_zero'
     *  Constant: '<S32>/Constant3'
     *  Constant: '<S32>/Constant4'
     *  RelationalOperator: '<S32>/fix for DT propagation issue1'
     */
    if (rtb_lo > 0.0F) {
      tmp_0 = 1;
    } else {
      tmp_0 = -1;
    }

    /* Switch: '<S32>/Switch' incorporates:
     *  Constant: '<S32>/Clamping_zero'
     *  Constant: '<S32>/Constant1'
     *  Logic: '<S32>/AND3'
     *  RelationalOperator: '<S32>/Equal1'
     *  RelationalOperator: '<S32>/Relational Operator'
     *  Switch: '<S32>/Switch1'
     *  Switch: '<S32>/Switch2'
     */
    if ((rtb_UkYk1 != 0.0F) && (tmp == tmp_0)) {
      rtb_lo = 0.0F;
    }

    Voltage_Control_DW->Integrator_DSTATE += rtb_lo * 0.0001F;
  }

  Voltage_Control_DW->Integrator_PrevResetState = tmp_1;

  /* End of Update for DiscreteIntegrator: '<S43>/Integrator' */
}

/* Model initialize function */
void Voltage_Control_initialize(RT_MODEL_Voltage_Control_T *const
  Voltage_Control_M)
{
  DW_Voltage_Control_T *Voltage_Control_DW = Voltage_Control_M->dwork;
  PrevZCX_Voltage_Control_T *Voltage_Control_PrevZCX =
    Voltage_Control_M->prevZCSigState;

  /* Registration code */

  /* states (dwork) */
  (void) memset((void *)Voltage_Control_DW, 0,
                sizeof(DW_Voltage_Control_T));

  /* Start for InitialCondition: '<S3>/IC' */
  Voltage_Control_DW->IC_FirstOutputTime = true;

  /* Start for InitialCondition: '<S3>/IC1' */
  Voltage_Control_DW->IC1_FirstOutputTime = true;
  Voltage_Control_PrevZCX->EnabledDelay_Reset_ZCE = UNINITIALIZED_ZCSIG;

  /* InitializeConditions for Delay: '<S62>/Enabled Delay' */
  Voltage_Control_DW->EnabledDelay_DSTATE = 0.0F;

  /* InitializeConditions for DiscreteIntegrator: '<S43>/Integrator' */
  Voltage_Control_DW->Integrator_DSTATE = 0.0F;
  Voltage_Control_DW->Integrator_PrevResetState = 0;
}

/* Model terminate function */
void Voltage_Control_terminate(RT_MODEL_Voltage_Control_T *const
  Voltage_Control_M)
{
  /* (no terminate code required) */
  UNUSED_PARAMETER(Voltage_Control_M);
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
