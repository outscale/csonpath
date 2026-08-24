use std::time::Instant;
use csonpath::CsonPath;
use serde_json::json;
use serde_json::Value;

fn rec_get_title(val: &Value, out: &mut Value) {
    match val {
        Value::Object(map) => {
            for (k, v) in map {
                if k == "title" {
                    if let Value::Array(arr) = out {
                        arr.push(v.clone());
                    }
                }
                rec_get_title(v, out);
            }
        }
        Value::Array(inner) => {
            for v in inner {
                rec_get_title(v, out);
            }
        }
        _ => {}
    }
}

fn build_book(i: usize) -> Value {
    json!({
        "category": "fiction",
        "title": format!("Book {}", i),
        "price": (i % 50) + 5,
        "deep_obj": {
            "a": {
                "b": {
                    "c": {
                        "d": {
                            "e": {
                                "f": {
                                    "g": {
                                        "h": {
                                            "i": {
                                                "j": {
                                                    "k": {
                                                        "l": 42
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        },
        "deep_mixed": {
            "a": {
                "b": {
                    "c": [
                        {
                            "d": {
                                "e": {
                                    "f": [
                                        {
                                            "g": {
                                                "h": {
                                                    "i": [
                                                        {
                                                            "j": {
                                                                "k": {
                                                                    "l": 42
                                                                }
                                                            }
                                                        }
                                                    ]
                                                }
                                            }
                                        }
                                    ]
                                }
                            }
                        }
                    ]
                }
            }
        }
    })
}

fn bench_csonpath(cp: &CsonPath, data: &Value, iters: usize) -> (Value, f64) {
    let t0 = Instant::now();
    let mut result = Value::Null;
    for _ in 0..iters {
        result = cp.find_all(data).unwrap();
    }
    (result, t0.elapsed().as_secs_f64())
}

fn bench_pure_rust(data: &Value, label: &str, iters: usize) -> (Value, f64) {
    let t0 = Instant::now();
    let mut result = Value::Null;
    for _ in 0..iters {
        result = match label {
            "all_titles" => {
                data["store"]["book"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|b| b["title"].clone())
                    .collect::<Value>()
            }
            "filter_gt_20" => {
                data["store"]["book"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .filter(|b| b["price"].as_i64().unwrap_or(0) > 20)
                    .map(|b| b["title"].clone())
                    .collect::<Value>()
            }
            "recursive_title" => {
                let mut out = json!([]);
                rec_get_title(&data, &mut out);
                out
            }
            "deep_obj" => {
                data["store"]["book"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|book| {
                        book["deep_obj"]["a"]["b"]["c"]["d"]["e"]["f"]["g"]["h"]["i"]["j"]["k"]["l"].clone()
                    })
                    .collect::<Value>()
            }
            "deep_mixed" => {
                data["store"]["book"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|book| {
                        book["deep_mixed"]["a"]["b"]["c"][0]["d"]["e"]["f"][0]["g"]["h"]["i"][0]["j"]["k"]["l"].clone()
                    })
                    .collect::<Value>()
            }
            _ => Value::Null,
        };
    }
    (result, t0.elapsed().as_secs_f64())
}

fn result_len(r: &Value) -> usize {
    r.as_array().map(|a| a.len()).unwrap_or(0)
}

fn main() {
    let data = json!({"a": "hello"});
    let cp = CsonPath::new("$.a").unwrap();
    let r = cp.find_first(&data).unwrap();
    assert_eq!(r, Some(json!("hello")));

    let n = 5000;

    let books: Value = (0..n)
        .map(build_book)
        .collect::<Value>();

    let data = json!({
        "store": {
            "book": books,
            "bicycle": { "color": "red", "price": 19.95 }
        }
    });

    let queries = vec![
        ("$.store.book[*].title", "all_titles"),
        ("$.store.book[?(@.price > 20)].title", "filter_gt_20"),
        ("$..title", "recursive_title"),
        ("$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l", "deep_obj"),
        ("$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l", "deep_mixed"),
    ];

    let iters = 1000;

    for (query, label) in queries {
        println!("=== {} ===", query);

        let cp = CsonPath::new(query).unwrap();
        let (r, t) = bench_csonpath(&cp, &data, iters);
        println!("csonpath rust : {} results, in {} loop, Time: {} seconds",
                 result_len(&r), iters, t);

        let (r_pure, t_pure) = bench_pure_rust(&data, label, iters);
        println!("pure rust     : {} results, in {} loop, Time: {} seconds",
                 result_len(&r_pure), iters, t_pure);

        let ratio = if t > 0.0 { t_pure / t } else { 0.0 };
        println!("speedup       : {:.2}x\n", ratio);
    }
}
