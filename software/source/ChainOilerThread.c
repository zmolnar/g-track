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
#include "chprintf.h"
#include "Dashboard.h"
#include "Sdcard.h"
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define DEFAULT_SLEEP_DURATION_IN_MS       (5*1000)

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  CHAIN_OILER_INIT,
  CHAIN_OILER_DISABLED,
  CHAIN_OILER_ENABLED,
  CHAIN_OILER_FORCED
} ChainOilerState_t;

typedef enum {
  CHAIN_OILER_START,
  CHAIN_OILER_FORCE_START,
  CHAIN_OILER_STOP,
  CHAIN_OILER_FORCE_STOP,
  CHAIN_OILER_FIRE
} ChainOilerCommand_t;

typedef enum {
  CHAIN_OILER_E_NO_ERROR
} ChainOilerError_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static virtual_timer_t timer;
static msg_t commands[10];
static mailbox_t mailbox;
ChainOilerState_t chainOilerState = CHAIN_OILER_INIT;
ChainOilerError_t chainOilerError = CHAIN_OILER_E_NO_ERROR;

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
  Position_t pos = {0};
  dbGetPosition(&pos);
  return pos.speed;
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

static void leaveSleepMode(void) {
  palSetLine(LINE_DCM_SLEEP);
  chThdSleepMilliseconds(2);
}

static void enterSleepMode(void) {
  chThdSleepMilliseconds(2);
  palClearLine(LINE_DCM_SLEEP);
}

static void outputHighZ(void) {
  palClearLine(LINE_DCM_AIN1);
  palClearLine(LINE_DCM_AIN2);
}

static void motorForwardDirection(void) {
  palSetLine(LINE_DCM_AIN1);
  palClearLine(LINE_DCM_AIN2);
}

static void releaseOilDrop(void) {
  outputHighZ();
  leaveSleepMode();
  motorForwardDirection();
  chThdSleepMilliseconds(1000);
  outputHighZ();
  enterSleepMode();
}

static void startOiler(void) {
  outputHighZ();
  leaveSleepMode();
  motorForwardDirection();
}

static void stopOiler(void) {
  outputHighZ();
  enterSleepMode();
}

static void addToLogfile(const char *data, size_t length) {
  FIL log;
  if (FR_OK == f_open(&log, "/chainoiler.log", FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&log, data, length, &bw);
    f_close(&log);
  }
}

static void logEvent(double speed, uint32_t sleep) { 
  char entry[100] = {0};
  size_t end = dbCreateTimestamp(entry, sizeof(entry));

  chsnprintf(entry + end, sizeof(entry) - end, "%.2f km/h %d sec\n", 
             speed, sleep/1000);
  addToLogfile(entry, strlen(entry));
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

  logEvent(speed, sleepDurationInMs);

  if (dropIsNeeded) 
    releaseOilDrop();
}

static void stopTimer(void) {
  chSysLock();
  chVTResetI(&timer);
  chSysUnlock();
}

static ChainOilerState_t chainOilerInitHandler(ChainOilerCommand_t cmd) {
  ChainOilerState_t newState = CHAIN_OILER_INIT;

  switch(cmd) {
  case CHAIN_OILER_START: {
    chSysLock();
    chMBPostI(&mailbox, CHAIN_OILER_FIRE);
    chSysUnlock();
    newState = CHAIN_OILER_ENABLED;
    break;
  }
  case CHAIN_OILER_STOP: {
    newState = CHAIN_OILER_DISABLED;
    break;
  }
  case CHAIN_OILER_FORCE_START:
  case CHAIN_OILER_FORCE_STOP:
  case CHAIN_OILER_FIRE:
    break;
  }

  return newState;
}

