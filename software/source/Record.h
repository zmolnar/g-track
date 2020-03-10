/**
 * @file Record.h
 * @brief
 */

#ifndef RECORD_H
#define RECORD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef struct Record_s {
  bool isBeingSent;
  uint32_t deviceId;
  char vehicleId[10];
  uint32_t year;
  uint32_t month;
  uint32_t day;
  uint32_t hour;
  uint32_t minute;
  uint32_t second;
  int32_t utcOffset;
  double latitude;
  double longitude;
  uint32_t speed;
  uint32_t numOfSatInUse;
  double batteryVoltage;
  uint32_t systemMode;
} Record_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
size_t REC_Serialize(const Record_t *rec, uint32_t id, char obuf[], size_t olen);

#endif /* RECORD_H */

/****************************** END OF FILE **********************************/
