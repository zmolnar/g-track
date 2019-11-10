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
#define GUARD_TIME_IN_MS 100
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
static virtual_timer_t guard_timer;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void SIM_timerCallback(void *p)
{
  Sim8xxDriver *simp = (Sim8xxDriver *)p;
  chSysLockFromISR();
  chSemResetI(&simp->guardSync, 1);
  chSysUnlockFromISR();
}
#if 0
static bool SIM_beginsWith(const char str[], const char pre[])
{
  return strncasecmp(pre, str, strlen(pre)) == 0;
}
#endif
static bool SIM_processResponse(Sim8xxDriver *simp)
{
  simp->next = simp->rxbuf;
  simp->atmsg = simp->rxbuf;
  char *lf, *end = simp->rxbuf + simp->rxlength;

  bool isAt = false;

  if (SIM8XX_INVALID_STATUS != SIM_GetCommandStatus(simp->atmsg)) {
    isAt = true;
    chThdResume(&simp->writer, MSG_OK);
    simp->next = simp->atmsg + strlen(simp->atmsg) + 1;
  } else {
    for (lf = strchr(simp->atmsg, '\n'); lf && (lf < end); lf = strchr(lf + 1, '\n')) {
      *lf = '\0';
      if (SIM8XX_INVALID_STATUS != SIM_GetCommandStatus(simp->atmsg)) {
        isAt = true;
        chThdResume(&simp->writer, MSG_OK);
        simp->next = lf + 1;
        break;
      } else {
        *lf = '\n';
      }
    }
  }

  return isAt;

#if 0

  if (0 == strcmp(simp->atmsg, "> ")) {
    isAt = true;
    simp->next = simp->atmsg + strlen(simp->atmsg);
    chThdResume(&simp->writer, MSG_OK);
  } else {
    for (lf = strchr(simp->atmsg, '\n'); lf && (lf < end); lf = strchr(lf + 1, '\n')) {
      *lf = '\0';
      if (SIM8XX_INVALID_STATUS != SIM_GetCommandStatus(simp->atmsg)) {
        isAt = true;
        chThdResume(&simp->writer, MSG_OK);
        simp->next = lf + 1;
        break;
      } else {
        *lf = '\n';
      }
    }
  }
#endif


#if 0
  if (SIM8XX_INVALID_STATUS == SIM_GetCommandStatus(simp->rxbuf))
    return false;

  chSysLock();

  if (simp->writer) {
    chThdResumeS(&simp->writer, MSG_OK);
  }

  chSysUnlock();

  return true;
#endif
}

static bool SIM_beginsAndEndsWithNewline(const char *msg, size_t length)
{
  bool result = false;

  if ((2 < length) && ('\n' == msg[length - 1]) && ('\n' == msg[0])) {
    result = true;
  }

  return result;
}

static bool SIM_processUrc(Sim8xxDriver *simp)
{
  bool isUrc = false;

  size_t length = strlen(simp->next);
  if (SIM_beginsAndEndsWithNewline(simp->next, length)) {
    simp->urc = simp->next + 1;
    *(simp->next + length - 1) = '\0';
    chSysLock();
    if (simp->urcprocessor) {
      chThdResumeS(&simp->urcprocessor, MSG_OK);
      chSysUnlock();
    } else {
      chSysUnlock();
      // TODO: Log missed URC
    }

    isUrc = true;
  }

  return isUrc;
}
#if 0
static bool SIM_processMessage(Sim8xxDriver *simp)
{
  return SIM_processResponse(simp) || SIM_processUrc(simp);
}
#endif
/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(SIM_ReaderThread, arg)
{
  Sim8xxDriver *simp = (Sim8xxDriver *)arg;
  event_listener_t serial_event_listener;

  chEvtRegisterMaskWithFlags(chnGetEventSource(simp->config->sdp),
                             &serial_event_listener,
                             EVENT_MASK(7),
                             CHN_INPUT_AVAILABLE);

  while (true) {
    chMtxLock(&simp->rxlock);
    memset(simp->rxbuf, 0, sizeof(simp->rxbuf));
    simp->rxlength = 0;

    eventmask_t evt = chEvtWaitAny(EVENT_MASK(7));

    if (evt && EVENT_MASK(7)) {
      eventflags_t flags = chEvtGetAndClearFlags(&serial_event_listener);
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

    chSysLock();
    chVTResetI(&guard_timer);
    chVTSetI(&guard_timer, TIME_MS2I(GUARD_TIME_IN_MS), SIM_timerCallback, simp);
    chSysUnlock();

    bool isAt = SIM_processResponse(simp);
    bool isUrc = SIM_processUrc(simp);

    LOG_Write(SIM_READER_LOGFILE, simp->rxbuf);

    if (isAt)
      chSemWait(&simp->atSync);

    if (isUrc)
      chSemWait(&simp->urcSync);

#if 0
    while (!SIM_processMessage(simp)) {
      eventmask_t evt = chEvtWaitAny(EVENT_MASK(7));
      
      if (evt && EVENT_MASK(7)) {
        eventflags_t flags = chEvtGetAndClearFlags(&serial_event_listener);
        if (flags & CHN_INPUT_AVAILABLE) {
          msg_t c;
          do {
            c = chnGetTimeout(simp->config->sdp, TIME_MS2I(5));
            if ((c != STM_TIMEOUT) && (c != '\r'))
              simp->rxbuf[simp->rxlength++] = (char)c;
          } while (c != STM_TIMEOUT);
        }
      }
    }

    LOG_Write(SIM_READER_LOGFILE, simp->rxbuf);

    chSysLock();
    chVTResetI(&guard_timer);
    chVTSetI(&guard_timer, TIME_MS2I(GUARD_TIME_IN_MS), SIM_timerCallback, simp);
    chMtxUnlockS(&simp->rxlock);
    chThdSuspendS(&simp->reader);
    simp->reader = NULL;
    chSysUnlock();
#endif    
  }
}

/****************************** END OF FILE **********************************/
