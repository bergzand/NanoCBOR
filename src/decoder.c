/*
 * SPDX-License-Identifier: CC0-1.0
 */

/**
 * @ingroup nanocbor
 * @{
 * @file
 * @brief   Minimalistic CBOR decoder implementation
 *
 * @author  Koen Zandberg <koen@bergzand.net>
 * @author  Mikolai Gütschow <mikolai.guetschow@tu-dresden.de>
 * @}
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nanocbor/config.h"
#include "nanocbor/nanocbor.h"

#include NANOCBOR_BYTEORDER_HEADER

void nanocbor_decoder_init(nanocbor_value_t *value, const uint8_t *buf,
                           size_t len)
{
    value->cur = buf;
    value->end = buf + len;
    value->flags = 0;
    /* no need to initialize value->remaining since this is not a container */
#if NANOCBOR_DECODE_PACKED_ENABLED
    memset(value->shared_item_tables, 0, sizeof(value->shared_item_tables));
#endif
}


static void _advance(nanocbor_value_t *cvalue, unsigned int res)
{
    cvalue->cur += res;
    cvalue->remaining--;
}

static int _advance_if(nanocbor_value_t *cvalue, int res)
{
    if (res > 0) {
        _advance(cvalue, (unsigned int)res);
    }
    return res;
}

static inline bool _over_end(const nanocbor_value_t *it)
{
    return it->cur >= it->end;
}

static int _value_match_exact(nanocbor_value_t *cvalue, uint8_t val)
{
    int res = NANOCBOR_ERR_INVALID_TYPE;

    if (_over_end(cvalue)) {
        res = NANOCBOR_ERR_END;
    }
    else if (*cvalue->cur == val) {
        _advance(cvalue, 1U);
        res = NANOCBOR_OK;
    }
    return res;
}

bool nanocbor_at_end(const nanocbor_value_t *it)
{
    bool end = false;
    /* The container is at the end when */
    if (_over_end(it) || /* Number of items exhausted */
        /* Indefinite container and the current item is the end marker */
        ((nanocbor_container_indefinite(it)
          && *it->cur
              == (NANOCBOR_TYPE_FLOAT << NANOCBOR_TYPE_OFFSET
                  | NANOCBOR_VALUE_MASK)))
        ||
        /* Or the remaining number of items is zero */
        (!nanocbor_container_indefinite(it) && nanocbor_in_container(it)
         && it->remaining == 0)) {
        end = true;
    }
    return end;
}

static inline int __get_type(const nanocbor_value_t *value)
{
    if (nanocbor_at_end(value)) {
        return NANOCBOR_ERR_END;
    }
    return ((*value->cur & NANOCBOR_TYPE_MASK) >> NANOCBOR_TYPE_OFFSET);
}

static int _get_uint64(const nanocbor_value_t *cvalue, uint64_t *value,
                       uint8_t max, int type)
{
    int ctype = __get_type(cvalue);

    if (ctype < 0) {
        return ctype;
    }

    if (type != ctype) {
        return NANOCBOR_ERR_INVALID_TYPE;
    }
    unsigned bytelen = *cvalue->cur & NANOCBOR_VALUE_MASK;

    if (bytelen < NANOCBOR_SIZE_BYTE) {
        *value = bytelen;
        /* Ptr should advance 1 pos */
        return 1;
    }
    if (bytelen > max) {
        return NANOCBOR_ERR_OVERFLOW;
    }

    unsigned bytes = 1U << (bytelen - NANOCBOR_SIZE_BYTE);

    if ((cvalue->cur + bytes) >= cvalue->end) {
        return NANOCBOR_ERR_END;
    }
    uint64_t tmp = 0;
    /* Copy the value from cbor to the least significant bytes */
    memcpy(((uint8_t *)&tmp) + sizeof(uint64_t) - bytes, cvalue->cur + 1U,
           bytes);
    /* NOLINTNEXTLINE: user supplied function */
    tmp = NANOCBOR_BE64TOH_FUNC(tmp);
    *value = 0;
    memcpy(value, &tmp, bytes);

    return (int)(1 + bytes);
}

#if NANOCBOR_DECODE_PACKED_ENABLED

/* forward declarations of functions used by packed CBOR handling */
static int __enter_container(const nanocbor_value_t *it,
                            nanocbor_value_t *container, uint8_t type);
static int _skip_limited(nanocbor_value_t *it, uint8_t limit);
static int _packed_handle(nanocbor_value_t *cvalue, uint8_t limit);

/**
 * Check whether packed CBOR decoding is enabled for the given @p value.
 *
 * @param       value   decoder context
 *
 * @return      `true` if packed CBOR decoding is enabled, otherwise `false`
 */
static inline bool _packed_enabled(const nanocbor_value_t *value)
{
    return (value->flags & NANOCBOR_DECODER_FLAG_PACKED_SUPPORT) != 0;
}

static inline void _packed_set_enabled(nanocbor_value_t *value, bool enabled)
{
    if (enabled) {
        value->flags |= NANOCBOR_DECODER_FLAG_PACKED_SUPPORT;
    }
    else {
        value->flags &= ~NANOCBOR_DECODER_FLAG_PACKED_SUPPORT;
    }
}

/**
 * Copy active set of packing tables from @p src to @p dest.
 *
 * @param       dest   decoder context whose active set of packing tables will be overwritten
 * @param       dest   decoder context with the active set of packing tables to be copied
 */
