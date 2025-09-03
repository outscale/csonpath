#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "csonpath_json-c.h"

double now_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

// Example: generate large JSON dataset in memory
char *generate_json() {
    // WARNING: This is very naive string building — for real use, use a JSON library!
    // Here we create: { "store": { "book": [ ... ], "bicycle": { ... } } }
    size_t buf_size = 1024 * 1024; // 1 MB to start
    char *json = malloc(buf_size);
    if (!json) { perror("malloc"); exit(1); }

    strcpy(json, "{ \"store\": { \"book\": [");

    for (int i = 0; i < 5000; i++) {
        char entry[128];
        snprintf(entry, sizeof(entry),
                 "{\"category\":\"fiction\",\"title\":\"Book %d\",\"price\":%d}%s",
                 i, (i % 50) + 5, (i < 4999) ? "," : "");
        if (strlen(json) + strlen(entry) + 64 > buf_size) {
            buf_size *= 2;
            json = realloc(json, buf_size);
            if (!json) { perror("realloc"); exit(1); }
        }
        strcat(json, entry);
    }

    strcat(json, "], \"bicycle\": {\"color\": \"red\", \"price\": 19.95} } }");
    return json;
}

int main() {
    char *json_text = generate_json();

    struct json_object *jobj = json_tokener_parse(json_text);

    const char *queries[] = {
        "$.store.book[*].title",
	"$.store.book[?title =~ \"Book\"].title",
        "$..title",
    };
    size_t query_count = sizeof(queries) / sizeof(queries[0]);
    struct csonpath p;
    size_t count = 0;

    for (size_t i = 0; i < query_count; i++) {
        double start = now_seconds();

	csonpath_init(&p, queries[i]);
	for (int j = 0; j < 1000; ++j) {
		struct json_object *ret = csonpath_find_all(&p, jobj);
		count = json_object_array_length(ret);
		json_object_put(ret);
	}

        double elapsed = now_seconds() - start;
        printf("recompile Query: %s\n", queries[i]);
        printf("Results: %zu, in 1000 loop, Time: %.6f seconds\n\n", count, elapsed);

    }

    free(json_text);
    csonpath_destroy(&p);
    return 0;
}
