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
static virtual_timer_t monitorTimer;
static event_source_t monitorTimerEvent;

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
        chSysLockFromISR();
        chMBPostI(&periphMailbox, SDC_INSERTED);
        chSysUnlockFromISR();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!isSdcardInserted()) {
      counter = DEBOUNCE_COUNTER_START;
      chSysLockFromISR();
      chMBPostI(&periphMailbox, SDC_REMOVED);
      chSysUnlockFromISR();
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
        chSysLockFromISR();
        chMBPostI(&periphMailbox, USB_CONNECTED);
        chMBPostI(&systemMailbox, SYS_EVT_IGNITION_ON);
        chSysUnlockFromISR();
      }
    } else
      counter = DEBOUNCE_COUNTER_START;
  } else {
    if (!isUsbConnected()) {
      counter = DEBOUNCE_COUNTER_START;
      chSysLockFromISR();
      chMBPostI(&periphMailbox, USB_DISCONNECTED);
      chMBPostI(&systemMailbox, SYS_EVT_IGNITION_OFF);
      chSysUnlockFromISR();
    }
  }
}

static void monitorTimerCallback(void *p) {
  (void)p;
  chSysLockFromISR();
  chEvtBroadcastI(&monitorTimerEvent);
  chVTSetI(&monitorTimer, TIME_MS2I(POLLING_DELAY), monitorTimerCallback, NULL);
  chSysUnlockFromISR();
}

static void timerEventHandler(eventid_t id) {
  (void)id;
  checkSdcard();
  checkUsb();
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(BoardMonitorThread, arg) {
  (void)arg;
  chRegSetThreadName("board");

  static const evhandler_t eventHandlers[] = {
    timerEventHandler
  };

  event_listener_t timerEventListener;
  chEvtRegister(&monitorTimerEvent, &timerEventListener, 0);

  while (true) {
    chEvtDispatch(eventHandlers, chEvtWaitOne(ALL_EVENTS));
  }
}

void BoardMonitorThreadInit(void) {
  chEvtObjectInit(&monitorTimerEvent);
  chSysLock();
  chVTSetI(&monitorTimer, TIME_MS2I(POLLING_DELAY), monitorTimerCallback, NULL);
  chSysUnlock();
}

/******************************* END OF FILE ***********************************/
