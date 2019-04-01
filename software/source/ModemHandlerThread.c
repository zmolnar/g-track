/**
 * @file ModemHandlerThread.c
 * @brief SIM8XX modem handler thread.
 * @author Molnar Zoltan
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "ModemHandlerThread.h"
#include "sim8xx/sim8xx.h"
#include "chprintf.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static Sim8xxDriver SIM8D1;

static SerialConfig serial_config = {115200,0,0,0};
static Sim8xxConfig sim_config = {&SD1, &serial_config};

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(ModemHandlerThread, arg) {
  (void)arg;
  chRegSetThreadName("modemhandler");

  sim8xxInit(&SIM8D1);
  sim8xxStart(&SIM8D1, &sim_config);

  palClearLine(LINE_WAVESHARE_POWER);
  chThdSleepMilliseconds(4000);
  palSetLine(LINE_WAVESHARE_POWER);
  
  while(true) {
    Sim8xxCommand cmd;
    sim8xxCommandInit(&cmd);
    chsnprintf(cmd.request, sizeof(cmd.request), "ati");
    sim8xxExecute(&SIM8D1,&cmd);
    chThdSleepMilliseconds(1000);
  }
} 

/******************************* END OF FILE ***********************************/

