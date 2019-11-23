/**
 * @file sim8xxReaderThread.c
 * @brief SIM8xx reader thread.
 * @author Molnar Zoltan
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "sim8xxReaderThread.h"

#include "sim8xx.h"
#include "urc.h"
#include "source/Sdcard.h"
#include "source/Logger.h"
#include "source/BluetoothManagerThread.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define SIM_READER_LOGFILE "/sim8xx.log"

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
static void SIM_clearRxBuffer(Sim8xxDriver *simp)
{
  memset(simp->rxbuf, 0, sizeof(simp->rxbuf));
  simp->rxlength = 0;
  simp->at = simp->rxbuf;
  simp->atlength = 0;
  simp->urc = simp->rxbuf;
  simp->urclength = 0;
}

static bool SIM_beginsWithAT(const char *str)
{
  bool result = false;
  if (strlen("AT") < strlen(str))
    result = (0 == strncasecmp("AT", str, strlen("AT")));

  return result;
}

static bool SIM_isValidResponseStatus(char *msg)
{
  return !(SIM8XX_INVALID_STATUS == SIM_GetCommandStatus(msg));
}

static size_t SIM_checkEmbeddedAtResponse(char *str)
{
  size_t length = 0;
  char *bufEnd = str + strlen(str);

  bool found = false;
  char *crlf = strstr(str, CRLF);
  while (!found && crlf && (crlf + strlen(CRLF) < bufEnd)) {
    char *e = crlf + strlen(CRLF);
    char c = *e;
    *e = '\0';

    if (SIM_isValidResponseStatus(str)) {
      length = strlen(str);
      found = true;
    }
    
    *e = c;
    crlf = strstr(crlf + strlen(CRLF), CRLF);
  }

  return length;
}

static bool SIM_checkAndProcessAtResponse(Sim8xxDriver *simp)
{
#if 0
  simp->at = simp->rxbuf;
#endif
  simp->at = strcasestr(simp->rxbuf, "AT");
  if (!simp->at)
    return false;

  simp->atlength = 0;

  if (!SIM_beginsWithAT(simp->at))
    return false;

  bool result = false;

  if (SIM_isValidResponseStatus(simp->at)) {
    result = true;
    simp->atlength = strlen(simp->at);
  } else {
    size_t length = SIM_checkEmbeddedAtResponse(simp->rxbuf);
    if (0 < length) {
      result = true;
      simp->atlength = length;
    }
  }

  if (result && simp->writer) {
    chThdResume(&simp->writer, MSG_OK);
  }

  return result;
}

static bool SIM_beginsAndEndsWithCRLF(const char *msg)
{
  bool result = false;

  size_t length = strlen(msg);
  if (length < 2*strlen(CRLF))
    return false;

  if (('\r' == msg[0]) && 
      ('\n' == msg[1]) &&
      ('\r' == msg[length - 2]) && 
      ('\n' == msg[length - 1])) {
    result = true;
  }

  return result;
}

static bool SIM_notifyUrcListener(char urc[])
{
  bool result = false;
#if 0
  if (SIM_beginsWith(urc, "\r\n+cpin:") || 
      SIM_beginsWith(urc, "\r\ncall") ||
      SIM_beginsWith(urc, "\r\nsms")) {
    CLL_UrcReceived();
    result = true;
  } else {
    result = false;
  }
#endif

  if (URC_IsBtUrc(urc)) {
    BLT_ProcessUrc();
    result = true;
  }

  return result;
}

static bool SIM_checkAndProcessUrc(Sim8xxDriver *simp)
{
#if 0
  simp->urc = simp->rxbuf + simp->atlength;
  simp->urclength = 0;
#endif
  char *start = simp->rxbuf + simp->atlength;
  simp->urc = strstr(start, CRLF);
  if (!simp->urc)
    return false;

  char *end = strstr(simp->urc + strlen(CRLF), CRLF);  
  if (!end)
    return false;

  end += strlen(CRLF);
  char c = *end;
  *end = '\0';

  bool result = false;

  if (SIM_beginsAndEndsWithCRLF(simp->urc)) {
    simp->urclength = strlen(simp->urc);
    if (SIM_notifyUrcListener(simp->urc))  {
      result = true;
    } else {
      LOG_Write(SIM_READER_LOGFILE, "URC, but not expected!");
    }
  }

  *end = c;

  return result;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(SIM_ReaderThread, arg)
{
  Sim8xxDriver *simp = (Sim8xxDriver *)arg;

  event_listener_t serialListener;
  chEvtRegisterMaskWithFlags(chnGetEventSource(simp->config->sdp),
                             &serialListener,
                             EVENT_MASK(7),
                             CHN_INPUT_AVAILABLE);

  while (true) {
    chMtxLock(&simp->rxlock);
    SIM_clearRxBuffer(simp);


    bool isAt = false;
    bool isUrc = false;

    while(!isAt && !isUrc) {
      eventmask_t evt = chEvtWaitAny(EVENT_MASK(7));
      if (evt && EVENT_MASK(7)) {
        eventflags_t flags = chEvtGetAndClearFlags(&serialListener);
        if (flags & CHN_INPUT_AVAILABLE) {
          msg_t c;
          do {
            c = chnGetTimeout(simp->config->sdp, TIME_MS2I(20));
            if (c != STM_TIMEOUT) {
              simp->rxbuf[simp->rxlength] = (char)c;
              ++simp->rxlength;
            }
          } while ((c != STM_TIMEOUT) && (simp->rxlength <= sizeof(simp->rxbuf)));
        }
      }

      isAt = SIM_checkAndProcessAtResponse(simp);
      isUrc = SIM_checkAndProcessUrc(simp);
    }

    chMtxUnlock(&simp->rxlock);

#if 0
    LOG_Write(SIM_READER_LOGFILE, simp->rxbuf);
#endif

    if (isAt)
      chSemWait(&simp->atSync);

    if (isUrc)
      chSemWait(&simp->urcSync);
  
  }
}

/****************************** END OF FILE **********************************/
