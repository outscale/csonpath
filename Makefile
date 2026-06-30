JSON_C_CFLAGS=$(shell pkg-config --cflags json-c)
JSON_C_LDFLAGS=$(shell pkg-config --libs json-c)
YYJSON_CFLAGS=$(shell pkg-config --cflags yyjson)
YYJSON_LDFLAGS=$(shell pkg-config --libs yyjson)

include config.mk

all: test-json-c-get-a test-json-update test-json-filter test-json-subpath test-json-c-array-root test-json-filter-and-missing-key test-json-get-array-big-index test-json-union test-json-audit-bugs test-json-crash-vectors

YYJSON_TESTS=test-yyjson

bench:
	make -C bench

bench-clean:
	make -C bench clean

.PHONY: all clean tests pip-dev pip-dev bench bench-clean tests-cli

CFLAGS+= -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -O0 -g

test-json-c-get-a: tests/json-c/get-a.c $(EXTRA_FILES) csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/get-a.c  $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-c-get-a  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-c-array-root: tests/json-c/root-array.c $(EXTRA_FILES) csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/root-array.c  $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-c-array-root  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-update: tests/json-c/set-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/set-a.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-update  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-filter: tests/json-c/filter.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/filter.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-filter  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-subpath: tests/json-c/subpath.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/subpath.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-subpath  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-filter-and-missing-key: tests/json-c/filter-and-missing-key.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/filter-and-missing-key.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-filter-and-missing-key  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-get-array-big-index: tests/json-c/get-array-big-index.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/get-array-big-index.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-get-array-big-index  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-union: tests/json-c/union.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/union.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-union  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-audit-bugs: tests/json-c/audit-bugs.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/audit-bugs.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-audit-bugs  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-crash-vectors: tests/json-c/crash-vectors.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/crash-vectors.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-crash-vectors  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-yyjson: tests/yyjson/test-yyjson.c csonpath_yyjson.h csonpath.h csonpath_do.h
	$(CC) tests/yyjson/test-yyjson.c $(EXTRA_FILES) $(YYJSON_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-yyjson $(YYJSON_LDFLAGS) $(LDFLAGS)

 tests-c: test-json-c-get-a test-json-update test-json-filter test-json-subpath test-json-c-array-root test-json-filter-and-missing-key test-json-get-array-big-index test-json-union test-json-audit-bugs test-json-crash-vectors test-yyjson
	./test-json-c-get-a
	./test-json-update
	./test-json-filter
	./test-json-subpath
	./test-json-c-array-root
	./test-json-filter-and-missing-key
	./test-json-get-array-big-index
	./test-json-union
	./test-json-audit-bugs
	./test-json-crash-vectors
	./test-yyjson

csonpath: cli/csonpath_cli.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) cli/csonpath_cli.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -I./ -o csonpath $(JSON_C_LDFLAGS) $(LDFLAGS)

tests-cli: csonpath
	bash tests/cli/run.sh

pip-dev:
	pip install -e .[dev] --force-reinstall

tests-py: pip-dev
	python -m pytest

tests: tests-py tests-c

clean:
	rm -rvf test-json-c-get-a test-json-update test-json-filter test-json-subpath test-json-c-array-root test-json-filter-and-missing-key test-json-get-array-big-index test-json-union test-json-audit-bugs test-json-crash-vectors test-yyjson csonpath

