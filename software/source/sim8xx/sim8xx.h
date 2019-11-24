/**
 * @file sim8xx.h
 * @brief SIM8xx modem driver.
 * @author Molnar Zoltan
 */

#ifndef SIM8XX_H
#define SIM8XX_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"
#include "hal.h"
#include "Parser.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  SIM8XX_UNINIT = 0,
  SIM8XX_STOP   = 1,
  SIM8XX_READY  = 2,
} sim8xxstate_t;

typedef struct Sim8xxConfig {
  SerialDriver *sdp;
  SerialConfig *sdConfig;
  ioline_t powerline;
} Sim8xxConfig;

typedef struct Sim8xxDriver {
  sim8xxstate_t state;
  const Sim8xxConfig *config;
  thread_reference_t writer;
  thread_reference_t reader;
  mutex_t lock;
  mutex_t rxlock;
  virtual_timer_t guardTimer;
  semaphore_t guardSync;
  semaphore_t atSync;
  semaphore_t urcSync;
  char rxbuf[512];
  size_t rxlength;
  SIM_Parser_t parser;
} Sim8xxDriver;

typedef struct Sim8xxCommand {
  char request[128];
  char data[128];
  char response[512];
  Sim8xxCommandStatus_t status;
} Sim8xxCommand;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/
extern Sim8xxDriver SIM8D1;

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
/**
 * 
 */
void SIM_Init(Sim8xxDriver *simp);

/**
 * 
 */
void SIM_Start(Sim8xxDriver *simp, Sim8xxConfig *cfgp);

/**
 * 
 */
void SIM_CommandInit(Sim8xxCommand *cmdp);

/**
 * 
 */
void SIM_ExecuteCommand(Sim8xxDriver *simp, Sim8xxCommand *cmdp);

/**
 *
 */
size_t SIM_GetAndClearUrc(Sim8xxDriver *simp, char *urc, size_t length);

/**
 * 
 */
bool SIM_IsConnected(Sim8xxDriver *simp);

/**
 * 
 */
void SIM_TogglePower(Sim8xxDriver *simp);

/**
 * 
 */
Sim8xxCommandStatus_t SIM_GetCommandStatus(char *data);

#endif

/****************************** END OF FILE **********************************/
