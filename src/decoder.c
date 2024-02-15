/*
 * SPDX-License-Identifier: CC0-1.0
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

void nanocbor_decoder_init(nanocbor_value_t *value, const uint8_t *buf,
                           size_t len)
{
    value->cur = buf;
    value->end = buf + len;
    value->flags = 0;
#if NANOCBOR_DECODE_PACKED_ENABLED
    memset(value->shared_item_tables, 0, sizeof(value->shared_item_tables));
#endif
}

#if NANOCBOR_DECODE_PACKED_ENABLED
void nanocbor_decoder_init_packed(nanocbor_value_t *value, const uint8_t *buf,
                           size_t len)
{
    nanocbor_decoder_init(value, buf, len);
    value->flags = NANOCBOR_DECODER_FLAG_PACKED_SUPPORT;
}
#endif

static void _advance(nanocbor_value_t *cvalue, unsigned int res)
{
    cvalue->cur += res;
    cvalue->remaining--;
}

static int _advance_if(nanocbor_value_t *cvalue, int res)
{
    if (res > 0) {
        _advance(cvalue, (unsigned int)res);
    }
    return res;
}

static inline bool _over_end(const nanocbor_value_t *it)
{
    return it->cur >= it->end;
}

static inline uint8_t _get_type(const nanocbor_value_t *value)
{
    return (*value->cur & NANOCBOR_TYPE_MASK);
}

static int _value_match_exact(nanocbor_value_t *cvalue, uint8_t val)
{
    int res = NANOCBOR_ERR_INVALID_TYPE;

    if (_over_end(cvalue)) {
        res = NANOCBOR_ERR_END;
    }
    else if (*cvalue->cur == val) {
        _advance(cvalue, 1U);
        res = NANOCBOR_OK;
    }
    return res;
}

bool nanocbor_at_end(const nanocbor_value_t *it)
{
    bool end = false;
    /* The container is at the end when */
    if (_over_end(it) || /* Number of items exhausted */
        /* Indefinite container and the current item is the end marker */
        ((nanocbor_container_indefinite(it)
          && *it->cur
              == (NANOCBOR_TYPE_FLOAT << NANOCBOR_TYPE_OFFSET
                  | NANOCBOR_VALUE_MASK)))
        ||
        /* Or the remaining number of items is zero */
        (!nanocbor_container_indefinite(it) && nanocbor_in_container(it)
         && it->remaining == 0)) {
        end = true;
    }
    return end;
}

int nanocbor_get_type(const nanocbor_value_t *value)
{
    if (nanocbor_at_end(value)) {
        return NANOCBOR_ERR_END;
    }
    return (_get_type(value) >> NANOCBOR_TYPE_OFFSET);
}

static int _get_uint64(const nanocbor_value_t *cvalue, uint64_t *value,
                       uint8_t max, int type)
{
    int ctype = nanocbor_get_type(cvalue);

    if (ctype < 0) {
        return ctype;
    }

    if (type != ctype) {
        return NANOCBOR_ERR_INVALID_TYPE;
    }
    unsigned bytelen = *cvalue->cur & NANOCBOR_VALUE_MASK;

    if (bytelen < NANOCBOR_SIZE_BYTE) {
        *value = bytelen;
        /* Ptr should advance 1 pos */
        return 1;
    }
    if (bytelen > max) {
        return NANOCBOR_ERR_OVERFLOW;
    }

    unsigned bytes = 1U << (bytelen - NANOCBOR_SIZE_BYTE);

    if ((cvalue->cur + bytes) >= cvalue->end) {
        return NANOCBOR_ERR_END;
    }
    uint64_t tmp = 0;
    /* Copy the value from cbor to the least significant bytes */
    memcpy(((uint8_t *)&tmp) + sizeof(uint64_t) - bytes, cvalue->cur + 1U,
           bytes);
    /* NOLINTNEXTLINE: user supplied function */
    tmp = NANOCBOR_BE64TOH_FUNC(tmp);
    *value = 0;
    memcpy(value, &tmp, bytes);

    return (int)(1 + bytes);
}


