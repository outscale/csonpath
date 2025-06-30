#ifndef CSONPATH_JSON_C_H_
#define CSONPATH_JSON_C_H_

#include <string.h>
#include "json.h"

#define CSONPATH_JSON struct json_object *

#define CSONPATH_NULL NULL

#define CSONPATH_GET csonpath_json_c_get

#define CSONPATH_AT csonpath_json_c_at

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
