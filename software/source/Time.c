/**
 * @file Time.c
 * @brief Unified date and time conversion methods.
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Time.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static int dayOfWeek(int year, int month, int day) {
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

static bool isDaylightSavingTime(int day, int month, int dow) {
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;

  int previousSunday = day - dow;

  if (month == 3) return previousSunday >= 25;
  if (month == 10) return previousSunday < 25;

  return false;
}

static uint32_t convertMillisecondToHour(uint32_t millisecond) {
  return millisecond / 3600000;
}

static uint32_t convertMillisecondToMinute(uint32_t millisecond) {
  return millisecond % 3600000 / 60000;
}

static uint32_t convertMillisecondToSecond(uint32_t millisecond) {
  return millisecond % 3600000 % 60000 / 1000;
}

static uint32_t convertMillisecondToMilliSecond(uint32_t millisecond) {
  return millisecond % 3600000 % 60000 % 1000;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void convertDateTimeToRTCDateTime(DateTime_t *dt, RTCDateTime *rtc) {
  rtc->year = dt->year - 1980;
  rtc->month = dt->month;
  rtc->day = dt->day;
  rtc->millisecond  = 60 * 60 * 1000 * dt->hour;
  rtc->millisecond += 60 * 1000 * dt->min;
  rtc->millisecond += 1000 * dt->sec;
  rtc->millisecond += dt->msec;
  rtc->dayofweek = dayOfWeek(dt->year, dt->month, dt->day);
  rtc->dstflag = isDaylightSavingTime(dt->day, dt->month, rtc->dayofweek) ? 1 : 0;
}

void convertRTCDateTimeToDateTime(RTCDateTime *rtc, DateTime_t *dt) {
  dt->year = rtc->year + 1980;
  dt->month = rtc->month;
  dt->day = rtc->day;
  dt->hour = convertMillisecondToHour(rtc->millisecond);
  dt->min = convertMillisecondToMinute(rtc->millisecond);
  dt->sec = convertMillisecondToSecond(rtc->millisecond);
  dt->msec = convertMillisecondToMilliSecond(rtc->millisecond);
}

/****************************** END OF FILE **********************************/
