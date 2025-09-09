#ifndef CSONPATH_YYJSON_H_
#define CSONPATH_YYJSON_H_

/*
 * !!!!!! WARNING !!!!!!!
 * This lib implement csonpath with yyjson_val, and as sure
 * are not mutable,so only non mutable functions work here
 * also, it has its own return type for find_all:
 * struct find_all_ret *, which need to be free using:
 * free_find_all
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include "yyjson.h"

#define CSONPATH_JSON yyjson_val *

#define CSONPATH_NULL NULL

#define CSONPATH_GET yyjson_obj_get

#define CSONPATH_AT yyjson_arr_get

#define CSONPATH_IS_OBJ(obj) (yyjson_get_type(obj) == YYJSON_TYPE_OBJ)
#define CSONPATH_IS_ARRAY(obj) (yyjson_get_type(obj) == YYJSON_TYPE_ARR)
#define CSONPATH_IS_STR(obj) (yyjson_get_type(obj) == YYJSON_TYPE_STR)

struct csonpath_child_info;
typedef void (*yyjson_val_callback)(yyjson_val *, struct csonpath_child_info *, yyjson_val *, void *);

#define CSONPATH_CALLBACK yyjson_val_callback

#define CSONPATH_CALLBACK_DATA void *

#define CSONPATH_GET_STR(obj)			\
    yyjson_get_str(obj)

#define CSONPATH_EQUAL_STR(obj, to_cmp)	({			\
      _Bool r = 0;						\
      if (yyjson_get_type(obj) == YYJSON_TYPE_STR)		\
	  r = !strcmp(yyjson_get_str(obj), to_cmp);		\
      r;							\
    })

#define CSONPATH_EQUAL_NUM(obj, to_cmp)	({			\
      _Bool r = 0;						\
      if (yyjson_get_type(obj) == YYJSON_TYPE_NUM)		\
	r = yyjson_get_num(obj) == to_cmp;			\
      r;							\
    })

#define CSONPATH_CALL_CALLBACK(callback, ctx, child_info, tmp, udata)   \
    callback(ctx, child_info, tmp, udata)


#define CSONPATH_FOREACH(obj, el, code)		\
    CSONPATH_FOREACH_EXT(obj, el, ({code}), key_idx)

#define CSONPATH_FOREACH_EXT(obj, el, code, key_idx_)			\
    if (yyjson_get_type(obj) == YYJSON_TYPE_ARR) {      		\
	size_t key_idx_, max_;						\
	yyjson_val *val;						\
	yyjson_arr_foreach(obj, key_idx_, max_,  el) {			\
	    (void) key_idx_; code;					\
	}								\
    } else if (yyjson_get_type(obj) == YYJSON_TYPE_OBJ) {		\
	size_t idx_, max_;						\
	yyjson_val *key, *val;						\
	yyjson_obj_foreach(obj, idx_, max_, key, el) {			\
	    const char *key_idx_ = yyjson_get_str(key);			\
	    (void) key_idx_; code;					\
	}								\
    }


#define CSONPATH_APPEND_AT(array, at, el)	\
  fail_on_non_mut(NULL)				\


#define CSONPATH_REMOVE_CHILD(a, b)		\
  fail_on_non_mut(NULL)

#define CSONPATH_NEED_FOREACH_REDO(o) 0

struct find_all_ret {
    int size;
    int i;
    yyjson_val **ret;
};

#define CSONPATH_FIND_ALL_RET_INIT()					\
    ({									\
	struct find_all_ret *r = malloc(sizeof *r);			\
	*r = (struct find_all_ret){.size = 1024,.ret = malloc(sizeof *r->ret * 1024)}; \
	r;})

#define CSONPATH_FIND_ALL_RET struct find_all_ret *

#define CSONPATH_ARRAY_APPEND_INCREF(ar, o)		\
    _Generic(ar,					\
	     struct find_all_ret *: find_all_append,	\
	     yyjson_val *: fail_on_non_mut)(ar, o)

static inline void find_all_append(struct find_all_ret *ar, yyjson_val *o) {
    if (ar->i + 1 >= ar->size) {
	ar->size = ar->size << 1;
	ar->ret = realloc(ar->ret, ar->size * sizeof *ar->ret);
    }
    ar->ret[ar->i++] = o;
}

static inline void free_find_all(struct find_all_ret *far) {
    free(far->ret);
    free(far);
}

static inline int fail_on_non_mut(yyjson_val *o_) {
    fprintf(stderr, "unssuported on mut");
    abort();
    (void)o_;
    return -1;
}

static inline yyjson_val *CSONPATH_NEW_ARRAY() {
    fail_on_non_mut(NULL);
    return NULL;
}

static inline yyjson_val *CSONPATH_NEW_OBJECT() {
    fail_on_non_mut(NULL);
    return NULL;
}


#define CSONPATH_REMOVE(o)						\
    _Generic(o,								\
	     struct find_all_ret *: free_find_all,			\
	     yyjson_val *: fail_on_non_mut)(o)

#include "csonpath.h"

#endif
