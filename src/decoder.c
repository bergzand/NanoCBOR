/*
 * Copyright (C) 2019 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup nanocbor
 * @{
 * @file
 * @brief   Minimalistic CBOR decoder implementation
 *
 * @author  Koen Zandberg <koen@bergzand.net>
 * @}
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nanocbor/config.h"
#include "nanocbor/nanocbor.h"

#include NANOCBOR_BYTEORDER_HEADER

void nanocbor_decoder_init(nanocbor_value_t *value,
                           const uint8_t *buf, size_t len)
{
    value->start = buf;
    value->end = buf + len;
    value->flags = 0;
}

static inline bool _overflow(const nanocbor_value_t *it)
{
    return it->start >= it->end;
}

static inline uint8_t _get_type(const nanocbor_value_t *value)
{
    return (*value->start & NANOCBOR_TYPE_MASK);
}

int nanocbor_get_type(const nanocbor_value_t *value)
{
    if (_overflow(value)) {
        return NANOCBOR_ERR_OVERFLOW;
    }
    return (_get_type(value) >> NANOCBOR_TYPE_OFFSET);
}

bool nanocbor_at_end(nanocbor_value_t *it)
{
    bool overflow = _overflow(it);

    if (it->flags & NANOCBOR_DECODER_FLAG_CONTAINER) {
        if (it->flags & NANOCBOR_DECODER_FLAG_INDEFINITE &&
            *it->start == (NANOCBOR_TYPE_FLOAT | NANOCBOR_SIZE_INDEFINITE)) {
            it->start++;
            overflow = true;
        }
        overflow = it->remaining == 0 || overflow;
    }
    return overflow;
}

static int _get_uint64(nanocbor_value_t *cvalue, uint8_t *value, uint8_t max, int type)
{
    int ctype = nanocbor_get_type(cvalue);

    if (type != ctype) {
        return NANOCBOR_ERR_INVALID_TYPE;
    }

    unsigned bytelen = *cvalue->start & NANOCBOR_VALUE_MASK;

    if (bytelen < NANOCBOR_SIZE_BYTE) {
        *value = bytelen;
        /* Ptr should advance 1 pos */
        return 1;
    }
    if (bytelen > max) {
        return NANOCBOR_ERR_OVERFLOW;
    }
    unsigned bytes = 1U << (bytelen - NANOCBOR_SIZE_BYTE);
    if (cvalue->start + bytes > cvalue->end) {
        return NANOCBOR_ERR_END;
    }
    uint64_t tmp = 0;
    /* Copy the value from cbor to the least significant bytes */
    memcpy(((uint8_t *)&tmp) + sizeof(uint64_t) - bytes, cvalue->start + 1U, bytes);
    /* NOLINTNEXTLINE: user supplied function */
    tmp = NANOCBOR_BE64TOH_FUNC(tmp);
    memcpy(value, &tmp, bytes);
    return (int)(1 + bytes);
}

static int _get_nint32(nanocbor_value_t *cvalue, int32_t *value, uint8_t type)
{
    uint32_t tmp = 0;
    int res = _get_uint64(cvalue, (uint8_t *)&tmp, NANOCBOR_SIZE_WORD, type);

    if (tmp <= INT32_MAX) {
        *value = (-(int32_t)tmp) - 1;
        return res;
    }
    return NANOCBOR_ERR_OVERFLOW;
}

static void _advance(nanocbor_value_t *cvalue, int res)
{
    cvalue->start += res;
    cvalue->remaining--;
}

static int _advance_if(nanocbor_value_t *cvalue, int res)
{
    if (res > 0) {
        _advance(cvalue, res);
    }
    return res;
}

/* Includes check */
int nanocbor_get_uint32(nanocbor_value_t *cvalue, uint32_t *value)
{
    int res = _get_uint64(cvalue, (uint8_t *)value, NANOCBOR_SIZE_WORD,
                          NANOCBOR_TYPE_UINT);

    return _advance_if(cvalue, res);
}

/* Read the int and advance past the int */
int nanocbor_get_int32(nanocbor_value_t *cvalue, int32_t *value)
{
    uint32_t intermediate = 0;
    int res = _get_uint64(cvalue, (uint8_t *)&intermediate,
                          NANOCBOR_SIZE_WORD, NANOCBOR_TYPE_UINT);

    *value = intermediate;
    if (res == NANOCBOR_ERR_INVALID_TYPE) {
        res = _get_nint32(cvalue, value, NANOCBOR_TYPE_NINT);
    }
    return _advance_if(cvalue, res);
}

int nanocbor_get_tag(nanocbor_value_t *cvalue, uint32_t *tag)
{
    int res = _get_uint64(cvalue, (uint8_t *)tag, NANOCBOR_SIZE_WORD,
                          NANOCBOR_TYPE_TAG);

    return _advance_if(cvalue, res);
}

