/*
 * SPDX-License-Identifier: CC0-1.0
 */
#include "nanocbor/nanocbor.h"
#include "nanocbor/stream_encoders/memory_buffer.h"
#include <string.h>

void MemoryStream_Init(memory_encoder *self, uint8_t *buf, size_t len)
{
    self->len = 0;
    self->cur = buf;
    self->end = buf + len;
}

size_t MemoryStream_Length(memory_encoder *self)
{
    return self->len;
}

int MemoryStream_Reserve(memory_encoder *self, size_t len)
{
    const int fits = ((size_t)(self->end - self->cur) >= len);
    self->len += len;
    return fits ? (int)len : NANOCBOR_ERR_END;
}

void MemoryStream_Insert(memory_encoder *self, const void *src, size_t n)
{
    memcpy(self->cur, src, n);
    self->cur += n;
}
