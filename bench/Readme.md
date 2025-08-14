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


# bench result (14/08/2025) on macos

```
python bench-jsonpath-ng.py
jsonpath-ng Query: $.store.book[*].title
jsonpath-ng Results: 5000, Time: 8.321527 seconds

csonpath python Query: $.store.book[*].title
csonpath python Results: 5000, Time: 0.894530 seconds

jsonpath-ng Query: $..title
jsonpath-ng Results: 5000, Time: 33.312233 seconds

csonpath python Query: $..title
csonpath python Results: 5000, Time: 1.912162 seconds

node bench-jsonpath.js
Query: $.store.book[*].title
Results: 5000, Time: 0.900235 seconds

Query: $..title
Results: 5000, Time: 1.647288 seconds

cargo run --release
    Finished `release` profile [optimized] target(s) in 0.08s
     Running `target/release/jsonpath_benchmark`
Query: $.store.book[*].title
Results: 5000, Time: 1.208829s

Query: $..title
Results: 5000, Time: 3.482235s

./bench-json-c-tcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.489903 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.945940 seconds

./bench-json-c-clang
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.453049 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.829027 seconds
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
