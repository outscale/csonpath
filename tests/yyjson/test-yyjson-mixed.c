#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "csonpath_yyjson.h"

static const char *json_str =
    "{\"a\":\"x\",\"b\":{\"B\":\"y\"},\"array\":[0,\"ah\",\"oh\"],"
    "\"items\":[{\"name\":\"A\",\"price\":10},{\"name\":\"B\",\"price\":50}]}";

int main(void)
{
    /* const backend (immutable yyjson_doc / yyjson_val) */
    yyjson_doc *doc = yyjson_read(json_str, strlen(json_str), 0);
    yyjson_val *root = yyjson_doc_get_root(doc);

    struct csonpath *p = yyjson_csonpath_new("$.a");
    yyjson_val *v = yyjson_csonpath_find_first(p, root);
    assert(v && !strcmp(yyjson_get_str(v), "x"));

    p = yyjson_csonpath_set_path(p, "$..name");
    struct find_all_ret *r = yyjson_csonpath_find_all(p, root);
    assert(r->i == 2);
    free_find_all(r);

    yyjson_csonpath_destroy(p);
    yyjson_doc_free(doc);

    /* mut backend (mutable yyjson_mut_doc / yyjson_mut_val) */
    yyjson_mut_doc *mdoc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *mroot = yyjson_mut_obj(mdoc);
    yyjson_mut_obj_add(mroot,
                       yyjson_mut_strcpy(mdoc, "a"),
                       yyjson_mut_strcpy(mdoc, "x"));
    yyjson_mut_doc_set_root(mdoc, mroot);
    yyjson_mut_val *mroot_get = yyjson_mut_doc_get_root(mdoc);

    struct csonpath *mp = yyjson_mut_csonpath_new("$.a");
    yyjson_mut_val *mv = yyjson_mut_csonpath_find_first(mp, mroot_get);
    assert(mv && !strcmp(yyjson_mut_get_str(mv), "x"));

    mp = yyjson_mut_csonpath_set_path(mp, "$.a");
    mv = yyjson_mut_csonpath_find_first(mp, mroot_get);
    assert(mv && !strcmp(yyjson_mut_get_str(mv), "x"));

    /* mutable update_or_create */
    mp->backend_ctx = mdoc;
    yyjson_mut_val *new_val = yyjson_mut_strcpy(mdoc, "z");
    int ret = yyjson_mut_csonpath_update_or_create(mp, mroot_get, new_val);
    assert(ret == 1);
    mv = yyjson_mut_obj_get(mroot_get, "a");
    assert(mv && !strcmp(yyjson_mut_get_str(mv), "z"));

    /* mutable create missing path */
    mp = yyjson_mut_csonpath_set_path(mp, "$.c");
    mp->backend_ctx = mdoc;
    yyjson_mut_val *new_obj = yyjson_mut_obj(mdoc);
    ret = yyjson_mut_csonpath_update_or_create(mp, mroot_get, new_obj);
    assert(ret == 1);
    mv = yyjson_mut_obj_get(mroot_get, "c");
    assert(mv && yyjson_mut_is_obj(mv));

    yyjson_mut_csonpath_destroy(mp);
    yyjson_mut_doc_free(mdoc);

    return 0;
}
