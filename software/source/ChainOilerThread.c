/**
 * @file ChainOilerThread.c
 * @brief Chain oiler module implementation.
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ChainOilerThread.h"

#include "Dashboard.h"
#include "Logger.h"
#include "OilPump.h"
#include "Sdcard.h"

#include "ch.h"
#include "chprintf.h"
#include "hal.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define DEFAULT_SLEEP_DURATION_IN_MS (5 * 1000)

#define SPEED_MIN (double)(10)
#define DELAY_MIN (double)(120)

#define SPEED_MAX (double)(100)
#define DELAY_MAX (double)(60)

#define LINEAR_SLOPE ((DELAY_MAX - DELAY_MIN) / (SPEED_MAX - SPEED_MIN))
#define LINEAR_OFFSET (DELAY_MIN - (LINEAR_SLOPE * SPEED_MIN))

#define ARRAY_LENGTH(array) (sizeof((array))/(sizeof((array)[0])))

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
/**
 * @brief State machine of the chain oiler.
 */
typedef enum {
  COT_STATE_INIT,
  COT_STATE_DISABLED,
  COT_STATE_ENABLED,
  COT_STATE_FORCED,
} COT_State_t;

/**
 * @brief Input commands of the chain oiler.
 */
typedef enum {
  COT_CMD_START,
  COT_CMD_FORCE_START,
  COT_CMD_STOP,
  COT_CMD_FORCE_STOP,
  COT_CMD_FIRE,
  COT_CMD_ONE_SHOT,
} COT_Command_t;

/**
 * @brief Chain oiler error codes.
 */
typedef enum {
  COT_ERR_NO_ERROR,
} COT_Error_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
/**
 * @brief Virtual timer to measure time for releasing oil drops.
 */
static virtual_timer_t COT_timer;

/**
 * @brief Command buffer for the mailbox.
 */
static msg_t COT_commands[10];

/**
 * @brief Mailbox to use for communication with the chain oiler thread.
 */
static mailbox_t COT_mailbox;

/**
 * @brief State machine of the chain oiler module.
 */
static COT_State_t COT_state = COT_STATE_INIT;

/**
 * @brief Error state.
 */
static COT_Error_t COT_error = COT_ERR_NO_ERROR;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void timerCallback(void *p)
{
  (void)p;

  chSysLockFromISR();
  chMBPostI(&COT_mailbox, COT_CMD_FIRE);
  chSysUnlockFromISR();
}

static double getSpeed(void)
{
  Position_t pos = {0};
  dbGetPosition(&pos);
  return pos.speed;
}

static uint32_t calculatePeriodInMs(double speed)
{
  double sec = 0;
  if (speed < SPEED_MIN)
    sec = 0;
  else if ((SPEED_MIN <= speed) && (speed < SPEED_MAX))
    sec = LINEAR_SLOPE * speed + LINEAR_OFFSET;
  else
    sec = DELAY_MAX;

  return (uint32_t)(sec * 1000.0);
}

static void logEvent(double speed, uint32_t sleep)
{
  char entry[100] = {0};
  chsnprintf(entry, sizeof(entry), "%.2f km/h %d sec", speed, sleep/1000);
  LOG_Write("chainoiler.log", entry);
}

static void handleFireCommand(void)
{
  double speed               = getSpeed();
  uint32_t sleepDurationInMs = calculatePeriodInMs(speed);
  bool dropIsNeeded          = true;

  if (0 == sleepDurationInMs) {
    dropIsNeeded      = false;
    sleepDurationInMs = DEFAULT_SLEEP_DURATION_IN_MS;
  }

  chVTSet(&COT_timer, chTimeMS2I(sleepDurationInMs), timerCallback, NULL);

  logEvent(speed, sleepDurationInMs);

  if (dropIsNeeded)
    OLP_ReleaseOneDrop();
}

static void stopTimer(void)
{
  chSysLock();
  chVTResetI(&COT_timer);
  chSysUnlock();
}

static COT_State_t initStateHandler(COT_Command_t cmd)
{
  COT_State_t newState = COT_STATE_INIT;

  switch (cmd) {
  case COT_CMD_START: {
    chSysLock();
    chMBPostI(&COT_mailbox, COT_CMD_FIRE);
    chSysUnlock();
    newState = COT_STATE_ENABLED;
    break;
  }
  case COT_CMD_STOP: {
    newState = COT_STATE_DISABLED;
    break;
  }
  case COT_CMD_FORCE_START:
  case COT_CMD_FORCE_STOP:
  case COT_CMD_FIRE:
  case COT_CMD_ONE_SHOT:
    break;
  }

  return newState;
}

