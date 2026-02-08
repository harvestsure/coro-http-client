# API Reference

## Coroutine API (CoroHttpClient)

### Basic Methods

```cpp
asio::io_context io_ctx;
coro_http::CoroHttpClient client(io_ctx);

client.run([&]() -> asio::awaitable<void> {
    // GET request
    auto response = co_await client.co_get(const std::string& url);
    auto response = co_await client.co_get(const HttpRequest& request);
    
    // POST request
    auto response = co_await client.co_post(const std::string& url, const std::string& body);
    auto response = co_await client.co_post(const HttpRequest& request);
    
    // PUT, DELETE, HEAD, PATCH similar pattern
    auto response = co_await client.co_put(...);
    auto response = co_await client.co_delete_(...);
    auto response = co_await client.co_head(...);
    auto response = co_await client.co_patch(...);
});
```

### SSE Streaming

```cpp
client.run([&client]() -> asio::awaitable<void> {
    coro_http::HttpRequest request(coro_http::HttpMethod::GET, "https://example.com/events");
    request.add_header("Accept", "text/event-stream");
    
    // Stream events with callback
    co_await client.co_stream_events(request, [](const coro_http::SseEvent& event) {
        std::cout << "Event: " << event.type << "\n";
        std::cout << "Data: " << event.data << "\n";
    });
});
```

## HttpResponse

```cpp
class HttpResponse {
    // Get HTTP status code
    int status_code() const;
    
    // Get response body
    const std::string& body() const;
    
    // Get response headers
    const std::map<std::string, std::string>& headers() const;
    
    // Get header value
    std::string get_header(const std::string& name) const;
};
```

## SseEvent

```cpp
struct SseEvent {
    std::string type;                          // Event type (from "event:" field)
    std::string data;                          // Event data (combined from "data:" lines)
    std::string id;                            // Event ID (from "id:" field)
    std::string retry;                         // Retry timing in ms (from "retry:" field)
    std::map<std::string, std::string> fields; // Custom fields
};
```

## Configuration

```cpp
coro_http::ClientConfig config;
config.connect_timeout = std::chrono::seconds(10);
config.read_timeout = std::chrono::seconds(30);
config.request_timeout = std::chrono::seconds(60);
config.enable_compression = true;
config.verify_ssl = true;
config.enable_cookies = true;

client.set_config(config);
```
