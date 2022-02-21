/*
 * SPDX-License-Identifier: CC0-1.0
 */

#ifndef TEST_H
#define TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function prototype for a test function
 */
typedef void (*test_func)(void);

/**
 * Struct to define a test
 */
typedef struct test {
    const test_func f; /**< Function to run as test */
    const char *n; /**< Name or description of the test */
} test_t;

#ifdef __cplusplus
}
#endif

#endif
