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

#include "BoardMonitorThread.h"
#include "SystemThread.h"
#include "PeripheralManagerThread.h"
#include "GpsReaderThread.h"
#include "ChainOilerThread.h"

static THD_WORKING_AREA(waSystemThread, 8192);
static THD_WORKING_AREA(waBoardMonitorThread, 8192);
static THD_WORKING_AREA(waPeripheralManagerThread, 8192);
static THD_WORKING_AREA(waGpsReaderThread, 8192);
static THD_WORKING_AREA(waChainOilerThread, 8192);

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waHeartBeatThread, 128);
static THD_FUNCTION(HeartBeatThread, arg) {

  (void)arg;
  chRegSetThreadName("heartbeat");

  palSetLine(LINE_LED_3_GREEN);

  while (true) {
    GpsLockState_t state = GPS_GetLockState();
    switch (state)
    {
    case GPS_NOT_POWERED: {
      palClearLine(LINE_LED_3_GREEN);
      chThdSleepMilliseconds(250);
      palSetLine(LINE_LED_3_GREEN);
      chThdSleepMilliseconds(1750);
      break;
    }
    case GPS_SEARCHING: {
      palClearLine(LINE_LED_3_GREEN);
      chThdSleepMilliseconds(250);
      palSetLine(LINE_LED_3_GREEN);
      chThdSleepMilliseconds(250);
      break;
    }
    case GPS_LOCKED: {
      palClearLine(LINE_LED_3_GREEN);
      chThdSleepMilliseconds(100);
      palSetLine(LINE_LED_3_GREEN);
      chThdSleepMilliseconds(100);
      palClearLine(LINE_LED_3_GREEN);
      chThdSleepMilliseconds(100);
      palSetLine(LINE_LED_3_GREEN);
      chThdSleepMilliseconds(1700);
      break;
    }
    default: {
      chThdSleepSeconds(1);
      break;
    }
    }
  }
}

/*
 * Application entry point.
 */
int main(void) {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  SYS_Init();
  BMT_Init();
  PRP_Init();
  GPS_Init();
  COT_Init();  

  chThdCreateStatic(waHeartBeatThread,
                    sizeof(waHeartBeatThread),
                    NORMALPRIO,
                    HeartBeatThread,
                    NULL);

  chThdCreateStatic(waSystemThread,
                    sizeof(waSystemThread),
                    NORMALPRIO,
                    SYS_Thread,
                    NULL);

  chThdCreateStatic(waBoardMonitorThread,
                    sizeof(waBoardMonitorThread),
                    NORMALPRIO,
                    BMT_Thread,
                    NULL);  

  chThdCreateStatic(waPeripheralManagerThread,
                    sizeof(waPeripheralManagerThread),
                    NORMALPRIO,
                    PeripheralManagerThread,
                    NULL);   

  chThdCreateStatic(waGpsReaderThread,
                    sizeof(waGpsReaderThread),
                    NORMALPRIO,
                    GPS_Thread,
                    NULL);       

  chThdCreateStatic(waChainOilerThread,
                    sizeof(waChainOilerThread),
                    NORMALPRIO,
                    COT_Thread,
                    NULL);     

  while (true) {
    chThdSleepMilliseconds(1000);
    // TODO: update watchdog here.
  }
}