static inline void _packed_copy_tables(nanocbor_value_t *dest, const nanocbor_value_t *src)
{
    memcpy(dest->shared_item_tables, src->shared_item_tables, sizeof(dest->shared_item_tables));
    dest->num_active_tables = src->num_active_tables;
}

static inline int _enter_array_unpacked(const nanocbor_value_t *it, nanocbor_value_t *array)
{
    if (_packed_enabled(it)) {
        _packed_copy_tables(array, it);
        /* mark container as being top-level shared item if _enter_container has been called recursively */
        array->flags = NANOCBOR_DECODER_FLAG_PACKED_SUPPORT;
    }
    else {
        array->flags = 0;
    }
    return __enter_container(it, array, NANOCBOR_TYPE_ARR);
}

/**
 * Internal macro to avoid code duplication in decoder function implementations.
 * Instead of this internal macro, please use @ref _PACKED_HANDLE_BEGIN combined with @ref _PACKED_HANDLE_END,
 * @ref _PACKED_HANDLE_CONST or @ref _PACKED_HANDLE_CONTAINER.
 *
 * It works as follows:
 * 1. a copy of @p cvalue is created on the stack
 * 2. if @p cvalue points to a supported packed CBOR data item, it is handled within the copy
 *   a. on success, @p _do is executed and @p cvalue is updated to point to the copy for the remainder of the enclosing function
 *   b. on packed CBOR handling failure, the failure code is returned and @p cvalue remains unchanged
 * 3. if @p cvalue does not point to a supported packed CBOR data item,
 *    execution continues without a change to @p cvalue within the enclosing function
 *
 * @warning This is an unhygienic macro which declares a variable called `__inner` of type @ref nanocbor_value_t.
 *
 * @param[inout]    cvalue  decoder context of type @ref nanocbor_value_t*
 * @param           _do     statement(s) to be executed if a packed CBOR item has been found and successfully handled
 */
#define __PACKED_HANDLE(cvalue, _do)                                    \
    nanocbor_value_t __inner;                                           \
    do {                                                                \
        __inner = *cvalue;                                              \
        int res = _packed_handle(&__inner, NANOCBOR_RECURSION_MAX-1);   \
        if (res == NANOCBOR_OK) {                                       \
            /* packed CBOR found and handled */                         \
            _do;                                                        \
            /* update cvalue for remaining function call */             \
            cvalue = &__inner;                                          \
        }                                                               \
        else if (res != NANOCBOR_NOT_FOUND) {                           \
            /* error during packed CBOR handling */                     \
            return res;                                                 \
        }                                                               \
        /* else: no packed CBOR found, simply continue */               \
    } while (0)

/**
 * Macro to avoid code duplication in decoder function implementations.
 * This is supposed to be used as a first line in all the functions
 * that should transparently handle packed CBOR data items.
 *
 * A matching invocation of @ref __PACKED_HANDLE_END must be used as a last line
 * in all functions using this macro.
 *
 * See @ref __PACKED_HANDLE for a more detailed description of its internals.
 * For containers, please use @ref _PACKED_HANDLE_CONTAINER instead.
 * If @p cvalue is const, please use @ref _PACKED_HANDLE_CONST instead.
 *
 * @warning This is an unhygienic macro which declares two variables called `__inner` and `__outer`.
 *
 * @param[inout]    cvalue  non-const decoder context of type @ref nanocbor_value_t*
 */
#define _PACKED_HANDLE_BEGIN(cvalue)                                    \
    nanocbor_value_t *__outer = NULL;                                   \
    __PACKED_HANDLE(cvalue, __outer = cvalue)

/**
 * Macro to avoid code duplication in decoder function implementations.
 * This is supposed to be used as a last line in all the functions
 * that should transparently handle packed CBOR data items.
 *
 * A matching invocation of @ref __PACKED_HANDLE_BEGIN must be used as a first line
 * in all functions using this macro.
 *
 * In case @p res is non-negative (not indicating an error) and a packed item has been
 * handled in the matching @ref __PACKED_HANDLE_BEGIN macro, cvalue as given to that
 * macro is forwarded by one CBOR item.
 *
 * @param[inout]    res  return value of the enclosing function
 */
#define _PACKED_HANDLE_END(res)                                         \
    if (__outer != NULL && res >= NANOCBOR_OK) {                        \
        int __res = _skip_limited(__outer, NANOCBOR_RECURSION_MAX-1);   \
        if (__res != NANOCBOR_OK) res = __res;                          \
    }


/**
 * Macro to avoid code duplication in decoder function implementations.
 * This is supposed to be used as a first line in all the functions
 * that should transparently handle packed CBOR data items.
 *
 * See @ref __PACKED_HANDLE for a more detailed description of its internals.
 * If @p cvalue should be updated from within the enclosed function, please use @ref _PACKED_HANDLE instead.
 *
 * @warning This is an unhygienic macro which declares a variable called `__inner` of type @ref nanocbor_value_t.
 *
 * @param[inout]    cvalue  decoder context of type const @ref nanocbor_value_t*
 */
#define _PACKED_HANDLE_CONST(cvalue) __PACKED_HANDLE(cvalue, )

