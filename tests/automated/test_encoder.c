/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include "nanocbor/nanocbor.h"
#include "test.h"
#include <CUnit/CUnit.h>
#include <float.h>
#include <math.h>

static void print_bytestr(const uint8_t *bytes, size_t len)
{
    printf("\n");
    for (unsigned int idx = 0; idx < len; idx++) {
        printf("%02X", bytes[idx]);
    }
    printf("\n");
}

static void test_encode_float_specials(void)
{
    uint8_t buf[64];
    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buf, sizeof(buf));

    nanocbor_fmt_array_indefinite(&enc);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, NAN), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, -NAN), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, INFINITY), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, -INFINITY), 3);

    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, NAN), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, -NAN), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, INFINITY), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, -INFINITY), 3);
    nanocbor_fmt_end_indefinite(&enc);
    print_bytestr(buf, nanocbor_encoded_len(&enc));
}

static void test_encode_float_to_half(void)
{
    // NOLINTBEGIN
    uint8_t buf[64];
    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buf, sizeof(buf));

    nanocbor_fmt_array_indefinite(&enc);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, 1.75), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, 1.9990234375), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, 1.99951171875), 5);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, 2.0009765625), 5);

    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, -1.75), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, -1.9990234375), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, -1.99951171875), 5);
    CU_ASSERT_EQUAL(nanocbor_fmt_float(&enc, -2.0009765625), 5);
    // NOLINTEND

    nanocbor_fmt_end_indefinite(&enc);
    print_bytestr(buf, nanocbor_encoded_len(&enc));
}

static void test_encode_double_to_float(void)
{
    uint8_t buf[128];
    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buf, sizeof(buf));

    nanocbor_fmt_array_indefinite(&enc);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, 1.75), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, 1.9990234375), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, 1.99951171875), 5);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, 2.0009765625), 5);

    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, -1.75), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, -1.9990234375), 3);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, -1.99951171875), 5);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, -2.0009765625), 5);

    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, 1.00000011920928955078125), 5);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, DBL_MIN), 9);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, DBL_MAX), 9);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, 1e39), 9);

    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, FLT_MIN), 5);
    CU_ASSERT_EQUAL(nanocbor_fmt_double(&enc, FLT_MAX), 5);

    nanocbor_fmt_end_indefinite(&enc);
    print_bytestr(buf, nanocbor_encoded_len(&enc));
}

static void test_encode_simple(void)
{
    uint8_t buf[128];
    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buf, sizeof(buf));

    nanocbor_fmt_array_indefinite(&enc);
    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 0), 1);
    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 15), 1);
    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 16), 1);
    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 19), 1);

    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 20), NANOCBOR_ERR_INVALID_TYPE);
    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 24), NANOCBOR_ERR_INVALID_TYPE);
    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 31), NANOCBOR_ERR_INVALID_TYPE);

    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 32), 2);
    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 128), 2);
    CU_ASSERT_EQUAL(nanocbor_fmt_simple(&enc, 255), 2);

    nanocbor_fmt_end_indefinite(&enc);
    print_bytestr(buf, nanocbor_encoded_len(&enc));
}

const test_t tests_encoder[] = {
    {
        .f = test_encode_float_specials,
        .n = "Float reduction encoder test",
    },
    {
        .f = test_encode_float_to_half,
        .n = "Float reduction encoder test",
    },
    {
        .f = test_encode_double_to_float,
        .n = "Double reduction encoder test",
    },
    {
        .f = test_encode_simple,
        .n = "Simple value encoder test",
    },
    {
        .f = NULL,
        .n = NULL,
    },
};
