/**
 * @file OilPump.c
 * @brief Implementation of the oil pump module.
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "OilPump.h"

#include "ch.h"
#include "hal.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void leaveSleepMode(void)
{
  palSetLine(LINE_DCM_SLEEP);
  chThdSleepMilliseconds(2);
}

static void enterSleepMode(void)
{
  chThdSleepMilliseconds(2);
  palClearLine(LINE_DCM_SLEEP);
}

static void outputHighZ(void)
{
  palClearLine(LINE_DCM_AIN1);
  palClearLine(LINE_DCM_AIN2);
  palClearLine(LINE_EXT_LED);
}

static void motorForwardDirection(void)
{
  palSetLine(LINE_EXT_LED);
  palSetLine(LINE_DCM_AIN1);
  palClearLine(LINE_DCM_AIN2);
}

static void motorReverseDirection(void)
{
  palSetLine(LINE_EXT_LED);
  palClearLine(LINE_DCM_AIN1);
  palSetLine(LINE_DCM_AIN2);
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void OLP_StartContinuous(void)
{
  outputHighZ();
  leaveSleepMode();
  motorForwardDirection();
}

void OLP_Stop(void)
{
  outputHighZ();
  enterSleepMode();
}

void OLP_ReleaseOneDrop(void)
{
  outputHighZ();
  leaveSleepMode();
  motorForwardDirection();
  chThdSleepMilliseconds(335);
  outputHighZ();
  chThdSleepMilliseconds(500);
  motorReverseDirection();
  chThdSleepMilliseconds(110);
  outputHighZ();
  enterSleepMode();
}

/****************************** END OF FILE **********************************/
