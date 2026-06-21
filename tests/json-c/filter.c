#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "csonpath_json-c.h"

// $.compute[?dn="sys/chassis-1/blade-6"].vendor

const char *json_str = "{"
	" \"array\": ["
	"{\"a\": \"la\", \"b\": 1},"
	"{\"a\": 2, \"b\": \"la\"},"
	"{\"b\": 400}"
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
  struct csonpath *p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *jobj_ar = json_object_object_get(jobj, "array");
  struct json_object *ret;

  assert((p = csonpath_new("$.array[?a==2]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(json_object_get_int(json_object_object_get(ret, "a")) == 2);

  assert((p = csonpath_set_path(p, "$.array[?b==400]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(json_object_get_int(json_object_object_get(ret, "b")) == 400);

  assert((p = csonpath_set_path(p, "$.array[?b>100]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(json_object_get_int(json_object_object_get(ret, "b")) == 400);

  assert((p = csonpath_set_path(p, "$.array[?b<100]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(json_object_get_int(json_object_object_get(ret, "b")) == 1);

  assert((p = csonpath_set_path(p, "$.array[?b>0]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(json_object_get_int(json_object_object_get(ret, "b")) == 1);

  assert((p = csonpath_set_path(p, "$.array[?a=\"la\"]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);

  p = csonpath_set_path(p, "$.array[?[\"a\"]==\"la\"]");
  ret = csonpath_find_first(p, jobj);
  assert(ret);

  assert((p = csonpath_set_path(p, "$.array[?a==9999]")));
  p->return_empty_array = 0;
  ret = csonpath_find_first(p, jobj);
  assert(ret == NULL);

  p->return_empty_array = 1;
  ret = csonpath_find_first(p, jobj);
  assert(ret == NULL);
  p->return_empty_array = 0;

  p = csonpath_set_path(p, "$.array[?(@[\"a\"]==\"la\")]");
  ret = csonpath_find_first(p, jobj);
  assert(ret);

  assert((p = csonpath_set_path(p, "$.array[?(@[\"a\"]==\"la\")]")));
  json_object *filter_val = json_object_new_string("la y'a l'C");
  int iret = csonpath_update_or_create(p, jobj, filter_val);
  json_object_put(filter_val);
  assert(iret == 1);
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(jobj_ar, 0)), "la y'a l'C"));

  p = csonpath_set_path(p, "$.compute[?dn=\"sys/chassis-1/blade-6\"].vendor");
  ret = csonpath_find_first(p, jobj);
  assert(ret);

  p = csonpath_set_path(p, "$.array[?b=\"la\"]");
  iret = csonpath_update_or_create_callback(p, jobj, update_callback,
					   "oh no !");
  assert(iret == 1);
  p = csonpath_set_path(p, "$.array[?b=\"oh no !\"]");
  assert(csonpath_find_first(p, jobj));

  json_object_put(jobj);
  csonpath_destroy(p);

  jobj = NULL;
  p = csonpath_new("$.ha[?h=~\"eo\"]");
  ret = csonpath_find_all(p, jobj);
  assert(!ret);
  jobj = json_tokener_parse("{\"ha\": [ {\"h\": \"Leodagan\"}"
			    ", {\"h\": \"George\"} ]}");

  assert((p = csonpath_set_path(p, "$.ha[?h != \"Leodagan\"]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  json_object_put(ret);

  p = csonpath_set_path(p, "$.ha[?h=~\"eo\"]");
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  json_object_put(ret);

  p = csonpath_set_path(p, "$.ha[?(@.h=~\"eo\")]");
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  json_object_put(ret);

  p = csonpath_set_path(p, "$.ha[?h=~\"wololo\"]");

  ret = csonpath_find_all(p, jobj);
  assert(!ret);

  json_object_put(ret);
  json_object_put(jobj);
  csonpath_destroy(p);

  jobj = json_tokener_parse("{\"ha\": [ {\"i\": {\"h\": \"Leodagan\"}}"
			    ", {\"h\": \"George\"} ]}");
  assert((p = csonpath_new("$.ha[?i.h==\"Leodagan\"]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  json_object_put(ret);

  assert((p = csonpath_set_path(p, "$.ha[?i.h =~ \"gan\"].i.h")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  json_object_put(ret);
  assert(csonpath_find_first(p, jobj));

  assert((p = csonpath_set_path(p, "$.ha[?(@[\"i\"].h =~ \"gan\")].i.h")));
  assert(csonpath_find_first(p, jobj));

  assert((p = csonpath_set_path(p, "$.ha[?(@.i[\"h\"] =~ \"gan\")].i.h")));
  assert(csonpath_find_first(p, jobj));

  assert((p = csonpath_set_path(p, "$.ha[?(@[\"i\"][\"h\"] =~ \"gan\")].i.h")));
  assert(csonpath_find_first(p, jobj));

  assert((p = csonpath_set_path(p, "$.ha[?(@[\"i\"][\"h\"] =~ \"gan\")][\"i\"][\"h\"]")));
  assert(csonpath_find_first(p, jobj));

  assert((p = csonpath_set_path(p, "$.ha[?i.h=~\"no_match_xyz\"]")));
  p->return_empty_array = 1;
  ret = csonpath_find_all(p, jobj);
  assert(ret != NULL);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 0);
  json_object_put(ret);
  p->return_empty_array = 0;
  ret = csonpath_find_all(p, jobj);
  assert(ret == NULL);
  json_object_put(ret);

  json_object_put(jobj);

  csonpath_destroy(p);

  /* == null semantics: absent -> match, JSON null -> match, other -> no match */
  jobj = json_tokener_parse("{"
    "\"items\": ["
    "{\"a\": \"x\"},"
    "{\"a\": null},"
    "{}"
    "]"
    "}");

  assert(jobj);
  assert((p = csonpath_new("$.items[?a == null]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 2); /* absent and JSON null */
  json_object_put(ret);

  assert((p = csonpath_set_path(p, "$.items[?a != null]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 1); /* only \"x\" */
  json_object_put(ret);

  /* ? (existence) matches JSON null too */
  assert((p = csonpath_set_path(p, "$.items[?(@.a)]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) > 0); /* \"x\" and JSON null */
  json_object_put(ret);

  json_object_put(jobj);
  csonpath_destroy(p);

  /* == true / == false semantics */
  jobj = json_tokener_parse("{"
    "\"items\": ["
    "{\"a\": true},"
    "{\"a\": false},"
    "{\"a\": 1},"
    "{\"a\": \"true\"},"
    "{}"
    "]"
    "}");

  assert(jobj);

  /* == true matches only JSON true */
  assert((p = csonpath_new("$.items[?a == true]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 1); /* only true */
  json_object_put(ret);

  /* == false matches only JSON false */
  assert((p = csonpath_set_path(p, "$.items[?a == false]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 1); /* only false */
  json_object_put(ret);

  /* != true matches false, number, string, absent */
  assert((p = csonpath_set_path(p, "$.items[?a != true]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 4); /* false, 1, "true", absent */
  json_object_put(ret);

  /* != false matches true, number, string, absent */
  assert((p = csonpath_set_path(p, "$.items[?a != false]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 4); /* true, 1, "true", absent */
  json_object_put(ret);

  /* ? (existence) matches both true and false */
  assert((p = csonpath_set_path(p, "$.items[?(@.a)]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 4); /* true, false, 1, "true" */
  json_object_put(ret);

  json_object_put(jobj);
  csonpath_destroy(p);

  /* Test filter with a medium key (200 chars): must match.
   * If char is signed and csonpath_do.h lacks an unsigned char cast,
   * the runtime filter_next becomes negative and silently skips the key. */
  {
#define KEY_LEN 200
    char key[KEY_LEN + 1];
    memset(key, 'k', KEY_LEN);
    key[KEY_LEN] = '\0';

    struct json_object *obj = json_object_new_object();
    struct json_object *arr = json_object_new_array();
    struct json_object *el = json_object_new_object();
    json_object_object_add(el, key, json_object_new_string("y"));
    json_object_array_add(arr, el);
    json_object_object_add(obj, "arr", arr);

    char path[KEY_LEN + 50];
    snprintf(path, sizeof(path), "$.arr[?['%s']=\"y\"]", key);
    p = csonpath_new(path);
    assert(p);
    ret = csonpath_find_all(p, obj);
    assert(ret && json_object_is_type(ret, json_type_array)
           && json_object_array_length(ret) == 1);
    json_object_put(ret);
    json_object_put(obj);
    csonpath_destroy(p);
#undef KEY_LEN
  }

  /* Test filter with a large key (>255 chars) to overflow a char length storage */
  {
    const int key_len = 300;
    char *big_key = malloc(key_len + 1);
    memset(big_key, 'k', key_len);
    big_key[key_len] = '\0';

    struct json_object *big_obj = json_object_new_object();
    struct json_object *big_arr = json_object_new_array();
    struct json_object *el1 = json_object_new_object();
    struct json_object *el2 = json_object_new_object();
    json_object_object_add(el1, big_key, json_object_new_string("match"));
    json_object_object_add(el2, big_key, json_object_new_string("no"));
    json_object_array_add(big_arr, el1);
    json_object_array_add(big_arr, el2);
    json_object_object_add(big_obj, "arr", big_arr);

    char *path = malloc(key_len + 50);
    sprintf(path, "$.arr[?['%s'] = \"match\"]", big_key);
    p = csonpath_new(path);
    assert(p == NULL); /* clé trop longue : doit refuser de compiler */
    (void)ret;

    free(path);
    json_object_put(big_obj);
    free(big_key);
  }

  csonpath_destroy(p);
}
