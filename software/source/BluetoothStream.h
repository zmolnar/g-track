/**
 * @file BluetoothStream.h
 * @brief
 */

#ifndef BLUETOOTH_STREAM_H
#define BLUETOOTH_STREAM_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "hal.h"
#include "ch.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define _bluetooth_stream_data                                                \
  _base_sequential_stream_data                                                \
  uint8_t rxbuf[128];                                                         \
  uint8_t txbuf[128];                                                         \
  uint8_t *ibuf;                                                              \
  const uint8_t *obuf;                                                        \
  size_t rxlength;                                                            \
  size_t txlength;                                                            \
  size_t rdindex;                                                             \
  size_t wrindex;                                                             \
  semaphore_t rxsync;                                                         \
  semaphore_t txsync;                                                         \
  mutex_t rxlock;                                                             \
  mutex_t txlock;                                                             \
  thread_reference_t reader;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
struct BluetoothStreamVMT {
  _base_sequential_stream_methods
};

typedef struct BluetoothStream_s {
  const struct BluetoothStreamVMT *vmt;
  _bluetooth_stream_data
} BluetoothStream_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
void BLS_ObjectInit(BluetoothStream_t *bsp);

#endif /* BLUETOOTH_STREAM_H */

/****************************** END OF FILE **********************************/
