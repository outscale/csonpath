#include <iostream>
#include <chrono>
#include <iomanip>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>

using namespace jsoncons;

static int bench_csv_mode = 0;

// Return current time in seconds, like Python's perf_counter()
double now_seconds() {
    auto tp = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(tp).count();
}

static void csv_header() {
    if (std::getenv("BENCH_CSV_HEADER"))
        std::cout << "category,query,results,time\n";
}

static std::string csv_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    return out;
}

static void csv_row(const std::string& category, const std::string& query, size_t results, double time) {
    if (bench_csv_mode)
        std::cout << category << ",\"" << csv_escape(query) << "\"," << results << "," << std::fixed << std::setprecision(6) << time << "\n";
}

static void csv_total(const std::string& category, double total) {
    if (bench_csv_mode)
        std::cout << category << ",TOTAL,0," << std::fixed << std::setprecision(6) << total << "\n";
}

json build_deep_obj() {
    json cur = 42;
    for (char c = 'l'; c >= 'a'; --c) {
        json wrap = json::object();
        std::string key(1, c);
        wrap.insert_or_assign(key, cur);
        cur = wrap;
    }
    return cur;
}

json build_deep_mixed() {
    json cur = 42;
    for (char c = 'l'; c >= 'a'; --c) {
        if ((c - 'a' + 1) % 3 == 0 && c != 'l') {
            json arr = json::array();
            arr.push_back(cur);
            cur = arr;
        }
        json wrap = json::object();
        std::string key(1, c);
        wrap.insert_or_assign(key, cur);
        cur = wrap;
    }
    return cur;
}

int main() {
    bench_csv_mode = std::getenv("BENCH_CSV") != nullptr;

    json deep_obj_proto = build_deep_obj();
    json deep_mixed_proto = build_deep_mixed();

    // Generate dataset
    json books = json::array();
    for (int i = 0; i < 5000; ++i) {
        json book = json::object();
        book.insert_or_assign("category", "fiction");
        book.insert_or_assign("title", "Book " + std::to_string(i));
        book.insert_or_assign("price", (i % 50) + 5);
        book.insert_or_assign("deep_obj", deep_obj_proto);
        book.insert_or_assign("deep_mixed", deep_mixed_proto);
        books.push_back(book);
    }

    json bicycle = json::object();
    bicycle.insert_or_assign("color", "red");
    bicycle.insert_or_assign("price", 19.95);

    json store = json::object();
    store.insert_or_assign("book", books);
    store.insert_or_assign("bicycle", bicycle);

    json data = json::object();
    data.insert_or_assign("store", store);

    std::vector<std::string> queries = {
	"$.store.book[?(@.price > 20)].title",
        "$.store.book[*].title",
        "$.store.book[?(@.title =~ /Book/)].title",
        "$.store.book[*]['title','category']",
	"$..title",
        "$.store.book[*].deep_obj.a.b.c.d.e.f.g.h.i.j.k.l",
        "$.store.book[*].deep_mixed.a.b.c[0].d.e.f[0].g.h.i[0].j.k.l"
    };

    csv_header();
    double total = 0.0;
    const int iters = 250;
    const double scale = 4.0;

    for (const auto& query : queries) {
        double start = now_seconds();

        // Execute JSONPath query with JsonCons
	size_t count;
	for (int j = 0; j < iters; ++j) {
	  json result = jsonpath::json_query(data, query);
	  count = result.is_array() ? result.size() : 0;
	}

        double elapsed = now_seconds() - start;
        total += elapsed * scale;

        if (bench_csv_mode) {
            csv_row("jsoncons", query, count, elapsed * scale);
        } else {
            std::cout << "Query: " << query << "\n"
                      << "Results: " << count << ", in " << iters << " loop, Time: "
                      << std::fixed << std::setprecision(6) << elapsed << " seconds\n\n";
        }
    }

    csv_total("jsoncons", total);
}
