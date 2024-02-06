/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include <argp.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "nanocbor/nanocbor.h"

#define CBOR_READ_BUFFER_BYTES 4096
#define MAX_DEPTH 20

static const struct argp_option cmdline_options[] = {
    { "pretty", 'p', 0, OPTION_ARG_OPTIONAL,
      "Produce pretty printing with newlines and indents", 0 },
    { "input", 'f', "input", 0, "Input file, - for stdin", 0 },
    { 0 },
};

struct arguments {
    bool pretty;
    char *input;
};

static struct arguments _args = { false, NULL };

static char buffer[CBOR_READ_BUFFER_BYTES];

static error_t _parse_opts(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;
    switch (key) {
    case 'p':
        arguments->pretty = true;
        break;
    case 'f':
        arguments->input = arg;
        break;
    case ARGP_KEY_END:
        if (!arguments->input) {
            argp_usage(state);
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static int _parse_type(nanocbor_value_t *value, unsigned indent);


/* NOLINTNEXTLINE(misc-no-recursion) */
static void _parse_cbor(nanocbor_value_t *it, unsigned indent)
{
    int depth = 0;
    while (!nanocbor_at_end(it)) {
        int res = _parse_type(it, indent);

        if (res < 0) {
            
            break;
        }

        if (!nanocbor_at_end(it)) {
            
        }
        depth++;
        if (depth > 100) break;
    }
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static void _parse_map(nanocbor_value_t *it, unsigned indent)
{
    int depth = 0;
    while (!nanocbor_at_end(it)) {
        int res = _parse_type(it, indent);
        
        if (res < 0) {
            
            break;
        }
        res = _parse_type(it, indent);
        if (res < 0) {
            
            break;
        }
        if (!nanocbor_at_end(it)) {
            
        }
        depth++;
        if (depth > 25) break;
    }
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static int _print_enter_map(nanocbor_value_t *value, unsigned indent)
{
    nanocbor_value_t map;
    if (nanocbor_enter_map(value, &map) >= NANOCBOR_OK) {
        
        _parse_map(&map, indent + 1);
        nanocbor_leave_container(value, &map);
        
        return 0;
    }
    return -1;
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static int _print_enter_array(nanocbor_value_t *value, unsigned indent)
{
    nanocbor_value_t arr;
    if (nanocbor_enter_array(value, &arr) >= 0) {
        
        _parse_cbor(&arr, indent + 1);
        nanocbor_leave_container(value, &arr);
        
        return 0;
    }
    return -1;
}

static int _print_float(nanocbor_value_t *value)
{
    bool test = false;
    uint8_t simple = 0;
    float fvalue = 0;
    double dvalue = 0;
    if (nanocbor_get_bool(value, &test) >= NANOCBOR_OK) {
    }
    else if (nanocbor_get_null(value) >= NANOCBOR_OK) {
        
    }
    else if (nanocbor_get_undefined(value) >= NANOCBOR_OK) {
        
    }
    else if (nanocbor_get_simple(value, &simple) >= NANOCBOR_OK) {
        
    }
    else if (nanocbor_get_float(value, &fvalue) >= 0) {
        
    }
    else if (nanocbor_get_double(value, &dvalue) >= 0) {
        
    }
    else {
        return -1;
    }
    return 0;
}

/* NOLINTNEXTLINE(misc-no-recursion, readability-function-cognitive-complexity) */
static int _parse_type(nanocbor_value_t *value, unsigned indent)
{
    uint8_t type = nanocbor_get_type(value);
    if (indent > MAX_DEPTH) {
        return -2;
    }
    int res = 0;
    switch (type) {
    case NANOCBOR_TYPE_UINT: {
        uint64_t uint = 0;
        res = nanocbor_get_uint64(value, &uint);
        if (res >= 0) {
            
        }
    } break;
    case NANOCBOR_TYPE_NINT: {
        int64_t nint = 0;
        res = nanocbor_get_int64(value, &nint);
        if (res >= 0) {
            
        }
    } break;
    case NANOCBOR_TYPE_BSTR: {
        const uint8_t *buf = NULL;
        size_t len = 0;
        res = nanocbor_get_bstr(value, &buf, &len);
        if (res >= 0) {
            if (!buf) {
                return -1;
            }
            size_t iter = 0;
            
            while (iter < len) {
                
                iter++;
            }
            
        }
    } break;
    case NANOCBOR_TYPE_TSTR: {
        const uint8_t *buf = NULL;
        size_t len = 0;
        res = nanocbor_get_tstr(value, &buf, &len);
        if (res >= 0) {
            
        }
    } break;
    case NANOCBOR_TYPE_ARR: {
        res = _print_enter_array(value, indent);
    } break;
    case NANOCBOR_TYPE_MAP: {
        res = _print_enter_map(value, indent);
    } break;
    case NANOCBOR_TYPE_FLOAT: {
        res = _print_float(value);
    } break;
    case NANOCBOR_TYPE_TAG: {
        uint32_t tag = 0;
        int res = nanocbor_get_tag(value, &tag);
        if (res >= NANOCBOR_OK) {
            
            _parse_type(value, 0);
            
        }
        break;
    }
    default:
        
        return -1;
    }
    if (res < 0) {
        return -1;
    }
    return 1;
}

__AFL_FUZZ_INIT()

int main(int argc, char *argv[])
{
    __AFL_INIT();
    unsigned char *buffer = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
        if (len > 1024) continue;

        nanocbor_value_t it;
        nanocbor_decoder_init(&it, (uint8_t *)buffer, len);
        _parse_cbor(&it, 0);
        
    }

    return 0;
}
