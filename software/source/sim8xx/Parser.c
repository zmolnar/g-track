/**
 * @file parser.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Parser.h"
#include <string.h>

#include <stdio.h>

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
#if defined(TEST)
static void printParser(SIM_Parser_t *parser)
{
    static const char *state[] = {
      [START]             = "START",   
      [ENTER_AT]          = "ENTER_AT",      
      [AT]                = "AT",
      [ENTER_STATUS]      = "ENTER_STATUS",          
      [STATUS]            = "STATUS",    
      [EXIT_STATUS]       = "EXIT_STATUS",         
      [USER_INPUT]        = "USER_INPUT",        
      [ENTER_SEND_STATUS] = "ENTER_SEND_STATUS",               
      [SEND_STATUS]       = "SEND_STATUS",         
      [EXIT_SEND_STATUS]  = "EXIT_SEND_STATUS",              
      [ENTER_URC]         = "ENTER_URC",       
      [URC]               = "URC", 
      [EXIT_URC]          = "EXIT_URC",      
      [FINISHED]          = "FINISHED",      
      [ERROR]             = "ERROR",   
    };
    
    printf("state:   %s\n", state[parser->state]);
    printf("input:       %s\n", parser->input);
    printf("atstart:     %d\n", parser->atstart);
    printf("statusstart: %d\n", parser->statusstart);
    printf("statusend:   %d\n", parser->statusend);
    printf("atend:       %d\n", parser->atend);
    printf("urcstart:    %d\n", parser->urcstart);
    printf("urcend:      %d\n\n", parser->urcend);
}
#endif

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void SIM_ParserReset(SIM_Parser_t *parser)
{
    memset(parser, 0, sizeof(*parser));
}

void SIM_ParserProcessInput(SIM_Parser_t *parser, const char *input)
{
  SIM_ParserReset(parser);

  parser->input = input;
  parser->state = START;

  size_t i = 0;
  while ((FINISHED != parser->state) && (ERROR != parser->state)) {
    char c = parser->input[i];

    switch (parser->state) {

    case START: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        switch(c) {
          case 'A':
          case 'a': {
            parser->state = ENTER_AT;
            break;
          }
          case '\r': {
            parser->state = ENTER_URC;
            break;
          }
          default: {
            parser->state = USER_INPUT;
            parser->atstart = i;
            break;
          }
        }
      }
      break;
    }

    case ENTER_AT: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        switch(c) {
          case 'T':
          case 't': {
            parser->state = AT;
            parser->atstart = i - 1;
            break;
          }
          default: {
            parser->state = USER_INPUT;
            break;
          }
        }
      }
      break;
    }

    case AT: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if ('\r' == c) {
          parser->state = ENTER_STATUS;
        } else {
          parser->state = AT;
        }
      }
      break;
    }

    case ENTER_STATUS: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        switch(c) {
          case '\n': {
            parser->state = STATUS;
            parser->statusstart = i + 1;
            break;
          }
          case '\r': {
            parser->state = ENTER_STATUS;
            break;
          }
          default: {
            parser->state = AT;
            break;
          }
        }
      }
      break;
    }

    case STATUS: {
      if ('\r' == c) {
        parser->state = EXIT_STATUS;
        parser->statusend = i;
      } else if ('\0' == c) {
        parser->state = FINISHED;
        parser->statusend = i;
        parser->atend = i;
      } else {
        parser->state = STATUS;
      }
      break;
    }

    case EXIT_STATUS: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if (('\n' == c) && SIM_ParserIsValidStatus(parser)) {
          parser->state = START;
          parser->atend = i + 1;
        } else {
          parser->state = AT;
          parser->statusstart = 0;
          parser->statusend = 0;
        }
      }
      break;
    }

    case USER_INPUT: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if ('\r' == c) {
          parser->state = ENTER_SEND_STATUS;
        } else {
          parser->state = USER_INPUT;
        }
      }
      break;
    }

    case ENTER_SEND_STATUS: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if ('\n' == c) {
          parser->state = SEND_STATUS;
          parser->statusstart = i + 1;
        } else {
          parser->state = USER_INPUT;
        }
      }
      break;
    }

    case SEND_STATUS: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if ('\r' == c) {
          parser->state = EXIT_SEND_STATUS;
          parser->statusend = i;
        } else {
          parser->state = SEND_STATUS;
        }
      }
      break;
    }

    case EXIT_SEND_STATUS: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if (('\n' == c) && SIM_ParserIsValidStatus(parser)) {
            parser->state = START;
            parser->atend = i + 1;
        } else {
          parser->state = USER_INPUT;
        }
      }
      break;
    }

    case ENTER_URC: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if ('\n' == c) {
          parser->state = URC;
          parser->urcstart = i - 1;
        } else {
          parser->state = USER_INPUT;
        }
      }
      break;
    }

    case URC: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if ('\r' == c) {
          parser->state = EXIT_URC;
        } else {
          parser->state = URC;
        }
      }
      break;
    }

    case EXIT_URC: {
      if ('\0' == c) {
        parser->state = FINISHED;
      } else {
        if ('\n' == c) {
          parser->state = START;
          parser->urcend = i + 1;
        } else {
          parser->state = URC;
        }
      }
      break;
    }

    default: {
      parser->state = ERROR;
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
