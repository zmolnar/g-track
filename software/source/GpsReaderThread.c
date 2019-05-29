/**
 * @file GpsReaderThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "GpsReaderThread.h"
#include "Sdcard.h"
#include "Dashboard.h"
#include "sim8xx.h"
#include "at.h"

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define GPS_UPDATE_PERIOD_IN_MS     5000
#define MAX_TIME_DIFF_IN_SEC        5

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  GPS_ERROR_NO_ERROR,
  GPS_ERROR_UNKNOWN_COMMAND,
  GPS_ERROR_POWER_ON,
  GPS_ERROR_POWER_OFF,
  GPS_ERROR_DATA_UPDATE,
  GPS_ERROR_IN_RESPONSE
} GpsError_t;

typedef enum { 
  GPS_CMD_START, 
  GPS_CMD_STOP, 
  GPS_CMD_UPDATE 
} GpsCommand_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static virtual_timer_t gpsTimer;
static Sim8xxCommand cmd;
static GpsError_t error;
static msg_t events[10];
static mailbox_t gpsMailbox;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void hijackChar(char *c, char *tmp) {
  *tmp = *c;
  *c = '\0';
}

static void restoreChar(char *c, char *tmp) { 
  *c = *tmp; 
}

static bool convertRawDateToDateTime(DateTime_t *dt, char *raw) {
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

static void saveInLogfile(const char *data, size_t length) {
  FIL log;
  if (FR_OK == f_open(&log, "/sim8xx_gnss.log", FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&log, data, length, &bw);
    f_close(&log);
  }
}

static void logPosition(CGNSINF_Response_t *pdata) {
  char buf[150] = {0};
  size_t end = dbCreateTimestamp(buf, sizeof(buf));
  chsnprintf(buf + end, sizeof(buf) - end, 
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

static void savePosition(CGNSINF_Response_t *data) {
  Position_t gpsPos = {0};
  strncpy(gpsPos.date, data->date, sizeof(gpsPos.date));
  gpsPos.latitude = data->latitude;
  gpsPos.longitude = data->longitude;
  gpsPos.altitude = data->altitude;
  gpsPos.speed = data->speed;
  gpsPos.gnssSatInUse = data->gnssSatInUse;
  gpsPos.gnssSatInView = data->gnssSatInView;
  gpsPos.gpsSatInView = data->gpsSatInView;
  dbSetPosition(&gpsPos);
}

static bool isUpdateNeeded(DateTime_t *gpsTime) {
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

static void updateClock(char *rawDateTime) {
  DateTime_t gpsDateTime = {0};
  convertRawDateToDateTime(&gpsDateTime, rawDateTime);

  if (isUpdateNeeded(&gpsDateTime)) 
    dbSetTime(&gpsDateTime);
}

static void updatePosition(void) {
  sim8xxCommandInit(&cmd);
  atCgnsinfCreate(cmd.request, sizeof(cmd.request));
  sim8xxExecute(&SIM8D1, &cmd);

  if (SIM8XX_OK == cmd.status) {
    CGNSINF_Response_t data;
    if (atCgnsinfParse(&data, cmd.response)) {
      error = GPS_ERROR_NO_ERROR;
      savePosition(&data);
      updateClock(data.date);
      logPosition(&data);
    } else {
      error = GPS_ERROR_IN_RESPONSE;
    }
  } else {
    error = GPS_ERROR_DATA_UPDATE;
  }
}

static void timerCallback(void *p) {
  (void)p;
  chSysLockFromISR();
  chVTSetI(&gpsTimer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), timerCallback, NULL);
  chMBPostI(&gpsMailbox, GPS_CMD_UPDATE);
  chSysUnlockFromISR();
}

static void gpsPowerOn(void) {
  do {
    sim8xxCommandInit(&cmd);
    atCgnspwrCreateOn(cmd.request, sizeof(cmd.request));
    sim8xxExecute(&SIM8D1, &cmd);
    if (SIM8XX_OK == cmd.status) {
      error = GPS_ERROR_NO_ERROR;
    } else {
      error = GPS_ERROR_POWER_ON;
      chThdSleepMilliseconds(1000);
    }
  } while (GPS_ERROR_NO_ERROR != error);
}

static void gpsPowerOff(void) {
  do {
    sim8xxCommandInit(&cmd);
    atCgnspwrCreateOff(cmd.request, sizeof(cmd.request));
    sim8xxExecute(&SIM8D1, &cmd);
    if (SIM8XX_OK == cmd.status) {
      error = GPS_ERROR_NO_ERROR;
    } else {
      error = GPS_ERROR_POWER_OFF;
      chThdSleepMilliseconds(1000);
    }
  } while (GPS_ERROR_NO_ERROR != error);
}

static void startTimer(void) {
  chSysLock();
  chVTSetI(&gpsTimer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), timerCallback, NULL);
  chSysUnlock();
}

static void stopTimer(void) {
  chSysLock();
  chVTResetI(&gpsTimer);
  chSysUnlock();
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(GpsReaderThread, arg) {
  (void)arg;
  chRegSetThreadName("gps");

  while (true) {
    GpsCommand_t ecmd;
    if (MSG_OK == chMBFetchTimeout(&gpsMailbox, (msg_t *)&ecmd, TIME_INFINITE)) {
      switch (ecmd) {
      case GPS_CMD_START: {
        gpsPowerOn();
        startTimer();
        break;
      }
      case GPS_CMD_STOP: {
        stopTimer();
        gpsPowerOff();
        break;
      }
      case GPS_CMD_UPDATE: {
        updatePosition();
        break;
      }
      default: { 
        error = GPS_ERROR_UNKNOWN_COMMAND;
        break;
      }
      }
    }
  }
}

void GpsReaderThreadInit(void) {
  chVTObjectInit(&gpsTimer);
  memset(events, 0, sizeof(events));
  chMBObjectInit(&gpsMailbox, events, sizeof(events) / sizeof(events[0]));
}

void GpsReaderStart(void) { 
  chSysLock();
  chMBPostI(&gpsMailbox, GPS_CMD_START);
  chSysUnlock();
}

void GpsReaderStop(void) {
  chSysLock();
  chMBPostI(&gpsMailbox, GPS_CMD_STOP);
  chSysUnlock();
}

/****************************** END OF FILE **********************************/
