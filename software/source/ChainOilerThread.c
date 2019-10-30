/**
 * @file ChainOilerThread.c
 * @brief Chain oiler module implementation.
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ChainOilerThread.h"

#include "Averager.h"
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
#define COT_LOGFILE  "/chainoiler.log"

#define SPEED_MIN (double)(10)
#define PERIOD_MIN (double)(120)

#define SPEED_MAX (double)(100)
#define PERIOD_MAX (double)(60)

#define LINEAR_SLOPE ((PERIOD_MAX - PERIOD_MIN) / (SPEED_MAX - SPEED_MIN))
#define LINEAR_OFFSET (PERIOD_MIN - (LINEAR_SLOPE * SPEED_MIN))

#define ARRAY_LENGTH(array) (sizeof((array))/(sizeof((array)[0])))

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
/**
 * @brief State machine of the chain oiler.
 */
typedef enum {
  COT_STATE_INIT,     /**< First state after system start.*/
  COT_STATE_DISABLED, /**< Chain oiler is disabled.*/
  COT_STATE_ENABLED,  /**< Chain oiler is enabled to run.*/
  COT_STATE_FORCED,   /**< Oil is forced to flow continuously.*/
} COT_State_t;

/**
 * @brief Input commands of the chain oiler.
 */
typedef enum {
  COT_CMD_START,           /**< Enable chain oiler.*/
  COT_CMD_FORCE_START,     /**< Enter force mode.*/
  COT_CMD_STOP,            /**< Disable chain oiler.*/
  COT_CMD_FORCE_STOP,      /**< Exit force mode.*/
  COT_CMD_SPEED_AVAILABLE, /**< Speed is avaible to read.*/ 
  COT_CMD_SHOOT,           /**< Release one oil drop.*/
} COT_Command_t;

/**
 * @brief Chain oiler error codes.
 */
typedef enum {
  COT_ERR_NO_ERROR,        /**< No error.*/
} COT_Error_t;

/**
 * @brief Chain oiler struct.
 */
typedef struct ChainOiler_s {
    /**
     * @brief Command buffer for the mailbox.
     */
    msg_t commands[10];

    /**
     * @brief Mailbox to use for communication with the chain oiler thread.
     */
    mailbox_t mailbox;

    /**
     * @brief State machine of the chain oiler module.
     */
    COT_State_t state;

    /**
     * @brief Error state.
     */
    COT_Error_t error;

    /**
     * @brief Speed averager.
     */
    Averager_t speedAverager;

    struct Period_s {
      /**
       * @brief Virtual timer to measure time for releasing oil drops.
       */
      virtual_timer_t timer;
      
      /**
       * @brief Start time-stamp of period measurement.
       */
      systime_t start;

      /**
       * @brief Average speed of the last period.
       */
      double speed;

      /**
       * @brief Length of the last period in seconds.
       */
      time_secs_t length;
    } period;
} ChainOiler_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
/**
 * @brief Chain oiler module.
 */
static ChainOiler_t chainOiler;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static double COT_getSpeed(void)
{
  DSB_Position_t pos = {0};
  DSB_GetPosition(&pos);
  return pos.speed;
}

static time_msecs_t COT_calculatePeriodLength(double speed)
{
  double sec = 0;
  if (speed < SPEED_MIN)
    sec = 0.0;
  else if ((SPEED_MIN <= speed) && (speed < SPEED_MAX))
    sec = LINEAR_SLOPE * speed + LINEAR_OFFSET;
  else
    sec = PERIOD_MAX;

  time_secs_t msec = (sec * 1000.0);

  return (time_msecs_t)msec;
}

void COT_timerCallbackI(void *p)
{
  (void)p;
  chSysLockFromISR();
  chMBPostI(&chainOiler.mailbox, COT_CMD_SHOOT);
  chSysUnlockFromISR();
}

