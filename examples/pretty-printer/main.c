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

static void _print_indent(unsigned indent)
{
    if (_args.pretty) {
        for (unsigned i = 0; i < indent; i++) {
            printf("  ");
        }
    }
}

static void _print_separator(void)
{
    if (_args.pretty) {
        printf("\n");
    }
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static void _parse_cbor(nanocbor_value_t *it, unsigned indent)
{
    while (!nanocbor_at_end(it)) {
        _print_indent(indent);
        int res = _parse_type(it, indent);

        if (res < 0) {
            printf("Err\n");
            break;
        }

        if (!nanocbor_at_end(it)) {
            printf(", ");
        }
        _print_separator();
    }
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static void _parse_map(nanocbor_value_t *it, unsigned indent)
{
    while (!nanocbor_at_end(it)) {
        _print_indent(indent);
        int res = _parse_type(it, indent);
        printf(": ");
        if (res < 0) {
            printf("Err\n");
            break;
        }
        res = _parse_type(it, indent);
        if (res < 0) {
            printf("Err\n");
            break;
        }
        if (!nanocbor_at_end(it)) {
            printf(", ");
        }
        _print_separator();
    }
}

/* NOLINTNEXTLINE(misc-no-recursion, readability-function-cognitive-complexity) */
static int _parse_type(nanocbor_value_t *value, unsigned indent)
{
    uint8_t type = nanocbor_get_type(value);
    if (indent > MAX_DEPTH) {
        return -2;
    }
    switch (type) {
    case NANOCBOR_TYPE_UINT: {
        uint64_t uint = 0;
        if (nanocbor_get_uint64(value, &uint) >= 0) {
            printf("%" PRIu64, uint);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_NINT: {
        int64_t nint = 0;
        if (nanocbor_get_int64(value, &nint) >= 0) {
            printf("%" PRIi64, nint);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_BSTR: {
        const uint8_t *buf = NULL;
        size_t len = 0;
        if (nanocbor_get_bstr(value, &buf, &len) >= 0 && buf) {
            size_t iter = 0;
            printf("h\'");
            while (iter < len) {
                printf("%.2x", buf[iter]);
                iter++;
            }
            printf("\'");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_TSTR: {
        const uint8_t *buf = NULL;
        size_t len = 0;
        if (nanocbor_get_tstr(value, &buf, &len) >= 0) {
            printf("\"%.*s\"", (int)len, buf);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_ARR: {
        nanocbor_value_t arr;
        if (nanocbor_enter_array(value, &arr) >= 0) {
            printf("[");
            _print_separator();
            _parse_cbor(&arr, indent + 1);
            nanocbor_leave_container(value, &arr);
            _print_indent(indent);
            printf("]");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_MAP: {
        nanocbor_value_t map;
        if (nanocbor_enter_map(value, &map) >= NANOCBOR_OK) {
            printf("{");
            _print_separator();
            _parse_map(&map, indent + 1);
            nanocbor_leave_container(value, &map);
            _print_indent(indent);
            printf("}");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_FLOAT: {
        bool test = false;
        uint8_t simple = 0;
        float fvalue = 0;
        double dvalue = 0;
        if (nanocbor_get_bool(value, &test) >= NANOCBOR_OK) {
            test ? printf("true") : printf("false");
        }
        else if (nanocbor_get_null(value) >= NANOCBOR_OK) {
            printf("null");
        }
        else if (nanocbor_get_undefined(value) >= NANOCBOR_OK) {
            printf("\"undefined\"");
        }
        else if (nanocbor_get_simple(value, &simple) >= NANOCBOR_OK) {
            printf("\"simple(%u)\"", simple);
        }
        else if (nanocbor_get_float(value, &fvalue) >= 0) {
            printf("%f", fvalue);
        }
        else if (nanocbor_get_double(value, &dvalue) >= 0) {
            printf("%f", dvalue);
        }
        else {
            return -1;
        }
        break;
    }
    case NANOCBOR_TYPE_TAG: {
        uint32_t tag = 0;
        if (nanocbor_get_tag(value, &tag) >= NANOCBOR_OK) {
            printf("%" PRIu32 "(", tag);
            _parse_type(value, 0);
            printf(")");
        }
        else {
            return -1;
        }
        break;
    }
    default:
        printf("Unsupported type\n");
        return -1;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    struct argp arg_parse
        = { cmdline_options, _parse_opts, NULL, NULL, NULL, NULL, NULL };
    argp_parse(&arg_parse, argc, argv, 0, 0, &_args);

    FILE *fp = stdin;

    if (_args.input == NULL) {
        return -1;
    }

    if (strcmp(_args.input, "-") != 0) {
        fp = fopen(_args.input, "rbe");
    }

    size_t len = fread(buffer, 1, sizeof(buffer), fp);

    fclose(fp);
    printf("Start decoding %lu bytes:\n", (long unsigned)len);

    nanocbor_value_t it;
    nanocbor_decoder_init(&it, (uint8_t *)buffer, len);
    while (!nanocbor_at_end(&it)) {
        if (nanocbor_skip(&it) < 0) {
            break;
        }
    }

    nanocbor_decoder_init(&it, (uint8_t *)buffer, len);
    _parse_cbor(&it, 0);
    printf("\n");

    return 0;
}
