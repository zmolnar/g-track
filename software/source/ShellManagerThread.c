/**
 * @file ShellManagerThread.c
 * @brief USB and Bluetooth debug shell handler thread.
 * @author Molnar Zoltan
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "SdcHandlerThread.h"
#include "shell.h"
#include "usbcfg.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/
#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static const ShellCommand commands[] = {
  {"tree", SdcCmdTree},
  {NULL, NULL}
};

static const ShellConfig usb_shell_cfg = {
  (BaseSequentialStream *)&SDU1,
  commands
};

static thread_t *shelltp = NULL;

event_source_t usb_plugged_in_event;
event_source_t usb_removed_event;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static void usb_plugged_in_handler(eventid_t id) {
  (void)id;

  sduStart(&SDU1, &serusbcfg);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(500);

  if (!shelltp) {
    shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                  "usbshell", NORMALPRIO + 1,
                                  shellThread, (void *)&usb_shell_cfg);
  }
}

static void usb_removed_handler(eventid_t id) {
  (void)id;

  if (shelltp && chThdTerminatedX(shelltp)) {
    chThdWait(shelltp);   
    shelltp = NULL;
  }
  
  usbDisconnectBus(serusbcfg.usbp);
  usbStop(serusbcfg.usbp);
  sduStop(&SDU1);
}

static void shell_terminated_handler(eventid_t id) {
  (void)id;

  if (chThdTerminatedX(shelltp)) {
    chThdWait(shelltp);   
    shelltp = NULL;
  }
}
/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(ShellManagerThread, arg) {

  (void)arg;
  chRegSetThreadName("shellmanager");

  static const evhandler_t handlers[] = {
    usb_plugged_in_handler,
    usb_removed_handler,
    shell_terminated_handler
  };

  event_listener_t usb_plugged_in_listener;
  event_listener_t usb_removed_listener;
  event_listener_t shell_terminated_listener;

  /*
   * Initializes a serial-over-USB CDC driver.
   */
  sduObjectInit(&SDU1);

  /*
   * Shell manager initialization.
   */
  shellInit(); 
  
  chEvtObjectInit(&usb_plugged_in_event);
  chEvtObjectInit(&usb_removed_event);
  
  chEvtRegister(&usb_plugged_in_event, &usb_plugged_in_listener, 0);
  chEvtRegister(&usb_removed_event, &usb_removed_listener, 1);
  chEvtRegister(&shell_terminated, &shell_terminated_listener, 2);

  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1000);
  
  while(true) {
    chEvtDispatch(handlers, chEvtWaitOne(ALL_EVENTS));
  }
}

/******************************* END OF FILE ***********************************/

