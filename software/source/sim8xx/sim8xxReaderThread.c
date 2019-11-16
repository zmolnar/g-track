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
  simp->atmsg = simp->rxbuf;
  simp->urc = simp->rxbuf;
  simp->next = 0;
}

static bool SIM_isValidResponseStatus(char *msg)
{
  return !(SIM8XX_INVALID_STATUS == SIM_GetCommandStatus(msg));
}

static size_t SIM_checkEmbeddedAtResponse(Sim8xxDriver *simp)
{
  size_t end = 0;
  char *bufEnd = simp->rxbuf + simp->rxlength;

  bool found = false;
  char *lf = strchr(simp->atmsg, '\n');
  while (!found && lf && (lf < bufEnd)) {
    *lf = '\0';

    if (SIM_isValidResponseStatus(simp->atmsg)) {
      end = strlen(simp->atmsg);
      found = true;
    }
    
    *lf = '\n';
    lf = strchr(lf + 1, '\n');
  }

  return end;
}

static bool SIM_checkAndSetAtResponse(Sim8xxDriver *simp)
{
  simp->atmsg = &simp->rxbuf[simp->next];

  bool result = false;

  if (SIM_isValidResponseStatus(simp->atmsg)) {
    result = true;
    simp->next = strlen(simp->atmsg) + 1;
  } else {
    size_t end = SIM_checkEmbeddedAtResponse(simp);
    if (0 < end) {
      result = true;
      simp->rxbuf[end] = '\0';
      simp->next = end + 1;
    }
  }

  if (result && simp->writer) {
    chThdResume(&simp->writer, MSG_OK);
  }

  return result;
}

static bool SIM_beginsAndEndsWithNewline(const char *msg)
{
  bool result = false;

  size_t length = strlen(msg);
  if ((2 < length) && ('\n' == msg[length - 1]) && ('\n' == msg[0])) {
    result = true;
  }

  return result;
}

static char *SIM_removeFirstChar(char *str)
{
  return str + 1;
}

static void SIM_removeLastChar(char *str)
{
  str[strlen(str) - 1] = '\0';
}

static bool SIM_checkAndSetUrc(Sim8xxDriver *simp)
{
  simp->urc = &simp->rxbuf[simp->next];

  bool result = false;

  if (SIM_beginsAndEndsWithNewline(simp->urc)) {
    simp->urc = SIM_removeFirstChar(simp->urc);
    SIM_removeLastChar(simp->urc);

    if (simp->urcprocessor) {
      chThdResume(&simp->urcprocessor, MSG_OK);
    } else {
      LOG_Write(SIM_READER_LOGFILE, "URC, but processor thread is busy!");
    }

    result = true;
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
          if ((c != STM_TIMEOUT) && (c != '\r'))
            simp->rxbuf[simp->rxlength++] = (char)c;
        } while (c != STM_TIMEOUT);
      }
    }

    chMtxUnlock(&simp->rxlock);

    bool isAt = SIM_checkAndSetAtResponse(simp);
    bool isUrc = SIM_checkAndSetUrc(simp);

    if (!isAt && !isUrc)
      LOG_Write(SIM_READER_LOGFILE, "Neither AT, nor URC!");

    LOG_Write(SIM_READER_LOGFILE, simp->rxbuf);

    if (isAt)
      chSemWait(&simp->atSync);

    if (isUrc)
      chSemWait(&simp->urcSync);
  
  }
}

/****************************** END OF FILE **********************************/
