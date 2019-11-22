/**
 * @file Logger.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Logger.h"

#include "Dashboard.h"
#include "Sdcard.h"
#include "ch.h"
#include "chprintf.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void LOG_createTimeStamp(uint8_t timestamp[], size_t length)
{
  DSB_DateTime_t dt = {0};
  DSB_GetTime(&dt);

  chsnprintf(timestamp,
             length,
             "%d-%02d-%02d %02d:%02d:%02d ",
             dt.year,
             dt.month,
             dt.day,
             dt.hour,
             dt.min,
             dt.sec);
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void LOG_Write(const uint8_t *file, const uint8_t *entry)
{
  uint8_t timestamp[25] = {0};
  LOG_createTimeStamp(timestamp, sizeof(timestamp));

  SDC_Lock();
  FIL logfile;
  if (FR_OK == f_open(&logfile, file, FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&logfile, timestamp, strlen(timestamp), &bw);
    f_write(&logfile, entry, strlen(entry), &bw);
    f_write(&logfile, "\n", 1, &bw);
    f_close(&logfile);
  }
  SDC_Unlock();
}

void LOG_Overwrite(const uint8_t *file, const uint8_t *entry)
{
  SDC_Lock();
  FIL logfile;
  if (FR_OK == f_open(&logfile, file, FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&logfile, entry, strlen(entry), &bw);
    f_write(&logfile, "\n", 1, &bw);
    f_close(&logfile);
  }
  SDC_Unlock();
}

/****************************** END OF FILE **********************************/
