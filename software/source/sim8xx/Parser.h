/**
 * @file Parser.h
 * @brief
 */

#ifndef PARSER_H
#define PARSER_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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
  START,
  ENTER_AT,
  AT,
  ENTER_STATUS,
  STATUS,
  EXIT_STATUS,
  USER_INPUT,
  ENTER_SEND_STATUS,
  SEND_STATUS,
  EXIT_SEND_STATUS,
  ENTER_URC,
  URC,
  EXIT_URC,
  FINISHED,
} SIM_ParserState_t;

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
  SIM8XX_BUFFER_OVERFLOW,
  SIM8XX_INVALID_STATUS
} Sim8xxCommandStatus_t;

typedef struct SIM_Parser_s {
  const char *input;
  SIM_ParserState_t state;
  size_t atstart;
  size_t atend;
  size_t statusstart;
  size_t statusend;
  size_t urcstart;
  size_t urcend;
} SIM_Parser_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
/**
 * 
 */
void SIM_ParserReset(SIM_Parser_t *parser);
void SIM_ParserProcessInput(SIM_Parser_t *parser, const char *input);
bool SIM_ParserIsAtMessage(SIM_Parser_t *parser);
bool SIM_ParserIsUrc(SIM_Parser_t *parser);
bool SIM_ParserGetAtMessage(SIM_Parser_t *parser, char *buf, size_t length);
bool SIM_ParserIsValidStatus(SIM_Parser_t *parser);
Sim8xxCommandStatus_t SIM_ParserGetStatus(SIM_Parser_t *parser);
bool SIM_ParserGetUrc(SIM_Parser_t *parser, char *buf, size_t length);

#endif /* PARSER_H */

/****************************** END OF FILE **********************************/
