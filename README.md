# csonpath

> that's not my path, that's not your path, but csonpath

[![Project Sandbox](https://docs.outscale.com/fr/userguide/_images/Project-Sandbox-yellow.svg)](https://docs.outscale.com/en/userguide/Open-Source-Projects.html)

**csonpath** is a partial [JSONPath](https://goessner.net/articles/JsonPath/) implementation in C, with Python bindings. It allows you to query, update, and remove data from JSON objects using path expressions.

## Features

- **Find First**: Retrieve the first value matching a path.
- **Find All**: Retrieve all values matching a path.
- **Update or Create**: Update values or create new ones according to the path.
- **Remove**: Remove values matching a path.
- **Supports both C (using json-c) and Python objects.**

## Getting Started

### Prerequisites

- C compiler (gcc/clang)
- [json-c](https://github.com/json-c/json-c) library for C usage
- Python 3.x (for Python bindings)

### Build

To build the C tests and examples:

```sh
make
```

### Usage (json-c)

See examples in [`tests/json-c/get-a.c`](tests/json-c/get-a.c):

```c
#include "csonpath_json-c.h"

const char *json_str = "{ \"a\": \"value\", \"array\": [1,2,3] }";
struct csonpath p;
struct json_object *jobj = json_tokener_parse(json_str);
struct json_object *ret;

csonpath_init(&p, "$.a");
ret = csonpath_find_first(&p, jobj);
// ret now points to "value"
```

### Usage (Python)

Check [`dump-test.py`](dump-test.py) for examples:

```python
import csonpath

d = { "a": "value", "array": [1,2,3] }
o = csonpath.CsonPath("$.a")
print(o.find_first(d))  # Output: "value"
```

### Backends and Direct Usage

csonpath is inspired by JSONPath, but is designed to be backend-agnostic: it can work with any C library or environment that manipulates array, object, and scalar types—not just JSON. Out of the box, backends for Python and json-c are provided, but you can create your own backend for other formats such as YAML, or any custom data structure.

To use csonpath with a different data type or library, simply define the required macros and types in a header file (similar to `csonpath_json-c.h` or `csonpath_python.c`), then include your backend header before `csonpath.h`. This approach allows you to adapt csonpath for manipulating data in any format that supports array/object semantics, giving you full flexibility beyond just JSON.

## Directory Structure

- `csonpath.h`, `csonpath_do.h` - Main implementation files
- `csonpath_json-c.h` - Json-C implementation, use it by including "csonpath_json-c.h" directly in your source
- `csonpath_python.c` - python implementation, use it like a python lib (so `pip install .`)
- `tests/` - Contains C tests using json-c
- `dump-test.py` - Python usage examples

## Contributing

Feel free to open issues or pull requests!

## License
BSD 3-Clause. See [LICENSE](LICENSE).
