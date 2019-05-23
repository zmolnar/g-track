/**
 * @file sim8xxReaderThread.c
 * @brief SIM8xx reader thread.
 * @author Molnar Zoltan
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "sim8xx.h"
#include "sim8xxReaderThread.h"
#include <string.h>
#include "source/Sdcard.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/
#define GUARD_TIME_IN_MS               250

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static virtual_timer_t guard_timer;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static void save_buffer(const char *data, size_t length) {
  FIL log;
  if (FR_OK == f_open(&log, "/sim8xx_at.log", FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&log, data, length, &bw);
    f_close(&log);
  }
}

static bool process_response(Sim8xxDriver *simp) {
  if (SIM8XX_INVALID_STATUS == sim8xxGetStatus(simp->rxbuf))
    return false;

  chSysLock();
  if (simp->writer) {
    chThdResumeS(&simp->writer, MSG_OK);
  }
  
  chSysUnlock();
  
  return true;
}

static bool process_urc(Sim8xxDriver *simp) {
  // send appropriate urc event
  (void)simp;
  return false;
}

static bool process_message(Sim8xxDriver *simp) {
  return process_response(simp) || process_urc(simp);
}

static void timer_cb(void *p) {
  Sim8xxDriver *simp = (Sim8xxDriver*)p;
  chSysLockFromISR();
  chSemSignalI(&simp->sync);
  chSysUnlockFromISR();
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(sim8xxReaderThread, arg) {
  Sim8xxDriver *simp = (Sim8xxDriver*)arg;
  event_listener_t serial_event_listener;
  
  chEvtRegisterMaskWithFlags(chnGetEventSource(simp->config->sdp),
                             &serial_event_listener,
                             EVENT_MASK(7),
                             CHN_INPUT_AVAILABLE);

  while(true) {
    chMtxLock(&simp->rxlock);
    memset(simp->rxbuf, 0, sizeof(simp->rxbuf));
    simp->rxlength = 0;
    
    while(!process_message(simp)) {
      eventmask_t evt = chEvtWaitAny(EVENT_MASK(7));
      if(evt && EVENT_MASK(7)) {
        eventflags_t flags = chEvtGetAndClearFlags(&serial_event_listener);
        if(flags & CHN_INPUT_AVAILABLE) {
          msg_t c;
          do {
            c = chnGetTimeout(simp->config->sdp, TIME_IMMEDIATE);
            if ( c != STM_TIMEOUT )
              simp->rxbuf[simp->rxlength++] = (char)c;
          }
          while (c != STM_TIMEOUT);
        }
      }
    }
    
    save_buffer(simp->rxbuf, simp->rxlength);
    
    chSysLock();
    chVTSetI(&guard_timer, TIME_MS2I(GUARD_TIME_IN_MS), timer_cb, simp);
    chMtxUnlockS(&simp->rxlock);
    chThdSuspendS(&simp->reader);
    simp->reader = NULL;
    chSysUnlock();   
  }
}

/******************************* END OF FILE ***********************************/