void COT_processSpeedAndReloadTimer(void)
{
  double speed = COT_getSpeed();
  AVG_Put(&chainOiler.speedAverager, speed);
  double avgSpeed = AVG_GetAverage(&chainOiler.speedAverager);

  time_msecs_t periodLength = COT_calculatePeriodLength(avgSpeed);

  chainOiler.period.speed = avgSpeed;
  chainOiler.period.length = periodLength / 1000;

  if (0 < periodLength) {
    sysinterval_t timeSinceStart = chVTTimeElapsedSinceX(chainOiler.period.start);
    time_msecs_t elapsedTime = TIME_I2MS(timeSinceStart);

    if (elapsedTime < periodLength) {
      time_msecs_t timeToWait = periodLength - elapsedTime;
      chVTReset(&chainOiler.period.timer);
      chVTSet(&chainOiler.period.timer,
              TIME_MS2I(timeToWait),
              COT_timerCallbackI,
              NULL);
    } else {
      chSysLock();
      chMBPostI(&chainOiler.mailbox, COT_CMD_SHOOT);
      chSysUnlock();
    }
  } else {
    AVG_Clear(&chainOiler.speedAverager);
    chVTReset(&chainOiler.period.timer);
  }
}

const char *COT_getStateString(COT_State_t state)
{
  const char *stateStrings[] = {
      [COT_STATE_INIT]     = "INIT",
      [COT_STATE_DISABLED] = "DISABLED",
      [COT_STATE_ENABLED]  = "ENABLED",
      [COT_STATE_FORCED]   = "FORCED",
  };

  return stateStrings[state];
}

static void COT_logStateChange(COT_State_t from, COT_State_t to)
{
  char entry[32] = {0};
  chsnprintf(entry, sizeof(entry), "%s -> %s",
             COT_getStateString(from), COT_getStateString(to));
  LOG_Write(COT_LOGFILE, entry);
}

static void COT_logPeriodData(void)
{
  char entry[100] = {0};
  chsnprintf(entry, sizeof(entry), "%.2f km/h %d sec", 
             chainOiler.period.speed, chainOiler.period.length);
  LOG_Write(COT_LOGFILE, entry);
}

static COT_State_t COT_initStateHandler(COT_Command_t cmd)
{
  COT_State_t newState = COT_STATE_INIT;

  switch (cmd) {
  case COT_CMD_START: {
    chainOiler.period.start = chVTGetSystemTimeX();
    newState = COT_STATE_ENABLED;
    break;
  }
  case COT_CMD_STOP: {
    newState = COT_STATE_DISABLED;
    break;
  }
  case COT_CMD_FORCE_START:
  case COT_CMD_FORCE_STOP:
  case COT_CMD_SPEED_AVAILABLE:
  case COT_CMD_SHOOT:
    break;
  }

  if (newState != COT_STATE_INIT)
    COT_logStateChange(COT_STATE_INIT, newState);

  return newState;
}

static COT_State_t COT_disabledStateHandler(COT_Command_t cmd)
{
  COT_State_t newState = COT_STATE_DISABLED;

  switch (cmd) {
  case COT_CMD_START: {
    chainOiler.period.start = chVTGetSystemTimeX();
    newState = COT_STATE_ENABLED;
    break;
  }
  case COT_CMD_FORCE_START:
  case COT_CMD_STOP:
  case COT_CMD_FORCE_STOP:
  case COT_CMD_SPEED_AVAILABLE:
  case COT_CMD_SHOOT:
    break;
  }

  if (newState != COT_STATE_DISABLED)
    COT_logStateChange(COT_STATE_DISABLED, newState);

  return newState;
}

static COT_State_t COT_enabledStateHandler(COT_Command_t cmd)
{
  COT_State_t newState = COT_STATE_ENABLED;

  switch (cmd) {
  case COT_CMD_FORCE_START: {
    chVTReset(&chainOiler.period.timer);
    OLP_Start();
    newState = COT_STATE_FORCED;
    break;
  }
  case COT_CMD_STOP: {
    chVTReset(&chainOiler.period.timer);
    newState = COT_STATE_DISABLED;
    break;
  }
  case COT_CMD_SPEED_AVAILABLE: {
    COT_processSpeedAndReloadTimer();
    break;
  }
  case COT_CMD_SHOOT: {
    double speed = COT_getSpeed();
    if (0 < speed) {
      COT_logPeriodData();
      chainOiler.period.start = chVTGetSystemTimeX();
      AVG_Clear(&chainOiler.speedAverager);
      OLP_ReleaseOneDrop();
    }
    break;
  }
  case COT_CMD_START:
  case COT_CMD_FORCE_STOP:
  default:
    break;
  }

  if (newState != COT_STATE_ENABLED)
    COT_logStateChange(COT_STATE_ENABLED, newState);

  return newState;
}

