/**
 * @file GpsReaderThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "GpsReaderThread.h"

#include "ChainOilerThread.h"

#include "Dashboard.h"
#include "Sdcard.h"
#include "Logger.h"
#include "at.h"
#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "sim8xx.h"

#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define GPS_UPDATE_PERIOD_IN_MS 5000
#define MAX_RTC_OFFSET_IN_SEC 5
#define MAX_TRIES 3

#define GPS_LOGFILE  "/gpsreader.log"

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
  Sim8xxCommand cmd;
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
static void GPS_hijackChar(char *c, char *tmp)
{
  *tmp = *c;
  *c   = '\0';
}

static void GPS_restoreChar(char *c, char *tmp)
{
  *c = *tmp;
}

static bool GPS_convertRawDateToDateTime(DSB_DateTime_t *dt, char *raw)
{
  if (18 != strlen(raw))
    return false;

  char tmp;
  GPS_hijackChar(raw + 4, &tmp);
  dt->year = atoi(raw);
  GPS_restoreChar(raw + 4, &tmp);

  GPS_hijackChar(raw + 6, &tmp);
  dt->month = atoi(raw + 4);
  GPS_restoreChar(raw + 6, &tmp);

  GPS_hijackChar(raw + 8, &tmp);
  dt->day = atoi(raw + 6);
  GPS_restoreChar(raw + 8, &tmp);

  GPS_hijackChar(raw + 10, &tmp);
  dt->hour = atoi(raw + 8);
  GPS_restoreChar(raw + 10, &tmp);

  GPS_hijackChar(raw + 12, &tmp);
  dt->min = atoi(raw + 10);
  GPS_restoreChar(raw + 12, &tmp);

  GPS_hijackChar(raw + 14, &tmp);
  dt->sec = atoi(raw + 12);
  GPS_restoreChar(raw + 14, &tmp);

  dt->msec = atoi(raw + 15);

  return true;
}

static void GPS_savePositionInLogfile(CGNSINF_Response_t *pdata)
{
  char entry[150] = {0};
  chsnprintf(entry, sizeof(entry),
             "%s %f %f %f %f %d %d %d %d",
             pdata->date,
             pdata->latitude,
             pdata->longitude,
             pdata->speed,
             pdata->altitude,
             pdata->fixStatus,
             pdata->gpsSatInView,
             pdata->gnssSatInView,
             pdata->gnssSatInUse);
  LOG_Write(GPS_LOGFILE, entry);
}

static void GPS_savePositionInDashboard(CGNSINF_Response_t *data)
{
  DSB_Position_t pos = {0};
  strncpy(pos.date, data->date, sizeof(pos.date));
  pos.latitude      = data->latitude;
  pos.longitude     = data->longitude;
  pos.altitude      = data->altitude;
  pos.speed         = data->speed;
  pos.gnssSatInUse  = data->gnssSatInUse;
  pos.gnssSatInView = data->gnssSatInView;
  pos.gpsSatInView  = data->gpsSatInView;
  DSB_SetPosition(&pos);

  COT_SpeedAvailable();
}

static bool GPS_isClockUpdateNeeded(DSB_DateTime_t *gpsTime)
{
  DSB_DateTime_t rtcTime = {0};
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

static void GPS_updateClockInDashboard(char *rawDateTime)
{
  DSB_DateTime_t gpsDateTime = {0};
  GPS_convertRawDateToDateTime(&gpsDateTime, rawDateTime);

  if (GPS_isClockUpdateNeeded(&gpsDateTime))
    DSB_SetTime(&gpsDateTime);
}

static void GPS_updatePosition(void)
{
  SIM_CommandInit(&gps.cmd);
  AT_CgnsinfCreate(gps.cmd.request, sizeof(gps.cmd.request));
  SIM_ExecuteCommand(&SIM8D1, &gps.cmd);

  if (SIM8XX_OK == gps.cmd.status) {
    CGNSINF_Response_t response;
    if (AT_CgnsinfParse(&response, gps.cmd.response)) {
      if (1 == response.fixStatus) {
        gps.lockState = GPS_LOCKED;
        gps.error = GPS_ERR_NO_ERROR;
        GPS_savePositionInDashboard(&response);
        GPS_updateClockInDashboard(response.date);
      } else {
        gps.lockState = GPS_SEARCHING;
      }
      GPS_savePositionInLogfile(&response);
    } else {
      gps.error = GPS_ERR_IN_RESPONSE;
    }
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

static bool GPS_modulePowerOn(void)
{
  bool success = false;
  uint32_t i   = 0;
  do {
    SIM_CommandInit(&gps.cmd);
    AT_CgnspwrCreateOn(gps.cmd.request, sizeof(gps.cmd.request));
    SIM_ExecuteCommand(&SIM8D1, &gps.cmd);
    if (SIM8XX_OK == gps.cmd.status) {
      success = true;
    } else {
      success = false;
      chThdSleepMilliseconds(1000);
    }
  } while ((!success) && (i++ < MAX_TRIES));

  return success;
}

static bool GPS_modulePowerOff(void)
{
  bool success = false;
  uint32_t i   = 0;
  do {
    SIM_CommandInit(&gps.cmd);
    AT_CgnspwrCreateOff(gps.cmd.request, sizeof(gps.cmd.request));
    SIM_ExecuteCommand(&SIM8D1, &gps.cmd);
    if (SIM8XX_OK == gps.cmd.status) {
      success = true;
    } else {
      success = false;
      chThdSleepMilliseconds(1000);
    }
  } while ((!success) && (i++ < MAX_TRIES));

  return success;
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
  chsnprintf(entry, sizeof(entry), "%s -> %s",
             GPS_getStateString(from), GPS_getStateString(to));
  LOG_Write(GPS_LOGFILE, entry);
}

static GPS_State_t GPS_initStateHandler(GPS_Command_t cmd)
{
  GPS_State_t newState = GPS_INIT;

  switch (cmd) {
  case GPS_CMD_START: {
    if (GPS_modulePowerOn()) {
      GPS_startTimer();
      gps.lockState = GPS_SEARCHING;
      newState = GPS_ENABLED;
    } else {
      newState = GPS_ERROR;
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
    if (GPS_modulePowerOff()) {
      gps.lockState = GPS_NOT_POWERED;
      newState = GPS_DISABLED;
    } else {
      newState = GPS_ERROR;
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
    if (GPS_modulePowerOn()) {
      GPS_startTimer();
      gps.lockState = GPS_SEARCHING;
      newState = GPS_ENABLED;
    } else {
      newState = GPS_ERROR;
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
    newState = GPS_INIT;
    gps.lockState = GPS_NOT_POWERED;
    gps.error = GPS_ERR_NO_ERROR;
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

  gps.state = GPS_INIT;

  while (true) {
    GPS_Command_t cmd;
    if (MSG_OK == chMBFetchTimeout(&gps.mailbox, (msg_t *)&cmd, TIME_INFINITE)) {
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
  chMBObjectInit(&gps.mailbox, 
                  gps.events, 
                  sizeof(gps.events) / sizeof(gps.events[0]));
  gps.lockState = GPS_NOT_POWERED;
  gps.state = GPS_INIT;
  gps.error = GPS_ERR_NO_ERROR;
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
