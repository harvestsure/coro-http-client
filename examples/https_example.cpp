#include <coro_http/coro_http.hpp>
#include <iostream>
#include <asio.hpp>

using namespace coro_http;

int main() {
    try {
        asio::io_context io_ctx;
        CoroHttpClient client(io_ctx);

        client.run([&]() -> asio::awaitable<void> {
            auto response = co_await client.co_get("https://httpbin.org/get");
            std::cout << "GET Status: " << response.status_code() << "\n";
            std::cout << "GET Body: " << response.body().substr(0, 200) << "...\n\n";

            auto post_response = co_await client.co_post(
                "https://httpbin.org/post",
                R"({"name": "test", "value": 123})"
            );
            std::cout << "POST Status: " << post_response.status_code() << "\n\n";

            auto put_response = co_await client.co_put(
                "https://httpbin.org/put",
                R"({"updated": true})"
            );
            std::cout << "PUT Status: " << put_response.status_code() << "\n\n";

            auto delete_response = co_await client.co_delete("https://httpbin.org/delete");
            std::cout << "DELETE Status: " << delete_response.status_code() << "\n\n";

            auto head_response = co_await client.co_head("https://httpbin.org/get");
            std::cout << "HEAD Status: " << head_response.status_code() << "\n\n";

            auto patch_response = co_await client.co_patch(
                "https://httpbin.org/patch",
                R"({"patched": true})"
            );
            std::cout << "PATCH Status: " << patch_response.status_code() << "\n\n";

            auto options_response = co_await client.co_options("https://httpbin.org/get");
            std::cout << "OPTIONS Status: " << options_response.status_code() << "\n";
        });

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
