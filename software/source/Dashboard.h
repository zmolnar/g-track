/**
 * @file Dashboard.h
 * @brief
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Time.h"
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
void dbGetTime(DateTime_t *time);
void dbSetTime(DateTime_t *time);

size_t dbCreateTimestamp(char buf[], size_t length);

#endif /* DASHBOARD_H */

/****************************** END OF FILE **********************************/
