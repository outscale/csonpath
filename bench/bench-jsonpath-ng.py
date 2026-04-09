import json
import time
import re
from jsonpath_ng.ext import parse
import csonpath

# Create a large test dataset
data = {
    "store": {
        "book": [
            {"category": "fiction", "title": f"Book {i}", "price": i % 50 + 5}
            for i in range(5000)
        ],
        "bicycle": {"color": "red", "price": 19.95},
    }
}

# JSONPath queries to benchmark
queries = [
    "$.store.book[?(@.price) > 20].title",
    "$.store.book[*].title",
    '$.store.book[?(@.title) =~ "Book"].title',
    "$..title",
]

# Benchmark
for query in queries:
    start = time.perf_counter()
    expr = parse(query)
    for x in range(1000):
        result = expr.find(data)
    elapsed = time.perf_counter() - start
    print(f"jsonpath-ng Query: {query}")
    print(f"jsonpath-ng Results: {len(result)}, Time: {elapsed:.6f} seconds\n")

    start = time.perf_counter()
    cp = csonpath.CsonPath(query)
    for x in range(1000):
        result = cp.find_all(data)
    elapsed = time.perf_counter() - start
    print(f"csonpath python Query: {query}")
    print(f"csonpath python Results: {len(result)}, Time: {elapsed:.6f} seconds\n")

    start = time.perf_counter()
    for x in range(1000):
        if query == "$.store.book[?(@.price) > 20].title":
            result = []
            books = data["store"]["book"]
            for b in books:
                if b["price"] > 20:
                    result.append(b["title"])
        elif query == '$.store.book[?(@.title) =~ "Book"].title':
            result = []
            reg = re.compile("Book")
            books = data["store"]["book"]
            for b in books:
                if reg.search(b["title"]):
                    result.append(b["title"])
        elif query == "$..title":
            result = []
            def rec_get_title(d, out):
                if isinstance(d, dict):
                    for dk,dd in d.items():
                        if dk == "title":
                            out.append(dd)
                        rec_get_title(dd, out)
                elif isinstance(d, list):
                    for l in d:
                        rec_get_title(l, out)
            rec_get_title(data, result)
        elif query == "$.store.book[*].title":
            result = []
            books = data["store"]["book"]
            for b in books:
                result.append(b["title"])
    elapsed = time.perf_counter() - start
    print(f"pure python Query: {query}")
    print(f"pure python Results: {len(result)}, Time: {elapsed:.6f} seconds\n")
