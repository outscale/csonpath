all: bench-json-c-tcc bench-json-c-gcc bench-json-c-clang bench-jsoncons bench-yyjson
	python bench-jsonpath-ng.py
	node bench-jsonpath.js
	cargo run --release
	./bench-jsoncons
	./bench-json-c-tcc
	./bench-json-c-gcc
	./bench-json-c-clang
	./bench-yyjson

.PHONY: all clean bench_csonpath

bench_csonpath: bench-json-c-tcc bench-json-c-gcc bench-json-c-gcc-pcrepos bench-json-c-gcc-tinyre bench-json-c-gcc-pcre bench-json-c-clang bench-yyjson
	./bench-json-c-tcc
	./bench-json-c-gcc
	./bench-json-c-gcc-pcrepos
	./bench-json-c-gcc-tinyre
	./bench-json-c-gcc-pcre
	./bench-json-c-clang
	./bench-yyjson


bench_regexs: bench-json-c-gcc-pcrepos bench-json-c-gcc-tinyre bench-json-c-gcc-pcre
	./bench-json-c-gcc-pcrepos a
	./bench-json-c-gcc-tinyre b
	./bench-json-c-gcc-pcre c


clean:
	rm -vf bench-json-c-tcc bench-json-c-gcc bench-json-c-clang bench-yyjson

bench-json-c-tcc: bench-json-c.c
	tcc bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-tcc $(shell pkg-config --libs json-c)

bench-json-c-gcc: bench-json-c.c
	gcc bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-gcc $(shell pkg-config --libs json-c)

bench-json-c-gcc-pcrepos: bench-json-c.c
	gcc bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c libpcre2-posix) -DCSONPATH_PCRE2_POSIX -o bench-json-c-gcc-pcrepos  $(shell pkg-config --libs json-c libpcre2-posix)

bench-json-c-gcc-tinyre: bench-json-c.c
	gcc bench-json-c.c ../tiny-regex-c/re.c -O3 -g -DCSONPATH_TINY_REGEX -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-gcc-tinyre $(shell pkg-config --libs json-c)

bench-json-c-gcc-pcre: bench-json-c.c
	gcc bench-json-c.c  -O3 -g -DCSONPATH_PCRE2 -I../ $(shell pkg-config --cflags json-c libpcre2-8) -o bench-json-c-gcc-pcre $(shell pkg-config --libs json-c libpcre2-8)

bench-json-c-clang: bench-json-c.c
	clang bench-json-c.c -O3 -g -I../ $(shell pkg-config --cflags json-c) -o bench-json-c-clang $(shell pkg-config --libs json-c)

bench-yyjson: bench-yyjson.c
	gcc bench-yyjson.c -O3 -g -I../ $(shell pkg-config --cflags yyjson) -o bench-yyjson $(shell pkg-config --libs yyjson)


jsoncons/:
	git clone https://github.com/danielaparker/jsoncons.git

bench-jsoncons: jsoncons/
	 g++ -std=c++17 -O3 bench-jsoncons.cc -I./jsoncons/include -o bench-jsoncons
