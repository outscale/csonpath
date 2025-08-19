#include <iostream>
#include <chrono>
#include <iomanip>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>

using namespace jsoncons;

// Return current time in seconds, like Python's perf_counter()
double now_seconds() {
    auto tp = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(tp).count();
}

int main() {
    // Generate dataset
    json books = json::array();
    for (int i = 0; i < 5000; ++i) {
        json book = json::object();
        book.insert_or_assign("category", "fiction");
        book.insert_or_assign("title", "Book " + std::to_string(i));
        book.insert_or_assign("price", (i % 50) + 5);
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
        "$.store.book[*].title",
	"$..title"
    };

    for (const auto& query : queries) {
        double start = now_seconds();

        // Execute JSONPath query with JsonCons
	size_t count;
	for (int j = 0; j < 1000; ++j) {
	  json result = jsonpath::json_query(data, query);
	  count = result.is_array() ? result.size() : 0;
	}

        double elapsed = now_seconds() - start;

        std::cout << "Query: " << query << "\n"
                  << "Results: " << count << ", Time: "
                  << std::fixed << std::setprecision(6) << elapsed << " seconds\n\n";
    }
}
