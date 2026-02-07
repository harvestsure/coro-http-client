# coro_http

A modern C++20 HTTP client library based on [ASIO](https://github.com/chriskohlhoff/asio), supporting both synchronous and coroutine-based interfaces.

## Features

- ✅ **HTTP/HTTPS Support** - Seamless HTTP and HTTPS requests with SSL/TLS
- ✅ **C++20 Coroutines** - Modern async/await syntax for non-blocking operations
- ✅ **Synchronous API** - Traditional blocking API for simple use cases
- ✅ **Complete HTTP Methods** - GET, POST, PUT, DELETE, HEAD, PATCH, OPTIONS
- ✅ **Header-Only** - Easy integration, no compilation required
- ✅ **Custom Headers** - Easy-to-use builder pattern for request customization
- ✅ **Auto Dependency** - CMake FetchContent automatically downloads ASIO

## Requirements

- C++20 compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.20+
- OpenSSL (for HTTPS support)

## Installation

### Using CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    coro_http
    GIT_REPOSITORY https://github.com/yourusername/coro_http.git
    GIT_TAG main
)

FetchContent_MakeAvailable(coro_http)

target_link_libraries(your_target PRIVATE coro_http)
```

### Manual Build

```bash
git clone https://github.com/yourusername/coro_http.git
cd coro_http
mkdir build && cd build
cmake ..
make
```

## Quick Start

### Synchronous API

```cpp
#include <coro_http/coro_http.hpp>
#include <iostream>

int main() {
    coro_http::HttpClient client;
    
    // Simple GET request
    auto response = client.get("https://api.github.com/users/github");
    std::cout << "Status: " << response.status_code() << "\n";
    std::cout << "Body: " << response.body() << "\n";
    
    // POST with body
    auto post_resp = client.post(
        "https://httpbin.org/post",
        R"({"name": "test", "value": 123})"
    );
    
    return 0;
}
```

### Coroutine API

```cpp
#include <coro_http/coro_http.hpp>
#include <iostream>

asio::awaitable<void> async_main(coro_http::CoroHttpClient& client) {
    // Async GET request with co_await
    auto response = co_await client.co_get("https://api.github.com/users/github");
    std::cout << "Status: " << response.status_code() << "\n";
    
    // Concurrent requests
    auto resp1 = client.co_get("https://httpbin.org/get");
    auto resp2 = client.co_get("https://httpbin.org/ip");
    
    auto r1 = co_await resp1;
    auto r2 = co_await resp2;
}

int main() {
    coro_http::CoroHttpClient client;
    client.run([&]() -> asio::awaitable<void> {
        co_await async_main(client);
    });
    return 0;
}
```

## API Reference

### Synchronous HttpClient

```cpp
coro_http::HttpClient client;

// HTTP Methods
auto resp = client.get(url);
auto resp = client.post(url, body);
auto resp = client.put(url, body);
auto resp = client.del(url);
auto resp = client.head(url);
auto resp = client.patch(url, body);
auto resp = client.options(url);

// Custom request with headers
coro_http::HttpRequest req(coro_http::HttpMethod::GET, url);
req.add_header("Authorization", "Bearer token")
   .add_header("User-Agent", "MyApp/1.0")
   .set_body("request body");
auto resp = client.execute(req);
```

### Coroutine CoroHttpClient

```cpp
coro_http::CoroHttpClient client;

client.run([&]() -> asio::awaitable<void> {
    // All methods return awaitable<HttpResponse>
    auto resp = co_await client.co_get(url);
    auto resp = co_await client.co_post(url, body);
    auto resp = co_await client.co_put(url, body);
    auto resp = co_await client.co_delete(url);
    auto resp = co_await client.co_head(url);
    auto resp = co_await client.co_patch(url, body);
    auto resp = co_await client.co_options(url);
    
    // Custom coroutine request
    coro_http::HttpRequest req(coro_http::HttpMethod::POST, url);
    req.add_header("Content-Type", "application/json");
    auto resp = co_await client.co_execute(req);
});
```

### Response Object

```cpp
class HttpResponse {
    int status_code() const;
    const std::string& reason() const;
    const std::map<std::string, std::string>& headers() const;
    const std::string& body() const;
};
```

## Examples

See the [examples](examples/) directory for complete working examples:

- [sync_example.cpp](examples/sync_example.cpp) - Synchronous API usage
- [coro_example.cpp](examples/coro_example.cpp) - Coroutine API usage
- [https_example.cpp](examples/https_example.cpp) - HTTPS requests demonstration

## License

MIT License

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
