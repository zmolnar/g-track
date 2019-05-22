/**
 * @file ChainOilerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ChainOilerThread.h"
#include "ch.h"
#include "hal.h"
#include "Dashboard.h"
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define STARTUP_DELAY_IN_SEC         120
#define DEFAULT_SLEEP_DURATION_IN_MS (5*1000)

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  CHAIN_OILER_DISABLED,
  CHAIN_OILER_ENABLED,
  CHAIN_OILER_ERROR
} ChainOilerState_t;

typedef enum {
  CHAIN_OILER_START,
  CHAIN_OILER_STOP,
  CHAIN_OILER_FIRE
} ChainOilerCommand_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static virtual_timer_t timer;
static msg_t commands[10];
static mailbox_t mailbox;
static ChainOilerState_t state;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void timerCallback(void *p) {
  (void)p;
  chSysLockFromISR();
  chMBPostI(&mailbox, CHAIN_OILER_FIRE);
  chSysUnlockFromISR();
}

static double getSpeed(void) {
  dbLock(&dashboard);
  Position_t *p = dbGetPosition(&dashboard);
  double speed = p->speed;
  dbUnlock(&dashboard);
  return speed;
}

static uint32_t calculatePeriodInMs(double speed) {
  double sec = 0;
  if (speed < 10.0)
    sec = 0;
  else if ((10.0 <= speed) && (speed < 100.0))
    sec = (60/9) - speed * 2/3;
  else
    sec = 60;

  return (uint32_t)(sec * 1000.0);
}

static void releaseOilDrop(void) {
  // Leave sleep mode
  palSetLine(LINE_DCM_SLEEP);
  chThdSleepMilliseconds(2);

  // Set mode configuration
  palSetLine(LINE_DCM_AIN1);
  palClearLine(LINE_DCM_AIN2);

  // Let the pump work
  chThdSleepMilliseconds(1000);

  // Stop the motor
  palClearLine(LINE_DCM_AIN1);

  // Enter sleep mode
  chThdSleepMilliseconds(2);
  palClearLine(LINE_DCM_SLEEP);
}

static void handleFireCommand(void) {
    double speed = getSpeed();
    uint32_t sleepDurationInMs = calculatePeriodInMs(speed);
    bool dropIsNeeded = true;
    
    if (0 == sleepDurationInMs) {
      dropIsNeeded = false;
      sleepDurationInMs = DEFAULT_SLEEP_DURATION_IN_MS;
    }

    chSysLock();
    chVTSetI(&timer, chTimeMS2I(sleepDurationInMs), timerCallback, NULL);
    chSysUnlock();
    
    if (dropIsNeeded)
      releaseOilDrop();
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(ChainOilerThread, arg) {
  (void)arg;
  chRegSetThreadName("chain-oiler");

  state = CHAIN_OILER_DISABLED;

  while(true) {
    ChainOilerCommand_t cmd;
    if (MSG_OK == chMBFetchTimeout(&mailbox, (msg_t*)&cmd, TIME_INFINITE)) {
      switch (cmd) {
        case CHAIN_OILER_START: {
          if (CHAIN_OILER_ENABLED != state) {
            state = CHAIN_OILER_ENABLED;
            chSysLock();
            chVTSetI(&timer, chTimeS2I(STARTUP_DELAY_IN_SEC), timerCallback, NULL);
            chSysUnlock();
          }
          break;
        }
        case CHAIN_OILER_STOP: {
          if (CHAIN_OILER_DISABLED != state) {
            state = CHAIN_OILER_DISABLED;
            chSysLock();
            chVTResetI(&timer);
            chSysUnlock();
          }
          break;
        }
        case CHAIN_OILER_FIRE: {
          if (CHAIN_OILER_ENABLED == state) {
            handleFireCommand();
          }
          break;
        }
        default: {
          state = CHAIN_OILER_ERROR;
        }
      }
    }
  }
} 

void ChainOilerThreadInit(void) {
  chVTObjectInit(&timer);
  memset(&commands, 0, sizeof(commands));
  chMBObjectInit(&mailbox, commands, sizeof(commands)/sizeof(commands[0]));
}

void ChainOilerStart(void) {
  chSysLock();
  chMBPostI(&mailbox, CHAIN_OILER_START);
  chSysUnlock();
}

void ChainOilerStop(void) {
  chSysLock();
  chMBPostI(&mailbox, CHAIN_OILER_STOP);
  chSysUnlock();
}

/****************************** END OF FILE **********************************/
