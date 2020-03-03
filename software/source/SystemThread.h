/**
 * @file SystemThread.h
 * @brief G-track system state machine handler thread.
 */

#ifndef SYSTEM_THREAD_H
#define SYSTEM_THREAD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define SYSTEM_THREAD_NAME "system"

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
/**
 * @brief System thread.
 */
THD_FUNCTION(SYS_Thread, arg);

/**
 * @brief Initialize resources used by the system thread.
 */
void SYS_Init(void);

/**
 * 
 */
void SYS_WaitForSuccessfulInit(void);

/**
 * @brief Notifies the system module that the ignition was set on.
 */
void SYS_IgnitionOn(void);

/**
 * @brief Notifies the system module that the ignition was set off.
 */
void SYS_IgnitionOff(void);

/**
 * @brief Get the text representation of system modules actual state.
 */
const char *SYS_GetStateString(void);

/**
 * @brief Get the text representation of the actual system module error code.
 */
const char *SYS_GetErrorString(void);

#endif /* SYSTEM_THREAD_H */

/***************************** END OF FILE *********************************/
