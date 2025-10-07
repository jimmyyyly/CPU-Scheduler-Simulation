// Author: Jimmy Ly
// Date: October 6 2025

#ifndef LOG_H
#define LOG_H
/*
* Compilation notes:
* C compilers:
* uses bool, must compile with -std=c99 or later (bool was introduced
* in the 1999 C standard.
*
* C++ compilers
* uses uint32_t, unsigned 32 bit integer type, introduced in C++11,
* The defaults in the g++ compiler on edoras should be fine with this
*/
/* C and C++ define some of their types in different places.
* Check and see if we are using C or C++ and include appropriately
* so that this will compile under C and C++
*/
#ifdef __cplusplus
/* C++ includes */
#include <stdint.h>
#else
/* C includes */
#include <inttypes.h>
#include <stdbool.h>
#endif
/*
* structure used for tracking execution stop reasons
*
* If compiled with a C compiler, make sure that the C99 dialect or later is used.
* (-std=c99 with a GNU C compiler)
*/
typedef enum {
ENTER_IO,
QUANTUM_EXPIRED,
COMPLETED,
} ExecutionStopReasonType;
/**
* @brief
*
* @param procID
* @param cpuExecutedTime
* @param ioExecutedTime
* @param totalElapsedTime
* @param stopReason
*/
void log_cpuburst_execution (unsigned int procID,
unsigned int cpuExecutedTime,
unsigned int ioExecutedTime,
unsigned int totalElapsedTime,
ExecutionStopReasonType stopReason);
/**
* @brief
*
* @param bursts - 1D array
*/
void log_process_bursts (unsigned int bursts[], size_t numOfBursts);
/**
* @brief
*
* @param procID
* @param completionTime
* @param totalWaitTime
*/
void log_process_completion (unsigned int procID,
unsigned int completionTime,
// wait time is the time spent in the ready queue
// wait time = completionTime - total cpu bursts - total io bursts
unsigned int totalWaitTime);
#endif
