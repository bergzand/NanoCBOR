# NanoCBOR

NanoCBOR is a tiny [CBOR](https://tools.ietf.org/html/rfc7049) library aimed at embedded and heavily constrained devices.
It is optimized for 32 bit architectures but should run fine on 8 bit and 16 bit architectures.
NanoCBOR is optimized for decoding known CBOR structures while optimizing the flash footprint of both NanoCBOR and the code using NanoCBOR.

The decoder of NanoCBOR should compile to 600-800 bytes on a Cortex-M0+ MCU, depending on whether floating point decoding is required.

## Usage

To achieve the small code size, two patterns are used throughout the decode library.

 - Every decode call will first check the type and refuse to decode if the CBOR element is not of the required type.
 - Every decode call will, on successful decode, advance the decode context to the next CBOR element.

This allows using code to call decode functions and check the return code of the function without requiring an if value of type, decode value, advance to next item dance, and requiring only a single call to decode an expected type and advance to the next element.

### Decoding

Start the decoding of a buffer with:

```C
nanocbor_value_t decoder;
nanocbor_decoder_init(&decoder, buffer, buffer_len);
```

Where `buffer` is an `const uint8_t` array containing an CBOR structure.

To decode an `int32_t` from a cbor structure and bail out if the element is not of the integer type:

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

### Encoding

NanoCBOR supports encoding via a polymorphic stream interface via function
pointers. This methods on this interface are:

```
typedef size_t (*FnStreamLength)(void *stream);
typedef int (*FnStreamReserve)(void *stream,  size_t len);
typedef void (*FnStreamInsert)(void *stream, const void *src, size_t n);
```

Where `FnStreamLength` queries the stream about how many CBOR bytes have been
provided, `FnStreamReserve` reserves the requested number of bytes within the
stream, and `FnStreamInsert` moves bytes into the stream. Each of these
functions accept a `stream` object, which contains the stream state if one is
needed. This is exactly like the "this" pointer in other object oriented
languages.

The `memory_buffer.h/c` is the canonical stream encoder, it expected a large
memory buffer that it can `memcpy` CBOR data into.

Construction of these objects looks like:

```
uint8_t buf[64];
memory_encoder stream;
MemoryStream_Init(&stream, buf, sizeof(buf));

FnStreamLength len_fn = (FnStreamLength)MemoryStream_Length;
FnStreamReserve res_fn = (FnStreamReserve)MemoryStream_Reserve;
FnStreamInsert ins_fn = (FnStreamInsert)MemoryStream_Insert;

nanocbor_encoder_t enc = NANOCBOR_ENCODER(&stream, len_fn, res_fn, ins_fn);
```

With a valid `nanocbor_encoder_t` you can pass it to the `fmt` functions:

```
nanocbor_fmt_array_indefinite(&enc);
nanocbor_fmt_float(&enc, 1.75);
nanocbor_fmt_float(&enc, 1.9990234375);
nanocbor_fmt_float(&enc, 1.99951171875);
nanocbor_fmt_float(&enc, 2.0009765625);
nanocbor_fmt_float(&enc, -1.75);
nanocbor_fmt_float(&enc, -1.9990234375);
nanocbor_fmt_float(&enc, -1.99951171875);
nanocbor_fmt_float(&enc, -2.0009765625);
nanocbor_fmt_end_indefinite(&enc);
```

The encoder places bytes in the stream on demand, so after you are done you can
find your encoded CBOR data within the buffer you created:

```
size_t len = nanocbor_encoded_len(&enc);

printf("\n");
for(unsigned int idx=0; idx < len; idx++)
{
    printf("%02X", buf[idx]);
}
printf("\n");
```

#### Implementing a new stream

Here is an example of a `stdout` stream and how to use it:

```
typedef struct stdout_stream {
  int len;
} stdout_stream;

void Stdout_Init(stdout_stream *self) {
  self->len = 0;
}

size_t Stdout_Length(stdout_stream *self) {
  return self->len;
}

int Stdout_Reserve(stdout_stream *self, size_t len) {
  self->len += len;
  return (int)len;
}

void Stdout_Insert(stdout_stream *self, const void *src, size_t n) {
  const uint8_t *bytes = (const uint8_t *)src;
  for (size_t i = 0; i < n; ++i) {
    printf("%02X ", bytes[i]);
  }
}

void encode(void) {
  stdout_stream stream;
  Stdout_Init(&stream);

  FnStreamLength len_fn = (FnStreamLength)Stdout_Length;
  FnStreamReserve res_fn = (FnStreamReserve)Stdout_Reserve;
  FnStreamInsert ins_fn = (FnStreamInsert)Stdout_Insert;

  nanocbor_encoder_t enc = NANOCBOR_ENCODER(&stream, len_fn, res_fn, ins_fn);
  nanocbor_fmt_array_indefinite(&enc);
  nanocbor_fmt_float(&enc, 1.75);
  nanocbor_fmt_float(&enc, 1.9990234375);
  nanocbor_fmt_float(&enc, 1.99951171875);
  nanocbor_fmt_float(&enc, 2.0009765625);
  nanocbor_fmt_end_indefinite(&enc);
}
```


### Dependencies

Only dependency are two functions to provide endian conversion.
These are not provided by the library and have to be configured in the header file.
On a bare metal ARM platform, `__builtin_bswap64` and `__builtin_bswap32` can be used for this conversion.

### Contributing

Open an issue, PR, the usual.

