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
static bool SIM_notifyUrcHanler(SIM_Parser_t *parser)
{
  char urc[128] = {0};
  SIM_ParserGetUrc(parser, urc, sizeof(urc));

  bool result = false;
  if (URC_IsBtUrc(urc)) {
    BLT_ProcessUrc();
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
    
    memset(simp->rxbuf, 0, sizeof(simp->rxbuf));
    simp->rxlength = 0;

    SIM_ParserReset(&simp->parser);

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

      SIM_ParserProcessInput(&simp->parser, simp->rxbuf);

      bool isAt = false;
      if (SIM_ParserIsAtMessage(&simp->parser)) {
        if (simp->writer) {
          chThdResume(&simp->writer, MSG_OK);
          isAt = true;
        }
      }

      bool isUrc = false;
      if (SIM_ParserIsUrc(&simp->parser)) {
        isUrc = SIM_notifyUrcHanler(&simp->parser);
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
}

/****************************** END OF FILE **********************************/
