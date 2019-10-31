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
#define GUARD_TIME_IN_MS 250
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
static bool SIM_processResponse(Sim8xxDriver *simp)
{
  if (SIM8XX_INVALID_STATUS == SIM_GetCommandStatus(simp->rxbuf))
    return false;

  chSysLock();
  if (simp->writer) {
    chThdResumeS(&simp->writer, MSG_OK);
  }

  chSysUnlock();

  return true;
}

static bool SIM_processUrc(Sim8xxDriver *simp)
{
  // send appropriate urc event
  (void)simp;
  return false;
}

static bool SIM_processMessage(Sim8xxDriver *simp)
{
  return SIM_processResponse(simp) || SIM_processUrc(simp);
}

static void SIM_timerCallback(void *p)
{
  Sim8xxDriver *simp = (Sim8xxDriver *)p;
  chSysLockFromISR();
  chSemSignalI(&simp->sync);
  chSysUnlockFromISR();
}

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

    while (!SIM_processMessage(simp)) {
      eventmask_t evt = chEvtWaitAny(EVENT_MASK(7));
      if (evt && EVENT_MASK(7)) {
        eventflags_t flags = chEvtGetAndClearFlags(&serial_event_listener);
        if (flags & CHN_INPUT_AVAILABLE) {
          msg_t c;
          do {
            c = chnGetTimeout(simp->config->sdp, TIME_IMMEDIATE);
            if (c != STM_TIMEOUT)
              simp->rxbuf[simp->rxlength++] = (char)c;
          } while (c != STM_TIMEOUT);
        }
      }
    }

    LOG_Write(SIM_READER_LOGFILE, simp->rxbuf);

    chSysLock();
    chVTSetI(&guard_timer, TIME_MS2I(GUARD_TIME_IN_MS), SIM_timerCallback, simp);
    chMtxUnlockS(&simp->rxlock);
    chThdSuspendS(&simp->reader);
    simp->reader = NULL;
    chSysUnlock();
  }
}

/****************************** END OF FILE **********************************/
