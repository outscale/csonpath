import { JSONPath } from 'jsonpath-plus';
import { performance } from 'perf_hooks';

// Create large test dataset
const books = [];
for (let i = 0; i < 5000; i++) {
    books.push({
	category: 'fiction',
	title: `Book ${i}`,
	price: (i % 50) + 5
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
];

// Benchmark
for (const query of queries) {
    const start = performance.now();
    let result;
    for (let i = 0; i < 100; ++i)
	result = JSONPath({ path: query, json: data });
    const elapsed = (performance.now() - start) / 1000; // seconds
    console.log(`Query: ${query}`);
    console.log(`Results: ${result.length}, Time: ${elapsed.toFixed(6)} seconds\n`);
}
