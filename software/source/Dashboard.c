/**
 * @file Dashboard.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Dashboard.h"
#include "ch.h"

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

void dbSetTime(RTCDateTime *ptime) {
    dbLock();
    rtcSetTime(&RTCD1, ptime);
    dbUnlock();
}

void dbGetTime(RTCDateTime *ptime) {
    dbLock();
    rtcGetTime(&RTCD1, ptime);
    dbUnlock();
}

/****************************** END OF FILE **********************************/
