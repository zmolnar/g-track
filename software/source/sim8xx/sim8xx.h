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
#include "sim8xxUrcThread.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define CRLF    "\r\n"

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
  thread_reference_t urcprocessor;
  mutex_t lock;
  mutex_t rxlock;
  virtual_timer_t guardTimer;
  semaphore_t guardSync;
  semaphore_t atSync;
  semaphore_t urcSync;
  semaphore_t urcClear;
  char rxbuf[512];
  size_t rxlength;
  char urcbuf[512];
  char *at;
  size_t atlength;
  char *urc;
  size_t urclength;
  size_t processend;
} Sim8xxDriver;

typedef enum {
  SIM8XX_OK,
  SIM8XX_CONNECT,
  SIM8XX_RING,
  SIM8XX_NO_CARRIER,
  SIM8XX_ERROR,
  SIM8XX_NO_DIALTONE,
  SIM8XX_BUSY,
  SIM8XX_NO_ANSWER,
  SIM8XX_PROCEEDING,
  SIM8XX_TIMEOUT,
  SIM8XX_WAITING_FOR_INPUT,
  SIM8XX_SEND_OK,
  SIM8XX_SEND_FAIL,
  SIM8XX_INVALID_STATUS
} Sim8xxCommandStatus_t;

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
char *SIM_GetUrcMessage(Sim8xxDriver *simp);

/**
 *
 */
void SIM_ClearUrcMessage(Sim8xxDriver *simp);

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
