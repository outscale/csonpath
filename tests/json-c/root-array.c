#include <stdio.h>
#include <assert.h>
#include "csonpath_json-c.h"

const char *json_str = "["
  "\"this is ZERO\","
  "\"la y'a l'A\","
  "{ \"B\": \"la y'a l'B\"},"
  " [0, \"ah\", \"oh\"]"
  "]";

#define TRY(a) assert((a));

int main(void)
{
  struct csonpath *p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *ret;

  assert(jobj);
  TRY(p = csonpath_new("$[0]"));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "this is ZERO"));

  json_object_put(jobj);
  csonpath_destroy(p);

}
