/**
 * @file Fifo.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Fifo.h"
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
void FIFO_ObjectInit(Fifo_t *this)
{
  memset(this, 0, sizeof(*this));
  chMtxObjectInit(&this->lock);
}

bool FIFO_PushChar(Fifo_t *this, char c)
{
  chMtxLock(&this->lock);
  
  if (this->rdindex == this->wrindex) {
    this->rdindex = 0;
    this->wrindex = 0;
  }

  bool result = false;
  if (this->wrindex < FIFO_LENGTH) {
    this->buffer[this->wrindex] = c;
    ++this->wrindex;
    this->buffer[this->wrindex] = '\0';
    result = true;
  }

  chMtxUnlock(&this->lock);

  return result;
}

Fifo_Data_t FIFO_GetData(Fifo_t *this)
{
  Fifo_Data_t data = {0};

  chMtxLock(&this->lock);

  if (this->rdindex <= this->wrindex) {
    data.data   = &this->buffer[this->rdindex];
    data.length = this->wrindex - this->rdindex;
  }

  chMtxUnlock(&this->lock);

  return data;
}

bool FIFO_PopData(Fifo_t *this, size_t length)
{
  bool result = false;

  chMtxLock(&this->lock);
  
  if ((this->rdindex + length) <= this->wrindex) {
    this->rdindex += length;
    result = true;
  }

  chMtxUnlock(&this->lock);

  return result;
}

size_t FIFO_GetLength(Fifo_t *this)
{
  size_t n = 0;
  
  chMtxLock(&this->lock);

  if (this->rdindex <= this->wrindex)
    n = this->wrindex - this->rdindex;

  chMtxUnlock(&this->lock);

  return n;
}

/****************************** END OF FILE **********************************/
