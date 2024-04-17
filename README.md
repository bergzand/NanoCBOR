# NanoCBOR

NanoCBOR is a tiny [CBOR](https://tools.ietf.org/html/rfc7049) library aimed at embedded and heavily constrained devices.
It is optimized for 32-bit architectures but should run fine on 8-bit and 16-bit architectures.
NanoCBOR is optimized for decoding known CBOR structures while optimizing the flash footprint of both NanoCBOR and the code using NanoCBOR.

The decoder of NanoCBOR should compile to 600-800 bytes on a Cortex-M0+ MCU, depending on whether floating point decoding is required.

## Compiling

Compiling NanoCBOR as is from this repository requires:
- [Meson] 
- [Ninja]

Furthermore, the tests make use of [CUnit] as test framework.

All of these can usually be found in the package repository of your Linux distribution.

Building NanoCBOR is a two-step process. First a build directory has to be created with the necessary Ninja build files:

```
meson build
```

Second step is to compile NanoCBOR:

```
ninja -C build
```

This results in a `libnanocbor.so` file inside the `build` directory and binaries for the examples and tests in their respective directories inside the `build` directory.

When including NanoCBOR into a custom project, it is usually sufficient to only include the source and header files into the project, the meson build system used in the repo is not mandatory to use.

## Usage

To achieve the small code size, two patterns are used throughout the decode library.

 - Every decode call will first check the type and refuse to decode if the CBOR element is not of the required type.
 - Every decode call will, on successful decode, advance the decode context to the next CBOR element.

This allows using code to call decode functions and check the return code of the function without requiring an if value of type, decode value, advance to next item dance, and requiring only a single call to decode an expected type and advance to the next element.

Start the decoding of a buffer with:

```C
nanocbor_value_t decoder;
nanocbor_decoder_init(&decoder, buffer, buffer_len);
```

Where `buffer` is a `const uint8_t` array containing a CBOR structure.

To decode an `int32_t` from a CBOR structure and bail out if the element is not of the integer type:

```C
int32_t value = 0;
if (nanocbor_get_int32(&decoder, &value) < 0) {
  return ERR_INVALID_STRUCTURE;
}
return use_value(value);
```

Iterating over an CBOR array and calling a function passing every element is as simple as:

```C
nanocbor_value_t arr; /* Array value instance */

if (nanocbor_enter_array(&decoder, &arr) < 0) {
    return ERR_INVALID_STRUCTURE;
}
while (!nanocbor_at_end(&arr)) {
    handle_array_element(&arr);
}
```

Decoding a map is similar to an array, except that every map entry consists of two CBOR elements requiring separate decoding.
For example, a map using integers as keys and strings as values can be decoded with:

```C
while (!nanocbor_at_end(&map)) {
    int32_t key;
    const char *value;
    size_t value_len;
    if (nanocbor_get_int32(&map, &integer_key) < 0) {
        return ERR_INVALID_STRUCTURE;
    }
    if (nanocbor_get_tstr(&map, &value, &value_len) < 0) {
        return ERR_INVALID_STRUCTURE;
    }
    handle_map_element(key, value, value_len);
}
```

## Packed CBOR support

NanoCBOR comes with optional support for transparent shared item decoding according to [packed CBOR].
This functionality needs to be explicitly enabled at compile-time by setting `NANOCBOR_DECODE_PACKED_ENABLED` to `1`,
and at runtime by using `nanocbor_decoder_init_packed()` or `nanocbor_decoder_init_packed_table()` instead of `nanocbor_decoder_init()` for a certain `nanocbor_value_t` decoding context.

In enabled, the normal NanoCBOR decoding functions support, transparently to the calling function,
- application-provided shared item table setup
- tag 113 for shared item table setup
- simple values 0-15 and tag 6 for shared item references

The maximum nesting level of shared item tables can be set at runtime with `NANOCBOR_DECODE_PACKED_NESTED_TABLES_MAX` and defaults to `3`.

## Dependencies

Only dependency are two functions to provide endian conversion.
These are not provided by the library and have to be configured in the header file.
On a bare metal ARM platform, `__builtin_bswap64` and `__builtin_bswap32` can be used for this conversion.

## Contributing

Open an issue, PR, the usual.

[Meson]: https://mesonbuild.com/
[Ninja]: https://ninja-build.org/
[CUnit]: https://cunit.sourceforge.net/
[packed CBOR]: https://datatracker.ietf.org/doc/draft-ietf-cbor-packed/
