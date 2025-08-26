all: bench-json-c-tcc bench-json-c-gcc bench-json-c-clang bench-jsoncons bench-yyjson
	python bench-jsonpath-ng.py
	node bench-jsonpath.js
	cargo run --release
	./bench-jsoncons
	./bench-json-c-tcc
	./bench-json-c-gcc
	./bench-json-c-clang
	./bench-yyjson

.PHONY: all clean


clean:
	rm -vf bench-json-c-tcc bench-json-c-gcc bench-json-c-clang

bench-json-c-tcc: bench-json-c.c
	tcc bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-tcc $(shell pkg-config --libs json-c)

bench-json-c-gcc: bench-json-c.c
	gcc bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-gcc $(shell pkg-config --libs json-c)

bench-json-c-clang: bench-json-c.c
	clang bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-clang $(shell pkg-config --libs json-c)

bench-yyjson: bench-yyjson.c
	gcc bench-yyjson.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-yyjson $(shell pkg-config --libs json-c)


jsoncons/:
	git clone https://github.com/danielaparker/jsoncons.git

bench-jsoncons: jsoncons/
	 g++ -std=c++17 -O3 bench-jsoncons.cc -I./jsoncons/include -o bench-jsoncons
