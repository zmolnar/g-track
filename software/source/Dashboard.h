/**
 * @file Dashboard.h
 * @brief
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/

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

typedef struct {
  mutex_t lock;
  Position_t position;
} Dashboard_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/
extern Dashboard_t dashboard;

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
void dbInit(Dashboard_t *pdsh);
void dbLock(Dashboard_t *pdsh);
void dbUnlock(Dashboard_t *pdsh);
Position_t *dbGetPosition(Dashboard_t *pdsh);

#endif /* DASHBOARD_H */

/****************************** END OF FILE **********************************/
