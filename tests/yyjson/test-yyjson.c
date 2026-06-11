#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "csonpath_yyjson.h"

const char *json_str = "{\"a\":\"x\",\"b\":{\"B\":\"y\"},\"array\":[0,\"ah\",\"oh\"],\"items\":[{\"name\":\"A\",\"price\":10},{\"name\":\"B\",\"price\":50}],\"key\":\"a\",\"threshold\":20}";

int main(void)
{
	yyjson_doc *doc = yyjson_read(json_str, strlen(json_str), 0);
	yyjson_val *root = yyjson_doc_get_root(doc);
	yyjson_val *v;
	struct find_all_ret *r;
	struct csonpath *p = csonpath_new("$.a");

	v = csonpath_find_first(p, root);
	assert(v && !strcmp(yyjson_get_str(v), "x"));

	p = csonpath_set_path(p, "$.b.B");
	v = csonpath_find_first(p, root);
	assert(v && !strcmp(yyjson_get_str(v), "y"));

	p = csonpath_set_path(p, "$['a']");
	v = csonpath_find_first(p, root);
	assert(v && !strcmp(yyjson_get_str(v), "x"));

	p = csonpath_set_path(p, "$.array[1]");
	v = csonpath_find_first(p, root);
	assert(v && !strcmp(yyjson_get_str(v), "ah"));

	p = csonpath_set_path(p, "$.array[*]");
	r = csonpath_find_all(p, root);
	assert(r->i == 3);
	free_find_all(r);

	p = csonpath_set_path(p, "$..name");
	r = csonpath_find_all(p, root);
	assert(r->i == 2);
	free_find_all(r);

	p = csonpath_set_path(p, "$.z|$.a");
	v = csonpath_find_first(p, root);
	assert(v && !strcmp(yyjson_get_str(v), "x"));

	p = csonpath_set_path(p, "$['a','b']");
	r = csonpath_find_all(p, root);
	assert(r->i == 2);
	free_find_all(r);

	p = csonpath_set_path(p, "$.items[?price>20]");
	r = csonpath_find_all(p, root);
	assert(r->i == 1);
	free_find_all(r);

	p = csonpath_set_path(p, "$.items[?name=\"A\"]");
	v = csonpath_find_first(p, root);
	assert(v && yyjson_get_type(v) == YYJSON_TYPE_OBJ);

	p = csonpath_set_path(p, "$.items[?name=~\"A.*\"]");
	r = csonpath_find_all(p, root);
	assert(r->i == 1);
	free_find_all(r);

	p = csonpath_set_path(p, "$.items[?price>$.threshold]");
	r = csonpath_find_all(p, root);
	assert(r->i == 1);
	free_find_all(r);

	p = csonpath_set_path(p, "$[$.key]");
	v = csonpath_find_first(p, root);
	assert(v && !strcmp(yyjson_get_str(v), "x"));

	p = csonpath_set_path(p, "$.items[?(@.name)]");
	r = csonpath_find_all(p, root);
	assert(r->i == 2);
	free_find_all(r);

	p = csonpath_set_path(p, "$.array[0,1]");
	r = csonpath_find_all(p, root);
	assert(r->i == 2);
	free_find_all(r);

	csonpath_destroy(p);
	yyjson_doc_free(doc);
	return 0;
}
