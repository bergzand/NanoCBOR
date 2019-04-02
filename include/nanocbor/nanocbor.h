/*
 * Copyright (C) 2019 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    nanocbor minimalistic CBOR library
 * @brief       Provides a minimal CBOR library
 *
 * NanoCBOR is a minimal CBOR encoder. For protocols such as CoAP, OSCORE,
 * SenML and CORECONF a well defined and thus predictable CBOR structure is
 * required. NanoCBOR tries to fill this requirement by providing a very
 * minimal CBOR encoder. Supported is:
 *  - All major types
 *  - Arrays including indefinite length arrays
 *  - Maps including indefinite length maps
 *  - Safe for decoding untrusted input
 *
 * Not included:
 *  - Date and time
 *
 * Assumptions:
 *  - Encoder input is already validated
 *
 * @{
 *
 * @file
 * @see         [rfc 7049](https://tools.ietf.org/html/rfc7049)
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef NANOCBOR_H
#define NANOCBOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NANOCBOR_TYPE_OFFSET    (5U)
#define NANOCBOR_TYPE_MASK      0xE0U
#define NANOCBOR_VALUE_MASK     0x1FU

#define NANOCBOR_TYPE_UINT      (0x00U)
#define NANOCBOR_TYPE_NINT      (0x01U)
#define NANOCBOR_TYPE_BSTR      (0x02U)
#define NANOCBOR_TYPE_TSTR      (0x03U)
#define NANOCBOR_TYPE_ARR       (0x04U)
#define NANOCBOR_TYPE_MAP       (0x05U)
#define NANOCBOR_TYPE_TAG       (0x06U)
#define NANOCBOR_TYPE_FLOAT     (0x07U)

/**
 * @name CBOR major types
 * @{
 */
#define NANOCBOR_MASK_UINT      (NANOCBOR_TYPE_UINT  << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_NINT      (NANOCBOR_TYPE_NINT  << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_BSTR      (NANOCBOR_TYPE_BSTR  << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_TSTR      (NANOCBOR_TYPE_TSTR  << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_ARR       (NANOCBOR_TYPE_ARR   << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_MAP       (NANOCBOR_TYPE_MAP   << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_TAG       (NANOCBOR_TYPE_TAG   << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_FLOAT     (NANOCBOR_TYPE_FLOAT << NANOCBOR_TYPE_OFFSET)
/** @} */

/**
 * @name CBOR simple data types
 * @{
 */
#define NANOCBOR_SIMPLE_FALSE       20U
#define NANOCBOR_SIMPLE_TRUE        21U
#define NANOCBOR_SIMPLE_NULL        22U
#define NANOCBOR_SIMPLE_UNDEF       23U
/** @} */

/**
 * @name CBOR integer special size values
 * @{
 */
#define NANOCBOR_INT_VAL_UINT8          24U
#define NANOCBOR_INT_VAL_UINT16         25U
#define NANOCBOR_INT_VAL_UINT32         26U
#define NANOCBOR_INT_VAL_UINT64         27U
/** @} */

/**
 * @name CBOR data sizes
 * @{
 */
#define NANOCBOR_SIZE_BYTE          24U
#define NANOCBOR_SIZE_SHORT         25U
#define NANOCBOR_SIZE_WORD          26U
#define NANOCBOR_SIZE_INDEFINITE    31U
/** @} */

typedef enum {
    NANOCBOR_OK = 0,
    NANOCBOR_ERR_OVERFLOW = -1,
    NANOCBOR_ERR_INVALID_TYPE = -2,
    NANOCBOR_ERR_END = -3,
    NANOCBOR_ERR_INVALID = -4,
    NANOCBOR_ERR_RECURSION = -5,
} nanocbor_error_t;

typedef struct nanocbor_value {
    const uint8_t *start;
    const uint8_t *end;
    uint32_t remaining;
    uint8_t flags;
} nanocbor_value_t;

#define NANOCBOR_DECODER_FLAG_CONTAINER  (0x01U)
#define NANOCBOR_DECODER_FLAG_INDEFINITE (0x02U)

void nanocbor_decoder_init(nanocbor_value_t *value,
                           const uint8_t *buf, size_t len);
uint8_t nanocbor_get_type(nanocbor_value_t *value);
bool nanocbor_at_end(nanocbor_value_t *it);
int nanocbor_get_uint32(nanocbor_value_t *cvalue, uint32_t *value);
int nanocbor_get_int32(nanocbor_value_t *cvalue, int32_t *value);
int nanocbor_get_bstr(nanocbor_value_t *cvalue, const uint8_t **buf, size_t *len);
int nanocbor_get_tstr(nanocbor_value_t *cvalue, const uint8_t **buf, size_t *len);
int nanocbor_enter_array(nanocbor_value_t *it, nanocbor_value_t *array);
int nanocbor_enter_map(nanocbor_value_t *it, nanocbor_value_t *map);
void nanocbor_leave_container(nanocbor_value_t *it, nanocbor_value_t *array);
int nanocbor_get_null(nanocbor_value_t *cvalue);
int nanocbor_get_bool(nanocbor_value_t *cvalue, bool *value);
int nanocbor_skip(nanocbor_value_t *it);
int nanocbor_skip_simple(nanocbor_value_t *it);

/**
 * @brief Write a CBOR boolean value into a buffer
 *
 * @param[in]   buf     Buffer to write into
 * @param[in]   content Boolean value to write
 *
 * @return              Number of bytes written
 */
size_t nanocbor_fmt_bool(uint8_t *buf, bool content);

/**
 * @brief Write an unsigned integer of at most sizeof uint32_t into the buffer
 *
 * @param[in]   buf     buffer to write into
 * @param[in]   num     unsigned integer to write
 *
 * @return  number of bytes written
 */
size_t nanocbor_fmt_uint(uint8_t *buf, uint32_t num);

/**
 * @brief Write a signed integer of at most sizeof int32_t into the buffer
 *
 * If it is not certain if the data is signed, use this function.
 *
 * @param[in]   buf     buffer to write into
 * @param[in]   num     unsigned integer to write
 *
 * @return  number of bytes written
 */
size_t nanocbor_fmt_int(uint8_t *buf, int32_t num);

size_t nanocbor_fmt_bstr(uint8_t *buf, size_t len);
size_t nanocbor_fmt_tstr(uint8_t *buf, size_t len);

/**
 * @brief Write an array indicator with @ref len items
 *
 * @param[in]   buf     buffer to write into
 * @param[in]   len     Number of items in the array
 *
 * @return              Number of bytes written
 */
size_t nanocbor_fmt_array(uint8_t *buf, size_t len);

/**
 * @brief Write a map indicator with @ref len pairs
 *
 * @param[in]   buf     buffer to write into
 * @param[in]   len     Number of pairs in the map
 *
 * @return              Number of bytes written
 */
size_t nanocbor_fmt_map(uint8_t *buf, size_t len);

/**
 * @brief Write an indefinite-length array indicator
 *
 * @param[in]   buf     buffer to write into
 *
 * @return              Number of bytes written
 */
size_t nanocbor_fmt_array_indefinite(uint8_t *buf);

/**
 * @brief Write an indefinite-length map indicator
 *
 * @param[in]   buf     buffer to write the map
 *
 * @return              Number of bytes written
 */
size_t nanocbor_fmt_map_indefinite(uint8_t *buf);

size_t nanocbor_fmt_float(uint8_t *buf, float num);

#ifdef __cplusplus
}
#endif

#endif /* NANOCBOR_H */
