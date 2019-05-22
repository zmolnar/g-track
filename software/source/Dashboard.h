/**
 * @file Dashboard.h
 * @brief
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"
#include "hal.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef struct {
  char date[18 + 1];
  double latitude;
  double longitude;
  double altitude;
  double speed;
  int gpsSatInView;
  int gnssSatInUse;
  int gnssSatInView;
} Position_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
void dbInit(void);
void dbGetPosition(Position_t *pos);
void dbSetPosition(Position_t *new);
void dbSetTime(RTCDateTime *ptime);
void dbGetTime(RTCDateTime *ptime);

#endif /* DASHBOARD_H */

/****************************** END OF FILE **********************************/
