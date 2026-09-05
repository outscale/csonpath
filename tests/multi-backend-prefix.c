#include <stdio.h>
#include <string.h>
#include <assert.h>

/* json-c backend with prefix, so it can coexist with yyjson */
#define CSONPATH_USE_PREFIX
#include "csonpath_json-c.h"

/* yyjson backend, unprefixed */
#undef CSONPATH_USE_PREFIX
#include "csonpath_yyjson_const.h"

static const char *json_str =
    "{\"a\":\"x\",\"b\":{\"B\":\"y\"},\"array\":[0,\"ah\",\"oh\"],"
    "\"items\":[{\"name\":\"A\",\"price\":10},{\"name\":\"B\",\"price\":50}]}";

int main(void)
{
    /* json-c side */
    struct json_object *jc_root = json_tokener_parse(json_str);
    struct csonpath *jc_p = json_c_csonpath_new("$.a");
    struct json_object *jc_v = json_c_csonpath_find_first(jc_p, jc_root);
    assert(jc_v && !strcmp(json_object_get_string(jc_v), "x"));

    jc_p = json_c_csonpath_set_path(jc_p, "$..name");
    struct json_object *jc_all = json_c_csonpath_find_all(jc_p, jc_root);
    assert(json_object_array_length(jc_all) == 2);

    json_c_csonpath_destroy(jc_p);
    json_object_put(jc_all);
    json_object_put(jc_root);

    /* yyjson side */
    yyjson_doc *yy_doc = yyjson_read(json_str, strlen(json_str), 0);
    yyjson_val *yy_root = yyjson_doc_get_root(yy_doc);
    struct csonpath *yy_p = csonpath_new("$.b.B");
    yyjson_val *yy_v = csonpath_find_first(yy_p, yy_root);
    assert(yy_v && !strcmp(yyjson_get_str(yy_v), "y"));

    yy_p = csonpath_set_path(yy_p, "$..name");
    struct find_all_ret *yy_all = csonpath_find_all(yy_p, yy_root);
    assert(yy_all->i == 2);
    free_find_all(yy_all);

    csonpath_destroy(yy_p);
    yyjson_doc_free(yy_doc);

    return 0;
}
