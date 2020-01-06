/**
 * @file BluetoothStream.h
 * @brief
 */

#ifndef BLUETOOTH_STREAM_H
#define BLUETOOTH_STREAM_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"
#include "hal.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
typedef struct OutBuffer_s {
  char data[128];
  size_t end;
  size_t index;
  mutex_t lock;
} IOBuffer_t;

#define _bluetooth_stream_data                                                \
  _base_sequential_stream_data IOBuffer_t rx;                                 \
  IOBuffer_t tx;                                                              \
  const char *udata;                                                          \
  size_t ulength;                                                             \
  mutex_t readlock;                                                           \
  mutex_t writelock;                                                          \
  thread_reference_t reader;                                                  \
  thread_reference_t writer;                                                  \
  virtual_timer_t txtimer;

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
void BLS_ProcessRxData(BluetoothStream_t *bsp, const char *rxdata, size_t rxlength);
void BLS_ClearTxBuffer(BluetoothStream_t *bsp);
void BLS_NotifyWriter(BluetoothStream_t *bsp);
void BLS_NotifyReader(BluetoothStream_t *bsp);

#endif /* BLUETOOTH_STREAM_H */

/****************************** END OF FILE **********************************/
