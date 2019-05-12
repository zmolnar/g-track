/**
 * @file GpsReaderThread.c
 * @brief
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "GpsReaderThread.h"
#include "BoardEvents.h"
#include "hal.h"
#include "sim8xx/sim8xx.h"
#include "chprintf.h"
#include <string.h>
#include <stdlib.h>

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/
#define GPS_UPDATE_PERIOD_IN_MS           5000

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/
typedef enum {
  GPS_ERROR_NO_ERROR,
  GPS_ERROR_POWER_ON,
  GPS_ERROR_POWER_OFF,
  GPS_ERROR_DATA_UPDATE
} gpsError_t;

typedef enum {
  GPS_POWER_ON,
  GPS_POWER_OFF
} gpsPowerAction_t;

typedef struct {
    int runStatus;
    int fixStatus;
    char date[18+1];
    float latitude;
    float longitude;
    float altitude;
    float speed;
    float course;
    int fixMode;
    float hdop;
    float pdop;
    float vdop;
    int gpsSatInView;
    int gnssSatInUse;
    int gnssSatInView;
    int cnomax;
    float hpa;
    float vpa;
} GpsCGNSINFData_t;

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static virtual_timer_t gpsTimer;
static Sim8xxCommand cmd;
static gpsError_t error;
static semaphore_t gpsSem;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static void gpsTimerCallback(void *p) {
  (void)p;
  chSysLockFromISR();
  chVTSetI(&gpsTimer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), gpsTimerCallback, NULL);
  chSemSignalI(&gpsSem);
  chSysUnlockFromISR();
}

static void gpsControlPower(gpsPowerAction_t action) {
  uint8_t arg = (GPS_POWER_ON == action) ? 1 : 0;

  do {
    chsnprintf(cmd.request, sizeof(cmd.request), "AT+CGNSPWR=%d", arg);
    sim8xxExecute(&SIM8D1, &cmd);
    if (SIM8XX_OK != cmd.status) {
        error = (GPS_POWER_ON == action) ? GPS_ERROR_POWER_ON : GPS_ERROR_POWER_OFF;
    } else {
        error = GPS_ERROR_NO_ERROR;
    }
    chThdSleepMilliseconds(1000);
  } while(GPS_ERROR_NO_ERROR != error);
}

static bool getNextInt(char **start, int *value, char delim) {
    char *end = strchr(*start, delim);
    if (!end) return false;
    *end = '\0';
    *value = atoi(*start);
    *end = delim;
    *start = end + 1;
    return true;
}

double my_atof(char str[]) {
    size_t len = strlen(str);
    if(0 == len) return 0.0;

    double val = 0.0;
    size_t i;
    for(i = 0; i<len && str[i]!='.'; ++i)
        val = 10*val +(str[i]-'0');

    if(i==len) return val;
    i++;

    double f = 1.0;
    while(i<len)
    {
        f *= 0.1;
        val += f*(str[i++]-'0');
    }

    return val;
}

bool getNextFloat(char **start, float *value, char delim) {
    char *end = strchr(*start, delim);
    if (!end) return false;
    *end = '\0';
    *value = my_atof(*start);
    *end = delim;
    *start = end + 1;
    return true;
}

bool getNextString(char **start, char *buf, size_t length, char delim) {
    char *end = strchr(*start, delim);
    if (!end) return false;
    *end = '\0';
    strncpy(buf, *start, length);
    *end = delim;
    *start = end + 1;
    return true;
}

bool skipReserved(char **start, size_t num, char delim) {
    while (num--) {
        char *end = strchr(*start, delim);
        if (!end) return false;
        *start = end + 1;
    }
    return true;
}

int dayOfWeek(int y, int m, int d) {
    return ((y-=m<3)+y/4-y/100+y/400+"-bed=pen+mad."[m]+d)%7;
}

void exchangeChar(char *c, char *tmp) {
    *tmp = *c;
    *c = '\0';
}

void restoreChar(char *c, char *tmp) {
    *c = *tmp;
}

bool isDaylightSavingTime(int day, int month, int dow) {
    if (month < 3 || month > 10)  return false; 
    if (month > 3 && month < 10)  return true; 

    int previousSunday = day - dow;

    if (month == 3) return previousSunday >= 25;
    if (month == 10) return previousSunday < 25;

    return false; // this line never gonna happend
}
#if 0    
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

bool parseCGNSINF(char *str) {
    const char *strEnd = str + strlen(str);
    
    char *start;
    
    char *begin = strchr(str, ' ');
    if (!begin) return false;
    
    start = ++begin;
    
    GpsCGNSINFData_t data;
    if (start < strEnd) {
        if (!getNextInt(&start, &data.runStatus, ','))
            return false;
    }
    
    if (start < strEnd) {
        if (!getNextInt(&start, &data.fixStatus, ','))
            return false;
    }

    if (start < strEnd) {
        if (!getNextString(&start, data.date, sizeof(data.date), ','))
            return false;
    }

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.latitude, ','))
            return false;
    }
    
    if (start < strEnd) {
        if (!getNextFloat(&start, &data.longitude, ','))
            return false;
    }

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.altitude, ','))
            return false;
    }

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.speed, ','))
            return false;
    }    

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.course, ','))
            return false;
    }

    if (start < strEnd) {
        if (!getNextInt(&start, &data.fixMode, ','))
            return false;
    }    
    
    if (start < strEnd) {
        skipReserved(&start, 1, ',');
    } 

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.hdop, ','))
            return false;
    }

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.pdop, ','))
            return false;
    }

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.vdop, ','))
            return false;
    }    
    
    if (start < strEnd) {
        skipReserved(&start, 1, ',');
    } 

    if (start < strEnd) {
        if (!getNextInt(&start, &data.gpsSatInView, ','))
            return false;
    }   

    if (start < strEnd) {
        if (!getNextInt(&start, &data.gnssSatInUse, ','))
            return false;
    }     
    
    if (start < strEnd) {
        if (!getNextInt(&start, &data.gnssSatInView, ','))
            return false;
    }     
    
    if (start < strEnd) {
        skipReserved(&start, 1, ',');
    } 

    if (start < strEnd) {
        if (!getNextInt(&start, &data.cnomax, ','))
            return false;
    } 

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.hpa, ','))
            return false;
    }

    if (start < strEnd) {
        if (!getNextFloat(&start, &data.vpa, '\r'))
            return false;
    }

    return true;
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(GpsReaderThread, arg) {
  (void)arg;
  chRegSetThreadName("gps-reader");

  while(true) {
    chSemWait(&gpsSem);

    sim8xxCommandInit(&cmd);
    strcpy(cmd.request, "AT+CGNSINF");
    sim8xxExecute(&SIM8D1, &cmd);
    
    if (SIM8XX_OK == cmd.status) {
#if 0        
      dashboardLockGpsData();
      GpsData_t *data = dashboardGetGpsData();
      parseResponse(cmd.response, data);
      dashboardUnlockGpsData();
      dashboardSaveOnSdcard();
#endif
      parseCGNSINF(cmd.response);
      error = GPS_ERROR_NO_ERROR;
    } else {
      error = GPS_ERROR_DATA_UPDATE;
    }
  }
} 

void GpsReaderThreadInit(void) {
    chVTObjectInit(&gpsTimer);
    chSemObjectInit(&gpsSem, 0);
}

void GpsReaderStartUpdate(void) {
  gpsControlPower(GPS_POWER_ON);
  chSysLock();
  chVTSetI(&gpsTimer, chTimeMS2I(GPS_UPDATE_PERIOD_IN_MS), gpsTimerCallback, NULL);
  chSemSignalI(&gpsSem);
  chSysUnlock();
}

void GpsReaderStopUpdate(void) {
  chSysLock();
  chVTResetI(&gpsTimer);
  chSysUnlock();
  gpsControlPower(GPS_POWER_OFF);
}

/******************************* END OF FILE ***********************************/
