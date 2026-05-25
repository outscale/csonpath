#include <stdio.h>
#include <assert.h>
#include "csonpath_json-c.h"

const char *json_str = "{"
	" \"array\": ["
	"{\"a\": \"la\", \"b\": 1},"
	"{\"a\": 2, \"b\": \"la\"},"
	"{\"b\": 400}"
	"],"
	"\"val\": \"la\","
	"\"val2\": \"val\""
"}";


int main(void)
{
  struct csonpath *p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *ret;

  assert(jobj);
  assert((p = csonpath_new("$.array[?a==$.val]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(json_object_object_get(ret, "a")), "la"));
  assert((p = csonpath_set_path(p, "$.array[?a==$.val2]")));
  ret = csonpath_find_first(p, jobj);
  assert(!ret);

  assert((p = csonpath_set_path(p, "$[$.val2]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);

  assert((p = csonpath_set_path(p, "$.array[?a==$[$.val2]]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);

  json_object_put(jobj);

  csonpath_destroy(p);

}
