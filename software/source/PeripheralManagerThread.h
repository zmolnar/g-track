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
THD_FUNCTION(PRP_Thread, arg);

/**
 * @brief Initialize resources used by peripheral manager.
 */
void PRP_Init(void);

/**
 * @brief Signal to the peripheral manager, that the SD card is inserted.
 */
void PRP_SdcInserted(void);

/**
 * @brief Signal to the peripheral manager, that the SD card is removed.
 */
void PRP_SdcRemoved(void);

/**
 * @brief Signal to the peripheral manager, that the USB is connected.
 */
void PRP_UsbConnected(void);

/**
 * @brief Signal to the peripheral manager, that the USB is disconnected.
 */
void PRP_UsbDisconnected(void);

#endif /* PERIPHERAL_MANAGER_THREAD_H */

/****************************** END OF FILE **********************************/
