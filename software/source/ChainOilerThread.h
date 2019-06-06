/**
 * @file ChainOilerThread.h
 * @brief
 */

#ifndef CHAIN_OILER_THREAD_H
#define CHAIN_OILER_THREAD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define CHAIN_OILER_THREAD_NAME        "chainoiler"

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
THD_FUNCTION(ChainOilerThread, arg);
void ChainOilerThreadInit(void);
void ChainOilerStart(void);
void ChainOilerStop(void);
void ChainOilerForceStart(void);
void ChainOilerForceStop(void);
void ChainOilerOneShot(void);

const char * ChainOilerGetStateString(void);
const char * ChainOilerGetErrorString(void);

#endif /* CHAIN_OILER_THREAD_H */

/****************************** END OF FILE **********************************/
