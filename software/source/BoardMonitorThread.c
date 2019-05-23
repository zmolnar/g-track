/**
 * @file BoardMonitorThread.c
 * @brief Thread to handle board events.
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "BoardMonitorThread.h"

#include "PeripheralManagerThread.h"
#include "SystemThread.h"
#include "hal.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/
#define DEBOUNCE_COUNTER_START 10
#define POLLING_DELAY          10

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static virtual_timer_t timer;
static semaphore_t sync;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static bool isSdcardInserted(void) {
  return PAL_HIGH == palReadLine(LINE_SDC_CARD_DETECT) ? true : false;
}

static void checkSdcard(void) {
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

static bool isUsbConnected(void) {
  return PAL_HIGH == palReadLine(LINE_USB_VBUS_SENSE) ? true : false;
}

static void checkUsb(void) {
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

static void timerCallback(void *p) {
  (void)p;
  chSysLockFromISR();
  chSemSignalI(&sync);
  chVTSetI(&timer, TIME_MS2I(POLLING_DELAY), timerCallback, NULL);
  chSysUnlockFromISR();
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(BoardMonitorThread, arg) {
  (void)arg;
  chRegSetThreadName("board");

  chSemObjectInit(&sync, 0);
  chVTObjectInit(&timer);

  chSysLock();
  chVTSetI(&timer, TIME_MS2I(POLLING_DELAY), timerCallback, NULL);
  chSysUnlock();

  while (true) {
    chSemWait(&sync);

    checkSdcard();
    checkUsb();
  }
}

void BoardMonitorThreadInit(void) {
  
}

/******************************* END OF FILE ***********************************/
