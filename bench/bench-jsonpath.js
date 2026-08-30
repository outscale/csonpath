import { JSONPath } from 'jsonpath-plus';
import { performance } from 'perf_hooks';

// Create large test dataset
const books = [];
for (let i = 0; i < 5000; i++) {
    books.push({
	category: 'fiction',
	title: `Book ${i}`,
	price: (i % 50) + 5,
	deep_obj: { a: { b: { c: { d: { e: { f: { g: { h: { i: { j: { k: { l: 42 } } } } } } } } } } } },
	deep_mixed: { a: { b: { c: [{ d: { e: { f: [{ g: { h: { i: [{ j: { k: { l: 42 } } }] } } }] } } }] } } }
    });
}

const data = {
    store: {
	book: books,
	bicycle: { color: 'red', price: 19.95 }
    }
};

const queries = [
    '$.store.book[*].title',
    '$..title',
    '$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l',
    '$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l',
    '$.store.book[?(@.price > 20)].title',
    '$.store.book[?(@.title.match(/Book/))].title',
];

const benchCsvMode = process.env.BENCH_CSV !== undefined;
const benchCsvHeader = process.env.BENCH_CSV_HEADER !== undefined;

if (benchCsvHeader) {
    console.log("category,query,results,time");
}

let total = 0.0;
const iters = 100;
const scale = 10.0;

// Benchmark
for (const query of queries) {
    const start = performance.now();
    let result;
    for (let i = 0; i < iters; ++i)
	result = JSONPath({ path: query, json: data });
    const elapsed = (performance.now() - start) / 1000 * scale; // seconds
    total += elapsed;
    if (benchCsvMode) {
        const escaped = query.replace(/"/g, '""');
        console.log(`jsonpath-plus,"${escaped}",${result.length},${elapsed.toFixed(6)}`);
    } else {
        console.log(`Query: ${query}`);
        console.log(`Results: ${result.length}, Time: ${elapsed.toFixed(6)} seconds\n`);
    }
}

if (benchCsvMode) {
    console.log(`jsonpath-plus,TOTAL,0,${total.toFixed(6)}`);
}
