/**
 * @file BluetoothManagerThread.h
 * @brief
 */

#ifndef BLUETOOTH_MANAGER_THREAD_H
#define BLUETOOTH_MANAGER_THREAD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"
#include "BluetoothStream.h"

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
THD_FUNCTION(BLT_Thread, arg);

/**
 * 
 */
void BLT_Init(void);

/**
 * 
 */
void BLT_Start(void);

/**
 * 
 */
void BLT_Stop(void);

/**
 * 
 */
void BLT_ProcessUrc(void);

/**
 * 
 */
void BLT_SendStreamData(void);

/**
 * 
 */
void BLT_SendUserData(void);

#endif /* BLUETOOTH_MANAGER_THREAD_H */

    /****************************** END OF FILE **********************************/
