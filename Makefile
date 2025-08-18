JSON_C_CFLAGS=$(shell pkg-config --cflags json-c)
JSON_C_LDFLAGS=$(shell pkg-config --libs json-c)

all: test-json-c-get-a test-json-update test-json-filter

bench:
	make -C bench

bench-clean:
	make -C bench clean

.PHONY: all clean tests pip-dev pip-dev bench bench-clean

CFLAGS=-fsanitize=address -fsanitize=undefined -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -O0 -g

test-json-c-get-a: tests/json-c/get-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/get-a.c $(JSON_C_CFLAGS) $(CFLAGS) -I./ -o test-json-c-get-a  $(JSON_C_LDFLAGS)

test-json-update: tests/json-c/set-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/set-a.c $(JSON_C_CFLAGS) $(CFLAGS) -I./ -o test-json-update  $(JSON_C_LDFLAGS)

test-json-filter: tests/json-c/filter.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/filter.c $(JSON_C_CFLAGS) $(CFLAGS) -I./ -o test-json-filter  $(JSON_C_LDFLAGS)

tests-c: test-json-c-get-a test-json-update test-json-filter
	./test-json-c-get-a
	./test-json-update
	./test-json-filter

pip-dev:
	pip install -e .[dev]

tests-py: pip-dev
	python -m pytest

tests: tests-py tests-c

clean:
	rm -rvf test-json-c-get-a test-json-update test-json-filter

