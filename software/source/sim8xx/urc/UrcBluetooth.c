/**
 * @file UrcBluetooth.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "UrcBluetooth.h"
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
bool URC_IsBtUrc(const char str[])
{
    bool result = (URC_IsBtConnect(str) ||
                   URC_IsBtConnecting(str) || 
                   URC_IsBtSppData(str) ||
                   URC_IsBtDisconnect(str));

    return result;
}

bool URC_IsBtConnect(const char str[])
{
  return UTL_BeginsWith(str, "\r\n+BTCONNECT:");
}

bool URC_BtConnectParse(char str[], URC_BtConnect_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtConnect(str))
    return false;

  const char *end = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start)
    return false;

  start += strlen(" ");

  if (start < end) {
    if (!UTL_GetNextInt(&start, &urc->id, ','))
      return false;
  }

  start += strlen("\"");

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->name, '"'))
      return false;
  }

  start += strlen(",");

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->address, ','))
      return false;
  }

  start += strlen("\"");

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->profile, '"'))
      return false;
  }

  return true;
}

bool URC_IsBtDisconnect(const char str[])
{
  return UTL_BeginsWith(str, "\r\n+BTDISCONN:");
}

bool URC_BtDisconnectParse(char str[], URC_BtDisconnect_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtDisconnect(str))
    return false;

  const char *end = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start)
    return false;

  start += strlen(" ");

  start += strlen("\"");

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->name, '"'))
      return false;
  }

  start += strlen(",");

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->address, ','))
      return false;
  }

  start += strlen("\"");

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->profile, '"'))
      return false;
  }

  return true;
}

bool URC_IsBtConnecting(const char str[])
{
  return UTL_BeginsWith(str, "\r\n+BTCONNECTING:");
}

bool URC_BtConnectingParse(char str[], URC_BtConnecting_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtConnecting(str))
    return false;

  const char *end = str + strlen(str);

  char *start = strchr(str, '"');
  if (!start)
    return false;

  start += strlen("\"");

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->address, '"'))
      return false;
  }

  start = strchr(str, '"');
  if (!start)
    return false;

  start += strlen("\"");

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->profile, '"'))
      return false;
  }

  return true;
}

bool URC_IsBtSppData(const char str[])
{
  return UTL_BeginsWith(str, "\r\n+BTSPPDATA:");
}

bool URC_BtSppDataParse(char str[], URC_BtSppData_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtSppData(str))
    return false;

  const char *end = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start)
    return false;

  start += strlen(" ");

  if (start < end) {
    if (!UTL_GetNextInt(&start, &urc->id, ','))
      return false;
  }

  if (start < end) {
    if (!UTL_GetNextInt(&start, &urc->length, ','))
      return false;
  }

  if (start < end) {
    if (!UTL_GetNextString(&start, &urc->data, '\r'))
      return false;
  }

  return true;
}

/****************************** END OF FILE **********************************/
