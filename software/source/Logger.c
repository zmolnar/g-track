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
static void LOG_createTimeStamp(char timestamp[], size_t length)
{
  GPS_Time_t dt = {0};
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
void LOG_AppendToFile(const char *file, const char *entry)
{
  LOG_WriteBuffer(file, entry, strlen(entry));
}

void LOG_WriteBuffer(const char *file, const char ibuf[], size_t ilen)
{
  char timestamp[25] = {0};
  LOG_createTimeStamp(timestamp, sizeof(timestamp));

  SDC_Lock();
  FIL logfile;
  if (FR_OK == f_open(&logfile, file, FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&logfile, timestamp, strlen(timestamp), &bw);
    f_write(&logfile, ibuf, ilen, &bw);
    f_write(&logfile, "\n", 1, &bw);
    f_close(&logfile);
  }
  SDC_Unlock();

}

void LOG_OverWriteFile(const char *file, const char *entry)
{
  SDC_Lock();
  FIL logfile;
  if (FR_OK == f_open(&logfile, file, FA_CREATE_ALWAYS | FA_WRITE)) {
    UINT bw = 0;
    f_write(&logfile, entry, strlen(entry), &bw);
    f_write(&logfile, "\n", 1, &bw);
    f_close(&logfile);
  }
  SDC_Unlock();
}

/****************************** END OF FILE **********************************/
