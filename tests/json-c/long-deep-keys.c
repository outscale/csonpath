#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "csonpath_json-c.h"

#define DEPTH 12
#define KEY_LEN 200

int main(void)
{
    struct json_object *root = json_object_new_object();
    struct json_object *cur = root;
    char key[KEY_LEN + 1];
    char path_dot[DEPTH * (KEY_LEN + 1) + 16];
    char path_bracket[DEPTH * (KEY_LEN + 4) + 16];
    struct csonpath *p;
    struct json_object *ret;
    struct json_object *arr;
    int i;

    memset(key, 'k', KEY_LEN);
    key[KEY_LEN] = '\0';

    strcpy(path_dot, "$");
    strcpy(path_bracket, "$");

    for (i = 0; i < DEPTH; ++i) {
        struct json_object *next = json_object_new_object();
        json_object_object_add(cur, key, next);
        cur = next;

        strcat(path_dot, ".");
        strcat(path_dot, key);

        strcat(path_bracket, "['");
        strcat(path_bracket, key);
        strcat(path_bracket, "']");
    }
    json_object_object_add(cur, "leaf", json_object_new_string("deep value"));
    strcat(path_dot, ".leaf");
    strcat(path_bracket, "['leaf']");

    /* Dot access on deep + long keys. */
    p = csonpath_new(path_dot);
    assert(p);
    ret = csonpath_find_first(p, root);
    assert(ret);
    assert(!strcmp(json_object_get_string(ret), "deep value"));
    csonpath_destroy(p);

    /* Bracket access on deep + long keys. */
    p = csonpath_new(path_bracket);
    assert(p);
    ret = csonpath_find_first(p, root);
    assert(ret);
    assert(!strcmp(json_object_get_string(ret), "deep value"));
    csonpath_destroy(p);

    /* Recursive descent on the leaf key. */
    p = csonpath_new("$..leaf");
    assert(p);
    ret = csonpath_find_all(p, root);
    assert(ret);
    assert(json_object_array_length(ret) == 1);
    assert(!strcmp(json_object_get_string(json_object_array_get_idx(ret, 0)), "deep value"));
    json_object_put(ret);
    csonpath_destroy(p);

    /* Array of objects, accessed by wildcard + short keys. */
    arr = json_object_new_array();
    for (i = 0; i < 3; ++i) {
        struct json_object *obj = json_object_new_object();
        struct json_object *inner = json_object_new_object();
        json_object_object_add(inner, "kk", json_object_new_string("array value"));
        json_object_object_add(obj, "k", inner);
        json_object_array_add(arr, obj);
    }
    json_object_object_add(cur, "arr", arr);

    p = csonpath_new("$..arr[*].k.kk");
    assert(p);
    ret = csonpath_find_all(p, root);
    assert(ret);
    assert(json_object_array_length(ret) == 3);
    json_object_put(ret);
    csonpath_destroy(p);

    json_object_put(root);
    return 0;
}
