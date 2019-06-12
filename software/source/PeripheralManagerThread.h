/**
 * @file PeripheralManagerThread.h
 * @brief
 */

#ifndef PERIPHERAL_MANAGER_THREAD_H
#define PERIPHERAL_MANAGER_THREAD_H

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
THD_FUNCTION(PeripheralManagerThread, arg);
void PeripheralManagerThreadInit(void);
void PeripheralManagerSdcInserted(void);
void PeripheralManagerSdcRemoved(void);
void PeripheralManagerUsbConnected(void);
void PeripheralManagerUsbDisconnected(void);

#endif /* PERIPHERAL_MANAGER_THREAD_H */

/****************************** END OF FILE **********************************/
