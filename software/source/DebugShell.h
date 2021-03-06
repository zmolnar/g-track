/**
 * @file DebugShell.h
 * @brief
 */

#ifndef DEBUG_SHELL_H
#define DEBUG_SHELL_H

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "ch.h"
#include "hal.h"
#include "shell.h"

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
void debugShellInit(void);
void debugShellStart(void);
void debugShellStop(void);
void debugShellTerminated(void);

#endif /* DEBUG_SHELL_H */

/******************************* END OF FILE ***********************************/
