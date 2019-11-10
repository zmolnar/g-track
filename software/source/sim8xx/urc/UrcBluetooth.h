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
  char name[18];
  char address[18];
  char profile[5];
} URC_BtConnect_t;

typedef struct URC_BtConnecting_s {
  char address[18];
  char profile[5];
} URC_BtConnecting_t;

typedef struct URC_BtSppData_s {
    int id;
    int length;
    char data[512];
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
bool URC_IsBtUrc(const char str[]);

/**
 *
 */
bool URC_IsBtConnect(const char str[]);

/**
 *
 */
bool URC_BtConnectParse(char str[], URC_BtConnect_t *urc);

/*
 *
 */ 
bool URC_IsBtConnecting(const char str[]);

/**
 *
 */
bool URC_BtConnectingParse(char str[], URC_BtConnecting_t *urc);

/**
 *
 */
bool URC_IsBtSppData(const char str[]);

/**
 *
 */
bool URC_BtSppDataParse(char str[], URC_BtSppData_t *urc);

#endif /* URC_BLUETOOTH_H */

/****************************** END OF FILE **********************************/
