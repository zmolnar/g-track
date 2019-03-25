/*
  ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "usbcfg.h"

#include "SdcHandlerThread.h"

static THD_WORKING_AREA(waSdcHandlerThread, 1024);

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static const ShellCommand commands[] = {
  {"tree", SdcCmdTree},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};

static thread_t *shelltp = NULL;

/*
 * Shell exit event.
 */
static void ShellHandler(eventid_t id) {

  (void)id;
  if (chThdTerminatedX(shelltp)) {
    chThdWait(shelltp);   
    shelltp = NULL;
  }
}

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waHeartBeatThread, 128);
static THD_FUNCTION(HeartBeatThread, arg) {

  (void)arg;
  chRegSetThreadName("heartbeat");
  while (true) {
    palClearLine(LINE_LED_3_GREEN);
    chThdSleepMilliseconds(500);
    palSetLine(LINE_LED_3_GREEN);
    chThdSleepMilliseconds(500);
  }
}

/*
 * Application entry point.
 */
int main(void) {
  static const evhandler_t evhndl[] = {
    ShellHandler
  };
  event_listener_t shell_listener;
  
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Initializes a serial-over-USB CDC driver.
   */
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);
  

  /*
   * Shell manager initialization.
   */
  shellInit();  
  chEvtRegister(&shell_terminated, &shell_listener, 0);

  /*
   * Creates the LED blinker hearbeat thread.
   */
  chThdCreateStatic(waHeartBeatThread,
                    sizeof(waHeartBeatThread),
                    NORMALPRIO,
                    HeartBeatThread,
                    NULL);
  chThdCreateStatic(waSdcHandlerThread,
                    sizeof(waSdcHandlerThread),
                    NORMALPRIO,
                    SdcHandlerThread,
                    NULL);
  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1000);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);
  
  while (true) {
    if (!shelltp) {
      shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                    "shell", NORMALPRIO + 1,
                                    shellThread, (void *)&shell_cfg1);
    }
    chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, TIME_MS2I(500)));
  }
}
