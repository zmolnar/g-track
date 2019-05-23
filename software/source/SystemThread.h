/**
 * @file SystemThread.h
 * @brief G-track system state machine handler thread.
 */

#ifndef SYSTEM_THREAD_H
#define SYSTEM_THREAD_H

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "ch.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                             */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                             */
/*******************************************************************************/
THD_FUNCTION(SystemThread, arg);
void SystemThreadInit(void);
void SystemThreadIgnitionOn(void);
void SystemThreadIgnitionOff(void);

#endif /* SYSTEM_THREAD_H */

/******************************* END OF FILE ***********************************/
