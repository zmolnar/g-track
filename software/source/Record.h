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
#define BUFFER_LENGTH 24

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef struct Record_s {
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
  double gsmSignalStrehgth;
  uint32_t numOfSatInUse;
  double batteryVoltage;
  uint32_t systemMode;
} Record_t;

typedef struct Item_s {
  Record_t record;
  bool isSent;
} Item_t;

typedef struct RecordBuffer_s {

  Item_t records[BUFFER_LENGTH];
  size_t wrindex;
  size_t rdindex;
} RecordBuffer_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
size_t REC_Serialize(const Record_t *rec, uint32_t id, char obuf[], size_t olen);

void REC_EmptyBuffer(RecordBuffer_t *this);

void REC_PushRecord(RecordBuffer_t *this, Record_t *new);

size_t REC_GetSize(RecordBuffer_t *this);

size_t REC_GetNumOfUnsentRecords(RecordBuffer_t *this);

bool REC_PopRecordIfSent(RecordBuffer_t *this);

bool REC_GetNextRecordAndMarkAsSent(RecordBuffer_t *this, Record_t **next);

size_t REC_CancelLastTransaction(RecordBuffer_t *this);

#endif /* RECORD_H */

/****************************** END OF FILE **********************************/