/**
 * Macro to avoid code duplication in decoder function implementations.
 * This is supposed to be used as a first line in all the functions
 * that should transparently handle packed CBOR data items.
 *
 * See @ref __PACKED_HANDLE for a more detailed description of its internals.
 * If @p cvalue should be updated from within the enclosed function, please use @ref _PACKED_HANDLE instead.
 *
 * @warning This is an unhygienic macro which declares a variable called `__inner` of type @ref nanocbor_value_t.
 *
 * @param[inout]    cvalue      decoder context of type const @ref nanocbor_value_t*
 * @param[out]      was_packed  boolean set to true iff a packed item has been successfully handled
 */
#define _PACKED_HANDLE_CONTAINER(cvalue, was_packed) __PACKED_HANDLE(cvalue, was_packed = true)

/**
 * @brief Add array to current set of active tables
 *
 * The array pointed to by @p cvalue is added to its active set of packing tables.
 *
 * @param       cvalue  decoder context
 * @param       limit   recursion limit
 *
 * @return      NANOCBOR_OK if a supported packed CBOR data item was found and handled
 * @return      NANOCBOR_ERR_RECURSION if the recursion limit has been exceeded
 * @return      NANOCBOR_ERR_PACKED_FORMAT if @p cvalue does not point to a valid CBOR array
 * @return      NANOCBOR_ERR_PACKED_MEMORY if the maximum number of active packing tables is exhausted
 * @return      negative on error
 */
static int _packed_add_array_as_table(nanocbor_value_t *cvalue, uint8_t limit)
{
    if (limit == 0) {
        return NANOCBOR_ERR_RECURSION;
    }

    /* handling for potentially packed item table */
    nanocbor_value_t followed = *cvalue;
    int ret = _packed_handle(&followed, limit-1);
    if (ret == NANOCBOR_OK) {
        _packed_copy_tables(cvalue, &followed);
    }
    else if (ret != NANOCBOR_NOT_FOUND) {
        return ret;
    }

    /* skip array or packed reference in cvalue */
    ret = _skip_limited(cvalue, limit-1);
    if (ret != NANOCBOR_OK) {
        return ret;
    }

    /* add table definition in &followed to cvalue */
    if (cvalue->num_active_tables >= NANOCBOR_DECODE_PACKED_NESTED_TABLES_MAX) {
        return NANOCBOR_ERR_PACKED_MEMORY;
    }
    struct nanocbor_packed_table *t = &cvalue->shared_item_tables[cvalue->num_active_tables];

    if (__get_type(&followed) != NANOCBOR_TYPE_ARR) {
        return NANOCBOR_ERR_PACKED_FORMAT;
    }
    t->start = followed.cur;
    ret = _skip_limited(&followed, limit-1);
    if (ret != NANOCBOR_OK) {
        return ret;
    }
    t->len = followed.cur - t->start;
    cvalue->num_active_tables += 1;

    return NANOCBOR_OK;
}

void nanocbor_decoder_init_packed(nanocbor_value_t *value, const uint8_t *buf,
                           size_t len)
{
    nanocbor_decoder_init(value, buf, len);
    value->flags = NANOCBOR_DECODER_FLAG_PACKED_SUPPORT;
    value->num_active_tables = 0;
}

int nanocbor_decoder_init_packed_table(nanocbor_value_t *value, const uint8_t *buf,
                           size_t len, const uint8_t *table_buf, size_t table_len)
{
    nanocbor_decoder_init_packed(value, buf, len);
    if (table_buf != NULL && table_len > 0) {
        nanocbor_value_t arr;
        nanocbor_decoder_init_packed(&arr, table_buf, table_len);
        int ret = _packed_add_array_as_table(&arr, NANOCBOR_RECURSION_MAX-1);
        if (ret != NANOCBOR_OK) return ret;
        _packed_copy_tables(value, &arr);
    }
    return NANOCBOR_OK;
}

/**
 * @brief Consume the content of a tag 113 packing table definition.
 *
 * It expects @p cvalue to point to a two-element array that is interpreted as [table, rump].
 * As a result of this function, @p cvalue is updated to point to rump
 * and to contain table in its active set of packing tables.
 *
 * @note this is supposed to be called only from within @ref _packed_handle
 *
 * @param       cvalue  decoder context at the start of the content of tag 113, i.e., a two-element array
 * @param       limit   recursion limit
 *
 * @return      NANOCBOR_OK on success
 * @return      NANOCBOR_ERR_RECURSION if the recursion limit has been exceeded
 * @return      NANOCBOR_ERR_PACKED_FORMAT if the tag content is not well-formed
 * @return      NANOCBOR_ERR_PACKED_MEMORY if the maximum number of active packing tables is exhausted
 *              (cf. @ref NANOCBOR_NANOCBOR_DECODE_PACKED_NESTED_TABLES_MAX)
 * @return      negative on error
 */
