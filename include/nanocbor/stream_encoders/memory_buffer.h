/*
 * SPDX-License-Identifier: CC0-1.0
 */

#ifndef ENCODER_MEMORY_BUFFER_H
#define ENCODER_MEMORY_BUFFER_H

#include <stdint.h>
#include <stdlib.h>

typedef struct memory_encoder {
    uint8_t *cur;   /**< Current position in the buffer */
    uint8_t *end;   /**< end of the buffer                      */
    size_t len;     /**< Length in bytes of supplied cbor data. Incremented
                      *  separate from the buffer check  */
} memory_encoder;

void MemoryStream_Init(memory_encoder *self, uint8_t *buf, size_t len);
size_t MemoryStream_Length(memory_encoder *self);
int MemoryStream_Reserve(memory_encoder *self,  size_t len);
void MemoryStream_Insert(memory_encoder *self, const void *src, size_t n);

#endif
