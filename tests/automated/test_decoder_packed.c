/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include "nanocbor/config.h"
#include "nanocbor/nanocbor.h"
#include "test.h"
#include <CUnit/CUnit.h>

/* NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers) */

// todo: decide for + implement semantics for get_subcbor()

#if NANOCBOR_DECODE_PACKED_ENABLED

#undef CU_ASSERT_EQUAL
#define CU_ASSERT_EQUAL(actual, expected) \
  { int ret = (actual); CU_assertImplementation((ret == (expected)), __LINE__, ("CU_ASSERT_EQUAL(" #actual "," #expected ")"), __FILE__, "", CU_FALSE); if (ret != (expected)) printf("was %d\n", ret); }

static void test_packed_follow_reference_by_simple(void)
{
    nanocbor_value_t val;
    bool b;

    // simple(0), simple(1)
    static const uint8_t cbor[] = { 0xE0, 0xE1 };
    // [true,false]
    static const uint8_t table[] = { 0x82, 0xF5, 0xF4 };

    CU_ASSERT_EQUAL(nanocbor_decoder_init_packed_table(&val, cbor, sizeof(cbor), table, sizeof(table)), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, false);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_follow_reference_by_tag(void)
{
    nanocbor_value_t val;
    bool b;

    // 6(0), 6(-1), 6(simple(0))
    static const uint8_t cbor[] = { 0xC6, 0x00, 0xC6, 0x20, 0xC6, 0xE0 };
    // [0,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,true,false]
    static const uint8_t table[] = { 0x92, 0x00, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF5, 0xF4 };

    nanocbor_decoder_init_packed_table(&val, cbor, sizeof(cbor), table, sizeof(table));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, false);
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_follow_reference_getters(void)
{
    nanocbor_value_t val;

    // sequence of simple(0) - simple(8)
    static const uint8_t cbor[] = { 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8 };
    // [42, -42, 3.14159, simple(255), undefined, null, true, "a", h'C0']
    static const uint8_t table[] = {
        0x89, 0x18, 0x2A, 0x38, 0x29, 0xFB, 0x40, 0x09, 0x21, 0xF9, 0xF0, 0x1B,
        0x86, 0x6E, 0xF8, 0xFF, 0xF7, 0xF6, 0xF5, 0x61, 0x61, 0x41, 0xC0 };

    nanocbor_decoder_init_packed_table(&val, cbor, sizeof(cbor), table, sizeof(table));

    // uint 42
    uint8_t uint;
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_UINT);
    CU_ASSERT_EQUAL(nanocbor_get_uint8(&val, &uint), 2); // len(CBOR(42)) = 2
    CU_ASSERT_EQUAL(uint, 42);

    // nint -42
    int8_t nint;
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_NINT);
    CU_ASSERT_EQUAL(nanocbor_get_int8(&val, &nint), 2);  // len(CBOR(-42)) = 2
    CU_ASSERT_EQUAL(nint, -42);

    // double 3.14159
    double_t dbl;
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_double(&val, &dbl), 9); // len(CBOR double-precision float)
    CU_ASSERT_DOUBLE_EQUAL(dbl, 3.14159, 0.0000001);

    // simple 255
    uint8_t simple;
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_simple(&val, &simple), NANOCBOR_OK);
    CU_ASSERT_EQUAL(simple, 255);

    // undefined
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_undefined(&val), NANOCBOR_OK);

    // null
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);

    // bool true
    bool b;
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);

    // tstr "a"
    const uint8_t *tstr;
    size_t tstr_len;
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_TSTR);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &tstr, &tstr_len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tstr_len, 1);
    CU_ASSERT_NSTRING_EQUAL(tstr, "a", tstr_len);

    // bstr h'C0'
    const uint8_t *bstr;
    size_t bstr_len;
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_BSTR);
    CU_ASSERT_EQUAL(nanocbor_get_bstr(&val, &bstr, &bstr_len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(bstr_len, 1);
    CU_ASSERT_EQUAL(bstr[0], 0xC0);

    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_follow_reference_containers(void)
{
    nanocbor_value_t val, val2, val3;
    bool b;

    (void)val3; (void)b;

    // sequence of simple(0) - simple(2)
    static const uint8_t cbor[] = { 0xE0, 0xE1, 0xE2 };
    // [[null], [[true], [false]], {null: [null]}]
    static const uint8_t table[] = {
        0x83, 0x81, 0xF6, 0x82, 0x81, 0xF5, 0x81, 0xF4, 0xA1, 0xF6, 0x81, 0xF6 };

    nanocbor_decoder_init_packed_table(&val, cbor, sizeof(cbor), table, sizeof(table));

    // array [null]
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_ARR);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 1);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);

    // nested array [[true], [false]]
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_ARR);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 2);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val3, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val3), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val3, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, false);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val3), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);

    // map {null: [null]}
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_MAP);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_map_items_remaining(&val2), 1);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val3), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);

    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_follow_reference_with_indirection(void)
{
    nanocbor_value_t val;

    // simple(0)
    static const uint8_t cbor[] = { 0xE0 };

    // [simple(1),null]
    static const uint8_t table[] = { 0x82, 0xE1, 0xF6 };

    nanocbor_decoder_init_packed_table(&val, cbor, sizeof(cbor), table, sizeof(table));

    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_enable(void)
{
    nanocbor_value_t val, val2, val3;
    uint32_t tag;
    uint8_t simple;

    // 113([[null], simple(0)])
    static const uint8_t cbor[] = { 0xD8, 0x71, 0x82, 0x81, 0xF6, 0xE0 };

    // disabled packed CBOR support
    nanocbor_decoder_init(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_TAG);
    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tag), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tag, 113);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val3), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_simple(&val2, &simple), NANOCBOR_OK);
    CU_ASSERT_EQUAL(simple, 0);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // enabled packed CBOR support
    nanocbor_decoder_init_packed(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_table_setup_empty_or_unused(void)
{
    nanocbor_value_t val;
    bool b;

    // 113([[], null]), 113([[true], false])
    static const uint8_t cbor[] = { 0xD8, 0x71, 0x82, 0x80, 0xF6, 0xD8, 0x71, 0x82, 0x81, 0xF5, 0xF4 };

    nanocbor_decoder_init_packed(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, false);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_table_setup_within_tag(void)
{
    nanocbor_value_t val;
    uint32_t tag;

    // 41(113([[null], simple(0)]))
    static const uint8_t cbor[] = { 0xD8, 0x29, 0xD8, 0x71, 0x82, 0x81, 0xF6, 0xE0 };

    nanocbor_decoder_init_packed(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_TAG);
    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tag), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tag, 41);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_table_setup_with_indefinite_length(void)
{
    nanocbor_value_t val;

    // 113([[null], simple(0)])
    static const uint8_t cbor1[] = { 0xD8, 0x71, 0x82, 0x9F, 0xF6, 0xFF, 0xE0 };

    nanocbor_decoder_init_packed(&val, cbor1, sizeof(cbor1));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([[null], 113([[false], simple(1)])])
    static const uint8_t cbor2[] = { 0xD8, 0x71, 0x82, 0x81, 0xF6, 0xD8, 0x71, 0x82, 0x9F, 0xF4, 0xFF, 0xE1 };

    nanocbor_decoder_init_packed(&val, cbor2, sizeof(cbor2));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113([[null], simple(0)])
    static const uint8_t cbor3[] = { 0xD8, 0x71, 0x9F, 0x81, 0xF6, 0xE0, 0xFF };

    nanocbor_decoder_init_packed(&val, cbor3, sizeof(cbor3));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_table_setup_nested(void)
{
    nanocbor_value_t val, val2;
    bool b;

    // 113([[false, true], 113([[null], [simple(0), simple(2), simple(1)]])])
    static const uint8_t cbor[] = { 0xD8, 0x71, 0x82, 0x82, 0xF4, 0xF5, 0xD8, 0x71, 0x82, 0x81, 0xF6, 0x83, 0xE0, 0xE2, 0xE1 };

    nanocbor_decoder_init_packed(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_ARR);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 3);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val2, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val2, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, false);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_table_setup_nested_within_table(void)
{
    nanocbor_value_t val;

    // 113([[null, 113([[undefined], simple(0)])], simple(1)])
    static const uint8_t cbor[] = { 0xD8, 0x71, 0x82, 0x82, 0xF6, 0xD8, 0x71, 0x82, 0x81, 0xF7, 0xE0, 0xE1 };

    nanocbor_decoder_init_packed(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_undefined(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_table_setup_with_packed_table(void)
{
    nanocbor_value_t val;

    // 113([simple(0), simple(0)])
    static const uint8_t cbor[] = { 0xD8, 0x71, 0x82, 0xE0, 0xE0 };
    // [[null]]
    static const uint8_t table[] = { 0x81, 0x81, 0xF6 };

    nanocbor_decoder_init_packed_table(&val, cbor, sizeof(cbor), table, sizeof(table));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // 113(simple(0))
    static const uint8_t cbor2[] = { 0xD8, 0x71, 0xE0 };
    // [[[null], simple(0)]]
    static const uint8_t table2[] = { 0x81, 0x82, 0x81, 0xF6, 0xE0 };

    nanocbor_decoder_init_packed_table(&val, cbor2, sizeof(cbor2), table2, sizeof(table2));
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_TYPE_FLOAT);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_error_undefined_table(void)
{
    nanocbor_value_t val;

    // simple(0)
    static const uint8_t cbor[] = { 0xE0 };

    nanocbor_decoder_init_packed(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_PACKED_UNDEFINED_REFERENCE);
}


static void test_packed_error_undefined_reference(void)
{
    nanocbor_value_t val;

    // 113([[], simple(0))
    static const uint8_t cbor[] = { 0xD8, 0x71, 0x82, 0x80, 0xE0 };

    nanocbor_decoder_init_packed(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_PACKED_UNDEFINED_REFERENCE);
}

static void test_packed_error_reference_loop(void)
{
    nanocbor_value_t val, val2;
    bool b;
    uint8_t u;
    int8_t i;
    float_t f;
    const uint8_t *buf;
    size_t len;
    uint32_t t;

    // simple(0), simple(2)
    static const uint8_t cbor[] = { 0xE0, 0xE2 };

    // [simple(0), simple(2), simple(1)]
    static const uint8_t table[] = { 0x83, 0xE0, 0xE2, 0xE1 };

    nanocbor_decoder_init_packed_table(&val, cbor, sizeof(cbor), table, sizeof(table));

    // immediate loop
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_undefined(&val), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_uint8(&val, &u), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_int8(&val, &i), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_float(&val, &f), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_bstr(&val, &buf, &len), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &t), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_simple(&val, &u), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &val2), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_skip(&val), NANOCBOR_OK);

    // indirect loop
    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_undefined(&val), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_uint8(&val, &u), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_int8(&val, &i), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_float(&val, &f), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_bstr(&val, &buf, &len), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &t), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_get_simple(&val, &u), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &val2), NANOCBOR_ERR_RECURSION);
    CU_ASSERT_EQUAL(nanocbor_skip(&val), NANOCBOR_OK);

    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_error_max_nesting_exceeded(void)
{
    nanocbor_value_t val;

    // 113([[], 113([[], 113([[], 113([[], null])])])])
    static const uint8_t cbor[] = { 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xF6 };

    nanocbor_decoder_init_packed(&val, cbor, sizeof(cbor));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_PACKED_MEMORY);
}

