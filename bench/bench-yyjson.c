#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "csonpath_yyjson.h"

static int bench_csv_mode = 0;

double now_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void csv_header(void) {
    if (getenv("BENCH_CSV_HEADER"))
        printf("category,query,results,time\n");
}

static void csv_row(const char *category, const char *query, size_t results, double time) {
    if (!bench_csv_mode) return;
    printf("%s,\"", category);
    for (const char *p = query; *p; ++p) {
        if (*p == '"') putchar('"');
        putchar(*p);
    }
    printf("\",%zu,%.6f\n", results, time);
}

static void csv_total(const char *category, double total) {
    if (bench_csv_mode)
        printf("%s,TOTAL,0,%.6f\n", category, total);
}

static yyjson_mut_val *build_deep_obj(yyjson_mut_doc *doc) {
    yyjson_mut_val *cur = yyjson_mut_int(doc, 42);
    for (char c = 'l'; c >= 'a'; --c) {
        yyjson_mut_val *wrap = yyjson_mut_obj(doc);
        char key[2] = {c, '\0'};
        yyjson_mut_obj_add(wrap, yyjson_mut_strcpy(doc, key), cur);
        cur = wrap;
    }
    return cur;
}

static yyjson_mut_val *build_deep_mixed(yyjson_mut_doc *doc) {
    yyjson_mut_val *cur = yyjson_mut_int(doc, 42);
    for (char c = 'l'; c >= 'a'; --c) {
        if ((c - 'a' + 1) % 3 == 0 && c != 'l') {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            yyjson_mut_arr_add_val(arr, cur);
            cur = arr;
        }
        yyjson_mut_val *wrap = yyjson_mut_obj(doc);
        char key[2] = {c, '\0'};
        yyjson_mut_obj_add(wrap, yyjson_mut_strcpy(doc, key), cur);
        cur = wrap;
    }
    return cur;
}

static yyjson_doc *build_data(void) {
    yyjson_mut_doc *mdoc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(mdoc);
    yyjson_mut_val *store = yyjson_mut_obj(mdoc);
    yyjson_mut_val *books = yyjson_mut_arr(mdoc);
    yyjson_mut_val *deep_obj_proto = build_deep_obj(mdoc);
    yyjson_mut_val *deep_mixed_proto = build_deep_mixed(mdoc);

    for (int i = 0; i < 5000; i++) {
        yyjson_mut_val *book = yyjson_mut_obj(mdoc);
        yyjson_mut_obj_add(book, yyjson_mut_strcpy(mdoc, "category"), yyjson_mut_strcpy(mdoc, "fiction"));
        char title[32];
        snprintf(title, sizeof(title), "Book %d", i);
        yyjson_mut_obj_add(book, yyjson_mut_strcpy(mdoc, "title"), yyjson_mut_strcpy(mdoc, title));
        yyjson_mut_obj_add(book, yyjson_mut_strcpy(mdoc, "price"), yyjson_mut_int(mdoc, (i % 50) + 5));
        yyjson_mut_val *deep_obj = yyjson_mut_val_mut_copy(mdoc, deep_obj_proto);
        yyjson_mut_val *deep_mixed = yyjson_mut_val_mut_copy(mdoc, deep_mixed_proto);
        yyjson_mut_obj_add(book, yyjson_mut_strcpy(mdoc, "deep_obj"), deep_obj);
        yyjson_mut_obj_add(book, yyjson_mut_strcpy(mdoc, "deep_mixed"), deep_mixed);
        yyjson_mut_arr_add_val(books, book);
    }

    yyjson_mut_val *bicycle = yyjson_mut_obj(mdoc);
    yyjson_mut_obj_add(bicycle, yyjson_mut_strcpy(mdoc, "color"), yyjson_mut_strcpy(mdoc, "red"));
    yyjson_mut_obj_add(bicycle, yyjson_mut_strcpy(mdoc, "price"), yyjson_mut_real(mdoc, 19.95));

    yyjson_mut_obj_add(store, yyjson_mut_strcpy(mdoc, "book"), books);
    yyjson_mut_obj_add(store, yyjson_mut_strcpy(mdoc, "bicycle"), bicycle);
    yyjson_mut_obj_add(root, yyjson_mut_strcpy(mdoc, "store"), store);
    yyjson_mut_doc_set_root(mdoc, root);

    yyjson_doc *jdoc = yyjson_mut_doc_imut_copy(mdoc, NULL);
    yyjson_mut_doc_free(mdoc);
    return jdoc;
}

int main() {
    bench_csv_mode = getenv("BENCH_CSV") != NULL;

    struct yyjson_doc *jdoc = build_data();
    yyjson_val *jobj = yyjson_doc_get_root(jdoc);

    const char *queries[] = {
	"$.store.book[?(@.price) > 20].title",
	"$.store.book[*].title",
        "$.store.book[*]['title','category']",
	"$.store.book[?title =~ /Book/].title",
	"$..title",
        "$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l",
        "$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l",
    };
    size_t query_count = sizeof(queries) / sizeof(queries[0]);
    struct csonpath *p;
    size_t count = 0;
    double total = 0.0;
    const int iters = 250;
    const double scale = 4.0;

    csv_header();

    for (size_t i = 0; i < query_count; i++) {
        double start = now_seconds();

	p = csonpath_new(queries[i]);
	for (int j = 0; j < iters; ++j) {
		struct find_all_ret *ret = csonpath_find_all(p, jobj);
		count = ret ? ret->i : 0;
		free_find_all(ret);
	}

        double elapsed = now_seconds() - start;
        total += elapsed * scale;
        if (bench_csv_mode) {
            csv_row("yyjson", queries[i], count, elapsed * scale);
        } else {
            printf("recompile Query: %s\n", queries[i]);
            printf("Results: %zu, in %d loop, Time: %.6f seconds\n\n", count, iters, elapsed);
        }

    }

    csv_total("yyjson", total);

    csonpath_destroy(p);
    yyjson_doc_free(jdoc);
    return 0;
}
