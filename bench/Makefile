all: bench-json-c-tcc bench-json-c-gcc bench-json-c-clang
	python bench-jsonpath-ng.py
	node bench-jsonpath.js
	cargo run --release
	./bench-json-c-tcc
	./bench-json-c-gcc
	./bench-json-c-clang

bench-json-c-tcc:
	tcc bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-tcc $(shell pkg-config --libs json-c)

bench-json-c-gcc:
	gcc bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-gcc $(shell pkg-config --libs json-c)

bench-json-c-clang:
	clang bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-clang $(shell pkg-config --libs json-c)

