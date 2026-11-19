# Benchmarks

## Usage

```sh
make bench          # build and run every suite
./bench/collect     # build, run and store results/v<version>/{meta.txt,results.csv}
BENCH_ONLY=csonpath ./bench/collect  # csonpath suites only, as CI does
./bench/render      # write results/v<version>/table.md
./bench/check       # check the latest run against the baseline (10 %)
./bench/make-baseline  # write bench/baseline.{csv,meta} from the latest run
```

`aggregate` runs every suite and prints CSV; `pivot` turns those rows into a
comparison table (or CSV with `--csv`). `collect` and `render` just call them.
The Python suites use `python3` from `PATH`, so activate the project virtualenv
first.

`BENCH_ONLY=csonpath` limits `aggregate` to the csonpath suites:
`json-c-{gcc,tcc,clang}`, `yyjson const`, `yyjson mut`, `csonpath-python` and
`csonpath-rust`. The Python and Rust benchmarks then skip their reference
implementations. CI always uses that mode, so the baseline is collected and
checked without filtering the results afterwards.

## Dependencies

Each benchmark has its own dependencies. The csonpath run that CI does needs:

- a Python virtualenv with the local extension:
  ```sh
  python3 -m venv venv
  . venv/bin/activate
  pip install -e .
  ```
- `cargo` for `csonpath-rust` (`rust/Cargo.toml --bin bench`)
- `libjson-c-dev`, `libyyjson-dev`, `tcc`, `clang`, `pkg-config` and `make`
  for the C suites
- `miller` for `bench/render`

The full local run also needs:

- `pip install jsonpath-ng` for the Python reference
- `npm ci --prefix bench` for jsonpath-plus (the lockfile is committed)
- `libjsoncons-dev` and `g++` for the C++ reference
- `cargo build --release --manifest-path bench/Cargo.toml` for jsonpath-rust

## Comparing runs

`bench/check` only compares a run with `bench/baseline.csv` when
`bench/baseline.meta` matches the run's `meta.txt` for OS, CPU, memory,
compilers and library versions. Results from different machines are not
comparable, so a metadata mismatch is an error.

To create or replace the baseline, push the `bench-baseline` branch. It must
point to the commit to measure (the same tree as `main`), so the artifact is
generated with exactly the workflow and the benchmark code that `main` runs.
The reusable `bench` workflow (`.github/workflows/bench.yml`) installs the
packages, builds and runs only the csonpath suites (`BENCH_ONLY=csonpath`), and
uploads `bench/baseline.csv` and `bench/baseline.meta` as an artifact. The
ratios are relative to `json-c-gcc`. Commit those files on `main` before
merging; the branch itself is not merged.

## Results

<!-- bench:start -->
v0.21.0, 2026-11-19, Apple M1, 16 GiB, macOS Darwin 25.6.0 arm64,
Apple clang 17.0.0, rustc 1.85.1, json-c 0.19, yyjson 0.12.0.

Times are in seconds. Rows are sorted by query, columns by total time
(fastest first). Lower is better.

| query | yyjson const | yyjson mut | json-c-gcc | json-c-clang | jsoncons | json-c-tcc | csonpath-python | pure-rust | pure-python | csonpath-rust | jsonpath-plus | jsonpath-rust | jsonpath-ng |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| $..title | 0.889507s | 1.232188s | 1.592109s | 1.600270s | 3.013270s | 3.905455s | 3.101552s | 16.944650s | 23.836152s | 20.622704s | 26.943513s | 106.125927s | 265.847036s |
| $.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l | 0.504687s | 0.531636s | 1.333324s | 1.308789s | 1.547757s | 2.296362s | 3.418387s | 6.009498s | 3.682296s | 6.251309s | 11.352487s | 25.084700s | 185.885449s |
| $.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l | 0.474287s | 0.513127s | 1.079866s | 1.154001s | 1.380706s | 2.236568s | 4.650710s | 7.118896s | 4.762336s | 8.572580s | 9.292324s | 24.067444s | 195.831935s |
| $.store.book[*].title | 0.058054s | 0.059945s | 0.151841s | 0.154894s | 0.189380s | 0.182801s | 0.273502s | 0.713900s | 0.350677s | 0.957579s | 1.824634s | 2.491967s | 17.126497s |
| $.store.book[*]['title','category'] | 0.164315s | 0.177054s | 0.355671s | 0.341316s | 0.442140s | 0.551924s | 0.658637s | 1.263828s | 0.614160s | 1.461142s |  | 3.913863s | 21.853570s |
| $.store.book[?(@.price > 20)].title | 0.118028s | 0.143158s | 0.273027s | 0.277232s | 0.711942s | 0.396383s | 0.549492s | 0.816972s | 0.617292s | 0.869105s | 2.842914s | 3.471420s | 26.648087s |
| $.store.book[?(@.title =~ /Book/)].title | 0.624325s | 0.675731s | 0.860739s | 0.844254s | 0.975418s | 1.059988s | 1.217965s | 1.071755s | 0.981474s | 1.849479s | 3.740393s |  | 31.867933s |
| TOTAL | 2.833203s | 3.332839s | 5.646577s | 5.680756s | 8.260612s | 10.629481s | 13.870245s | 33.939500s | 34.844387s | 40.583897s | 55.996265s | 165.155322s | 745.060506s |
<!-- bench:end -->
