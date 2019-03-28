/**
 * @file sim8xx.h
 * @brief SIM8xx modem driver.
 * @author Molnar Zoltan
*/

#ifndef SIM8XX_H
#define SIM8XX_H

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "ch.h"
#include "hal.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/
typedef enum {
  SIM8XX_UNINIT = 0,
  SIM8XX_STOP = 1,
  SIM8XX_READY = 2,
} sim8xxstate_t;

typedef struct Sim8xxConfig {
  SerialDriver *sdp;
  SerialConfig *sdConfig;
} Sim8xxConfig;

typedef struct Sim8xxDriver {
  sim8xxstate_t state;
  const Sim8xxConfig *config;
  BaseSequentialStream *stream;
  thread_reference_t writer;
  thread_reference_t reader;
  mutex_t lock;
  mutex_t rxlock;
  semaphore_t sync;
  char rxbuf[512];
  size_t rxlength;

  char *needle;
} Sim8xxDriver;

typedef struct Sim8xxCommand {
  char request[512];
  char response[512];
} Sim8xxCommand;

/*******************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                             */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                             */
/*******************************************************************************/
void sim8xxInit(Sim8xxDriver *simp);
void sim8xxStart(Sim8xxDriver *simp, Sim8xxConfig *cfgp);
void sim8xxCommandInit(Sim8xxCommand *cmdp);
void sim8xxExecute(Sim8xxDriver *simp, Sim8xxCommand *cmdp);

#endif

/******************************* END OF FILE ***********************************/
