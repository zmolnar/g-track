/**
 * @file BluetoothStream.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "BluetoothStream.h"
#include "BluetoothManagerThread.h"
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
static void BLS_clearBuffer(Buffer_t *bp)
{
  memset(bp->data, 0, sizeof(bp->data));
  bp->end = 0;
  bp->index = 0;
}

static void BLS_initBuffer(Buffer_t *bp)
{
  BLS_clearBuffer(bp);
  chMtxObjectInit(&bp->lock);
}

static uint8_t *BLS_getFirstFree(Buffer_t *bp)
{
  return bp->data + bp->end;
}

static size_t BLS_getFreeLength(Buffer_t *bp)
{
  return sizeof(bp->data)/sizeof(bp->data[0]) - bp->end;
}

static size_t BLS_write(void *ip, const uint8_t *bp, size_t n) {
  (void)n;
  BluetoothStream_t *bsp = ip;

  chMtxLock(&bsp->writelock);

  bsp->udata = bp;
  bsp->ulength = n;

  BLT_SendUserData();
  
  chSysLock();
  chThdSuspendTimeoutS(&bsp->writer, TIME_INFINITE);
  bsp->writer = NULL;
  chSysUnlock();

  chMtxUnlock(&bsp->writelock);

  return n;
}

static size_t BLS_read(void *ip, uint8_t *bp, size_t n)
{
  BluetoothStream_t *bsp = ip;
  Buffer_t *rxbuf = &bsp->rx;

  chMtxLock(&bsp->readlock);
  chMtxLock(&rxbuf->lock);

  if (rxbuf->end <= rxbuf->index) {
    chMtxUnlock(&rxbuf->lock);

    chSysLock();
    chThdSuspendTimeoutS(&bsp->reader, TIME_INFINITE);
    bsp->reader = NULL;
    chSysUnlock();
    
    chMtxLock(&rxbuf->lock);
  } 
  
  size_t length = rxbuf->end - rxbuf->index;
  length = (n < length) ? n : length;
  memcpy(bp, rxbuf->data + rxbuf->index, length);
  rxbuf->index += length;
  
  if (rxbuf->end <= rxbuf->index)
    BLS_clearBuffer(rxbuf);

  chMtxUnlock(&rxbuf->lock);
  chMtxUnlock(&bsp->readlock);

  return length;
}

static void txTimerCallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  BLT_SendStreamDataI();
  chSysUnlockFromISR();
}

static msg_t BLS_put(void *ip, uint8_t b) {
  BluetoothStream_t *bsp = ip;
  Buffer_t *txbuf = &bsp->tx;

  chMtxLock(&bsp->writelock);
  chMtxLock(&txbuf->lock);

  chVTReset(&bsp->txtimer);

  if (0 == BLS_getFreeLength(txbuf)) {
    chMtxUnlock(&txbuf->lock);
    BLT_SendStreamData();
    
    chSysLock();
    chThdSuspendTimeoutS(&bsp->writer, TIME_INFINITE);
    bsp->writer = NULL;
    chSysUnlock();

    chMtxLock(&txbuf->lock);
  }

  txbuf->data[txbuf->index] = b;
  ++txbuf->index;
  ++txbuf->end;

  chVTSet(&bsp->txtimer, TIME_MS2I(20), txTimerCallback, bsp);

  chMtxUnlock(&txbuf->lock);
  chMtxUnlock(&bsp->writelock);

  return MSG_OK;
}
 
static msg_t BLS_get(void *ip) {
  BluetoothStream_t *bsp = ip;
  Buffer_t *rxbuf = &bsp->rx;

  chMtxLock(&bsp->readlock);
  chMtxLock(&rxbuf->lock);

  if (0 == rxbuf->end) {
    chMtxUnlock(&rxbuf->lock);

    chSysLock();
    chThdSuspendTimeoutS(&bsp->reader, TIME_INFINITE);
    bsp->reader = NULL;
    chSysUnlock();

    chMtxLock(&rxbuf->lock);
  } 
  
  msg_t result = MSG_RESET;

  if (rxbuf->index < rxbuf->end) {
    result = (msg_t)rxbuf->data[rxbuf->index];
    ++rxbuf->index;
  }

  if (rxbuf->end <= rxbuf->index) {
    BLS_clearBuffer(rxbuf);
  }

  chMtxUnlock(&rxbuf->lock);
  chMtxUnlock(&bsp->readlock);

  return result;
}

static const struct BluetoothStreamVMT vmt = {
    .write = BLS_write,
    .read = BLS_read,
    .put = BLS_put,
    .get = BLS_get,
};

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void BLS_ObjectInit(BluetoothStream_t *bsp)
{
    bsp->vmt = &vmt;
    BLS_initBuffer(&bsp->rx);
    BLS_initBuffer(&bsp->tx);
    bsp->udata = NULL;
    bsp->ulength = 0;
    chMtxObjectInit(&bsp->readlock);
    chMtxObjectInit(&bsp->writelock);
    bsp->reader = NULL;
    bsp->writer = NULL;
    chVTObjectInit(&bsp->txtimer);    
}

void BLS_ProcessRxData(BluetoothStream_t *bsp, const char *rxdata, size_t rxlength)
{
  Buffer_t *rxbuf = &bsp->rx;
  chMtxLock(&rxbuf->lock);

  uint8_t *begin = BLS_getFirstFree(rxbuf);
  size_t length = BLS_getFreeLength(rxbuf);

  if (rxlength < length)
    length = rxlength;

  memcpy(begin, rxdata, length);
  rxbuf->end += length;

  BLS_NotifyReader(bsp);

  chMtxUnlock(&rxbuf->lock);
}

void BLS_ClearTxBuffer(BluetoothStream_t *bsp)
{
  chMtxLock(&bsp->tx.lock);
  BLS_clearBuffer(&bsp->tx);
  chMtxUnlock(&bsp->tx.lock);
}

void BLS_NotifyWriter(BluetoothStream_t *bsp)
{
  if (bsp->writer) {
    chThdResume(&bsp->writer, MSG_OK);
  }
}

void BLS_NotifyReader(BluetoothStream_t *bsp)
{
  if (bsp->reader) {
    chThdResume(&bsp->reader, MSG_OK);
  }
}

/****************************** END OF FILE **********************************/
