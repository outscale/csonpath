#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "csonpath_json-c.h"

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

static json_object *build_deep_obj(void) {
    json_object *cur = json_object_new_int(42);
    for (char c = 'l'; c >= 'a'; --c) {
        json_object *wrap = json_object_new_object();
        char key[2] = {c, '\0'};
        json_object_object_add(wrap, key, cur);
        cur = wrap;
    }
    return cur;
}

static json_object *build_deep_mixed(void) {
    json_object *cur = json_object_new_int(42);
    for (char c = 'l'; c >= 'a'; --c) {
        if ((c - 'a' + 1) % 3 == 0 && c != 'l') {
            json_object *arr = json_object_new_array();
            json_object_array_add(arr, cur);
            cur = arr;
        }
        json_object *wrap = json_object_new_object();
        char key[2] = {c, '\0'};
        json_object_object_add(wrap, key, cur);
        cur = wrap;
    }
    return cur;
}

static json_object *build_data(void) {
    json_object *root = json_object_new_object();
    json_object *store = json_object_new_object();
    json_object *books = json_object_new_array();
    json_object *deep_obj_proto = build_deep_obj();
    json_object *deep_mixed_proto = build_deep_mixed();

    for (int i = 0; i < 5000; i++) {
        json_object *book = json_object_new_object();
        json_object_object_add(book, "category", json_object_new_string("fiction"));
        char title[32];
        snprintf(title, sizeof(title), "Book %d", i);
        json_object_object_add(book, "title", json_object_new_string(title));
        json_object_object_add(book, "price", json_object_new_int((i % 50) + 5));
        json_object_object_add(book, "deep_obj", json_object_get(deep_obj_proto));
        json_object_object_add(book, "deep_mixed", json_object_get(deep_mixed_proto));
        json_object_array_add(books, book);
    }

    json_object *bicycle = json_object_new_object();
    json_object_object_add(bicycle, "color", json_object_new_string("red"));
    json_object_object_add(bicycle, "price", json_object_new_double(19.95));

    json_object_object_add(store, "book", books);
    json_object_object_add(store, "bicycle", bicycle);
    json_object_object_add(root, "store", store);

    json_object_put(deep_obj_proto);
    json_object_put(deep_mixed_proto);
    return root;
}

int main_(const char **queries, int query_count, const char *category) {
    json_object *jobj = build_data();

    csv_header();

    struct csonpath *p;
    size_t count = 0;
    double total = 0.0;
    const int iters = 250;
    const double scale = 4.0;

    for (size_t i = 0; i < query_count; i++) {
        double start = now_seconds();

	p = csonpath_new(queries[i]);
	for (int j = 0; j < iters; ++j) {
		struct json_object *ret = csonpath_find_all(p, jobj);
		count = ret ? json_object_array_length(ret) : 0;
		json_object_put(ret);
	}

        double elapsed = now_seconds() - start;
        total += elapsed * scale;
        if (bench_csv_mode) {
            csv_row(category, queries[i], count, elapsed * scale);
        } else {
            printf("recompile Query: %s\n", queries[i]);
            printf("Results: %zu, in %d loop, Time: %.6f seconds\n\n", count, iters, elapsed);
        }

    }

    csv_total(category, total);

    json_object_put(jobj);
    csonpath_destroy(p);
    return 0;
}

int main(int ac, char **av) {
    bench_csv_mode = getenv("BENCH_CSV") != NULL;

    const char *compiler = "";
    if (strstr(av[0], "gcc")) compiler = "-gcc";
    else if (strstr(av[0], "clang")) compiler = "-clang";
    else if (strstr(av[0], "tcc")) compiler = "-tcc";

    char category[32];
    snprintf(category, sizeof(category), "json-c%s%s", ac > 1 ? "-regex" : "", compiler);

    const char *queries[] = {
	"$.store.book[?(@.price) > 20].title",
        "$.store.book[*].title",
        "$.store.book[*]['title','category']",
	"$.store.book[?title =~ \"Book\"].title",
        "$..title",
        "$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l",
        "$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l",
    };
    size_t query_count = sizeof(queries) / sizeof(queries[0]);
    const char *queries_reg[] = {
	"$.store.book[?title =~ \"Book\"].title",
	"$.store.book[?title =~ \"Bo*[a-z]\"].title",
	"$.store.book[?title =~ \"Bo*[a-z] \\d*\"].title",
    };
    size_t query_reg_count = sizeof(queries_reg) / sizeof(queries_reg[0]);
    if (ac > 1)
      return main_(queries_reg, query_reg_count, category);
    return main_(queries, query_count, category);
}