static inline bool _packed_enabled(const nanocbor_value_t *value)
{
    return NANOCBOR_DECODE_PACKED_ENABLED && (value->flags & NANOCBOR_DECODER_FLAG_PACKED_SUPPORT) != 0;
}

static inline int _packed_consume_table(nanocbor_value_t *cvalue, nanocbor_value_t *target)
{
    int ret;
    ret = _value_match_exact(cvalue, NANOCBOR_TYPE_ARR << NANOCBOR_TYPE_OFFSET | 0x02);
    if (ret < 0) {
        return NANOCBOR_ERR_INVALID_TYPE; // todo: which error code to return here?
    }

    *target = *cvalue;
    for (size_t i=0; i<NANOCBOR_DECODE_PACKED_NESTED_TABLES_MAX; i++) {
        struct nanocbor_packed_table *t = &target->shared_item_tables[i];
        if (t->start == NULL) {
            ret = nanocbor_get_subcbor(cvalue, &t->start, &t->len);
            if (ret != NANOCBOR_OK) {
                return NANOCBOR_ERR_INVALID_TYPE; // todo: which error code to return here?
            }
            // todo: will this break if rump is array? > shouldn't if skip behaves correctly
            // this should in theory allow to fix leave_container if array inside table -> check for flag, then no-op
            // skip rump
            nanocbor_skip(cvalue);
            _advance(target, t->len);
            return NANOCBOR_OK;
        }
    }
    return NANOCBOR_ERR_RECURSION; // todo: which error?
}

static inline int _packed_follow_reference(const nanocbor_value_t *cvalue, nanocbor_value_t *target, uint64_t idx)
{
    // int ret;
    for (size_t i=0; i<NANOCBOR_DECODE_PACKED_NESTED_TABLES_MAX; i++) {
        const struct nanocbor_packed_table *t = &cvalue->shared_item_tables[NANOCBOR_DECODE_PACKED_NESTED_TABLES_MAX-1-i];
        if (t->start != NULL) {
            // todo: do we really have to store length in B of each table? > probably yes, also for implicit tables added before decoding!
            // todo: proper error handling everywhere
            nanocbor_decoder_init_packed(target, t->start, t->len);
            target->flags |= NANOCBOR_DECODER_FLAG_SHARED;
            uint64_t table_size = 0;
            // don't use _get_and_advance_uint64() to avoid packed
            int res = _get_uint64(target, &table_size, NANOCBOR_SIZE_LONG, NANOCBOR_TYPE_ARR);
            _advance_if(target, res);
            if (idx < table_size) {
                for (size_t j=0; j<idx; j++) {
                    nanocbor_skip(target);
                }
                return NANOCBOR_OK;
            } else {
                idx -= table_size;
            }
        }
    }
    // idx not within current tables
    return NANOCBOR_ERR_INVALID_TYPE; // todo: which error code to return here?
}

static inline int _packed_follow(nanocbor_value_t *cvalue, nanocbor_value_t *target)
{
    if (!_packed_enabled(cvalue)) {
        return NANOCBOR_NOT_FOUND;
    }

    int ret;
    // todo: might break since using API-facing function
    int ctype = nanocbor_get_type(cvalue);
    if (ctype == NANOCBOR_TYPE_TAG) {
        uint64_t tag = 0;
        ret = _get_uint64(cvalue, &tag, NANOCBOR_SIZE_WORD, NANOCBOR_TYPE_TAG);
        if (ret < 0) {
            return NANOCBOR_NOT_FOUND;
        }
        if (tag == NANOCBOR_TAG_PACKED_TABLE) {
            _advance(cvalue, ret); // todo: should be broken, as decrements counter for cvalue->remaining, also below -> test & fix
            return _packed_consume_table(cvalue, target);
        }
        else if (tag == NANOCBOR_TAG_PACKED_REF_SHARED) {
            _advance(cvalue, ret);
            int64_t n;
            // todo: might break since using API-facing function
            ret = nanocbor_get_int64(cvalue, &n);
            if (ret != NANOCBOR_OK)
                return NANOCBOR_ERR_INVALID_TYPE; // todo: which error code to return here?
            uint64_t idx = 16 + (n >= 0 ? 2*n : -2*n-1);
            return _packed_follow_reference(cvalue, target, idx);
        }
    }
    else if (ctype == NANOCBOR_TYPE_FLOAT) {
        uint8_t simple = *cvalue->cur & NANOCBOR_VALUE_MASK;
        if (simple < 16) {
            _advance(cvalue, 1);
            return _packed_follow_reference(cvalue, target, simple);
        }
    }
    return NANOCBOR_NOT_FOUND;
}

