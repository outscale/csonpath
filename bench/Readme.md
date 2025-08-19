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

# bench result (14/08/2025) on Arch Linux BTW
clang version 20.1.8
gcc (GCC) 15.1.1 20250729
```
python bench-jsonpath-ng.py
jsonpath-ng Query: $.store.book[*].title
jsonpath-ng Results: 5000, Time: 13.322457 seconds

csonpath python Query: $.store.book[*].title
csonpath python Results: 5000, Time: 2.556125 seconds

jsonpath-ng Query: $..title
jsonpath-ng Results: 5000, Time: 58.038847 seconds

csonpath python Query: $..title
csonpath python Results: 5000, Time: 5.465303 seconds

node bench-jsonpath.js
Query: $.store.book[*].title
Results: 5000, Time: 2.003608 seconds

Query: $..title
Results: 5000, Time: 4.104983 seconds

cargo run --release
    Finished `release` profile [optimized] target(s) in 0.04s
     Running `target/release/jsonpath_benchmark`
Query: $.store.book[*].title
Results: 5000, Time: 1.9315683479999999s

Query: $..title
Results: 5000, Time: 5.805299793s

./bench-json-c-tcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.806636 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 1.507889 seconds

./bench-json-c-gcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.744458 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 1.300269 seconds

./bench-json-c-clang
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.761084 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 1.312443 seconds
```
