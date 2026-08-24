#ifndef CSONPATH_RUST_BACKEND_H_
#define CSONPATH_RUST_BACKEND_H_

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The C core treats JSON values as opaque pointers. All access goes through
 * FFI functions implemented in Rust (backend_serde.rs). */
#define CSONPATH_JSON void *

#define CSONPATH_NULL NULL

/* -- accessor function declarations (defined in Rust) -- */
extern void  rust_decref(void *obj);
extern void *rust_new_object(void);
extern void *rust_new_array(void);
extern void  rust_array_append(void *list, void *el);
extern void  rust_array_append_incref(void *list, void *el);
extern void  rust_array_clear(void *o);
extern void  rust_obj_clear(void *o);
extern void *rust_get(void *obj, const char *key);
extern void *rust_at(void *arr, int idx);
extern int   rust_is_obj(void *o);
extern int   rust_is_array(void *o);
extern int   rust_is_str(void *o);
extern int   rust_is_num(void *o);
extern int   rust_is_bool(void *o);
extern int   rust_is_null(void *o);
extern const char *rust_get_str(void *o);
extern void        rust_free_str(const char *s);
extern long long   rust_get_num(void *o);
extern int   rust_get_bool(void *o);
extern int   rust_equal_num(void *o, long long v);
extern int   rust_equal_str(void *o, const char *s);
extern void  rust_remove_child(void *o, void *ci);
extern int   rust_append_at_int(void *arr, int idx, void *el, int do_incref);
extern int   rust_append_at_str(void *arr, const char *key, void *el, int do_incref);
extern int   rust_need_foreach_redo(void *o);
extern int   rust_call_callback(void *cb, void *ctx, void *ci, void *tmp, void *udata);

/* iterator functions */
extern void  rust_array_iter_init(void *it, void *arr);
extern int   rust_array_iter_next(void *it, void **out_el, size_t *out_idx);
extern void  rust_obj_iter_init(void *it, void *obj);
extern int   rust_obj_iter_next(void *it, void **out_val,
                                const char **out_key, size_t *out_key_len);
extern void  rust_obj_iter_free(void *it);
extern void  rust_obj_iter_cleanup(void *it);

/* -- macros used by csonpath core -- */
#define CSONPATH_GET(o, k)              rust_get((o), (k))
#define CSONPATH_AT(o, i)               rust_at((o), (i))
#define CSONPATH_IS_OBJ(o)              rust_is_obj(o)
#define CSONPATH_IS_ARRAY(o)            rust_is_array(o)
#define CSONPATH_IS_STR(o)              rust_is_str(o)
#define CSONPATH_IS_NUM(o)              rust_is_num(o)
#define CSONPATH_IS_BOOL(o)             rust_is_bool(o)
#define CSONPATH_IS_NULL(o)             rust_is_null(o)
#define CSONPATH_GET_STR(o)             rust_get_str(o)
#define CSONPATH_GET_NUM(o)             rust_get_num(o)
#define CSONPATH_GET_BOOL(o)            rust_get_bool(o)
#define CSONPATH_EQUAL_NUM(o, c)        rust_equal_num((o), (c))
#define CSONPATH_EQUAL_STR(o, c)        rust_equal_str((o), (c))

/* -- string lifetime (owned by Rust; freed via rust_free_str on scope exit) --
 * Temporary fix: rust_get_str allocates a fresh NUL-terminated CString per
 * call (serde_json::String is not NUL-terminated). Eventually the C string
 * comparisons below should be rewritten (e.g. mem* against the raw bytes) so
 * this ownership dance disappears. Every backend except Rust defines
 * CSONPATH_CLEANUP_STR to nothing, since their GET_STR never allocates. */
static inline void csonpath_cleanup_str(const char **p) {
    if (*p) rust_free_str(*p);
}
#define CSONPATH_CLEANUP_STR __attribute__((cleanup(csonpath_cleanup_str)))