static int _get_and_advance_uint8(nanocbor_value_t *cvalue, uint8_t *value,
                                  int type)
{
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return _get_and_advance_uint8(&followed, value, type);
    }
#endif
    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_BYTE, type);
    *value = (uint8_t)tmp;

    return _advance_if(cvalue, res);
}

static int _get_and_advance_uint16(nanocbor_value_t *cvalue, uint16_t *value,
                                   int type)
{
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return _get_and_advance_uint16(&followed, value, type);
    }
#endif
    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_SHORT, type);
    *value = (uint16_t)tmp;

    return _advance_if(cvalue, res);
}

static int _get_and_advance_uint32(nanocbor_value_t *cvalue, uint32_t *value,
                                   int type)
{
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return _get_and_advance_uint32(&followed, value, type);
    }
#endif
    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_WORD, type);
    *value = tmp;

    return _advance_if(cvalue, res);
}

static int _get_and_advance_uint64(nanocbor_value_t *cvalue, uint64_t *value,
                                   int type)
{
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return _get_and_advance_uint64(&followed, value, type);
    }
#endif
    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_LONG, type);
    *value = tmp;

    return _advance_if(cvalue, res);
}

int nanocbor_get_uint8(nanocbor_value_t *cvalue, uint8_t *value)
{
    return _get_and_advance_uint8(cvalue, value, NANOCBOR_TYPE_UINT);
}

int nanocbor_get_uint16(nanocbor_value_t *cvalue, uint16_t *value)
{
    return _get_and_advance_uint16(cvalue, value, NANOCBOR_TYPE_UINT);
}

int nanocbor_get_uint32(nanocbor_value_t *cvalue, uint32_t *value)
{
    return _get_and_advance_uint32(cvalue, value, NANOCBOR_TYPE_UINT);
}

int nanocbor_get_uint64(nanocbor_value_t *cvalue, uint64_t *value)
{
    return _get_and_advance_uint64(cvalue, value, NANOCBOR_TYPE_UINT);
}

static int _get_and_advance_int64(nanocbor_value_t *cvalue, int64_t *value,
                                  uint8_t max, uint64_t bound)
{
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return _get_and_advance_int64(&followed, value, max, bound);
    }
#endif
    int type = nanocbor_get_type(cvalue);
    if (type < 0) {
        return type;
    }
    int res = NANOCBOR_ERR_INVALID_TYPE;
    if (type == NANOCBOR_TYPE_NINT || type == NANOCBOR_TYPE_UINT) {
        uint64_t intermediate = 0;
        res = _get_uint64(cvalue, &intermediate, max, type);
        if (intermediate > bound) {
            res = NANOCBOR_ERR_OVERFLOW;
        }
        if (type == NANOCBOR_TYPE_NINT) {
            *value = (-(int64_t)intermediate) - 1;
        }
        else {
            *value = (int64_t)intermediate;
        }
    }
    return _advance_if(cvalue, res);
}

int nanocbor_get_int8(nanocbor_value_t *cvalue, int8_t *value)
{
    int64_t tmp = 0;
    int res
        = _get_and_advance_int64(cvalue, &tmp, NANOCBOR_SIZE_BYTE, INT8_MAX);

    *value = (int8_t)tmp;

    return res;
}

