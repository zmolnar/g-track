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
#include "source/Sdcard.h"
#include "source/Logger.h"

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
  simp->processend = simp->rxbuf;
}

static bool SIM_isValidResponseStatus(uint8_t *msg)
{
  return !(SIM8XX_INVALID_STATUS == SIM_GetCommandStatus(msg));
}

static size_t SIM_checkEmbeddedAtResponse(Sim8xxDriver *simp)
{
  size_t end = 0;
  uint8_t *bufEnd = simp->rxbuf + simp->rxlength;

  bool found = false;
  uint8_t *crlf = strstr(simp->atmsg, CRLF);
  while (!found && crlf && (crlf + strlen(CRLF) <= bufEnd)) {
    uint8_t *e = crlf + strlen(CRLF);
    *e = '\0';

    if (SIM_isValidResponseStatus(simp->atmsg)) {
      end = strlen(simp->atmsg);
      found = true;
    }
    
    *e = '\r';
    crlf = strstr(crlf + strlen(CRLF), CRLF);
  }

  return end;
}

static bool SIM_beginsWithAT(const uint8_t *str)
{
  bool result = false;
  if (strlen("AT") < strlen(str)) {
    result = (0 == strncasecmp("AT", str, strlen("AT")));

  return result;
}

static bool SIM_checkAndProcessAtResponse(Sim8xxDriver *simp)
{
  simp->at = simp->rxbuf;
  simp->atlength = 0;
  simp->processend = 0;

  if (!SIM_beginsWithAT(simp->at))
    return false;

  bool result = false;

  if (SIM_isValidResponseStatus(simp->atmsg)) {
    result = true;
    simp->atlength = strlen(simp->atmsg);
    simp->processend = simp->atlength + 1;
  } else {
    size_t length = SIM_checkEmbeddedAtResponse(simp);
    if (0 < length) {
      result = true;
      simp->atlength = length;
      simp->processend = simp->atlength + 1;
    }
  }

  if (result && simp->writer) {
    chThdResume(&simp->writer, MSG_OK);
  }

  return result;
}

static bool SIM_beginsAndEndsWithCRLF(const uint8_t *msg)
{
  bool result = false;

  size_t length = strlen(msg);
  if (length < 2*strlen(CRLF))
    return false;

  if (('r' == msg[0]) && 
      ('\n' == msg[1]) &&
      ('\r' == msg[length - 2]) && 
      ('\n' == msg[length - 1])) {
    result = true;
  }

  return result;
}

static bool SIM_notifyUrcListener(uint8_t urc[])
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
  simp->urc = simp->rxbuf + simp->processend;
  simp->urclength = 0;

  bool result = false;

  if (SIM_beginsAndEndsWithCRLF(simp->urc)) {
    simp->urclength = simp->rxlength - simp->processend;
    if (SIM_notifyUrcListener(simp->urc))  {
      result = true;
    } else {
      LOG_Write(SIM_READER_LOGFILE, "URC, but not expected!");
    }
  }

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

    eventmask_t evt = chEvtWaitAny(EVENT_MASK(7));

    if (evt && EVENT_MASK(7)) {
      eventflags_t flags = chEvtGetAndClearFlags(&serialListener);
      if (flags & CHN_INPUT_AVAILABLE) {
        msg_t c;
        do {
          c = chnGetTimeout(simp->config->sdp, TIME_MS2I(5));
          if (c != STM_TIMEOUT)
            simp->rxbuf[simp->rxlength++] = (uint8_t)c;
        } while ((c != STM_TIMEOUT) && (simp->rxlength < sizeof(simp->rxbuf)));
      }
    }

    bool isAt = SIM_checkAndProcessAtResponse(simp);
    bool isUrc = SIM_checkAndProcessUrc(simp);

    chMtxUnlock(&simp->rxlock);

    //LOG_Write(SIM_READER_LOGFILE, simp->rxbuf);

    if (isAt)
      chSemWait(&simp->atSync);

    if (isUrc)
      chSemWait(&simp->urcSync);
  
  }
}

/****************************** END OF FILE **********************************/
