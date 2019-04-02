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
#include "nanocbor/nanocbor.h"
#include "nanocbor/config.h"
#include NANOCBOR_BYTEORDER_HEADER

size_t nanocbor_fmt_bool(uint8_t *buf, bool content)
{
    *buf = NANOCBOR_MASK_FLOAT | (content ? NANOCBOR_SIMPLE_TRUE
                                          : NANOCBOR_SIMPLE_FALSE);
    return 1;
}

static size_t _fmt_uint32(uint8_t *buf, uint32_t num, uint8_t type)
{
    if (num < 23) {
        *buf = type | num;
        return 1;
    }
    else if (num <= UINT8_MAX) {
        *buf++ = type | NANOCBOR_SIZE_BYTE;
        *buf = num;
        return 2;
    }
    else if (num <= UINT16_MAX) {
        *buf++ = type | NANOCBOR_SIZE_SHORT;
        *buf++ = ((num & 0xff00) >> 8) & 0xff;
        *buf =  num & 0x00ff;
        return 3;
    }
    else {
        *buf++ = type | NANOCBOR_SIZE_WORD;
        uint32_t bnum = NANOCBOR_HTOBE32_FUNC(num);
        memcpy(buf, &bnum, sizeof(bnum));
        return 5;
    }
}

size_t nanocbor_fmt_uint(uint8_t *buf, uint32_t num)
{
    return _fmt_uint32(buf, num, NANOCBOR_MASK_UINT);
}

size_t nanocbor_fmt_int(uint8_t *buf, int32_t num)
{
    if (num < 0) {
        /* Always negative at this point */
        num = -1 * (num + 1);
        return _fmt_uint32(buf, num, NANOCBOR_MASK_NINT);
    }
    else {
        return nanocbor_fmt_uint(buf, (uint32_t)num);
    }
}

size_t nanocbor_fmt_bstr(uint8_t *buf, size_t len)
{
    return _fmt_uint32(buf, (uint32_t)len, NANOCBOR_MASK_BSTR);
}

size_t nanocbor_fmt_tstr(uint8_t *buf, size_t len)
{
    return _fmt_uint32(buf, (uint32_t)len, NANOCBOR_MASK_TSTR);
}

size_t nanocbor_put_tstr(uint8_t *buf, char *str)
{
    size_t len = strlen(str);
    size_t hdrlen = nanocbor_fmt_tstr(buf, len);

    memcpy(buf + hdrlen, str, len);
    return hdrlen + len;
}

size_t nanocbor_put_bstr(uint8_t *buf, char *str, size_t len)
{
    size_t hdrlen = nanocbor_fmt_bstr(buf, len);

    memcpy(buf + hdrlen, str, len);
    return hdrlen + len;
}

size_t nanocbor_fmt_array(uint8_t *buf, size_t len)
{
    return _fmt_uint32(buf, (uint32_t)len, NANOCBOR_MASK_ARR);
}

size_t nanocbor_fmt_map(uint8_t *buf, size_t len)
{
    return _fmt_uint32(buf, (uint32_t)len, NANOCBOR_MASK_MAP);
}

size_t nanocbor_fmt_array_indefinite(uint8_t *buf)
{
    *buf = NANOCBOR_MASK_ARR | NANOCBOR_SIZE_INDEFINITE;
    return 1;
}

size_t nanocbor_fmt_map_indefinite(uint8_t *buf)
{
    *buf = NANOCBOR_MASK_ARR | NANOCBOR_SIZE_INDEFINITE;
    return 1;
}

size_t nanocbor_fmt_end_indefinite(uint8_t *buf)
{
    /* End is marked with float major and indefinite minor number */
    *buf = NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_INDEFINITE;
    return 1;
}

size_t nanocbor_fmt_null(uint8_t *buf)
{
    *buf = NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_NULL;
    return 1;
}

#define FLOAT_EXP_OFFSET   (127U)
#define HALF_EXP_OFFSET     (15U)

size_t nanocbor_fmt_float(uint8_t *buf, float num)
{
    /* Check if lower 13 bits of fraction are zero, if so we might be able to
     * convert without precision loss */
    uint32_t *unum = (uint32_t *)&num;

#ifdef MODULE_NANOCBOR_HALFFLOAT
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
        return 3;
    }
#endif
    /* normal float */
    *buf++ = NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_WORD;
    uint32_t bnum = NANOCBOR_HTOBE32_FUNC(*unum);
    memcpy(buf, &bnum, sizeof(bnum));
    return 5;
}
