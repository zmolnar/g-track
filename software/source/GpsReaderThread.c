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
#define MAX_TIME_DIFF_IN_SEC 5
#define MAX_TRIES 3

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  GPS_E_NO_ERROR,
  GPS_E_POWER_ON,
  GPS_E_POWER_OFF,
  GPS_E_UPDATE,
  GPS_E_IN_RESPONSE,
} GpsError_t;

typedef enum {
  GPS_CMD_START,
  GPS_CMD_STOP,
  GPS_CMD_UPDATE,
} GpsCommand_t;

typedef enum {
  GPS_INIT,
  GPS_ENABLED,
  GPS_DISABLED,
  GPS_ERROR,
} GpsReaderState_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static virtual_timer_t gpsTimer;
static Sim8xxCommand cmd;
static msg_t events[10];
static mailbox_t gpsMailbox;

GpsReaderState_t gpsState = GPS_INIT;
GpsError_t gpsError       = GPS_E_NO_ERROR;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS */
/*****************************************************************************/
static void hijackChar(char *c, char *tmp)
{
  *tmp = *c;
  *c   = '\0';
}

static void restoreChar(char *c, char *tmp)
{
  *c = *tmp;
}

static bool convertRawDateToDateTime(DateTime_t *dt, char *raw)
{
  if (18 != strlen(raw))
    return false;

  char tmp;
  hijackChar(raw + 4, &tmp);
  dt->year = atoi(raw);
  restoreChar(raw + 4, &tmp);

  hijackChar(raw + 6, &tmp);
  dt->month = atoi(raw + 4);
  restoreChar(raw + 6, &tmp);

  hijackChar(raw + 8, &tmp);
  dt->day = atoi(raw + 6);
  restoreChar(raw + 8, &tmp);

  hijackChar(raw + 10, &tmp);
  dt->hour = atoi(raw + 8);
  restoreChar(raw + 10, &tmp);

  hijackChar(raw + 12, &tmp);
  dt->min = atoi(raw + 10);
  restoreChar(raw + 12, &tmp);

  hijackChar(raw + 14, &tmp);
  dt->sec = atoi(raw + 12);
  restoreChar(raw + 14, &tmp);

  dt->msec = atoi(raw + 15);

  return true;
}

static void saveInLogfile(const char *data, size_t length)
{
  FIL log;

  sdcardLock();
  if (FR_OK == f_open(&log, "/sim8xx_gnss.log", FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&log, data, length, &bw);
    f_close(&log);
  }
  sdcardUnlock();
}

static void logPosition(CGNSINF_Response_t *pdata)
{
  char buf[150] = {0};
  size_t end    = dbCreateTimestamp(buf, sizeof(buf));
  chsnprintf(buf + end,
             sizeof(buf) - end,
             "%s %f %f %f %f %d %d %d %d\n",
             pdata->date,
             pdata->latitude,
             pdata->longitude,
             pdata->speed,
             pdata->altitude,
             pdata->fixStatus,
             pdata->gpsSatInView,
             pdata->gnssSatInView,
             pdata->gnssSatInUse);
  saveInLogfile(buf, strlen(buf));
}

static void savePosition(CGNSINF_Response_t *data)
{
  Position_t gpsPos = {0};
  strncpy(gpsPos.date, data->date, sizeof(gpsPos.date));
  gpsPos.latitude      = data->latitude;
  gpsPos.longitude     = data->longitude;
  gpsPos.altitude      = data->altitude;
  gpsPos.speed         = data->speed;
  gpsPos.gnssSatInUse  = data->gnssSatInUse;
  gpsPos.gnssSatInView = data->gnssSatInView;
  gpsPos.gpsSatInView  = data->gpsSatInView;
  dbSetPosition(&gpsPos);

#warning Fix it
  COT_SpeedAvailable();
}

static bool isUpdateNeeded(DateTime_t *gpsTime)
{
  DateTime_t rtcTime = {0};
  dbGetTime(&rtcTime);

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

    result = (MAX_TIME_DIFF_IN_SEC < diff);
  }

  return result;
}

static void updateClock(char *rawDateTime)
{
  DateTime_t gpsDateTime = {0};
  convertRawDateToDateTime(&gpsDateTime, rawDateTime);

  if (isUpdateNeeded(&gpsDateTime))
    dbSetTime(&gpsDateTime);
}

static bool updatePosition(GpsError_t *error)
{
  sim8xxCommandInit(&cmd);
  atCgnsinfCreate(cmd.request, sizeof(cmd.request));
  sim8xxExecute(&SIM8D1, &cmd);

  if (SIM8XX_OK == cmd.status) {
    CGNSINF_Response_t data;
    if (atCgnsinfParse(&data, cmd.response)) {
      *error = GPS_E_NO_ERROR;
      savePosition(&data);
      updateClock(data.date);
      logPosition(&data);
    } else {
      *error = GPS_E_IN_RESPONSE;
    }
  } else {
    *error = GPS_E_UPDATE;
  }

  return GPS_E_NO_ERROR == *error;
}

static void timerCallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  chVTSetI(&gpsTimer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), timerCallback, NULL);
  chMBPostI(&gpsMailbox, GPS_CMD_UPDATE);
  chSysUnlockFromISR();
}

