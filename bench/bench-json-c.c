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
    };
    size_t query_count = sizeof(queries) / sizeof(queries[0]);
    struct csonpath p;
    size_t count = 0;

    for (size_t i = 0; i < query_count; i++) {
        double start = now_seconds();

        // TODO: call your JSONPath library here
        // Example (pseudo-code):
	csonpath_init(&p, queries[i]);
	for (int j = 0; j < 100; ++j) {
		struct json_object *ret = csonpath_find_all(&p, jobj);
		count = json_object_array_length(ret);
		json_object_put(ret);
	}
        // size_t count = result_list_size(res);

        // For now, we’ll fake a result count for demonstration:

        double elapsed = now_seconds() - start;
        printf("Query: %s\n", queries[i]);
        printf("Results: %zu, Time: %.6f seconds\n\n", count, elapsed);

        // TODO: free result list if needed
    }

    free(json_text);
    // TODO: free parsed JSON if needed

    return 0;
}
