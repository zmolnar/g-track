/**
 * @file Fifo.h
 * @brief
 */

#ifndef FIFO_H
#define FIFO_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <stddef.h>
#include <stdbool.h>

#include <ch.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define FIFO_LENGTH       1024

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef struct Fifo_s {
  mutex_t lock;
  char buffer[FIFO_LENGTH];
  size_t wrindex;
  size_t rdindex;
} Fifo_t;

typedef struct Fifo_Data_s {
  const char *data;
  size_t length;
} Fifo_Data_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
void FIFO_ObjectInit(Fifo_t *this);

bool FIFO_PushChar(Fifo_t *this, char c);

Fifo_Data_t FIFO_GetData(Fifo_t *this);

bool FIFO_PopData(Fifo_t *this, size_t length);

size_t FIFO_GetLength(Fifo_t *this);


#endif /* FIFO_H */

/****************************** END OF FILE **********************************/
