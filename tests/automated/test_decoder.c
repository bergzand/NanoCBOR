/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include "test.h"
#include "nanocbor/nanocbor.h"
#include <CUnit/CUnit.h>

static void test_decode_indefinite(void)
{
    /* Test vector, 3 integers in an indefinite array */
    static const uint8_t indefinite[] = {
        0x9f, 0x01, 0x02, 0x03, 0xff
    };

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

    static const uint8_t map_empty[] = {
        0xa0
    };

    static const uint8_t map_one[] = {
        0xa1, 0x01, 0x02
    };

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
}

static void test_tag(void)
{
    static const uint8_t arraytag[] = {
        0x82, 0xc1, 0x01, 0x02
    };

    nanocbor_value_t val;
    nanocbor_value_t cont;

    uint32_t tmp = 0;

    nanocbor_decoder_init(&val, arraytag, sizeof(arraytag));

    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &cont), NANOCBOR_OK);

    CU_ASSERT_EQUAL(nanocbor_get_tag(&cont, &tmp), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tmp, 1);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 1);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 2);

    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
}

static void test_decode_none(void)
{
    nanocbor_value_t val;
    nanocbor_value_t cont;
    uint64_t tmp;
    nanocbor_decoder_init(&val, NULL, 0);

    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_uint32(&val, (uint32_t*)&tmp), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_int32(&val, (int32_t*)&tmp), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &cont), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, (bool*)&tmp), NANOCBOR_ERR_END);
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
    int32_t m;
    int32_t e;
    nanocbor_decoder_init(&decoder, decimal_frac, sizeof(decimal_frac));
    CU_ASSERT_EQUAL(nanocbor_get_decimal_frac(&decoder, &e, &m), 0);
    CU_ASSERT_EQUAL(e, -2);
    CU_ASSERT_EQUAL(m, 27315);
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
        .f = NULL,
        .n = NULL,
    }
};
