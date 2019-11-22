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
bool URC_IsBtUrc(const uint8_t str[])
{
    bool result = (URC_IsBtConnect(str) ||
                   URC_IsBtConnecting(str) || 
                   URC_IsBtSppData(str) ||
                   URC_IsBtDisconnect(str));

    return result;
}

bool URC_IsBtConnect(const uint8_t str[])
{
  return UTL_BeginsWith(str, "\r\n+BTCONNECT:");
}

bool URC_BtConnectParse(uint8_t str[], URC_BtConnect_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtConnect(str))
    return false;

  const uint8_t *end = str + strlen(str);

  uint8_t *start = strchr(str, ' ');
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

bool URC_IsBtDisconnect(const uint8_t str[])
{
  return UTL_BeginsWith(str, "\r\n+BTDISCONN:");
}

bool URC_BtDisconnectParse(uint8_t str[], URC_BtDisconnect_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtDisconnect(str))
    return false;

  const uint8_t *end = str + strlen(str);

  uint8_t *start = strchr(str, ' ');
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

bool URC_IsBtConnecting(const uint8_t str[])
{
  return UTL_BeginsWith(str, "\r\n+BTCONNECTING:");
}

bool URC_BtConnectingParse(uint8_t str[], URC_BtConnecting_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtConnecting(str))
    return false;

  const uint8_t *end = str + strlen(str);

  uint8_t *start = strchr(str, '"');
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

bool URC_IsBtSppData(const uint8_t str[])
{
  return UTL_BeginsWith(str, "\r\n+BTSPPDATA:");
}

bool URC_BtSppDataParse(uint8_t str[], URC_BtSppData_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtSppData(str))
    return false;

  const uint8_t *end = str + strlen(str);

  uint8_t *start = strchr(str, ' ');
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