static int _packed_consume_table_definition_113(nanocbor_value_t *cvalue, uint8_t limit)
{
    /* 2-element array within tag 113 */
    nanocbor_value_t arr;
    int ret;

    /* content of tag 113 may itself be packed */
    ret = _packed_handle(cvalue, limit-1);
    if (ret != NANOCBOR_OK && ret != NANOCBOR_NOT_FOUND) return ret;

    ret = _enter_array_unpacked(cvalue, &arr);
    if (ret != NANOCBOR_OK) {
        return NANOCBOR_ERR_PACKED_FORMAT;
    }
    ret = _packed_add_array_as_table(&arr, limit-1);
    if (ret != NANOCBOR_OK) return ret;
    _packed_copy_tables(cvalue, &arr);

    /* set cvalue to rump */
    cvalue->cur = arr.cur;
    ret = _skip_limited(&arr, limit-1);
    if (ret != NANOCBOR_OK) {
        return ret;
    }
    /* ensure array within tag 113 had exactly 2 elements */
    if (!nanocbor_at_end(&arr)) {
        return NANOCBOR_ERR_PACKED_FORMAT;
    }
    cvalue->end = arr.cur;

    return NANOCBOR_OK;
}

/**
 * @brief Follow a packed CBOR shared item reference.
 *
 * This function updates @p cvalue to point to the start of the referenced data item,
 * which resides in one of its active packing tables.
 *
 * @note this is supposed to be called only from within @ref _packed_handle
 *
 * @param       cvalue  decoder context
 * @param       idx     index of the requested reference
 * @param       limit   recursion limit
 *
 * @return      NANOCBOR_OK on success
 * @return      NANOCBOR_ERR_RECURSION if the recursion limit has been exceeded
 * @return      NANOCBOR_ERR_PACKED_FORMAT if one of the packing table definitions is not well-formed
 * @return      NANOCBOR_ERR_PACKED_UNDEFINED_REFERENCE if the reference is undefined,
 *              i.e., the index larger than the sum of the sizes of all active packing tables
 * @return      negative on error
 */
static int _packed_follow_reference(nanocbor_value_t *cvalue, uint64_t idx, uint8_t limit)
{
    const size_t num = cvalue->num_active_tables;
    for (size_t i=0; i<num; i++) {
        const struct nanocbor_packed_table *t = &cvalue->shared_item_tables[num-1-i];
        if (t->start == NULL) {
            /* this actually indicates an internal error */
            return NANOCBOR_ERR_PACKED_FORMAT;
        }
        nanocbor_value_t table;
        nanocbor_decoder_init_packed(&table, t->start, t->len);
        _packed_copy_tables(&table, cvalue);

        /* stored tables are always fully unpacked, therefore no further _packed_handle needed */
        int res = _enter_array_unpacked(&table, cvalue);
        if (res < 0) {
            return res == NANOCBOR_ERR_RECURSION ? res : NANOCBOR_ERR_PACKED_FORMAT;
        }

        uint32_t table_size = nanocbor_container_indefinite(cvalue) ? UINT32_MAX : nanocbor_array_items_remaining(cvalue);
        if (idx < table_size) {
            size_t j=0;
            while (j<idx && !nanocbor_at_end(cvalue)) {
                res = _skip_limited(cvalue, limit);
                if (res < 0) return res;
                j++;
            }
            if (nanocbor_at_end(cvalue)) {
                /* for indefinite length tables, this means that j now contains the actual table size */
                table_size = j;
                goto next_table;
            }
            /* only retain common tables, i.e., ones that were defined up to and including i */
            cvalue->num_active_tables = num-i;
            return NANOCBOR_OK;
        } else {
next_table:
            idx -= table_size;
        }
    }
    /* idx not within current tables */
    return NANOCBOR_ERR_PACKED_UNDEFINED_REFERENCE;
}

/**
 * @brief Check for and handle a supported packed CBOR data item, if present at the current decoder position.
 *
 * If @p cvalue has support for packed CBOR handling enabled and
 * points to a supported packed CBOR data item,
 * it is updated to point to the start of the reconstructed data item
 * or the rump of a table definition, and @ref NANOCBOR_OK is returned.
 * Otherwise, @ref NANOCBOR_NOT_FOUND is returned.
 *
 * @param       cvalue  decoder context
 * @param       limit   recursion limit
 *
 * @return      NANOCBOR_OK if a supported packed CBOR data item was found and handled
 * @return      NANOCBOR_ERR_RECURSION if the recursion limit has been exceeded
 * @return      NANOCBOR_NOT_FOUND if no supported packed CBOR data item was found
 * @return      NANOCBOR_ERR_PACKED_FORMAT if one of the packed CBOR data items is not well-formed
 * @return      NANOCBOR_ERR_PACKED_MEMORY if the maximum number of active packing tables is exhausted
 * @return      NANOCBOR_ERR_PACKED_UNDEFINED_REFERENCE if an undefined reference is encountered
 * @return      negative on error
 */
