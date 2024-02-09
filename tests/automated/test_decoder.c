/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include "nanocbor/config.h"
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
    CU_ASSERT_EQUAL(nanocbor_get_decimal_frac(&decoder, &exponent, &mantissa),
                    0);
    CU_ASSERT_EQUAL(exponent, -2);
    CU_ASSERT_EQUAL(mantissa, 27315);
}

static void _decode_skip(const uint8_t *test_case, size_t test_case_len, bool simple)
{
    nanocbor_value_t decoder;

    nanocbor_decoder_init(&decoder, test_case, test_case_len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&decoder), false);
    int res = nanocbor_skip(&decoder);
    CU_ASSERT_EQUAL(res, NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&decoder), true);

    if (simple) {
        nanocbor_decoder_init(&decoder, test_case, test_case_len);
        CU_ASSERT_EQUAL(nanocbor_at_end(&decoder), false);
        int res = nanocbor_skip_simple(&decoder);
        CU_ASSERT_EQUAL(res, NANOCBOR_OK);
        CU_ASSERT_EQUAL(nanocbor_at_end(&decoder), true);
    }
}

static void test_decode_skip(void)
{
    static const uint8_t test_uint[] = { 0x00 };
    _decode_skip(test_uint, sizeof(test_uint), true);
    static const uint8_t test_nint[] = { 0x20 };
    _decode_skip(test_nint, sizeof(test_nint), true);
    static const uint8_t test_bstr_empty[] = { 0x40 };
    _decode_skip(test_bstr_empty, sizeof(test_bstr_empty), true);
    static const uint8_t test_bstr[] = { 0x42, 0xAA, 0xBB };
    _decode_skip(test_bstr, sizeof(test_bstr), true);
    static const uint8_t test_tstr[] = { 0x65, 0x68, 0x65, 0x6C, 0x6C, 0x6F };
    _decode_skip(test_tstr, sizeof(test_tstr), true);

    static const uint8_t test_float[] = { 0xF9, 0x42, 0x00 };
    _decode_skip(test_float, sizeof(test_float), true);
    static const uint8_t test_simple[] = { 0xF4 };
    _decode_skip(test_simple, sizeof(test_simple), true);

    static const uint8_t test_tag[] = { 0xD8, 0x29, 0x82, 0xF5, 0xF4 };
    _decode_skip(test_tag, sizeof(test_tag), false);
    static const uint8_t test_tag_within_array[] = { 0x81, 0xD8, 0x29, 0x80 };
    _decode_skip(test_tag_within_array, sizeof(test_tag_within_array), false);

    static const uint8_t test_array[] = { 0x81, 0xF6 };
    _decode_skip(test_array, sizeof(test_array), false);
    static const uint8_t test_array_nested[] = { 0x81, 0x81, 0xF6 };
    _decode_skip(test_array_nested, sizeof(test_array_nested), false);

    static const uint8_t test_map[] = { 0xA1, 0x61, 0x61, 0xF6 };
    _decode_skip(test_map, sizeof(test_map), false);
    static const uint8_t test_map_nested[] = { 0xA1, 0x61, 0x61, 0xA1, 0x61, 0x62, 0xF6 };
    _decode_skip(test_map_nested, sizeof(test_map_nested), false);
}

