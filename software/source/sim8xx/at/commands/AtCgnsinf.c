/**
 * @file AtCgnsinf.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "AtCgnsinf.h"
#include "AtUtils.h"

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

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
bool atCgnsinfCreate(char buf[], size_t length) {
  strncpy(buf, length, "AT+CGNSINF");
  return true;
}

bool atCgnsinfParse(CGNSINF_Response_t *pdata, char str[]) {
  memset(pdata, 0, sizeof(*pdata));

  const char *strEnd = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start) return false;

  ++start;

  if (start < strEnd) {
    if (!atGetNextInt(&start, &data.runStatus, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &data.fixStatus, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextString(&start, data.date, sizeof(data.date), ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.latitude, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.longitude, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.altitude, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.speed, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.course, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &data.fixMode, ',')) return false;
  }

  if (start < strEnd) {
    atSkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.hdop, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.pdop, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.vdop, ',')) return false;
  }

  if (start < strEnd) {
    atSkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &data.gpsSatInView, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &data.gnssSatInUse, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &data.gnssSatInView, ',')) return false;
  }

  if (start < strEnd) {
    atSkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &data.cnomax, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.hpa, ',')) return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &data.vpa, '\r')) return false;
  }

  return true;
}

/****************************** END OF FILE **********************************/
