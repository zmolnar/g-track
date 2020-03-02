/**
 * @file ReporterThread.h
 * @brief
 */

#ifndef REPORTER_THREAD_H
#define REPORTER_THREAD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
THD_FUNCTION(RPT_Thread, arg);

void RPT_Init(void);
void RPT_Start(void);
void RPT_Stop(void);

#endif /* REPORTER_THREAD_H */

/****************************** END OF FILE **********************************/
