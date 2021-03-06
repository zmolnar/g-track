/**
 * @file GpsReaderThread.h
 * @brief
 */

#ifndef GPS_READER_THREAD_H
#define GPS_READER_THREAD_H

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "ch.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                             */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                             */
/*******************************************************************************/
THD_FUNCTION(GpsReaderThread, arg);
void GpsReaderThreadInit(void);
void GpsReaderStart(void);
void GpsReaderStop(void);

#endif /* GPS_READER_THREAD_H */

/******************************* END OF FILE ***********************************/