static int _packed_handle(nanocbor_value_t *cvalue, uint8_t limit)
{
    if (!_packed_enabled(cvalue)) {
        return NANOCBOR_NOT_FOUND;
    }
    if (limit == 0) {
        return NANOCBOR_ERR_RECURSION;
    }

    int ret = NANOCBOR_NOT_FOUND;
    int ctype = __get_type(cvalue);
    if (ctype == NANOCBOR_TYPE_TAG) {
        uint64_t tag = 0;
        ret = _get_uint64(cvalue, &tag, NANOCBOR_SIZE_WORD, ctype);
        if (ret < 0) {
            /* tag number could not be decoded */
            return NANOCBOR_NOT_FOUND;
        }
        if (tag == NANOCBOR_TAG_PACKED_TABLE) {
            /* handle tag 113 (shared item table definition) */
            cvalue->cur += ret;
            ret = _packed_consume_table_definition_113(cvalue, limit);
        }
        else if (tag == NANOCBOR_TAG_PACKED_REF_SHARED) {
            /* handle tag 6 (shared item reference) */
            uint64_t n;
            cvalue->cur += ret;

            ret = _packed_handle(cvalue, limit-1);
            if (ret != NANOCBOR_OK && ret != NANOCBOR_NOT_FOUND) return ret;

            /* avoid nanocbor_get_int64 to skip repeated packed handling */
            ctype = __get_type(cvalue);
            if (ctype != NANOCBOR_TYPE_NINT && ctype != NANOCBOR_TYPE_UINT) {
                return NANOCBOR_ERR_PACKED_FORMAT;
            }
            ret = _get_uint64(cvalue, &n, NANOCBOR_SIZE_LONG, ctype);
            if (ret <= 0) {
                return NANOCBOR_ERR_PACKED_FORMAT;
            }
            _advance(cvalue, ret);

            uint64_t idx = 16 + 2*n + (ctype == NANOCBOR_TYPE_NINT ? 1 : 0);
            ret = _packed_follow_reference(cvalue, idx, limit);
        }
        else {
            return NANOCBOR_NOT_FOUND;
        }
    }
    else if (ctype == NANOCBOR_TYPE_FLOAT) {
        uint8_t simple = *cvalue->cur & NANOCBOR_VALUE_MASK;
        if (simple < 16) {
            /* handle simple value 0-15 (shared item reference) */
            _advance(cvalue, 1);
            ret = _packed_follow_reference(cvalue, simple, limit);
        }
        else {
            return NANOCBOR_NOT_FOUND;
        }
    }

    if (ret == NANOCBOR_OK) {
        /* recursively unpack on success, decrement limit to bound recursion and prevent infinite loops */
        ret = _packed_handle(cvalue, limit-1);
        return (ret < 0 && ret != NANOCBOR_NOT_FOUND) ? ret : NANOCBOR_OK;
    }
    return ret;
}

#else /* !NANOCBOR_DECODE_PACKED_ENABLED */
#define _PACKED_HANDLE_BEGIN(cvalue)
#define _PACKED_HANDLE_END(res)
#define _PACKED_HANDLE_CONST(cvalue)
#define _PACKED_HANDLE_CONTAINER(cvalue, was_packed)
#endif

int nanocbor_get_type(const nanocbor_value_t *value)
{
    _PACKED_HANDLE_CONST(value);
    return __get_type(value);
}

static int _get_and_advance_uint8(nanocbor_value_t *cvalue, uint8_t *value,
                                  int type)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_BYTE, type);
    *value = (uint8_t)tmp;

    res = _advance_if(cvalue, res);

    _PACKED_HANDLE_END(res);
    return res;
}

int nanocbor_get_uint8(nanocbor_value_t *cvalue, uint8_t *value)
{
    return _get_and_advance_uint8(cvalue, value, NANOCBOR_TYPE_UINT);
}

static int _get_and_advance_uint64(nanocbor_value_t *cvalue, uint64_t *value,
                                  uint8_t max)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    int res = _get_uint64(cvalue, value, max, NANOCBOR_TYPE_UINT);
    res = _advance_if(cvalue, res);

    _PACKED_HANDLE_END(res);
    return res;
}

int nanocbor_get_uint16(nanocbor_value_t *cvalue, uint16_t *value)
{
    uint64_t tmp = 0;
    int res
        = _get_and_advance_uint64(cvalue, &tmp, NANOCBOR_SIZE_SHORT);

    *value = (uint16_t)tmp;

    return res;
}

int nanocbor_get_uint32(nanocbor_value_t *cvalue, uint32_t *value)
{
    uint64_t tmp = 0;
    int res
        = _get_and_advance_uint64(cvalue, &tmp, NANOCBOR_SIZE_WORD);

    *value = (uint32_t)tmp;

    return res;
}

int nanocbor_get_uint64(nanocbor_value_t *cvalue, uint64_t *value)
{
    return _get_and_advance_uint64(cvalue, value, NANOCBOR_SIZE_LONG);
}

static int _get_and_advance_int64(nanocbor_value_t *cvalue, int64_t *value,
                                  uint8_t max, uint64_t bound)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    int type = __get_type(cvalue);
    if (type < 0) {
        return type;
    }
    int res = NANOCBOR_ERR_INVALID_TYPE;
    if (type == NANOCBOR_TYPE_NINT || type == NANOCBOR_TYPE_UINT) {
        uint64_t intermediate = 0;
        res = _get_uint64(cvalue, &intermediate, max, type);
        if (intermediate > bound) {
            res = NANOCBOR_ERR_OVERFLOW;
        }
        if (type == NANOCBOR_TYPE_NINT) {
            *value = (-(int64_t)intermediate) - 1;
        }
        else {
            *value = (int64_t)intermediate;
        }
    }
    res = _advance_if(cvalue, res);

    _PACKED_HANDLE_END(res);
    return res;
}

int nanocbor_get_int8(nanocbor_value_t *cvalue, int8_t *value)
{
    int64_t tmp = 0;
    int res
        = _get_and_advance_int64(cvalue, &tmp, NANOCBOR_SIZE_BYTE, INT8_MAX);

    *value = (int8_t)tmp;

    return res;
}

