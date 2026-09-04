# csonpath

> That's not my path, that's not your path, but **csonpath**.

[![Project Sandbox](https://docs.outscale.com/fr/userguide/_images/Project-Sandbox-yellow.svg)](https://docs.outscale.com/en/userguide/Open-Source-Projects.html)

**csonpath** is a partial [JSONPath](https://goessner.net/articles/JsonPath/) implementation in C, with Python bindings. It allows you to query, update, and remove data from JSON objects using path expressions.

Unlike many JSONPath libraries, csonpath is **backend-agnostic**: it can work with any C library or environment that manipulates array, object, and scalar types—not just JSON. Out of the box, backends for [json-c](https://github.com/json-c/json-c), [yyjson](https://github.com/ibireme/yyjson), Python and Rust (via `serde_json`) are provided.

---

## 🚀 Features

### JSONPath Syntax

| Feature | Example | Description |
|---|---|---|
| Dot notation | `$.a.b` | Access nested object fields |
| Bracket notation | `$['a']['b']` | Alternative object/array access |
| Array index | `$.array[0]` | Access by zero-based index |
| Wildcard `[*]` | `$.array[*].field` | Iterate all array elements |
| Recursive descent `..` | `$..name` | Search recursively for a key |
| Union `,` (inside brackets) | `$['a','b']`, `$.array[0,1]`, `$.items[?n==1, ?n==2]` | Match all listed selectors at once |
| OR fallback `\|` | `$.a \| $.b` | Try the left path first; fall back to the right one if it does not match. Only the first successful path is used. |
| Filters | `$.items[?price > 10]` | Filter array elements |
| Regex filters | `$.items[?name =~ "foo"]` | POSIX regex-based filtering |
| Multiple filters (`&`) | `$.items[?a=1 & b=2]` | Combine conditions |
| Subpath expressions | `$.obj[$.key]` | Use JSON values as dynamic path keys |
| `@` current object | `$.items[?@.price > 10]` | Reference the current element in filters |

### Operations
- **Find First** — retrieve the first match.
- **Find All** — retrieve all matches (returns an array).
- **Update or Create** — modify existing values or create missing ones.
- **Remove** — delete matching elements.
- **Callback** — execute a custom callback on each match.
- **Update or Create Callback** — traverse the path, creating missing intermediate objects and arrays, then invoke the callback on each leaf node.

---

## 🌐 Links

- Project website: https://github.com/outscale/csonpath
- Join our community on [Discord](https://discord.gg/HUVtY5gT6s)

---

## 📄 Table of Contents

- [Features](#-features)
- [Installation](#-installation)
- [Usage](#-usage)
- [C API Reference](#-c-api-reference)
- [Python API Reference](#-python-api-reference)
- [CLI](#%EF%B8%8F-cli)
- [Custom Backends](#-custom-backends)
- [Running Tests](#-running-tests)
- [Directory Structure](#-directory-structure)
- [Contributing](#-contributing)
- [License](#-license)

---

## 📦 Installation

### Prerequisites
- C compiler (`gcc` or `clang`)
- [json-c](https://github.com/json-c/json-c) library (for C usage)
- Python 3.x (for Python bindings)

### C (json-c)
Just include the header in your project. There is no separate install step required:
```c
#include "csonpath_json-c.h"
```
Make sure to link against `json-c` when compiling:
```sh
gcc myapp.c -o myapp $(pkg-config --cflags --libs json-c)
```

### C (Rust backend)
The Rust backend is located in `rust/`. It builds both a safe Rust API and the C glue required by the csonpath core.

```sh
cd rust
cargo build
cargo test
```

To use it from C, include the backend header and link against the produced static library:
```c
#include "rust/csonpath_rust_backend.h"
```

### Python
Install from PyPI:
```sh
pip install csonpath
```

To install from source (development):
```sh
pip install .
# or
make pip-dev
```

---

## 🛠️ Usage

### C (json-c)

```c
#include "csonpath_json-c.h"

static void my_cb(json_object *parent, struct csonpath_child_info *info,
                  json_object *current, void *ud)
{
    json_object_set_string(current, "modified");
}

int main(void)
{
    struct json_object *jobj = json_tokener_parse(json_str);
    struct csonpath *p = csonpath_new("$.a");

    /* Find First: return the first match, or NULL */
    struct json_object *ret = csonpath_find_first(p, jobj);

    /* Find All: return a NEW json_object array. Caller must free it. */
    ret = csonpath_find_all(p, jobj);
    json_object_put(ret);

    /* Remove: delete matching keys (or set array slots to null). Returns count. */
    int removed = csonpath_remove(p, jobj);

    /* Update or Create: replace matches, or create the full path if missing. */
    csonpath_update_or_create(p, jobj, json_object_new_string("new_value"));

    /* Callback: call a user function for every match. */
    csonpath_callback(p, jobj, my_cb, NULL);

    /* Update or Create Callback: like callback, but creates missing parents first,
       then invokes the callback on every leaf (existing or newly created). */
    csonpath_update_or_create_callback(p, jobj, my_cb, NULL);

    csonpath_destroy(p);
    json_object_put(jobj);
    return 0;
}
```

### Python

```python
import csonpath

data = {"a": "value", "array": [1, 2, 3]}
p = csonpath.CsonPath("$.a")

# Find First / Find All
p.find_first(data)   # -> "value"
p.find_all(data)     # -> ["value"]

# Remove: returns number of removed items
p.set_path("$.array[*]")
p.remove(data)

# Update or Create: builds missing objects/arrays automatically
p.set_path("$.x.y.z")
p.update_or_create(data, [])
# data is now {"a": "value", "array": [1, 2, 3], "x": {"y": {"z": []}}}

# Callback
p.set_path("$.a")
p.callback(data, lambda parent, idx, cur, _: parent.__setitem__(idx, cur.upper()))

# Update or Create Callback: creates parents, then calls cb on each leaf
p.set_path("$[*].a")
p.update_or_create_callback(dst, my_sync_fn, userdata)
```

---

## 📘 C API Reference

```c
struct csonpath *csonpath_new(const char *path);
```
Create and initialize a new csonpath object.

```c
int csonpath_set_path(struct csonpath *p, const char *path);
```
Change the path of an existing object.

```c
int csonpath_compile(struct csonpath *p);
```
Compile the path expression. This is optional—paths are compiled automatically on first use—but explicit compilation can help catch syntax errors earlier.

```c
void csonpath_print_instruction(struct csonpath *p);
```
Print the compiled bytecode instructions (useful for debugging).

```c
struct json_object *csonpath_find_first(struct csonpath *p, struct json_object *json);
```
Return the first matching value, or `NULL` if none is found.

```c
struct json_object *csonpath_find_all(struct csonpath *p, struct json_object *json);
```
Return a new `json_object` array containing all matches. **Must be freed with `json_object_put()`.**

```c
int csonpath_remove(struct csonpath *p, struct json_object *json);
```
Remove all matching elements. Returns the number of elements removed.

```c
int csonpath_update_or_create(struct csonpath *p, struct json_object *json, struct json_object *new_val);
```
Replace matching values with `new_val`, or create the path if it does not exist.

```c
int csonpath_callback(struct csonpath *p, struct json_object *json,
                      json_c_callback callback, void *userdata);
```
Invoke `callback` for every match.

```c
int csonpath_update_or_create_callback(struct csonpath *p, struct json_object *json,
                                       json_c_callback callback, void *userdata);
```
Like `callback`, but traverses the path while updating/creating missing intermediate objects.

```c
void csonpath_destroy(struct csonpath *p);
```
Free the csonpath object.

---

## 📗 Python API Reference

- **`CsonPath(path, return_empty_array=False, jq_like=False)`** — Create a new csonpath object. Optional flags: `return_empty_array` returns `[]` instead of `None` when `find_all()` finds nothing; `jq_like` allows jq-style paths without a leading `$`.
- **`set_path(path)`** — Change the path expression.
- **`find_first(json)`** — Return the first match, or `None`.
- **`find_all(json)`** — Return a list of all matches, or `None` (or `[]` if configured).
- **`remove(json)`** — Remove all matches. Returns the number of removed items.
- **`update_or_create(json, value)`** — Replace matches with `value`, or create the path.
- **`callback(json, callback, callback_data=None)`** — Call `callback(parent, idx, current, callback_data)` for every match.
- **`update_or_create_callback(json, callback, callback_data=None)`** — Same as `callback`, but creates missing parent objects along the path.

---

## 🖥️ CLI

A standalone C CLI is available in `cli/csonpath_cli.c`. It links directly against `json-c` and the csonpath C core, so it works without Python.

```sh
make csonpath          # build ./csonpath
make tests-cli         # run shell tests
```

It reads JSON from stdin, a file (`-f`), or a string (`-s`) and exposes the library operations through action flags.

```sh
# Get the first match (default)
echo '{"a": "value", "array": [1, 2, 3]}' | ./csonpath '$.a'
# => "value"

# Find all matches
echo '{"items": [{"price": 5}, {"price": 15}]}' | ./csonpath -a '$.items[?price > 10]'
# => [{"price":15}]

# One match per line
echo '{"array": [1, 2, 3]}' | ./csonpath -a -o lines '$.array[*]'
# => 1
# => 2
# => 3

# Set or create a value
echo '{"a": 1}' | ./csonpath --set '42' '$.x.y.z'
# => {"a":1,"x":{"y":{"z":42}}}

# Or with a positional value
echo '{"a": 1}' | ./csonpath '$.x.y.z' '42'

# Remove matches
echo '{"a": 1, "b": 2}' | ./csonpath -d '$.b'
# => {"a":1}

# Edit a file in place
./csonpath -f data.json -p -i --set '"2.0"' '$.version'
```

### Options

| Option | Description |
|---|---|
| `-a`, `--all` | Return all matches instead of the first one. |
| `-d`, `--delete` | Remove matches and print the modified JSON. |
| `--set VALUE` | Set `PATH` to `VALUE` (JSON). |
| `VALUE` (positional) | Alternative to `--set`. |
| `-r`, `--raw` | Treat the value as a raw string. |
| `-i`, `--in-place` | Edit `FILE` in place (requires `--file`). |
| `--strict` | Exit with an error if `--delete` removes nothing. |
| `-f FILE`, `--file FILE` | Read JSON from `FILE` instead of stdin. |
| `-s JSON`, `--string JSON` | Read JSON from a string. |
| `-j`, `--jq-like` | Allow jq-style paths without a leading `$`. |
| `-p`, `--pretty` | Pretty-print JSON output. |
| `-o {json,pretty,raw,lines}` | Output format (default: `json`). |
| `-e`, `--empty-array` | Return `[]` instead of nothing when `-a` matches nothing. |

### Exit codes

| Code | Meaning |
|---|---|
| `0` | Success. |
| `1` | No match found, or `--strict` delete found nothing. |
| `EINVAL` | Usage error, JSON parse error, JSONPath compilation error, or invalid JSON value. |
| `errno` | I/O error (e.g. `ENOENT`, `EACCES`). |

---

## 🔌 Custom Backends

csonpath is designed to be backend-agnostic. It can work with any data structure that supports array, object, and scalar semantics.

To create a custom backend, define the required macros and types in a header file (similar to `csonpath_json-c.h` or `csonpath_python.c`), then include your backend header before `csonpath.h`. This allows you to adapt csonpath for manipulating data in any format that supports array/object semantics, giving you full flexibility beyond just JSON.

For more details, see the existing backend implementations:
- `csonpath_json-c.h` — json-c backend
- `csonpath_yyjson.h` — yyjson backend
- `csonpath_python.c` — Python backend
- `rust/csonpath_rust_backend.h` — Rust / serde_json backend

---

## 🧪 Running Tests

### C Tests
```sh
make tests-c
```

### Rust Tests
```sh
cd rust
cargo test
```

### Python Tests
```sh
make tests-py
```

### All Tests
```sh
make tests
```

---

## 📁 Directory Structure

| File / Directory | Description |
|---|---|
| `csonpath.h`, `csonpath_do.h` | Core implementation (header-only style) |
| `csonpath_json-c.h` | json-c backend |
| `csonpath_yyjson.h` | yyjson backend |
| `csonpath_python.c` | Python C extension backend |
| `rust/` | Rust backend (safe API, FFI, C glue) |
| `csonpath_my_fuzzing.h` | Fuzzer helpers (C) |
| `tests/` | C and Python test suites |
| `bench/` | Performance benchmarks |

---

## 🤝 Contributing

We welcome contributions!

Please read our [Contributing Guidelines](CONTRIBUTING.md) and [Code of Conduct](CODE_OF_CONDUCT.md) before submitting a pull request.

Feel free to open issues or pull requests!

---

## 📜 License

BSD 3-Clause. See [LICENSE](LICENSE).
