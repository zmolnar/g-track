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
  Position_t position;
} Dashboard_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static Dashboard_t dashboard;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void dbLock(void) {
    chMtxLock(&dashboard.lock);
}

static void dbUnlock(void) {
    chMtxUnlock(&dashboard.lock);
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void dbInit(void) {
    memset(&dashboard, 0, sizeof(dashboard));
    chMtxObjectInit(&dashboard.lock);
    rtcObjectInit(&RTCD1);
}

void dbGetPosition(Position_t *pos) {
    dbLock();
    *pos = dashboard.position;
    dbUnlock();
}

void dbSetPosition(Position_t *new) {
    dbLock();
    dashboard.position = *new;
    dbUnlock();
}

void dbGetTime(DateTime_t *time) {
    RTCDateTime rtcDateTime = {0};
    dbLock();
    rtcGetTime(&RTCD1, &rtcDateTime);
    dbUnlock();
    convertRTCDateTimeToDateTime(&rtcDateTime, time);
}

void dbSetTime(DateTime_t *time) {
    RTCDateTime rtcDateTime = {0};
    convertDateTimeToRTCDateTime(time, &rtcDateTime);
    dbLock();
    rtcSetTime(&RTCD1, &rtcDateTime);
    dbUnlock();
}

size_t dbCreateTimestamp(char buf[], size_t length) {
    DateTime_t dt = {0};
    dbGetTime(&dt);

    int n = chsnprintf(buf, length, "%d-%02d-%02d %02d:%02d:%02d >>> ",
                       dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
    return (size_t)n;
}

/****************************** END OF FILE **********************************/