static COT_State_t disabledStateHandler(COT_Command_t cmd)
{
  COT_State_t newState = COT_STATE_DISABLED;

  switch (cmd) {
  case COT_CMD_START: {
    chSysLock();
    chMBPostI(&COT_mailbox, COT_CMD_FIRE);
    chSysUnlock();
    newState = COT_STATE_ENABLED;
    break;
  }
  case COT_CMD_FORCE_START:
  case COT_CMD_STOP:
  case COT_CMD_FORCE_STOP:
  case COT_CMD_FIRE:
  case COT_CMD_ONE_SHOT:
    break;
  }

  return newState;
}

static COT_State_t enabledStateHandler(COT_Command_t cmd)
{
  COT_State_t newState = COT_STATE_ENABLED;

  switch (cmd) {
  case COT_CMD_START: {
    break;
  }
  case COT_CMD_FORCE_START: {
    stopTimer();
    OLP_StartContinuous();
    newState = COT_STATE_FORCED;
    break;
  }
  case COT_CMD_STOP: {
    OLP_Stop();
    newState = COT_STATE_DISABLED;
    break;
  }
  case COT_CMD_FORCE_STOP: {
    break;
  }
  case COT_CMD_FIRE: {
    handleFireCommand();
    break;
  }
  case COT_CMD_ONE_SHOT: {
    OLP_ReleaseOneDrop();
    break;
  }
  default:
    break;
  }

  return newState;
}

static COT_State_t forcedStateHandler(COT_Command_t cmd)
{
  COT_State_t newState = COT_STATE_FORCED;

  switch (cmd) {
  case COT_CMD_START: {
    break;
  }
  case COT_CMD_FORCE_START: {
    break;
  }
  case COT_CMD_STOP: {
    OLP_Stop();
    newState = COT_STATE_DISABLED;
    break;
  }
  case COT_CMD_FORCE_STOP: {
    OLP_Stop();
    chSysLock();
    chMBPostI(&COT_mailbox, COT_CMD_FIRE);
    chSysUnlock();
    newState = COT_STATE_ENABLED;
    break;
  }
  case COT_CMD_FIRE:
  case COT_CMD_ONE_SHOT:
  default:
    break;
  }

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(COT_Thread, arg)
{
  (void)arg;
  chRegSetThreadName(CHAIN_OILER_THREAD_NAME);

  COT_state = COT_STATE_INIT;

  while (true) {
    COT_Command_t cmd;
    if (MSG_OK == chMBFetchTimeout(&COT_mailbox, (msg_t *)&cmd, TIME_INFINITE)) {
      switch (COT_state) {
      case COT_STATE_INIT: {
        COT_state = initStateHandler(cmd);
        break;
      }
      case COT_STATE_DISABLED: {
        COT_state = disabledStateHandler(cmd);
        break;
      }
      case COT_STATE_ENABLED: {
        COT_state = enabledStateHandler(cmd);
        break;
      }
      case COT_STATE_FORCED: {
        COT_state = forcedStateHandler(cmd);
        break;
      }
      default: {
        break;
      }
      }
    }
  }
}

void COT_Init(void)
{
  chVTObjectInit(&COT_timer);
  memset(&COT_commands, 0, sizeof(COT_commands));
  chMBObjectInit(&COT_mailbox, COT_commands, ARRAY_LENGTH(COT_commands));
}

void COT_StartI(void)
{
  chMBPostI(&COT_mailbox, COT_CMD_START);
}

void COT_Start(void)
{
  chSysLock();
  COT_StartI();
  chSysUnlock();
}

void COT_StopI(void)
{
  chMBPostI(&COT_mailbox, COT_CMD_STOP);
}

void COT_Stop(void)
{
  chSysLock();
  COT_StopI();
  chSysUnlock();
}

void COT_ForceStartI(void)
{
  chMBPostI(&COT_mailbox, COT_CMD_FORCE_START);
}

void COT_ForceStart(void)
{
  chSysLock();
  COT_ForceStartI();
  chSysUnlock();
}

void COT_ForceStopI(void)
{
  chMBPostI(&COT_mailbox, COT_CMD_FORCE_STOP);
}

void COT_ForceStop(void)
{
  chSysLock();
  COT_ForceStopI();
  chSysUnlock();
}

void COT_OneShotI(void)
{
  chMBPostI(&COT_mailbox, COT_CMD_ONE_SHOT);
}

void COT_OneShot(void)
{
  chSysLock();
  COT_OneShotI();
  chSysUnlock();
}

const char *COT_GetStateString(void)
{
  const char *stateString[] = {
      [COT_STATE_INIT]     = "INIT",
      [COT_STATE_DISABLED] = "DISABLED",
      [COT_STATE_ENABLED]  = "ENABLED",
      [COT_STATE_FORCED]   = "FORCED",
  };

  return stateString[(size_t)COT_state];
}

const char *COT_GetErrorString(void)
{
  const char *errorString[] = {
      [COT_ERR_NO_ERROR] = "NO_ERROR",
  };

  return errorString[(size_t)COT_error];
}

/****************************** END OF FILE **********************************/
