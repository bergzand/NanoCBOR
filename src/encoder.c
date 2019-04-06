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
 * @brief   Minimalistic CBOR encoder implementation
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

void nanocbor_encoder_init(nanocbor_encoder_t *enc, uint8_t *buf, size_t len)
{
    enc->len = 0;
    enc->cur = buf;
    enc->end = buf + len;
}

size_t nanocbor_encoded_len(nanocbor_encoder_t *enc)
{
    return enc->len;
}

static int _fits(nanocbor_encoder_t *enc, size_t len)
{
    enc->len += len;
    return ((size_t)(enc->end - enc->cur) >= len) ? 0 : -1;
}

static int _fmt_single(nanocbor_encoder_t *enc, uint8_t single)
{
    int res = _fits(enc, 1);

    if (res == 0) {
        *enc->cur++ = single;
    }
    return res;
}

int nanocbor_fmt_bool(nanocbor_encoder_t *enc, bool content)
{
    uint8_t single = NANOCBOR_MASK_FLOAT | (content ? NANOCBOR_SIMPLE_TRUE
                                            : NANOCBOR_SIMPLE_FALSE);

    return _fmt_single(enc, single);
}

static int _fmt_uint64(nanocbor_encoder_t *enc, uint64_t num, uint8_t type)
{
    unsigned extrabytes = 0;

    if (num < NANOCBOR_SIZE_BYTE) {
        type |= num;
    }
    else {
        if (num > UINT32_MAX) {
            /* Requires long size */
            type |= NANOCBOR_SIZE_LONG;
            extrabytes = sizeof(uint64_t);
        }
        else if (num > UINT16_MAX) {
            /* At least word size */
            type |= NANOCBOR_SIZE_WORD;
            extrabytes = sizeof(uint32_t);
        }
        else if (num > UINT8_MAX) {
            type |= NANOCBOR_SIZE_SHORT;
            extrabytes = sizeof(uint16_t);
        }
        else {
            type |= NANOCBOR_SIZE_BYTE;
            extrabytes = sizeof(uint8_t);
        }
    }
    int res = _fits(enc, extrabytes + 1);
    if (res == 0) {
        *enc->cur++ = type;

        /* NOLINTNEXTLINE: user supplied function */
        uint64_t benum = NANOCBOR_HTOBE64_FUNC(num);

        memcpy(enc->cur, (uint8_t *)&benum + sizeof(benum) - extrabytes,
               extrabytes);
        enc->cur += extrabytes;
    }
    return res;
}

int nanocbor_fmt_uint(nanocbor_encoder_t *enc, uint64_t num)
{
    return _fmt_uint64(enc, num, NANOCBOR_MASK_UINT);
}

int nanocbor_fmt_tag(nanocbor_encoder_t *enc, uint64_t num)
{
    return _fmt_uint64(enc, num, NANOCBOR_MASK_TAG);
}

int nanocbor_fmt_int(nanocbor_encoder_t *enc, int64_t num)
{
    if (num < 0) {
        /* Always negative at this point */
        num = -1 * (num + 1);
        return _fmt_uint64(enc, num, NANOCBOR_MASK_NINT);
    }
    return nanocbor_fmt_uint(enc, (uint64_t)num);
}

int nanocbor_fmt_bstr(nanocbor_encoder_t *enc, size_t len)
{
    return _fmt_uint64(enc, (uint64_t)len, NANOCBOR_MASK_BSTR);
}

int nanocbor_fmt_tstr(nanocbor_encoder_t *enc, size_t len)
{
    return _fmt_uint64(enc, (uint64_t)len, NANOCBOR_MASK_TSTR);
}

static int _put_bytes(nanocbor_encoder_t *enc, uint8_t *str, size_t len)
{
    int res = _fits(enc, len);

    if (res == 0) {
        memcpy(enc->cur, str, len);
        enc->cur += len;
    }
    return res;
}

int nanocbor_put_tstr(nanocbor_encoder_t *enc, char *str)
{
    size_t len = strlen(str);

    nanocbor_fmt_tstr(enc, len);
    return _put_bytes(enc, (uint8_t *)str, len);
}

int nanocbor_put_bstr(nanocbor_encoder_t *enc, uint8_t *str, size_t len)
{
    nanocbor_fmt_bstr(enc, len);
    return _put_bytes(enc, str, len);
}

int nanocbor_fmt_array(nanocbor_encoder_t *enc, size_t len)
{
    return _fmt_uint64(enc, (uint64_t)len, NANOCBOR_MASK_ARR);
}

int nanocbor_fmt_map(nanocbor_encoder_t *enc, size_t len)
{
    return _fmt_uint64(enc, (uint64_t)len, NANOCBOR_MASK_MAP);
}

int nanocbor_fmt_array_indefinite(nanocbor_encoder_t *enc)
{
    return _fmt_single(enc, NANOCBOR_MASK_ARR | NANOCBOR_SIZE_INDEFINITE);
}

int nanocbor_fmt_map_indefinite(nanocbor_encoder_t *enc)
{
    return _fmt_single(enc, NANOCBOR_MASK_MAP | NANOCBOR_SIZE_INDEFINITE);
}

int nanocbor_fmt_end_indefinite(nanocbor_encoder_t *enc)
{
    /* End is marked with float major and indefinite minor number */
    return _fmt_single(enc, NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_INDEFINITE);
}

int nanocbor_fmt_null(nanocbor_encoder_t *enc)
{
    return _fmt_single(enc, NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_NULL);
}

#define FLOAT_EXP_OFFSET   (127U)
#define HALF_EXP_OFFSET     (15U)

int nanocbor_fmt_float(nanocbor_encoder_t *enc, float num)
{
    /* Check if lower 13 bits of fraction are zero, if so we might be able to
     * convert without precision loss */
    uint32_t *unum = (uint32_t *)&num;

#if 0
    uint8_t exp = (*unum >> 23) & 0xff;
    /* Copy sign bit */
    uint16_t half = ((*unum >> 16) & 0x8000);
    if ((*unum & 0x1FFF) == 0 &&  ((
                                       /* Exponent within range for half float */
                                       exp <= 15 + FLOAT_EXP_OFFSET &&
                                       exp >= -14 + FLOAT_EXP_OFFSET) ||
                                   /* Inf or NaN */
                                   exp == 255 || exp == 0)) {
        /* Shift exponent */
        if (exp != 255 && exp != 0) {
            exp = exp + HALF_EXP_OFFSET - FLOAT_EXP_OFFSET;
        }
        /* Add exp */
        half |= ((exp & 0x1F) << 10) | ((*unum >> 13) & 0x03FF);
        *buf++ = NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_SHORT;
        *buf++ = ((half & 0xff00) >> 8) & 0xff;
        *buf =  half & 0x00ff;
        return 0;
    }
#endif
    /* normal float */
    int res = _fits(enc, 1 + sizeof(float));
    if (res == 0) {
        *enc->cur++ = NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_WORD;
        /* NOLINTNEXTLINE: user supplied function */
        uint32_t bnum = NANOCBOR_HTOBE32_FUNC(*unum);
        memcpy(enc->cur, &bnum, sizeof(bnum));
        enc->cur += sizeof(float);
    }
    return res;
}
