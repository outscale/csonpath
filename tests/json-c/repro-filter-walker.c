#include <stdio.h>
#include <json-c/json.h>
#include "csonpath_json-c.h"

/*
 * Reproducer: csonpath_walker_next_inst() uses the static
 * csonpath_instuction_len[] table, which does not describe the variable-size
 * operand emitted behind a filter operator:
 *
 *     [opcode][nb_getter - 1][getter...]
 *
 * Walking "past" a filter (missing path -> CSONPATH_DO_GET_NOTFOUND, or a
 * union -> csonpath_skipp_union_jmp() / GET_UNION) therefore lands inside the
 * operand, then on the Pascal length byte of the filter key: the first key
 * byte is used as a string length.
 *
 * With a 0xFF first key byte, csonpath_str_skip() sees a 2-byte Pascal length
 * and jumps ~32 KiB outside the bytecode. The walker then ramps through the
 * zero-filled heap until the first unmapped page: reading it gives SIGBUS.
 * The ASCII-only variants below hang or crash depending on the heap layout.
 */

static void run(int id, const char *path, struct json_object *root)
{
    fprintf(stderr, "\n[repro] ---------- case %d ----------\n", id);
    fprintf(stderr, "[repro] >>> BEFORE csonpath_new(\"%s\")\n", path);
    fflush(stderr);

    struct csonpath *p = csonpath_new(path);
    if (!p) {
        fprintf(stderr, "[repro] <<< AFTER  case %d: compile error "
                        "(skipped)\n", id);
        fflush(stderr);
        return;
    }

    fprintf(stderr, "[repro] >>> BEFORE csonpath_find_all()\n");
    fflush(stderr);

    struct json_object *r = csonpath_find_all(p, root);

    fprintf(stderr, "[repro] <<< AFTER  case %d: find_all returned %p\n",
            id, (void *)r);
    fflush(stderr);
    if (r)
        json_object_put(r);
    csonpath_destroy(p);
}

int main(void)
{
    fprintf(stderr, "=== repro-filter-walker: start ===\n");
    fflush(stderr);

    struct json_object *arr = json_object_new_array();
    struct json_object *o;

    o = json_object_new_object();
    json_object_object_add(o, "a", json_object_new_int(1));
    json_object_array_add(arr, o);
    o = json_object_new_object();
    json_object_object_add(o, "b", json_object_new_int(2));
    json_object_array_add(arr, o);
    o = json_object_new_object();
    json_object_object_add(o, "\xff", json_object_new_int(3));
    json_object_array_add(arr, o);

    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "x", json_object_new_int(1));

    /* Control: this one completes, so you see BEFORE + AFTER. */
    run(0, "$[?a]", arr);

    /* Deterministic: the 0xFF key byte is read as a Pascal length. */
    run(1, "$.missing[?['\xff']]", obj);   /* ~SIGBUS  */
    run(2, "$[?['\xff'],?b]", arr);        /* ~SIGSEGV */

    /* Same bug with plain ASCII: hang or crash depending on layout. */
    run(3, "$[?a,?b]", arr);
    run(4, "$[?a,?b,?c]", arr);
    run(5, "$[?(@.a),?(@.b)]", arr);
    run(6, "$.missing[?a]", obj);
    run(7, "$.missing[?a,?b]", obj);
    run(8, "$.a[0][?x]", obj);
    run(9, "$.a[0].b[?x]", obj);

    json_object_put(arr);
    json_object_put(obj);
    fprintf(stderr, "=== repro-filter-walker: all cases survived "
                    "(no crash) ===\n");
    return 0;
}