int nanocbor_get_int16(nanocbor_value_t *cvalue, int16_t *value)
{
    int64_t tmp = 0;
    int res
        = _get_and_advance_int64(cvalue, &tmp, NANOCBOR_SIZE_SHORT, INT16_MAX);

    *value = (int16_t)tmp;

    return res;
}

int nanocbor_get_int32(nanocbor_value_t *cvalue, int32_t *value)
{
    int64_t tmp = 0;
    int res
        = _get_and_advance_int64(cvalue, &tmp, NANOCBOR_SIZE_WORD, INT32_MAX);

    *value = (int32_t)tmp;

    return res;
}

int nanocbor_get_int64(nanocbor_value_t *cvalue, int64_t *value)
{
    return _get_and_advance_int64(cvalue, value, NANOCBOR_SIZE_LONG, INT64_MAX);
}

int nanocbor_get_tag(nanocbor_value_t *cvalue, uint32_t *tag)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_WORD, NANOCBOR_TYPE_TAG);

    if (res >= 0) {
        cvalue->cur += res;
        res = NANOCBOR_OK;
    }
    *tag = (uint32_t)tmp;

    _PACKED_HANDLE_END(res);
    return res;
}

int nanocbor_get_decimal_frac(nanocbor_value_t *cvalue, int32_t *e, int32_t *m)
{
    int res = NANOCBOR_NOT_FOUND;
    uint32_t tag = UINT32_MAX;
    if (nanocbor_get_tag(cvalue, &tag) == NANOCBOR_OK) {
        if (tag == NANOCBOR_TAG_DEC_FRAC) {
            nanocbor_value_t arr;
            if (nanocbor_enter_array(cvalue, &arr) == NANOCBOR_OK) {
                res = nanocbor_get_int32(&arr, e);
                if (res >= 0) {
                    res = nanocbor_get_int32(&arr, m);
                    if (res >= 0) {
                        res = NANOCBOR_OK;
                    }
                }
                nanocbor_leave_container(cvalue, &arr);
            }
        }
    }

    return res;
}

static int _get_str(nanocbor_value_t *cvalue, const uint8_t **buf, size_t *len,
                    uint8_t type)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    uint64_t tmp = 0;
    int res = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_SIZET, type);
    *len = tmp;

    if (cvalue->end - cvalue->cur < 0
        || (size_t)(cvalue->end - cvalue->cur) < *len) {
        return NANOCBOR_ERR_END;
    }
    if (res >= 0) {
        *buf = (cvalue->cur) + res;
        _advance(cvalue, (unsigned int)((size_t)res + *len));
        res = NANOCBOR_OK;
    }

    _PACKED_HANDLE_END(res);
    return res;
}

int nanocbor_get_bstr(nanocbor_value_t *cvalue, const uint8_t **buf,
                      size_t *len)
{
    return _get_str(cvalue, buf, len, NANOCBOR_TYPE_BSTR);
}

int nanocbor_get_tstr(nanocbor_value_t *cvalue, const uint8_t **buf,
                      size_t *len)
{
    return _get_str(cvalue, buf, len, NANOCBOR_TYPE_TSTR);
}

static int _get_simple_exact(nanocbor_value_t *cvalue, uint8_t val)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    int res = _value_match_exact(cvalue, val);

    _PACKED_HANDLE_END(res);
    return res;
}

int nanocbor_get_null(nanocbor_value_t *cvalue)
{
    return _get_simple_exact(cvalue,
                            NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_NULL);
}

int nanocbor_get_bool(nanocbor_value_t *cvalue, bool *value)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    int res = _value_match_exact(cvalue,
                                 NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_FALSE);
    if (res >= NANOCBOR_OK) {
        *value = false;
    } else {
        res = _value_match_exact(cvalue,
                                 NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_TRUE);
        if (res >= NANOCBOR_OK) {
            *value = true;
        }
    }

    _PACKED_HANDLE_END(res);
    return res;
}

int nanocbor_get_undefined(nanocbor_value_t *cvalue)
{
    return _get_simple_exact(cvalue,
                            NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_UNDEF);
}

int nanocbor_get_simple(nanocbor_value_t *cvalue, uint8_t *value)
{
    int res = _get_and_advance_uint8(cvalue, value, NANOCBOR_TYPE_FLOAT);
    if (res == NANOCBOR_ERR_OVERFLOW) {
        res = NANOCBOR_ERR_INVALID_TYPE;
    }
    return res > 0 ? NANOCBOR_OK : res;
}

/* float bit mask related defines */
#define FLOAT_EXP_OFFSET (127U)
#define FLOAT_SIZE (32U)
#define FLOAT_EXP_POS (23U)
#define FLOAT_EXP_MASK ((uint32_t)0xFFU)
#define FLOAT_SIGN_POS (31U)
#define FLOAT_FRAC_MASK (0x7FFFFFU)
#define FLOAT_SIGN_MASK ((uint32_t)1U << FLOAT_SIGN_POS)
#define FLOAT_EXP_IS_NAN (0xFFU)
#define FLOAT_IS_ZERO (~(FLOAT_SIGN_MASK))
/* Part where a float to halffloat leads to precision loss */
#define FLOAT_HALF_LOSS (0x1FFFU)

