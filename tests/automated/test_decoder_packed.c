/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include "nanocbor/config.h"
#include "nanocbor/nanocbor.h"
#include "test.h"
#include <CUnit/CUnit.h>

/* NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers) */

// todo: implement packed for get_type -> will likely break stuff, because used internally

static void test_packed_enable(void)
{
    nanocbor_value_t val, val2, val3;
    uint32_t tag;
    uint8_t simple;

    // 113([[null], simple(0)])
    static const uint8_t packed[] = { 0xD8, 0x71, 0x82, 0x81, 0xF6, 0xE0 };

    // disabled packed CBOR support
    nanocbor_decoder_init(&val, packed, sizeof(packed));
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
    nanocbor_decoder_init_packed(&val, packed, sizeof(packed));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_empty(void)
{
    nanocbor_value_t val;

    // 113([[], null])
    static const uint8_t empty[] = { 0xD8, 0x71, 0x82, 0x80, 0xF6 };
    nanocbor_decoder_init_packed(&val, empty, sizeof(empty));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_unused(void)
{
    nanocbor_value_t val;
    const uint8_t *buf;
    size_t len;

    // 113([["a", "b"], "c"])
    static const uint8_t unused[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0x61, 0x63 };
    nanocbor_decoder_init_packed(&val, unused, sizeof(unused));
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "c", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}


static void test_packed_uint(void)
{
    nanocbor_value_t val;
    uint8_t num;

    // 113([[42], simple(0)])
    static const uint8_t uint[] = { 0xD8, 0x71, 0x82, 0x81, 0x18, 0x2A, 0xE0 };
    nanocbor_decoder_init_packed(&val, uint, sizeof(uint));
    CU_ASSERT_EQUAL(nanocbor_get_uint8(&val, &num), 2); // len(CBOR(42)) = 2
    CU_ASSERT_EQUAL(num, 42);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_nint(void)
{
    nanocbor_value_t val;
    int8_t num;

    // 113([[-42], simple(0)])
    static const uint8_t uint[] = { 0xD8, 0x71, 0x82, 0x81, 0x38, 0x29, 0xE0 };
    nanocbor_decoder_init_packed(&val, uint, sizeof(uint));
    CU_ASSERT_EQUAL(nanocbor_get_int8(&val, &num), 2);  // len(CBOR(-42)) = 2
    CU_ASSERT_EQUAL(num, -42);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_float(void)
{
    nanocbor_value_t val;
    double_t num;

    // 113([[3.14159], simple(0)])
    static const uint8_t _float[] = { 0xD8, 0x71, 0x82, 0x81, 0xFB, 0x40, 0x09, 0x21, 0xF9, 0xF0, 0x1B, 0x86, 0x6E, 0xE0 };
    nanocbor_decoder_init_packed(&val, _float, sizeof(_float));
    CU_ASSERT_EQUAL(nanocbor_get_double(&val, &num), 9); // len(CBOR double-precision float)
    CU_ASSERT_DOUBLE_EQUAL(num, 3.14159, 0.0000001);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_undefined(void)
{
    nanocbor_value_t val;

    // 113([[undefined], simple(0)])
    static const uint8_t undefined[] = { 0xD8, 0x71, 0x82, 0x81, 0xF7, 0xE0 };
    nanocbor_decoder_init_packed(&val, undefined, sizeof(undefined));
    CU_ASSERT_EQUAL(nanocbor_get_undefined(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_null(void)
{
    nanocbor_value_t val;

    // 113([[null], simple(0)])
    static const uint8_t null[] = { 0xD8, 0x71, 0x82, 0x81, 0xF6, 0xE0 };
    nanocbor_decoder_init_packed(&val, null, sizeof(null));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_boolean(void)
{
    nanocbor_value_t val;
    bool b;

    // 113([[true], simple(0)])
    static const uint8_t boolean[] = { 0xD8, 0x71, 0x82, 0x81, 0xF5, 0xE0 };
    nanocbor_decoder_init_packed(&val, boolean, sizeof(boolean));
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_tstr(void)
{
    nanocbor_value_t val;
    const uint8_t *buf;
    size_t len;

    // 113([["a"], simple(0)])
    static const uint8_t tstr[] = { 0xD8, 0x71, 0x82, 0x81, 0x61, 0x61, 0xE0 };
    nanocbor_decoder_init_packed(&val, tstr, sizeof(tstr));
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_bstr(void)
{
    nanocbor_value_t val;
    const uint8_t *buf;
    size_t len;

    // 113([[h'C0'], simple(0)])
    static const uint8_t bstr[] = { 0xD8, 0x71, 0x82, 0x81, 0x41, 0xC0, 0xE0 };
    nanocbor_decoder_init_packed(&val, bstr, sizeof(bstr));
    CU_ASSERT_EQUAL(nanocbor_get_bstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_EQUAL(buf[0], 0xC0);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_array(void)
{
    nanocbor_value_t val, val2;

    // 113([[[null]], simple(0)])
    static const uint8_t array[] = { 0xD8, 0x71, 0x82, 0x81, 0x81, 0xF6, 0xE0 };
    nanocbor_decoder_init_packed(&val, array, sizeof(array));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 1);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_array_nested(void)
{
    nanocbor_value_t val, val2, val3;
    bool b;

    // 113([[[[true], [false]]], simple(0)])
    static const uint8_t shared_nested_container[] = { 0xD8, 0x71, 0x82, 0x81, 0x82, 0x81, 0xF5, 0x81, 0xF4, 0xE0 };
    nanocbor_decoder_init_packed(&val, shared_nested_container, sizeof(shared_nested_container));
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
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}


static void test_packed_map(void)
{
    nanocbor_value_t val, val2, val3;

    // 113([[{null: [null]}], simple(0)])
    static const uint8_t map[] = { 0xD8, 0x71, 0x82, 0x81, 0xA1, 0xF6, 0x81, 0xF6, 0xE0 };
    nanocbor_decoder_init_packed(&val, map, sizeof(map));
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

static void test_packed_within_array(void)
{
    nanocbor_value_t val, val2;
    const uint8_t *buf;
    size_t len;

    // 113([["a", "b"], [simple(1), simple(0)]])
    static const uint8_t twice[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0x82, 0xE1, 0xE0 };
    nanocbor_decoder_init_packed(&val, twice, sizeof(twice));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 2);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "b", len);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_within_map(void)
{
    nanocbor_value_t val, val2, val3, val4;
    const uint8_t *buf;
    size_t len;

    // 113([["a", "b"], [{simple(0): simple(1)}, {simple(1): simple(0)}]])
    static const uint8_t map[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0x82, 0xA1, 0xE0, 0xE1, 0xA1, 0xE1, 0xE0 };
    nanocbor_decoder_init_packed(&val, map, sizeof(map));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 2);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_map_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_key_tstr(&val3, "a", &val4), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val4, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "b", len);
    // CU_ASSERT_EQUAL(nanocbor_leave_container(&val2, &val3), NANOCBOR_OK);
    // todo: above breaks as leave_container does not check being at the end of it, also below
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val2, &val4), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_map_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_key_tstr(&val3, "b", &val4), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val4, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val2, &val4), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_within_tag(void)
{
    nanocbor_value_t val, val2;
    uint32_t tag;

    // 41(113([[null], [simple(0)]]))
    static const uint8_t within_tag[] = { 0xD8, 0x29, 0xD8, 0x71, 0x82, 0x81, 0xF6, 0x81, 0xE0 };
    nanocbor_decoder_init_packed(&val, within_tag, sizeof(within_tag));
    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tag), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tag, 41);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 1);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_indirection(void)
{
    nanocbor_value_t val;

    // 113([[simple(1),null], simple(0)])
    static const uint8_t indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xE1, 0xF6, 0xE0 };
    nanocbor_decoder_init_packed(&val, indirection, sizeof(indirection));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_nested_tables(void)
{
    nanocbor_value_t val, val2;
    bool b;

    // 113([[false, true], 113([[null], [simple(0), simple(2), simple(1)]])])
    static const uint8_t nested_tables[] = { 0xD8, 0x71, 0x82, 0x82, 0xF4, 0xF5, 0xD8, 0x71, 0x82, 0x81, 0xF6, 0x83, 0xE0, 0xE2, 0xE1 };
    nanocbor_decoder_init_packed(&val, nested_tables, sizeof(nested_tables));
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

static void test_packed_nested_tables_with_indirection(void)
{
    nanocbor_value_t val;
    bool b;

    // 113([[true, simple(0)], 113([[false], simple(2)])])
    static const uint8_t nested_tables_with_indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xF5, 0xE0, 0xD8, 0x71, 0x82, 0x81, 0xF4, 0xE2 };
    nanocbor_decoder_init_packed(&val, nested_tables_with_indirection, sizeof(nested_tables_with_indirection));
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_nested_table_within_table(void)
{
    nanocbor_value_t val;

    // 113([[null, 113([[undefined], simple(0)])], simple(1)])
    static const uint8_t nested_table_within_table[] = { 0xD8, 0x71, 0x82, 0x82, 0xF6, 0xD8, 0x71, 0x82, 0x81, 0xF7, 0xE0, 0xE1 };
    nanocbor_decoder_init_packed(&val, nested_table_within_table, sizeof(nested_table_within_table));
    CU_ASSERT_EQUAL(nanocbor_get_undefined(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_table_within_array(void)
{
    nanocbor_value_t val, val2;
    bool b;

    // [113([[false], simple(0)]), true]
    static const uint8_t table_within_array[] = { 0x82, 0xD8, 0x71, 0x82, 0x81, 0xF4, 0xE0, 0xF5 };
    nanocbor_decoder_init_packed(&val, table_within_array, sizeof(table_within_array));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 2);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val2, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, false);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val2, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_reference_by_tag(void)
{
    nanocbor_value_t val, val2;
    bool b;

    // 113([[0,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true,null], [6(0), 6(-1), 6(simple(0))]])
    static const uint8_t reference_by_tag[] = { 0xD8, 0x71, 0x82, 0x92, 0x00, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF5, 0xF6, 0x83, 0xC6, 0x00, 0xC6, 0x20, 0xC6, 0xE0 };
    nanocbor_decoder_init_packed(&val, reference_by_tag, sizeof(reference_by_tag));
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 3);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val2, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val2, &b), NANOCBOR_OK);
    CU_ASSERT_EQUAL(b, true);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_undefined_table(void)
{
    nanocbor_value_t val;

    // simple(0)
    static const uint8_t undefined_table[] = { 0xE0 };
    nanocbor_decoder_init_packed(&val, undefined_table, sizeof(undefined_table));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_INVALID_TYPE); // todo: which error expected?
}


static void test_packed_undefined_reference(void)
{
    nanocbor_value_t val;

    // 113([[], simple(0))
    static const uint8_t undefined_ref[] = { 0xD8, 0x71, 0x82, 0x80, 0xE0 };
    nanocbor_decoder_init_packed(&val, undefined_ref, sizeof(undefined_ref));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_INVALID_TYPE); // todo: which error expected?
}

static void test_packed_loop(void)
{
    nanocbor_value_t val, val2;
    bool b;
    uint8_t u;
    int8_t i;
    float_t f;
    const uint8_t *buf;
    size_t len;
    uint32_t t;

    // 113([[simple(0)], simple(0)])
    static const uint8_t loop[] = { 0xD8, 0x71, 0x82, 0x81, 0xE0, 0xE0 };
    nanocbor_decoder_init_packed(&val, loop, sizeof(loop));
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
    // todo: define + implement semantics for get_subcbor?
    // CU_ASSERT_EQUAL(nanocbor_get_subcbor(&val, &buf, &len), NANOCBOR_ERR_RECURSION);
}

static void test_packed_loop_indirection(void)
{
    nanocbor_value_t val;

    // 113([[simple(1), simple(0)], simple(0)])
    static const uint8_t loop_indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xE1, 0xE0, 0xE0 };
    nanocbor_decoder_init_packed(&val, loop_indirection, sizeof(loop_indirection));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_RECURSION);
}

static void test_packed_max_nesting_exceeded(void)
{
    nanocbor_value_t val;

    // 113([[], 113([[], 113([[], 113([[], null])])])])
    static const uint8_t max_nesting_exceeded[] = { 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xF6 };
    nanocbor_decoder_init_packed(&val, max_nesting_exceeded, sizeof(max_nesting_exceeded));
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_RECURSION); // todo: which error expected?
}

const test_t tests_decoder_packed[] = {
    {
        .f = test_packed_enable,
        .n = "CBOR packed enable support test",
    },
    {
        .f = test_packed_empty,
        .n = "CBOR packed empty shared item table test",
    },
    {
        .f = test_packed_unused,
        .n = "CBOR packed unused shared item table test",
    },
    {
        .f = test_packed_uint,
        .n = "CBOR packed uint test",
    },
    {
        .f = test_packed_nint,
        .n = "CBOR packed nint test",
    },
    {
        .f = test_packed_float,
        .n = "CBOR packed float test",
    },
    {
        .f = test_packed_undefined,
        .n = "CBOR packed undefined test",
    },
    {
        .f = test_packed_null,
        .n = "CBOR packed null test",
    },
        {
        .f = test_packed_boolean,
        .n = "CBOR packed boolean test",
    },
    {
        .f = test_packed_tstr,
        .n = "CBOR packed tstr test",
    },
    {
        .f = test_packed_bstr,
        .n = "CBOR packed bstr test",
    },
    {
        .f = test_packed_array,
        .n = "CBOR packed array test",
    },
    {
        .f = test_packed_array_nested,
        .n = "CBOR packed nested array test",
    },
    {
        .f = test_packed_map,
        .n = "CBOR packed map test",
    },
    {
        .f = test_packed_within_array,
        .n = "CBOR packed array with shared items as content test",
    },
    {
        .f = test_packed_within_map,
        .n = "CBOR packed map with shared items as keys/values test",
    },
    {
        .f = test_packed_within_tag,
        .n = "CBOR packed shared item table within tag test",
    },
    {
        .f = test_packed_indirection,
        .n = "CBOR packed indirect reference test",
    },
    {
        .f = test_packed_nested_tables,
        .n = "CBOR packed nested shared item tables test",
    },
    {
        .f = test_packed_nested_tables_with_indirection,
        .n = "CBOR packed nested shared item tables with indirect reference test",
    },
    {
        .f = test_packed_nested_table_within_table,
        .n = "CBOR packed nested shared item table definition within table definition test",
    },
    {
        .f = test_packed_table_within_array,
        .n = "CBOR packed table definition within array test",
    },
    {
        .f = test_packed_reference_by_tag,
        .n = "CBOR packed reference by tag 6 test",
    },
    {
        .f = test_packed_undefined_table,
        .n = "CBOR packed undefined table test",
    },
    {
        .f = test_packed_undefined_reference,
        .n = "CBOR packed undefined reference test",
    },
    {
        .f = test_packed_loop,
        .n = "CBOR packed reference loop test",
    },
    {
        .f = test_packed_loop_indirection,
        .n = "CBOR packed indirect reference loop test",
    },
    {
        .f = test_packed_max_nesting_exceeded,
        .n = "CBOR packed max nesting exceeded test",
    },
    {
        .f = NULL,
        .n = NULL,
    },
};

/* NOLINTEND(cppcoreguidelines-avoid-magic-numbers) */
