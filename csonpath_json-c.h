#ifndef CSONPATH_JSON_C_H_
#define CSONPATH_JSON_C_H_

#include <string.h>
#include "json.h"

#define CSONPATH_JSON struct json_object *

#define CSONPATH_NULL NULL

#define CSONPATH_GET csonpath_json_c_get

#define CSONPATH_AT csonpath_json_c_at

#define CSONPATH_NEED_FOREACH_REDO(o)		\
  json_object_is_type(o, json_type_object)

#define CSONPATH_NEW_INT()

#define CSONPATH_REMOVE(o) json_object_put(o)

#define CSONPATH_IS_OBJ(o) json_object_is_type(o, json_type_object)

#define CSONPATH_IS_ARRAY(o) json_object_is_type(o, json_type_array)

#define CSONPATH_FOREACH(obj, el, code)					\
  if (json_object_is_type(obj, json_type_object)) {			\
    json_object_object_foreach(obj, key_idx, el) { (void) key_idx; code } \
  } else if (json_object_is_type(obj, json_type_array)) {		\
    int array_len_ = json_object_array_length(obj);			\
    for (intptr_t key_idx = 0; key_idx < array_len_; ++key_idx) {	\
      el = json_object_array_get_idx(obj, key_idx);			\
      code								\
	}								\
  }

#define CSONPATH_REMOVE_CHILD(obj, child_info)				\
  if (child_info.type == CSONPATH_INTEGER) {				\
    json_object_array_put_idx(obj, child_info.idx, NULL);		\
  } else {								\
    json_object_object_del(obj, child_info.key);			\
  }

#define CSONPATH_NEW_ARRAY() json_object_new_array()

#define CSONPATH_ARRAY_APPEND(array, el) json_object_array_add(array, el)

#define CSONPATH_ARRAY_APPEND_INCREF(array, el) ({	\
      json_object_get(el);				\
      json_object_array_add(array, el);			\
    })

static struct json_object *csonpath_json_c_get(struct json_object *obj,
					       const char *key)
{
  if (!json_object_is_type(obj, json_type_object))
    return NULL;
  return json_object_object_get(obj, key);
}

static struct json_object *csonpath_json_c_at(struct json_object *obj,
					      int at)
{
  if (!json_object_is_type(obj, json_type_array))
    return NULL;
  return json_object_array_get_idx(obj, at);
}



#include "csonpath.h"

#endif
