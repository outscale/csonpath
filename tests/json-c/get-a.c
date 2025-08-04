#include <stdio.h>
#include <assert.h>
#include "csonpath_json-c.h"

const char *json_str = "{"
  "\"0\": \"this is ZERO\","
  " \"a\": \"la y'a l'A\","
  " \"b\": { \"B\": \"la y'a l'B\"},"
  " \"array\": [0, \"ah\", \"oh\"]"
  "}";

#define TRY(a) assert(a >= 0);

int main(void)
{
  struct csonpath p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *ret;

  TRY(csonpath_init(&p, "$.a"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'A"));

  TRY(csonpath_set_path(&p, "$['0']"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "this is ZERO"));

  TRY(csonpath_set_path(&p, "$['b'].B"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  TRY(csonpath_set_path(&p, "$.b.B"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  TRY(csonpath_set_path(&p, "$.z|$.b.B"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  TRY(csonpath_set_path(&p, "$.z.a|$.b.B"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  TRY(csonpath_set_path(&p, "$[0]|$.b.B"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  TRY(csonpath_set_path(&p, "$.b.B|$.z"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  TRY(csonpath_set_path(&p, "$..B"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  ret = csonpath_find_all(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 0)),
		 "la y'a l'B"));
  json_object_put(ret);

  TRY(csonpath_set_path(&p, "$.b[\"B\"]"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'B"));

  TRY(csonpath_set_path(&p, "$.c.B"));
  ret = csonpath_find_first(&p, jobj);
  assert(!ret);

  TRY(csonpath_set_path(&p, "$.array[1]"));
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "ah"));

  TRY(csonpath_set_path(&p, "$.array[1]"));
  ret = csonpath_find_all(&p, jobj);
  assert(ret);
  assert(json_object_array_get_idx(ret, 0));
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 0)), "ah"));
  json_object_put(ret);

  TRY(csonpath_set_path(&p, "$.array[*]"));
  ret = csonpath_find_all(&p, jobj);
  assert(ret);

  assert(!json_object_get_int(json_object_array_get_idx(ret, 0)));
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 1)), "ah"));
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 2)), "oh"));

  json_object_put(ret);

  /* redo all to check lib doesn't mess jobj */
  ret = csonpath_find_all(&p, jobj);
  assert(ret);

  assert(!json_object_get_int(json_object_array_get_idx(ret, 0)));
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 1)), "ah"));
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 2)), "oh"));
  json_object_put(ret);

  int iret = csonpath_remove(&p, jobj);
  assert(iret == 3);
  struct json_object *array = json_object_object_get(jobj, "array");
  assert(array);
  assert(json_object_is_type(json_object_array_get_idx(array, 1), json_type_null));

  TRY(csonpath_set_path(&p, "$.array"));
  iret = csonpath_remove(&p, jobj);
  assert(iret == 1);
  assert(!json_object_object_get(jobj, "array"));

  json_object_put(jobj);
  csonpath_destroy(&p);
}
