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

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/
#define GUARD_TIME_IN_MS               100

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static virtual_timer_t write_timer;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static bool process_status(Sim8xxDriver *simp, const char *s) {
  if(strcmp(s, "OK") &&
     strcmp(s, "CONNECT") &&
     strcmp(s, "RING") &&
     strcmp(s, "NO CARRIER") &&
     strcmp(s, "ERROR") &&
     strcmp(s, "NO DIALTONE") &&
     strcmp(s, "BUSY") &&
     strcmp(s, "NO ANSWER") &&
     strcmp(s, "PROCEEDING"))
    return false;

  if(simp->writer) {
    chSysLock();
    chThdResumeS(&simp->writer, MSG_OK);
    chSysUnlock();
  }
  
  return true;
}

static bool process_urc(Sim8xxDriver *simp, const char *s) {
  // send appropriate urc event
  (void)s;
  (void)simp;
  return false;
}

static bool process_message(Sim8xxDriver *simp) {
  char *data = simp->rxbuf;
  size_t length = simp->rxlength;

  if(length < 2)
    return false;

  if(('\r' != data[length-2]) || ('\n' != data[length-1]))
    return false;

  data[length-2] = '\0';

  static const char *crlf = "\r\n";
  
  char *s, *needle;
  for(s = needle = data; s; s = strstr(needle, crlf))
    needle = s + strlen(crlf);    

  bool result = process_status(simp, needle) || process_urc(simp, needle);

  data[length-2] = '\r';

  return result;
}

static void timer_cb(void *p) {
  Sim8xxDriver *simp = (Sim8xxDriver*)p;
  chSysLock();
  chSemSignalI(&simp->sync);
  chSysUnlock();
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(sim8xxReaderThread, arg) {
  Sim8xxDriver *simp = (Sim8xxDriver*)arg;
  event_listener_t serial_event_listener;
  event_source_t *pserial_event_source = chnGetEventSource(simp->config->sdp);
  
  chEvtRegisterMaskWithFlags(pserial_event_source,
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
    
    chSysLock();
    chVTSetI(&write_timer, TIME_MS2I(GUARD_TIME_IN_MS), timer_cb, simp);
    chMtxUnlockS(&simp->rxlock);
    chThdSuspendS(&simp->reader);
    chSysUnlock();      
  }
}

/******************************* END OF FILE ***********************************/