static void test_decode_packed(void)
{
    nanocbor_value_t val, val2, val3, val4;
    const uint8_t *buf;
    size_t len;

    // 113([[], null])
    static const uint8_t empty[] = { 0xD8, 0x71, 0x82, 0x80, 0xF6 };
    nanocbor_decoder_init(&val, empty, sizeof(empty));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([["a", "b"], "c"])
    static const uint8_t unused[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0x61, 0x63 };
    nanocbor_decoder_init(&val, unused, sizeof(unused));
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "c", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([["a", "b"], simple(1)])
    static const uint8_t once[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0xE1 };
    nanocbor_decoder_init(&val, once, sizeof(once));
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "b", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([["a", "b"], [simple(1), simple(0)]])
    static const uint8_t twice[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0x82, 0xE1, 0xE0 };
    nanocbor_decoder_init(&val, twice, sizeof(twice));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 2);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "b", len);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    nanocbor_leave_container(&val, &val2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([[simple(1),"a"], simple(0)])
    static const uint8_t indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xE1, 0x61, 0x61, 0xE0 };
    nanocbor_decoder_init(&val, indirection, sizeof(indirection));
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // todo: split packed tests in extra file
    // todo: get all tests to succeed
    // todo: implement packed for get_{uint,int,float,double,boolean,undefined}, also tests
    // todo: implement packed for get_type -> will likely break stuff
    // todo: implement flags for packed: enable + within_packed for leave_container?

    // 113([["b", "c"], 113([["a"], [simple(0), simple(1), simple(2)]])])
    static const uint8_t nested[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x62, 0x61, 0x63, 0xD8, 0x71, 0x82, 0x81, 0x61, 0x61, 0x83, 0xE0, 0xE1, 0xE2 };
    nanocbor_decoder_init(&val, nested, sizeof(nested));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 3);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "b", len);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "c", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    nanocbor_leave_container(&val, &val2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([[null, 113([["a"], simple(0)])], simple(1)])
    static const uint8_t nested_indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xF6, 0xD8, 0x71, 0x82, 0x81, 0x61, 0x61, 0xE0, 0xE1 };
    nanocbor_decoder_init(&val, nested_indirection, sizeof(nested_indirection));
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([["a", "b"], [{simple(0): simple(1)}, {simple(1): simple(0)}]])
    static const uint8_t map[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0x82, 0xA1, 0xE0, 0xE1, 0xA1, 0xE1, 0xE0 };
    nanocbor_decoder_init(&val, map, sizeof(map));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 2);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_map_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_key_tstr(&val3, "a", &val4), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val4, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "b", len);
    // nanocbor_leave_container(&val2, &val3);
    // todo: above breaks as leave_container does not check being at the end of it, also below
    nanocbor_leave_container(&val2, &val4);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_map_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_key_tstr(&val3, "b", &val4), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val4, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    nanocbor_leave_container(&val2, &val4);
    nanocbor_leave_container(&val, &val2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 41(113([["a"], [simple(0)]]))
    uint32_t tag;
    static const uint8_t within_tag[] = { 0xD8, 0x29, 0xD8, 0x71, 0x82, 0x81, 0x61, 0x61, 0x81, 0xE0 };
    nanocbor_decoder_init(&val, within_tag, sizeof(within_tag));
    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tag), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tag, 41);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 1);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    nanocbor_leave_container(&val, &val2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([[["a"]], simple(0)])
    static const uint8_t shared_container[] = { 0xD8, 0x71, 0x82, 0x81, 0x81, 0x61, 0x61, 0xE0 };
    nanocbor_decoder_init(&val, shared_container, sizeof(shared_container));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 1);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    nanocbor_leave_container(&val, &val2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // // 113([[], simple(0))
    // static const uint8_t undefined_ref[] = { 0xD8, 0x71, 0x82, 0x80, 0xE0 };
    // // 113([[simple(0)], simple(0)])
    // static const uint8_t loop[] = { 0xD8, 0x71, 0x82, 0x81, 0xE0, 0xE0 };
    // // 113([[simple(1), simple(0)], simple(0)])
    // static const uint8_t loop_indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xE1, 0xE0, 0xE0 };
    // // 113([[], 113([[], 113([[], 113([[], null])])])])
    // static const uint8_t max_nesting_exceeded[] = { 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xF6 };
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
        .f = test_decode_skip,
        .n = "CBOR skip test",
    },
    {
        .f = test_decode_packed,
        .n = "CBOR packed test",
    },
    {
        .f = NULL,
        .n = NULL,
    },
};

/* NOLINTEND(cppcoreguidelines-avoid-magic-numbers) */
