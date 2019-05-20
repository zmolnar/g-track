/**
 * @file ChainOilerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ChainOilerThread.h"
#include "ch.h"
#include "Dashboard.h"
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define STARTUP_DELAY_IN_SEC   120

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

static time_msecs_t calculatePeriodInMs(double speed) {
  // TODO: implement algorithm
  return 60*1000;
}

static void releaseOilDrop(void) {
  // TODO: drive DC motor
}

static void handleFireCommand(void) {
    double speed = getSpeed();
    time_msecs_t periodInMs = calculatePeriodInMs(speed);
    chSysLock();
    chVTSetI(&timer, chTimeMS2I(periodInMs), timerCallback, NULL);
    chSysUnlock();
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
