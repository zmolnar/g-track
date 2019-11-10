/**
 * @file AtCgnsinf.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "AtCgnsinf.h"

#include "utils/Utils.h"

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
bool AT_CgnsinfCreate(char buf[], size_t length)
{
  strncpy(buf, "AT+CGNSINF", length);
  return true;
}

bool AT_CgnsinfParse(CGNSINF_Response_t *pdata, char str[])
{
  memset(pdata, 0, sizeof(*pdata));

  const char *strEnd = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start)
    return false;

  ++start;

  if (start < strEnd) {
    if (!UTL_GetNextInt(&start, &pdata->runStatus, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextInt(&start, &pdata->fixStatus, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextString(&start, pdata->date, sizeof(pdata->date), ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->latitude, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->longitude, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->altitude, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->speed, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->course, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextInt(&start, &pdata->fixMode, ','))
      return false;
  }

  if (start < strEnd) {
    UTL_SkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->hdop, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->pdop, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->vdop, ','))
      return false;
  }

  if (start < strEnd) {
    UTL_SkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!UTL_GetNextInt(&start, &pdata->gpsSatInView, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextInt(&start, &pdata->gnssSatInUse, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextInt(&start, &pdata->gnssSatInView, ','))
      return false;
  }

  if (start < strEnd) {
    UTL_SkipReserved(&start, 1, ',');
  }

  if (start < strEnd) {
    if (!UTL_GetNextInt(&start, &pdata->cnomax, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->hpa, ','))
      return false;
  }

  if (start < strEnd) {
    if (!UTL_GetNextDouble(&start, &pdata->vpa, '\n'))
      return false;
  }

  return true;
}

/****************************** END OF FILE **********************************/
