#ifndef CSONPATH_JSON_C_H_
#define CSONPATH_JSON_C_H_

#include <string.h>
#include "json.h"

#define CSONPATH_JSON struct json_object *

#define CSONPATH_NULL NULL

#define CSONPATH_GET csonpath_json_c_get

#define CSONPATH_AT csonpath_json_c_at

#define CSONPATH_FOREACH(obj, el, code)				\
  if (json_object_is_type(obj, json_type_object)) {		\
    json_object_object_foreach(obj, key_, val) { code }		\
  } else if (json_object_is_type(obj, json_type_array)) {	\
    int array_len_ = json_object_array_length(obj);		\
    for (int idx_ = 0; idx_ < array_len_; ++idx_) {		\
      el = json_object_array_get_idx(obj, idx_);		\
      code							\
    }								\
  }



#define CSONPATH_NEW_ARRAY() json_object_new_array()

#define CSONPATH_ARRAY_APPEND(array, el) json_object_array_add(array, el)

static struct json_object *csonpath_json_c_get(struct json_object *obj,
					       const char *key)
{
  return json_object_object_get(obj, key);
}

static struct json_object *csonpath_json_c_at(struct json_object *obj,
					      int at)
{
  return json_object_array_get_idx(obj, at);
}



#include "csonpath.h"

#endif
