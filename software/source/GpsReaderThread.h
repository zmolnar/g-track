/**
 * @file GpsReaderThread.h
 * @brief
 */

#ifndef GPS_READER_THREAD_H
#define GPS_READER_THREAD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define GPS_READER_THREAD_NAME "gps"

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  GPS_NOT_POWERED,
  GPS_SEARCHING,
  GPS_LOCKED,
} GpsLockState_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
THD_FUNCTION(GpsReaderThread, arg);
void GpsReaderThreadInit(void);
void GpsReaderStart(void);
void GpsReaderStop(void);
GpsLockState_t GpsGetLockState(void);

    const char *GpsReaderGetStateString(void);
const char *GpsReaderGetErrorString(void);

#endif /* GPS_READER_THREAD_H */

/****************************** END OF FILE **********************************/
