/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include "nanocbor/nanocbor.h"
#include "test.h"
#include <CUnit/CUnit.h>

/* NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers) */

static void test_decode_indefinite(void)
{
    /* Test vector, 3 integers in an indefinite array */
    static const uint8_t indefinite[] = { 0x9f, 0x01, 0x02, 0x03, 0xff };

    nanocbor_value_t val;
    nanocbor_value_t cont;

    uint32_t tmp = 0;

    nanocbor_decoder_init(&val, indefinite, sizeof(indefinite));

    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &cont), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_container_indefinite(&cont), true);

    /* Decode the three values */
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);

    CU_ASSERT_EQUAL(nanocbor_get_uint32(&cont, &tmp), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
}

static void test_decode_map(void)
{

    static const uint8_t map_empty[] = { 0xa0 };

    static const uint8_t map_one[] = { 0xa1, 0x01, 0x02 };

    static const uint8_t complex_map_decode[]
        = { 0xa5, 0x01, 0x02, 0x03, 0x80, 0x04, 0x9F,
            0xFF, 0x05, 0x9F, 0xff, 0x06, 0xf6 };

    nanocbor_value_t val;
    nanocbor_value_t cont;

    uint32_t tmp = 0;

    /* Init the decoder and assert the properties of the empty map */
    nanocbor_decoder_init(&val, map_empty, sizeof(map_empty));
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
    nanocbor_leave_container(&val, &cont);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    /* Init the decoder and verify the decoding of the map elements */
    nanocbor_decoder_init(&val, map_one, sizeof(map_one));
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_OK);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 1);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
    nanocbor_leave_container(&val, &cont);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    /* Init the decoder and skip over the empty map */
    nanocbor_decoder_init(&val, map_empty, sizeof(map_empty));
    CU_ASSERT_EQUAL(nanocbor_skip(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    /* Init the decoder and skip over the non-empty map */
    nanocbor_decoder_init(&val, map_one, sizeof(map_one));
    CU_ASSERT_EQUAL(nanocbor_skip(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    nanocbor_value_t array;
    /* Init decoder and start decoding */
    nanocbor_decoder_init(&val, complex_map_decode, sizeof(complex_map_decode));
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_OK);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 1);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 2);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 3);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&array), true);
    nanocbor_leave_container(&cont, &array);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), false);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 4);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&array), true);
    nanocbor_leave_container(&cont, &array);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), false);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 5);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&array), true);
    nanocbor_leave_container(&cont, &array);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), false);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 6);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), false);
    CU_ASSERT_EQUAL(nanocbor_get_null(&cont), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
}

static void test_tag(void)
{
    static const uint8_t arraytag[] = { 0x82, 0xd8, 0x37, 0x01, 0x02 };

    nanocbor_value_t val;
    nanocbor_value_t cont;

    uint32_t tmp = 0x12345678;

    nanocbor_decoder_init(&val, arraytag, sizeof(arraytag));

    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &cont), NANOCBOR_OK);

    CU_ASSERT_EQUAL(nanocbor_get_tag(&cont, &tmp), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tmp, 0x37);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 1);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 2);

    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
}

static void test_double_tag(void)
{
    static const uint8_t arraytag[] = {
        0xD9, 0xD9, 0xF7, // tag(55799)
        0xDA, 0x52, 0x49, 0x4F, 0x54, // tag(1380536148) 'RIOT'
        0x43, // bytes(3) -> 'C'
        0x42, 0x4F, 0x52 // 'BOR'
    };

    nanocbor_value_t val;
    const uint8_t *bytes = NULL;
    size_t bytes_len = 0;

    uint32_t tmp = 0x12345678;

    nanocbor_decoder_init(&val, arraytag, sizeof(arraytag));

    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tmp), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tmp, 55799);

    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tmp), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tmp, 1380536148);

    CU_ASSERT_EQUAL(nanocbor_get_bstr(&val, &bytes, &bytes_len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(bytes_len, 3);
    CU_ASSERT_EQUAL(bytes[0], 'B');
    CU_ASSERT_EQUAL(bytes[1], 'O');
    CU_ASSERT_EQUAL(bytes[2], 'R');

    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_decode_none(void)
{
    nanocbor_value_t val;
    nanocbor_value_t cont;
    uint64_t tmp = 0;
    nanocbor_decoder_init(&val, NULL, 0);

    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_uint32(&val, (uint32_t *)&tmp),
                    NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_int32(&val, (int32_t *)&tmp),
                    NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &cont), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, (bool *)&tmp), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_skip(&val), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_skip_simple(&val), NANOCBOR_ERR_END);
}

static void test_decode_basic(void)
{
    nanocbor_value_t decoder;
    uint8_t byteval = 5; /* unsigned integer, value 5 */
    uint32_t value = 0;

    nanocbor_decoder_init(&decoder, &byteval, sizeof(byteval));
    CU_ASSERT_EQUAL(nanocbor_get_type(&decoder), NANOCBOR_TYPE_UINT);
    printf("\"val: %u\"\n", value);
    CU_ASSERT_EQUAL(nanocbor_get_uint32(&decoder, &value), 1);
    printf("\"val: %u\"\n", value);
    CU_ASSERT_EQUAL(5, value);

    int32_t intval = 0;
    nanocbor_decoder_init(&decoder, &byteval, sizeof(byteval));
    CU_ASSERT_EQUAL(nanocbor_get_int32(&decoder, &intval), 1);
    CU_ASSERT_EQUAL(5, intval);

    const uint8_t decimal_frac[] = { 0xC4, 0x82, 0x21, 0x19, 0x6a, 0xb3 };
    int32_t mantissa = 0;
    int32_t exponent = 0;
    nanocbor_decoder_init(&decoder, decimal_frac, sizeof(decimal_frac));
    CU_ASSERT_EQUAL(nanocbor_get_decimal_frac(&decoder, &exponent, &mantissa), 0);
    CU_ASSERT_EQUAL(exponent, -2);
    CU_ASSERT_EQUAL(mantissa, 27315);
}

const test_t tests_decoder[] = {
    {
        .f = test_decode_none,
        .n = "get type on empty buffer",
    },
    {
        .f = test_decode_basic,
        .n = "Simple CBOR integer tests",
    },
    {
        .f = test_decode_indefinite,
        .n = "CBOR indefinite array decode tests",
    },
    {
        .f = test_decode_map,
        .n = "CBOR map decode tests",
    },
    {
        .f = test_tag,
        .n = "CBOR tag decode test",
    },
    {
        .f = test_double_tag,
        .n = "CBOR double tag decode test",
    },
    {
        .f = NULL,
        .n = NULL,
    },
};

/* NOLINTEND(cppcoreguidelines-avoid-magic-numbers) */
