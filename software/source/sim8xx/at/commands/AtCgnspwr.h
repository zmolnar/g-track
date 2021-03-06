/**
 * @file AtCgnspwr.h
 * @brief
 */

#ifndef AT_CGNSPWR_H
#define AT_CGNSPWR_H

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
bool atCgnspwrCreateOn(char buf[], size_t length);
bool atCgnspwrCreateOff(char buf[], size_t length);

#endif /* AT_CGNSPWR_H */

/****************************** END OF FILE **********************************/
