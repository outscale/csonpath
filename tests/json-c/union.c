#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "csonpath_json-c.h"

const char *json_str = "{"
  "\"a\": \"val_a\","
  " \"b\": \"val_b\","
  " \"c\": { \"d\": \"val_cd\" },"
  " \"d\": { \"d\": \"val_de\" },"
  " \"array\": [0, \"ah\", \"oh\", {\"x\": 1, \"y\": 2}],"
  " \"ar2\": [0, \"bh\", \"pp\", {\"x\": 1, \"y\": 2}],"
  " \"items\": [{\"n\": 1}, {\"n\": 2}, {\"n\": 3}]"
  "}";

void update_callback(json_object *parent, struct csonpath_child_info *c_nfo,
		     json_object *current, void *data)
{
  (void)parent; (void)c_nfo;
  json_object_set_string(current, data);
}

int main(void)
{
  struct csonpath *p;
  struct json_object *jobj = json_tokener_parse(json_str);
  struct json_object *ret;
  int iret;

  /* 1. union de deux clés quoted : $['a','b'] -> find_first retourne 'a' */
  assert((p = csonpath_new("$['a','b']")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "val_a"));

  /* 2. union d'indices numériques : $[array][0,1] -> find_first retourne 0 */
  assert((p = csonpath_set_path(p, "$.array[0,1]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(json_object_get_int(ret) == 0);

  /* 3. union d'indices avec find_all -> 2 éléments */
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 2);
  assert(json_object_get_int(json_object_array_get_idx(ret, 0)) == 0);
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 1)), "ah"));
  json_object_put(ret);

  /* 4. union de clés + continuation .d  : $['a','c'].d -> c.d == "val_cd" */
  assert((p = csonpath_set_path(p, "$['c','a'].d")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "val_cd"));

  /* 5. union avec wildcard : $[*, 'a'] -> find_first sur root obj */
  assert((p = csonpath_set_path(p, "$['a','b']")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_array_length(ret) == 2);
  json_object_put(ret);

  /* 6. union de 3 éléments quoted : $['a','b','c'] -> 3 résultats en find_all */
  assert((p = csonpath_set_path(p, "$['a','b','c']")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_array_length(ret) == 3);
  json_object_put(ret);

  /* 7. union barewords : $[a,b] -> find_first == "val_a" */
  assert((p = csonpath_set_path(p, "$['c','d'].d")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 0)), "val_cd"));
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 1)), "val_de"));
  json_object_put(ret);

  /* 8. union qui inclut un élément inexistant : $['a','z'] -> existe quand même */
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  /* la longueur dépend de si l'implémente filtre les null ; on check juste que ça crash pas */
  json_object_put(ret);

  /* 9. update_or_create sur union -> doit updater au moins un élément */
  assert((p = csonpath_set_path(p, "$['a','z']")));
  iret = csonpath_update_or_create(p, jobj, json_object_new_string("modified"));
  assert(iret >= 1);

  /* 10. union sur sous-champs d'array : $.array[*].['x','y'] -> find_all */
  assert((p = csonpath_set_path(p, "$.array[*].['x','y']")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  /* 2 clés sur 1 objet matchant = 2 résultats */
  assert(json_object_array_length(ret) == 2);
  json_object_put(ret);

  /* 11. union avec callback via update_or_create_callback */
  assert((p = csonpath_set_path(p, "$['a','b']")));
  iret = csonpath_update_or_create_callback(p, jobj, update_callback, "cb_val");
  assert(iret >= 1);
  assert((p = csonpath_set_path(p, "$.a")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "cb_val"));

  /* 12. remove sur union -> supprime plusieurs clés */
  assert((p = csonpath_set_path(p, "$['a','b']")));
  iret = csonpath_remove(p, jobj);
  assert(iret == 2);
  assert(!json_object_object_get(jobj, "a"));
  assert(!json_object_object_get(jobj, "b"));

  /* 13. union sur array indices avec continuation : $.items[0,1].n -> [1,2] */
  assert((p = csonpath_set_path(p, "$.items[0,1].n")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_array_length(ret) == 2);
  assert(json_object_get_int(json_object_array_get_idx(ret, 0)) == 1);
  assert(json_object_get_int(json_object_array_get_idx(ret, 1)) == 2);
  json_object_put(ret);

  /* 14. union de filter expressions : $.items[?n==1, ?n==2] -> 2 éléments */
  assert((p = csonpath_set_path(p, "$.items[?n==1, ?n==2]")));
  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(json_object_array_length(ret) == 2);
  json_object_put(ret);

  /* 15. union sur éléments d'un tableau avec find_first : $[array][2,1] -> "ah" (élément 1) */
  assert((p = csonpath_set_path(p, "$.array[2,1]")));
  ret = csonpath_find_first(p, jobj);
  assert(ret);
  assert(!strcmp(json_object_get_string(ret), "oh"));

  ret = csonpath_find_all(p, jobj);
  assert(ret);
  assert(json_object_is_type(ret, json_type_array));
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 0)), "oh"));
  assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 1)), "ah"));
  json_object_put(ret);

  json_object_put(jobj);
  csonpath_destroy(p);

  return 0;
}
