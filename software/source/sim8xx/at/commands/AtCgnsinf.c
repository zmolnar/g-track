/**
 * @file AtCgnsinf.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "AtCgnsinf.h"

#include "AtUtil.h"

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

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
bool atCgnsinfCreate(char buf[], size_t length)
{
  strncpy(buf, "AT+CGNSINF", length);
  return true;
}

bool atCgnsinfParse(CGNSINF_Response_t *pdata, char str[])
{
  memset(pdata, 0, sizeof(*pdata));

  const char *strEnd = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start)
    return false;

  ++start;

  if (start < strEnd) {
    if (!atGetNextInt(&start, &pdata->runStatus, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &pdata->fixStatus, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextString(&start, pdata->date, sizeof(pdata->date), ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->latitude, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->longitude, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->altitude, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->speed, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->course, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &pdata->fixMode, ','))
      return false;
  }

  if (start < strEnd) {
    atSkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->hdop, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->pdop, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->vdop, ','))
      return false;
  }

  if (start < strEnd) {
    atSkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &pdata->gpsSatInView, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &pdata->gnssSatInUse, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &pdata->gnssSatInView, ','))
      return false;
  }

  if (start < strEnd) {
    atSkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!atGetNextInt(&start, &pdata->cnomax, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->hpa, ','))
      return false;
  }

  if (start < strEnd) {
    if (!atGetNextDouble(&start, &pdata->vpa, '\r'))
      return false;
  }

  return true;
}

/****************************** END OF FILE **********************************/
