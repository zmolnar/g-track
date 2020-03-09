/**
 * @file GpsReaderThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ChainOilerThread.h"
#include "ConfigManagerThread.h"
#include "ReporterThread.h"
#include "SystemThread.h"
#include "Dashboard.h"
#include "GpsReaderThread.h"
#include "Logger.h"
#include "Sdcard.h"
#include "Sim8xx.h"
#include "SimHandlerThread.h"

#include "ch.h"
#include "chprintf.h"
#include "hal.h"

#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define GPS_UPDATE_PERIOD_IN_MS 5000
#define MAX_RTC_OFFSET_IN_SEC 5
#define GPS_LOGFILE "/gpsreader.log"

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  GPS_ERR_NO_ERROR,
  GPS_ERR_POWER_ON,
  GPS_ERR_POWER_OFF,
  GPS_ERR_UPDATE,
  GPS_ERR_IN_RESPONSE,
} GPS_Error_t;

typedef enum {
  GPS_CMD_START,
  GPS_CMD_STOP,
  GPS_CMD_UPDATE,
} GPS_Command_t;

typedef enum {
  GPS_INIT,
  GPS_ENABLED,
  GPS_DISABLED,
  GPS_ERROR,
} GPS_State_t;

typedef struct {
  virtual_timer_t timer;
  msg_t events[10];
  mailbox_t mailbox;
  const GpsConfig_t *config;
  GpsLockState_t lockState;
  GPS_State_t state;
  GPS_Error_t error;
} GPS_Reader_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
GPS_Reader_t gps;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS */
/*****************************************************************************/
static void GPS_savePositionInLogfile(GPS_Data_t *pdata)
{
  char entry[150] = {0};

  chsnprintf(entry,
             sizeof(entry),
             "%04d%02d%02d%02d%02d%02d%03d ",
             pdata->time.year,
             pdata->time.month,
             pdata->time.day,
             pdata->time.hour,
             pdata->time.min,
             pdata->time.sec,
             pdata->time.msec);

  size_t length = strlen(entry);
  chsnprintf(entry + length,
             sizeof(entry) - length,
             "%d %f %f %f %f %d %d %d",
             pdata->isLocked ? 1 : 0,
             pdata->latitude,
             pdata->longitude,
             pdata->altitude,
             pdata->speed,
             pdata->gpsSatInView,
             pdata->gnssSatInView,
             pdata->gnssSatInUse);
  LOG_Write(GPS_LOGFILE, entry);
}

static bool GPS_isClockUpdateNeeded(GPS_Time_t *gpsTime)
{
  GPS_Time_t rtcTime = {0};
  DSB_GetTime(&rtcTime);

  bool result = false;
  if (rtcTime.year != gpsTime->year)
    result = true;
  else if (rtcTime.month != gpsTime->month)
    result = true;
  else if (rtcTime.day != gpsTime->day)
    result = true;
  else if (rtcTime.hour != gpsTime->hour)
    result = true;
  else if (rtcTime.min != gpsTime->min)
    result = true;
  else {
    uint32_t diff = 0;
    if (rtcTime.sec < gpsTime->sec)
      diff = gpsTime->sec - rtcTime.sec;
    else
      diff = rtcTime.sec - gpsTime->sec;

    result = (MAX_RTC_OFFSET_IN_SEC < diff);
  }

  return result;
}

static void GPS_updateClockInDashboard(GPS_Time_t *gpstime)
{
  if (GPS_isClockUpdateNeeded(gpstime))
    DSB_SetTime(gpstime);
}

static void GPS_updatePosition(void)
{
  GPS_Data_t gpsdata = {0};
  bool succeeded     = SIM_GpsReadPosition(&SIM868, &gpsdata);

  if (succeeded) {
    gps.lockState = gpsdata.isLocked ? GPS_LOCKED : GPS_SEARCHING;
    gpsdata.utcOffset = gps.config->utcOffset;
    DSB_SetPosition(&gpsdata);
    GPS_updateClockInDashboard(&gpsdata.time);
    GPS_savePositionInLogfile(&gpsdata);
    COT_SpeedAvailable();
    if (gpsdata.isLocked)
      RPT_CreateRecord();
  } else {
    gps.error = GPS_ERR_UPDATE;
  }
}

static void GPS_timerCallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  chVTSetI(&gps.timer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), GPS_timerCallback, NULL);
  chMBPostI(&gps.mailbox, GPS_CMD_UPDATE);
  chSysUnlockFromISR();
}

static void GPS_startTimer(void)
{
  chVTSet(&gps.timer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), GPS_timerCallback, NULL);
}

static void GPS_stopTimer(void)
{
  chSysLock();
  chVTResetI(&gps.timer);
  chSysUnlock();
}

static const char *GPS_getStateString(GPS_State_t state)
{
  static const char *stateStrs[] = {
      [GPS_INIT]     = "INIT",
      [GPS_ENABLED]  = "ENABLED",
      [GPS_DISABLED] = "DISABLED",
      [GPS_ERROR]    = "ERROR",
  };

  return stateStrs[(size_t)state];
}

static void GPS_logStateChange(GPS_State_t from, GPS_State_t to)
{
  char entry[32] = {0};
  chsnprintf(entry, sizeof(entry), "%s -> %s", GPS_getStateString(from), GPS_getStateString(to));
  LOG_Write(GPS_LOGFILE, entry);
}

