/**
 * @file Record.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Record.h"

#include <string.h>

#if defined(TEST)
#include <stdio.h>
#define SNPRINTF snprintf
#else
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#define SNPRINTF chsnprintf
#endif

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/
#define INCREMENT_INDEX(i) ((i)=(((i)+1)%BUFFER_LENGTH))

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
size_t REC_Serialize(const Record_t *rec, uint32_t id, char obuf[], size_t olen)
{
  memset(obuf, 0, olen);
  SNPRINTF(obuf, olen,
             "record[%d]=%d,%s,%d,%d,%d,%d,%d,%d,%d,%f,%f,%d,%d,%f,%d",
             id,
             rec->deviceId,
             rec->vehicleId,
             rec->year,
             rec->month,
             rec->day,
             rec->hour,
             rec->minute,
             rec->second,
             rec->utcOffset,
             rec->latitude,
             rec->longitude,
             rec->speed,
             rec->numOfSatInUse,
             rec->batteryVoltage,
             rec->systemMode
             );

  return strlen(obuf);
}

void REC_EmptyBuffer(RecordBuffer_t *this)
{
  memset(this, 0, sizeof(*this));
}

void REC_PushRecord(RecordBuffer_t *this, Record_t *new)
{
  this->records[this->wrindex].record = *new;

  INCREMENT_INDEX(this->wrindex);
  if (this->wrindex == this->rdindex)
    INCREMENT_INDEX(this->rdindex);
}

size_t REC_GetSize(RecordBuffer_t *this)
{
  size_t length = BUFFER_LENGTH + this->wrindex - this->rdindex;
  length %= BUFFER_LENGTH;

  return length;
}

bool REC_PopRecordIfSent(RecordBuffer_t *this)
{
  bool result = false;

  if (REC_GetSize(this)) {
    Item_t *pitem = &this->records[this->rdindex];
    if (pitem->isSent) {
      INCREMENT_INDEX(this->rdindex);
      result = true;
    }
  }

  return result;
}

bool REC_GetNextRecordAndMarkAsSent(RecordBuffer_t *this, Record_t **next)
{
  bool result = false;
  size_t size = REC_GetSize(this);

  size_t i;
  for (i = 0; i <= size; ++i) {
    size_t index = (this->rdindex + i) % BUFFER_LENGTH;
    Item_t *pitem = &this->records[index];
    if (!pitem->isSent) {
      pitem->isSent = true;
      *next = &pitem->record;
      result = true;
      break;
    }
  }

  return result;
}

/****************************** END OF FILE **********************************/