int nanocbor_get_int16(nanocbor_value_t *cvalue, int16_t *value)
{
    int64_t tmp = 0;
    int res
        = _get_and_advance_int64(cvalue, &tmp, NANOCBOR_SIZE_SHORT, INT16_MAX);

    *value = (int16_t)tmp;

    return res;
}

int nanocbor_get_int32(nanocbor_value_t *cvalue, int32_t *value)
{
    int64_t tmp = 0;
    int res
        = _get_and_advance_int64(cvalue, &tmp, NANOCBOR_SIZE_WORD, INT32_MAX);

    *value = (int32_t)tmp;

    return res;
}

int nanocbor_get_int64(nanocbor_value_t *cvalue, int64_t *value)
{
    return _get_and_advance_int64(cvalue, value, NANOCBOR_SIZE_LONG, INT64_MAX);
}

int nanocbor_get_tag(nanocbor_value_t *cvalue, uint32_t *tag)
{
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return nanocbor_get_tag(&followed, tag);
    }
#endif

    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_WORD, NANOCBOR_TYPE_TAG);

    if (res >= 0) {
        cvalue->cur += res;
        res = NANOCBOR_OK;
    }
    *tag = (uint32_t)tmp;

    return res;
}

int nanocbor_get_decimal_frac(nanocbor_value_t *cvalue, int32_t *e, int32_t *m)
{
    int res = NANOCBOR_NOT_FOUND;
    uint32_t tag = UINT32_MAX;
    if (nanocbor_get_tag(cvalue, &tag) == NANOCBOR_OK) {
        if (tag == NANOCBOR_TAG_DEC_FRAC) {
            nanocbor_value_t arr;
            if (nanocbor_enter_array(cvalue, &arr) == NANOCBOR_OK) {
                res = nanocbor_get_int32(&arr, e);
                if (res >= 0) {
                    res = nanocbor_get_int32(&arr, m);
                    if (res >= 0) {
                        res = NANOCBOR_OK;
                    }
                }
                nanocbor_leave_container(cvalue, &arr);
            }
        }
    }

    return res;
}

static int _get_str(nanocbor_value_t *cvalue, const uint8_t **buf, size_t *len,
                    uint8_t type)
{
// todo: recursion count needed for loop detection!
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return _get_str(&followed, buf, len, type);
    }
#endif

    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_SIZET, type);
    *len = tmp;

    if (cvalue->end - cvalue->cur < 0
        || (size_t)(cvalue->end - cvalue->cur) < *len) {
        return NANOCBOR_ERR_END;
    }
    if (res >= 0) {
        *buf = (cvalue->cur) + res;
        _advance(cvalue, (unsigned int)((size_t)res + *len));
        res = NANOCBOR_OK;
    }
    return res;
}

int nanocbor_get_bstr(nanocbor_value_t *cvalue, const uint8_t **buf,
                      size_t *len)
{
    return _get_str(cvalue, buf, len, NANOCBOR_TYPE_BSTR);
}

int nanocbor_get_tstr(nanocbor_value_t *cvalue, const uint8_t **buf,
                      size_t *len)
{
    return _get_str(cvalue, buf, len, NANOCBOR_TYPE_TSTR);
}

int nanocbor_get_null(nanocbor_value_t *cvalue)
{
// todo: recursion count needed for loop detection!
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return nanocbor_get_null(&followed);
    }
#endif
    return _value_match_exact(cvalue,
                              NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_NULL);
}

int nanocbor_get_bool(nanocbor_value_t *cvalue, bool *value)
{
// todo: recursion count needed for loop detection!
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return nanocbor_get_bool(&followed, value);
    }
#endif
    int res = _value_match_exact(cvalue,
                                 NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_FALSE);
    if (res >= NANOCBOR_OK) {
        *value = false;
    } else {
        res = _value_match_exact(cvalue,
                                 NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_TRUE);
        if (res >= NANOCBOR_OK) {
            *value = true;
        }
    }

    return res;
}

int nanocbor_get_undefined(nanocbor_value_t *cvalue)
{
// todo: recursion count needed for loop detection!
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return nanocbor_get_undefined(&followed);
    }
