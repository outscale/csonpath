JSON_C_CFLAGS=$(shell pkg-config --cflags json-c)
JSON_C_LDFLAGS=$(shell pkg-config --libs json-c)

all: test-json-c-get-a test-json-update

.PHONY: all clean

test-json-c-get-a: tests/json-c/get-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	cc tests/json-c/get-a.c $(JSON_C_CFLAGS) $(JSON_C_LDFLAGS) -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -O0 -g -I./ -o test-json-c-get-a

test-json-update: tests/json-c/set-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	cc tests/json-c/set-a.c $(JSON_C_CFLAGS) $(JSON_C_LDFLAGS) -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -O0 -g -I./ -o test-json-c-set-a

clean:
	rm -rvf test-json-c-get-a test-json-c-set-a

