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
void ChainOilerStartI(void);
void ChainOilerStart(void);
void ChainOilerStopI(void);
void ChainOilerStop(void);
void ChainOilerForceStartI(void);
void ChainOilerForceStart(void);
void ChainOilerForceStopI(void);
void ChainOilerForceStop(void);
void ChainOilerOneShotI(void);
void ChainOilerOneShot(void);

const char * ChainOilerGetStateString(void);
const char * ChainOilerGetErrorString(void);

#endif /* CHAIN_OILER_THREAD_H */

/****************************** END OF FILE **********************************/