#endif
    return _value_match_exact(cvalue,
                              NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_UNDEF);
}

int nanocbor_get_simple(nanocbor_value_t *cvalue, uint8_t *value)
{
    int res = _get_and_advance_uint8(cvalue, value, NANOCBOR_TYPE_FLOAT);
    if (res == NANOCBOR_ERR_OVERFLOW) {
        res = NANOCBOR_ERR_INVALID_TYPE;
    }
    return res > 0 ? NANOCBOR_OK : res;
}

/* float bit mask related defines */
#define FLOAT_EXP_OFFSET (127U)
#define FLOAT_SIZE (32U)
#define FLOAT_EXP_POS (23U)
#define FLOAT_EXP_MASK ((uint32_t)0xFFU)
#define FLOAT_SIGN_POS (31U)
#define FLOAT_FRAC_MASK (0x7FFFFFU)
#define FLOAT_SIGN_MASK ((uint32_t)1U << FLOAT_SIGN_POS)
#define FLOAT_EXP_IS_NAN (0xFFU)
#define FLOAT_IS_ZERO (~(FLOAT_SIGN_MASK))
/* Part where a float to halffloat leads to precision loss */
#define FLOAT_HALF_LOSS (0x1FFFU)

/* halffloat bit mask related defines */
#define HALF_EXP_OFFSET (15U)
#define HALF_SIZE (16U)
#define HALF_EXP_POS (10U)
#define HALF_EXP_MASK (0x1FU)
#define HALF_SIGN_POS (15U)
#define HALF_FRAC_MASK (0x3FFU)
#define HALF_SIGN_MASK ((uint16_t)(1U << HALF_SIGN_POS))
#define HALF_MASK_HALF (0xFFU)

#define HALF_FLOAT_EXP_DIFF ((uint16_t)(FLOAT_EXP_OFFSET - HALF_EXP_OFFSET))
#define HALF_FLOAT_EXP_POS_DIFF ((uint16_t)(FLOAT_EXP_POS - HALF_EXP_POS))
#define HALF_EXP_TO_FLOAT (HALF_FLOAT_EXP_DIFF << HALF_EXP_POS)

static int _decode_half_float(nanocbor_value_t *cvalue, float *value)
{
    uint64_t tmp = 0;
    int res
        = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_SHORT, NANOCBOR_TYPE_FLOAT);
    if (res == 1 + sizeof(uint16_t)) {
        uint32_t *ifloat = (uint32_t *)value;
        *ifloat = (tmp & HALF_SIGN_MASK) << (FLOAT_SIGN_POS - HALF_SIGN_POS);

        uint32_t significant = tmp & HALF_FRAC_MASK;
        uint32_t exponent = tmp & (HALF_EXP_MASK << HALF_EXP_POS);

        static const uint32_t magic = ((uint32_t)FLOAT_EXP_OFFSET - 1)
            << FLOAT_EXP_POS;
        static const float *fmagic = (float *)&magic;

        if (exponent == 0) {
            *ifloat = magic + significant;
            *value -= *fmagic;
        }
        else {
            if (exponent == (HALF_EXP_MASK << HALF_EXP_POS)) {
                /* Set exponent to max value */
                exponent
                    = ((FLOAT_EXP_MASK - HALF_FLOAT_EXP_DIFF) << HALF_EXP_POS);
            }
            *ifloat
                |= ((exponent + HALF_EXP_TO_FLOAT) << HALF_FLOAT_EXP_POS_DIFF)
                | (significant << HALF_FLOAT_EXP_POS_DIFF);
        }
        return _advance_if(cvalue, res);
    }
    return res > 0 ? NANOCBOR_ERR_INVALID_TYPE : res;
}

static int _decode_float(nanocbor_value_t *cvalue, float *value)
{
    uint64_t tmp = 0;
    int res
        = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_WORD, NANOCBOR_TYPE_FLOAT);
    if (res == 1 + sizeof(uint32_t)) {
        uint32_t ifloat = tmp;
        memcpy(value, &ifloat, sizeof(uint32_t));
        return _advance_if(cvalue, res);
    }
    return res > 0 ? NANOCBOR_ERR_INVALID_TYPE : res;
}