static int _get_str(nanocbor_value_t *cvalue, const uint8_t **buf, size_t *len, uint8_t type)
{
    *len = 0;
    int res = _get_uint64(cvalue, (uint8_t *)len, NANOCBOR_SIZE_SIZET, type);

    if (cvalue->end - cvalue->start < 0 || (size_t)(cvalue->end - cvalue->start) < *len) {
        return NANOCBOR_ERR_END;
    }
    if (_advance_if(cvalue, res) > 0) {
        *buf = (cvalue->start);
        cvalue->start += *len;
    }
    return res;
}

int nanocbor_get_bstr(nanocbor_value_t *cvalue, const uint8_t **buf, size_t *len)
{
    return _get_str(cvalue, buf, len, NANOCBOR_TYPE_BSTR);
}

int nanocbor_get_tstr(nanocbor_value_t *cvalue, const uint8_t **buf, size_t *len)
{
    return _get_str(cvalue, buf, len, NANOCBOR_TYPE_TSTR);
}

int nanocbor_get_null(nanocbor_value_t *cvalue)
{
    if (*cvalue->start == (NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_NULL)) {
        _advance(cvalue, 1);
        return NANOCBOR_OK;
    }
    return NANOCBOR_ERR_INVALID_TYPE;
}

int nanocbor_get_bool(nanocbor_value_t *cvalue, bool *value)
{
    if ((*cvalue->start & (NANOCBOR_TYPE_MASK | (NANOCBOR_VALUE_MASK - 1U))) ==
        (NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_FALSE)) {
        *value = (*cvalue->start & 0x01U);
        _advance(cvalue, 1);
        return NANOCBOR_OK;
    }
    return NANOCBOR_ERR_INVALID_TYPE;
}

int _enter_container(nanocbor_value_t *it, nanocbor_value_t *container, uint8_t type)
{
    container->end = it->end;
    container->remaining = 0;

    /* Indefinite container */
    if ((*it->start & NANOCBOR_VALUE_MASK) == NANOCBOR_VALUE_MASK) {
        container->flags = NANOCBOR_DECODER_FLAG_INDEFINITE |
                           NANOCBOR_DECODER_FLAG_CONTAINER;
        container->start = it->start + 1;
    }
    else {
        int res = _get_uint64(it, (uint8_t *)&container->remaining,
                              NANOCBOR_SIZE_WORD, type);
        if (res < 0) {
            return res;
        }
        container->flags = NANOCBOR_DECODER_FLAG_CONTAINER;
        container->start = it->start + res;
    }
    return NANOCBOR_OK;
}

int nanocbor_enter_array(nanocbor_value_t *it, nanocbor_value_t *array)
{
    return _enter_container(it, array, NANOCBOR_TYPE_ARR);
}

int nanocbor_enter_map(nanocbor_value_t *it, nanocbor_value_t *map)
{
    int res = _enter_container(it, map, NANOCBOR_TYPE_MAP);

    if (map->remaining > UINT32_MAX / 2) {
        return NANOCBOR_ERR_OVERFLOW;
    }
    map->remaining = map->remaining * 2;
    return res;
}

void nanocbor_leave_container(nanocbor_value_t *it, nanocbor_value_t *container)
{
    /* NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign) */
    it->start = container->start;
    if (it->flags == NANOCBOR_DECODER_FLAG_CONTAINER) {
        it->remaining--;
    }
}

static int _skip_simple(nanocbor_value_t *it)
{
    uint64_t tmp;
    int res = _get_uint64(it, (uint8_t*)&tmp, NANOCBOR_SIZE_LONG,
                          nanocbor_get_type(it));
    (void)tmp;
    return _advance_if(it, res);
}

int nanocbor_skip_simple(nanocbor_value_t *it)
{
    return _skip_simple(it);
}

int _skip_limited(nanocbor_value_t *it, uint8_t limit)
{
    if (limit == 0) {
        return NANOCBOR_ERR_RECURSION;
    }
    int type = nanocbor_get_type(it);
    int res = type;

    if (type == NANOCBOR_TYPE_BSTR || type == NANOCBOR_TYPE_TSTR) {
        const uint8_t *tmp;
        size_t len;
        res = _get_str(it, &tmp, &len, type);
    }
    /* map or array */
    else if (type == NANOCBOR_TYPE_ARR || type == NANOCBOR_TYPE_MAP) {
        nanocbor_value_t recurse;
        res = (type == NANOCBOR_TYPE_MAP
               ? nanocbor_enter_map(it, &recurse)
               : nanocbor_enter_array(it, &recurse));
        if (!(res < 0)) {
            while (!nanocbor_at_end(&recurse)) {
                res = _skip_limited(&recurse, limit - 1);
                if (res < 0) {
                    break;
                }
            }
        }
        nanocbor_leave_container(it, &recurse);
    }
    else if (type >= 0) {
        res = _skip_simple(it);
    }
    return res;
}

int nanocbor_skip(nanocbor_value_t *it)
{
    return _skip_limited(it, NANOCBOR_RECURSION_MAX);
}
