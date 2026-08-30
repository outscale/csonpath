use std::time::Instant;
use std::env;
use jsonpath_rust::JsonPath;
use serde_json::json;

fn build_book(i: usize) -> serde_json::Value {
    json!({
        "category": "fiction",
        "title": format!("Book {}", i),
        "price": (i % 50) + 5,
        "deep_obj": { "a": { "b": { "c": { "d": { "e": { "f": { "g": { "h": { "i": { "j": { "k": { "l": 42 } } } } } } } } } } } },
        "deep_mixed": { "a": { "b": { "c": [{ "d": { "e": { "f": [{ "g": { "h": { "i": [{ "j": { "k": { "l": 42 } } }] } } }] } } }] } } }
    })
}

fn main() {
    // Create large test dataset
    let books: Vec<_> = (0..5000).map(build_book).collect();

    let data = json!({
        "store": {
            "book": books,
            "bicycle": {
                "color": "red",
                "price": 19.95
            }
        }
    });

    let queries = vec![
        "$.store.book[*].title",
        "$..title",
        "$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l",
        "$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l",
        "$.store.book[?(@.price > 20)].title",
        "$.store.book[*]['title','category']",
    ];

    let csv_mode = env::var("BENCH_CSV").is_ok();
    let csv_header = env::var("BENCH_CSV_HEADER").is_ok();

    if csv_header {
        println!("category,query,results,time");
    }

    let mut total = 0.0;

    for query in queries {
        let (iters, scale): (usize, f64) = if query == "$..title" {
            (50, 20.0)
        } else {
            (100, 10.0)
        };
        let escaped = query.replace("\"", "\"\"");
        let start = Instant::now();
        for _i in 0..iters {
            data.query(query).unwrap();
        }
        let result = data.query(query).unwrap();
        let elapsed = start.elapsed().as_secs_f64() * scale;
        total += elapsed;
        if csv_mode {
            println!("jsonpath-rust,\"{}\",{},{:.6}", escaped, result.len(), elapsed);
        } else {
            println!("Query: {}", query);
            println!("Results: {}, Time: {:?}s\n", result.len(), elapsed);
        }
    }

    if csv_mode {
        println!("jsonpath-rust,TOTAL,0,{:.6}", total);
    }
}
