import json
import time
from jsonpath_ng.ext import parse
import csonpath

# Create a large test dataset
data = {
    "store": {
        "book": [{"category": "fiction", "title": f"Book {i}", "price": i % 50 + 5} for i in range(5000)],
        "bicycle": {"color": "red", "price": 19.95}
    }
}

# JSONPath queries to benchmark
queries = [
    "$.store.book[*].title",
    "$.store.book[?(@.title) =~ \"Book\"].title",
    "$..title",
]

# Benchmark
for query in queries:
    start = time.perf_counter()
    expr = parse(query)
    for x in range (1000):
        result = expr.find(data)
    elapsed = time.perf_counter() - start
    print(f"jsonpath-ng Query: {query}")
    print(f"jsonpath-ng Results: {len(result)}, Time: {elapsed:.6f} seconds\n")

    start = time.perf_counter()
    cp = csonpath.CsonPath(query)
    for x in range (1000):
        result = cp.find_all(data)
    elapsed = time.perf_counter() - start
    print(f"csonpath python Query: {query}")
    print(f"csonpath python Results: {len(result)}, Time: {elapsed:.6f} seconds\n")
