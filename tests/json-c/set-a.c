#include <stdio.h>
#include <assert.h>
#include "csonpath_json-c.h"

const char *json_str = "{"
  "\"0\": \"this is ZERO\","
  " \"a\": \"la y'a l'A\","
  " \"b\": { \"B\": \"la y'a l'B\"},"
  " \"array\": [0, \"ah\", \"oh\"],"
  " \"ar2\": [{\"o\": 100}, {\"p\": 100}]"
  "}";

int main(void)
{
  struct csonpath *p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *ret, *val;

  assert((p = csonpath_new("$.c")));
  ret = csonpath_find_first(p, jobj);
  assert(!ret);

  val = json_object_new_string("la y'a l'C");
  assert(csonpath_update_or_create(p, jobj, val) == 1);
  json_object_put(val);
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'C"));

  val = json_object_new_string("la y'a l'gros C");
  assert(csonpath_update_or_create(p, jobj, val) == 1);
  json_object_put(val);
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'gros C"));

  p = csonpath_set_path(p, "$['array'][0]");
  val = json_object_new_string("la y'a l'D");
  assert(csonpath_update_or_create(p, jobj, val) == 1);
  json_object_put(val);
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'D"));

  p = csonpath_set_path(p, "$.ar2[0].o");
  val = json_object_new_string("la y'a l'E");
  assert(csonpath_update_or_create(p, jobj, val) == 1);
  json_object_put(val);
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'E"));

  p = csonpath_set_path(p, "$['ar2'][*].p");
  val = json_object_new_string("la y'a l'F");
  assert(csonpath_update_or_create(p, jobj, val) == 2);
  json_object_put(val);
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'F"));
  ret = csonpath_find_all(p, jobj);

  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 2);
  json_object_put(ret);

  p = csonpath_set_path(p, "$..0");
  val = json_object_new_string("la y'a encore 0");
  assert(csonpath_update_or_create(p, jobj, val) == 1);
  json_object_put(val);

  p = csonpath_set_path(p, "$");
  printf("check error happen:\n");
  val = json_object_new_string("la y'a encore 0");
  assert(csonpath_update_or_create(p, jobj, val) == -1);
  json_object_put(val);

  json_object_put(jobj);
  csonpath_destroy(p);
}
