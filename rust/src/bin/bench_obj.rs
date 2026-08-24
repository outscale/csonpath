use std::time::Instant;
use csonpath::CsonPath;
use serde_json::{json, Value};

fn main() {
    let n = 5000;
    let mut obj = serde_json::Map::new();
    for i in 0..n {
        obj.insert(format!("key_{:04}", i), json!(i));
    }
    let data = Value::Object(obj);

    let iters = 100;

    // find_all sur objet
    {
        let cp = CsonPath::new("$.*").unwrap();
        let t0 = Instant::now();
        for _ in 0..iters {
            let r = cp.find_all(&data).unwrap();
            assert_eq!(r.as_array().map(|a| a.len()), Some(n));
        }
        println!("find_all $.* on {} keys: {} iters in {:.4}s",
                 n, iters, t0.elapsed().as_secs_f64());
    }

    // remove sur objet, un seul appel qui supprime tout
    {
        let cp = CsonPath::new("$.*").unwrap();
        let t0 = Instant::now();
        for _ in 0..iters {
            let mut copy = data.clone();
            let count = cp.remove(&mut copy).unwrap();
            assert_eq!(count, n);
        }
        println!("remove $.* on {} keys: {} iters in {:.4}s",
                 n, iters, t0.elapsed().as_secs_f64());
    }
}
