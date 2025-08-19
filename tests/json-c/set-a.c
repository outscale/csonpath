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
  struct csonpath p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *ret;

  assert(csonpath_init(&p, "$.c") >= 0);
  ret = csonpath_find_first(&p, jobj);
  assert(!ret);
  assert(csonpath_update_or_ceate(&p, jobj, json_object_new_string("la y'a l'C")) == 1);
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'C"));

  assert(csonpath_update_or_ceate(&p, jobj, json_object_new_string("la y'a l'gros C")) == 1);
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'gros C"));

  csonpath_set_path(&p, "$['array'][0]");
  assert(csonpath_update_or_ceate(&p, jobj, json_object_new_string("la y'a l'D")) == 1);
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'D"));

  csonpath_set_path(&p, "$.ar2[0].o");
  assert(csonpath_update_or_ceate(&p, jobj, json_object_new_string("la y'a l'E")) == 1);
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'E"));

  csonpath_set_path(&p, "$['ar2'][*].p");
  assert(csonpath_update_or_ceate(&p, jobj, json_object_new_string("la y'a l'F")) == 2);
  ret = csonpath_find_first(&p, jobj);
  json_object_get(ret); // update references counter
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "la y'a l'F"));
  ret = csonpath_find_all(&p, jobj);

  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 2);
  json_object_put(ret);

  csonpath_set_path(&p, "$..0");
  assert(csonpath_update_or_ceate(&p, jobj, json_object_new_string("la y'a encore 0")) == 1);

  json_object_put(jobj);
  csonpath_destroy(&p);
}