static GPS_State_t GPS_initStateHandler(GPS_Command_t cmd)
{
  GPS_State_t newState = GPS_INIT;

  switch (cmd) {
  case GPS_CMD_START: {
    if (SIM_GpsStart(&SIM868)) {
      GPS_startTimer();
      gps.lockState = GPS_SEARCHING;
      newState      = GPS_ENABLED;
    } else {
      newState  = GPS_ERROR;
      gps.error = GPS_ERR_POWER_ON;
    }
    break;
  }
  case GPS_CMD_STOP: {
    newState = GPS_DISABLED;
    break;
  }
  case GPS_CMD_UPDATE:
  default: {
    break;
  }
  }

  if (GPS_INIT != newState)
    GPS_logStateChange(GPS_INIT, newState);

  return newState;
}

static GPS_State_t GPS_enabledStateHandler(GPS_Command_t cmd)
{
  GPS_State_t newState = GPS_ENABLED;

  switch (cmd) {
  case GPS_CMD_STOP: {
    GPS_stopTimer();
    if (SIM_GpsStop(&SIM868)) {
      gps.lockState = GPS_NOT_POWERED;
      newState      = GPS_DISABLED;
    } else {
      newState  = GPS_ERROR;
      gps.error = GPS_ERR_POWER_OFF;
    }
    break;
  }
  case GPS_CMD_UPDATE: {
    GPS_updatePosition();
    if (GPS_ERR_NO_ERROR != gps.error) {
      GPS_stopTimer();
      newState = GPS_ERROR;
    }
    break;
  }
  case GPS_CMD_START:
  default: {
    break;
  }
  }

  if (GPS_ENABLED != newState)
    GPS_logStateChange(GPS_ENABLED, newState);

  return newState;
}

static GPS_State_t GPS_disabledStateHandler(GPS_Command_t cmd)
{
  GPS_State_t newState = GPS_DISABLED;

  switch (cmd) {
  case GPS_CMD_START: {
    if (SIM_GpsStart(&SIM868)) {
      GPS_startTimer();
      gps.lockState = GPS_SEARCHING;
      newState      = GPS_ENABLED;
    } else {
      newState  = GPS_ERROR;
      gps.error = GPS_ERR_POWER_ON;
    }
  }
  case GPS_CMD_STOP:
  case GPS_CMD_UPDATE:
  default: {
    break;
  }
  }

  if (GPS_DISABLED != newState)
    GPS_logStateChange(GPS_DISABLED, newState);

  return newState;
}

static GPS_State_t GPS_errorStateHandler(GPS_Command_t cmd)
{
  GPS_State_t newState = GPS_ERROR;

  switch (cmd) {
  case GPS_CMD_STOP: {
    newState      = GPS_INIT;
    gps.lockState = GPS_NOT_POWERED;
    gps.error     = GPS_ERR_NO_ERROR;
    break;
  }
  case GPS_CMD_START:
  case GPS_CMD_UPDATE:
  default: {
    break;
  }
  }

  if (GPS_ERROR != newState)
    GPS_logStateChange(GPS_ERROR, newState);

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(GPS_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("gps");

  SYS_WaitForSuccessfulInit();
  CFM_WaitForValidConfig();

  gps.config = CFM_GetGpsConfig();
  
  while (true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&gps.mailbox, &msg, TIME_INFINITE)) {
      GPS_Command_t cmd = (GPS_Command_t)msg;
      switch (gps.state) {
      case GPS_INIT: {
        gps.state = GPS_initStateHandler(cmd);
        break;
      }
      case GPS_ENABLED: {
        gps.state = GPS_enabledStateHandler(cmd);
        break;
      }
      case GPS_DISABLED: {
        gps.state = GPS_disabledStateHandler(cmd);
        break;
      }
      case GPS_ERROR: {
        gps.state = GPS_errorStateHandler(cmd);
        break;
      }
      default: {
        break;
      }
      }
    }
  }
}

void GPS_Init(void)
{
  chVTObjectInit(&gps.timer);
  memset(gps.events, 0, sizeof(gps.events));
  chMBObjectInit(&gps.mailbox, gps.events, sizeof(gps.events) / sizeof(gps.events[0]));
  gps.config    = NULL;
  gps.lockState = GPS_NOT_POWERED;
  gps.state     = GPS_INIT;
  gps.error     = GPS_ERR_NO_ERROR;
}

void GPS_Start(void)
{
  chSysLock();
  chMBPostI(&gps.mailbox, GPS_CMD_START);
  chSysUnlock();
}

void GPS_Stop(void)
{
  chSysLock();
  chMBPostI(&gps.mailbox, GPS_CMD_STOP);
  chSysUnlock();
}

GpsLockState_t GPS_GetLockState(void)
{
  return gps.lockState;
}

const char *GPS_GetStateString(void)
{
  return GPS_getStateString(gps.state);
}

const char *GPS_GetErrorString(void)
{
  static const char *errorStr[] = {
      [GPS_ERR_NO_ERROR]    = "NO_ERROR",
      [GPS_ERR_POWER_ON]    = "START FAIL",
      [GPS_ERR_POWER_OFF]   = "STOP FAIL",
      [GPS_ERR_UPDATE]      = "UPDATE FAIL",
      [GPS_ERR_IN_RESPONSE] = "INVALID RESPONSE",
  };

  return errorStr[(size_t)gps.error];
}

/****************************** END OF FILE **********************************/
