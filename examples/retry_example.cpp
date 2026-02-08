#include <iostream>
#include <chrono>
#include <asio.hpp>
#include "coro_http/coro_http_client.hpp"

using namespace coro_http;

asio::awaitable<void> async_retry_demo() {
    std::cout << "=== Asynchronous Retry Demo ===\n\n";
    
    ClientConfig config;
    config.enable_retry = true;
    config.max_retries = 2;  // Reduced for faster demo
    config.initial_retry_delay = std::chrono::milliseconds(500);
    config.retry_backoff_factor = 2.0;
    config.retry_on_timeout = true;
    config.connect_timeout = std::chrono::milliseconds(1000);  // Shorter timeout
    config.read_timeout = std::chrono::milliseconds(1000);
    
    auto executor = co_await asio::this_coro::executor;
    asio::io_context& io_ctx = static_cast<asio::io_context&>(executor.context());
    CoroHttpClient client(io_ctx, config);
    
    std::cout << "Testing async retry with short timeout...\n";
    std::cout << "Requesting httpbin.org/delay/5 with 1s timeout...\n";
    std::cout << "(This should timeout and retry)\n\n";
    
    auto start = std::chrono::steady_clock::now();
    
    try {
        // This will timeout because server delays 5 seconds but we only wait 1 second
        auto response = co_await client.co_get("http://httpbin.org/delay/5");
        std::cout << "Unexpected success: " << response.status_code() << "\n";
    } catch (const std::exception& e) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Failed after retries (expected): " << e.what() << "\n";
        std::cout << "Total time: " << duration.count() << "ms\n";
        std::cout << "Expected: ~" << (config.max_retries + 1) * 1000 + 500 + 1000 << "ms ";
        std::cout << "(" << (config.max_retries + 1) << " timeouts + retry delays)\n\n";
    }
    
    // Test with a successful quick request
    std::cout << "Testing successful request (no retry needed)...\n";
    try {
        auto response = co_await client.co_get("http://httpbin.org/uuid");
        std::cout << "Success on first try: " << response.status_code() << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n";
    }
}

void production_example() {
    std::cout << "=== Production Configuration Example ===\n\n";
    
    asio::io_context io_ctx;
    ClientConfig config;
    // Connection pool for performance
    config.enable_connection_pool = true;
    config.max_connections_per_host = 5;
    
    // Rate limiting to respect API limits
    config.enable_rate_limit = true;
    config.rate_limit_requests = 10;
    config.rate_limit_window = std::chrono::seconds(1);
    
    // Automatic retry for reliability
    config.enable_retry = true;
    config.max_retries = 3;
    config.initial_retry_delay = std::chrono::milliseconds(1000);
    config.retry_on_timeout = true;
    config.retry_on_connection_error = true;
    config.retry_on_5xx = false;  // Only retry transient errors
    
    // Reasonable timeouts
    config.connect_timeout = std::chrono::milliseconds(5000);
    config.read_timeout = std::chrono::milliseconds(10000);
    
    CoroHttpClient client(io_ctx, config);
    
    std::cout << "Configuration:\n";
    std::cout << "  - Connection pooling: enabled\n";
    std::cout << "  - Rate limiting: 10 req/s\n";
    std::cout << "  - Auto retry: up to 3 attempts\n";
    std::cout << "  - Retry on: timeouts, connection errors\n\n";
    
    client.run([&]() -> asio::awaitable<void> {
        std::cout << "Making reliable API calls...\n\n";
        
        for (int i = 1; i <= 3; ++i) {
            try {
                std::cout << "Request " << i << "... ";
                auto response = co_await client.co_get("http://httpbin.org/uuid");
                std::cout << "OK (" << response.status_code() << ")\n";
            } catch (const std::exception& e) {
                std::cout << "Failed: " << e.what() << "\n";
            }
        }
        
        std::cout << "\nAll requests completed with automatic retry protection!\n\n";
    });
}

int main() {
    std::cout << "HTTP Client Retry Examples\n";
    std::cout << "===========================\n\n";
    
    std::cout << "This demo shows automatic retry with exponential backoff:\n";
    std::cout << "1. Retrying on timeout errors (async)\n";
    std::cout << "2. Production-ready configuration\n\n";
    
    std::cout << "Note: These tests make real HTTP requests to httpbin.org\n";
    std::cout << "      Some tests intentionally trigger timeouts to demonstrate retry.\n";
    std::cout << "      Total runtime: ~10-15 seconds\n\n";
    std::cout << "Press Ctrl+C to stop at any time.\n\n";
    
    // Run async demos
    asio::io_context io_ctx;
    asio::co_spawn(io_ctx, async_retry_demo(), asio::detached);
    
    // Run production example
    production_example();
    
    std::cout << "All demos completed successfully!\n";
    
    return 0;
}
