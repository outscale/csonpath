#ifndef CSONPATH_YYJSON_MUT_H_
#define CSONPATH_YYJSON_MUT_H_

/*
 * !!!!!! WARNING !!!!!!!
 * This backend works on yyjson_mut_val. Read-only operations are
 * supported; mutable operations (update/remove) currently abort at
 * runtime and will be implemented in a follow-up change.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include "yyjson.h"

/* This backend always uses a yyjson_mut_ prefix so that it can coexist
 * with the immutable backend in the same translation unit. Define
 * CSONPATH_USE_PREFIX before including this header to get an additional
 * yyjson_mut_ prefix applied to csonpath symbols. */
#ifdef CSONPATH_USE_PREFIX
# undef CSONPATH_API_PREFIX
# define CSONPATH_API_PREFIX yyjson_mut_
#else
# undef CSONPATH_API_PREFIX
# define CSONPATH_API_PREFIX yyjson_mut_
#endif

#include "csonpath_undef.h"

#define CSONPATH_JSON yyjson_mut_val *

#define CSONPATH_NULL NULL

#define CSONPATH_GET yyjson_mut_obj_get

#define CSONPATH_AT yyjson_mut_arr_get

#define CSONPATH_IS_OBJ(obj) yyjson_mut_is_obj(obj)
#define CSONPATH_IS_ARRAY(obj) yyjson_mut_is_arr(obj)
#define CSONPATH_IS_STR(obj) yyjson_mut_is_str(obj)
#define CSONPATH_IS_NUM(o) yyjson_mut_is_num(o)

#define CSONPATH_IS_NULL(o) ((o) == NULL || yyjson_mut_is_null(o))

#define CSONPATH_IS_BOOL(o) yyjson_mut_is_bool(o)

#define CSONPATH_GET_BOOL(o) yyjson_mut_get_bool(o)

struct csonpath_child_info;
typedef void (*yyjson_mut_val_callback)(yyjson_mut_val *, struct csonpath_child_info *, yyjson_mut_val *, void *);

#define CSONPATH_CALLBACK yyjson_mut_val_callback

#define CSONPATH_CALLBACK_DATA void *

#define CSONPATH_GET_STR(obj)			\
    yyjson_mut_get_str(obj)

#define CSONPATH_GET_NUM(obj)			\
    yyjson_mut_get_num(obj)


#define CSONPATH_EQUAL_STR(obj, to_cmp)	({			\
      _Bool r = 0;							\
      if (yyjson_mut_is_str(obj))				\
	  r = !strcmp(yyjson_mut_get_str(obj), to_cmp);		\
      r;								\
    })

#define CSONPATH_EQUAL_NUM(obj, to_cmp)	({			\
      _Bool r = 0;							\
      if (yyjson_mut_is_num(obj))				\
	r = yyjson_mut_get_num(obj) == to_cmp;			\
      r;								\
    })

#define CSONPATH_CALL_CALLBACK(callback, ctx, child_info, tmp, udata)   \
    (callback(ctx, child_info, tmp, udata), 0)


#define CSONPATH_FOREACH(obj, el, code)		\
    CSONPATH_FOREACH_EXT(obj, el, ({code}), key_idx)

#define CSONPATH_ARRAY_LENGTH(o) ((intptr_t)yyjson_mut_arr_size(o))

#define CSONPATH_FOREACH_ARRAY(obj, el, key_idx_)	\
  intptr_t max_;					\
  yyjson_mut_arr_foreach(obj, key_idx_, max_, el)

#define CSONPATH_FOREACH_OBJ(obj, val, key)						\
    for (yyjson_mut_obj_iter csonpath_iter_ = {0}, *csonpath_iter_p_ = NULL;	\
         (csonpath_iter_p_ == NULL) &&						\
         (yyjson_mut_obj_iter_init(obj, &csonpath_iter_),				\
          (csonpath_iter_p_ = &csonpath_iter_), 1);					\
         csonpath_iter_p_ = (void *)1)						\
        for (yyjson_mut_val *csonpath_key_ = yyjson_mut_obj_iter_next(&csonpath_iter_);	\
             csonpath_key_ != NULL &&						\
             ((val) = yyjson_mut_obj_iter_get_val(csonpath_key_),			\
              (key) = yyjson_mut_get_str(csonpath_key_), 1);			\
             csonpath_key_ = yyjson_mut_obj_iter_next(&csonpath_iter_))

#define CSONPATH_FOREACH_EXT(obj, el, code, key_idx_)			\
    if (yyjson_mut_is_arr(obj)) {				\
	intptr_t key_idx_, max_;				\
	yyjson_mut_val *val;					\
	(void)val;						\
	yyjson_mut_arr_foreach(obj, key_idx_, max_, el) {	\
	    (void) key_idx_; code;				\
	}							\
    } else if (yyjson_mut_is_obj(obj)) {			\
	intptr_t idx_, max_;					\
	yyjson_mut_val *key, *val;					\
	(void)val;						\
	(void)key;						\
	yyjson_mut_obj_foreach(obj, idx_, max_, key, el) {	\
	    const char *key_idx_ = yyjson_mut_get_str(key);	\
	    (void) key_idx_; code;				\
	}							\
    }


