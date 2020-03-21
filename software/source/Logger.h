/**
 * @file Logger.h
 * @brief Declaration of the logger module.
 */

#ifndef LOGGER_H
#define LOGGER_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <stddef.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
/**
 * @brief Append content to the end of the given file.
 */
void LOG_AppendToFile(const char *file, const char *entry);

/**
 * @brief Append the content of the buffer to the end of the given file.
 */
void LOG_WriteBuffer(const char *file, const char ibuf[], size_t ilen);

/**
 * @brief Overwrite the given file with the new content.
 */
void LOG_OverWriteFile(const char *file, const char *entry);

#endif /* LOGGER_H */

/****************************** END OF FILE **********************************/
