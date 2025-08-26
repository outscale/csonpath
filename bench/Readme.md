Still WIP

# usage
```sh
make bench
```

# dependancies

Each benchmark have it's own dependancies.
you need npm for nodejs
jsonpath-ng for python
cargo for rust


# bench result (26/08/2025) on macos

```
python bench-jsonpath-ng.py
jsonpath-ng Query: $.store.book[*].title
jsonpath-ng Results: 5000, Time: 8.190528 seconds

csonpath python Query: $.store.book[*].title
csonpath python Results: 5000, Time: 0.198578 seconds

jsonpath-ng Query: $..title
jsonpath-ng Results: 5000, Time: 30.959582 seconds

csonpath python Query: $..title
csonpath python Results: 5000, Time: 0.149349 seconds

node bench-jsonpath.js
Query: $.store.book[*].title
Results: 5000, Time: 0.881951 seconds

Query: $..title
Results: 5000, Time: 1.605545 seconds

cargo run --release
Query: $.store.book[*].title
Results: 5000, Time: 1.18956625s

Query: $..title
Results: 5000, Time: 3.42593s

./bench-jsoncons
Query: $.store.book[*].title
Results: 5000, Time: 0.159977 seconds

Query: $..title
Results: 5000, Time: 0.244324 seconds

./bench-json-c-tcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.156014 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.239929 seconds

./bench-json-c-gcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.129898 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.156560 seconds

./bench-json-c-clang
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.464044 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.823912 seconds

./bench-yyjson
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.038708 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.074619 seconds
```

# bench result (19/08/2025) on Arch Linux BTW
clang version 20.1.8
gcc (GCC) 15.1.1 20250729
```
jsonpath-ng Query: $.store.book[*].title
jsonpath-ng Results: 5000, Time: 13.279904 seconds

csonpath python Query: $.store.book[*].title
csonpath python Results: 5000, Time: 0.394685 seconds

jsonpath-ng Query: $..title
jsonpath-ng Results: 5000, Time: 51.131575 seconds

csonpath python Query: $..title
csonpath python Results: 5000, Time: 0.245051 seconds

node bench-jsonpath.js
Query: $.store.book[*].title
Results: 5000, Time: 1.955359 seconds

Query: $..title
Results: 5000, Time: 4.007358 seconds

cargo run --release
    Finished `release` profile [optimized] target(s) in 0.03s
     Running `target/release/jsonpath_benchmark`
Query: $.store.book[*].title
Results: 5000, Time: 1.945419007s

Query: $..title
Results: 5000, Time: 5.851153824s

./bench-jsoncons
Query: $.store.book[*].title
Results: 5000, Time: 0.325564 seconds

Query: $..title
Results: 5000, Time: 0.456741 seconds

./bench-json-c-tcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.493680 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.582695 seconds

./bench-json-c-gcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.421294 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.445293 seconds

./bench-json-c-clang
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.399849 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.425791 seconds
```
