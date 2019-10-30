/**
 * @file Dashboard.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Dashboard.h"

#include "ch.h"
#include "chprintf.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef struct {
  mutex_t lock;
  DSB_Position_t position;
} DSB_Dashboard_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static DSB_Dashboard_t dashboard;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void DSB_lock(void)
{
  chMtxLock(&dashboard.lock);
}

static void DSB_unlock(void)
{
  chMtxUnlock(&dashboard.lock);
}

static int DSB_dayOfWeek(int year, int month, int day)
{
  if (month == 1) {
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
  int h = q + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j;
  h %= 7;

  return h;
}

static bool DSB_isDaylightSavingTime(int day, int month, int dow)
{
  if (month < 3 || month > 10)
    return false;
  if (month > 3 && month < 10)
    return true;

  int previousSunday = day - dow;

  if (month == 3)
    return previousSunday >= 25;
  if (month == 10)
    return previousSunday < 25;

  return false;
}

static uint32_t DSB_convertMillisecondToHour(uint32_t millisecond)
{
  return millisecond / 3600000;
}

static uint32_t DSB_convertMillisecondToMinute(uint32_t millisecond)
{
  return millisecond % 3600000 / 60000;
}

static uint32_t DSB_convertMillisecondToSecond(uint32_t millisecond)
{
  return millisecond % 3600000 % 60000 / 1000;
}

static uint32_t DSB_convertMillisecondToMilliSecond(uint32_t millisecond)
{
  return millisecond % 3600000 % 60000 % 1000;
}

static void DSB_convertDateTimeToRTCDateTime(DSB_DateTime_t *dt, RTCDateTime *rtc)
{
  rtc->year        = dt->year - 1980;
  rtc->month       = dt->month;
  rtc->day         = dt->day;
  rtc->millisecond = 60 * 60 * 1000 * dt->hour;
  rtc->millisecond += 60 * 1000 * dt->min;
  rtc->millisecond += 1000 * dt->sec;
  rtc->millisecond += dt->msec;
  rtc->dayofweek = DSB_dayOfWeek(dt->year, dt->month, dt->day);
  rtc->dstflag = DSB_isDaylightSavingTime(dt->day, dt->month, rtc->dayofweek) ? 1 : 0;
}

static void DSB_convertRTCDateTimeToDateTime(RTCDateTime *rtc, DSB_DateTime_t *dt)
{
  dt->year  = rtc->year + 1980;
  dt->month = rtc->month;
  dt->day   = rtc->day;
  dt->hour  = DSB_convertMillisecondToHour(rtc->millisecond);
  dt->min   = DSB_convertMillisecondToMinute(rtc->millisecond);
  dt->sec   = DSB_convertMillisecondToSecond(rtc->millisecond);
  dt->msec  = DSB_convertMillisecondToMilliSecond(rtc->millisecond);
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void DSB_Init(void)
{
  memset(&dashboard, 0, sizeof(dashboard));
  chMtxObjectInit(&dashboard.lock);
  rtcObjectInit(&RTCD1);
}

void DSB_GetPosition(DSB_Position_t *pos)
{
  DSB_lock();
  *pos = dashboard.position;
  DSB_unlock();
}

void DSB_SetPosition(DSB_Position_t *new)
{
  DSB_lock();
  dashboard.position = *new;
  DSB_unlock();
}

void DSB_GetTime(DSB_DateTime_t *time)
{
  RTCDateTime rtcDateTime = {0};
  DSB_lock();
  rtcGetTime(&RTCD1, &rtcDateTime);
  DSB_unlock();
  DSB_convertRTCDateTimeToDateTime(&rtcDateTime, time);
}

void DSB_SetTime(DSB_DateTime_t *time)
{
  RTCDateTime rtcDateTime = {0};
  DSB_convertDateTimeToRTCDateTime(time, &rtcDateTime);
  DSB_lock();
  rtcSetTime(&RTCD1, &rtcDateTime);
  DSB_unlock();
}

/****************************** END OF FILE **********************************/
