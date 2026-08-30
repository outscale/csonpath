import json
import time
import re
import os
from jsonpath_ng.ext import parse
import csonpath

# Create a large test dataset
def build_book(i):
    deep_obj = {
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
    }
    deep_mixed = {
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
    return {
        "category": "fiction",
        "title": f"Book {i}",
        "price": i % 50 + 5,
        "deep_obj": deep_obj,
        "deep_mixed": deep_mixed,
    }

data = {
    "store": {
        "book": [build_book(i) for i in range(5000)],
        "bicycle": {"color": "red", "price": 19.95},
    }
}

# JSONPath queries to benchmark
queries = [
    "$.store.book[?(@.price) > 20].title",
    "$.store.book[*].title",
    "$.store.book[*]['title','category']",
    '$.store.book[?(@.title) =~ "Book"].title',
    "$..title",
    "$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l",
    "$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l",
]

bench_csv_mode = os.environ.get("BENCH_CSV") is not None
bench_csv_header = os.environ.get("BENCH_CSV_HEADER") is not None

def csv_header():
    if bench_csv_header:
        print("category,query,results,time")

def csv_row(category, query, results, time):
    if bench_csv_mode:
        escaped = query.replace('"', '""')
        print(f'{category},"{escaped}",{results},{time:.6f}')

def csv_total(category, total):
    if bench_csv_mode:
        print(f'{category},TOTAL,0,{total:.6f}')

def ng_iters_scale(query):
    if query in ("$..title",
                 "$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l",
                 "$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l"):
        return 50, 20
    return 100, 10

csv_ng = []
csv_csonpath = []
csv_pure = []

csv_header()

total_jsonpath_ng = 0.0
total_csonpath_python = 0.0
total_pure_python = 0.0

# Benchmark
for query in queries:
    expr = parse(query)
    iters, scale = ng_iters_scale(query)
    start = time.perf_counter()
    for x in range(iters):
        result = expr.find(data)
    elapsed = (time.perf_counter() - start) * scale
    total_jsonpath_ng += elapsed
    if bench_csv_mode:
        csv_ng.append((query, len(result), elapsed))
    else:
        print(f"jsonpath-ng Query: {query}")
        print(f"jsonpath-ng Results: {len(result)}, Time: {elapsed:.6f} seconds\n")

    start = time.perf_counter()
    cp = csonpath.CsonPath(query)
    iters = 100
    scale = 10
    for x in range(iters):
        result = cp.find_all(data)
    elapsed = (time.perf_counter() - start) * scale
    total_csonpath_python += elapsed
    if bench_csv_mode:
        csv_csonpath.append((query, len(result), elapsed))
    else:
        print(f"csonpath python Query: {query}")
        print(f"csonpath python Results: {len(result)}, Time: {elapsed:.6f} seconds\n")

    start = time.perf_counter()
    iters = 100
    scale = 10
    for x in range(iters):
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
        elif query == "$.store.book[*]['title','category']":
            result = []
            books = data["store"]["book"]
            for b in books:
                result.append(b["title"])
                result.append(b["category"])
        elif query == "$.store.book[*].title":
            result = []
            books = data["store"]["book"]
            for b in books:
                result.append(b["title"])
        elif query == "$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l":
            result = []
            books = data["store"]["book"]
            for b in books:
                result.append(b["deep_obj"]["a"]["b"]["c"]["d"]["e"]["f"]["g"]["h"]["i"]["j"]["k"]["l"])
        elif query == "$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l":
            result = []
            books = data["store"]["book"]
            for b in books:
                result.append(b["deep_mixed"]["a"]["b"]["c"][0]["d"]["e"]["f"][0]["g"]["h"]["i"][0]["j"]["k"]["l"])
    elapsed = (time.perf_counter() - start) * scale
    total_pure_python += elapsed
    if bench_csv_mode:
        csv_pure.append((query, len(result), elapsed))
    else:
        print(f"pure python Query: {query}")
        print(f"pure python Results: {len(result)}, Time: {elapsed:.6f} seconds\n")

if bench_csv_mode:
    for query, results, elapsed in csv_ng:
        csv_row("jsonpath-ng", query, results, elapsed)
    csv_total("jsonpath-ng", total_jsonpath_ng)

    for query, results, elapsed in csv_csonpath:
        csv_row("csonpath-python", query, results, elapsed)
    csv_total("csonpath-python", total_csonpath_python)

    for query, results, elapsed in csv_pure:
        csv_row("pure-python", query, results, elapsed)
    csv_total("pure-python", total_pure_python)
