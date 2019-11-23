/**
 * @file Logger.h
 * @brief Declaration of the logger module.
 */

#ifndef LOGGER_H
#define LOGGER_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/

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
void LOG_Write(const char *file, const char *entry);

/**
 * @brief Overwrite the given file with the new content.
 */
void LOG_Overwrite(const char *file, const char *entry);

#endif /* LOGGER_H */

/****************************** END OF FILE **********************************/
