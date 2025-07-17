JSON_C_CFLAGS=$(shell pkg-config --cflags json-c)
JSON_C_LDFLAGS=$(shell pkg-config --libs json-c)

test-json-c-get-a: tests/json-c/get-a.c csonpath_json-c.h csonpath.h csonpath_do.h
	cc tests/json-c/get-a.c $(JSON_C_CFLAGS) $(JSON_C_LDFLAGS) -O0 -g -I./ -o test-json-c-get-a
