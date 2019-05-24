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
  CHAIN_OILER_DISABLED_FORCED,
  CHAIN_OILER_ENABLED,
  CHAIN_OILER_ENABLED_FORCED,
  CHAIN_OILER_ERROR
} ChainOilerState_t;

typedef enum {
  CHAIN_OILER_START,
  CHAIN_OILER_FORCE_START,
  CHAIN_OILER_STOP,
  CHAIN_OILER_FORCE_STOP,
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

static uint32_t convertMillisecondToHour(uint32_t millisecond) {
  return millisecond / 3600000;
}

static uint32_t convertMillisecondToMinute(uint32_t millisecond) {
  return millisecond % 3600000 / 60000;
}

static uint32_t convertMillisecondToSecond(uint32_t millisecond) {
  return millisecond % 3600000 % 60000 / 1000;
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
  RTCDateTime dateTime = {0};
  dbGetTime(&dateTime);

  chsnprintf(entry, sizeof(entry), "%d-%d-%d %d:%d:%d %f km/h %d sec\n", 
             dateTime.year,
             dateTime.month, 
             dateTime.day,
             convertMillisecondToHour(dateTime.millisecond),
             convertMillisecondToMinute(dateTime.millisecond),
             convertMillisecondToSecond(dateTime.millisecond),
             speed, sleep);

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

static ChainOilerState_t chainOilerEnabledHandler(ChainOilerCommand_t cmd) {
  switch(cmd) {

  }
}

static ChainOilerState_t chainOilerEnabledForcedHandler(ChainOilerCommand_t cmd) {
  switch (cmd) {
    case 
  }
}

static ChainOilerState_t chainOilerDisabledHandler(ChainOilerCommand_t cmd) {
  switch(cmd) {

  }
}

static ChainOilerState_t chainOilerDisabledForcedHandler(ChainOilerCommand_t cmd) {
  switch (cmd) {

  }
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
      switch(state) {
        case CHAIN_OILER_ENABLED: {
          state = chainOilerEnabledHandler(cmd);
          break;
        }
        case CHAIN_OILER_ENABLED_FORCED: {
          state = chainOilerEnabledForcedHandler(cmd);
          break;
        }
        case CHAIN_OILER_DISABLED: {
          state = chainOilerDisabledHandler(cmd);
          break;
        }
        case CHAIN_OILER_DISABLED_FORCED: {
          state = chainOilerDisabledForcedHandler(cmd);
          break;
        }
        case CHAIN_OILER_ERROR:
        default: {
          state = CHAIN_OILER_ERROR;
          break;
        }
      }

#if 0      
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
#endif    
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