static int _decode_double(nanocbor_value_t *cvalue, double *value)
{
    uint64_t tmp = 0;
    int res
        = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_LONG, NANOCBOR_TYPE_FLOAT);
    if (res == 1 + sizeof(uint64_t)) {
        uint64_t ifloat = tmp;
        memcpy(value, &ifloat, sizeof(uint64_t));
        return _advance_if(cvalue, res);
    }
    return res > 0 ? NANOCBOR_ERR_INVALID_TYPE : res;
}

int nanocbor_get_float(nanocbor_value_t *cvalue, float *value)
{
// todo: recursion count needed for loop detection!
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return nanocbor_get_float(&followed, value);
    }
#endif
    int res = _decode_half_float(cvalue, value);
    if (res < 0) {
        res = _decode_float(cvalue, value);
    }
    return res;
}

int nanocbor_get_double(nanocbor_value_t *cvalue, double *value)
{
// todo: recursion count needed for loop detection!
#if NANOCBOR_DECODE_PACKED_ENABLED
    nanocbor_value_t followed;
    if (_packed_follow(cvalue, &followed) == NANOCBOR_OK) {
        return nanocbor_get_double(&followed, value);
    }
#endif
    float tmp = 0;
    int res = nanocbor_get_float(cvalue, &tmp);
    if (res >= NANOCBOR_OK) {
        *value = tmp;
        return res;
    }
    return _decode_double(cvalue, value);
}

static int _enter_container(const nanocbor_value_t *it,
                            nanocbor_value_t *container, uint8_t type)
{
    if (_packed_enabled(it)) {
        memcpy(container->shared_item_tables, it->shared_item_tables, sizeof(it->shared_item_tables));

        // todo: additional nanocbor_value_t whis is only needed because `it` is const
        // todo: getting rid of const would avoid skip in leave_container, too
        nanocbor_value_t tmp = *it;
        nanocbor_value_t followed;
        if (_packed_follow(&tmp, &followed) == NANOCBOR_OK) {
            int res = _enter_container(&followed, container, type);
            container->flags |= (followed.flags & NANOCBOR_DECODER_FLAG_SHARED) ? NANOCBOR_DECODER_FLAG_SHARED : 0;
            return res;
        }

        container->flags = NANOCBOR_DECODER_FLAG_PACKED_SUPPORT;
    }
    else {
        container->flags = 0;
    }
    container->end = it->end;
    container->remaining = 0;

    uint8_t value_match = (uint8_t)(((unsigned)type << NANOCBOR_TYPE_OFFSET)
                                    | NANOCBOR_SIZE_INDEFINITE);

    /* Not using _value_match_exact here to keep *it const */
    if (!_over_end(it) && *it->cur == value_match) {
        container->flags |= NANOCBOR_DECODER_FLAG_INDEFINITE
            | NANOCBOR_DECODER_FLAG_CONTAINER;
        container->cur = it->cur + 1;
        return NANOCBOR_OK;
    }

    int res = _get_uint64(it, &container->remaining, NANOCBOR_SIZE_LONG, type);
    if (res < 0) {
        return res;
    }
    container->flags |= NANOCBOR_DECODER_FLAG_CONTAINER;
    container->cur = it->cur + res;
    return NANOCBOR_OK;
}

int nanocbor_enter_array(const nanocbor_value_t *it, nanocbor_value_t *array)
{
    return _enter_container(it, array, NANOCBOR_TYPE_ARR);
}

int nanocbor_enter_map(const nanocbor_value_t *it, nanocbor_value_t *map)
{
    int res = _enter_container(it, map, NANOCBOR_TYPE_MAP);

    if (map->remaining > UINT64_MAX / 2) {
        return NANOCBOR_ERR_OVERFLOW;
    }
    map->remaining = map->remaining * 2;
    return res;
}

