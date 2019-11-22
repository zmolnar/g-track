/**
 * @file UrcBluetooth.h
 * @brief
 */

#ifndef URC_BLUETOOTH_H
#define URC_BLUETOOTH_H

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
typedef struct URC_BtConnect_s {
  int id;
  uint8_t *name;
  uint8_t *address;
  uint8_t *profile
} URC_BtConnect_t;

typedef struct URC_BtDisconnect_s {
  uint8_t *name;
  uint8_t *address;
  uint8_t *profile;
} URC_BtDisconnect_t;

typedef struct URC_BtConnecting_s {
  uint8_t *address;
  uint8_t *profil;
} URC_BtConnecting_t;

typedef struct URC_BtSppData_s {
    int id;
    int length;
    uint8_t *data;
} URC_BtSppData_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
/**
 * 
 */
bool URC_IsBtUrc(const uint8_t str[]);

/**
 *
 */
bool URC_IsBtConnect(const uint8_t str[]);

/**
 *
 */
bool URC_BtConnectParse(uint8_t str[], URC_BtConnect_t *urc);

/**
 *
 */
bool URC_IsBtDisconnect(const uint8_t str[]);

/**
 *
 */
bool URC_BtDisconnectParse(uint8_t str[], URC_BtDisconnect_t *urc);

/*
 *
 */ 
bool URC_IsBtConnecting(const uint8_t str[]);

/**
 *
 */
bool URC_BtConnectingParse(uint8_t str[], URC_BtConnecting_t *urc);

/**
 *
 */
bool URC_IsBtSppData(const uint8_t str[]);

/**
 *
 */
bool URC_BtSppDataParse(uint8_t str[], URC_BtSppData_t *urc);

#endif /* URC_BLUETOOTH_H */

/****************************** END OF FILE **********************************/
