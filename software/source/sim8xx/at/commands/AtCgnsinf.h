/**
 * @file AtCgnsinf.h
 * @brief
 */

#ifndef AT_CGNSINF_H
#define AT_CGNSINF_H

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
typedef struct {
  int runStatus;
  int fixStatus;
  uint8_t *date;
  double latitude;
  double longitude;
  double altitude;
  double speed;
  double course;
  int fixMode;
  double hdop;
  double pdop;
  double vdop;
  int gpsSatInView;
  int gnssSatInUse;
  int gnssSatInView;
  int cnomax;
  double hpa;
  double vpa;
} CGNSINF_Response_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
/**
 * 
 */
bool AT_CgnsinfCreate(uint8_t buf[], size_t length);

/**
 * 
 */
bool AT_CgnsinfParse(CGNSINF_Response_t *pres, uint8_t str[]);

#endif /* AT_CGNSINF_H */

/****************************** END OF FILE **********************************/