#define CSONPATH_ARRAY_CLEAR(o)			\
    yyjson_mut_arr_clear(o)

#define CSONPATH_OBJ_CLEAR(o)			\
    yyjson_mut_obj_clear(o)

static inline int csonpath_yyjson_mut_append_at_idx(yyjson_mut_val *arr, size_t idx,
					    yyjson_mut_val *val, yyjson_mut_doc *doc) {
    (void)doc;
    return yyjson_mut_arr_insert(arr, val, idx) ? 1 : -1;
}

static inline int csonpath_yyjson_mut_append_at_key(yyjson_mut_val *obj, const char *key,
					    yyjson_mut_val *val, yyjson_mut_doc *doc) {
    yyjson_mut_val *key_val = yyjson_mut_strcpy(doc, key);
    if (yyjson_mut_obj_replace(obj, key_val, val))
	return 1;
    return yyjson_mut_obj_add(obj, key_val, val) ? 1 : -1;
}

#define CSONPATH_APPEND_AT(container, at, el, do_incref)		\
    _Generic(at,							\
	     size_t: csonpath_yyjson_mut_append_at_idx,			\
	     intptr_t: csonpath_yyjson_mut_append_at_idx,			\
	     int: csonpath_yyjson_mut_append_at_idx,			\
	     const char *: csonpath_yyjson_mut_append_at_key)		\
	 (container, at, el, (yyjson_mut_doc *)cjp->backend_ctx)

static inline int csonpath_yyjson_mut_remove_child_idx(yyjson_mut_val *arr, size_t idx) {
    return yyjson_mut_arr_remove(arr, idx) ? 1 : -1;
}

static inline int csonpath_yyjson_mut_remove_child_key(yyjson_mut_val *obj, const char *key) {
    return yyjson_mut_obj_remove_key(obj, key) ? 1 : -1;
}

#define CSONPATH_REMOVE_CHILD(obj, child_info)				\
    ((child_info).type == CSONPATH_INTEGER				\
     ? csonpath_yyjson_mut_remove_child_idx(obj, (child_info).idx)	\
     : csonpath_yyjson_mut_remove_child_key(obj, (child_info).key))

#define CSONPATH_NEED_FOREACH_REDO(o) 0

struct csonpath_yyjson_mut_find_all_ret {
    int size;
    int i;
    yyjson_mut_val **ret;
};

#define CSONPATH_FIND_ALL_RET_INIT()							\
    ({										\
	struct csonpath_yyjson_mut_find_all_ret *r = malloc(sizeof *r);			\
	*r = (struct csonpath_yyjson_mut_find_all_ret){.size = 1024,			\
									\
						    .ret = malloc(sizeof *r->ret * 1024)};	\
	r;})

#define CSONPATH_FIND_ALL_RET struct csonpath_yyjson_mut_find_all_ret *

#define CSONPATH_ARRAY_APPEND(ar, o)						\
    _Generic(ar,							\
	     struct csonpath_yyjson_mut_find_all_ret *: csonpath_yyjson_mut_find_all_append, \
	     yyjson_mut_val *: csonpath_yyjson_mut_fail_on_non_mut)(ar, o)

static inline void csonpath_yyjson_mut_find_all_append(struct csonpath_yyjson_mut_find_all_ret *ar, yyjson_mut_val *o) {
    if (ar->i + 1 >= ar->size) {
	ar->size = ar->size << 1;
	ar->ret = realloc(ar->ret, ar->size * sizeof *ar->ret);
    }
    ar->ret[ar->i++] = o;
}

static inline void csonpath_yyjson_mut_free_find_all(struct csonpath_yyjson_mut_find_all_ret *far) {
    if (!far)
	return;
    free(far->ret);
    free(far);
}

static inline int csonpath_yyjson_mut_fail_on_non_mut(yyjson_mut_val *o_) {
    fprintf(stderr, "unssuported on mut");
    abort();
    (void)o_;
    return -1;
}

#define CSONPATH_NEW_ARRAY()							\
    yyjson_mut_arr((yyjson_mut_doc *)cjp->backend_ctx)

#define CSONPATH_NEW_OBJECT()							\
    yyjson_mut_obj((yyjson_mut_doc *)cjp->backend_ctx)

static inline void csonpath_yyjson_mut_remove_val(yyjson_mut_val *o_) { (void)o_; }

#define CSONPATH_REMOVE(o)							\
    _Generic(o,								\
	     struct csonpath_yyjson_mut_find_all_ret *: csonpath_yyjson_mut_free_find_all, \
	     yyjson_mut_val *: csonpath_yyjson_mut_remove_val)(o)

#include "csonpath.h"

#endif