int nanocbor_leave_container(nanocbor_value_t *it, nanocbor_value_t *container)
{
    /* check `container` to be a valid, fully consumed container that is plausible to have been entered from `it` */
    if (!nanocbor_in_container(container) || !nanocbor_at_end(container)) {
        return NANOCBOR_ERR_INVALID_TYPE;
    }
    if (container->flags & NANOCBOR_DECODER_FLAG_SHARED) {
        return nanocbor_skip(it); // todo: use _skip_limited instead to bound recursion?!
    }
    if (container->cur <= it->cur || container->cur > it->end) {
        return NANOCBOR_ERR_INVALID_TYPE;
    }
    if (it->remaining) {
        it->remaining--;
    }
    if (nanocbor_container_indefinite(container)) {
        it->cur = container->cur + 1;
    }
    else {
        it->cur = container->cur;
    }
    return NANOCBOR_OK;
}

static int _skip_simple(nanocbor_value_t *it)
{
    int type = nanocbor_get_type(it);
    uint64_t tmp = 0;
    if (type == NANOCBOR_TYPE_BSTR || type == NANOCBOR_TYPE_TSTR) {
        const uint8_t *tmp = NULL;
        size_t len = 0;
        return _get_str(it, &tmp, &len, (uint8_t)type);
    }
    int res = _get_uint64(it, &tmp, NANOCBOR_SIZE_LONG, type);
    res = _advance_if(it, res);
    return res > 0 ? NANOCBOR_OK : res;
}

int nanocbor_get_subcbor(nanocbor_value_t *it, const uint8_t **start,
                         size_t *len)
{
    *start = it->cur;
    int res = nanocbor_skip(it);
    *len = (size_t)(it->cur - *start);
    return res;
}

int nanocbor_skip_simple(nanocbor_value_t *it)
{
    return _skip_simple(it);
}

/* NOLINTNEXTLINE(misc-no-recursion): Recursion is limited by design */
static int _skip_limited(nanocbor_value_t *it, uint8_t limit)
{
    if (limit == 0) {
        return NANOCBOR_ERR_RECURSION;
    }
    int type = nanocbor_get_type(it);
    int res = type;

    /* map or array */
    if (type == NANOCBOR_TYPE_ARR || type == NANOCBOR_TYPE_MAP) {
        nanocbor_value_t recurse;
        res = (type == NANOCBOR_TYPE_MAP ? nanocbor_enter_map(it, &recurse)
                                         : nanocbor_enter_array(it, &recurse));
        if (res == NANOCBOR_OK) {
            while (!nanocbor_at_end(&recurse)) {
                res = _skip_limited(&recurse, limit - 1);
                if (res < 0) {
                    break;
                }
            }
            nanocbor_leave_container(it, &recurse);
        }
    }
    /* tag */
    else if (type == NANOCBOR_TYPE_TAG) {
        uint64_t tmp = 0;
        int res = _get_uint64(it, &tmp, NANOCBOR_SIZE_WORD, NANOCBOR_TYPE_TAG);
        if (res >= 0) {
            it->cur += res;
            res = _skip_limited(it, limit - 1);
        }
    }
    /* other basic types */
    else if (type >= 0) {
        res = _skip_simple(it);
    }
    return res < 0 ? res : NANOCBOR_OK;
}

int nanocbor_skip(nanocbor_value_t *it)
{
    return _skip_limited(it, NANOCBOR_RECURSION_MAX);
}

// todo: start could be const?
int nanocbor_get_key_tstr(nanocbor_value_t *start, const char *key,
                          nanocbor_value_t *value)
{
    int res = NANOCBOR_NOT_FOUND;
    size_t len = strlen(key);
    *value = *start;

    while (!nanocbor_at_end(value)) {
        const uint8_t *s = NULL;
        size_t s_len = 0;

        res = nanocbor_get_tstr(value, &s, &s_len);
        if (res < 0) {
            break;
        }

        if (s_len == len && !strncmp(key, (const char *)s, len)) {
            res = NANOCBOR_OK;
            break;
        }

        res = nanocbor_skip(value);
        if (res < 0) {
            break;
        }
    }

    return res;
}
