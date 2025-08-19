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
jsonpath-ng Results: 5000, Time: 8.309326 seconds

csonpath python Query: $.store.book[*].title
csonpath python Results: 5000, Time: 0.296337 seconds

jsonpath-ng Query: $..title
jsonpath-ng Results: 5000, Time: 31.388187 seconds

csonpath python Query: $..title
csonpath python Results: 5000, Time: 0.382689 seconds

node bench-jsonpath.js
Query: $.store.book[*].title
Results: 5000, Time: 0.882948 seconds

Query: $..title
Results: 5000, Time: 1.600658 seconds

cargo run --release
    Finished `release` profile [optimized] target(s) in 0.10s
     Running `target/release/jsonpath_benchmark`
Query: $.store.book[*].title
Results: 5000, Time: 1.210204792s

Query: $..title
Results: 5000, Time: 3.7900430419999998s

./bench-json-c-tcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.489979 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.968341 seconds

./bench-json-c-gcc
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.451743 seconds

recompile Query: $..title
Results: 5000, in 1000 loop, Time: 0.838162 seconds

./bench-json-c-clang
recompile Query: $.store.book[*].title
Results: 5000, in 1000 loop, Time: 0.442238 seconds

recompile Query: $..title
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