/* halffloat bit mask related defines */
#define HALF_EXP_OFFSET (15U)
#define HALF_SIZE (16U)
#define HALF_EXP_POS (10U)
#define HALF_EXP_MASK (0x1FU)
#define HALF_SIGN_POS (15U)
#define HALF_FRAC_MASK (0x3FFU)
#define HALF_SIGN_MASK ((uint16_t)(1U << HALF_SIGN_POS))
#define HALF_MASK_HALF (0xFFU)

#define HALF_FLOAT_EXP_DIFF ((uint16_t)(FLOAT_EXP_OFFSET - HALF_EXP_OFFSET))
#define HALF_FLOAT_EXP_POS_DIFF ((uint16_t)(FLOAT_EXP_POS - HALF_EXP_POS))
#define HALF_EXP_TO_FLOAT (HALF_FLOAT_EXP_DIFF << HALF_EXP_POS)

static int _decode_half_float(nanocbor_value_t *cvalue, float *value)
{
    uint64_t tmp = 0;
    int res
        = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_SHORT, NANOCBOR_TYPE_FLOAT);
    if (res == 1 + sizeof(uint16_t)) {
        uint32_t *ifloat = (uint32_t *)value;
        *ifloat = (tmp & HALF_SIGN_MASK) << (FLOAT_SIGN_POS - HALF_SIGN_POS);

        uint32_t significant = tmp & HALF_FRAC_MASK;
        uint32_t exponent = tmp & (HALF_EXP_MASK << HALF_EXP_POS);

        static const uint32_t magic = ((uint32_t)FLOAT_EXP_OFFSET - 1)
            << FLOAT_EXP_POS;
        static const float *fmagic = (float *)&magic;

        if (exponent == 0) {
            *ifloat = magic + significant;
            *value -= *fmagic;
        }
        else {
            if (exponent == (HALF_EXP_MASK << HALF_EXP_POS)) {
                /* Set exponent to max value */
                exponent
                    = ((FLOAT_EXP_MASK - HALF_FLOAT_EXP_DIFF) << HALF_EXP_POS);
            }
            *ifloat
                |= ((exponent + HALF_EXP_TO_FLOAT) << HALF_FLOAT_EXP_POS_DIFF)
                | (significant << HALF_FLOAT_EXP_POS_DIFF);
        }
        return _advance_if(cvalue, res);
    }
    return res > 0 ? NANOCBOR_ERR_INVALID_TYPE : res;
}

static int _decode_float(nanocbor_value_t *cvalue, float *value)
{
    uint64_t tmp = 0;
    int res
        = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_WORD, NANOCBOR_TYPE_FLOAT);
    if (res == 1 + sizeof(uint32_t)) {
        uint32_t ifloat = tmp;
        memcpy(value, &ifloat, sizeof(uint32_t));
        return _advance_if(cvalue, res);
    }
    return res > 0 ? NANOCBOR_ERR_INVALID_TYPE : res;
}

static int _decode_double(nanocbor_value_t *cvalue, double *value)
{
    uint64_t tmp = 0;
    int res
        = _get_uint64(cvalue, &tmp, NANOCBOR_SIZE_LONG, NANOCBOR_TYPE_FLOAT);
    if (res == 1 + sizeof(uint64_t)) {
        uint64_t ifloat = tmp;
        memcpy(value, &ifloat, sizeof(uint64_t));
        return _advance_if(cvalue, res);
    }
    return res > 0 ? NANOCBOR_ERR_INVALID_TYPE : res;
}

int nanocbor_get_float(nanocbor_value_t *cvalue, float *value)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    int res = _decode_half_float(cvalue, value);
    if (res < 0) {
        res = _decode_float(cvalue, value);
    }

    _PACKED_HANDLE_END(res);
    return res;
}

int nanocbor_get_double(nanocbor_value_t *cvalue, double *value)
{
    _PACKED_HANDLE_BEGIN(cvalue);

    float tmp = 0;
    int res = nanocbor_get_float(cvalue, &tmp);
    if (res >= NANOCBOR_OK) {
        *value = tmp;
        return res;
    }
    res = _decode_double(cvalue, value);

    _PACKED_HANDLE_END(res);
    return res;
}

static int __enter_container(const nanocbor_value_t *it,
                            nanocbor_value_t *container, uint8_t type)
{
    /* pre: container->flags *must* be initialized according to packed support */
    container->end = it->end;
    container->remaining = 0;

    uint8_t value_match = (uint8_t)(((unsigned)type << NANOCBOR_TYPE_OFFSET)
                                    | NANOCBOR_SIZE_INDEFINITE);

    /* Not using _value_match_exact here to keep *it const */
    if (!_over_end(it) && *it->cur == value_match) {
        container->flags |= NANOCBOR_DECODER_FLAG_INDEFINITE
            | NANOCBOR_DECODER_FLAG_CONTAINER;
        container->cur = it->cur + 1;
        return NANOCBOR_OK;
    }

    int res = _get_uint64(it, &container->remaining, NANOCBOR_SIZE_LONG, type);
    if (res < 0) {
        return res;
    }
    container->flags |= NANOCBOR_DECODER_FLAG_CONTAINER;
    container->cur = it->cur + res;
    return NANOCBOR_OK;
}

