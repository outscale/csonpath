# csonpath

[JSONPath](https://goessner.net/articles/JsonPath/) implementation for Rust, backed by `serde_json::Value`.

## Installation

```toml
[dependencies]
csonpath = "0.19.1"
serde_json = "1"
```

## Quick start

```rust
use csonpath::CsonPath;
use serde_json::json;

let data = json!({
    "store": {
        "book": [
            {"title": "A", "price": 10},
            {"title": "B", "price": 20}
        ]
    }
});

let p = CsonPath::new("$.store.book[*].title").unwrap();
let titles = p.find_all(&data).unwrap().unwrap();
println!("{}", titles); // ["A", "B"]
```

## Constructors

### Strict

```rust
let p = CsonPath::new("$.a.b").unwrap();
```

Returns an error if the path fails to compile. Best for static, known paths.

### Strict with flags

```rust
use csonpath::CSONPATH_RETURN_EMPTY_ARRAY;

let p = CsonPath::new_with_flags("$.a", CSONPATH_RETURN_EMPTY_ARRAY).unwrap();
```

### Lenient

Use this for **dynamic paths** or when you want to defer error handling to query time:

```rust
let field = "name";
let p = CsonPath::new_lenient(&format!("$.users[*].{}", field));
let names = p.find_all(&data)?; // error happens here if the path is invalid
```

### Lenient with flags

```rust
let p = CsonPath::new_lenient_with_flags("$.a", CSONPATH_RETURN_EMPTY_ARRAY);
```

### Builder

```rust
let p = CsonPath::builder("$.items[*]")
    .auto_root(true)
    .return_empty_array(true)
    .build()
    .unwrap();
```

## Operations

### Find first

```rust
let p = CsonPath::new("$.store.book[0].title").unwrap();
let title = p.find_first(&data).unwrap();
```

### Find all

```rust
let p = CsonPath::new("$.store.book[*].price").unwrap();
let prices = p.find_all(&data).unwrap(); // Some([10, 20]) or None
```

### Remove

```rust
let mut data = json!({"items": [1, 2, 3]});
let p = CsonPath::new("$.items[*]").unwrap();
let removed = p.remove(&mut data).unwrap();
```

### Update or create

```rust
let mut data = json!({});
let p = CsonPath::new("$.a.b").unwrap();
p.update_or_create(&mut data, json!(42)).unwrap();
```

### Callback

```rust
let mut data = json!({"items": [{"v": 1}, {"v": 2}]});
let p = CsonPath::new("$.items[*]").unwrap();
p.callback(&mut data, |ctx| {
    if let Some(key) = ctx.child_info.key() {
        println!("matched key: {}", key);
    }
    Ok(())
}).unwrap();
```

## Supported syntax

- Dot notation: `$.a.b`
- Bracket notation: `$['a']`, `$.array[0]`
- Wildcards: `$.array[*]`, `$..*`
- Recursive descent: `$..name`
- Filters: `$.items[?price > 10]`
- Regex filters: `$.items[?name =~ "foo"]`
- Unions: `$.['a','b']`
- OR fallback: `$.a | $.b`
- Type selectors: `$.*@string()`
- Subpaths: `$.array[$.index]`

## License

BSD-3-Clause
