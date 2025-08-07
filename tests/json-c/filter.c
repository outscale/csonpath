#include <stdio.h>
#include <assert.h>
#include "csonpath_json-c.h"

const char *json_str = "{"
	" \"array\": ["
	"{\"a\": \"la\", \"b\": 1},"
	"{\"a\": 2, \"b\": \"la\"},"
	"{\"b\": 4}"
	"]"
  "}";

int main(void)
{
  struct csonpath p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *ret;

  assert(csonpath_init(&p, "$.array[?a=\"la\"]") >= 0);
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  int iret = csonpath_update_or_ceate(&p, jobj, json_object_new_string("la y'a l'C"));
  printf("%d - %s\n", iret, json_object_to_json_string(jobj));

  json_object_put(jobj);
  csonpath_destroy(&p);
}