static int _enter_container(const nanocbor_value_t *it,
                            nanocbor_value_t *container, uint8_t type)
{
#if NANOCBOR_DECODE_PACKED_ENABLED
    if (_packed_enabled(it)) {
        bool was_packed = false;
        _PACKED_HANDLE_CONTAINER(it, was_packed);
        _packed_copy_tables(container, it);
        /* mark container as being top-level shared item if _packed_handle was successful */
        container->flags = NANOCBOR_DECODER_FLAG_PACKED_SUPPORT | (was_packed ? NANOCBOR_DECODER_FLAG_SHARED : 0);
    }
#else
    if (false) {}
#endif
    else {
        container->flags = 0;
    }
    return __enter_container(it, container, type);
}

int nanocbor_enter_array(const nanocbor_value_t *it, nanocbor_value_t *array)
{
    return _enter_container(it, array, NANOCBOR_TYPE_ARR);
}

int nanocbor_enter_map(const nanocbor_value_t *it, nanocbor_value_t *map)
{
    int res;
    res = _enter_container(it, map, NANOCBOR_TYPE_MAP);

    if (map->remaining > UINT64_MAX / 2) {
        return NANOCBOR_ERR_OVERFLOW;
    }
    map->remaining = map->remaining * 2;
    return res;
}

int nanocbor_leave_container(nanocbor_value_t *it, nanocbor_value_t *container)
{
    /* check `container` to be a valid, fully consumed container that is plausible to have been entered from `it` */
    if (!nanocbor_in_container(container) || !nanocbor_at_end(container)) {
        return NANOCBOR_ERR_INVALID_TYPE;
    }
#if NANOCBOR_DECODE_PACKED_ENABLED
    if (container->flags & NANOCBOR_DECODER_FLAG_SHARED) {
        return _skip_limited(it, NANOCBOR_RECURSION_MAX);
    }
#endif
    if (container->cur <= it->cur || container->cur > it->end) {
        return NANOCBOR_ERR_INVALID_TYPE;
    }
    if (it->remaining) {
        it->remaining--;
    }
    if (nanocbor_container_indefinite(container)) {
        it->cur = container->cur + 1;
    }
    else {
        it->cur = container->cur;
    }
    return NANOCBOR_OK;
}

static int _skip_simple(nanocbor_value_t *it)
{
    int type = __get_type(it);
    uint64_t tmp = 0;
    if (type == NANOCBOR_TYPE_BSTR || type == NANOCBOR_TYPE_TSTR) {
        const uint8_t *tmp = NULL;
        size_t len = 0;
        return _get_str(it, &tmp, &len, (uint8_t)type);
    }
    int res = _get_uint64(it, &tmp, NANOCBOR_SIZE_LONG, type);
    res = _advance_if(it, res);
    return res > 0 ? NANOCBOR_OK : res;
}

int nanocbor_get_subcbor(nanocbor_value_t *it, const uint8_t **start,
                         size_t *len)
{
    *start = it->cur;
    int res = nanocbor_skip(it);
    *len = (size_t)(it->cur - *start);
    return res;
}

int nanocbor_skip_simple(nanocbor_value_t *it)
{
    return _skip_simple(it);
}

/* NOLINTNEXTLINE(misc-no-recursion): Recursion is limited by design */
static int _skip_limited(nanocbor_value_t *it, uint8_t limit)
{
    if (limit == 0) {
        return NANOCBOR_ERR_RECURSION;
    }
    int type = __get_type(it);
    int res = type;

    /* map or array */
    if (type == NANOCBOR_TYPE_ARR || type == NANOCBOR_TYPE_MAP) {
        nanocbor_value_t recurse;
        /* no need for passing down limit to _enter_* since we already know it is a real container */
        res = (type == NANOCBOR_TYPE_MAP ? nanocbor_enter_map(it, &recurse)
                                         : nanocbor_enter_array(it, &recurse));
        if (res == NANOCBOR_OK) {
            while (!nanocbor_at_end(&recurse)) {
                res = _skip_limited(&recurse, limit - 1);
                if (res < 0) {
                    break;
                }
            }
            nanocbor_leave_container(it, &recurse);
        }
    }
    /* tag */
    else if (type == NANOCBOR_TYPE_TAG) {
        uint64_t tmp = 0;
        int res = _get_uint64(it, &tmp, NANOCBOR_SIZE_WORD, NANOCBOR_TYPE_TAG);
        if (res >= 0) {
            it->cur += res;
            res = _skip_limited(it, limit - 1);
        }
    }
    /* other basic types */
    else if (type >= 0) {
        res = _skip_simple(it);
    }
    return res < 0 ? res : NANOCBOR_OK;
}

int nanocbor_skip(nanocbor_value_t *it)
{
    return _skip_limited(it, NANOCBOR_RECURSION_MAX);
}

// todo: start could be const?
int nanocbor_get_key_tstr(nanocbor_value_t *start, const char *key,
                          nanocbor_value_t *value)
{
    int res = NANOCBOR_NOT_FOUND;
    size_t len = strlen(key);
    *value = *start;

    while (!nanocbor_at_end(value)) {
        const uint8_t *s = NULL;
        size_t s_len = 0;

        res = nanocbor_get_tstr(value, &s, &s_len);
        if (res < 0) {
            break;
        }

        if (s_len == len && !strncmp(key, (const char *)s, len)) {
            res = NANOCBOR_OK;
            break;
        }

        res = nanocbor_skip(value);
        if (res < 0) {
            break;
        }
    }

    return res;
}
