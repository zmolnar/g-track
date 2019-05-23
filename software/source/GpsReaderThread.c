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
#define MAX_TIME_DIFF_IN_MS         30000

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

static int dayOfWeek(int year, int month, int day) {
  if(month == 1) {
    month = 13;
    year--;
  }
  
  if (month == 2) {
    month = 14;
    year--;
  }
  
  int q = day;
  int m = month;
  int k = year % 100;
  int j = year / 100;
  int h = q + 13*(m+1)/5 + k + k/4 + j/4 + 5*j;
  h %= 7;
 
  return h;
}

static bool isDaylightSavingTime(int day, int month, int dow) {
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;

  int previousSunday = day - dow;

  if (month == 3) return previousSunday >= 25;
  if (month == 10) return previousSunday < 25;

  return false;  // this line never gonna happend
}

static bool convertDateToRTCDateTime(RTCDateTime *rtc, char *date) {
    char tmp;
    hijackChar(date+4, &tmp);
    rtc->year = atoi(date) - 1980;
    restoreChar(date+4, &tmp);
    
    hijackChar(date+6, &tmp);
    rtc->month = atoi(date+4);
    restoreChar(date+6, &tmp);
    
    hijackChar(date+8, &tmp);
    rtc->day = atoi(date+6);
    restoreChar(date+8, &tmp);
    
    hijackChar(date+10, &tmp);
    int hour = atoi(date+8);
    restoreChar(date+10, &tmp);
    
    hijackChar(date+12, &tmp);
    int min = atoi(date+10);
    restoreChar(date+12, &tmp);
    
    hijackChar(date+14, &tmp);
    int sec = atoi(date+12);
    restoreChar(date+14, &tmp);
    
    int msec = atoi(date+15);
    
    rtc->millisecond = 60*60*1000*hour + 60*1000*min + 1000*sec + msec;
    rtc->dayofweek = dayOfWeek(rtc->year+1980, rtc->month, rtc->day);
    rtc->dstflag = isDaylightSavingTime(rtc->day, rtc->month, rtc->dayofweek) ? 1 : 0;

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
  chsnprintf(buf, sizeof(buf), 
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

static bool isUpdateNeeded(RTCDateTime *gpsTime) {
  RTCDateTime rtcTime = {0};
  dbGetTime(&rtcTime);

  bool result = false;
  if (rtcTime.year != gpsTime->year)
    result = true;
  else if (rtcTime.month != gpsTime->month)
    result = true;
  else if (rtcTime.day != gpsTime->day)
    result = true;
  else {
    uint32_t diff = 0;
    if (rtcTime.millisecond < gpsTime->millisecond)
      diff = gpsTime->millisecond - rtcTime.millisecond;
    else 
      diff = rtcTime.millisecond - gpsTime->millisecond;

    result = (MAX_TIME_DIFF_IN_MS < diff);
  }

  return result;
}

static void updateClock(char *date) {
  RTCDateTime gpsTime = {0};
  convertDateToRTCDateTime(&gpsTime, date);

  if (isUpdateNeeded(&gpsTime))
    dbSetTime(&gpsTime);
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
