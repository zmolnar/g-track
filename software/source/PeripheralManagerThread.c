/**
 * @file PeripheralManagerThread.c
 * @brief
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "PeripheralManagerThread.h"
#include "BoardEvents.h"
#include "Sdcard.h"
#include "DebugShell.h"
#include "sim8xx.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static SerialConfig sd_config = {115200,0,0,0};
static Sim8xxConfig sim_config = {&SD1, &sd_config, LINE_WAVESHARE_POWER};

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static void sdcardInsertedHandler(eventid_t id) {
  (void)id;
  sdcardMount();
}

static void sdcardRemovedHandler(eventid_t id) {
  (void)id;
  sdcardUnmount();
}

static void usbConnectedHandler(eventid_t id) {
  (void)id;
  debugShellStart();
}

static void usbDisconnectedHandler(eventid_t id) {
  (void)id;
  debugShellStop();
}

static void debugShellTerminatedHandler(eventid_t id) {
  (void)id;
  debugShellTerminated();
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(PeripheralManagerThread, arg) {
  (void)arg;
  chRegSetThreadName("peripheral");

  static const evhandler_t eventHandlers[] = {
    sdcardInsertedHandler,
    sdcardRemovedHandler,
    usbConnectedHandler,
    usbDisconnectedHandler,
    debugShellTerminatedHandler
  };
  
  event_listener_t sdcardInsertedListener;
  event_listener_t sdcardRemovedListener;
  event_listener_t usbConnectedListener;
  event_listener_t usbDisconnectedListener;
  event_listener_t debugShellTerminatedListener;

  chEvtRegister(&besSdcardInserted, &sdcardInsertedListener, 0);
  chEvtRegister(&besSdcardRemoved, &sdcardRemovedListener, 1);
  chEvtRegister(&besUsbConnected, &usbConnectedListener, 2);
  chEvtRegister(&besUsbDisconnected, &usbDisconnectedListener, 3);
  chEvtRegister(&shell_terminated, &debugShellTerminatedListener, 4);

  sdcardInit();
  debugShellInit();

  while(true) {
    chEvtDispatch(eventHandlers, chEvtWaitOne(ALL_EVENTS));
  }
} 

void PeripheralManagerThreadInit(void) {
  sim8xxInit(&SIM8D1);
  sim8xxStart(&SIM8D1, &sim_config);
}

/******************************* END OF FILE ***********************************/
