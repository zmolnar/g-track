/**
 * @file sim8xxUrcThread.h
 * @brief
 */

#ifndef SIM8XX_URC_THREAD_H
#define SIM8XX_URC_THREAD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define URCPROCESSOR_WA_SIZE THD_WORKING_AREA_SIZE(2048)

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
THD_FUNCTION(SIM_UrcThread, arg);

#endif /* SIM8XX_URC_THREAD_H */

/****************************** END OF FILE **********************************/