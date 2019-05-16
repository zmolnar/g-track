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

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
void dbInit(void);

void dbLock(void);

void dbUnlock(void);

Position_t *dbGetPosition(void);

#endif /* DASHBOARD_H */

/****************************** END OF FILE **********************************/