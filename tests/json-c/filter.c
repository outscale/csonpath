#include <stdio.h>
#include <assert.h>
#include "csonpath_json-c.h"

// $.compute[?dn="sys/chassis-1/blade-6"].vendor

const char *json_str = "{"
	" \"array\": ["
	"{\"a\": \"la\", \"b\": 1},"
	"{\"a\": 2, \"b\": \"la\"},"
	"{\"b\": 4}"
	"],"
  	"\"compute\": [{\"dn\": \"sys/chassis-1/blade-6\", \"vendor\": \"coucou\"}]"
  "}";

void update_callback(json_object *parent, struct csonpath_child_info *c_nfo,
		     json_object *curent, void *data)
{
  json_object *tmp = json_object_object_get(curent, "b");
  json_object_set_string(tmp, data);
}

int main(void)
{
  struct csonpath p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *jobj_ar = json_object_object_get(jobj, "array");
  struct json_object *ret;

  assert(csonpath_init(&p, "$.array[?a=\"la\"]") >= 0);
  ret = csonpath_find_first(&p, jobj);
  assert(ret);
  int iret = csonpath_update_or_ceate(&p, jobj, json_object_new_string("la y'a l'C"));
  assert(iret == 1);
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(jobj_ar, 0)), "la y'a l'C"));

  csonpath_set_path(&p, "$.array[?b=\"la\"]");
  iret = csonpath_update_or_ceate_callback(&p, jobj, update_callback,
					   "oh no !");
  assert(iret == 1);
  csonpath_set_path(&p, "$.array[?b=\"oh no !\"]");
  assert(csonpath_find_first(&p, jobj));

  json_object_put(jobj);
  csonpath_destroy(&p);

  jobj = json_tokener_parse("{\"ha\": [ {\"h\": \"Leodagan\"}"
			    ", {\"h\": \"George\"} ]}");
  assert(csonpath_init(&p, "$.ha[?h != \"Leodagan\"]") >= 0);
  ret = csonpath_find_all(&p, jobj);
  assert(ret);
  json_object_put(ret);

  csonpath_set_path(&p, "$.ha[?h=~\"eo\"]");
  ret = csonpath_find_all(&p, jobj);
  assert(ret);
  json_object_put(ret);

  csonpath_set_path(&p, "$.ha[?(@.h=~\"eo\")]");
  ret = csonpath_find_all(&p, jobj);
  assert(ret);
  json_object_put(ret);

  csonpath_set_path(&p, "$.ha[?h=~\"wololo\"]");

  ret = csonpath_find_all(&p, jobj);
  assert(!ret);

  json_object_put(ret);
  json_object_put(jobj);
  csonpath_destroy(&p);

  jobj = json_tokener_parse("{\"ha\": [ {\"i\": {\"h\": \"Leodagan\"}}"
			    ", {\"h\": \"George\"} ]}");
  assert(csonpath_init(&p, "$.ha[?i.h==\"Leodagan\"]") >= 0);
  ret = csonpath_find_all(&p, jobj);
  assert(ret);
  json_object_put(ret);

  assert(!csonpath_set_path(&p, "$.ha[?i.h =~ \"L*gan\"].i.h"));

  if (csonpath_compile(&p) < 0) {
    printf("compile error: %s\n", p.compile_error);
  }
  assert(iret);
  ret = csonpath_find_all(&p, jobj);
  assert(ret);
  json_object_put(ret);

  json_object_put(jobj);

  csonpath_destroy(&p);
}
