/**
 * @file ModemHandlerThread.h
 * @brief SIM8xx modem handler thread.
 * @author Molnar Zoltan
*/

#ifndef MODEMHANDLERTHREAD_H
#define MODEMHANDLERTHREAD_H

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "ch.h"
#include "hal.h"

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
THD_FUNCTION(ModemHandlerThread, arg);

#endif

/******************************* END OF FILE ***********************************/
