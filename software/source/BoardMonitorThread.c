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
typedef struct {
  virtual_timer_t timer;
  virtual_timer_t sw1Timer;
  semaphore_t sync;
} BoardMonitor_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
BoardMonitor_t monitor = {0};

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static bool BMT_isSdcardInserted(void)
{
  return PAL_HIGH == palReadLine(LINE_SDC_CARD_DETECT);
}

static void BMT_checkSdcard(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (BMT_isSdcardInserted()) {
      if (--counter == 0) {
        PeripheralManagerSdcInserted();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!BMT_isSdcardInserted()) {
      counter = DEBOUNCE_COUNTER_START;
      PeripheralManagerSdcRemoved();
    }
  }
}

static bool BMT_isUsbConnected(void)
{
  return PAL_HIGH == palReadLine(LINE_USB_VBUS_SENSE);
}

static void BMT_checkUsb(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (BMT_isUsbConnected()) {
      if (--counter == 0) {
        PeripheralManagerUsbConnected();
        SYS_IgnitionOn();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!BMT_isUsbConnected()) {
      counter = DEBOUNCE_COUNTER_START;
      PeripheralManagerUsbDisconnected();
      SYS_IgnitionOff();
    }
  }
}

static bool BMT_isBT0Pressed(void)
{
  return PAL_LOW == palReadLine(LINE_BT0);
}

static void BMT_checkBT0(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (BMT_isBT0Pressed()) {
      if (--counter == 0) {
        PeripheralManagerSdcRemoved();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!BMT_isBT0Pressed()) {
      counter = DEBOUNCE_COUNTER_START;
    }
  }
}

static bool BMT_isIgnitionPressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_IGNITION);
}

static void BMT_checkIgnition(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (BMT_isIgnitionPressed()) {
      if (--counter == 0) {
        SYS_IgnitionOn();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!BMT_isIgnitionPressed()) {
      counter = DEBOUNCE_COUNTER_START;
      SYS_IgnitionOff();
    }
  }
}

static void BMT_sw1TimerCallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  COT_ForceStartI();
  chSysUnlockFromISR();
}

static bool BMT_isSw1Pressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_SW1);
}

static void BMT_checkSw1(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;
  static systime_t start;

  if (counter > 0) {
    if (BMT_isSw1Pressed()) {
      if (--counter == 0) {
        start = chVTGetSystemTime();
        chVTSet(
            &sw1Timer, TIME_MS2I(SW1_CYCLE_IN_MS), BMT_sw1TimerCallback, NULL);
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!BMT_isSw1Pressed()) {
      counter = DEBOUNCE_COUNTER_START;
      if (chVTIsSystemTimeWithinX(start, start + chTimeMS2I(SW1_CYCLE_IN_MS))) {
        chVTReset(&sw1Timer);
        COT_OneShot();
      } else {
        COT_ForceStop();
      }
    }
  }
}

static bool BMT_isSw2Pressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_SW2);
}

static void BMT_checkSw2(void)
{
  static uint8_t counter = DEBOUNCE_COUNTER_START;

  if (counter > 0) {
    if (BMT_isSw2Pressed()) {
      if (--counter == 0) {
        ;
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!BMT_isSw2Pressed()) {
      counter = DEBOUNCE_COUNTER_START;
    }
  }
}

static void BMT_timerCallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  chSemSignalI(&sync);
  chVTSetI(&timer, TIME_MS2I(POLLING_DELAY), BMT_timerCallback, NULL);
  chSysUnlockFromISR();
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(BMT_Thread, arg)
{
  (void)arg;
  chRegSetThreadName(BOARD_MONITOR_THREAD_NAME);

  chVTSet(&timer, TIME_MS2I(POLLING_DELAY), BMT_timerCallback, NULL);

  while (true) {
    chSemWait(&sync);

    BMT_checkIgnition();
    BMT_checkSdcard();
    BMT_checkUsb();
    BMT_checkBT0();
    BMT_checkSw1();
    BMT_checkSw2();
  }
}

void BMT_Init(void)
{
  chSemObjectInit(&monitor.sync, 0);
  chVTObjectInit(&monitor.timer);
  chVTObjectInit(&monitor.sw1Timer);
}

/****************************** END OF FILE **********************************/
