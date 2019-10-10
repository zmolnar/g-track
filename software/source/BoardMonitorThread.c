/**
 * @file BoardMonitorThread.c
 * @brief Thread to handle board events.
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "BoardMonitorThread.h"

#include "ChainOilerThread.h"
#include "PeripheralManagerThread.h"
#include "SystemThread.h"
#include "hal.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define DEBOUNCE_COUNTER_START 10
#define POLLING_DELAY 10
#define SW1_CYCLE_IN_MS 1000

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static virtual_timer_t timer;
static virtual_timer_t Sw1Timer;
static semaphore_t sync;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static bool isSdcardInserted(void)
{
  return PAL_HIGH == palReadLine(LINE_SDC_CARD_DETECT);
}

static void checkSdcard(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (isSdcardInserted()) {
      if (--counter == 0) {
        PeripheralManagerSdcInserted();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!isSdcardInserted()) {
      counter = DEBOUNCE_COUNTER_START;
      PeripheralManagerSdcRemoved();
    }
  }
}

static bool isUsbConnected(void)
{
  return PAL_HIGH == palReadLine(LINE_USB_VBUS_SENSE);
}

static void checkUsb(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (isUsbConnected()) {
      if (--counter == 0) {
        PeripheralManagerUsbConnected();
        SystemThreadIgnitionOn();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!isUsbConnected()) {
      counter = DEBOUNCE_COUNTER_START;
      PeripheralManagerUsbDisconnected();
      SystemThreadIgnitionOff();
    }
  }
}

static bool isBT0Pressed(void)
{
  return PAL_LOW == palReadLine(LINE_BT0);
}

static void checkBT0(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (isBT0Pressed()) {
      if (--counter == 0) {
        PeripheralManagerSdcRemoved();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!isBT0Pressed()) {
      counter = DEBOUNCE_COUNTER_START;
    }
  }
}

static bool isIgnitionPressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_IGNITION);
}

static void checkIgnition(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (isIgnitionPressed()) {
      if (--counter == 0) {
        SystemThreadIgnitionOn();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!isIgnitionPressed()) {
      counter = DEBOUNCE_COUNTER_START;
      SystemThreadIgnitionOff();
    }
  }
}

static void ExtSW1TimerCallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  COT_ForceStartI();
  chSysUnlockFromISR();
}

static bool isExtSW1Pressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_SW1);
}

static void checkExtSW1(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;
  static systime_t start;

  if (counter > 0) {
    if (isExtSW1Pressed()) {
      if (--counter == 0) {
        start = chVTGetSystemTime();
        chVTSet(
            &Sw1Timer, TIME_MS2I(SW1_CYCLE_IN_MS), ExtSW1TimerCallback, NULL);
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!isExtSW1Pressed()) {
      counter = DEBOUNCE_COUNTER_START;
      if (chVTIsSystemTimeWithinX(start, start + chTimeMS2I(SW1_CYCLE_IN_MS))) {
        chVTReset(&Sw1Timer);
        COT_OneShot();
      } else {
        COT_ForceStop();
      }
    }
  }
}

static bool isExtSW2Pressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_SW2);
}

static void checkExtSW2(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (isExtSW2Pressed()) {
      if (--counter == 0) {
        ;
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!isExtSW2Pressed()) {
      counter = DEBOUNCE_COUNTER_START;
    }
  }
}

static void timerCallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  chSemSignalI(&sync);
  chVTSetI(&timer, TIME_MS2I(POLLING_DELAY), timerCallback, NULL);
  chSysUnlockFromISR();
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(BMT_Thread, arg)
{
  (void)arg;
  chRegSetThreadName(BOARD_MONITOR_THREAD_NAME);

  chVTSet(&timer, TIME_MS2I(POLLING_DELAY), timerCallback, NULL);

  while (true) {
    chSemWait(&sync);

    checkIgnition();
    checkSdcard();
    checkUsb();
    checkBT0();
    checkExtSW1();
    checkExtSW2();
  }
}

void BMT_Init(void)
{
  chSemObjectInit(&sync, 0);
  chVTObjectInit(&timer);
  chVTObjectInit(&Sw1Timer);
}

/****************************** END OF FILE **********************************/