static ChainOilerState_t chainOilerDisabledHandler(ChainOilerCommand_t cmd) {
  ChainOilerState_t newState = CHAIN_OILER_DISABLED;

  switch(cmd) {
  case CHAIN_OILER_START: {
    chSysLock();
    chMBPostI(&mailbox, CHAIN_OILER_FIRE);
    chSysUnlock();
    newState = CHAIN_OILER_ENABLED;
    break;
  }
  case CHAIN_OILER_FORCE_START:
  case CHAIN_OILER_STOP:
  case CHAIN_OILER_FORCE_STOP:
  case CHAIN_OILER_FIRE:
    break;
  }

  return newState;
}

static ChainOilerState_t chainOilerEnabledHandler(ChainOilerCommand_t cmd) {
  ChainOilerState_t newState = CHAIN_OILER_ENABLED;

  switch(cmd) {
  case CHAIN_OILER_START: {
    break;
  }
  case CHAIN_OILER_FORCE_START: {
    stopTimer();
    startOiler();
    newState = CHAIN_OILER_FORCED;
    break;
  }
  case CHAIN_OILER_STOP: {
    stopTimer();
    newState = CHAIN_OILER_DISABLED;
    break;
  }
  case CHAIN_OILER_FORCE_STOP: {
    break;
  }
  case CHAIN_OILER_FIRE: {
    handleFireCommand();
    break;
  }
  default:
    break;
  }

  return newState;
}

static ChainOilerState_t chainOilerForcedHandler(ChainOilerCommand_t cmd) {
  ChainOilerState_t newState = CHAIN_OILER_FORCED;

  switch(cmd) {
  case CHAIN_OILER_START: {
    break;
  }
  case CHAIN_OILER_FORCE_START: {
    break;
  }
  case CHAIN_OILER_STOP: {
    stopOiler();
    newState = CHAIN_OILER_DISABLED;
    break;
  }
  case CHAIN_OILER_FORCE_STOP: {
    stopOiler();
    chSysLock();
    chMBPostI(&mailbox, CHAIN_OILER_FIRE);
    chSysUnlock();
    newState = CHAIN_OILER_ENABLED;
    break;
  }
  case CHAIN_OILER_FIRE: {
    break;
  }
  default:
    break;
  }

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(ChainOilerThread, arg) {
  (void)arg;
  chRegSetThreadName(CHAIN_OILER_THREAD_NAME);

  chainOilerState = CHAIN_OILER_INIT;

  while(true) {
    ChainOilerCommand_t cmd;
    if (MSG_OK == chMBFetchTimeout(&mailbox, (msg_t*)&cmd, TIME_INFINITE)) {
      switch(chainOilerState) {
        case CHAIN_OILER_INIT: {
          chainOilerState = chainOilerInitHandler(cmd);
          break;
        }
        case CHAIN_OILER_DISABLED: {
          chainOilerState = chainOilerDisabledHandler(cmd);
          break;
        }
        case CHAIN_OILER_ENABLED: {
          chainOilerState = chainOilerEnabledHandler(cmd);
          break;
        }
        case CHAIN_OILER_FORCED: {
          chainOilerState = chainOilerForcedHandler(cmd);
          break;
        }
        default: {
          break;
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

void ChainOilerForceStart(void) {
  chSysLock();
  chMBPostI(&mailbox, CHAIN_OILER_FORCE_START);
  chSysUnlock();
}

void ChainOilerForceStop(void) {
  chSysLock();
  chMBPostI(&mailbox, CHAIN_OILER_FORCE_STOP);
  chSysUnlock();
}

const char * ChainOilerGetStateString(void) {
  const char * stateStr[] = {
    [CHAIN_OILER_INIT]     = "INIT";
    [CHAIN_OILER_DISABLED] = "DISABLED",
    [CHAIN_OILER_ENABLED]  = "ENABLED",
    [CHAIN_OILER_FORCED]   = "FORCED"
  }

  return stateStr[(size_t)chainOilerState];
}

const char * ChainOilerGetErrorString(void) {
  const char * errorStr[] = {
    [CHAIN_OILER_E_NO_ERROR] = "NO ERROR"
  }

  return errorStrp[(size_t)chainOilerError];
}

/****************************** END OF FILE **********************************/
