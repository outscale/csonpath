use std::time::Instant;
use jsonpath_rust::JsonPath;
use serde_json::json;

fn main() {
    // Create large test dataset
    let mut books = vec![];
    for i in 0..5000 {
        books.push(json!({
            "category": "fiction",
            "title": format!("Book {}", i),
            "price": (i % 50) + 5
        }));
    }

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
    ];

    for query in queries {
        let start = Instant::now();
        for _i in 0..99 {
            data.query(query).unwrap();
        }
        let result = data.query(query).unwrap();
        let elapsed = start.elapsed().as_secs_f64();
        println!("Query: {}", query);
        println!("Results: {}, Time: {:?}s\n", result.len(), elapsed);
    }
}