/* -- lifecycle -- */
#define CSONPATH_REMOVE(o)              rust_decref(o)
#define CSONPATH_NEW_OBJECT()           rust_new_object()
#define CSONPATH_NEW_ARRAY()            rust_new_array()
#define CSONPATH_ARRAY_APPEND(l, e)     rust_array_append_incref((l), (e))
#define CSONPATH_ARRAY_CLEAR(o)         rust_array_clear(o)
#define CSONPATH_OBJ_CLEAR(o)           rust_obj_clear(o)

/* -- iterator structs (must match #[repr(C)] layouts in backend_serde.rs) -- */
struct rust_array_iter {
    void *arr;
    size_t len;
    size_t idx;
};

struct rust_obj_iter {
    void *obj;
    size_t len;
    size_t idx;
    void *entries;   /* opaque: points to Rust-allocated Vec<ObjEntry> */
};

#define _CSONPATH_ITER_CLEANUP(fn) __attribute__((cleanup(fn)))

/* -- iteration macros -- */
#define CSONPATH_FOREACH_ARRAY(obj, child, idx) \
    struct rust_array_iter _cpr_arr_it_; \
    size_t _cpr_arr_idx_; \
    rust_array_iter_init(&_cpr_arr_it_, (obj)); \
    while (rust_array_iter_next(&_cpr_arr_it_, (void **)&(child), &_cpr_arr_idx_) && \
           (((idx) = _cpr_arr_idx_), 1))

#define CSONPATH_FOREACH_OBJ(obj, child, key) \
    struct rust_obj_iter _cpr_obj_it_ _CSONPATH_ITER_CLEANUP(rust_obj_iter_cleanup); \
    size_t _cpr_key_len_; \
    rust_obj_iter_init(&_cpr_obj_it_, (obj)); \
    while (rust_obj_iter_next(&_cpr_obj_it_, (void **)&(child), &(key), &_cpr_key_len_))

#define CSONPATH_FOREACH_EXT(obj, el, code, key_idx_) \
    if (rust_is_array(obj)) { \
        struct rust_array_iter _cpr_ext_it_; \
        rust_array_iter_init(&_cpr_ext_it_, (obj)); \
        size_t key_idx_; \
        void *el; \
        while (rust_array_iter_next(&_cpr_ext_it_, (void **)&(el), &(key_idx_))) { \
            code \
        } \
    } else if (rust_is_obj(obj)) { \
        struct rust_obj_iter _cpr_ext_it_ _CSONPATH_ITER_CLEANUP(rust_obj_iter_cleanup); \
        size_t _cpr_key_len2_; \
        rust_obj_iter_init(&_cpr_ext_it_, (obj)); \
        const char *key_idx_; \
        void *el; \
        while (rust_obj_iter_next(&_cpr_ext_it_, (void **)&(el), &(key_idx_), &_cpr_key_len2_)) { \
            code \
        } \
    }

/* -- mutation -- */
#define CSONPATH_REMOVE_CHILD(obj, child_info) \
    rust_remove_child((obj), &(child_info))

#define CSONPATH_APPEND_AT(array, at, el, do_incref) \
    _Generic((at), \
        int: rust_append_at_int, \
        unsigned int: rust_append_at_int, \
        long: rust_append_at_int, \
        unsigned long: rust_append_at_int, \
        long long: rust_append_at_int, \
        unsigned long long: rust_append_at_int, \
        const char *: rust_append_at_str, \
        char *: rust_append_at_str \
    )((array), (at), (el), (do_incref))

#define CSONPATH_NEED_FOREACH_REDO(o) rust_need_foreach_redo(o)

/* -- callbacks (opaque stubs) -- */
#define CSONPATH_CALLBACK void *
#define CSONPATH_CALLBACK_DATA void *
#define CSONPATH_CALL_CALLBACK(callback, ctx, child_info, tmp, udata) \
    rust_call_callback((callback), (ctx), &(child_info), (tmp), (udata))

/* -- exceptions / formatting -- */
#define CSONPATH_FORMAT_EXCEPTION(args...) fprintf(stderr, args)
#define CSONPATH_EXCEPTION(args...) do { \
    fprintf(stderr, args); \
    return -1; \
} while (0)

/* -- pull in csonpath core -- */
#include "csonpath.h"

#endif