static COT_State_t COT_forcedStateHandler(COT_Command_t cmd)
{
  COT_State_t newState = COT_STATE_FORCED;

  switch (cmd) {
  case COT_CMD_STOP: {
    OLP_Stop();
    newState = COT_STATE_DISABLED;
    break;
  }
  case COT_CMD_FORCE_STOP: {
    OLP_Stop();
    newState = COT_STATE_ENABLED;
    break;
  }
  case COT_CMD_START:
  case COT_CMD_FORCE_START:
  case COT_CMD_SPEED_AVAILABLE:
  case COT_CMD_SHOOT:
  default:
    break;
  }

  if (newState != COT_STATE_FORCED)
    COT_logStateChange(COT_STATE_FORCED, newState);

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(COT_Thread, arg)
{
  (void)arg;
  chRegSetThreadName(CHAIN_OILER_THREAD_NAME);

  chainOiler.state = COT_STATE_INIT;

  chThdSleepSeconds(2);

  while (true) {
    COT_Command_t cmd;
    if (MSG_OK == chMBFetchTimeout(&chainOiler.mailbox, (msg_t *)&cmd, TIME_INFINITE)) {
      switch (chainOiler.state) {
      case COT_STATE_INIT: {
        chainOiler.state = COT_initStateHandler(cmd);
        break;
      }
      case COT_STATE_DISABLED: {
        chainOiler.state = COT_disabledStateHandler(cmd);
        break;
      }
      case COT_STATE_ENABLED: {
        chainOiler.state = COT_enabledStateHandler(cmd);
        break;
      }
      case COT_STATE_FORCED: {
        chainOiler.state = COT_forcedStateHandler(cmd);
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
  memset(&chainOiler.commands, 0, sizeof(chainOiler.commands));
  chMBObjectInit(&chainOiler.mailbox, chainOiler.commands, ARRAY_LENGTH(chainOiler.commands));
  chainOiler.state = COT_STATE_INIT;
  chainOiler.error = COT_ERR_NO_ERROR;
  AVG_Init(&chainOiler.speedAverager);
  chVTObjectInit(&chainOiler.period.timer);
  chainOiler.period.start = 0;
  chainOiler.period.speed = 0;
  chainOiler.period.length = 0;
}

void COT_StartI(void)
{
  chMBPostI(&chainOiler.mailbox, COT_CMD_START);
}

void COT_Start(void)
{
  chSysLock();
  COT_StartI();
  chSysUnlock();
}

void COT_StopI(void)
{
  chMBPostI(&chainOiler.mailbox, COT_CMD_STOP);
}

void COT_Stop(void)
{
  chSysLock();
  COT_StopI();
  chSysUnlock();
}

void COT_ForceStartI(void)
{
  chMBPostI(&chainOiler.mailbox, COT_CMD_FORCE_START);
}

void COT_SpeedAvailableI(void)
{
  chMBPostI(&chainOiler.mailbox, COT_CMD_SPEED_AVAILABLE);
}

void COT_SpeedAvailable(void)
{
  chSysLock();
  COT_SpeedAvailableI();
  chSysUnlock();
}

void COT_ForceStart(void)
{
  chSysLock();
  COT_ForceStartI();
  chSysUnlock();
}

void COT_ForceStopI(void)
{
  chMBPostI(&chainOiler.mailbox, COT_CMD_FORCE_STOP);
}

void COT_ForceStop(void)
{
  chSysLock();
  COT_ForceStopI();
  chSysUnlock();
}

void COT_OneShotI(void)
{
  chMBPostI(&chainOiler.mailbox, COT_CMD_SHOOT);
}

void COT_OneShot(void)
{
  chSysLock();
  COT_OneShotI();
  chSysUnlock();
}

const char *COT_GetStateString(void)
{
  return COT_getStateString(chainOiler.state);
}

const char *COT_GetErrorString(void)
{
  const char *errorString[] = {
      [COT_ERR_NO_ERROR] = "NO_ERROR",
  };

  return errorString[(size_t)chainOiler.error];
}

/****************************** END OF FILE **********************************/
