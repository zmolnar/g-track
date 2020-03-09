/**
 * @file ConfigManagerThread.h
 * @brief
 */

#ifndef CONFIG_MANAGER_THREAD_H
#define CONFIG_MANAGER_THREAD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"
#include "Config.h"

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
THD_FUNCTION(CFM_Thread, arg);

void CFM_Init(void);

void CFM_WaitForValidConfig(void);

void CFM_LoadConfigI(void);

void CFM_LoadConfig(void);

void CFM_StoreConfigI(void);

void CFM_StoreConfig(void);

const VehicleConfig_t *CFM_GetVehicleConfig(void);

const BluetoothConfig_t *CFM_GetBluetoothConfig(void);

const GprsConfig_t * CFM_GetGprsConfig(void);

const BackendConfig_t * CFM_GetBackendConfig(void);

const SimConfig_t * CFM_GetSimConfig(void);

const GpsConfig_t * CFM_GetGpsConfig(void);

const MotionDetectorConfig_t * CFM_GetMotionDetectorConfig(void);

#endif /* CONFIG_MANAGER_THREAD_H */

/****************************** END OF FILE **********************************/