static bool gpsPowerOn(void)
{
  bool success = false;
  uint32_t i   = 0;
  do {
    sim8xxCommandInit(&cmd);
    atCgnspwrCreateOn(cmd.request, sizeof(cmd.request));
    sim8xxExecute(&SIM8D1, &cmd);
    if (SIM8XX_OK == cmd.status) {
      success = true;
    } else {
      success = false;
      chThdSleepMilliseconds(1000);
    }
  } while ((!success) && (i++ < MAX_TRIES));

  return success;
}

static bool gpsPowerOff(void)
{
  bool success = false;
  uint32_t i   = 0;
  do {
    sim8xxCommandInit(&cmd);
    atCgnspwrCreateOff(cmd.request, sizeof(cmd.request));
    sim8xxExecute(&SIM8D1, &cmd);
    if (SIM8XX_OK == cmd.status) {
      success = true;
    } else {
      success = false;
      chThdSleepMilliseconds(1000);
    }
  } while ((!success) && (i++ < MAX_TRIES));

  return success;
}

static void startTimer(void)
{
  chVTSet(&gpsTimer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), timerCallback, NULL);
}

static void stopTimer(void)
{
  chSysLock();
  chVTResetI(&gpsTimer);
  chSysUnlock();
}

static GpsReaderState_t GpsReaderInitHandler(GpsCommand_t cmd)
{
  GpsReaderState_t newState = GPS_INIT;
  switch (cmd) {
  case GPS_CMD_START: {
    if (gpsPowerOn()) {
      startTimer();
      newState = GPS_ENABLED;
    } else {
      newState = GPS_ERROR;
      gpsError = GPS_E_POWER_ON;
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

  return newState;
}

static GpsReaderState_t GpsReaderEnabledHandler(GpsCommand_t cmd)
{
  GpsReaderState_t newState = GPS_ENABLED;
  switch (cmd) {
  case GPS_CMD_START: {
    break;
  }
  case GPS_CMD_STOP: {
    stopTimer();
    if (gpsPowerOff()) {
      newState = GPS_DISABLED;
    } else {
      newState = GPS_ERROR;
      gpsError = GPS_E_POWER_OFF;
    }
    break;
  }
  case GPS_CMD_UPDATE: {
    if (!updatePosition(&gpsError)) {
      stopTimer();
      newState = GPS_ERROR;
    }
    break;
  }
  default: {
    break;
  }
  }

  return newState;
}

static GpsReaderState_t GpsReaderDisabledHandler(GpsCommand_t cmd)
{
  GpsReaderState_t newState = GPS_DISABLED;
  switch (cmd) {
  case GPS_CMD_START: {
    if (gpsPowerOn()) {
      startTimer();
      newState = GPS_ENABLED;
    } else {
      newState = GPS_ERROR;
      gpsError = GPS_E_POWER_ON;
    }
  }
  case GPS_CMD_STOP:
  case GPS_CMD_UPDATE:
  default: {
    break;
  }
  }

  return newState;
}

static GpsReaderState_t GpsReaderErrorHandler(GpsCommand_t cmd)
{
  GpsReaderState_t newState = GPS_ERROR;
  switch (cmd) {
  case GPS_CMD_STOP: {
    newState = GPS_INIT;
    gpsError = GPS_E_NO_ERROR;
    break;
  }
  case GPS_CMD_START:
  case GPS_CMD_UPDATE:
  default: {
    break;
  }
  }

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(GpsReaderThread, arg)
{
  (void)arg;

  chRegSetThreadName("gps");

  gpsState = GPS_INIT;

  while (true) {
    GpsCommand_t ecmd;
    if (MSG_OK ==
        chMBFetchTimeout(&gpsMailbox, (msg_t *)&ecmd, TIME_INFINITE)) {
      switch (gpsState) {
      case GPS_INIT: {
        gpsState = GpsReaderInitHandler(ecmd);
        break;
      }
      case GPS_ENABLED: {
        gpsState = GpsReaderEnabledHandler(ecmd);
        break;
      }
      case GPS_DISABLED: {
        gpsState = GpsReaderDisabledHandler(ecmd);
        break;
      }
      case GPS_ERROR: {
        gpsState = GpsReaderErrorHandler(ecmd);
        break;
      }
      default: {
        break;
      }
      }
    }
  }
}

void GpsReaderThreadInit(void)
{
  chVTObjectInit(&gpsTimer);
  memset(events, 0, sizeof(events));
  chMBObjectInit(&gpsMailbox, events, sizeof(events) / sizeof(events[0]));
}

void GpsReaderStart(void)
{
  chSysLock();
  chMBPostI(&gpsMailbox, GPS_CMD_START);
  chSysUnlock();
}

void GpsReaderStop(void)
{
  chSysLock();
  chMBPostI(&gpsMailbox, GPS_CMD_STOP);
  chSysUnlock();
}

const char *GpsReaderGetStateString(void)
{
  static const char *stateStr[] = {
      [GPS_INIT]     = "INIT",
      [GPS_ENABLED]  = "ENABLED",
      [GPS_DISABLED] = "DISABLED",
      [GPS_ERROR]    = "ERROR",
  };

  return stateStr[(size_t)gpsState];
}

const char *GpsReaderGetErrorString(void)
{
  static const char *errorStr[] = {
      [GPS_E_NO_ERROR]    = "NO_ERROR",
      [GPS_E_POWER_ON]    = "START FAIL",
      [GPS_E_POWER_OFF]   = "STOP FAIL",
      [GPS_E_UPDATE]      = "UPDATE FAIL",
      [GPS_E_IN_RESPONSE] = "INVALID RESPONSE",
  };

  return errorStr[(size_t)gpsError];
}

/****************************** END OF FILE **********************************/
