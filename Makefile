JSON_C_CFLAGS=$(shell pkg-config --cflags json-c)
JSON_C_LDFLAGS=$(shell pkg-config --libs json-c)

all: test-json-c-get-a test-json-update test-json-filter

.PHONY: all clean tests

CFLAGS=-fsanitize=address -fsanitize=undefined -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -O0 -g

test-json-c-get-a: tests/json-c/get-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	cc tests/json-c/get-a.c $(JSON_C_CFLAGS) $(CFLAGS) -I./ -o test-json-c-get-a  $(JSON_C_LDFLAGS)

test-json-update: tests/json-c/set-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	cc tests/json-c/set-a.c $(JSON_C_CFLAGS) $(CFLAGS) -I./ -o test-json-update  $(JSON_C_LDFLAGS)

test-json-filter: tests/json-c/filter.c csonpath_json-c.h csonpath.h csonpath_do.h
	cc tests/json-c/filter.c $(JSON_C_CFLAGS) $(CFLAGS) -I./ -o test-json-filter  $(JSON_C_LDFLAGS)

tests: test-json-c-get-a test-json-update test-json-filter
	./test-json-c-get-a
	./test-json-update
	./test-json-filter

clean:
	rm -rvf test-json-c-get-a test-json-update test-json-filter

