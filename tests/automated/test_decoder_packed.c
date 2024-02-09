/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include "nanocbor/config.h"
#include "nanocbor/nanocbor.h"
#include "test.h"
#include <CUnit/CUnit.h>

/* NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers) */

// todo: get all tests to succeed
// todo: implement packed for get_{uint,int,float,double,boolean,undefined}, also dedicated tests
// todo: implement packed for get_type -> will likely break stuff

static void test_packed_enable(void)
{
    nanocbor_value_t val, val2, val3;
    uint32_t tag;
    uint8_t simple;

    // 113([[null], simple(0)])
    static const uint8_t packed[] = { 0xD8, 0x71, 0x82, 0x81, 0xF6, 0xE0 };

    // disabled packed CBOR support
    nanocbor_decoder_init(&val, packed, sizeof(packed));
    nanocbor_decoder_set_packed_support(&val, false);
    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tag), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tag, 113);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val3), true);
    nanocbor_leave_container(&val2, &val3);
    CU_ASSERT_EQUAL(nanocbor_get_simple(&val2, &simple), NANOCBOR_OK);
    CU_ASSERT_EQUAL(simple, 0);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    nanocbor_leave_container(&val, &val2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    // enabled packed CBOR support
    nanocbor_decoder_init(&val, packed, sizeof(packed));
    nanocbor_decoder_set_packed_support(&val, true);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_empty(void)
{
    nanocbor_value_t val;

    // 113([[], null])
    static const uint8_t empty[] = { 0xD8, 0x71, 0x82, 0x80, 0xF6 };
    nanocbor_decoder_init(&val, empty, sizeof(empty));
    nanocbor_decoder_set_packed_support(&val, true);
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
    nanocbor_decoder_init(&val, unused, sizeof(unused));
    nanocbor_decoder_set_packed_support(&val, true);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "c", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_once(void)
{
    nanocbor_value_t val;
    const uint8_t *buf;
    size_t len;

    // 113([["a", "b"], simple(1)])
    static const uint8_t once[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0xE1 };
    nanocbor_decoder_init(&val, once, sizeof(once));
    nanocbor_decoder_set_packed_support(&val, true);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "b", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_twice(void)
{
    nanocbor_value_t val, val2;
    const uint8_t *buf;
    size_t len;

    // 113([["a", "b"], [simple(1), simple(0)]])
    static const uint8_t twice[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0x82, 0xE1, 0xE0 };
    nanocbor_decoder_init(&val, twice, sizeof(twice));
    nanocbor_decoder_set_packed_support(&val, true);
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
}

static void test_packed_indirection(void)
{
    nanocbor_value_t val;
    const uint8_t *buf;
    size_t len;

    // 113([[simple(1),"a"], simple(0)])
    static const uint8_t indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xE1, 0x61, 0x61, 0xE0 };
    nanocbor_decoder_init(&val, indirection, sizeof(indirection));
    nanocbor_decoder_set_packed_support(&val, true);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_nested(void)
{
    nanocbor_value_t val, val2;
    const uint8_t *buf;
    size_t len;

    // 113([["b", "c"], 113([["a"], [simple(0), simple(1), simple(2)]])])
    static const uint8_t nested[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x62, 0x61, 0x63, 0xD8, 0x71, 0x82, 0x81, 0x61, 0x61, 0x83, 0xE0, 0xE1, 0xE2 };
    nanocbor_decoder_init(&val, nested, sizeof(nested));
    nanocbor_decoder_set_packed_support(&val, true);
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
}

static void test_packed_nested_indirection(void)
{
    nanocbor_value_t val;
    const uint8_t *buf;
    size_t len;

    // 113([[null, 113([["a"], simple(0)])], simple(1)])
    static const uint8_t nested_indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xF6, 0xD8, 0x71, 0x82, 0x81, 0x61, 0x61, 0xE0, 0xE1 };
    nanocbor_decoder_init(&val, nested_indirection, sizeof(nested_indirection));
    nanocbor_decoder_set_packed_support(&val, true);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_map(void)
{
    nanocbor_value_t val, val2, val3, val4;
    const uint8_t *buf;
    size_t len;

    // 113([["a", "b"], [{simple(0): simple(1)}, {simple(1): simple(0)}]])
    static const uint8_t map[] = { 0xD8, 0x71, 0x82, 0x82, 0x61, 0x61, 0x61, 0x62, 0x82, 0xA1, 0xE0, 0xE1, 0xA1, 0xE1, 0xE0 };
    nanocbor_decoder_init(&val, map, sizeof(map));
    nanocbor_decoder_set_packed_support(&val, true);
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
}

static void test_packed_within_tag(void)
{
    nanocbor_value_t val, val2;
    const uint8_t *buf;
    size_t len;

    // 41(113([["a"], [simple(0)]]))
    uint32_t tag;
    static const uint8_t within_tag[] = { 0xD8, 0x29, 0xD8, 0x71, 0x82, 0x81, 0x61, 0x61, 0x81, 0xE0 };
    nanocbor_decoder_init(&val, within_tag, sizeof(within_tag));
    nanocbor_decoder_set_packed_support(&val, true);
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
}

static void test_packed_shared_container(void)
{
    nanocbor_value_t val, val2;
    const uint8_t *buf;
    size_t len;

    // 113([[["a"]], simple(0)])
    static const uint8_t shared_container[] = { 0xD8, 0x71, 0x82, 0x81, 0x81, 0x61, 0x61, 0xE0 };
    nanocbor_decoder_init(&val, shared_container, sizeof(shared_container));
    nanocbor_decoder_set_packed_support(&val, true);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 1);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val2, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    nanocbor_leave_container(&val, &val2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_packed_shared_nested_container(void)
{
    nanocbor_value_t val, val2, val3;
    const uint8_t *buf;
    size_t len;

    // 113([[[["a"], ["b"]]], simple(0)])
    static const uint8_t shared_nested_container[] = { 0xD8, 0x71, 0x82, 0x81, 0x82, 0x81, 0x61, 0x61, 0x81, 0x61, 0x62, 0xE0 };
    nanocbor_decoder_init(&val, shared_nested_container, sizeof(shared_nested_container));
    nanocbor_decoder_set_packed_support(&val, true);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &val2), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val2), 2);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val3, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "a", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val3), true);
    nanocbor_leave_container(&val2, &val3);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val2, &val3), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_array_items_remaining(&val3), 1);
    CU_ASSERT_EQUAL(nanocbor_get_tstr(&val3, &buf, &len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_NSTRING_EQUAL(buf, "b", len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val3), true);
    nanocbor_leave_container(&val2, &val3);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val2), true);
    nanocbor_leave_container(&val, &val2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

// static void test_packed_empty(void)
// {
//     nanocbor_value_t val, val2, val3, val4;
//     const uint8_t *buf;
//     size_t len;

//     // // simple(0)
//     // static const uint8_t undefined_table[] = { 0xE0 };
//     // // 113([[], simple(0))
//     // static const uint8_t undefined_ref[] = { 0xD8, 0x71, 0x82, 0x80, 0xE0 };
//     // // 113([[simple(0)], simple(0)])
//     // static const uint8_t loop[] = { 0xD8, 0x71, 0x82, 0x81, 0xE0, 0xE0 };
//     // // 113([[simple(1), simple(0)], simple(0)])
//     // static const uint8_t loop_indirection[] = { 0xD8, 0x71, 0x82, 0x82, 0xE1, 0xE0, 0xE0 };
//     // // 113([[], 113([[], 113([[], 113([[], null])])])])
//     // static const uint8_t max_nesting_exceeded[] = { 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xD8, 0x71, 0x82, 0x80, 0xF6 };
// }

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
        .f = test_packed_once,
        .n = "CBOR packed xx test",
    },
    {
        .f = test_packed_twice,
        .n = "CBOR packed xx test",
    },
    {
        .f = test_packed_indirection,
        .n = "CBOR packed indirect reference test",
    },
    {
        .f = test_packed_nested,
        .n = "CBOR packed nested shared item tables test",
    },
    {
        .f = test_packed_nested_indirection,
        .n = "CBOR packed nested shared item tables with indirect reference test",
    },
    {
        .f = test_packed_map,
        .n = "CBOR packed maps with shared item as key/value test",
    },
    {
        .f = test_packed_within_tag,
        .n = "CBOR packed shared item table within tag test",
    },
    {
        .f = test_packed_shared_container,
        .n = "CBOR packed container as shared item test",
    },
    {
        .f = test_packed_shared_nested_container,
        .n = "CBOR packed nested container as shared item test",
    },
    {
        .f = NULL,
        .n = NULL,
    },
};

/* NOLINTEND(cppcoreguidelines-avoid-magic-numbers) */
