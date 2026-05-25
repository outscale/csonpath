JSON_C_CFLAGS=$(shell pkg-config --cflags json-c)
JSON_C_LDFLAGS=$(shell pkg-config --libs json-c)

include config.mk

all: test-json-c-get-a test-json-update test-json-filter test-json-subpath

bench:
	make -C bench

bench-clean:
	make -C bench clean

.PHONY: all clean tests pip-dev pip-dev bench bench-clean

CFLAGS+= -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -O0 -g

test-json-c-get-a: tests/json-c/get-a.c $(EXTRA_FILES) csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/get-a.c  $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-c-get-a  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-update: tests/json-c/set-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/set-a.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-update  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-filter: tests/json-c/filter.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/filter.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-filter  $(JSON_C_LDFLAGS) $(LDFLAGS)

test-json-subpath: tests/json-c/subpath.c csonpath_json-c.h csonpath.h csonpath_do.h
	$(CC) tests/json-c/subpath.c $(EXTRA_FILES) $(JSON_C_CFLAGS) $(CFLAGS) -Wno-format -I./ -o test-json-subpath  $(JSON_C_LDFLAGS) $(LDFLAGS)

tests-c: test-json-c-get-a test-json-update test-json-filter test-json-subpath
	./test-json-c-get-a
	./test-json-update
	./test-json-filter
	./test-json-subpath

pip-dev:
	pip install -e .[dev] --force-reinstall

tests-py: pip-dev
	python -m pytest

tests: tests-py tests-c

clean:
	rm -rvf test-json-c-get-a test-json-update test-json-filter test-json-subpath

