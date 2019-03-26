/**
 * @file BoardMonitorThread.c
 * @brief Thread to handle board events.
 * @author Molnar Zoltan
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "BoardMonitorThread.h"
#include "ShellManagerThread.h"
#include "SdcHandlerThread.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/
#define DEBOUNCE_COUNTER_START       10
#define POLLING_DELAY                10

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static virtual_timer_t monitor_timer;
static event_source_t timer_event;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static bool is_sdc_inserted(void) {
  return PAL_HIGH == palReadLine(LINE_SDC_CARD_DETECT) ? true : false;
}

static void check_sdcard(void) {
  static uint8_t counter = DEBOUNCE_COUNTER_START;
  
  if (counter > 0) {
    if (is_sdc_inserted()) {
      if (--counter == 0) {
        chSysLockFromISR();
        chEvtBroadcastI(&sdc_inserted_event);
        chSysUnlockFromISR();
      }
    }
    else
      counter = DEBOUNCE_COUNTER_START;
  }
  else {
    if (!is_sdc_inserted()) {
      counter = DEBOUNCE_COUNTER_START;
      chSysLockFromISR();
      chEvtBroadcastI(&sdc_removed_event);
      chSysUnlockFromISR();
    }
  }
}

static bool is_usb_plugged_in(void) {
  return PAL_HIGH == palReadLine(LINE_USB_VBUS_SENSE) ? true : false;
}

static void check_usb(void) {
  static uint8_t counter = DEBOUNCE_COUNTER_START;
  
  if (counter > 0) {
    if (is_usb_plugged_in()) {
      if (--counter == 0) {
        chSysLockFromISR();
        chEvtBroadcastI(&usb_plugged_in_event);
        chSysUnlockFromISR();
      }
    }
    else
      counter = DEBOUNCE_COUNTER_START;
  }
  else {
    if (!is_usb_plugged_in()) {
      counter = DEBOUNCE_COUNTER_START;
      chSysLockFromISR();
      chEvtBroadcastI(&usb_removed_event);
      chSysUnlockFromISR();
    }
  }
}

static void monitor_timer_callback(void *p) {
  (void)p;
  chSysLockFromISR();
  chEvtBroadcastI(&timer_event);
  chVTSetI(&monitor_timer, TIME_MS2I(POLLING_DELAY),
           monitor_timer_callback, NULL);
  chSysUnlockFromISR();
}

static void timer_handler(eventid_t id) {
  (void)id;
  check_sdcard();
  check_usb();
}

static void monitor_init(void) {
  chEvtObjectInit(&timer_event);
  chSysLock();
  chVTSetI(&monitor_timer, TIME_MS2I(POLLING_DELAY),
           monitor_timer_callback, NULL);
  chSysUnlock();
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(BoardMonitorThread, arg)
{
    (void)arg;
    chRegSetThreadName("boardmonitor");

    static const evhandler_t handlers[] = {
      timer_handler
    };

    monitor_init();
    
    event_listener_t timer_listener;
    chEvtRegister(&timer_event, &timer_listener, 0);
    
    while(true) {
      chEvtDispatch(handlers, chEvtWaitOne(ALL_EVENTS));
    }
}


/******************************* END OF FILE ***********************************/

