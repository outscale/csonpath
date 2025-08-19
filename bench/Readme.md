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


# bench result (19/08/2025) on macos

```
jsonpath-ng Query: $.store.book[*].title
jsonpath-ng Results: 5000, Time: 8.291868 seconds

csonpath python Query: $.store.book[*].title
csonpath python Results: 5000, Time: 0.195057 seconds

jsonpath-ng Query: $..title
jsonpath-ng Results: 5000, Time: 31.199278 seconds

csonpath python Query: $..title
csonpath python Results: 5000, Time: 0.148505 seconds

node bench-jsonpath.js
Query: $.store.book[*].title
Results: 5000, Time: 0.885409 seconds

Query: $..title
Results: 5000, Time: 1.690542 seconds

cargo run --release
Query: $.store.book[*].title
Results: 5000, Time: 1.193257834s

Query: $..title
Results: 5000, Time: 3.439993458s

./bench-jsoncons
Query: $.store.book[*].title
Results: 5000, Time: 0.159941 seconds

Query: $..title
Results: 5000, Time: 0.261324 seconds

./bench-json-c-tcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.161415 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.237807 seconds

./bench-json-c-clang
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.130240 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.157839 seconds
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
