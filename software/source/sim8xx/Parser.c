/**
 * @file parser.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Parser.h"
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

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

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void SIM_ParserReset(SIM_Parser_t *parser)
{
    memset(parser, 0, sizeof(*parser));
}

void SIM_ParserProcessInput(SIM_Parser_t *parser, const char *input)
{
  parser->input = input;
  parser->state = SIM_PARSER_BEGIN;

  size_t i = 0;
  while (SIM_PARSER_FINISHED != parser->state)
  {
    char c = input[i];

    switch (parser->state)
    {

    case SIM_PARSER_BEGIN:
    {
      switch (c)
      {
      case 'A':
      case 'a':
      {
        parser->state = SIM_PARSER_ENTER_AT;
        break;
      }
      case '\r':
      {
        parser->state = SIM_PARSER_ENTER_URC;
        break;
      }
      case '\0': {
        parser->state = SIM_PARSER_FINISHED;
        break;
      }
      default:
      {
        parser->state = SIM_PARSER_BEGIN;
        break;
      }
      }
      break;
    }

    case SIM_PARSER_ENTER_AT: {
      if (('T' == c) || ('t' == c)) {
        parser->state = SIM_PARSER_AT;
        parser->atstart = i - 1;
      } else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      else 
        parser->state= SIM_PARSER_BEGIN;

      break;
    }

    case SIM_PARSER_AT: {
      if ('\r' == c)
        parser->state= SIM_PARSER_ENTER_STATUS;
      else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      break;
    }

    case SIM_PARSER_ENTER_STATUS: {
      if ('\n' == c) {
        parser->state = SIM_PARSER_STATUS;
        parser->statusstart = i + 1;
      } else if ('>' == c) {
        parser->state = SIM_PARSER_ENTER_WAIT_USER_INPUT;
      }else if ('\r' == c) {
        parser->state = SIM_PARSER_ENTER_STATUS;
      } else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      else 
        parser->state = SIM_PARSER_AT;

      break;
    }

    case SIM_PARSER_STATUS: {
      if ('\r' == c)
        parser->state = SIM_PARSER_EXIT_AT;
      else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      break;
    }

    case SIM_PARSER_ENTER_WAIT_USER_INPUT: {
      if (' ' == c)
        parser->state = SIM_PARSER_USER_INPUT;
      else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      else 
        parser->state = SIM_PARSER_AT;
      break;
    }

    case SIM_PARSER_USER_INPUT: {
      if ('\r' == c)
        parser->state = SIM_PARSER_EXIT_AT;
      else if ('\0' == c) {
        parser->statusstart = i - 2;
        parser->statusend = i;
        parser->atend = i;
        parser->state = SIM_PARSER_FINISHED;
      }
      break;
    }

    case SIM_PARSER_EXIT_AT: {
      if ('\n' == c) {
        parser->statusend = i - 1;
        if (SIM_ParserIsValidStatus(parser)) {
          parser->state = SIM_PARSER_BEGIN;
          parser->atend = i + 1;
        } else {
          parser->statusend = 0;
          parser->state = SIM_PARSER_AT; 
        }
      } else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      else
        parser->state = SIM_PARSER_STATUS;
      break;
    }

    case SIM_PARSER_ENTER_URC: {
      if ('\n' == c) {
        parser->state = SIM_PARSER_URC;
        parser->urcstart = i - 1;
      } else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      else 
        parser->state = SIM_PARSER_BEGIN;
      break;
    }

    case SIM_PARSER_URC: {
      if ('\r' == c)
        parser->state = SIM_PARSER_EXIT_URC;
      else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      break;
    }

    case SIM_PARSER_EXIT_URC: {
      if ('\n' == c) {
        parser->state = SIM_PARSER_BEGIN;
        parser->urcend = i + 1;
      } else if ('\0' == c) {
        parser->state = SIM_PARSER_FINISHED;
      }
      break;
    }

    default: {
      parser->state = SIM_PARSER_FINISHED;
      break;
    }
    }
    
    ++i;
  }
}

bool SIM_ParserIsAtMessage(SIM_Parser_t *parser)
{
    return parser->atstart < parser->atend;
}

bool SIM_ParserIsUrc(SIM_Parser_t *parser)
{
    return parser->urcstart < parser->urcend;
}

bool SIM_ParserGetAtMessage(SIM_Parser_t *parser, char *buf, size_t length)
{
    memset(buf, 0, length);

    bool result = false;
    if (SIM_ParserIsAtMessage(parser)) {
        size_t n = parser->atend - parser->atstart;
        n = (length < n) ? length : n;
        memcpy(buf, parser->input + parser->atstart, n);
        result = true;
    }

    return result;
}

bool SIM_ParserIsValidStatus(SIM_Parser_t *parser)
{
  return SIM8XX_INVALID_STATUS != SIM_ParserGetStatus(parser);
}

Sim8xxCommandStatus_t SIM_ParserGetStatus(SIM_Parser_t *parser)
{
  char status[16] = {0};
  if (parser->statusend <= parser->statusstart)
    return SIM8XX_INVALID_STATUS;

  size_t length   = parser->statusend - parser->statusstart;
  length = (length < sizeof(status)) ? length : sizeof(status);
  memcpy(status, parser->input + parser->statusstart, length);

  Sim8xxCommandStatus_t result;

  if (0 == strcmp(status, "OK"))
    result = SIM8XX_OK;
  else if (0 == strcmp(status, "> "))
    result = SIM8XX_WAITING_FOR_INPUT;
  else if (0 == strcmp(status, "CONNECT"))
    result = SIM8XX_CONNECT;
  else if (0 == strcmp(status, "SEND OK"))
    result = SIM8XX_SEND_OK;
  else if (0 == strcmp(status, "SEND FAIL"))
    result = SIM8XX_SEND_FAIL;
  else if (0 == strcmp(status, "RING"))
    result = SIM8XX_RING;
  else if (0 == strcmp(status, "NO CARRIER"))
    result = SIM8XX_NO_CARRIER;
  else if (0 == strcmp(status, "ERROR"))
    result = SIM8XX_ERROR;
  else if (0 == strcmp(status, "NO DIALTONE"))
    result = SIM8XX_NO_DIALTONE;
  else if (0 == strcmp(status, "BUSY"))
    result = SIM8XX_BUSY;
  else if (0 == strcmp(status, "NO ANSWER"))
    result = SIM8XX_NO_ANSWER;
  else if (0 == strcmp(status, "PROCEEDING"))
    result = SIM8XX_PROCEEDING;
  else
    result = SIM8XX_INVALID_STATUS;

  return result;
}

bool SIM_ParserGetUrc(SIM_Parser_t *parser, char *buf, size_t length)
{
  memset(buf, 0, length);

  bool result = false;
  if (SIM_ParserIsUrc(parser)) {
    size_t n = parser->urcend - parser->urcstart;
    n        = (length < n) ? length : n;
    memcpy(buf, parser->input + parser->urcstart, n);
    result = true;
  }

  return result;
}

/****************************** END OF FILE **********************************/
