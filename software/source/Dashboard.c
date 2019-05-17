/**
 * @file Dashboard.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Dashboard.h"
#include "ch.h"

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
Dashboard_t dashboard;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void dbInit(Dashboard_t *pdsh) {
    memset(pdsh, 0, sizeof(*pdsh));
    chMtxObjectInit(&pdsh->lock);
}

void dbLock(Dashboard_t *pdsh) {
    chMtxLock(&pdsh->lock);
}

void dbUnlock(Dashboard_t *pdsh) {
    chMtxUnlock(&pdsh->lock);
}

Position_t *dbGetPosition(Dashboard_t *pdsh) {
    return &pdsh->position;
}

/****************************** END OF FILE **********************************/