static void test_packed_error_invalid_table(void)
{
    nanocbor_value_t val;

    // 113([null, simple(0)])
    static const uint8_t cbor1[] = { 0xD8, 0x71, 0x82, 0xF6, 0xE0 };

    nanocbor_decoder_init_packed(&val, cbor1, sizeof(cbor1));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_PACKED_FORMAT);

    // simple(0)
    static const uint8_t cbor2[] = { 0xE0 };
    // null
    static const uint8_t table[] = { 0xF6 };

    CU_ASSERT_EQUAL(nanocbor_decoder_init_packed_table(&val, cbor2, sizeof(cbor2), table, sizeof(table)), NANOCBOR_ERR_PACKED_FORMAT);

    CU_ASSERT_EQUAL(nanocbor_decoder_init_packed_table(&val, cbor2, sizeof(cbor2), table, 0), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_PACKED_UNDEFINED_REFERENCE);

    // 113([[], simple(0), null])
    static const uint8_t cbor3[] = { 0xD8, 0x71, 0x83, 0x80, 0xE0, 0xF6 };

    nanocbor_decoder_init_packed(&val, cbor3, sizeof(cbor3));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_PACKED_FORMAT);
}

const test_t tests_decoder_packed[] = {
    {
        .f = test_packed_follow_reference_by_simple,
        .n = "CBOR packed follow reference via simple value test",
    },
    {
        .f = test_packed_follow_reference_by_tag,
        .n = "CBOR packed follow reference via tag 6 test",
    },
    {
        .f = test_packed_follow_reference_getters,
        .n = "CBOR packed follow reference for nanocbor_get_* functions test",
    },
    {
        .f = test_packed_follow_reference_containers,
        .n = "CBOR packed follow reference for nanocbor_enter_* functions test",
    },
    {
        .f = test_packed_follow_reference_with_indirection,
        .n = "CBOR packed follow indirect reference test",
    },
    {
        .f = test_packed_enable,
        .n = "CBOR packed enable support test",
    },
    {
        .f = test_packed_table_setup_empty_or_unused,
        .n = "CBOR packed empty or unused shared item table test",
    },
    {
        .f = test_packed_table_setup_within_tag,
        .n = "CBOR packed shared item table within tag test",
    },
    {
        .f = test_packed_table_setup_with_indefinite_length,
        .n = "CBOR packed shared item table with indefinite length arrays test",
    },
    {
        .f = test_packed_table_setup_nested,
        .n = "CBOR packed nested shared item tables test",
    },
    {
        .f = test_packed_table_setup_nested_within_table,
        .n = "CBOR packed nested shared item table definition within table definition test",
    },
    {
        .f = test_packed_table_setup_with_packed_table,
        .n = "CBOR packed with itself packed table definition test",
    },
    {
        .f = test_packed_error_undefined_table,
        .n = "CBOR packed undefined table test",
    },
    {
        .f = test_packed_error_undefined_reference,
        .n = "CBOR packed undefined reference test",
    },
    {
        .f = test_packed_error_reference_loop,
        .n = "CBOR packed reference loop test",
    },
    {
        .f = test_packed_error_max_nesting_exceeded,
        .n = "CBOR packed max nesting exceeded test",
    },
        {
        .f = test_packed_error_invalid_table,
        .n = "CBOR packed invalid table definition test",
    },
    {
        .f = NULL,
        .n = NULL,
    },
};

#else /* !NANOCBOR_DECODE_PACKED_ENABLED */

const test_t tests_decoder_packed[] = {
    {
        .f = NULL,
        .n = NULL,
    },
};

#endif

/* NOLINTEND(cppcoreguidelines-avoid-magic-numbers) */
