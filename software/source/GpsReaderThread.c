/**
 * @file GpsReaderThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "GpsReaderThread.h"
#include "Sdcard.h"
#include "BoardEvents.h"
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

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  GPS_ERROR_NO_ERROR,
  GPS_ERROR_POWER_ON,
  GPS_ERROR_POWER_OFF,
  GPS_ERROR_DATA_UPDATE,
  GPS_ERROR_IN_RESPONSE
} gpsError_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static virtual_timer_t gpsTimer;
static Sim8xxCommand cmd;
static gpsError_t error;
static semaphore_t gpsSem;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void gpsTimerCallback(void *p) {
  (void)p;
  chSysLockFromISR();
  chVTSetI(&gpsTimer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), gpsTimerCallback,
           NULL);
  chSemSignalI(&gpsSem);
  chSysUnlockFromISR();
}

static void gpsPowerOn(void) {
  do {
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

#if 0    
int dayOfWeek(int y, int m, int d) {
  return ((y -= m < 3) + y / 4 - y / 100 + y / 400 + "-bed=pen+mad."[m] + d) %
         7;
}

bool isDaylightSavingTime(int day, int month, int dow) {
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;

  int previousSunday = day - dow;

  if (month == 3) return previousSunday >= 25;
  if (month == 10) return previousSunday < 25;

  return false;  // this line never gonna happend
}

bool convertDateToRTCDateTime(RTCDateTime *rtc, char *date) {
    char buf[19] = {0};
    size_t i;

    char tmp;
    exchangeChar(date+4, &tmp);
    rtc->year = atoi(date) - 1980;
    restoreChar(date+4, &tmp);
    
    exchangeChar(date+6, &tmp);
    rtc->month = atoi(date+4);
    restoreChar(date+6, &tmp);
    
    exchangeChar(date+8, &tmp);
    rtc->day = atoi(date+6);
    restoreChar(date+8, &tmp);
    
    exchangeChar(date+10, &tmp);
    int hour = atoi(date+8);
    restoreChar(date+10, &tmp);
    
    exchangeChar(date+12, &tmp);
    int min = atoi(date+10);
    restoreChar(date+12, &tmp);
    
    exchangeChar(date+14, &tmp);
    int sec = atoi(date+12);
    restoreChar(date+14, &tmp);
    
    int msec = atoi(date+15);

    
    rtc->millisecond = 60*60*1000*hour + 60*1000*min + 1000*sec + msec;
    
    rtc->dayofweek = dayOfWeek(rtc->year+1980, rtc->month, rtc->day);
    
    rtc->dstflag = isDaylightSavingTime(rtc->day, rtc->month, rtc->dayofweek) ? 1 : 0;

    return true;
}
#endif

static void saveBuffer(const char *data, size_t length) {
  FIL log;
  if (FR_OK == f_open(&log, "/sim8xx_gnss.log", FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&log, data, length, &bw);
    f_close(&log);
  }
}

static void printGpsData(CGNSINF_Response_t *pdata) {
  char buf[150] = {0};
  chsnprintf(buf, sizeof(buf), "%s %f %f %f %f %d %d %d %d\n", 
              pdata->date, 
              pdata->latitude, 
              pdata->longitude,
              pdata->speed,
              pdata->altitude,
              pdata->fixStatus,
              pdata->gpsSatInView,
              pdata->gnssSatInView,
              pdata->gnssSatInUse);
  saveBuffer(buf, strlen(buf));  
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(GpsReaderThread, arg) {
  (void)arg;
  chRegSetThreadName("gps");

  while (true) {
    chSemWait(&gpsSem);

    sim8xxCommandInit(&cmd);
    atCgnsinfCreate(cmd.request, sizeof(cmd.request));
    sim8xxExecute(&SIM8D1, &cmd);

    if (SIM8XX_OK == cmd.status) {
      CGNSINF_Response_t data;
      bool status = atCgnsinfParse(&data, cmd.response);
      error = status ? GPS_ERROR_NO_ERROR : GPS_ERROR_IN_RESPONSE;
      printGpsData(&data);
    } else {
      error = GPS_ERROR_DATA_UPDATE;
    }
  }
}

void GpsReaderThreadInit(void) {
  chVTObjectInit(&gpsTimer);
  chSemObjectInit(&gpsSem, 0);
}

void GpsReaderStart(void) {
  gpsPowerOn();
  chSysLock();
  chVTSetI(&gpsTimer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), gpsTimerCallback,
           NULL);
  chSemSignalI(&gpsSem);
  chSysUnlock();
}

void GpsReaderStop(void) {
  chSysLock();
  chVTResetI(&gpsTimer);
  chSysUnlock();
  gpsPowerOff();
}

/****************************** END OF FILE **********************************/
